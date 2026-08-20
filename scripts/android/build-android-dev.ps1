# Tech Aim - Android development APK build (milestone A1/A2).
#
#   powershell -File scripts\android\build-android-dev.ps1 [-Abi arm64-v8a|x86_64] [-Clean]
#
# Produces a DEBUG-SIGNED development APK. This is not a release process:
# there is no production key, nothing is uploaded, and the version string
# carries an ANDROID development channel so the artefact can never be mistaken
# for the frozen Windows release candidate.
#
# ONE ABI PER RUN. Qt's multi-ABI single-APK support is CMake-only; a qmake
# project builds against one ABI-specific Qt kit at a time. Run the script
# twice to produce both APKs (arm64-v8a for the tablet, x86_64 for the
# emulator).
#
# The script sets JAVA_HOME for ITS OWN PROCESS ONLY. It never modifies a
# machine-global environment variable, and it never installs anything.

[CmdletBinding()]
param(
    [ValidateSet('arm64-v8a','x86_64')]
    [string]$Abi = 'arm64-v8a',
    [switch]$Clean,
    [string]$ExpectedBranch = 'feature/android-tablet',
    # Milestone label used for the dist folder and the APK filename. Bump this
    # per milestone so artefacts from different qualification rounds can never
    # be confused with each other.
    [string]$Milestone = 'A2_5'
)

$ErrorActionPreference = 'Stop'
function Fail([string]$m) { Write-Host "FAIL: $m" -ForegroundColor Red; exit 1 }
function Step([string]$m) { Write-Host ''; Write-Host "==> $m" -ForegroundColor Cyan }

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $repo

# -- 1. Branch guard ----
# Android platform changes must not be built from, or mistaken for, a product
# or foundation branch.
Step 'Verifying branch'
$branch = (& git rev-parse --abbrev-ref HEAD).Trim()
Write-Host "  branch: $branch"
if ($branch -ne $ExpectedBranch) {
    Fail "expected '$ExpectedBranch' but this worktree is on '$branch'. Pass -ExpectedBranch to override deliberately."
}
$commit = (& git rev-parse --short HEAD).Trim()
$dirty  = (& git status --porcelain)
Write-Host "  commit: $commit"
if ($dirty) { Write-Host '  NOTE: working tree has uncommitted changes.' -ForegroundColor Yellow }

# -- 2. Toolchain ----
Step 'Resolving toolchain'
$kitDir = if ($Abi -eq 'arm64-v8a') { 'android_arm64_v8a' } else { 'android_x86_64' }
$qtKit  = "C:\Qt\6.5.3\$kitDir"
$qmake  = Join-Path $qtKit 'bin\qmake.bat'
if (-not (Test-Path $qmake)) { Fail "Qt Android kit not found: $qtKit" }

$make = 'C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe'
if (-not (Test-Path $make)) { Fail "mingw32-make not found: $make" }

$sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
if (-not (Test-Path $sdk)) { Fail "Android SDK not found: $sdk" }

$ndkRoot = Join-Path $sdk 'ndk'
$ndk = Get-ChildItem $ndkRoot -Directory -ErrorAction SilentlyContinue |
       Sort-Object Name -Descending | Select-Object -First 1
