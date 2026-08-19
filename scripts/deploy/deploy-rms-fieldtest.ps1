<#
.SYNOPSIS
    Produce a self-contained Tech Aim RMS field-test package from a Release
    build.

.DESCRIPTION
    One command, repeatable, no manual copying:

        powershell -ExecutionPolicy Bypass -File scripts\deploy\deploy-rms-fieldtest.ps1

    The result runs by double-clicking TechAimRMS.exe on a machine with no Qt,
    no MinGW and no developer environment. It is a FIELD TEST / DEVELOPMENT
    EVALUATION package, not a competition release and not an installer.

    WHY THIS IS NOT A PLAIN windeployqt CALL
    ----------------------------------------
    This is the method already established for this repository by
    scripts\deploy\deploy-seta-release.ps1 on the SETA product line, for the
    reason documented there: on this Qt 6.5.3 MinGW installation
    `windeployqt --release` ends with

        Unable to find the platform plugin.

    and copies nothing, because the MinGW plugin DLLs ship unstripped and
    windeployqt's debug/release heuristic reads their debug sections as a debug
    build, then filters every plugin out in --release mode. This Qt install has
    ONE set of binaries (there is no Qt6Cored.dll), so --debug and --release
    would deploy the same files; only the filter differs. The deployment
    therefore runs windeployqt in the mode this installation accepts and then
    removes what that mode adds for developers, plus the module families RMS
    never loads. Everything removed is listed in PRUNE below, and the result is
    verified by tests\release\check_rms_deployment.py, which walks the PE import
    table of every binary and fails if anything would have to be found outside
    the folder.

    RMS's QML surface is small - QtQuick and QtQuick.Window, no Controls, no
    Charts, no Multimedia - so its prune list is longer than SETA's.

.PARAMETER BuildDir
    The Release build output. Default: <repo>\rms\release
.PARAMETER OutDir
    Deployment target, wiped and recreated.
    Default: <repo>\dist\TechAimRMS-FieldTest-M4_5
.PARAMETER QtBin
    Qt bin directory. Default: C:\Qt\6.5.3\mingw_64\bin
.PARAMETER MingwBin
    MinGW bin directory. Default: C:\Qt\Tools\mingw1120_64\bin
.PARAMETER SkipValidate
    Skip the validation gate (for debugging the script itself only).
.PARAMETER NoZip
    Do not produce the handoff ZIP.
#>
[CmdletBinding()]
param(
    [string]$BuildDir,
    [string]$OutDir,
    [string]$QtBin    = 'C:\Qt\6.5.3\mingw_64\bin',
    [string]$MingwBin = 'C:\Qt\Tools\mingw1120_64\bin',
    [switch]$SkipValidate,
    [switch]$NoZip
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $BuildDir) { $BuildDir = Join-Path $repo 'rms\release' }
if (-not $OutDir)   { $OutDir   = Join-Path $repo 'dist\TechAimRMS-FieldTest-M4_5' }

function Say($m) { Write-Host "[rms-deploy] $m" }
function Die($m) { Write-Host "[rms-deploy] ERROR: $m" -ForegroundColor Red; exit 1 }

# ── 1. validate the input build ───────────────────────────────────────────
$exe = Join-Path $BuildDir 'TechAimRMS.exe'
if (-not (Test-Path $exe))   { Die "no build at $exe - build Release first" }
if (-not (Test-Path $QtBin)) { Die "Qt bin not found: $QtBin" }
$windeployqt = Join-Path $QtBin 'windeployqt.exe'
if (-not (Test-Path $windeployqt)) { Die "windeployqt not found in $QtBin" }

$bytes = [System.IO.File]::ReadAllBytes($exe)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
$utf16 = [System.Text.Encoding]::Unicode.GetString($bytes)

# The flavour is read out of the BINARY, not assumed from the branch.
if ($utf16 -notmatch 'Range Management System') {
    Die "$exe does not identify as Tech Aim RMS in its version resource"
}

