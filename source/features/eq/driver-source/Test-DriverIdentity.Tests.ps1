$ErrorActionPreference='Stop'
$testRoot=Join-Path $PSScriptRoot ('build\identity-tests-'+[Guid]::NewGuid().ToString('N'))
$fixture=Join-Path $testRoot 'driver-source'
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
foreach($name in @('driver-identity.json','shipping-payload-sha256.json','fxvad\common.cpp','fxvad\msvad.h','fxvad\hhubvad.rc','fxvad\pcmex\hhubvad.inf','fxvad\pcmex\vadpcmex.vcxproj')) {
    $destination=Join-Path $fixture $name
    New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $destination
}
Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot) 'driver') -Destination (Join-Path $testRoot 'driver') -Recurse
$checker=Join-Path $PSScriptRoot 'Test-DriverIdentity.ps1'
& $checker -SourceRoot $fixture | Out-Null
$results=@('valid source: PASS')
function RejectMutation([string]$Name,[string]$File,[string]$Old,[string]$New,[string]$Expected) {
    $path=Join-Path $fixture $File
    $original=[IO.File]::ReadAllBytes($path)
    $text=[IO.File]::ReadAllText($path)
    if(-not $text.Contains($Old)){throw "Invalid test mutation: $Name"}
    [IO.File]::WriteAllText($path,$text.Replace($Old,$New))
    $failure=$null
    try { & $checker -SourceRoot $fixture | Out-Null } catch {$failure=$_.Exception.Message}
    finally {[IO.File]::WriteAllBytes($path,$original)}
    if(-not $failure -or $failure -notlike "*$Expected*"){throw "Unexpected result for ${Name}: $failure"}
    return "$Name`: PASS (rejected)"
}
$results+=RejectMutation 'upstream service' 'fxvad\pcmex\hhubvad.inf' 'DEV_NAME="HHUBVAD"' 'DEV_NAME="FXVAD"' 'Wrong INF service'
$results+=RejectMutation 'upstream binary' 'fxvad\pcmex\hhubvad.inf' 'drivers\hhubvad.sys' 'drivers\fxvad.sys' 'Wrong service binary'
$results+=RejectMutation 'interface mismatch' 'fxvad\common.cpp' 'L"HhubWave"' 'L"Wave"' 'Kernel registration differs'
$results+=RejectMutation 'Windows GUID changed' 'fxvad\pcmex\hhubvad.inf' '6994AD04-93EF-11D0-A3CC-00A0C9223196' '6994AD04-93EF-11D0-A3CC-00A0C9223197' 'Windows standard GUID changed'
$results+=RejectMutation 'static GUID mismatch' 'fxvad\msvad.h' '0x8b6f317d' '0x303b166e' 'Static product GUID differs'
$results+=RejectMutation 'old build output' 'fxvad\pcmex\vadpcmex.vcxproj' '<TargetName>hhubvad</TargetName>' '<TargetName>fxvad</TargetName>' 'Wrong project output name'
$results+=RejectMutation 'signed payload edited' '..\driver\payload\fxvad.inf' 'FXVAD' 'HHUBVAD' 'Shipping signed payload changed'
$results
$results | ConvertTo-Json | Set-Content (Join-Path $testRoot 'results.json') -Encoding UTF8
