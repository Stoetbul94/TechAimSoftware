# Tech Aim portable Windows field-test package builder (RC1, RC2, rollback).
#
# Produces a folder that runs on a machine with NO Qt, NO repository and NO
# MinGW/Visual Studio, then zips it and records the SHA-256.
#
# It deliberately copies an explicit ALLOW-LIST out of the build tree rather
# than copying release/ wholesale: release/ also holds object files, moc/qrc
# output, the developer config and (during review) capture data, none of which
# may ship. windeployqt then adds only the Qt runtime the binary actually
# imports.
#
#   powershell -File tools\release\build-rc1-package.ps1
[CmdletBinding()]
param(
    [string]$QtBin   = 'C:\Qt\6.5.3\mingw_64\bin',
    [string]$OutRoot = '',
    # The ROLLBACK package is built by the SAME process from a temporary
    # worktree, so the two artefacts cannot diverge in how they were produced.
    # -SourceRepo points at that worktree; -PackageName overrides the folder and
    # ZIP name so an operator can never confuse the two on a USB stick.
    [string]$SourceRepo  = '',
    [string]$PackageName = ''
)
$ErrorActionPreference = 'Stop'

$repo = if ([string]::IsNullOrWhiteSpace($SourceRepo)) {
    Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
} else { $SourceRepo }
# Documents and the support tool always come from the CURRENT checkout: the
# rollback build predates them, and an operator needs today's instructions.
$docRepo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exe  = Join-Path $repo 'release\TechAim.exe'
if (-not (Test-Path $exe)) { throw "Not built: $exe. Run qmake + mingw32-make -f Makefile.Release first." }
if (-not (Test-Path (Join-Path $QtBin 'windeployqt.exe'))) { throw "windeployqt not found in $QtBin" }

$version = '0.9.0-RC2'
$name    = if ([string]::IsNullOrWhiteSpace($PackageName)) {
    "TechAim-$version-FieldTest-Windows-x64"
} else { $PackageName }
if ([string]::IsNullOrWhiteSpace($OutRoot)) { $OutRoot = Join-Path $docRepo 'dist' }
$pkg = Join-Path $OutRoot $name
$zip = Join-Path $OutRoot "$name.zip"

Write-Host "== Tech Aim $version portable package =="
Write-Host "   repo    : $repo"
Write-Host "   package : $pkg"

# ---- 1. clean slate ------------------------------------------------------
if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
if (Test-Path $zip) { Remove-Item $zip -Force }
New-Item -ItemType Directory -Force $pkg | Out-Null

# ---- 2. the binary -------------------------------------------------------
Copy-Item $exe (Join-Path $pkg 'TechAim.exe')

# ---- 3. a RELEASE configuration template ---------------------------------
# NOT the developer's config.ini: written fresh so no local mode, developer
# flag or machine-specific value can leak into a shipped package.
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

# ---- 4. field-test documents ---------------------------------------------
$docsOut = Join-Path $pkg 'docs'
New-Item -ItemType Directory -Force $docsOut | Out-Null
foreach ($d in @(
    'docs\release\0.9.0-rc2-known-limitations.md',
    'docs\release\0.9.0-rc2-field-test-checklist.md',
    'docs\release\0.9.0-rc2-smoke-test.md',
    'docs\release\0.9.0-rc1-field-test-plan.md',
    'docs\release\0.9.0-rc1-rollback.md')) {
    $src = Join-Path $docRepo $d
    if (Test-Path $src) { Copy-Item $src (Join-Path $docsOut (Split-Path $d -Leaf)) }
}
foreach ($l in @('LICENSE', 'LICENSE.txt', 'docs\legal\THIRD-PARTY-NOTICES.md')) {
    $src = Join-Path $docRepo $l
    if (Test-Path $src) { Copy-Item $src (Join-Path $pkg (Split-Path $l -Leaf)) }
}

# ---- 5. the support-bundle tool the operator runs -------------------------
Copy-Item (Join-Path $docRepo 'tools\release\Make-SupportBundle.ps1') $pkg

