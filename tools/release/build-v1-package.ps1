# Tech Aim Single Target — Windows v1.0 portable package builder.
#
# Produces a folder that runs on a machine with NO Qt, NO repository and NO
# MinGW/Visual Studio, then zips it and records the SHA-256 of both the ZIP and
# the executable.
#
# Built on the RC1 packager's proven deployment logic — the same explicit
# allow-list, the same freshly written release config, the same scrub — and
# differs in exactly two ways: the version, and the document set. A v1.0
# package carries the release gate and the evidence-inheritance record, because
# anyone holding this ZIP must be able to see what it was and was not tested
# against without asking anybody.
#
#   powershell -File tools\release\build-v1-package.ps1
[CmdletBinding()]
param(
    [string]$QtBin   = 'C:\Qt\6.5.3\mingw_64\bin',
    [string]$OutRoot = ''
)
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exe  = Join-Path $repo 'release\TechAim.exe'
if (-not (Test-Path $exe)) { throw "Not built: $exe. Run qmake + mingw32-make -f Makefile.Release first." }
if (-not (Test-Path (Join-Path $QtBin 'windeployqt.exe'))) { throw "windeployqt not found in $QtBin" }

$version = '1.0.0'
$name    = "TechAim-$version-Windows-x64"
# A NEW folder. dist\v1.0.0-rc1 is preserved as historical evidence and is
# never overwritten by this script.
if ([string]::IsNullOrWhiteSpace($OutRoot)) { $OutRoot = Join-Path $repo 'dist\v1.0.0' }
$pkg = Join-Path $OutRoot $name
$zip = Join-Path $OutRoot "$name.zip"

# The binary must actually BE the version this script claims. An incremental
# build silently keeps a stale APP_VERSION_STR and APP_GIT_SHA, which is how a
# package ends up mislabelled; refuse rather than ship a lie.
$bytes = [System.IO.File]::ReadAllBytes($exe)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
if ($ascii -notmatch [regex]::Escape($version)) {
    throw "The built TechAim.exe does not contain '$version'. Rebuild clean (remove release\main.o) before packaging."
}
$sha = (git -C $repo rev-parse --short HEAD)
if ($ascii -notmatch [regex]::Escape($sha)) {
    throw "The built TechAim.exe does not contain the current commit '$sha'. Rebuild clean before packaging."
}

Write-Host "== Tech Aim Single Target $version =="
Write-Host "   repo    : $repo"
Write-Host "   commit  : $sha"
Write-Host "   package : $pkg"

# ---- 1. clean slate ------------------------------------------------------
if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
if (Test-Path $zip) { Remove-Item $zip -Force }
New-Item -ItemType Directory -Force $pkg | Out-Null

# ---- 2. the binary -------------------------------------------------------
Copy-Item $exe (Join-Path $pkg 'TechAim.exe')

# ---- 3. a RELEASE configuration template ---------------------------------
# Written fresh, never copied: no local mode, developer flag or machine value
# may leak out of a developer checkout into a shipped package.
$cfg = @'
[shot_count_and_timer]
timer=yes

[App_Settings]
app_mode=Live
developer_mode=0
is_single_decimal=1
motor_movement_time=1
motor_movement_time_sighter=1
'@
[System.IO.File]::WriteAllText((Join-Path $pkg 'config.ini'), ($cfg -replace "`r`n", "`n"),
                               (New-Object System.Text.UTF8Encoding($false)))

# ---- 4. the documents a v1.0 holder needs --------------------------------
# What this build is, what it was tested against, and what it does NOT claim.
$docsOut = Join-Path $pkg 'docs'
New-Item -ItemType Directory -Force $docsOut | Out-Null
$docSet = @(
    'docs\release\TECH-AIM-SINGLE-TARGET-V1.0-RELEASE-GATE.md',
    'docs\release\V1.0-RELEASE-BLOCKER-INVENTORY.md',
    'docs\release\V1.0-REPORT-PERSISTENCE-MATRIX.md',
    'docs\release\V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md',
    'docs\release\RC3F-FIELD-BASELINE.md',
    'docs\architecture\CROSS-PLATFORM-FIX-REGISTER.md',
    'docs\issf-rules\README.md',
    'docs\rules\RULE-AUTHORITY-INDEX.md'
)
foreach ($d in $docSet) {
    $src = Join-Path $repo $d
    if (Test-Path $src) { Copy-Item $src (Join-Path $docsOut (Split-Path $d -Leaf)) }
    else { Write-Warning "document not found, not shipped: $d" }
}
# The applicable discipline rule files, so an operator can check a course
# against the rule the software implements without a repository.
$rulesOut = Join-Path $docsOut 'issf-rules'
New-Item -ItemType Directory -Force $rulesOut | Out-Null
Get-ChildItem (Join-Path $repo 'docs\issf-rules') -Filter '*.md' -ErrorAction SilentlyContinue |
    ForEach-Object { Copy-Item $_.FullName $rulesOut }

