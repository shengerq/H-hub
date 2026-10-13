param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskEqReport=$ReportPath
. (Join-Path $PSScriptRoot 'test-ipc.ps1')
$taskResults=[Collections.Generic.List[string]]::new()
function Assert-Eq($Condition,[string]$Message){if(-not $Condition){throw $Message};$taskResults.Add('PASS '+$Message)}
$taskControl=Join-Path $env:LOCALAPPDATA 'HhubPrototype\eq-audit-control.json'
[IO.File]::WriteAllText($taskControl,'{"tone":false,"quit":false}',[Text.UTF8Encoding]::new($false))
$taskExe=(Resolve-Path (Join-Path $PSScriptRoot '..\app\Hhub.AudioTest.exe')).Path
$taskPlayer=Start-Process -FilePath $taskExe -ArgumentList ('"'+$taskControl+'"') -WindowStyle Hidden -PassThru
$taskAppId='app:'+$taskExe.ToUpperInvariant()
try {
 Start-Sleep -Milliseconds 500
 $taskRuntime=Invoke-HhubRequest 'eqRuntime'
 $taskApp=$taskRuntime.state.eqRuntime.apps | Where-Object id -eq $taskAppId
 Assert-Eq ([bool]$taskApp) 'Real WASAPI sessions identify the owned silent test application'
 Assert-Eq ($taskRuntime.state.eqRuntime.outputs.Count -gt 0) 'EQ output choices are actual active Windows endpoints'
 Assert-Eq ($taskRuntime.state.eqRuntime.routingAvailable) 'Current Windows exposes compatible per-application routing API'
 $taskOutput=$taskRuntime.state.eqRuntime.outputs[0].id
 $taskBody=@{app=$taskAppId;output=$taskOutput;source='';enabled=$true;preamp=-3;autoPreamp=$true;preset='custom';bands=@(0,0,0,0,0,6,0,0,0,0,0,0);frequencies=@(31.25,62.5,125,250,500,1000,2000,4000,8000,16000,40,12000)}
 $taskSaved=Invoke-HhubRequest 'eqRealSave' $taskBody
 Assert-Eq $taskSaved.ok 'Real app/output configuration saves without starting audio processing'
 $taskEntry=$taskSaved.state.eqRealPrefs | Where-Object {$_.app -eq $taskAppId -and $_.output -eq $taskOutput}
 Assert-Eq ($taskEntry.bands[5] -eq 6 -and $taskEntry.preamp -eq -3 -and -not $taskEntry.active) 'Saved gain, preamp and inactive processing state match the request'
 if(-not $taskRuntime.state.eqRuntime.driverReady){
  $taskStart=Invoke-HhubRequest 'eqStart' @{app=$taskAppId;output=$taskOutput}
  Assert-Eq (-not $taskStart.ok) 'Unavailable virtual endpoint blocks live EQ instead of pretending to process audio'
  $taskResults.Add(('Virtual audio driver installed: '+$taskRuntime.state.eqRuntime.driverInstalled+'; session endpoints available: '+$taskRuntime.state.eqRuntime.driverReady))
 }
 $taskCleanup=Invoke-HhubRequest 'eqForget' @{app=$taskAppId}
 Assert-Eq (-not ($taskCleanup.state.eqRealPrefs | Where-Object app -eq $taskAppId)) 'Test-only EQ configuration is removed without touching other apps'
} finally {
 [IO.File]::WriteAllText($taskControl,'{"tone":false,"quit":true}',[Text.UTF8Encoding]::new($false))
 if(-not $taskPlayer.WaitForExit(1500)){$taskPlayer.Kill()}
 $taskPlayer.Dispose()
}
$taskResults.Add('This script verifies configuration and endpoint gating, not full live EQ playback or listening quality.')
if($taskEqReport){[IO.File]::WriteAllLines($taskEqReport,$taskResults,[Text.UTF8Encoding]::new($true))}
$taskResults
