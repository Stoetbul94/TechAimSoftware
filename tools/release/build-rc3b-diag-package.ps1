# ─────────────────────────────────────────────────────────────────────────────
# Tech Aim 0.9.0-RC3B-DIAG — INTERNAL physical-qualification package.
#
# A NEW SCRIPT, not an edit of build-rc3-seta-package.ps1. RC3a is tagged,
# hashed and handed to SETA; its script stays byte-for-byte reproducible and
# this one cannot reach its output directory.
#
# THIS IS NOT AN EVALUATION BUILD. It ships developer_mode=1 because it exists
# to be diagnosed at the range, and it is a ZIP rather than an installer so it
# cannot be mistaken for something to hand over. The external build comes after
# physical qualification passes, with its own number and its own documentation.
#
#   powershell -ExecutionPolicy Bypass -File tools\release\build-rc3b-diag-package.ps1
#
# Produces dist\rc3b\TechAim-0.9.0-RC3B-DIAG.zip and a manifest beside it.
# It does not send anything anywhere.
# ─────────────────────────────────────────────────────────────────────────────
param(
    [string]$QtBin = 'C:\Qt\6.5.3\mingw_64\bin',
    [switch]$SkipRegression
)

$ErrorActionPreference = 'Stop'
$repo    = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$version = '0.9.0-RC3B-DIAG'
$name    = "TechAim-$version"
$outRoot = Join-Path $repo 'dist\rc3b'
$stage   = Join-Path $outRoot 'staging'

function Fail($m) { Write-Host "FAIL  $m" -ForegroundColor Red; exit 1 }
function Ok($m)   { Write-Host "  ok  $m" }

# ---- 1. repository state ---------------------------------------------------
Write-Host "== repository state =="
Push-Location $repo
$dirty  = (git status --porcelain | Where-Object { $_ -notmatch '^\?\? ' })
$commit = (git rev-parse --short HEAD)
$branch = (git rev-parse --abbrev-ref HEAD)
$remote = (git rev-parse --short '@{u}' 2>$null)
Pop-Location
if ($dirty) { Fail "tracked files are modified - commit or revert first:`n$dirty" }
Ok "branch $branch at $commit (upstream $remote)"

# ---- 2. the binary must BE this commit ------------------------------------
# Seta.pro bakes the sha in at qmake time, so an incremental build after new
# commits keeps reporting the old one while carrying the new code. A manifest
# that names a commit the binary was never built from is worse than no manifest.
$exe = Join-Path $repo 'release\TechAim.exe'
if (-not (Test-Path $exe)) { Fail "no build at $exe" }
$bytes = [System.IO.File]::ReadAllBytes($exe)
$utf16 = [System.Text.Encoding]::Unicode.GetString($bytes)
if ($utf16 -notmatch [regex]::Escape($commit)) {
    Fail ("release\TechAim.exe was not built from HEAD ($commit).`n" +
          "  Release builds must be CLEAN builds:`n" +
          "    mingw32-make -f Makefile.Release clean; qmake Seta.pro; mingw32-make -f Makefile.Release")
}
Ok "binary carries commit $commit"
if ($utf16 -notmatch [regex]::Escape($version)) {
    Fail "release\TechAim.exe does not report $version - wrong channel, refusing to package"
}
Ok "binary reports $version"

# ---- 3. regression ---------------------------------------------------------
if (-not $SkipRegression) {
    Write-Host "== regression =="
    $suites = @{ reliability = 'tests\reliability\release'; training = 'tests\training\release'
                 finals10m   = 'tests\finals10m\release';   finals   = 'tests\finals\release'
                 qml         = 'tests\qml\release' }
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:Path = "$QtBin;$env:Path"
    $script:totals = @()
    foreach ($s in $suites.GetEnumerator()) {
        $bin = Get-ChildItem (Join-Path $repo $s.Value) -Filter '*_tests.exe' -ErrorAction SilentlyContinue |
               Select-Object -First 1
        if (-not $bin) { Fail "no test binary for $($s.Key)" }
        $line = & $bin.FullName 2>$null | Select-String '=== \d+ checks' | Select-Object -Last 1
        if (-not $line -or $line -notmatch '(\d+) checks, (\d+) failures') { Fail "$($s.Key): no result line" }
        if ([int]$Matches[2] -ne 0) { Fail "$($s.Key): $($Matches[2]) failures" }
        $script:totals += "$($s.Key)=$($Matches[1])/0"
        Ok "$($s.Key): $($Matches[1]) checks, 0 failures"
    }
    foreach ($c in @('check_manuals','check_project_memory','check_training_lab_evidence')) {
        $out = & python (Join-Path $repo "tests\docs\$c.py") 2>&1 | Select-String '=== \d+ checks'
        if (-not $out -or "$out" -notmatch '(\d+) checks, (\d+) failures') { Fail "$c: no result line" }
        if ([int]$Matches[2] -ne 0) { Fail "$c: $($Matches[2]) failures" }
        $script:totals += "$c=$($Matches[1])/0"
        Ok "$c: $($Matches[1]) checks, 0 failures"
    }
}

