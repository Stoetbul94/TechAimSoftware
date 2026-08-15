# ─────────────────────────────────────────────────────────────────────────────
# Tech Aim 0.9.0-RC3 — SETA Evaluation: staging, audit and installer build.
#
# NEW SCRIPT ON PURPOSE. build-rc1-package.ps1 is left untouched so RC1/RC2
# packages stay reproducible; this one owns RC3 and its extra gates.
#
# The payload is built by ALLOW-LIST into a fresh staging directory. The
# development release\ tree is NEVER copied recursively: it holds object files,
# generated C++ (including qrc_qml.cpp, which contains every QML file as byte
# arrays), development .tch sessions, logs and report PDFs.
#
#   powershell -ExecutionPolicy Bypass -File tools\release\build-rc3-seta-package.ps1
#
# Produces dist\rc3\TechAim-0.9.0-RC3-SETA-Evaluation-Setup.exe plus a manifest.
# It does not send anything anywhere.
# ─────────────────────────────────────────────────────────────────────────────
param(
    [string]$QtBin   = 'C:\Qt\6.5.3\mingw_64\bin',
    [string]$Iscc    = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    [switch]$SkipRegression
)

$ErrorActionPreference = 'Stop'
$repo    = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$version = '0.9.0-RC3a'
$name    = 'TechAim-0.9.0-RC3a-SETA-Evaluation'
$outRoot = Join-Path $repo 'dist\rc3'
$stage   = Join-Path $outRoot 'staging'

function Fail($m) { Write-Host "FAIL  $m" -ForegroundColor Red; exit 1 }
function Ok($m)   { Write-Host "  ok  $m" }

# ---- 1. repository state --------------------------------------------------
Write-Host "== repository state =="
Push-Location $repo
$dirty  = (git status --porcelain)
$commit = (git rev-parse --short HEAD)
$remote = (git rev-parse --short '@{u}' 2>$null)
Pop-Location
if ($dirty) { Fail "working tree is not clean:`n$dirty" }
if ($remote -and $commit -ne $remote) { Fail "local ($commit) != remote ($remote)" }
Ok "clean at $commit (local == remote)"

# ---- 2. required configuration -------------------------------------------
$proVer = (Select-String -Path (Join-Path $repo 'Seta.pro') -Pattern 'APP_VERSION_STR\s*=\s*(\S+)').Matches[0].Groups[1].Value
if ($proVer -notlike '0.9.0-RC3a*') { Fail "Seta.pro APP_VERSION_STR is '$proVer', expected 0.9.0-RC3a*" }
Ok "APP_VERSION_STR = $proVer"

# ---- 3. Release build ------------------------------------------------------
Write-Host "== Release build =="
$exe = Join-Path $repo 'release\TechAim.exe'
if (-not (Test-Path $exe)) { Fail "release\TechAim.exe not found - build Release first" }
Ok "TechAim.exe present"

# ---- 4. release gates ------------------------------------------------------
if (-not $SkipRegression) {
    Write-Host "== regression gates =="
    $suites = @{ reliability='tests\reliability\release'; training='tests\training\release';
                 finals10m='tests\finals10m\release';   finals='tests\finals\release';
                 qml='tests\qml\release' }
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:Path = "$QtBin;$env:Path"
    foreach ($s in $suites.GetEnumerator()) {
        $bin = Get-ChildItem (Join-Path $repo $s.Value) -Filter '*.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $bin) { Fail "no test binary for $($s.Key)" }
        $line = & $bin.FullName 2>$null | Select-String '=== \d+ checks' | Select-Object -Last 1
        if (-not $line -or $line -notmatch '(\d+) checks, (\d+) failures') { Fail "$($s.Key): no result line" }
        if ([int]$Matches[2] -ne 0) { Fail "$($s.Key): $($Matches[2]) failures" }
        Ok "$($s.Key): $($Matches[1]) checks, 0 failures"
    }
}

