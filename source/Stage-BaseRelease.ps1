param([string]$ApplicationDirectory=(Join-Path $PSScriptRoot '..\app'),[string]$Destination=(Join-Path $PSScriptRoot '..\release\Hhub-1.0\Hhub'))
$ErrorActionPreference='Stop'
$taskApp=[IO.Path]::GetFullPath($ApplicationDirectory);$taskStage=[IO.Path]::GetFullPath($Destination)
if(Test-Path -LiteralPath $taskStage){throw 'Destination already exists; use a new staging directory.'}
foreach($taskRequired in @('Hhub.exe','Hhub.Core.exe','Hhub.Launcher.exe','Hhub.Catalog.exe','Hhub.Update.exe')){if(-not (Test-Path -LiteralPath (Join-Path $taskApp $taskRequired))){throw ('Missing build output: '+$taskRequired)}}
New-Item -ItemType Directory -Path $taskStage -Force|Out-Null
$taskRuntime=Join-Path $taskStage 'runtime';New-Item -ItemType Directory -Path $taskRuntime|Out-Null
$taskRootExecutables=@('Hhub.exe','Hhub.Core.exe','Hhub.Catalog.exe','Hhub.Update.exe','Hhub.Launcher.exe','RestartAgent.exe')
foreach($taskFile in Get-ChildItem -LiteralPath $taskApp -Recurse -File){
 $taskRelative=$taskFile.FullName.Substring($taskApp.Length).TrimStart('\');$taskParts=$taskRelative.Split('\');$taskName=$taskFile.Name;if($taskName -in @('onnxruntime.dll','DirectML.dll')){continue}
 if($taskFile.Extension -in @('.pdb','.ilk','.lib','.hhub','.pfx','.key','.obj','.log','.bak') -or $taskRelative -match '(?i)(^|\\)(phone-web|native-plugin-library|native-components|plugin-ui|cache|logs|obj)(\\|$)' -or $taskRelative -match '(?i)^drivers\\audio\\' -or $taskName -match '(?i)hhub\.(phone|windows|audiotest)|hhubaudiodriversetup|dfxdsp\.dll|hhub\.eq\.dll|\.before|backup'){continue}
 if($taskParts.Count -eq 1 -and $taskFile.Extension -eq '.exe' -and $taskName -notin $taskRootExecutables){continue}
 if($taskParts.Count -eq 1 -and $taskFile.Extension -notin @('.exe','.dll','.winmd','.pri') -and $taskName -ne 'WindowsAppRuntime.png' -and $taskName -notmatch '^workloads(?:\.[A-Za-z0-9]+)?\.json$'){continue}
 if($taskParts.Count -gt 1 -and $taskParts[0] -match '^[a-z]{2,3}-' -and $taskParts[0] -notin @('en-us','zh-CN')){continue}
 if($taskName -in @('build-info.json','SHA256SUMS.txt','plugin-library-cache.json','plugin-library-source.json','update-source.json')){continue}
 $taskTarget=Join-Path $taskStage $taskRelative
 if($taskParts.Count -eq 1 -and ($taskFile.Extension -in @('.dll','.winmd','.pri') -or $taskName -in @('Hhub.exe','RestartAgent.exe','WindowsAppRuntime.png')) -or $taskParts[0] -in @('en-us','zh-CN','Microsoft.UI.Xaml','NpuDetect')){$taskTarget=Join-Path $taskRuntime $taskRelative}
 if($taskName -eq 'Hhub.Launcher.exe'){$taskTarget=Join-Path $taskStage 'Hhub.exe'}
 New-Item -ItemType Directory -Path (Split-Path -Parent $taskTarget) -Force|Out-Null;Copy-Item -LiteralPath $taskFile.FullName -Destination $taskTarget
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'languages') -Destination $taskStage -Recurse -Force
[IO.File]::WriteAllText((Join-Path $taskRuntime 'Hhub.runtime'),"Hhub UI runtime layout 1`n",[Text.UTF8Encoding]::new($false))
$taskRepo=Split-Path -Parent $PSScriptRoot
$taskExtra=@{'driver\pawnio\PawnIO_setup.exe'='drivers\pawnio\PawnIO_setup.exe';'driver\pawnio\COPYING'='licenses\PawnIO-GPL.txt';'driver\pawnio\README.md'='licenses\PawnIO-source-notice.md';'source\vendor\EarTrumpet-LICENSE.txt'='licenses\EarTrumpet-LICENSE.txt'}
foreach($taskEntry in $taskExtra.GetEnumerator()){$taskTarget=Join-Path $taskStage $taskEntry.Value;New-Item -ItemType Directory -Path (Split-Path $taskTarget) -Force|Out-Null;Copy-Item -LiteralPath (Join-Path $taskRepo $taskEntry.Key) -Destination $taskTarget -Force}
$taskUtf8=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $taskStage 'update-source.json'),'{"manifestUrl":"https://raw.githubusercontent.com/shengerq/H-hub/main/updates/stable.json"}',$taskUtf8)
[IO.File]::WriteAllText((Join-Path $taskStage 'plugin-library-source.json'),'{"name":"H hub plugins","repository":"shengerq/H-hub-plugins","indexUrl":"https://raw.githubusercontent.com/shengerq/H-hub-plugins/main/index.json"}',$taskUtf8)
$taskBinaries=@{};foreach($taskRelative in @('Hhub.exe','runtime\Hhub.exe','Hhub.Core.exe','Hhub.Catalog.exe','Hhub.Update.exe','sensors\Hhub.Sensors.exe')){$taskBinaries[$taskRelative.Replace('\','/')]=(Get-FileHash -LiteralPath (Join-Path $taskStage $taskRelative) -Algorithm SHA256).Hash.ToLowerInvariant()}
$taskInfo=@{applicationVersion='1.0';releaseEpoch=1;defaultPluginLibraries=@('shengerq/H-hub-plugins','shengerq/H-hub-app-plugins');edition='base';architecture='x64';languages=@('zh-CN','en-US');languageDirectory='languages';runtimeDirectory='runtime';bundledPluginPackages=@();bundledNativePluginComponents=@();binaries=$taskBinaries}
[IO.File]::WriteAllText((Join-Path $taskStage 'build-info.json'),($taskInfo|ConvertTo-Json -Depth 8),$taskUtf8)
[IO.File]::WriteAllText((Join-Path $taskStage 'Optional-plugins.md'),"# Optional plugins / 可选插件`n`nUniversal EQ, Windows settings and Phone Remote are downloaded separately from the plugin market. This base installer does not include their packages or native components.`n基础安装包不携带通用 EQ、Windows 设置和手机遥控。请从插件市场独立安装。升级保留已安装插件及其配置。`n",$taskUtf8)
[IO.File]::WriteAllText((Join-Path $taskStage '使用说明.txt'),"H hub 1.0`n`nOpen H hub using the installed shortcut or Hhub.exe. Language packs are in languages; the UI runtime is in runtime.`n使用快捷方式或 Hhub.exe 启动。应用语言包位于 languages，界面运行库位于 runtime。`n",$taskUtf8)
$taskSums=foreach($taskFile in Get-ChildItem -LiteralPath $taskStage -Recurse -File|Sort-Object FullName){(Get-FileHash -LiteralPath $taskFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant()+'  '+$taskFile.FullName.Substring($taskStage.Length+1).Replace('\','/')}
[IO.File]::WriteAllText((Join-Path $taskStage 'SHA256SUMS.txt'),(($taskSums -join "`n")+"`n"),$taskUtf8)
Write-Output $taskStage

