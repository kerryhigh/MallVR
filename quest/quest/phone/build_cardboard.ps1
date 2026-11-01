param(
    [string]$GameFiles = '',
    [string]$AndroidSdk = '',
    [string]$SdlDir = '',
    [string]$Gl4esDir = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$workRoot = (Resolve-Path (Join-Path $repoRoot '..')).Path
$gradleHome = Join-Path $workRoot 'gradle-home'
New-Item -ItemType Directory -Path $gradleHome -Force | Out-Null
$env:GRADLE_USER_HOME = $gradleHome
$androidTemp = Join-Path $workRoot 'android-temp'
New-Item -ItemType Directory -Path $androidTemp -Force | Out-Null
$env:TEMP = $androidTemp
$env:TMP = $androidTemp
$env:TMPDIR = $androidTemp
$env:ANDROID_USER_HOME = Join-Path $workRoot 'android-user'
New-Item -ItemType Directory -Path $env:ANDROID_USER_HOME -Force | Out-Null
$env:MAKEFLAGS = '-j4'

if (-not $AndroidSdk) {
    if ($env:ANDROID_HOME) { $AndroidSdk = $env:ANDROID_HOME }
    elseif (Test-Path 'D:\AndroidDevelopment\Sdk') { $AndroidSdk = 'D:\AndroidDevelopment\Sdk' }
    else { throw 'Set -AndroidSdk or ANDROID_HOME to the installed Android SDK.' }
}
if (-not $GameFiles) {
    throw 'Pass -GameFiles <path to your MallVR games folder>. Game files are not included in this repository.'
}
if (-not $SdlDir) { $SdlDir = Join-Path $workRoot 'deps\SDL2' }
if (-not $Gl4esDir) { $Gl4esDir = Join-Path $workRoot 'deps\gl4es' }
foreach ($path in @($AndroidSdk, $GameFiles, $SdlDir, $Gl4esDir)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required path does not exist: $path" }
}

$knownJdk = 'C:\Program Files\Eclipse Adoptium\jdk-17.0.20.101-hotspot'
if (Test-Path -LiteralPath $knownJdk) { $env:JAVA_HOME = $knownJdk }
$env:ANDROID_HOME = $AndroidSdk
$env:ANDROID_SDK_ROOT = $AndroidSdk

$gradleArgs = @(
    '--no-daemon', '--console=plain', '--stacktrace', ':app:assembleDebug',
    "-PsdlDir=$SdlDir", "-Pgl4esDir=$Gl4esDir", "-PgameFiles=$GameFiles"
)
Push-Location $PSScriptRoot
try {
    & '.\gradlew.bat' @gradleArgs
    if ($LASTEXITCODE -ne 0) { throw "Gradle build failed with exit code $LASTEXITCODE" }

    $builtApk = Join-Path $PSScriptRoot 'app\build\outputs\apk\debug\app-debug.apk'
    if (-not (Test-Path -LiteralPath $builtApk)) { throw "Gradle reported success but no APK was created at $builtApk" }
    $outputDir = Join-Path $repoRoot '..\..\outputs'
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    $outputApk = Join-Path $outputDir 'MallVR-Cardboard-PacMan-debug.apk'
    Copy-Item -LiteralPath $builtApk -Destination $outputApk -Force
    $hash = Get-FileHash -LiteralPath $outputApk -Algorithm SHA256
    Write-Output "APK: $outputApk"
    Write-Output "SHA-256: $($hash.Hash)"
    Write-Output "Size: $((Get-Item -LiteralPath $outputApk).Length) bytes"
}
finally {
    Pop-Location
}
