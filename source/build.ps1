param([string]$Configuration='Release',[string]$OutputDirectory=(Join-Path $PSScriptRoot '../app'))
$ErrorActionPreference='Stop'
if($Configuration -ne 'Release'){throw 'Integrated release builds require Configuration=Release'}
# Use a complete unpacked application as the runtime base; this entry point and
# Build-Integrated share the same feature, sensor, core and UI build pipeline.
& (Join-Path $PSScriptRoot 'Build-Integrated.ps1') -OutputDirectory $OutputDirectory