# The commit is baked in at QMAKE time (TechAimRMS.pro reads git once), so a
# binary left over from an earlier commit looks identical on disk and would be
# shipped with a manifest naming a commit it was never built from. The binary is
# asked instead of git: HEAD's sha must be inside it.
Push-Location $repo
$headSha = (git rev-parse --short HEAD 2>$null)
$treeDirty = -not [string]::IsNullOrWhiteSpace((git status --porcelain 2>$null))
Pop-Location
if ([string]::IsNullOrWhiteSpace($headSha)) { Die 'cannot read git HEAD' }
if ($ascii -notmatch [regex]::Escape($headSha)) {
    Die ("$exe was not built from HEAD ($headSha). Rebuild before deploying:`n" +
         "    cd rms; qmake TechAimRMS.pro; mingw32-make -f Makefile.Release`n" +
         "  (qmake must be re-run - the commit is baked in at qmake time)")
}
Say "input build: $exe"
Say "built from HEAD: $headSha$(if ($treeDirty) { ' (WORKING TREE DIRTY)' })"

# ── 2. fresh output ───────────────────────────────────────────────────────
if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
Copy-Item $exe $OutDir
Say "output: $OutDir"

# ── 3. Qt runtime, via the official tool ──────────────────────────────────
# --qmldir points at the SOURCE tree because the application's QML lives in the
# executable's resources, where windeployqt cannot scan it; the .qml files it
# reads there are the same ones compiled into the qrc.
$env:PATH = "$QtBin;$MingwBin;$env:PATH"
Say 'running windeployqt (see the note at the top of this script for the mode)'
& $windeployqt --debug --no-translations --qmldir (Join-Path $repo 'rms\qml') `
    (Join-Path $OutDir 'TechAimRMS.exe') 2>&1 |
    Select-Object -Last 2 | ForEach-Object { Say "  $_" }
if (-not (Test-Path (Join-Path $OutDir 'platforms\qwindows.dll'))) {
    Die 'windeployqt did not deploy the platform plugin'
}

# ── 4. prune what the deploy mode adds for developers, and what RMS ───────
#      never loads. Each entry is a decision, not a guess:
$PRUNE_DIRS = @(
    'qmltooling',            # QML debugger/profiler plugins - developer only
    'sqldrivers',            # RMS has no database
    'assetimporters',        # Qt Quick 3D asset import - not imported
    'geometryloaders',       # Qt 3D - not imported
    'renderers', 'renderplugins', 'sceneparsers',
    'designer',              # Qt Designer plugins
    'multimedia',            # RMS plays no audio and shows no video
    'networkinformation',    # RMS does not query connectivity state
    'generic'                # extra input plugins (tablet/touch) - not used
)
$PRUNE_DLL_PREFIXES = @(
    'Qt63D', 'Qt6Quick3D', 'Qt6ShaderTools',
    'Qt6VirtualKeyboard',    # no touch keyboard surface
    'Qt6Multimedia',         # no audio, no video
    'Qt6Sql', 'Qt6Test', 'Qt6Charts', 'Qt6Pdf'
)
$PRUNE_QML_MODULES = @(
    'QtQuick3D', 'Qt3D', 'QtQuick\VirtualKeyboard',
    'QtQuick\Scene2D', 'QtQuick\Scene3D',
    'QtMultimedia', 'QtCharts', 'QtTest', 'QtQuick\Particles'
)

foreach ($d in $PRUNE_DIRS) {
    $p = Join-Path $OutDir $d
    if (Test-Path $p) { Remove-Item $p -Recurse -Force; Say "pruned $d\" }
}
foreach ($pfx in $PRUNE_DLL_PREFIXES) {
    Get-ChildItem $OutDir -Filter "$pfx*.dll" -ErrorAction SilentlyContinue |
        ForEach-Object { Remove-Item $_.FullName -Force; Say "pruned $($_.Name)" }
}
foreach ($m in $PRUNE_QML_MODULES) {
    $p = Join-Path $OutDir "qml\$m"
    if (Test-Path $p) { Remove-Item $p -Recurse -Force; Say "pruned qml\$m\" }
}
Get-ChildItem (Join-Path $OutDir 'platforminputcontexts') -Filter '*virtualkeyboard*' `
    -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item $_.FullName -Force }

