$ErrorActionPreference='Stop'
$output=Join-Path $PSScriptRoot 'out';New-Item -ItemType Directory -Force $output|Out-Null
$csc=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
& $csc ('/win32manifest:'+(Join-Path $PSScriptRoot 'app.manifest')) /nologo /target:exe /platform:x64 /optimize+ ('/out:'+(Join-Path $output 'Hhub.Phone.exe')) /reference:System.Web.Extensions.dll /reference:System.Security.dll /reference:Microsoft.CSharp.dll (Join-Path $PSScriptRoot 'PhoneServer.cs')
if($LASTEXITCODE -ne 0){throw 'Phone backend build failed'}
$msbuild='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe'
& $msbuild (Join-Path $PSScriptRoot 'Hhub.Phone.UI.vcxproj') /restore /p:Configuration=Release /p:Platform=x64 /p:RestoreSources=C:\Users\Administrator\.nuget\packages /p:RestoreIgnoreFailedSources=true /p:RestoreSources=C:\Users\Administrator\.nuget\packages /p:RestoreIgnoreFailedSources=true ('/p:OutDir='+$output+'\') /verbosity:minimal
if($LASTEXITCODE -ne 0){throw 'Phone page build failed'}
$manifest=Get-Content (Join-Path $PSScriptRoot 'manifest-template.json') -Raw -Encoding UTF8|ConvertFrom-Json
$assets=@();foreach($name in @('Hhub.Phone.UI.dll','Hhub.Phone.exe','phone-web/app.js','phone-web/index.html','phone-web/style.css','phone-worker.js','languages/en-US.json')){$file=if($name -in @('Hhub.Phone.exe','Hhub.Phone.UI.dll')){Join-Path $output $name}else{Join-Path $PSScriptRoot $name};$assets+=@{path=$name;encoding='base64';sha256=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant();data=[Convert]::ToBase64String([IO.File]::ReadAllBytes($file))}}
$manifest.nativeAssets=$assets
[IO.File]::WriteAllText((Join-Path $output 'Hhub-Phone-Remote-1.0.hhub'),($manifest|ConvertTo-Json -Depth 30 -Compress),[Text.UTF8Encoding]::new($false))
