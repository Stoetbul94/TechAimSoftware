# Tech Aim - Android build environment check (milestone A1/A2).
#
# READ-ONLY. This script reports; it never installs, never sets a machine-global
# environment variable, and never modifies the SDK. If something is missing it
# says so and exits non-zero so a build script can refuse to continue.
#
#   powershell -File scripts\android\check-android-env.ps1
#
# Exit codes: 0 = everything required is present, 1 = something is missing.

[CmdletBinding()]
param()

$ErrorActionPreference = 'Continue'
$problems = New-Object System.Collections.Generic.List[string]

function Report([string]$label, [string]$value, [bool]$ok, [string]$hint = '') {
    $mark = if ($ok) { 'OK  ' } else { 'MISS' }
    Write-Host ("  [{0}] {1,-22} {2}" -f $mark, $label, $value)
    if (-not $ok) {
        $msg = "$label not found"
        if ($hint) { $msg += " - $hint" }
        $script:problems.Add($msg)
    }
}

Write-Host ''
Write-Host 'Tech Aim - Android environment check'
Write-Host '===================================='

# -- Qt Android kits ----
# Each Qt Android kit is ABI-specific. qmake-based projects build ONE ABI per
# invocation (Qt's multi-ABI single-APK support is CMake-only), so both kits
# are reported and the build script picks one.
$qtRoot = 'C:\Qt\6.5.3'
foreach ($abi in @(@('arm64-v8a','android_arm64_v8a'), @('x86_64','android_x86_64'))) {
    $p = Join-Path $qtRoot $abi[1]
    Report "Qt kit $($abi[0])" $p (Test-Path (Join-Path $p 'bin\qmake.bat'))
}
$hostQt = Join-Path $qtRoot 'mingw_64'
Report 'Qt host (mingw_64)' $hostQt (Test-Path (Join-Path $hostQt 'bin\qmake.exe'))
Report 'androiddeployqt' (Join-Path $hostQt 'bin\androiddeployqt.exe') `
       (Test-Path (Join-Path $hostQt 'bin\androiddeployqt.exe'))
Report 'mingw32-make' 'C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe' `
       (Test-Path 'C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe')

# -- Android SDK ----
$sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT }
       else { Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
Report 'Android SDK' $sdk (Test-Path $sdk)

if (Test-Path $sdk) {
    $platforms = @(Get-ChildItem (Join-Path $sdk 'platforms') -Directory -ErrorAction SilentlyContinue |
                   Select-Object -ExpandProperty Name)
    Report 'SDK platforms' ($platforms -join ', ') ($platforms.Count -gt 0)

    $bt = @(Get-ChildItem (Join-Path $sdk 'build-tools') -Directory -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty Name)
    Report 'SDK build-tools' ($bt -join ', ') ($bt.Count -gt 0)

    Report 'adb' (Join-Path $sdk 'platform-tools\adb.exe') `
           (Test-Path (Join-Path $sdk 'platform-tools\adb.exe'))

    # NDK. Qt 6.5 documents r25; 25.1.8937393 is what this project builds with.
    $ndkDir = Join-Path $sdk 'ndk'
    $ndks = @(Get-ChildItem $ndkDir -Directory -ErrorAction SilentlyContinue |
              Select-Object -ExpandProperty Name)
    Report 'Android NDK' ($ndks -join ', ') ($ndks.Count -gt 0) `
           "install with: sdkmanager `"ndk;25.1.8937393`""
}

# -- JDK ----
# Qt 6.5.3 ships Android Gradle Plugin 7.4.1, which supports JDK 11-17 and
# does NOT support JDK 21. The build must therefore point at a 17 explicitly
# rather than inheriting whatever JAVA_HOME happens to be. Android Studio's
# bundled JBR is a JDK 17 and is preferred when present, because using it
# means nothing has to be installed or changed machine-wide.
$jdkCandidates = @(
    'C:\Program Files\Android\Android Studio\jbr',
    'C:\Program Files\Eclipse Adoptium\jdk-17',
    $env:JAVA_HOME_17
) | Where-Object { $_ }

# Read the version from the JDK's `release` file, not by running java. In
# Windows PowerShell 5.1 `2>&1` on a native exe wraps stderr lines in
# ErrorRecords - and java prints its version banner to stderr - which turns a
# successful probe into a scripted error.
# NOTE: the parameter is $jdkPath, not $home - $HOME is a read-only
# PowerShell automatic variable and binding it here fails at runtime.
function Get-JdkMajor([string]$jdkPath) {
    $rel = Join-Path $jdkPath 'release'
    if (-not (Test-Path $rel)) { return 0 }
    $line = Select-String -Path $rel -Pattern '^JAVA_VERSION="([^"]+)"' -ErrorAction SilentlyContinue |
            Select-Object -First 1
    if (-not $line) { return 0 }
    $v = $line.Matches[0].Groups[1].Value
    if ($v -match '^(\d+)') { return [int]$Matches[1] }
    return 0
}

$jdk17 = $null
foreach ($c in $jdkCandidates) {
    if ((Test-Path (Join-Path $c 'bin\java.exe')) -and (Get-JdkMajor $c) -eq 17) {
        $jdk17 = $c; break
    }
}
Report 'JDK 17 (for Gradle)' ($(if ($jdk17) { $jdk17 } else { '<none>' })) ($null -ne $jdk17) `
       'AGP 7.4.1 requires JDK 11-17; JDK 21 will fail'

if ($env:JAVA_HOME) {
    Write-Host "  [note] JAVA_HOME is set to '$env:JAVA_HOME' - the build script overrides it for its own process only."
} else {
    Write-Host '  [note] JAVA_HOME is not set machine-wide. That is fine; the build script sets it per-process.'
}

# -- Devices ----
if (Test-Path (Join-Path $sdk 'platform-tools\adb.exe')) {
    Write-Host ''
    Write-Host 'Connected devices (adb):'
    $devices = & (Join-Path $sdk 'platform-tools\adb.exe') devices 2>&1 | Select-Object -Skip 1 |
               Where-Object { $_ -match '\S' }
    if ($devices) {
        $devices | ForEach-Object { Write-Host "  $_" }
        Write-Host '  NOTE: an arm64-v8a APK will NOT install on an x86_64 emulator. Check the ABI.'
    } else {
        Write-Host '  (none)'
    }
}

Write-Host ''
if ($problems.Count -eq 0) {
    Write-Host 'RESULT: environment complete.' -ForegroundColor Green
    exit 0
}
Write-Host 'RESULT: environment INCOMPLETE' -ForegroundColor Yellow
$problems | ForEach-Object { Write-Host "  - $_" }
exit 1
