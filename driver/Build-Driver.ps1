param([string]$MSBuild='C:\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe',[string]$KitVersion='10.0.26100.0')
$ErrorActionPreference='Stop'
$driverProject=Join-Path $PSScriptRoot 'fxvad\pcmex\vadpcmex.vcxproj'
$driverOutput=Join-Path $PSScriptRoot '..\build\audio-driver'
New-Item -ItemType Directory -Path $driverOutput -Force | Out-Null
$driverCache=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\work\wdk-nuget'))
$driverArgs=@('/m:1','/p:Configuration=Release','/p:Platform=x64',"/p:WindowsTargetPlatformVersion=$KitVersion", "/p:OutDir=$driverOutput\",'/p:SignMode=Off','/p:PreferredToolArchitecture=x64','/p:RCToolPath=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\','/verbosity:minimal')
if(Test-Path "$driverCache\vc\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0\Toolset.props") {
 $driverArgs+=@("/p:VCTargetsPath=$driverCache\vc\", "/p:WDKContentRoot=$driverCache\wdk\c\",'/p:VCInstallDir=C:\BuildTools\VC\',"/p:VC_LibraryPath_VC_Desktop_CurrentPlatform_spectre=$driverCache\spectre\Contents\VC\Tools\MSVC\14.44.35207\lib\spectre\x64\", "/p:InfToolPath=$driverCache\wdk\c\bin\$KitVersion\x64\", "/p:Inf2CatToolPath=$driverCache\wdk\c\bin\$KitVersion\x86\")
}
& $MSBuild $driverProject @driverArgs
if($LASTEXITCODE -ne 0){throw 'Driver build failed. Install the matching WDK and Visual Studio Windows Driver Kit component; NuGet headers alone do not provide the kernel platform toolset.'}
Write-Output 'Unsigned development build completed. Production signing and playback validation remain required.'


