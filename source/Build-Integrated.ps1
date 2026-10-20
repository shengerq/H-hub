param([string]$OutputDirectory=(Join-Path $PSScriptRoot '../integrated-app'),[switch]$SkipFeatures)
$ErrorActionPreference='Stop'
$taskOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $taskOutput|Out-Null
if(-not $SkipFeatures){& (Join-Path $PSScriptRoot 'Build-BuiltinFeatures.ps1') -OutputDirectory $taskOutput}
$taskSensorDir=Join-Path $taskOutput 'sensors'
$taskCsc=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
& $taskCsc ("/win32manifest:"+(Join-Path $PSScriptRoot 'app.manifest')) ("/win32icon:"+(Join-Path $PSScriptRoot 'assets/hhub.ico')) /nologo /target:exe /platform:x64 /optimize+ ("/out:"+(Join-Path $taskSensorDir 'Hhub.exe')) ("/reference:"+(Join-Path $taskSensorDir 'LibreHardwareMonitorLib.dll')) /reference:System.Web.Extensions.dll /reference:System.Management.dll /reference:System.Core.dll (Join-Path $PSScriptRoot 'HardwareMonitoring.cs') (Join-Path $PSScriptRoot 'Sensors.cs')
if($LASTEXITCODE -ne 0){throw 'Sensor helper build failed'}
& $taskCsc ("/win32manifest:"+(Join-Path $PSScriptRoot 'app.manifest')) /nologo /target:exe /platform:x64 /optimize+ ("/out:"+(Join-Path $taskOutput 'Hhub.Update.exe')) /reference:System.Web.Extensions.dll (Join-Path $PSScriptRoot 'UpdateService.cs')
if($LASTEXITCODE -ne 0){throw 'Update helper build failed'}
if(Test-Path -LiteralPath (Join-Path $taskSensorDir 'Hhub.Sensors.exe.config')){Copy-Item -LiteralPath (Join-Path $taskSensorDir 'Hhub.Sensors.exe.config') -Destination (Join-Path $taskSensorDir 'Hhub.exe.config') -Force}
$taskVswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskMsbuild=& $taskVswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/amd64/MSBuild.exe'|Select-Object -First 1
foreach($taskProject in @('Hhub.Core','Hhub.PluginHost','Hhub')){
 $taskTarget=$taskOutput;$taskExtra=@()
 if($taskProject -eq 'Hhub.Core'){$taskTarget=Join-Path $taskOutput 'core';New-Item -ItemType Directory -Force $taskTarget|Out-Null;$taskExtra=@('/p:TargetName=Hhub')}
 & $taskMsbuild (Join-Path $PSScriptRoot "$taskProject.vcxproj") /restore /m /p:Configuration=Release /p:Platform=x64 ("/p:OutDir=$taskTarget\") @taskExtra /p:RestoreSources=C:\Users\Administrator\.nuget\packages /p:RestoreIgnoreFailedSources=true /verbosity:minimal
 if($LASTEXITCODE -ne 0){throw "Build failed: $taskProject"}
}
foreach($taskLegacy in @('Hhub.Core.exe','sensors/Hhub.Sensors.exe','sensors/Hhub.Sensors.exe.config')){$taskLegacyPath=Join-Path $taskOutput $taskLegacy;if(Test-Path -LiteralPath $taskLegacyPath){Remove-Item -LiteralPath $taskLegacyPath -Force}}
foreach($taskDirectory in @('assets','languages','webui')){Copy-Item (Join-Path $PSScriptRoot $taskDirectory) $taskOutput -Recurse -Force}
$taskVsRoot=& $taskVswhere -latest -products '*' -property installationPath
$taskRedist=Get-ChildItem -LiteralPath (Join-Path $taskVsRoot 'VC/Redist/MSVC') -Directory|Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'x64/Microsoft.VC143.CRT')}|Sort-Object {[version]$_.Name} -Descending|Select-Object -First 1
if($taskRedist){Get-ChildItem -LiteralPath (Join-Path $taskRedist.FullName 'x64/Microsoft.VC143.CRT') -Filter '*.dll'|Copy-Item -Destination $taskOutput -Force}
Write-Output "Integrated legacy application: $taskOutput"
