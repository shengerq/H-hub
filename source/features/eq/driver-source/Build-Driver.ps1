param([string]$MSBuild,[string]$KitVersion='10.0.26100.0',[string]$OutputRoot=(Join-Path $PSScriptRoot 'build'),[string]$ToolchainRoot,[switch]$CheckOnly)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'Test-DriverIdentity.ps1') | Out-Host
if(-not $MSBuild) {
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if(Test-Path $vswhere) {
        $MSBuild=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\Current\Bin\amd64\MSBuild.exe' | Select-Object -First 1
    }
}
if(-not $MSBuild -or -not (Test-Path -LiteralPath $MSBuild)){throw '64-bit Visual Studio MSBuild with C++ tools was not found. Supply -MSBuild explicitly.'}
$vsRoot=[IO.Path]::GetFullPath((Join-Path (Split-Path $MSBuild) '..\..\..\..'))
$toolset=Join-Path $vsRoot 'MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0\Toolset.props'
$kitRoot=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$wdkRoot=$kitRoot
$portableArgs=@()
if($ToolchainRoot) {
    $ToolchainRoot=(Resolve-Path -LiteralPath $ToolchainRoot).Path
    $wdkRoot=Join-Path $ToolchainRoot 'microsoft.windows.wdk.x64\c'
    $overlay=Join-Path $ToolchainRoot 'wdk-build\Contents\MSBuild\Microsoft\VC\v170'
    $privateVc=Join-Path $ToolchainRoot 'vc'
    if(-not (Test-Path (Join-Path $privateVc 'Microsoft.Cpp.props'))) {
        Copy-Item -LiteralPath (Join-Path $vsRoot 'MSBuild\Microsoft\VC\v170') -Destination $privateVc -Recurse
    }
    Get-ChildItem -LiteralPath $overlay | Copy-Item -Destination $privateVc -Recurse -Force
    $toolset=Join-Path $privateVc 'Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0\Toolset.props'
    $spectre=@(Get-ChildItem (Join-Path $ToolchainRoot 'spectre\Contents\VC\Tools\MSVC') -Directory | ForEach-Object {Join-Path $_.FullName 'lib\spectre\x64'})
    if($spectre.Count -ne 1 -or -not (Test-Path (Join-Path $spectre[0] 'libcmt.lib'))){throw 'Expected exactly one extracted x64 Spectre library package.'}
    $inf2cat=Join-Path $ToolchainRoot "microsoft.windows.wdk.x86\c\bin\$KitVersion\x86"
    if(-not (Test-Path (Join-Path $inf2cat 'Inf2Cat.exe'))){throw 'The matching Microsoft.Windows.WDK.x86 package is required for Inf2Cat.'}
    $portableArgs=@("/p:VCTargetsPath=$privateVc\", "/p:VCInstallDir=$vsRoot\VC\", "/p:WDKContentRoot=$wdkRoot\", "/p:VC_LibraryPath_VC_Desktop_CurrentPlatform_spectre=$($spectre[0])\", "/p:InfToolPath=$wdkRoot\bin\$KitVersion\x64\", "/p:Inf2CatToolPath=$inf2cat\")
}
$required=@($toolset,(Join-Path $wdkRoot "Include\$KitVersion\km\ntddk.h"),(Join-Path $wdkRoot "Lib\$KitVersion\km\x64\portcls.lib"))
$missing=@($required | Where-Object {-not (Test-Path -LiteralPath $_)})
if($missing.Count){throw ("Matching WDK/SDK and Visual Studio Windows Driver Kit component are required. Missing:`n"+($missing -join "`n"))}
if($CheckOnly){Write-Output 'Identity and build prerequisites passed.'; return}
$OutputRoot=[IO.Path]::GetFullPath($OutputRoot)
# Each build gets an empty destination to prevent stale upstream output being packaged.
$run=Join-Path $OutputRoot ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $run | Out-Null
$driverProject=Join-Path $PSScriptRoot 'fxvad\pcmex\vadpcmex.vcxproj'
$driverArgs=@('/m:1','/t:Rebuild','/p:Configuration=Release','/p:Platform=x64',"/p:WindowsTargetPlatformVersion=$KitVersion", "/p:OutDir=$run\out\", "/p:IntDir=$run\obj\",'/p:SignMode=Off','/p:PreferredToolArchitecture=x64','/verbosity:minimal')
$driverArgs+=$portableArgs
$driverArgs+="/p:RCToolPath=$kitRoot\bin\$KitVersion\x64\"
& $MSBuild $driverProject @driverArgs
if($LASTEXITCODE -ne 0){throw "Driver build failed. Output is not ready: $run"}
$inf=@(Get-ChildItem (Join-Path $run 'out') -Recurse -Filter hhubvad.inf)
$packages=@($inf | Where-Object {(Test-Path (Join-Path $_.DirectoryName 'hhubvad.sys')) -and (Test-Path (Join-Path $_.DirectoryName 'hhubvadntamd64.cat'))})
if($packages.Count -ne 1){throw 'Expected exactly one newly built Hhub SYS/INF/CAT package.'}
$package=$packages[0].DirectoryName
foreach($name in @('DRIVER-NOTICE.txt','LICENSE','license.rtf')) {Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $package $name)}
$record=[ordered]@{schemaVersion=2; builtUtc=[DateTime]::UtcNow.ToString('o'); package=$package; productionSigned=$false; files=@{}}
foreach($name in @('hhubvad.inf','hhubvad.sys','hhubvadntamd64.cat','DRIVER-NOTICE.txt','LICENSE','license.rtf')) {$record.files[$name]=(Get-FileHash (Join-Path $package $name) -Algorithm SHA256).Hash}
$record | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $package 'build-record.json') -Encoding UTF8
Write-Output "Unsigned development package: $package"
