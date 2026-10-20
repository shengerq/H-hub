param([Parameter(Mandatory=$true)][string]$ApplicationDirectory,[Parameter(Mandatory=$true)][string]$ProfileDirectory)
$ErrorActionPreference='Stop'
$taskApp=[IO.Path]::GetFullPath($ApplicationDirectory)
$taskProfile=[IO.Path]::GetFullPath($ProfileDirectory)
New-Item -ItemType Directory -Force (Join-Path $taskProfile 'HhubPrototype')|Out-Null
$taskOldLocal=$env:LOCALAPPDATA;$taskOldInstance=$env:HHUB_INSTANCE
$taskSid=[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
$taskResults=[Collections.Generic.List[object]]::new()
function Request([string]$Operation,$Body=@{}){
 $taskPipe=[IO.Pipes.NamedPipeClientStream]::new('.','HhubPrototype.v1.'+$taskSid+'.'+$env:HHUB_INSTANCE,[IO.Pipes.PipeDirection]::InOut)
 try{
  $taskPipe.Connect(20000);$taskWriter=[IO.BinaryWriter]::new($taskPipe,[Text.Encoding]::UTF8,$true);$taskReader=[IO.BinaryReader]::new($taskPipe,[Text.Encoding]::UTF8,$true)
  $taskBytes=[Text.Encoding]::UTF8.GetBytes((@{op=$Operation;body=$Body}|ConvertTo-Json -Depth 50 -Compress))
  $taskWriter.Write([int]$taskBytes.Length);$taskWriter.Write($taskBytes);$taskWriter.Flush()
  $taskSize=$taskReader.ReadInt32();if($taskSize -le 0 -or $taskSize -gt 32MB){throw 'Invalid frame'}
  $taskReply=[Text.Encoding]::UTF8.GetString($taskReader.ReadBytes($taskSize))|ConvertFrom-Json
  $taskWriter.Write([byte]1);$taskWriter.Flush();return $taskReply
 }finally{$taskPipe.Dispose()}
}
function Check([string]$Name,[bool]$Passed,$Evidence){$taskResults.Add(@{name=$Name;ok=$Passed;evidence=$Evidence});if(-not $Passed){Write-Warning $Name}}
try{
 $env:LOCALAPPDATA=$taskProfile;$env:HHUB_INSTANCE='LegacyFeatures-'+[guid]::NewGuid().ToString('N')
 $taskModel=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'Model.h'))
 $taskInitial=[regex]::Match($taskModel,'initialState=LR"JSON\((.*?)\)JSON"',[Text.RegularExpressions.RegexOptions]::Singleline).Groups[1].Value|ConvertFrom-Json
 $taskInitial.settings.autoUpdate=$false;$taskInitial.settings.appAutoUpdate=$false;$taskInitial.settings.appAutoDetect=$false
 [IO.File]::WriteAllText((Join-Path $taskProfile 'HhubPrototype/state.json'),($taskInitial|ConvertTo-Json -Depth 50 -Compress),[Text.UTF8Encoding]::new($false))
 $taskCore=Start-Process -FilePath (Join-Path $taskApp 'core/Hhub.exe') -WorkingDirectory $taskApp -WindowStyle Hidden -PassThru
 $taskSnapshot=Request 'snapshot'
 Check 'Three built-in features available without importing packages' (@($taskSnapshot.state.plugins|Where-Object builtin).Count -eq 3) @($taskSnapshot.state.plugins|Select-Object id,builtin,installed)
 Check 'Host rejects direct peripheral control' (-not (Request 'deviceVibrate' @{id='invalid'}).ok) $null
 Check 'Host rejects simulated mouse parameters' (-not (Request 'device' @{id='invalid';key='dpi';value=1600}).ok) $null
 Check 'Host rejects embedded peripheral protocol encoders' (-not (Request 'importPlugin' @{manifest=@{id='verification-ir';module=@{format='hhub-ir/1'}}}).ok) $null
 $taskWindows=Request 'windowsSnapshot';Check 'Windows settings readback' ([bool]$taskWindows.ok) $taskWindows.error
 $taskEq=Request 'eqRuntime';Check 'EQ state and engine readback' ([bool]$taskEq.ok) $taskEq.error
 $taskDsp=Request 'pluginInvoke' @{id='eq';operation='offlineTest';body=@{}};Check 'EQ offline DSP processing and invalid-input rejection' ([bool]$taskDsp.ok) $taskDsp.error
 $taskPhone=Request 'pluginInvoke' @{id='phone';operation='state';body=@{}};Check 'Phone control service readback' ([bool]$taskPhone.ok) $taskPhone.error
 $taskUi=Start-Process -FilePath (Join-Path $taskApp 'Hhub.exe') -ArgumentList '--verify-builtin-lifecycle' -WorkingDirectory $taskApp -WindowStyle Hidden -PassThru
 if(-not $taskUi.WaitForExit(90000)){Stop-Process -Id $taskUi.Id -Force;throw 'UI lifecycle check timed out'}
 $taskLifecycleFile=Join-Path $taskProfile 'HhubPrototype/builtin-lifecycle.json'
 if(Test-Path -LiteralPath $taskLifecycleFile){$taskLifecycle=Get-Content -LiteralPath $taskLifecycleFile -Raw|ConvertFrom-Json;Check 'Repeated feature page switching and page process release' ([bool]$taskLifecycle.ok -and @($taskLifecycle.samples|Where-Object {$_.retiredPages -ne 0}).Count -eq 0) $taskLifecycle;$taskMarketSamples=@($taskLifecycle.samples|Where-Object {$_.route -like 'market*'});Check 'Market empty/full/next/small layouts fit without scrolling' ($taskMarketSamples.Count -eq 4 -and @($taskMarketSamples|Where-Object {-not $_.ok -or -not $_.noScroll -or -not $_.cardsFit}).Count -eq 0) $taskMarketSamples}
 else{Check 'Repeated feature page switching and page process release' $false "No result; exit=$($taskUi.ExitCode)"}
 Start-Sleep -Seconds 8
 $taskAfter=Request 'snapshot'
 Check 'Idle feature processes reclaimed' (@($taskAfter.state.pluginRuntime.PSObject.Properties|Where-Object {$_.Value.processes -gt 0}).Count -eq 0) $taskAfter.state.pluginRuntime
 $taskSensorPath=Join-Path $taskApp 'sensors/Hhub.exe'
 $taskSensors=@(Get-CimInstance Win32_Process -Filter "Name='Hhub.exe'"|Where-Object ExecutablePath -eq $taskSensorPath)
 Check 'Sensor helper stopped after UI closes' ($taskSensors.Count -eq 0) @($taskSensors.ProcessId)
 Check 'Monitoring samples released from core state' (-not $taskAfter.state.PSObject.Properties['monitor'] -and -not $taskAfter.state.PSObject.Properties['homeMonitoring']) $null
 $null=Request 'quit';$null=$taskCore.WaitForExit(10000)
}finally{
 if($taskUi -and -not $taskUi.HasExited){Stop-Process -Id $taskUi.Id -Force}
 if($taskCore -and -not $taskCore.HasExited){try{$null=Request 'quit';$null=$taskCore.WaitForExit(6000)}catch{};if(-not $taskCore.HasExited){Stop-Process -Id $taskCore.Id -Force}}
 $env:LOCALAPPDATA=$taskOldLocal;$env:HHUB_INSTANCE=$taskOldInstance
 $taskReport=@{ok=(@($taskResults|Where-Object {-not $_.ok}).Count -eq 0);checks=@($taskResults)}
 [IO.File]::WriteAllText((Join-Path $taskProfile 'verification.json'),($taskReport|ConvertTo-Json -Depth 50),[Text.UTF8Encoding]::new($false))
 $taskReport|ConvertTo-Json -Depth 50
}
if(-not $taskReport.ok){throw 'Integrated verification failed'}
