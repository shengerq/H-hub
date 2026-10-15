param([string]$Configuration = 'Release',[string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$taskOriginalApp=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../app'))
$taskAppDir=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{$taskOriginalApp}
if($taskAppDir -ne $taskOriginalApp){New-Item -ItemType Directory -Force $taskAppDir | Out-Null;Copy-Item (Join-Path $taskOriginalApp '*') $taskAppDir -Recurse -Force}
$ErrorActionPreference = 'Stop'
$taskSensorDir=Join-Path $taskAppDir 'sensors'
$taskCsc=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if(-not (Test-Path -LiteralPath (Join-Path $taskSensorDir 'LibreHardwareMonitorLib.dll'))){throw '缺少官方 LibreHardwareMonitor 0.9.6 运行库：app/sensors'}
& $taskCsc /nologo /target:exe /platform:x64 /optimize+ ("/out:"+(Join-Path $taskSensorDir 'Hhub.Sensors.exe')) ("/reference:"+(Join-Path $taskSensorDir 'LibreHardwareMonitorLib.dll')) /reference:System.Web.Extensions.dll /reference:System.Management.dll /reference:System.Core.dll (Join-Path $PSScriptRoot 'HardwareMonitoring.cs') (Join-Path $PSScriptRoot 'Sensors.cs')
if($LASTEXITCODE -ne 0){throw '传感器组件构建失败'}
$taskPhoneExe=Join-Path $taskAppDir 'Hhub.Phone.exe'
& $taskCsc /nologo /target:exe /platform:x64 /optimize+ ("/out:"+$taskPhoneExe) /reference:System.Web.Extensions.dll /reference:System.Security.dll /reference:Microsoft.CSharp.dll (Join-Path $PSScriptRoot 'PhoneServer.cs')
if($LASTEXITCODE -ne 0){throw '手机连接服务构建失败'}
$taskUpdateExe=Join-Path $taskAppDir 'Hhub.Update.exe'
& $taskCsc /nologo /target:exe /platform:x64 /optimize+ ("/out:"+$taskUpdateExe) /reference:System.Web.Extensions.dll (Join-Path $PSScriptRoot 'UpdateService.cs')
if($LASTEXITCODE -ne 0){throw '更新服务构建失败'}
$taskCatalogExe=Join-Path $taskAppDir 'Hhub.Catalog.exe'
& $taskCsc /nologo /target:exe /platform:x64 /optimize+ ("/out:"+$taskCatalogExe) /reference:System.Web.Extensions.dll (Join-Path $PSScriptRoot 'PluginCatalog.cs') (Join-Path $PSScriptRoot 'PluginContract.cs') /main:PluginCatalog
if($LASTEXITCODE -ne 0){throw 'Plugin catalog build failed'}
$taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $taskVswhere)) { throw '请先安装 Visual Studio C++ Build Tools 与 Windows SDK。' }
$taskMsbuild = & $taskVswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if (-not $taskMsbuild) { throw '未找到 MSBuild。' }
foreach ($taskProject in @('Hhub.Core.vcxproj','Hhub.AudioTest.vcxproj','Hhub.Windows.vcxproj','Hhub.vcxproj')) {
    & $taskMsbuild (Join-Path $PSScriptRoot $taskProject) /restore /m "/p:Configuration=$Configuration" /p:Platform=x64 ("/p:OutDir="+$taskAppDir+[IO.Path]::DirectorySeparatorChar) /verbosity:minimal
    if ($LASTEXITCODE -ne 0) { throw "构建失败：$taskProject" }
}
$taskVsRoot = & $taskVswhere -latest -products '*' -property installationPath
$taskRedist = Get-ChildItem -LiteralPath (Join-Path $taskVsRoot 'VC\Redist\MSVC') -Directory | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'x64\Microsoft.VC143.CRT') } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if ($taskRedist) {
    $taskCrt = Join-Path $taskRedist.FullName 'x64\Microsoft.VC143.CRT'
    if (Test-Path -LiteralPath $taskCrt) { Get-ChildItem -LiteralPath $taskCrt -Filter '*.dll' | Copy-Item -Destination ($taskAppDir) -Force }
}
$taskAssets=Join-Path $taskAppDir 'assets'
if(-not (Test-Path -LiteralPath $taskAssets)){throw '缺少 app/assets 产品图目录'}
Write-Output ('应用目录：' + (Resolve-Path $taskAppDir).Path)
