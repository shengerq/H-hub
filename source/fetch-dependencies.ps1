$ErrorActionPreference='Stop'
$folder=Join-Path $PSScriptRoot '..\app\sensors'
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$scratch=Join-Path $PSScriptRoot 'obj\dependencies'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$archive=Join-Path $scratch 'LibreHardwareMonitor-0.9.6.zip'
Invoke-WebRequest -Uri 'https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases/download/v0.9.6/LibreHardwareMonitor.zip' -OutFile $archive
if((Get-FileHash -LiteralPath $archive).Hash -ne '086D9F1B5A99E643EDC2CFAAAC16051685B551E4C5AC0B32A57C58C0E529C001'){throw 'Official dependency archive hash does not match'}
$unpacked=Join-Path $scratch 'upstream-0.9.6'
Expand-Archive -LiteralPath $archive -DestinationPath $unpacked -Force
Get-ChildItem -LiteralPath $unpacked -Filter '*.dll' | Copy-Item -Destination $folder -Force
& (Join-Path $PSScriptRoot "Fetch-PawnIO.ps1")
