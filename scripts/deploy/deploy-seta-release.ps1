<#
.SYNOPSIS
    Produce a self-contained SETA Windows runtime from a Release build.

.DESCRIPTION
    One command, repeatable, no manual copying:

        powershell -ExecutionPolicy Bypass -File scripts\deploy\deploy-seta-release.ps1

    The result runs by double-clicking the product executable on a machine with
    MinGW and no developer environment. It is the INPUT to a future installer,
    not an installer itself.

    WHY THIS IS NOT A PLAIN windeployqt CALL
    ----------------------------------------
    On this Qt 6.5.3 MinGW installation `windeployqt --release` ends with

        Unable to find the platform plugin.

    and copies nothing. Verbose output shows why: it reads every plugin as
    "64 bit, MinGW, debug" - the MinGW plugin DLLs ship unstripped, and
    windeployqt's debug/release heuristic reads their debug sections as a debug
    build. In --release mode it then filters out every plugin, including
    platforms\qwindows.dll, and reports it as missing.

    This Qt install has ONE set of binaries (there is no Qt6Cored.dll), so
    --debug and --release would deploy the same files; only the filter differs.
    So the deployment runs windeployqt in the mode this installation accepts and
    then removes what that mode adds for developers (the QML debugging plugins)
    plus module families the application never loads. Everything removed is
    listed in PRUNE below, and the result is verified by
    tests\release\check_deployment.py, which walks the PE import table of every
    binary and fails if anything would have to be found outside the folder.

.PARAMETER BuildDir
    The Release build output. Default: <repo>\release
.PARAMETER OutDir
    Deployment target, wiped and recreated. Default: <repo>\dist\seta-runtime
.PARAMETER QtBin
    Qt bin directory. Default: C:\Qt\6.5.3\mingw_64\bin
.PARAMETER MingwBin
    MinGW bin directory. Default: C:\Qt\Tools\mingw1120_64\bin
.PARAMETER SkipValidate
    Skip the validation gate (for debugging the script itself only).
#>
[CmdletBinding()]
param(
    [string]$BuildDir,
    [string]$OutDir,
    [string]$QtBin    = 'C:\Qt\6.5.3\mingw_64\bin',
    [string]$MingwBin = 'C:\Qt\Tools\mingw1120_64\bin',
    [switch]$SkipValidate
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $BuildDir) { $BuildDir = Join-Path $repo 'release' }
if (-not $OutDir)   { $OutDir   = Join-Path $repo 'dist\seta-runtime' }

function Say($m) { Write-Host "[deploy] $m" }
function Die($m) { Write-Host "[deploy] ERROR: $m" -ForegroundColor Red; exit 1 }

# ── 1. validate the input build ───────────────────────────────────────────
# The shipped executable is named by the BRAND: SETA.exe for the SETA
# product line, TechAim.exe for Tech Aim. Discovered rather than assumed,
# so a rename cannot silently produce an empty package.
$exeName = if (Test-Path (Join-Path $BuildDir 'SETA.exe')) { 'SETA.exe' } else { 'TechAim.exe' }
$exe = Join-Path $BuildDir $exeName
if (-not (Test-Path $exe))   { Die "no build at $exe - build Release first" }
if (-not (Test-Path $QtBin)) { Die "Qt bin not found: $QtBin" }
$windeployqt = Join-Path $QtBin 'windeployqt.exe'
if (-not (Test-Path $windeployqt)) { Die "windeployqt not found in $QtBin" }

# The flavour is read out of the BINARY, not assumed from the branch: a Tech Aim
# build must never be shipped through the SETA deployment path.
$bytes = [System.IO.File]::ReadAllBytes($exe)
$utf16 = [System.Text.Encoding]::Unicode.GetString($bytes)
if ($utf16 -notmatch 'SETA Electronic Target Control') {
    Die "$exe is not a SETA build (its version resource does not say SETA)"
}
Say "input build: $exe (SETA)"

