param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskMonitorReportPath=$ReportPath
. (Join-Path $PSScriptRoot 'test-ipc.ps1')
$taskResults=[Collections.Generic.List[string]]::new()
function Assert-Monitor($Condition,[string]$Message){if(-not $Condition){throw $Message};$taskResults.Add('PASS '+$Message)}
$taskStatePath=Join-Path $env:LOCALAPPDATA 'HhubPrototype\state.json'
$taskWriteTime=(Get-Item -LiteralPath $taskStatePath).LastWriteTimeUtc
$taskFirst=Invoke-HhubRequest 'monitor'
Start-Sleep -Milliseconds 2300
$taskSecond=Invoke-HhubRequest 'monitor'
$taskDeadline = (Get-Date).AddSeconds(30)
while (-not $taskSecond.state.monitor.sensorReady -and (Get-Date) -lt $taskDeadline) { Start-Sleep -Milliseconds 500; $taskSecond = Invoke-HhubRequest 'monitor' }
$taskMonitor=$taskSecond.state.monitor
Assert-Monitor ($taskSecond.ok -and $taskMonitor.real -and $taskMonitor.cpuName -and $taskMonitor.cpuName -ne 'CPU') 'Real provider returns hardware identity and live telemetry'
Assert-Monitor ($null -ne $taskMonitor.cpu -and $taskMonitor.cpu -ge 0 -and $taskMonitor.cpu -le 100) 'CPU interval sample is valid and bounded'
Assert-Monitor ($taskMonitor.sampleTime -gt $taskFirst.state.monitor.sampleTime) 'Sampling timestamp advances'
Assert-Monitor ($taskMonitor.memoryUsedGB -gt 0 -and $taskMonitor.memoryTotalGB -ge $taskMonitor.memoryUsedGB -and $taskMonitor.memory -le 100) 'Physical memory totals and use are consistent'
Assert-Monitor ($taskMonitor.diskTotalGB -gt 0 -and $taskMonitor.diskUsedGB -ge 0 -and $taskMonitor.diskUsedGB -le $taskMonitor.diskTotalGB) 'Disk capacity is real and bounded'
foreach ($taskVolume in $taskMonitor.volumes) {
 $taskDrive = [IO.DriveInfo]::new($taskVolume.name)
 Assert-Monitor ([math]::Abs($taskVolume.totalGB - $taskDrive.TotalSize / 1000000000.0) -lt 0.02) ('Supplement volume capacity matches Windows: ' + $taskVolume.name)
}
Assert-Monitor ($taskMonitor.monitorBackend -eq 'librehardwaremonitor') 'Default source remains LibreHardwareMonitor'
Assert-Monitor ($null -ne $taskMonitor.diskReadMB -and $taskMonitor.diskReadMB -ge 0 -and $null -ne $taskMonitor.diskWriteMB -and $taskMonitor.diskWriteMB -ge 0) 'Disk read/write rates return without elevation'
Assert-Monitor ($taskMonitor.sensorStatus -like 'LibreHardwareMonitor 0.9.6*' -and $taskMonitor.sensors.Count -gt 0) 'LibreHardwareMonitor worker returns actual sensors'
$taskTemps=$taskMonitor.sensors | Where-Object {$_.category -eq 'Cpu' -and $_.type -eq 'Temperature' -and $null -ne $_.value}
if(-not $taskTemps){Assert-Monitor ($null -eq $taskMonitor.cpuTemp) 'Unsupported CPU temperature remains null, without simulated fallback'}
Assert-Monitor ($taskSecond.state.checks -eq $taskFirst.state.checks) 'Monitor refresh never checks plugin updates'
Assert-Monitor ((Get-Item -LiteralPath $taskStatePath).LastWriteTimeUtc -eq $taskWriteTime) 'Read-only monitoring does not rewrite preferences'
$taskSensorExe=(Resolve-Path (Join-Path $PSScriptRoot '..\app\sensors\Hhub.Sensors.exe')).Path
$taskPid=(Get-Process Hhub.Sensors | Where-Object {$_.Path -eq $taskSensorExe}).Id
$taskBefore=(Get-Process -Id $taskPid).TotalProcessorTime.TotalMilliseconds
Start-Sleep -Milliseconds 2300
$taskAfter=(Get-Process -Id $taskPid).TotalProcessorTime.TotalMilliseconds
Assert-Monitor (($taskAfter-$taskBefore) -lt 200) 'Sensor worker stays idle without sampling requests'
$taskStopped=Invoke-HhubRequest 'monitorStop'
Start-Sleep -Milliseconds 200
Assert-Monitor (-not (Get-Process -Id $taskPid -ErrorAction SilentlyContinue)) 'Stopping monitoring releases the sensor process'
$taskRestart=Invoke-HhubRequest 'monitor'
Start-Sleep -Milliseconds 2300
$taskRestart=Invoke-HhubRequest 'monitor'
$taskDeadline = (Get-Date).AddSeconds(30)
while (-not $taskRestart.state.monitor.sensorReady -and (Get-Date) -lt $taskDeadline) { Start-Sleep -Milliseconds 500; $taskRestart = Invoke-HhubRequest 'monitor' }
Assert-Monitor ($taskRestart.state.monitor.sensors.Count -gt 0) 'Monitoring resumes immediately after an explicit stop'
Invoke-HhubRequest 'monitorStop' | Out-Null
$taskResults.Add(('Environment: '+$(if($taskMonitor.virtualMachine){'virtual machine'}else{'physical machine'})+'; elevated sensor worker: '+$taskMonitor.sensorElevated))
if($taskMonitorReportPath){[IO.File]::WriteAllLines($taskMonitorReportPath,$taskResults,[Text.UTF8Encoding]::new($true))}
$taskResults