# ---- 5. fresh staging ------------------------------------------------------
Write-Host "== staging =="
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Ok "fresh staging at $stage"

# ---- 6. curated runtime allow-list ----------------------------------------
Copy-Item $exe (Join-Path $stage 'TechAim.exe')
Ok 'TechAim.exe'

# ---- 7/8. Qt runtime ------------------------------------------------------
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
    # Known Qt 6.5.3 MinGW windeployqt platform-plugin detection failure; the
    # allow-list below is windeployqt's own resolved module set plus the QML
    # modules this application actually imports.
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

# ---- 9. fresh RC3 configuration -------------------------------------------
# Written here, never copied: the developer's config.ini carries a local mode,
# developer flag and machine-specific values.
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
[System.IO.File]::WriteAllText((Join-Path $stage 'config.ini'), ($cfg -replace "`r`n", "`n"),
                               (New-Object System.Text.UTF8Encoding($false)))
Ok 'config.ini (app_mode=Live, developer_mode=0)'

# ---- 10. translations ------------------------------------------------------
foreach ($qm in @('german.qm','french.qm','chinese.qm')) {
    $s = Join-Path $repo "translations\$qm"
    if (Test-Path $s) { Copy-Item $s $stage -Force }
}

# ---- 11. external-facing documents ONLY ------------------------------------
$docsOut = Join-Path $stage 'docs'
New-Item -ItemType Directory -Force $docsOut | Out-Null
foreach ($d in @('docs\release\0.9.0-rc3-seta-manual-check.md',
                 'docs\release\0.9.0-rc2-known-limitations.md',
                 'docs\legal\THIRD-PARTY-NOTICES.md')) {
    $s = Join-Path $repo $d
    if (Test-Path $s) { Copy-Item $s (Join-Path $docsOut (Split-Path $d -Leaf)) }
}
foreach ($l in @('LICENSE','LICENSE.txt')) {
    $s = Join-Path $repo $l; if (Test-Path $s) { Copy-Item $s $stage -Force }
}
# Operator manuals, as generated PDFs - never the markdown source.
$manOut = Join-Path $docsOut 'manuals'
New-Item -ItemType Directory -Force $manOut | Out-Null
Get-ChildItem (Join-Path $repo 'docs\manual\output') -Filter '*.pdf' -ErrorAction SilentlyContinue |
    ForEach-Object { Copy-Item $_.FullName $manOut -Force }
Ok "documents + $((Get-ChildItem $manOut -Filter *.pdf).Count) manual PDFs"

# ---- 12. support-bundle utility (approved exception) ----------------------
Copy-Item (Join-Path $repo 'tools\deployment\Make-SupportBundle.ps1') $stage -Force
Ok 'Make-SupportBundle.ps1 (approved runtime support utility)'

# ---- 13. SOURCE-LEAKAGE SCAN — HARD GATE ----------------------------------
Write-Host "== source-leakage scan =="
$banned = @('*.cpp','*.cc','*.cxx','*.h','*.hpp','*.pro','*.pri','CMakeLists.txt','*.cmake',
            '*.o','*.obj','*.pdb','*.py','*.bat','*.cmd','*.tch','*.log')
$leaks = @()
foreach ($pat in $banned) {
    $leaks += Get-ChildItem $stage -Recurse -File -Filter $pat -ErrorAction SilentlyContinue
}
# .ps1 is banned EXCEPT the approved support utility.
$leaks += Get-ChildItem $stage -Recurse -File -Filter '*.ps1' -ErrorAction SilentlyContinue |
          Where-Object { $_.Name -ne 'Make-SupportBundle.ps1' }
foreach ($d in @('.git','tests','tools','emulator','.claude')) {
    if (Test-Path (Join-Path $stage $d)) { $leaks += Get-Item (Join-Path $stage $d) }
}
if ($leaks) {
    $leaks | ForEach-Object { Write-Host "   LEAK: $($_.FullName.Substring($stage.Length))" -ForegroundColor Red }
    Fail "source-leakage scan found $($leaks.Count) item(s)"
}
Ok 'no source, tests, tools, build files, sessions or logs in the payload'