foreach ($l in @('LICENSE', 'LICENSE.txt', 'docs\legal\THIRD-PARTY-NOTICES.md')) {
    $src = Join-Path $repo $l
    if (Test-Path $src) { Copy-Item $src (Join-Path $pkg (Split-Path $l -Leaf)) }
}

# ---- 5. the support tools the operator runs ------------------------------
foreach ($t in @('tools\release\Make-SupportBundle.ps1', 'tools\release\Collect-Logs.cmd')) {
    $src = Join-Path $repo $t
    if (Test-Path $src) { Copy-Item $src $pkg }
}

# ---- 6. Qt runtime -------------------------------------------------------
# windeployqt in this Qt 6.5.3 MinGW install fails to find its own platform
# plugin. Its result is judged from the filesystem, and an explicit allow-list
# — built from windeployqt's own dependency report plus the QML modules the
# application actually imports — takes over when it fails.
Write-Host "== Qt runtime deployment =="
$qtRoot = Split-Path $QtBin -Parent
$deployedBy = 'windeployqt'
try {
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & (Join-Path $QtBin 'windeployqt.exe') --release --compiler-runtime --no-translations `
        --no-system-d3d-compiler --no-opengl-sw --qmldir $repo `
        (Join-Path $pkg 'TechAim.exe') 2>&1 | Out-Null
} catch {
    Write-Host "   windeployqt reported: $($_.Exception.Message)"
} finally {
    $ErrorActionPreference = $prev
}
if (-not (Test-Path (Join-Path $pkg 'platforms\qwindows.dll'))) {
    Write-Warning "windeployqt did not deploy the platform plugin - using the explicit allow-list."
    $deployedBy = 'explicit allow-list (windeployqt platform-plugin detection failed)'

    $qtDlls = @('Qt6Core','Qt6Gui','Qt6Qml','Qt6QmlModels','Qt6QmlWorkerScript','Qt6Quick',
                'Qt6QuickControls2','Qt6QuickControls2Impl','Qt6QuickTemplates2',
                'Qt6QuickShapes','Qt6QuickDialogs2','Qt6QuickDialogs2Utils',
                'Qt6QuickDialogs2QuickImpl','Qt6QuickLayouts',
                'Qt6Widgets','Qt6Network','Qt6SerialPort','Qt6Xml','Qt6Svg',
                'Qt6Multimedia','Qt6MultimediaQuick','Qt6Charts','Qt6ChartsQml',
                'Qt6OpenGL','Qt6OpenGLWidgets','Qt6Concurrent')
    foreach ($d in $qtDlls) {
        $src = Join-Path $QtBin "$d.dll"
        if (Test-Path $src) { Copy-Item $src $pkg -Force }
    }
    foreach ($rt in @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
        $src = Join-Path $QtBin $rt
        if (Test-Path $src) { Copy-Item $src $pkg -Force }
    }
    $pluginSets = @('platforms','imageformats','iconengines','styles','multimedia',
                    'tls','networkinformation','generic')
    foreach ($set in $pluginSets) {
        $src = Join-Path $qtRoot "plugins\$set"
        if (Test-Path $src) { Copy-Item $src (Join-Path $pkg $set) -Recurse -Force }
    }
    $qmlMods = @('QtQuick','QtQml','QtCharts')
    foreach ($m in $qmlMods) {
        $src = Join-Path $qtRoot "qml\$m"
        if (Test-Path $src) { Copy-Item $src (Join-Path $pkg $m) -Recurse -Force }
    }
    $geSrc = Join-Path $qtRoot 'qml\Qt5Compat\GraphicalEffects'
    if (Test-Path $geSrc) {
        $geOut = Join-Path $pkg 'Qt5Compat\GraphicalEffects'
        New-Item -ItemType Directory -Force (Split-Path $geOut -Parent) | Out-Null
        Copy-Item $geSrc $geOut -Recurse -Force
    }
    $vk = Join-Path $pkg 'QtQuick\VirtualKeyboard'
    if (Test-Path $vk) { Remove-Item $vk -Recurse -Force }
}
Write-Host "   deployed by: $deployedBy"