if (-not $ndk) { Fail "No Android NDK under $ndkRoot. Install with: sdkmanager `"ndk;25.1.8937393`"" }

# AGP 7.4.1 (shipped by Qt 6.5.3) supports JDK 11-17 and fails on JDK 21.
# Prefer Android Studio's bundled JBR so nothing has to be installed.
# Version is read from the JDK's own `release` file rather than by running
# `java -version`. In Windows PowerShell 5.1, redirecting a native
# executable's stderr (`2>&1`) wraps each line in an ErrorRecord and trips
# $ErrorActionPreference='Stop' even on success - java writes its version
# banner to stderr, so probing it that way aborts this script.
# NOTE: the parameter is $jdkPath, not $home - $HOME is a read-only
# PowerShell automatic variable and binding it here fails at runtime.
function Get-JdkMajor([string]$jdkPath) {
    $rel = Join-Path $jdkPath 'release'
    if (-not (Test-Path $rel)) { return 0 }
    $line = Select-String -Path $rel -Pattern '^JAVA_VERSION="([^"]+)"' -ErrorAction SilentlyContinue |
            Select-Object -First 1
    if (-not $line) { return 0 }
    $v = $line.Matches[0].Groups[1].Value      # e.g. 17.0.7
    if ($v -match '^(\d+)') { return [int]$Matches[1] }
    return 0
}

$jdk17 = @(
    'C:\Program Files\Android\Android Studio\jbr',
    'C:\Program Files\Eclipse Adoptium\jdk-17',
    $env:JAVA_HOME_17
) | Where-Object { $_ -and (Test-Path (Join-Path $_ 'bin\java.exe')) } |
    Where-Object { (Get-JdkMajor $_) -eq 17 } | Select-Object -First 1
if (-not $jdk17) { Fail 'No JDK 17 found. Qt 6.5.3 uses Android Gradle Plugin 7.4.1, which does not support JDK 21.' }

Write-Host "  Qt kit : $qtKit"
Write-Host "  SDK    : $sdk"
Write-Host "  NDK    : $($ndk.FullName)"
Write-Host "  JDK 17 : $jdk17"

# Process-scoped only. Deliberately NOT [Environment]::SetEnvironmentVariable.
$env:ANDROID_SDK_ROOT = $sdk
$env:ANDROID_NDK_ROOT = $ndk.FullName
$env:JAVA_HOME        = $jdk17

# CRITICAL: remove Git's Unix tools from PATH for this build.
#
# If qmake can find sh.exe it decides the Makefile will be interpreted by a
# Unix shell and emits MSYS-style paths (/C/Users/...) throughout. The compile
# and link still succeed, but the install step then writes the .so to a path
# androiddeployqt cannot resolve, and packaging dies with:
#     Cannot find application binary in build dir .../libs/arm64-v8a/libTechAim_arm64-v8a.so
# Git for Windows puts sh.exe in usr\bin and is on PATH on this machine, so
# this filter is what makes the APK step work at all.
#
# Scoped to this process: the developer's PATH is untouched.
$env:PATH = (($env:PATH -split ';') |
             Where-Object { $_ -and ($_ -notmatch '\\Git\\usr\\bin') -and ($_ -notmatch '\\Git\\bin') } |
             ForEach-Object { $_ }) -join ';'
$env:PATH = "C:\Qt\Tools\mingw1120_64\bin;$env:PATH"
if (Get-Command sh.exe -ErrorAction SilentlyContinue) {
    Fail 'sh.exe is still on PATH; qmake would generate MSYS paths and APK packaging would fail.'
}

# -- 3. Configure + build ----
$buildDir = Join-Path $repo "build-android-$Abi"
if ($Clean -and (Test-Path $buildDir)) {
    Step "Cleaning $buildDir"
    Remove-Item $buildDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Step "Running qmake ($Abi)"
Push-Location $buildDir
try {
    & $qmake (Join-Path $repo 'Seta.pro') -spec android-clang 'CONFIG+=release'
    if ($LASTEXITCODE -ne 0) { Fail "qmake failed ($LASTEXITCODE)" }

    Step "Compiling native library ($Abi)"
    & $make -j8
    if ($LASTEXITCODE -ne 0) { Fail "compile failed ($LASTEXITCODE)" }

    Step 'Packaging APK (androiddeployqt + Gradle)'
    & $make apk
    if ($LASTEXITCODE -ne 0) { Fail "APK packaging failed ($LASTEXITCODE)" }
} finally {
    Pop-Location
}

# -- 4. Collect the artefact ----
Step 'Collecting APK'
$apk = Get-ChildItem (Join-Path $buildDir 'android-build\build\outputs\apk') -Recurse -Filter '*.apk' -ErrorAction SilentlyContinue |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $apk) { Fail "no APK found under $buildDir\android-build\build\outputs\apk" }

$distDir = Join-Path $repo "dist\TechAim-Android-$Milestone"
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
$outName = "TechAim-Android-$Milestone-$Abi.apk"
$outPath = Join-Path $distDir $outName
Copy-Item $apk.FullName $outPath -Force

$sha = (Get-FileHash $outPath -Algorithm SHA256).Hash.ToLower()

Write-Host ''
Write-Host '================ BUILD COMPLETE ================' -ForegroundColor Green
Write-Host "  Milestone: $Milestone"
Write-Host "  ABI      : $Abi"
Write-Host "  Branch   : $branch"
Write-Host "  Commit   : $commit"
Write-Host "  Built    : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Host "  Source   : $($apk.FullName)"
Write-Host "  Artefact : $outPath"
Write-Host "  Size     : $([math]::Round((Get-Item $outPath).Length / 1MB, 2)) MB"
Write-Host "  SHA256   : $sha"
Write-Host "  Signing  : DEBUG (development only - not for distribution)"
Write-Host '==============================================='
