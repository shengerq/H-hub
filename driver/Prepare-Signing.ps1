param([string]$Package=(Join-Path $PSScriptRoot '..\build\audio-driver\fxvad'))
$ErrorActionPreference='Stop'
$Package=[IO.Path]::GetFullPath($Package)
$driverSigning=Join-Path $PSScriptRoot '..\build\audio-driver\signing'
New-Item -ItemType Directory -Path $driverSigning -Force | Out-Null
$driverFiles=@('fxvad.inf','fxvad.sys','fxvadntamd64.cat')
foreach($driverFile in $driverFiles){if(-not (Test-Path (Join-Path $Package $driverFile))){throw "Missing package file: $driverFile"}}
$driverInf=Get-Content (Join-Path $Package 'fxvad.inf') -Raw
if($driverInf -notmatch 'DEV_NAME="HHUBVAD"' -or $driverInf -notmatch 'H hub Audio'){throw 'Not the H hub dedicated driver package'}
$driverDdf=@('.OPTION EXPLICIT','.Set Cabinet=on','.Set Compress=on','.Set CompressionType=MSZIP','.Set CabinetNameTemplate=Hhub-Audio-Unsigned.cab',".Set DiskDirectoryTemplate=$driverSigning",'.Set DestinationDir=HhubAudio')
foreach($driverFile in $driverFiles){$driverDdf+='"'+(Join-Path $Package $driverFile)+'"'}
$driverDdfPath=Join-Path $driverSigning 'Hhub-Audio.ddf'
$driverDdf | Set-Content $driverDdfPath -Encoding ascii
& "$env:WINDIR\System32\makecab.exe" /F $driverDdfPath
if($LASTEXITCODE -ne 0){throw 'Signing cabinet preparation failed'}
$driverFiles | ForEach-Object {Get-FileHash (Join-Path $Package $_) -Algorithm SHA256} | Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $driverSigning 'SHA256.json') -Encoding utf8
Write-Output 'Unsigned submission cabinet prepared. It still requires organizational signing and Microsoft submission; do not install or distribute it as a production driver.'
