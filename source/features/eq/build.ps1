$ErrorActionPreference='Stop'
$taskVswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$taskMsbuild=& $taskVswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if(-not $taskMsbuild){throw 'Visual Studio C++ Build Tools required'}
$taskOutput=Join-Path $PSScriptRoot 'out';New-Item -ItemType Directory -Force $taskOutput|Out-Null
$taskResources="#pragma code_page(65001)`n"
$taskId=201
foreach($taskName in @('fxvad.inf','fxvad.sys','fxvadntamd64.cat','LICENSE.txt','license.rtf')){
 $taskFile=(Join-Path (Join-Path $PSScriptRoot 'driver\payload') $taskName).Replace('\','\\')
 $taskResources+=('{0} RCDATA "{1}"' -f $taskId,$taskFile)+"`n";$taskId++
}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'DriverBundle.rc'),$taskResources,[Text.UTF8Encoding]::new($true))
foreach($taskProject in @('Hhub.EQ.vcxproj','Hhub.AudioInstaller.vcxproj')){
 & $taskMsbuild (Join-Path $PSScriptRoot $taskProject) /restore /p:Configuration=Release /p:Platform=x64 /p:RestoreSources=C:\Users\Administrator\.nuget\packages /p:RestoreIgnoreFailedSources=true ("/p:OutDir="+$taskOutput+'\') /verbosity:minimal
 if($LASTEXITCODE -ne 0){throw "Build failed: $taskProject"}
}
$taskLanguages=Join-Path $taskOutput 'languages';New-Item -ItemType Directory -Force $taskLanguages|Out-Null
Copy-Item (Join-Path $PSScriptRoot 'languages/*.json') $taskLanguages -Force
$taskManifest=Get-Content (Join-Path $PSScriptRoot 'manifest-template.json') -Raw -Encoding UTF8|ConvertFrom-Json
$taskAssets=@()
foreach($taskName in @('Hhub.EQ.dll','HhubAudioDriverSetup.exe','languages/en-US.json','languages/zh-CN.json')){
 $taskFile=Join-Path $taskOutput $taskName;$taskHash=(Get-FileHash -LiteralPath $taskFile).Hash.ToLowerInvariant()
 $taskAssets+=@{path=$taskName;encoding='base64';sha256=$taskHash;data=[Convert]::ToBase64String([IO.File]::ReadAllBytes($taskFile))}
}
$taskManifest.nativeAssets=$taskAssets
$taskPackage=Join-Path $taskOutput 'Hhub-EQ-1.0.hhub'
[IO.File]::WriteAllText($taskPackage,($taskManifest|ConvertTo-Json -Depth 40 -Compress),[Text.UTF8Encoding]::new($false))
if((Get-Item -LiteralPath $taskPackage).Length -gt 8MB){throw 'Package exceeds host limit'}
Write-Output "Package: $taskPackage"
Write-Output 'The host does not need rebuilding.'