# ---- 6. Qt runtime -------------------------------------------------------
# windeployqt in this Qt 6.5.3 MinGW install fails with "Unable to find the
# platform plugin" even though plugins/platforms/qwindows.dll is present, from
# any working directory and with --plugindir given explicitly. It is run first
# and its result recorded; when it fails, deployment falls back to an EXPLICIT
# allow-list built from windeployqt's own dependency report plus the QML
# modules the application actually imports.
#
# The allow-list is not a guess: the Qt module set below is exactly what
# windeployqt resolved ("To be deployed"), and the QML modules are the imports
# found in the repository's .qml files. Being explicit also makes the package
# auditable - the deployment audit under tests/release verifies every entry.
Write-Host "== Qt runtime deployment =="
$qtRoot = Split-Path $QtBin -Parent
$deployedBy = 'windeployqt'
# windeployqt writes its failure to stderr, which $ErrorActionPreference='Stop'
# would turn into a terminating error. Its failure is an EXPECTED branch here,
# so it is contained and the outcome judged from the filesystem instead.
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

    # Qt modules windeployqt itself resolved, plus their runtime companions.
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
    # MinGW C++ runtime - without these the app will not start on a clean box.
    foreach ($rt in @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
        $src = Join-Path $QtBin $rt
        if (Test-Path $src) { Copy-Item $src $pkg -Force }
    }
    # Plugins. Only the sets this application can actually reach.
    $pluginSets = @('platforms','imageformats','iconengines','styles','multimedia',
                    'tls','networkinformation','generic')
    foreach ($set in $pluginSets) {
        $src = Join-Path $qtRoot "plugins\$set"
        if (Test-Path $src) { Copy-Item $src (Join-Path $pkg $set) -Recurse -Force }
    }
    # QML modules for the imports the repository ACTUALLY uses. Verified by
    # grepping the .qml files, not assumed: QtCharts (CenterPane, coach views)
    # and Qt5Compat.GraphicalEffects (TechAimDialog) are imported; the rest of
    # Qt5Compat and all of QtQuick.VirtualKeyboard are not, and shipping them
    # would be packaging unused Qt modules blindly.
    $qmlMods = @('QtQuick','QtQml','QtCharts')
    foreach ($m in $qmlMods) {
        $src = Join-Path $qtRoot "qml\$m"
        if (Test-Path $src) { Copy-Item $src (Join-Path $pkg $m) -Recurse -Force }
    }
    # Only the one Qt5Compat submodule that is imported.
    $geSrc = Join-Path $qtRoot 'qml\Qt5Compat\GraphicalEffects'
    if (Test-Path $geSrc) {
        $geOut = Join-Path $pkg 'Qt5Compat\GraphicalEffects'
        New-Item -ItemType Directory -Force (Split-Path $geOut -Parent) | Out-Null
        Copy-Item $geSrc $geOut -Recurse -Force
    }
    # QtQuick ships a large VirtualKeyboard module this application never
    # imports. Dropping it removes ~1500 files the field test cannot reach.
    $vk = Join-Path $pkg 'QtQuick\VirtualKeyboard'
    if (Test-Path $vk) { Remove-Item $vk -Recurse -Force }
}
Write-Host "   deployed by: $deployedBy"

# Qt Multimedia is loaded at RUNTIME for the finals audio cues, so its plugins
# are not always detected from the import graph. Ensure they are present.
foreach ($extra in @('Qt6Multimedia.dll')) {
    $src = Join-Path $QtBin $extra
    if ((Test-Path $src) -and -not (Test-Path (Join-Path $pkg $extra))) { Copy-Item $src $pkg }
}
$mmSrc = Join-Path $qtRoot 'plugins\multimedia'
if ((Test-Path $mmSrc) -and -not (Test-Path (Join-Path $pkg 'multimedia'))) {
    Copy-Item $mmSrc (Join-Path $pkg 'multimedia') -Recurse
}

# ---- 7. scrub anything windeployqt brought that must not ship -------------
foreach ($junk in @('*.pdb', '*.ilk', '*.exp', '*.lib', 'vc_redist*.exe')) {
    Get-ChildItem $pkg -Recurse -Filter $junk -ErrorAction SilentlyContinue | Remove-Item -Force
}

# ---- 8. zip + SHA-256 ----------------------------------------------------
Write-Host "== packaging =="
Compress-Archive -Path (Join-Path $pkg '*') -DestinationPath $zip -CompressionLevel Optimal
$sha = (Get-FileHash $zip -Algorithm SHA256).Hash
$size = [math]::Round((Get-Item $zip).Length / 1MB, 2)

Write-Host ""
Write-Host "PACKAGE : $pkg"
Write-Host "ZIP     : $zip"
Write-Host "SIZE    : $size MB"
Write-Host "SHA-256 : $sha"
[System.IO.File]::WriteAllText("$zip.sha256", "$sha *$name.zip`n",
                               (New-Object System.Text.UTF8Encoding($false)))
