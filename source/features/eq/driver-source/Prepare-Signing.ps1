param([Parameter(Mandatory=$true)][string]$Package,[string]$Output=(Join-Path $PSScriptRoot 'build\signing'))
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'Test-DriverIdentity.ps1') | Out-Host
$Package=(Resolve-Path -LiteralPath $Package).Path
$record=Get-Content (Join-Path $Package 'build-record.json') -Raw | ConvertFrom-Json
if($record.schemaVersion -ne 2 -or $record.productionSigned -ne $false){throw 'Expected a fresh unsigned Build-Driver.ps1 build record.'}
$driverFiles=@('hhubvad.inf','hhubvad.sys','hhubvadntamd64.cat','DRIVER-NOTICE.txt','LICENSE','license.rtf')
foreach($name in $driverFiles) {
    $hash=(Get-FileHash (Join-Path $Package $name) -Algorithm SHA256).Hash
    if($hash -ine $record.files.$name){throw "Build output has changed: $name. Rebuild instead of renaming upstream artifacts."}
}
$driverInf=Get-Content (Join-Path $Package 'hhubvad.inf') -Raw
if($driverInf -notmatch 'DEV_NAME="HHUBVAD"' -or $driverInf -notmatch 'ServiceBinary=%10%\\system32\\drivers\\hhubvad.sys'){throw 'Not an independent Hhub driver package'}
$version=(Get-Item (Join-Path $Package 'hhubvad.sys')).VersionInfo
if($version.OriginalFilename -ne 'hhubvad.sys' -or $version.FileDescription -ne 'Hhub Audio' -or $version.Comments -notmatch 'Independent Hhub identities prevent conflicts'){throw 'SYS version resource is not the Hhub Audio fork with its origin notice'}
$Output=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$ddf=@('.OPTION EXPLICIT','.Set Cabinet=on','.Set Compress=on','.Set CompressionType=MSZIP','.Set CabinetNameTemplate=Hhub-Audio-Unsigned.cab',('.Set DiskDirectoryTemplate="'+$Output+'"'),'.Set DestinationDir=HhubAudio')
$ddf+=('.Set RptFileName="'+(Join-Path $Output 'makecab.rpt')+'"')
$ddf+=('.Set InfFileName="'+(Join-Path $Output 'makecab.inf')+'"')
foreach($name in $driverFiles){$ddf+='"'+(Join-Path $Package $name)+'"'}
$ddfPath=Join-Path $Output 'Hhub-Audio.ddf'
# makecab reads the system ANSI code page, even when launched from PowerShell 7.
$encoding=[Text.Encoding]::GetEncoding((Get-Culture).TextInfo.ANSICodePage)
$text=($ddf -join "`r`n")+"`r`n"
if($encoding.GetString($encoding.GetBytes($text)) -cne $text){throw 'The cabinet paths cannot be represented in the system ANSI code page. Supply -Package and -Output paths using supported characters.'}
[IO.File]::WriteAllText($ddfPath,$text,$encoding)
& "$env:WINDIR\System32\makecab.exe" /F $ddfPath
if($LASTEXITCODE -ne 0){throw 'Signing cabinet preparation failed'}
Copy-Item -LiteralPath (Join-Path $Package 'build-record.json') -Destination (Join-Path $Output 'build-record.json')
Write-Output "Unsigned submission cabinet prepared: $Output. Organizational and Microsoft signing remain required."