# The commit is baked in at QMAKE time (Seta.pro reads git once), so a binary
# left over from an earlier commit looks identical on disk and would be shipped
# with a manifest naming a commit it was never built from. The binary is asked
# instead of git: HEAD's sha must be inside it.
Push-Location $repo
$headSha = (git rev-parse --short HEAD 2>$null)
$treeDirty = -not [string]::IsNullOrWhiteSpace((git status --porcelain 2>$null))
Pop-Location
if ([string]::IsNullOrWhiteSpace($headSha)) { Die 'cannot read git HEAD' }
if ($utf16 -notmatch [regex]::Escape($headSha)) {
    Die ("$exe was not built from HEAD ($headSha). Rebuild before deploying:`n" +
         "    qmake Seta.pro && mingw32-make -f Makefile.Release`n" +
         "  (qmake must be re-run - the commit is baked in at qmake time)")
}
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
# --no-translations: the .qm catalogues are in the qrc too.
$env:PATH = "$QtBin;$MingwBin;$env:PATH"
Say 'running windeployqt (see the note at the top of this script for the mode)'
& $windeployqt --debug --no-translations --qmldir $repo (Join-Path $OutDir $exeName) 2>&1 |
    Select-Object -Last 2 | ForEach-Object { Say "  $_" }
if (-not (Test-Path (Join-Path $OutDir 'platforms\qwindows.dll'))) {
    Die 'windeployqt did not deploy the platform plugin'
}

# ── 4. prune what the deploy mode adds for developers, and what this ──────
#      application never loads. Each entry is a decision, not a guess:
$PRUNE_DIRS = @(
    'qmltooling',            # QML debugger/profiler plugins - developer only
    'sqldrivers',            # no database anywhere in the product
    'assetimporters',        # Qt Quick 3D asset import - not imported
    'geometryloaders',       # Qt 3D - not imported
    'renderers',             # Qt 3D
    'renderplugins',         # Qt 3D
    'sceneparsers',          # Qt 3D
    'designer'               # Qt Designer plugins
)
$PRUNE_DLL_PREFIXES = @(
    'Qt63D',                 # Qt 3D module family
    'Qt6Quick3D',            # Quick 3D
    'Qt6ShaderTools',        # only used by Quick 3D here
    'Qt6VirtualKeyboard'     # no touch keyboard surface in this product
)
$PRUNE_QML_MODULES = @(
    'QtQuick3D', 'Qt3D', 'QtQuick\VirtualKeyboard',
    'QtQuick\Scene2D',       # Qt 3D bridge - its plugin imports Qt63DCore
    'QtQuick\Scene3D'        # Qt 3D bridge - its plugin imports Qt63DAnimation
)

# Finals audio loads Qt Multimedia at RUNTIME, so windeployqt does not see the
# import and does not deploy the QML module - only the DLL and the plugins. The
# module is what the QML engine actually resolves `import QtMultimedia` against,
# so without it the command cues are silent on a machine with no Qt installed.
# Added explicitly, and check_deployment.py fails the package if it is missing.
$mmQml = Join-Path (Split-Path $QtBin -Parent) 'qml\QtMultimedia'
$mmOut = Join-Path $OutDir 'qml\QtMultimedia'
if ((Test-Path $mmQml) -and -not (Test-Path $mmOut)) {
    New-Item -ItemType Directory -Force (Split-Path $mmOut -Parent) | Out-Null
    Copy-Item $mmQml $mmOut -Recurse -Force
    Say 'added qml\QtMultimedia (runtime import, invisible to windeployqt)'
    # The module's plugin imports Qt6MultimediaQuick, which windeployqt also
    # never saw. Deployed with it: a QML module whose plugin cannot load is a
    # folder that passes inspection and fails at runtime.
    $mmq = Join-Path $QtBin 'Qt6MultimediaQuick.dll'
    if ((Test-Path $mmq) -and -not (Test-Path (Join-Path $OutDir 'Qt6MultimediaQuick.dll'))) {
        Copy-Item $mmq $OutDir -Force
        Say 'added Qt6MultimediaQuick.dll (imported by the QtMultimedia QML plugin)'
    }
}

