param()
$ErrorActionPreference = 'Stop'
$taskSensorDirectory = Join-Path $PSScriptRoot '..\app\sensors'
$taskOutputDirectory = Join-Path $PSScriptRoot '..\build\monitor-tests'
New-Item -ItemType Directory -Path $taskOutputDirectory -Force | Out-Null
Get-ChildItem -LiteralPath $taskSensorDirectory -Filter '*.dll' | Copy-Item -Destination $taskOutputDirectory -Force
$taskCompiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$taskExecutable = Join-Path $taskOutputDirectory 'MonitoringTests.exe'
& $taskCompiler /nologo /target:exe /platform:x64 /optimize+ ("/out:" + $taskExecutable) ("/reference:" + (Join-Path $taskSensorDirectory 'LibreHardwareMonitorLib.dll')) /reference:System.Web.Extensions.dll /reference:System.Management.dll /reference:System.Core.dll (Join-Path $PSScriptRoot 'HardwareMonitoring.cs') (Join-Path $PSScriptRoot 'MonitoringTests.cs')
if ($LASTEXITCODE -ne 0) { throw 'Monitoring contract tests did not compile' }
& $taskExecutable
if ($LASTEXITCODE -ne 0) { throw 'Monitoring contract tests failed' }
