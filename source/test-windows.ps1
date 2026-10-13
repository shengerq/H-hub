param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskWindowsReport=$ReportPath
. (Join-Path $PSScriptRoot 'test-ipc.ps1')
$taskResults=[Collections.Generic.List[string]]::new()
function Assert-Windows($Condition,[string]$Message){if(-not $Condition){throw $Message};$taskResults.Add('PASS '+$Message)}
$taskCapture={@('HKCU:\Software\Microsoft\GameBar','HKCU:\Control Panel\Mouse','HKCU:\Control Panel\Accessibility\StickyKeys','HKCU:\Control Panel\Accessibility\Keyboard Response','HKCU:\Control Panel\Accessibility\ToggleKeys') | ForEach-Object {if(Test-Path $_){Get-ItemProperty $_ | Select-Object * -ExcludeProperty PSPath,PSParentPath,PSChildName,PSDrive,PSProvider | ConvertTo-Json -Depth 4 -Compress}}}
$taskBefore=(& $taskCapture) -join "`n"
$taskFirst=Invoke-HhubRequest 'windowsSnapshot'
Assert-Windows ($taskFirst.ok -and $taskFirst.state.windowsSettings.real) 'Windows settings read actual system preferences'
$taskData=$taskFirst.state.windowsSettings
$taskExpected=@('gameMode','controllerGameBar','mousePrecision','stickyShortcut','filterShortcut','toggleShortcut','systemSound','hdr','capture','gpuScheduling','windowedOptimizations','notifications')
Assert-Windows (($taskExpected | Where-Object {$_ -notin $taskData.options.id}).Count -eq 0 -and $taskData.options.Count -eq $taskExpected.Count) 'All current Windows controls and native settings routes are present'
Assert-Windows (($taskData.options.id | Select-Object -Unique).Count -eq $taskExpected.Count) 'Setting identifiers are unique'
foreach($taskOption in $taskData.options){Assert-Windows ($taskOption.uri -like 'ms-settings:*') 'Every switch includes a native Windows settings route';if(-not $taskOption.present -and $taskOption.id -in 'gameMode','controllerGameBar'){Assert-Windows ($null -eq $taskOption.enabled) 'Absent preference stays unknown instead of being reported off'}}
$taskPowerOutput=(& powercfg /getactivescheme) -join ' '
$taskPowerMatch=[regex]::Match($taskPowerOutput,'[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}')
Assert-Windows ($taskPowerMatch.Success -and $taskData.activePower.Trim('{}') -eq $taskPowerMatch.Value) 'Active power plan matches independent Windows powercfg query'
Assert-Windows ($taskData.activePower -in $taskData.powerPlans.id) 'Power selector includes the actual active plan'
$taskSecond=Invoke-HhubRequest 'windowsSnapshot'
Assert-Windows ($taskSecond.state.checks -eq $taskFirst.state.checks) 'Reading Windows settings does not check plugins'
Assert-Windows (((& $taskCapture) -join "`n") -eq $taskBefore) 'Reading the page does not modify gaming, mouse or accessibility settings'
$taskResults.Add('Write/undo paths were tested with a separate H hub registry fixture and injected SPI data. Actual user Windows switches and power plans were not changed by these tests. Native settings pages require user manual interaction.')
if($taskWindowsReport){[IO.File]::WriteAllLines($taskWindowsReport,$taskResults,[Text.UTF8Encoding]::new($true))}
$taskResults
