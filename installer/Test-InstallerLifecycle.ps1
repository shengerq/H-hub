param([Parameter(Mandatory=$true)][string]$TestDirectory)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath($TestDirectory)
[void][IO.Directory]::CreateDirectory($taskRoot)
$taskShell=Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe'
$taskPrepare=Join-Path $PSScriptRoot 'Prepare-Upgrade.ps1'
$taskCleanup=Join-Path $PSScriptRoot 'Remove-UserData.ps1'
$taskLocal=$env:LOCALAPPDATA
$taskChecks=[Collections.Generic.List[object]]::new()
function Check([string]$Name,[bool]$Ok){
 $taskChecks.Add(@{name=$Name;ok=$Ok})
 if(-not $Ok){throw ('Failed: '+$Name)}
}
function Run-Prepare($Reply,[string]$Name){
 $suffix='.'+[guid]::NewGuid().ToString('N')
 $pipeName='HhubPrototype.v1.'+[Security.Principal.WindowsIdentity]::GetCurrent().User.Value+$suffix
 $job=Start-Job -ArgumentList $pipeName,($Reply|ConvertTo-Json -Depth 40 -Compress) -ScriptBlock {
  param($Name,$Reply)
  $server=[IO.Pipes.NamedPipeServerStream]::new($Name,[IO.Pipes.PipeDirection]::InOut)
  try {
   $server.WaitForConnection()
   $reader=[IO.BinaryReader]::new($server)
   $size=$reader.ReadInt32();$null=$reader.ReadBytes($size)
   $writer=[IO.BinaryWriter]::new($server)
   $bytes=[Text.Encoding]::UTF8.GetBytes($Reply)
   $writer.Write([int]$bytes.Length);$writer.Write($bytes);$writer.Flush()
   $null=$reader.ReadByte()
  } finally {$server.Dispose()}
 }
 try {
  Start-Sleep -Milliseconds 900
  & $taskShell -NoProfile -ExecutionPolicy Bypass -File $taskPrepare -InstanceSuffix $suffix -WindowProperty ('Unused'+$suffix) -LogPath (Join-Path $taskRoot ($Name+'.log'))
  $code=$LASTEXITCODE
  $null=Wait-Job $job -Timeout 10
  if($job.State -ne 'Completed'){throw 'Mock core did not complete'}
  Receive-Job $job | Out-Null
  return $code
 }finally {Stop-Job $job;Remove-Job $job}
}
try {
 $env:LOCALAPPDATA=Join-Path $taskRoot 'target'
 $target=Join-Path $env:LOCALAPPDATA 'HhubPrototype'
 [void][IO.Directory]::CreateDirectory($target)
 $original='{"schema":1,"sentinel":"preserve exact bytes"}'
 [IO.File]::WriteAllText((Join-Path $target 'state.json'),$original)
 & $taskShell -NoProfile -ExecutionPolicy Bypass -File $taskPrepare -InstanceSuffix '.AbsentInstallerTest' -WindowProperty 'UnusedInstallerTest' -LogPath (Join-Path $taskRoot 'absent.log')
 Check 'No running core allows installation' ($LASTEXITCODE -eq 0)
 $state=@{schema=1;settings=@{theme='dark'};plugins=@();corePid=$PID}
 $reply=@{ok=$true;state=$state;profileDirectory=$target}
 Check 'Same profile preparation succeeds' ((Run-Prepare $reply 'same') -eq 0)
 Check 'Same profile state is not rewritten' ([IO.File]::ReadAllText((Join-Path $target 'state.json')) -eq $original)
 $reply.Remove('profileDirectory')
 Check 'Legacy core without profile path is supported' ((Run-Prepare $reply 'legacy') -eq 0)
 $source=Join-Path $taskRoot 'source/HhubPrototype'
 [void][IO.Directory]::CreateDirectory((Join-Path $source 'native-components/example'))
 [void][IO.Directory]::CreateDirectory((Join-Path $target 'native-components/example'))
 $sourceAsset=Join-Path $source 'native-components/example/test.dll'
 $targetAsset=Join-Path $target 'native-components/example/test.dll'
 [IO.File]::WriteAllText($sourceAsset,'verified source component')
 [IO.File]::WriteAllText($targetAsset,'previous destination component')
 [IO.File]::WriteAllText((Join-Path $source 'example.json'),'{"value":"source"}')
 [IO.File]::WriteAllText((Join-Path $target 'example.json'),'{"value":"previous"}')
 $state.plugins=@(@{id='example';installed=$true;nativeAssets=@(@{path='test.dll';sha256=(Get-FileHash $sourceAsset).Hash.ToLower()});storage=@{migrationFiles=@(@{path='example.json';format='json';maxBytes=4096})}})
 $state.plugins+=@{id='eq';installed=$true;nativeAssets=@(@{path='missing-old-driver.exe';sha256=('0'*64)})}
 $reply.profileDirectory=$source
 [void][IO.Directory]::CreateDirectory((Join-Path $source 'phone'))
 [IO.File]::WriteAllText((Join-Path $source 'phone/pairing.json'),'{"fixture":"paired-device"}')
 Check 'Different component and plugin data versions migrate' ((Run-Prepare $reply 'collision') -eq 0)
 Check 'Old built-in components replaced by bundled features' (Test-Path (Join-Path $target 'state.json'))
 Check 'Phone pairing files migrate' (Test-Path (Join-Path $target 'phone/pairing.json'))
 Check 'Verified component is used' ([IO.File]::ReadAllText($targetAsset) -eq 'verified source component')
 $backups=@(Get-ChildItem (Join-Path $target 'backups') -Recurse -File)
 Check 'Previous component, plugin data and state backed up' ($backups.Count -eq 3)
 Check 'Personal settings migrate' ((Get-Content (Join-Path $target 'state.json') -Raw|ConvertFrom-Json).settings.theme -eq 'dark')
 $before=(Get-FileHash (Join-Path $target 'state.json')).Hash
 & $taskShell -NoProfile -ExecutionPolicy Bypass -File $taskCleanup -ProfileDirectory $target -LogPath (Join-Path $taskRoot 'keep.log')
 Check 'Default uninstall keeps plugins and data' ($LASTEXITCODE -eq 0 -and (Get-FileHash (Join-Path $target 'state.json')).Hash -eq $before -and (Test-Path $targetAsset))
 & $taskShell -NoProfile -ExecutionPolicy Bypass -File $taskCleanup -RemoveData -ProfileDirectory $taskRoot -LogPath (Join-Path $taskRoot 'invalid.log') 2>$null
 Check 'Cleanup refuses unexpected root' ($LASTEXITCODE -ne 0 -and (Test-Path $target))
 & $taskShell -NoProfile -ExecutionPolicy Bypass -File $taskCleanup -RemoveData -ProfileDirectory $target -LogPath (Join-Path $taskRoot 'remove.log')
 Check 'Explicit clear removes plugins and personal data' ($LASTEXITCODE -eq 0 -and -not (Test-Path $target))
 Check 'Uninstall cleanup leaves other profiles intact' (Test-Path $sourceAsset)
} finally {
 $env:LOCALAPPDATA=$taskLocal
 $report=@{ok=(@($taskChecks|Where-Object {-not $_.ok}).Count -eq 0);checks=@($taskChecks)}
 [IO.File]::WriteAllText((Join-Path $taskRoot 'verification.json'),($report|ConvertTo-Json -Depth 20),[Text.UTF8Encoding]::new($false))
 $report|ConvertTo-Json -Depth 20
}
