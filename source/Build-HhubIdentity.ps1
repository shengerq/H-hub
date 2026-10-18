param(
    [Parameter(Mandatory=$true)][string]$ApplicationDirectory,
    [switch]$Register,
    [ValidateSet('CurrentUser','LocalMachine')][string]$TrustScope='CurrentUser'
)
$ErrorActionPreference='Stop'
$taskApp=[IO.Path]::GetFullPath($ApplicationDirectory)
if(-not (Test-Path -LiteralPath (Join-Path $taskApp 'Hhub.exe'))){throw 'Application directory has no Hhub.exe'}
$taskSdk=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin\10.0.26100.0\x64'
$taskPack=Join-Path $taskSdk 'makeappx.exe'
$taskSign=Join-Path $taskSdk 'signtool.exe'
$taskIdentity=Join-Path $taskApp 'identity'
New-Item -ItemType Directory -Path $taskIdentity -Force | Out-Null
$taskPackage=Join-Path $taskIdentity 'Hhub.Identity.msix'
& $taskPack pack /o /d (Join-Path $PSScriptRoot 'identity') /nv /p $taskPackage
if($LASTEXITCODE -ne 0){throw 'Identity package build failed'}
if(-not $Register){Write-Output ('Unsigned identity package: '+$taskPackage);return}
# Development signing uses a non-CA code-signing certificate scoped to this user.
# Its private key remains in the certificate store and is never exported.
$taskCert=Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -eq 'CN=H hub Development' -and $_.NotAfter -gt (Get-Date).AddDays(7) } | Sort-Object NotAfter -Descending | Select-Object -First 1
if(-not $taskCert){
    $taskCert=New-SelfSignedCertificate -Type Custom -Subject 'CN=H hub Development' -FriendlyName 'H hub local identity signing' -CertStoreLocation Cert:\CurrentUser\My -KeyUsage DigitalSignature -KeyExportPolicy NonExportable -HashAlgorithm SHA256 -KeyLength 2048 -NotAfter (Get-Date).AddYears(1) -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3','2.5.29.19={text}CA=false')
}
& $taskSign sign /fd SHA256 /s My /sha1 $taskCert.Thumbprint $taskPackage
if($LASTEXITCODE -ne 0){throw 'Identity package signing failed'}
Export-Certificate -Cert $taskCert -FilePath (Join-Path $taskIdentity 'Hhub.Development.cer') | Out-Null
if($Register){
    $taskTrustStore='Cert:\'+$TrustScope+'\TrustedPeople'
    $taskTrusted=Get-ChildItem $taskTrustStore | Where-Object Thumbprint -eq $taskCert.Thumbprint
    if(-not $taskTrusted){Import-Certificate -FilePath (Join-Path $taskIdentity 'Hhub.Development.cer') -CertStoreLocation $taskTrustStore | Out-Null}
    # Registration does not terminate or restart existing H hub processes.
    Add-AppxPackage -Path $taskPackage -ExternalLocation $taskApp
    Get-AppxPackage -Name Hhub.Desktop | Select-Object Name,PackageFamilyName,Status
}
Write-Output ('Identity package: '+$taskPackage)
