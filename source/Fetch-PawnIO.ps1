param([string]$Destination=(Join-Path $PSScriptRoot '..\driver\pawnio\PawnIO_setup.exe'))
$ErrorActionPreference='Stop'
$taskExpectedHash='1f519a22e47187f70a1379a48ca604981c4fcf694f4e65b734aaa74a9fba3032'
function Test-PawnInstaller([string]$Path) {
 if(-not (Test-Path -LiteralPath $Path)){return $false}
 if((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $taskExpectedHash){return $false}
 $taskSignature=Get-AuthenticodeSignature -LiteralPath $Path
 return $taskSignature.Status -eq 'Valid' -and $taskSignature.SignerCertificate.Subject -match 'namazso'
}
if(Test-PawnInstaller $Destination){Write-Output 'PawnIO 2.2.0 official installer verified';return}
$taskDirectory=Split-Path -Parent ([IO.Path]::GetFullPath($Destination))
New-Item -ItemType Directory -Force -Path $taskDirectory | Out-Null
$taskDownload=Join-Path $taskDirectory ('PawnIO-'+[guid]::NewGuid().ToString('N')+'.download')
$taskOptions=@{Uri='https://github.com/namazso/PawnIO.Setup/releases/download/2.2.0/PawnIO_setup.exe';OutFile=$taskDownload}
if($env:http_proxy){$taskOptions.Proxy=$env:http_proxy}
Invoke-WebRequest @taskOptions
if(-not (Test-PawnInstaller $taskDownload)){throw 'PawnIO official installer hash or publisher signature verification failed'}
Move-Item -LiteralPath $taskDownload -Destination $Destination -Force
Write-Output 'PawnIO 2.2.0 official installer downloaded and verified'
