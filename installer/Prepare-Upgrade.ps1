param([string]$InstanceSuffix='', [string]$WindowProperty='Hhub.UI.MainWindow', [string]$ApplicationDirectory)
$ErrorActionPreference='Stop'
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
 try{$taskClient.Connect(1000)}catch [TimeoutException]{exit 0}
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
 $taskPreviousApp=if($taskProcess -and $taskProcess.ProcessName -eq 'Hhub.Core'){Split-Path -Parent $taskProcess.Path}else{$null}
 $taskClient.WriteByte(1);$taskClient.Flush()
 if(-not $taskReply.ok){throw 'Hhub 核心拒绝退出。'}
 if($taskProcess -and $taskProcess.ProcessName -eq 'Hhub.Core' -and -not $taskProcess.WaitForExit(10000)){throw 'Hhub 核心尚未退出。'}
 # Copy verified component files before moving the live profile between roots.
 $taskSourceData=[string]$taskReply.profileDirectory
 $taskProfile=if($InstanceSuffix -eq '.PhoneTests'){'HhubPhoneTests'}else{'HhubPrototype'}
 $taskTargetData=Join-Path $env:LOCALAPPDATA $taskProfile
 if($taskSourceData -and [IO.Path]::GetFullPath($taskSourceData) -ne [IO.Path]::GetFullPath($taskTargetData)){
  foreach($taskPlugin in $taskReply.state.plugins){
   if(-not $taskPlugin.installed){continue}
   $taskAllowed=switch($taskPlugin.id){'windows' {@('Hhub.Windows.exe')} 'phone' {@('Hhub.Phone.exe','phone-web/app.js','phone-web/index.html','phone-web/style.css')} 'eq' {@('HhubAudioDriverSetup.exe')} default {@()}}
   foreach($taskAsset in $taskPlugin.nativeAssets){
    if($taskAsset.path -notin $taskAllowed -or $taskAsset.sha256 -notmatch '^[0-9a-f]{64}$'){throw 'Invalid native component metadata'}
    $taskFrom=Join-Path (Join-Path (Join-Path $taskSourceData 'native-components') $taskPlugin.id) $taskAsset.path
    $taskTo=Join-Path (Join-Path (Join-Path $taskTargetData 'native-components') $taskPlugin.id) $taskAsset.path
    if(-not [IO.File]::Exists($taskFrom)){throw 'Installed native component is missing'}
    if((Get-Item -LiteralPath $taskFrom).Length -gt 4194304 -or (Get-FileHash -LiteralPath $taskFrom -Algorithm SHA256).Hash -ne $taskAsset.sha256){throw 'Installed native component hash mismatch'}
    if(-not [IO.File]::Exists($taskTo)){
     [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($taskTo));[IO.File]::Copy($taskFrom,$taskTo,$false)
    }elseif((Get-FileHash -LiteralPath $taskTo -Algorithm SHA256).Hash -ne $taskAsset.sha256){throw 'Destination has a different native component; migration stopped'}
   }
  }
 }
 # A portable copy can have redirected AppData. Preserve its live preferences
 # in the installer's profile, with an atomic replacement and a local backup.
 $taskState=$taskReply.state
 if($taskState.schema -ne 1 -or -not $taskState.settings -or $null -eq $taskState.plugins){throw 'Hhub 配置格式无效，安装已停止。'}
 foreach($taskKey in @('phoneDeviceControls','deviceProfileOverrides','profileRuntime','monitor','detected','windowsSettings','eqRuntime','phoneRuntime','applications','updates')){$taskState.PSObject.Properties.Remove($taskKey)}
 foreach($taskPlugin in $taskState.plugins){$taskPlugin|Add-Member -NotePropertyName runtime -NotePropertyValue 'stopped' -Force;$taskPlugin|Add-Member -NotePropertyName leaseUntil -NotePropertyValue 0 -Force}
 $taskProfile=if($InstanceSuffix -eq '.PhoneTests'){'HhubPhoneTests'}else{'HhubPrototype'}
 $taskData=Join-Path $env:LOCALAPPDATA $taskProfile
 [void][IO.Directory]::CreateDirectory($taskData)
 $taskFile=Join-Path $taskData 'state.json'
 $taskTemporary=Join-Path $taskData ('install-'+[guid]::NewGuid().ToString('N')+'.tmp')
 [IO.File]::WriteAllText($taskTemporary,($taskState|ConvertTo-Json -Depth 100 -Compress),[Text.UTF8Encoding]::new($false))
 if([IO.File]::Exists($taskFile)){
  $taskBackups=Join-Path $taskData 'backups';[void][IO.Directory]::CreateDirectory($taskBackups)
  [IO.File]::Replace($taskTemporary,$taskFile,(Join-Path $taskBackups ('before-base-install-'+[guid]::NewGuid().ToString('N')+'.json')),$true)
 }else{[IO.File]::Move($taskTemporary,$taskFile)}
 # Carry forward only components of plugins the user already installed.
 # Setup does not bundle or download these optional components.
 if($ApplicationDirectory -and $taskPreviousApp){
  $taskDestination=[IO.Path]::GetFullPath($ApplicationDirectory)
  foreach($taskPlugin in $taskState.plugins){
   if(-not $taskPlugin.installed){continue}
   $taskComponents=switch($taskPlugin.id){
    'windows' {@('Hhub.Windows.exe')}
    'phone' {@('Hhub.Phone.exe','phone-web\app.js','phone-web\index.html','phone-web\style.css')}
    'eq' {@('Hhub.AudioTest.exe','drivers\audio\fxvad.inf','drivers\audio\fxvad.sys','drivers\audio\fxvadntamd64.cat','drivers\audio\HhubAudioDriverSetup.exe')}
    default {@()}
   }
   foreach($taskRelative in $taskComponents){
    $taskFrom=Join-Path $taskPreviousApp $taskRelative;$taskTo=Join-Path $taskDestination $taskRelative
    if([IO.File]::Exists($taskFrom) -and -not [IO.File]::Exists($taskTo)){
     [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($taskTo));[IO.File]::Copy($taskFrom,$taskTo,$false)
    }
   }
  }
 }
} finally {$taskClient.Dispose()}
