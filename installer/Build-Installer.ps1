param([Parameter(Mandatory=$true)][string]$ApplicationDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference='Stop'
$taskApp=[IO.Path]::GetFullPath($ApplicationDirectory)
$taskOutput=[IO.Path]::GetFullPath($OutputDirectory)
foreach($taskName in @('Hhub.exe','core/Hhub.exe','Hhub.PluginHost.exe','features/eq/HhubAudioDriverSetup.exe')){if(-not (Test-Path -LiteralPath (Join-Path $taskApp $taskName))){throw "Missing installer input: $taskName"}}
New-Item -ItemType Directory -Force $taskOutput|Out-Null
& $Compiler ("/DAppRoot=$taskApp") ("/DReleaseOutput=$taskOutput") (Join-Path $PSScriptRoot 'Hhub.Integrated.iss')
if($LASTEXITCODE -ne 0){throw 'Installer compilation failed'}
Get-FileHash -LiteralPath (Join-Path $taskOutput 'Hhub-Setup-1.0-Integrated-EQ-r4.exe') -Algorithm SHA256