# ---- 14. proprietary QML scan ---------------------------------------------
# Qt's own modules legitimately ship .qml. Tech Aim's application QML must be
# embedded in the binary, so ANY staged .qml whose name matches a repository
# root .qml is a leak.
$appQml = Get-ChildItem $repo -Filter '*.qml' -File | Select-Object -ExpandProperty Name
$staged = Get-ChildItem $stage -Recurse -File -Filter '*.qml' -ErrorAction SilentlyContinue
$propr  = $staged | Where-Object { $appQml -contains $_.Name }
if ($propr) {
    $propr | ForEach-Object { Write-Host "   PROPRIETARY QML: $($_.FullName)" -ForegroundColor Red }
    Fail "proprietary Tech Aim QML present in the payload"
}
Ok "PROPRIETARY LOOSE QML = 0  ($($staged.Count) Qt-supplied .qml files are runtime module content)"

# ---- 15. legacy-brand scan -------------------------------------------------
$brand = Get-ChildItem $stage -Recurse -File -ErrorAction SilentlyContinue |
         Where-Object { $_.Name -match '(?i)tachus' }
if ($brand) { $brand | ForEach-Object { Write-Host "   LEGACY: $($_.Name)" }; Fail 'legacy Tachus asset in payload' }
Ok 'no legacy Tachus files in the payload'

# ---- 16. clean first-run scan ---------------------------------------------
$dirtyData = Get-ChildItem $stage -Recurse -File -ErrorAction SilentlyContinue |
             Where-Object { $_.Extension -in '.tch','.log' -or $_.Name -like 'SupportBundle*' }
if ($dirtyData) { Fail "development/user data in payload: $($dirtyData.Name -join ', ')" }
Ok 'ATHLETE DATA = 0, .tch = 0, LOGS = 0, SUPPORT BUNDLES = 0'

# ---- 17. installer ---------------------------------------------------------
Write-Host "== installer =="
if (-not (Test-Path $Iscc)) { Fail "Inno Setup compiler not found at $Iscc" }
$iss = Join-Path $PSScriptRoot 'TechAim-RC3-SETA.iss'
& $Iscc "/DStageDir=$stage" "/DOutDir=$outRoot" "/DAppVer=$version" $iss | Select-Object -Last 3
$setup = Join-Path $outRoot "$name-Setup.exe"
if (-not (Test-Path $setup)) { Fail 'installer was not produced' }
Ok (Split-Path $setup -Leaf)

# ---- 18. hashes + manifest -------------------------------------------------
$hSetup = (Get-FileHash $setup -Algorithm SHA256).Hash
$hExe   = (Get-FileHash (Join-Path $stage 'TechAim.exe') -Algorithm SHA256).Hash
$manifest = [ordered]@{
    product          = 'Tech Aim'
    version          = $version
    channel          = 'SETA Evaluation'
    sourceCommit     = $commit
    installer        = (Split-Path $setup -Leaf)
    installerSha256  = $hSetup
    techAimExeSha256 = $hExe
    buildTimestampUtc= (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    qtVersion        = '6.5.3 MinGW 64-bit'
    compiler         = 'MinGW 11.2.0'
    appMode          = 'Live'
    developerMode    = 0
    qtDeployedBy     = $deployedBy
    payloadFileCount = (Get-ChildItem $stage -Recurse -File).Count
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $outRoot "$name-manifest.json") -Encoding utf8
Write-Host ""
Write-Host "installer      : $setup"
Write-Host "installer sha  : $hSetup"
Write-Host "TechAim.exe sha: $hExe"
Write-Host "source commit  : $commit"
Write-Host "DONE - nothing has been sent anywhere."