# Finals audio loads Qt Multimedia at RUNTIME, so it is not always visible in
# the import graph. Ensure the module and its plugins are present regardless.
foreach ($extra in @('Qt6Multimedia.dll')) {
    $src = Join-Path $QtBin $extra
    if ((Test-Path $src) -and -not (Test-Path (Join-Path $pkg $extra))) { Copy-Item $src $pkg }
}
$mmSrc = Join-Path $qtRoot 'plugins\multimedia'
if ((Test-Path $mmSrc) -and -not (Test-Path (Join-Path $pkg 'multimedia'))) {
    Copy-Item $mmSrc (Join-Path $pkg 'multimedia') -Recurse
}

# ---- 7. scrub anything that must not ship --------------------------------
foreach ($junk in @('*.pdb', '*.ilk', '*.exp', '*.lib', 'vc_redist*.exe')) {
    Get-ChildItem $pkg -Recurse -Filter $junk -ErrorAction SilentlyContinue | Remove-Item -Force
}

# ---- 8. the release manifest ---------------------------------------------
$exeSha = (Get-FileHash (Join-Path $pkg 'TechAim.exe') -Algorithm SHA256).Hash
$built  = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss')
$manifest = @"
Tech Aim Single Target - Windows release manifest
=================================================

VERSION        : $version
COMMIT         : $sha
BUILT          : $built
DEPLOYED BY    : $deployedBy
CONFIG         : app_mode=Live, developer_mode=0, is_single_decimal=1
EXE SHA-256    : $exeSha

BLOCKERS       : 0  (see docs/TECH-AIM-SINGLE-TARGET-V1.0-RELEASE-GATE.md)

PHYSICAL EVIDENCE
  Acquisition, Modbus, counter reconciliation, paper feed and the 50 m 3P
  state machines are unchanged since RC3F-DIAG (39df782) and INHERIT its
  field evidence: 385 accepted physical shots, 0 acquisition faults, three
  completed qualifications and three completed Finals, 2026-08-29.

  NO LIVE TARGET TEST WAS PERFORMED FOR THIS BUILD. The reporting and
  persistence work in this release is AUTOMATED, REPLAY AND RENDER VALIDATED,
  not physically retested. See docs/V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md.

THIS BUILD DOES NOT CLAIM
  - that all v1.0 features were physically tested
  - that the 10 m Final report path was physically tested
  - any ranking, placing, medal or elimination result: one lane cannot know them

RULE AUTHORITY
  ISSF Rule Book 2026, Edition 2025, Second Print 07/2026, effective
  1 July 2026.
"@
[System.IO.File]::WriteAllText((Join-Path $pkg 'RELEASE-MANIFEST.txt'),
                               ($manifest -replace "`r`n", "`n"),
                               (New-Object System.Text.UTF8Encoding($false)))

# ---- 9. zip + SHA-256 ----------------------------------------------------
Write-Host "== packaging =="
Compress-Archive -Path (Join-Path $pkg '*') -DestinationPath $zip -CompressionLevel Optimal
$zipSha = (Get-FileHash $zip -Algorithm SHA256).Hash
$size   = [math]::Round((Get-Item $zip).Length / 1MB, 2)

Write-Host ""
Write-Host "PACKAGE    : $pkg"
Write-Host "ZIP        : $zip"
Write-Host "SIZE       : $size MB"
Write-Host "ZIP SHA-256: $zipSha"
Write-Host "EXE SHA-256: $exeSha"
[System.IO.File]::WriteAllText("$zip.sha256", "$zipSha *$name.zip`n",
                               (New-Object System.Text.UTF8Encoding($false)))
