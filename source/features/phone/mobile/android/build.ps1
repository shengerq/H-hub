param(
    [string]$AndroidJar = "$PSScriptRoot\..\..\..\work\android-sdk\platform\android-36\android.jar",
    [string]$BuildTools = "$PSScriptRoot\..\..\..\work\android-build-tools\tools\android-16",
    [string]$SigningKey = "$PSScriptRoot\..\..\..\work\hhub-android-debug.p12",
    [string]$ZxingJar = "$PSScriptRoot\..\..\..\work\zxing-core-3.5.4.jar"
)
$ErrorActionPreference='Stop'
function Check-Exit([string]$Step){if($LASTEXITCODE -ne 0){throw "$Step failed: $LASTEXITCODE"}}
$AndroidJar=[IO.Path]::GetFullPath($AndroidJar)
$BuildTools=[IO.Path]::GetFullPath($BuildTools)
$SigningKey=[IO.Path]::GetFullPath($SigningKey)
$ZxingJar=[IO.Path]::GetFullPath($ZxingJar)
if(-not (Test-Path -LiteralPath $ZxingJar)){throw 'Missing ZXing core 3.5.4; pass -ZxingJar'}
if((Get-FileHash -LiteralPath $ZxingJar -Algorithm SHA256).Hash -ne '71DE5D89341B5FCF5DD89DA7F44E84D825D0E084CDF3EC77C9ABE26B0F0CEB13'){throw 'ZXing jar SHA-256 mismatch'}
if(-not (Test-Path -LiteralPath $AndroidJar)){throw 'Missing Android SDK platform android.jar; pass -AndroidJar'}
if(-not (Test-Path -LiteralPath (Join-Path $BuildTools 'aapt2.exe'))){throw 'Missing Android SDK build tools; pass -BuildTools'}
$taskBuild=Join-Path $PSScriptRoot 'build'
$taskClasses=Join-Path $taskBuild 'classes'
$taskDex=Join-Path $taskBuild 'dex'
$taskClassTarget=[IO.Path]::GetFullPath($taskClasses)
$taskBuildRoot=[IO.Path]::GetFullPath($taskBuild)+[IO.Path]::DirectorySeparatorChar
if(-not $taskClassTarget.StartsWith($taskBuildRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Class cache is outside the build directory'}
if(Test-Path -LiteralPath $taskClassTarget){
    if((Get-Item -LiteralPath $taskClassTarget -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Class cache must not be a linked directory'}
    Remove-Item -LiteralPath $taskClassTarget -Recurse -Force
}
New-Item -ItemType Directory -Force $taskClasses,$taskDex | Out-Null
$taskSources=Get-ChildItem (Join-Path $PSScriptRoot 'src') -Recurse -Filter '*.java' | ForEach-Object {$_.FullName}
& javac -encoding UTF-8 --release 8 -classpath ($AndroidJar+';'+$ZxingJar) -d $taskClasses @taskSources
Check-Exit 'Java compile'
$taskClassesJar=Join-Path $taskBuild 'classes.jar'
& jar cf $taskClassesJar -C $taskClasses .
Check-Exit 'Class package'
& java -cp (Join-Path $BuildTools 'lib\d8.jar') com.android.tools.r8.D8 --min-api 26 --lib $AndroidJar --output $taskDex $taskClassesJar $ZxingJar
Check-Exit 'DEX compile'
$taskUnsigned=Join-Path $taskBuild 'unsigned.apk'
$taskResources=Join-Path $taskBuild 'resources.zip'
& (Join-Path $BuildTools 'aapt2.exe') compile --dir (Join-Path $PSScriptRoot 'res') -o $taskResources
Check-Exit 'Resource compile'
& (Join-Path $BuildTools 'aapt2.exe') link -I $AndroidJar $taskResources --manifest (Join-Path $PSScriptRoot 'AndroidManifest.xml') --version-code 4 --version-name '0.4.0' -o $taskUnsigned
Check-Exit 'Android package'
& jar uf $taskUnsigned -C $taskDex classes.dex
Check-Exit 'DEX package'
& jar uf $taskUnsigned -C $PSScriptRoot assets
Check-Exit 'License assets'
$taskAligned=Join-Path $taskBuild 'aligned.apk'
& (Join-Path $BuildTools 'zipalign.exe') -f -p 4 $taskUnsigned $taskAligned
Check-Exit 'ZIP align'
if(-not (Test-Path -LiteralPath $SigningKey)){
    New-Item -ItemType Directory -Force (Split-Path $SigningKey) | Out-Null
    & keytool -genkeypair -keystore $SigningKey -storepass android -keypass android -alias hhub-debug -keyalg RSA -keysize 2048 -validity 3650 -dname 'CN=H hub Android Development' -storetype PKCS12
    Check-Exit 'Development key'
}
$taskApk=Join-Path $taskBuild 'Hhub-Android-0.4.0.apk'
& java -jar (Join-Path $BuildTools 'lib\apksigner.jar') sign --ks $SigningKey --ks-key-alias hhub-debug --ks-pass pass:android --key-pass pass:android --out $taskApk $taskAligned
Check-Exit 'APK sign'
& java -jar (Join-Path $BuildTools 'lib\apksigner.jar') verify --verbose $taskApk
Check-Exit 'APK verify'
& (Join-Path $BuildTools 'aapt2.exe') dump badging $taskApk
Check-Exit 'APK manifest verify'
Write-Output "APK: $taskApk"