# ---- 4. fresh staging ------------------------------------------------------
Write-Host "== staging =="
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item $exe (Join-Path $stage 'TechAim.exe')
Ok "fresh staging at $stage"

# ---- 5. Qt runtime ---------------------------------------------------------
# windeployqt --release fails on this Qt 6.5.3 MinGW install: it reads the
# unstripped MinGW plugins as debug builds and filters every plugin out,
# platforms\qwindows.dll included. The allow-list below is windeployqt's own
# resolved module set plus the QML modules this application imports.
Write-Host "== Qt runtime =="
$qtRoot = Split-Path $QtBin -Parent
try {
    $prev = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    & (Join-Path $QtBin 'windeployqt.exe') --release --compiler-runtime --no-translations `
        --no-system-d3d-compiler --no-opengl-sw --qmldir $repo `
        (Join-Path $stage 'TechAim.exe') 2>&1 | Out-Null
} catch { } finally { $ErrorActionPreference = $prev }

$deployedBy = 'windeployqt'
if (-not (Test-Path (Join-Path $stage 'platforms\qwindows.dll'))) {
    $deployedBy = 'explicit allow-list (windeployqt platform-plugin detection failed)'
    $qtDlls = @('Qt6Core','Qt6Gui','Qt6Qml','Qt6QmlModels','Qt6QmlWorkerScript','Qt6Quick',
                'Qt6QuickControls2','Qt6QuickControls2Impl','Qt6QuickTemplates2',
                'Qt6QuickShapes','Qt6QuickDialogs2','Qt6QuickDialogs2Utils',
                'Qt6QuickDialogs2QuickImpl','Qt6QuickLayouts',
                'Qt6Widgets','Qt6Network','Qt6SerialPort','Qt6Xml','Qt6Svg',
                'Qt6Multimedia','Qt6MultimediaQuick','Qt6Charts','Qt6ChartsQml',
                'Qt6OpenGL','Qt6OpenGLWidgets','Qt6Concurrent','Qt6PrintSupport')
    foreach ($d in $qtDlls) { $s = Join-Path $QtBin "$d.dll"; if (Test-Path $s) { Copy-Item $s $stage -Force } }
    foreach ($rt in @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
        $s = Join-Path $QtBin $rt; if (Test-Path $s) { Copy-Item $s $stage -Force }
    }
    foreach ($set in @('platforms','imageformats','iconengines','styles','multimedia',
                       'tls','networkinformation','generic','printsupport')) {
        $s = Join-Path $qtRoot "plugins\$set"
        if (Test-Path $s) { Copy-Item $s (Join-Path $stage $set) -Recurse -Force }
    }
    foreach ($m in @('QtQuick','QtQml','QtCharts')) {
        $s = Join-Path $qtRoot "qml\$m"
        if (Test-Path $s) { Copy-Item $s (Join-Path $stage $m) -Recurse -Force }
    }
    $ge = Join-Path $qtRoot 'qml\Qt5Compat\GraphicalEffects'
    if (Test-Path $ge) {
        New-Item -ItemType Directory -Force (Join-Path $stage 'Qt5Compat') | Out-Null
        Copy-Item $ge (Join-Path $stage 'Qt5Compat\GraphicalEffects') -Recurse -Force
    }
    $vk = Join-Path $stage 'QtQuick\VirtualKeyboard'
    if (Test-Path $vk) { Remove-Item $vk -Recurse -Force }
}
Ok "Qt runtime deployed by: $deployedBy"
if (-not (Test-Path (Join-Path $stage 'platforms\qwindows.dll'))) {
    Fail 'no platform plugin in the payload - Qt would abort at startup'
}

