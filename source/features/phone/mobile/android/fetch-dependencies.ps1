param([string]$Destination="$PSScriptRoot\..\..\..\work\zxing-core-3.5.4.jar")
$ErrorActionPreference='Stop'
$Destination=[IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Force (Split-Path $Destination) | Out-Null
Invoke-WebRequest 'https://repo.maven.apache.org/maven2/com/google/zxing/core/3.5.4/core-3.5.4.jar' -OutFile $Destination
if((Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -ne '71DE5D89341B5FCF5DD89DA7F44E84D825D0E084CDF3EC77C9ABE26B0F0CEB13'){throw 'ZXing SHA-256 mismatch'}
Write-Output $Destination
