param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$taskManifests=@()
$taskVswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskMsbuild=& $taskVswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/amd64/MSBuild.exe'|Select-Object -First 1
foreach($taskId in @('windows','eq','phone')){
 $taskSource=Join-Path $PSScriptRoot "features/$taskId"
 & (Join-Path $taskSource 'build.ps1')
 if($LASTEXITCODE -ne 0){throw "Feature build failed: $taskId"}
 $taskPackage=Get-ChildItem -LiteralPath (Join-Path $taskSource 'out') -Filter '*.hhub' | Select-Object -First 1
 $taskManifest=Get-Content -LiteralPath $taskPackage.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
 $taskFolder=Join-Path $OutputDirectory "features/$taskId"
 foreach($taskAsset in $taskManifest.nativeAssets){
  $taskBytes=[Convert]::FromBase64String($taskAsset.data)
  $taskPath=Join-Path $taskFolder $taskAsset.path
  New-Item -ItemType Directory -Force (Split-Path -Parent $taskPath)|Out-Null
  [IO.File]::WriteAllBytes($taskPath,$taskBytes)
  $taskAsset.PSObject.Properties.Remove('data')
  $taskAsset.PSObject.Properties.Remove('encoding')
  $taskAsset | Add-Member -NotePropertyName bytes -NotePropertyValue $taskBytes.Length -Force
 }
 $taskManifest.description=$taskManifest.description.Replace('插件','内置功能')
 & $taskMsbuild (Join-Path $PSScriptRoot 'features/Feature.Service.vcxproj') /p:Configuration=Release /p:Platform=x64 ("/p:FeatureId="+$taskId) /verbosity:minimal
 if($LASTEXITCODE -ne 0){throw "Integrated service build failed: $taskId"}
 $taskServiceName="Hhub.$taskId.Service.dll"
 $taskService=Join-Path $taskSource "out/$taskServiceName"
 Copy-Item -LiteralPath $taskService -Destination (Join-Path $taskFolder $taskServiceName) -Force
 $taskManifest.implementation|Add-Member -NotePropertyName serviceLibrary -NotePropertyValue $taskServiceName -Force
 $taskManifest.nativeAssets+=@{path=$taskServiceName;sha256=(Get-FileHash -LiteralPath $taskService).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $taskService).Length}
 if($taskId -eq 'eq'){$taskManifest.navigation.title='耳机 EQ'}
 $taskManifests+=,$taskManifest
}
$taskJson=ConvertTo-Json -InputObject @($taskManifests) -Depth 50 -Compress
$taskLiterals=@()
for($taskOffset=0;$taskOffset -lt $taskJson.Length;$taskOffset+=3000){$taskLiterals+='LR"BUILTIN('+$taskJson.Substring($taskOffset,[Math]::Min(3000,$taskJson.Length-$taskOffset))+')BUILTIN"'}
$taskHeader='#pragma once'+"`n"+'inline wchar_t const* builtinFeatureManifests='+"`n"+($taskLiterals -join "`n")+';'+"`n"
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'BuiltinFeatureManifests.h'),$taskHeader,[Text.UTF8Encoding]::new($false))
Write-Output 'Three built-in features staged and pinned.'