# ---- 6. qualification configuration ---------------------------------------
# developer_mode=1 is the POINT of this build. Written here, never copied from
# the developer's own config.ini.
$cfg = @'
[shot_count_and_timer]
timer=yes

[App_Settings]
app_mode=Live
developer_mode=1
is_single_decimal=1
motor_movement_time=1
motor_movement_time_sighter=1
'@
[System.IO.File]::WriteAllText((Join-Path $stage 'config.ini'), ($cfg -replace "`r`n", "`n"),
                               (New-Object System.Text.UTF8Encoding($false)))
Ok 'config.ini (app_mode=Live, developer_mode=1 - INTERNAL)'

# ---- 7. the qualification plan travels with the build ---------------------
$docsOut = Join-Path $stage 'docs'
New-Item -ItemType Directory -Force $docsOut | Out-Null
foreach ($d in @('docs\field-tests\rc3b-diag-physical-qualification-plan.md',
                 'docs\field-tests\rc3b-diag-cross-discipline-audit.md',
                 'docs\field-tests\2026-08-23-tablet02-score-invalidation.md',
                 'docs\legal\THIRD-PARTY-NOTICES.md')) {
    $s = Join-Path $repo $d
    if (Test-Path $s) { Copy-Item $s (Join-Path $docsOut (Split-Path $d -Leaf)) }
}
Ok "documents: $((Get-ChildItem $docsOut).Count)"

# The emulator goes WITH it: the bench rehearsal in the plan needs it, and the
# operator should not have to build one.
$emu = Join-Path $repo 'tools\emulator\release\target_emulator.exe'
if (Test-Path $emu) { Copy-Item $emu $stage -Force; Ok 'target_emulator.exe (bench rehearsal)' }

# ---- 8. source-leak audit --------------------------------------------------
$leaks = Get-ChildItem $stage -Recurse -File |
         Where-Object { $_.Extension -in '.cpp','.h','.pro','.pri','.o','.obj','.tch','.log' -and
                        $_.FullName -notmatch '\\QtQuick\\|\\QtQml\\|\\QtCharts\\' }
if ($leaks) { Fail "source or build artefacts in the payload:`n$($leaks.Name -join "`n")" }
Ok 'no source, object or session file in the payload'

# ---- 9. zip + hashes -------------------------------------------------------
Write-Host "== package =="
$zip = Join-Path $outRoot "$name.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$zipHash = (Get-FileHash $zip -Algorithm SHA256).Hash
$exeHash = (Get-FileHash (Join-Path $stage 'TechAim.exe') -Algorithm SHA256).Hash
$files   = @(Get-ChildItem $stage -Recurse -File)

$manifest = [ordered]@{
    product        = 'Tech Aim Electronic Target Control'
    version        = $version
    channel        = 'INTERNAL PHYSICAL QUALIFICATION - not a SETA evaluation build'
    developerMode  = 1
    appMode        = 'Live'
    branch         = $branch
    commit         = $commit
    upstream       = $remote
    builtUtc       = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    qtVersion      = '6.5.3'
    toolchain      = 'MinGW 11.2.0 64-bit'
    zip            = (Split-Path $zip -Leaf)
    zipSha256      = $zipHash
    exeSha256      = $exeHash
    fileCount      = $files.Count
    payloadBytes   = ($files | Measure-Object Length -Sum).Sum
    qtRuntimeBy    = $deployedBy
    testTotals     = ($script:totals -join ' ')
    physicalStatus = 'PENDING - no range result exists for this build'
}
$manifest | ConvertTo-Json -Depth 4 |
    Set-Content (Join-Path $outRoot "$name-manifest.json") -Encoding utf8

Write-Host ""
Ok "zip           $zip"
Ok "zip  SHA-256  $zipHash"
Ok "exe  SHA-256  $exeHash"
Ok "commit        $commit ($branch)"
Ok "files         $($files.Count)"
Write-Host ""
Write-Host "RC3B-DIAG is INTERNAL. Do not send it to SETA." -ForegroundColor Yellow
Write-Host "Physical qualification: docs\field-tests\rc3b-diag-physical-qualification-plan.md"
