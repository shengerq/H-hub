param()
$ErrorActionPreference = 'Stop'
$taskCsc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$taskExe = Join-Path ([IO.Path]::GetTempPath()) ('Hhub.ContractTests-' + [Guid]::NewGuid().ToString('N') + '.exe')
try {
    & $taskCsc /nologo /target:exe ("/out:" + $taskExe) /reference:System.Web.Extensions.dll /main:PluginContractTests (Join-Path $PSScriptRoot 'PluginContract.cs') (Join-Path $PSScriptRoot 'PluginContractTests.cs')
    if ($LASTEXITCODE -ne 0) { throw '契约测试编译失败' }
    & $taskExe
    if ($LASTEXITCODE -ne 0) { throw '契约测试失败' }
} finally { if (Test-Path -LiteralPath $taskExe) { Remove-Item -LiteralPath $taskExe } }
