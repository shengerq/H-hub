param(
 [Parameter(Mandatory=$true)][string]$LibraryPath,
 [string]$CatalogExecutable=(Join-Path $PSScriptRoot '..\app\Hhub.Catalog.exe'),
 [string]$OutputPath
)
$ErrorActionPreference='Stop'
$taskLibrary=[IO.Path]::GetFullPath($LibraryPath)
if(!(Test-Path -LiteralPath $taskLibrary -PathType Container)){throw '插件库目录不存在'}
if(!$OutputPath){$OutputPath=Join-Path $taskLibrary 'index.json'}
$taskRequest=[IO.Path]::GetTempFileName();$taskReply=[IO.Path]::GetTempFileName()
try {
 @{directory=$taskLibrary} | ConvertTo-Json -Compress | Set-Content -LiteralPath $taskRequest -Encoding UTF8
 & $CatalogExecutable '--index-directory' $taskRequest $taskReply
 if($LASTEXITCODE -ne 0){throw (Get-Content -LiteralPath $taskReply -Raw)}
 $taskResult=Get-Content -LiteralPath $taskReply -Raw -Encoding UTF8 | ConvertFrom-Json
 if(!$taskResult.ok){throw $taskResult.error}
 $taskIndex=$taskResult.catalog | ConvertTo-Json -Depth 12 -Compress
 [IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputPath),$taskIndex,[Text.UTF8Encoding]::new($false))
 [pscustomobject]@{index=[IO.Path]::GetFullPath($OutputPath);plugins=$taskResult.catalog.plugins.Count;bytes=[Text.Encoding]::UTF8.GetByteCount($taskIndex)}
} finally {
 Remove-Item -LiteralPath $taskRequest,$taskReply -ErrorAction SilentlyContinue
}