# The support collector ships WITH the product. SETA's physical test is the next
# and most important evidence gate, and a bundle that cannot be produced after
# it is a test that cannot be investigated. Collect-Logs.cmd is double-clickable
# on purpose: an operator at a range does not type PowerShell.
foreach ($t in @('tools\release\Collect-Logs.cmd', 'tools\release\Make-SupportBundle.ps1')) {
    $src = Join-Path $repo $t
    if (Test-Path $src) { Copy-Item $src $OutDir -Force; Say "added $(Split-Path $t -Leaf)" }
    else { throw "support tooling missing from the repository: $t" }
}

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
# The virtual-keyboard input context plugin goes with its module.
Get-ChildItem (Join-Path $OutDir 'platforminputcontexts') -Filter '*virtualkeyboard*' `
    -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item $_.FullName -Force }

# A plugin directory Qt scans but that now holds nothing is noise in a shipped
# folder; remove any directory the prune emptied.
Get-ChildItem $OutDir -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending |
    Where-Object { -not (Get-ChildItem $_.FullName -Recurse -File) } |
    ForEach-Object { Remove-Item $_.FullName -Recurse -Force; Say "pruned empty $($_.Name)\" }

# ── 5. MinGW runtime (windeployqt copies these; belt and braces) ──────────
foreach ($rt in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    $src = Join-Path $MingwBin $rt
    $dst = Join-Path $OutDir $rt
    if ((Test-Path $src) -and -not (Test-Path $dst)) { Copy-Item $src $dst }
}

# ── 6. nothing else. No config.ini, no user state, no documents: the first ─
#      run must create its own, in the SETA data namespace.

# ── 7. manifest + checksums ───────────────────────────────────────────────
$sha = (Get-FileHash (Join-Path $OutDir $exeName) -Algorithm SHA256).Hash.ToLower()
# Verified above to be the commit compiled INTO the binary, not merely today's HEAD.
$commit = $headSha
$dirty  = $treeDirty
$qtVersion = (& $windeployqt --help 2>&1 | Select-String -Pattern 'Qt Deploy Tool (\S+)').Matches.Groups[1].Value

$dlls    = @(Get-ChildItem $OutDir -Filter *.dll -File)
$plugins = @(Get-ChildItem $OutDir -Directory | Where-Object { $_.Name -ne 'qml' } |
             ForEach-Object { $_.Name })
$qmlMods = @()
$qmlRoot = Join-Path $OutDir 'qml'
if (Test-Path $qmlRoot) {
    $qmlMods = @(Get-ChildItem $qmlRoot -Directory | ForEach-Object { $_.Name })
}
$allFiles = @(Get-ChildItem $OutDir -Recurse -File)

$manifest = [ordered]@{
    product           = 'SETA Electronic Target Control'
    # The support bundle reads these to say WHICH build produced an evidence
    # set. Without them it reported the version and channel as blank, which is
    # the first thing an investigation asks for.
    version           = (Get-Item $exe).VersionInfo.FileVersion
    releaseChannel    = 'SETA v1.0 Evaluation'
    limitation        = 'EVALUATION BUILD - NOT FOR OFFICIAL COMPETITION RESULTS'
    buildFlavour      = 'SETA_OEM'
    executable        = $exeName
    executableSha256  = $sha
    qtVersion         = $qtVersion
    toolchain         = 'MinGW 11.2.0 64-bit'
    deployedUtc       = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    gitCommit         = $commit
    gitDirty          = $dirty
    runtimeDllCount   = $dlls.Count
    fileCount         = $allFiles.Count
    pluginDirectories = $plugins
    qmlModules        = $qmlMods
    deployMethod      = 'windeployqt (debug-mode filter; see scripts/deploy/deploy-seta-release.ps1) + repository prune'
}
$manifest | ConvertTo-Json -Depth 4 |
    Set-Content (Join-Path $OutDir 'deployment-manifest.json') -Encoding utf8

# SHA256 for every deployed file, so a handoff can be verified byte for byte.
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
    Say 'validating (tests\release\check_deployment.py)'
    & python (Join-Path $repo 'tests\release\check_deployment.py') $OutDir
    if ($LASTEXITCODE -ne 0) { Die 'deployment validation FAILED' }
}
Say 'done'
