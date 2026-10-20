param([string]$InstanceSuffix='', [string]$WindowProperty='Hhub.UI.MainWindow', [string]$ApplicationDirectory, [string]$LogPath=(Join-Path $env:TEMP 'Hhub-prepare-upgrade.log'), [switch]$StopOnly)
$ErrorActionPreference='Stop'
function Write-UpgradeLog([string]$Message) {
 [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($LogPath)))
 [IO.File]::AppendAllText($LogPath,([DateTime]::Now.ToString('s')+' '+$Message+[Environment]::NewLine),[Text.UTF8Encoding]::new($false))
}
trap {
 try { Write-UpgradeLog ('FAILED: '+$_.Exception.Message+' at '+$_.ScriptStackTrace) } catch {}
 Write-Error -Message $_.Exception.Message -ErrorAction Continue
 exit 1
}
Write-UpgradeLog 'Starting profile preparation'

# Stop the current user's existing instance even when it runs from a portable
# or development directory. Keep plugin installation records and preferences.
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class HhubUpgradeWindows {
 public delegate bool Callback(IntPtr hwnd,IntPtr context);
 [DllImport("user32.dll")] public static extern bool EnumWindows(Callback callback,IntPtr context);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr GetProp(IntPtr hwnd,string name);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint id);
 [DllImport("user32.dll",SetLastError=true)] public static extern IntPtr SendMessageTimeout(IntPtr hwnd,uint message,UIntPtr wParam,IntPtr lParam,uint flags,uint timeout,out UIntPtr result);
}
'@
$taskWindows=[Collections.Generic.List[IntPtr]]::new()
[void][HhubUpgradeWindows]::EnumWindows({param($taskWindow,$taskContext)
 if([HhubUpgradeWindows]::GetProp($taskWindow,$WindowProperty) -ne [IntPtr]::Zero){$taskWindows.Add($taskWindow)}
 return $true
},[IntPtr]::Zero)
foreach($taskWindow in $taskWindows){
 [uint32]$taskOwner=0;[void][HhubUpgradeWindows]::GetWindowThreadProcessId($taskWindow,[ref]$taskOwner)
 $taskProcess=Get-Process -Id $taskOwner -ErrorAction SilentlyContinue
 if($taskProcess -and $taskProcess.ProcessName -eq 'Hhub'){
  $taskResult=[UIntPtr]::Zero
  [void][HhubUpgradeWindows]::SendMessageTimeout($taskWindow,0x10,[UIntPtr]::Zero,[IntPtr]::Zero,2,3000,[ref]$taskResult)
  if(-not $taskProcess.WaitForExit(10000)){throw 'Hhub 界面未退出，请关闭后重新安装。'}
 }
}
$taskPipe='HhubPrototype.v1.'+[Security.Principal.WindowsIdentity]::GetCurrent().User.Value+$InstanceSuffix
$taskClient=[IO.Pipes.NamedPipeClientStream]::new('.',$taskPipe,[IO.Pipes.PipeDirection]::InOut)
try {
 try{$taskClient.Connect(2000)}catch [TimeoutException]{Write-UpgradeLog 'No running core; existing profile left intact'; exit 0}
 $taskPayload=[Text.Encoding]::UTF8.GetBytes('{"op":"quit","body":{}}')
 $taskPrefix=[BitConverter]::GetBytes([int]$taskPayload.Length)
 $taskClient.Write($taskPrefix,0,4);$taskClient.Write($taskPayload,0,$taskPayload.Length);$taskClient.Flush()
 function Read-Exact([byte[]]$Buffer){
  $taskOffset=0
  while($taskOffset -lt $Buffer.Length){
   $taskRead=$taskClient.ReadAsync($Buffer,$taskOffset,$Buffer.Length-$taskOffset)
   if(-not $taskRead.Wait(6000)){throw 'Hhub 核心退出响应超时。'}
   if($taskRead.Result -le 0){throw 'Hhub 核心响应不完整。'}
   $taskOffset+=$taskRead.Result
  }
 }
 $taskPrefix=New-Object byte[] 4;Read-Exact $taskPrefix
 $taskSize=[BitConverter]::ToInt32($taskPrefix,0)
 if($taskSize -lt 1 -or $taskSize -gt 33554432){throw 'Hhub 核心响应大小无效。'}
 $taskBytes=New-Object byte[] $taskSize;Read-Exact $taskBytes
 $taskReply=[Text.Encoding]::UTF8.GetString($taskBytes)|ConvertFrom-Json
 $taskProcess=Get-Process -Id ([int]$taskReply.state.corePid) -ErrorAction SilentlyContinue
 $taskIsCore=$taskProcess -and ($taskProcess.ProcessName -eq 'Hhub.Core' -or ($taskProcess.ProcessName -eq 'Hhub' -and (Split-Path -Leaf (Split-Path -Parent $taskProcess.Path)) -eq 'core'))
 $taskPreviousApp=if($taskIsCore){$folder=Split-Path -Parent $taskProcess.Path;if((Split-Path -Leaf $folder) -eq 'core'){Split-Path -Parent $folder}else{$folder}}else{$null}
 $taskClient.WriteByte(1);$taskClient.Flush()
 if(-not $taskReply.ok){throw 'Hhub 核心拒绝退出。'}
 if($taskIsCore -and -not $taskProcess.WaitForExit(10000)){throw 'Hhub 核心尚未退出。'}
 Write-UpgradeLog 'Core and UI stopped'
 if($StopOnly){Write-UpgradeLog 'Stop-only completed'; return}
 # Copy verified component files before moving the live profile between roots.
 $taskSourceData=[string]$taskReply.profileDirectory
 $taskProfile='HhubPrototype'
 $taskTargetData=Join-Path $env:LOCALAPPDATA $taskProfile
 # The core has already saved its state during quit. Do not replace an existing
 # profile with a transient snapshot when installation uses that same profile.
 if(-not $taskSourceData -or [IO.Path]::GetFullPath($taskSourceData) -eq [IO.Path]::GetFullPath($taskTargetData)){
  Write-UpgradeLog 'Same profile (or legacy core without profile path); no migration required'; return
 }
 Write-UpgradeLog ('Migrating profile from '+$taskSourceData+' to '+$taskTargetData)
 function Copy-MigratedFile([string]$From,[string]$To) {
  [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($To))
  if([IO.File]::Exists($To)){
   if((Get-FileHash -LiteralPath $From).Hash -eq (Get-FileHash -LiteralPath $To).Hash){return}
   $backup=Join-Path $taskTargetData ('backups/upgrade-'+$taskBackupId+'/'+$To.Substring($taskTargetData.Length).TrimStart('\','/'))
   [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($backup))
   [IO.File]::Copy($To,$backup,$false)
  }
  $temporary=$To+'.migration-'+[guid]::NewGuid().ToString('N')
  [IO.File]::Copy($From,$temporary,$false)
  if([IO.File]::Exists($To)){[IO.File]::Replace($temporary,$To,$backup,$true)}else{[IO.File]::Move($temporary,$To)}
 }
 $taskBackupId=[guid]::NewGuid().ToString('N')
 if($taskSourceData -and [IO.Path]::GetFullPath($taskSourceData) -ne [IO.Path]::GetFullPath($taskTargetData)){
  foreach($taskPlugin in $taskReply.state.plugins){
   if(-not $taskPlugin.installed -or $taskPlugin.builtin -or $taskPlugin.id -in @('windows','eq','phone')){continue}
   if($taskPlugin.id -notmatch '^[a-z0-9][a-z0-9._-]{0,79}$' -or $taskPlugin.id.Contains('..') -or $taskPlugin.id.EndsWith('.')){throw 'Invalid plugin ID'}
   foreach($taskAsset in $taskPlugin.nativeAssets){
    if([string]::IsNullOrEmpty($taskAsset.path) -or $taskAsset.path -match '(^/|\\|:|(^|/)\.\.?(/|$)|[<>"|?*]|[. ](/|$))' -or $taskAsset.sha256 -notmatch '^[0-9a-f]{64}$'){throw 'Invalid native component metadata'}
    $taskFrom=Join-Path (Join-Path (Join-Path $taskSourceData 'native-components') $taskPlugin.id) $taskAsset.path
    $taskTo=Join-Path (Join-Path (Join-Path $taskTargetData 'native-components') $taskPlugin.id) $taskAsset.path
    if(-not [IO.File]::Exists($taskFrom)){throw 'Installed native component is missing'}
    if((Get-Item -LiteralPath $taskFrom).Length -gt 4194304 -or (Get-FileHash -LiteralPath $taskFrom -Algorithm SHA256).Hash -ne $taskAsset.sha256){throw 'Installed native component hash mismatch'}
    Copy-MigratedFile $taskFrom $taskTo
   }
  }
 }
 if($taskSourceData -and [IO.Path]::GetFullPath($taskSourceData) -ne [IO.Path]::GetFullPath($taskTargetData)){
  foreach($taskPlugin in $taskReply.state.plugins){
   if(-not $taskPlugin.installed){continue}
   foreach($taskFile in $taskPlugin.storage.migrationFiles){
    if([string]::IsNullOrEmpty($taskFile.path) -or $taskFile.path -match '(^/|\\|:|(^|/)\.\.?(/|$)|[<>"|?*]|[. ](/|$))'){throw 'Invalid plugin storage path'}
    $taskMaximum=[Math]::Min(4194304,[Math]::Max(1,[int64]$taskFile.maxBytes))
    $taskFrom=Join-Path $taskSourceData $taskFile.path;$taskTo=Join-Path $taskTargetData $taskFile.path
    if(-not [IO.File]::Exists($taskFrom)){continue}
    if((Get-Item -LiteralPath $taskFrom).Length -gt $taskMaximum){throw 'Plugin storage file exceeds declared size'}
    if($taskFile.format -eq 'json'){$null=Get-Content -LiteralPath $taskFrom -Raw -Encoding UTF8|ConvertFrom-Json}
    Copy-MigratedFile $taskFrom $taskTo
   }
  }
 }

 # Phone certificates and authorization records belong to the user profile,
 # not to the replaceable built-in phone module.
 $taskPhone=Join-Path $taskSourceData 'phone'
 if(Test-Path -LiteralPath $taskPhone){
  function Copy-PhoneDirectory([string]$Directory){
   $item=Get-Item -LiteralPath $Directory -Force
   if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Phone data contains a redirected directory'}
   foreach($entry in Get-ChildItem -LiteralPath $Directory -Force){
    if($entry.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Phone data contains a link'}
    if($entry.PSIsContainer){Copy-PhoneDirectory $entry.FullName}
    else{
     if($entry.Length -gt 4194304){throw 'Phone profile file exceeds maximum size'}
     Copy-MigratedFile $entry.FullName (Join-Path $taskTargetData $entry.FullName.Substring($taskSourceData.Length).TrimStart('\','/'))
    }
   }
  }
  Copy-PhoneDirectory $taskPhone
 }
 # A portable copy can have redirected AppData. Preserve its live preferences
 # in the installer's profile, with an atomic replacement and a local backup.
 $taskState=$taskReply.state
 if($taskState.schema -ne 1 -or -not $taskState.settings -or $null -eq $taskState.plugins){throw 'Hhub 配置格式无效，安装已停止。'}
 foreach($taskKey in @('phoneDeviceControls','deviceProfileOverrides','profileRuntime','monitor','detected','windowsSettings','eqRuntime','phoneRuntime','applications','updates')){$taskState.PSObject.Properties.Remove($taskKey)}
 foreach($taskPlugin in $taskState.plugins){$taskPlugin|Add-Member -NotePropertyName runtime -NotePropertyValue 'stopped' -Force;$taskPlugin|Add-Member -NotePropertyName leaseUntil -NotePropertyValue 0 -Force}
 $taskProfile='HhubPrototype'
 $taskData=Join-Path $env:LOCALAPPDATA $taskProfile
 [void][IO.Directory]::CreateDirectory($taskData)
 $taskFile=Join-Path $taskData 'state.json'
 $taskTemporary=Join-Path $taskData ('install-'+[guid]::NewGuid().ToString('N')+'.tmp')
 [IO.File]::WriteAllText($taskTemporary,($taskState|ConvertTo-Json -Depth 100 -Compress),[Text.UTF8Encoding]::new($false))
 if([IO.File]::Exists($taskFile)){
  $taskBackups=Join-Path $taskData 'backups';[void][IO.Directory]::CreateDirectory($taskBackups)
  [IO.File]::Replace($taskTemporary,$taskFile,(Join-Path $taskBackups ('before-base-install-'+[guid]::NewGuid().ToString('N')+'.json')),$true)
 }else{[IO.File]::Move($taskTemporary,$taskFile)}
 Write-UpgradeLog 'Profile migration completed; previous files saved under backups'
} finally {$taskClient.Dispose()}

