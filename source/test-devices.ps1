param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskDeviceReport=$ReportPath
. (Join-Path $PSScriptRoot 'test-ipc.ps1')
$taskResults=[Collections.Generic.List[string]]::new()
function Assert-Device($Condition,[string]$Message){if(-not $Condition){throw $Message};$taskResults.Add('PASS '+$Message)}
$taskStatePath=Join-Path $env:LOCALAPPDATA 'HhubPrototype\state.json'
$taskWriteTime=(Get-Item -LiteralPath $taskStatePath).LastWriteTimeUtc
$taskFirst=Invoke-HhubRequest 'deviceScan'
Assert-Device ($taskFirst.ok -and $taskFirst.state.detected.real) 'Windows device reader returns a real inventory'
$taskList=$taskFirst.state.detected.devices
Assert-Device ($taskList.Count -gt 0) 'Current machine exposes real peripheral nodes'
Assert-Device (($taskList | Where-Object {-not $_.real -or $_.id -in @('rapoo-vt3s','rapoo-vt3pro','rog-hfx','k03s','overture','beitong-20')}).Count -eq 0) 'Inventory does not substitute predefined demo models'
Assert-Device (($taskList.id | Select-Object -Unique).Count -eq $taskList.Count) 'Device identifiers are unique'
$taskMouseIds=Get-CimInstance Win32_PnPEntity -Filter "PNPClass='Mouse'" | Where-Object Present | ForEach-Object PNPDeviceID
$taskNodeIds=$taskList.interfaces.instanceId
foreach($taskId in $taskMouseIds){Assert-Device ($taskId -in $taskNodeIds) 'Mouse instance is independently confirmed by Windows CIM inventory'}
$taskSecond=Invoke-HhubRequest 'deviceSnapshot'
Assert-Device ($taskSecond.state.detected.scanCount -eq $taskFirst.state.detected.scanCount) 'Rapid refresh uses cached inventory instead of re-enumerating hardware'
Assert-Device ($taskSecond.state.checks -eq $taskFirst.state.checks) 'Device reads never trigger plugin update checks'
Assert-Device ((Get-Item -LiteralPath $taskStatePath).LastWriteTimeUtc -eq $taskWriteTime) 'Reading devices does not rewrite preferences'
Assert-Device (($taskList | Where-Object {$null -ne $_.battery -and ($_.battery -lt 0 -or $_.battery -gt 100 -or -not $_.batterySource)}).Count -eq 0) 'Battery values require a real source and stay within 0 to 100 percent'
$taskForced=Invoke-HhubRequest 'deviceScan'
Assert-Device ($taskForced.state.detected.scanCount -gt $taskSecond.state.detected.scanCount) 'Explicit refresh performs a fresh scan'
$taskResults.Add('Model-specific control protocols and wireless battery values require matching hardware evidence; this inventory check does not certify them.')
if($taskDeviceReport){[IO.File]::WriteAllLines($taskDeviceReport,$taskResults,[Text.UTF8Encoding]::new($true))}
$taskResults
