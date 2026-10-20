param([string]$SourceRoot=$PSScriptRoot)
$ErrorActionPreference='Stop'
$id=Get-Content (Join-Path $SourceRoot 'driver-identity.json') -Raw | ConvertFrom-Json
$inf=Get-Content (Join-Path $SourceRoot "fxvad\pcmex\$($id.inf)") -Raw
$common=Get-Content (Join-Path $SourceRoot 'fxvad\common.cpp') -Raw
$header=Get-Content (Join-Path $SourceRoot 'fxvad\msvad.h') -Raw
$resource=Get-Content (Join-Path $SourceRoot 'fxvad\hhubvad.rc') -Raw
[xml]$project=Get-Content (Join-Path $SourceRoot 'fxvad\pcmex\vadpcmex.vcxproj') -Raw
function Require([bool]$Condition,[string]$Message) {if(-not $Condition){throw "Driver identity check failed: $Message"}}
function InfValue([string]$Key) {
    $m=[regex]::Match($inf,'(?im)^\s*'+[regex]::Escape($Key)+'\s*=\s*"?([^";\r\n]+)')
    Require $m.Success "Missing INF key $Key"
    return $m.Groups[1].Value.Trim()
}
Require ($id.hardwareId -ceq ('ROOT\'+$id.service)) 'Hardware ID must match the independent service'
Require ($id.service -ceq 'HHUBVAD') 'Do not reuse FXVAD service'
Require ((InfValue 'DEV_NAME') -ceq $id.service) 'Wrong INF service'
Require ($inf.Contains('Root\%DEV_NAME%')) 'Missing independent root hardware ID'
Require ($inf.Contains('AddService=%DEV_NAME%,')) 'Wrong service registration'
Require ($inf.Contains('ServiceBinary=%10%\system32\drivers\'+$id.binary)) 'Wrong service binary'
Require ($inf.Contains('HKR,,Driver,,'+$id.binary)) 'Wrong registry binary'
Require ((InfValue 'CatalogFile.NTAMD64') -ceq $id.catalog) 'Wrong catalog'
Require ((InfValue 'DFX_Device.DeviceDesc') -ceq $id.displayName) 'Wrong endpoint name'
foreach($pair in @(@('KSNAME_Wave','waveReference'),@('KSNAME_Topology','topologyReference'))) {
    $reference=$id.($pair[1])
    Require ((InfValue $pair[0]) -ceq $reference) "Wrong INF $($pair[0])"
    Require ($common.Contains('L"'+$reference+'"')) "Kernel registration differs from $reference"
}
Require (-not ($common -match 'L"(Wave|Topology)"')) 'Upstream registration remains'
Require ((InfValue 'GUID.DSPEAKERS') -ceq ('{'+$id.mediaCategoryGuid+'}')) 'Wrong private media category'
Require ($header.Contains('DEFINE_GUIDSTRUCT("'+$id.productGuid+'", PID_MSVAD)')) 'Wrong product GUID'
$bytes=([Guid]$id.productGuid).ToByteArray()
$expected=@([BitConverter]::ToUInt32($bytes,0),[BitConverter]::ToUInt16($bytes,4),[BitConverter]::ToUInt16($bytes,6)) + @($bytes[8..15])
$static=[regex]::Match($header,'(?s)#define STATIC_PID_MSVAD\\\s*(.*?)\s*DEFINE_GUIDSTRUCT').Groups[1].Value
$actual=@([regex]::Matches($static,'0x[0-9a-fA-F]+') | ForEach-Object {[Convert]::ToUInt32($_.Value.Substring(2),16)})
Require (($actual -join ',') -ceq ($expected -join ',')) 'Static product GUID differs from declared GUID'
foreach($key in @{'KSCATEGORY_AUDIO'='{6994AD04-93EF-11D0-A3CC-00A0C9223196}';'KSCATEGORY_RENDER'='{65E8773E-8F56-11D0-A3B9-00A0C9223196}';'Proxy.CLSID'='{17CCA71B-ECD7-11D0-B908-00A0C9223196}';'KSNODETYPE_SPEAKER'='{DFF21CE1-F70F-11D0-B917-00A0C9223196}'}.GetEnumerator()) {
    Require ((InfValue $key.Key) -ceq $key.Value) "Windows standard GUID changed: $($key.Key)"
}
foreach($node in $project.SelectNodes('//*[local-name()="TargetName"]')) {
    Require ($node.InnerText -ceq [IO.Path]::GetFileNameWithoutExtension($id.binary)) 'Wrong project output name'
}
Require ($resource.Contains('"'+$id.binary+'"') -and $resource.Contains('"'+$id.displayName+'"')) 'Wrong version resource'
Require ($resource.Contains('FxSound LLC')) 'Upstream attribution missing'
$activeInf=($inf -split "`n" | Where-Object {$_ -notmatch '^\s*;'}) -join "`n"
Require (-not ($activeInf -match '(?i)(?<!hhub)fxvad|FxSound')) 'Upstream identity remains in active INF'
$baseline=Get-Content (Join-Path $SourceRoot 'shipping-payload-sha256.json') -Raw | ConvertFrom-Json
foreach($file in $baseline.PSObject.Properties) {
    $hash=(Get-FileHash (Join-Path (Split-Path $SourceRoot) $file.Name) -Algorithm SHA256).Hash
    Require ($hash -ieq $file.Value) "Shipping signed payload changed: $($file.Name)"
}
[pscustomobject]@{validated=$true; hardwareId=$id.hardwareId; binary=$id.binary; shippingPayloadUnchanged=$true; productionSigned=$false}
