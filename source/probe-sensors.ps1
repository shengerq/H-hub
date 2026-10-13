param([string]$ReportPath)
$ErrorActionPreference='Stop'
if(-not $ReportPath){$ReportPath=Join-Path $PSScriptRoot '..\build\host-acceptance\sensors-elevated.json'}
$taskExe=Join-Path $PSScriptRoot '..\app\sensors\Hhub.Sensors.exe'
$taskSamples=@('sample','sample','quit') | & $taskExe
if($LASTEXITCODE -ne 0){throw 'Sensor probe failed'}
$taskSamples | Select-Object -Last 1 | Set-Content -LiteralPath $ReportPath -Encoding utf8