Get-ChildItem $OutDir -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending |
    Where-Object { -not (Get-ChildItem $_.FullName -Recurse -File) } |
    ForEach-Object { Remove-Item $_.FullName -Recurse -Force; Say "pruned empty $($_.Name)\" }

# ── 5. MinGW runtime (windeployqt copies these; belt and braces) ──────────
foreach ($rt in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    $src = Join-Path $MingwBin $rt
    $dst = Join-Path $OutDir $rt
    if ((Test-Path $src) -and -not (Test-Path $dst)) { Copy-Item $src $dst }
}

# ── 6. the field-test handoff files ───────────────────────────────────────
# These are the ONLY non-runtime files in the package: two launchers, a reset,
# a README and a human checklist. No source, no tests, no configuration and no
# range data - the first run starts clean.
$handoff = Join-Path $repo 'scripts\deploy\fieldtest'
foreach ($f in @('Launch-TechAimRMS-Demo.cmd', 'Launch-TechAimRMS-Live.cmd',
                 'Reset-Demo.cmd', 'README-FIELD-TEST.txt',
                 'FIELD-TEST-CHECKLIST.txt')) {
    $src = Join-Path $handoff $f
    if (-not (Test-Path $src)) { Die "missing handoff file: $src" }
    Copy-Item $src $OutDir
}
Say 'handoff files copied'

# ── 7. manifest + checksums ───────────────────────────────────────────────
$sha = (Get-FileHash (Join-Path $OutDir 'TechAimRMS.exe') -Algorithm SHA256).Hash.ToLower()
$qtVersion = (& $windeployqt --help 2>&1 | Select-String -Pattern 'Qt Deploy Tool (\S+)').Matches.Groups[1].Value

$dlls    = @(Get-ChildItem $OutDir -Filter *.dll -File)
$plugins = @(Get-ChildItem $OutDir -Directory | Where-Object { $_.Name -ne 'qml' } |
             ForEach-Object { $_.Name })
$qmlMods = @()
$qmlRoot = Join-Path $OutDir 'qml'
if (Test-Path $qmlRoot) { $qmlMods = @(Get-ChildItem $qmlRoot -Directory | ForEach-Object { $_.Name }) }
$allFiles = @(Get-ChildItem $OutDir -Recurse -File)

$manifest = [ordered]@{
    product           = 'Tech Aim Range Management System'
    packageKind       = 'FIELD TEST / DEVELOPMENT EVALUATION - not a competition release'
    milestone         = 'M4.5 target display MVP'
    executable        = 'TechAimRMS.exe'
    executableSha256  = $sha
    qtVersion         = $qtVersion
    toolchain         = 'MinGW 11.2.0 64-bit'
    deployedUtc       = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    gitCommit         = $headSha
    gitDirty          = $treeDirty
    runtimeDllCount   = $dlls.Count
    fileCount         = $allFiles.Count
    pluginDirectories = $plugins
    qmlModules        = $qmlMods
    observationPort   = 7755
    deployMethod      = 'windeployqt (debug-mode filter; see scripts/deploy/deploy-rms-fieldtest.ps1) + repository prune'
}
$manifest | ConvertTo-Json -Depth 4 |
    Set-Content (Join-Path $OutDir 'deployment-manifest.json') -Encoding utf8

$lines = foreach ($f in ($allFiles | Sort-Object FullName)) {
    if ($f.Name -eq 'SHA256SUMS.txt') { continue }
    $rel = $f.FullName.Substring($OutDir.Length + 1).Replace('\', '/')
    "$((Get-FileHash $f.FullName -Algorithm SHA256).Hash.ToLower())  $rel"
}
$lines | Set-Content (Join-Path $OutDir 'SHA256SUMS.txt') -Encoding utf8

Say "executable sha256: $sha"
Say "files: $($allFiles.Count)  dlls: $($dlls.Count)"

# ── 8. validate ───────────────────────────────────────────────────────────
if (-not $SkipValidate) {
    Say 'validating (tests\release\check_rms_deployment.py)'
    & python (Join-Path $repo 'tests\release\check_rms_deployment.py') $OutDir
    if ($LASTEXITCODE -ne 0) { Die 'deployment validation FAILED' }
}

# ── 9. handoff ZIP ────────────────────────────────────────────────────────
if (-not $NoZip) {
    $zip = "$OutDir.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path $OutDir -DestinationPath $zip
    $zipSha = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
    Say "zip: $zip"
    Say "zip sha256: $zipSha"
}
Say 'done'
