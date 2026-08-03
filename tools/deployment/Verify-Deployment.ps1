# Tech Aim — deployment package verification.
#
# Checks ONE extracted portable package against a release manifest and fails
# loudly when anything does not match. Written to be run by an operator on the
# deployment machine, not only by a developer in the repository.
#
#   powershell -File Verify-Deployment.ps1 -PackageDir C:\TechAim\RC2a
#   powershell -File Verify-Deployment.ps1 -PackageDir . -Manifest .\release-manifest.json
#   powershell -File Verify-Deployment.ps1 -PackageDir . -ExpectDeveloperMode 1
#
# Exit code 0 = every check passed. Exit code 1 = at least one FAIL.
# A WARN never fails the run; it marks something that could not be checked here.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$PackageDir,
    [string]$Manifest = '',
    # RC2a's diagnostic field test deliberately runs with developer_mode=1.
    # Production deployment must be 0. The caller states which it expects, so
    # a diagnostic package can never quietly pass a production verification.
    [ValidateSet('0', '1')][string]$ExpectDeveloperMode = '0',
    [string]$ExpectVersion = '',
    [string]$ExpectChannel = '',
    # Optional: the ZIP this directory was extracted from, hashed and compared
    # against the manifest so provenance is checked, not assumed.
    [string]$SourceZip = ''
)
$ErrorActionPreference = 'Stop'

$script:Pass = 0
$script:Fail = 0
$script:Warn = 0

function Ok   ($n, $d = '') { $script:Pass++; Write-Host ("PASS  {0}{1}" -f $n, $(if ($d) { "  ($d)" })) }
function Bad  ($n, $d = '') { $script:Fail++; Write-Host ("FAIL  {0}{1}" -f $n, $(if ($d) { "  -> $d" })) -ForegroundColor Red }
function Warn ($n, $d = '') { $script:Warn++; Write-Host ("WARN  {0}{1}" -f $n, $(if ($d) { "  ($d)" })) -ForegroundColor Yellow }
function Check($cond, $n, $d = '') { if ($cond) { Ok $n } else { Bad $n $d } }

Write-Host "=== Tech Aim deployment verification ==="
Write-Host "package : $PackageDir"

if (-not (Test-Path $PackageDir)) {
    Bad "the package directory exists" $PackageDir
    Write-Host "`n=== $script:Pass passed, $script:Fail failed, $script:Warn warnings ==="
    exit 1
}
$PackageDir = (Resolve-Path $PackageDir).Path
Ok "the package directory exists" $PackageDir

# Inventory once. Everything below reads this list rather than re-walking.
$files = Get-ChildItem $PackageDir -Recurse -File
$rels  = $files | ForEach-Object { $_.FullName.Substring($PackageDir.Length).TrimStart('\') }
Write-Host "files   : $($rels.Count)"

# ---- 1. the manifest ------------------------------------------------------
if (-not $Manifest) {
    foreach ($c in @('release-manifest.json',
                     'manifests\release-manifest.json',
                     'docs\release-manifest.json')) {
        $p = Join-Path $PackageDir $c
        if (Test-Path $p) { $Manifest = $p; break }
    }
}
$mf = $null
if ($Manifest -and (Test-Path $Manifest)) {
    try {
        $mf = Get-Content $Manifest -Raw | ConvertFrom-Json
        Ok "a release manifest was found and parsed" $Manifest
    } catch {
        Bad "the release manifest parses as JSON" $_.Exception.Message
    }
} else {
    Warn "no release manifest found - hash comparison is skipped, not assumed to pass"
}

# ---- 2. the executable ----------------------------------------------------
$exe = Join-Path $PackageDir 'TechAim.exe'
Check (Test-Path $exe) "TechAim.exe is present" $exe

if (Test-Path $exe) {
    $exeSha = (Get-FileHash $exe -Algorithm SHA256).Hash
    Write-Host "        TechAim.exe SHA-256 : $exeSha"
    if ($mf -and $mf.executableSha256) {
        Check ($exeSha -eq $mf.executableSha256) `
              "TechAim.exe matches the manifest hash" `
              "package=$exeSha manifest=$($mf.executableSha256)"
    } else {
        Warn "no manifest executable hash to compare against"
    }

    # File version is the qmake VERSION (0.9.0). The full release string
    # (0.9.0-RC2a) and the channel live inside the binary and are shown in the
    # application, so they are checked as embedded strings where practical
    # rather than claimed from the file-version resource alone.
    $fv = (Get-Item $exe).VersionInfo.FileVersion
    Write-Host "        file version        : $fv"
    if ($ExpectVersion -or $ExpectChannel) {
        # Read the binary once and look for the literal UTF-16/ASCII markers.
        $bytes = [System.IO.File]::ReadAllBytes($exe)
        $ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
        $utf16 = [System.Text.Encoding]::Unicode.GetString($bytes)
        if ($ExpectVersion) {
            $hit = $ascii.Contains($ExpectVersion) -or $utf16.Contains($ExpectVersion)
            Check $hit "the expected release version is embedded in the executable" $ExpectVersion
        }
        if ($ExpectChannel) {
            $hit = $ascii.Contains($ExpectChannel) -or $utf16.Contains($ExpectChannel)
            if ($hit) { Ok "the expected release channel is embedded in the executable" $ExpectChannel }
            else { Warn "release channel '$ExpectChannel' not found as a literal in the executable - confirm on screen at first start" }
        }
    }
}

# ---- 3. provenance of the ZIP --------------------------------------------
if ($SourceZip) {
    if (Test-Path $SourceZip) {
        $zipSha = (Get-FileHash $SourceZip -Algorithm SHA256).Hash
        Write-Host "        source ZIP SHA-256  : $zipSha"
        if ($mf -and $mf.packageSha256) {
            Check ($zipSha -eq $mf.packageSha256) "the source ZIP matches the manifest hash" `
                  "zip=$zipSha manifest=$($mf.packageSha256)"
        } else {
            Warn "no manifest package hash to compare the ZIP against"
        }
    } else {
        Bad "the named source ZIP exists" $SourceZip
    }
}

# ---- 4. required Qt runtime ----------------------------------------------
foreach ($d in @('Qt6Core.dll','Qt6Gui.dll','Qt6Qml.dll','Qt6Quick.dll','Qt6Widgets.dll',
                 'Qt6Network.dll','Qt6SerialPort.dll','Qt6Charts.dll','Qt6Multimedia.dll',
                 'Qt6OpenGL.dll','Qt6OpenGLWidgets.dll')) {
    Check (Test-Path (Join-Path $PackageDir $d)) "required Qt library present: $d"
}
# The MinGW C++ runtime. Without these the application will not start at all on
# a machine that has never had a compiler installed - which is every real one.
foreach ($d in @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
    Check (Test-Path (Join-Path $PackageDir $d)) "compiler runtime present: $d"
}
# Plugins.
Check (Test-Path (Join-Path $PackageDir 'platforms\qwindows.dll')) `
      "the Qt Windows platform plugin is present" `
      "without platforms\qwindows.dll the application cannot create a window"
foreach ($p in @('imageformats','styles','multimedia','iconengines')) {
    Check (Test-Path (Join-Path $PackageDir $p)) "plugin set present: $p"
}
Check ((Test-Path (Join-Path $PackageDir 'QtQuick')) -or (Test-Path (Join-Path $PackageDir 'QtQuick.2'))) `
      "the QtQuick QML modules are present"

# ---- 5. nothing that must never ship -------------------------------------
$forbiddenExt = @('.cpp','.h','.hpp','.cc','.cxx','.o','.obj','.a','.lib','.pdb','.ilk',
                  '.exp','.pro','.pri','.qrc','.user','.py','.pem','.key','.pfx','.p12')
foreach ($e in $forbiddenExt) {
    $bad = @($rels | Where-Object { $_.ToLower().EndsWith($e) })
    Check ($bad.Count -eq 0) "no $e files are packaged" (($bad | Select-Object -First 3) -join ', ')
}

# Qt's own QML modules ship .qml by design - that is what a QML module IS.
# Tech Aim QML must never appear: the interface is compiled into the binary's
# resources, so a loose copy would be source code.
$qtQmlRoots = @('QtQuick','QtQml','QtCharts','Qt5Compat','Qt','QtCore','QtMultimedia','QtQuick.2','QtTest')
$appQml = @($rels | Where-Object {
    $_.ToLower().EndsWith('.qml') -and ($qtQmlRoots -notcontains ($_ -split '\\')[0])
})
Check ($appQml.Count -eq 0) "no Tech Aim QML source is packaged" (($appQml | Select-Object -First 3) -join ', ')

# Directories that must not appear at any depth.
foreach ($d in @('.git','.github','.claude','tests','src','ModReader','dist','node_modules','__pycache__','debug')) {
    $bad = @($rels | Where-Object { $_ -like "$d\*" -or $_ -like "*\$d\*" })
    Check ($bad.Count -eq 0) "no '$d' directory is packaged" (($bad | Select-Object -First 2) -join ', ')
}

# Test executables and build tooling.
$bad = @($rels | Where-Object { $_ -match '(?i)(_tests?\.exe$|^Makefile|\.bat$|qmake|mingw32-make)' })
Check ($bad.Count -eq 0) "no test executables or build scripts are packaged" (($bad | Select-Object -First 3) -join ', ')

# Athlete / seeded / private data. Journals and session stores are per-user
# runtime data in AppData and must never be inside a distributable package.
$bad = @($rels | Where-Object { $_ -match '(?i)(\.jsonl$|session.*\.json$|snapshot\.json$|_session\b)' })
Check ($bad.Count -eq 0) "no session journals or snapshots are packaged" (($bad | Select-Object -First 3) -join ', ')
$seedNames = @('Fitzwilliam','Short-Session','Arnold Bailie','windmap-review','finals_session')
$bad = @($rels | Where-Object { $n = $_; ($seedNames | Where-Object { $n -like "*$_*" }).Count -gt 0 })
Check ($bad.Count -eq 0) "no seeded athlete or review data is packaged" (($bad | Select-Object -First 3) -join ', ')
$bad = @($rels | Where-Object { $_ -match '(?i)(^Logs\\|\\Logs\\|tachus_log.*\.log$|\.log$)' })
Check ($bad.Count -eq 0) "no logs are packaged" (($bad | Select-Object -First 3) -join ', ')

# ---- 6. no secrets and no developer/personal paths in shipped text -------
$textExt = @('.ini','.txt','.md','.json','.conf','.ps1','.xml')
$secretRe = [regex]'(?i)\b(password|passwd|secret|api[_-]?key|token|private[_-]?key)\b\s*[:=]\s*\S'
# An absolute path into a developer checkout, a Qt install, or ANY user profile
# other than a placeholder. A personal Windows username leaking into a shipped
# file is a privacy problem, not just untidiness.
$devPathRe = [regex]'(?i)([A-Z]:[\\/]Users[\\/][^\\/\s"'']+[\\/](Downloads|Documents|Desktop)[\\/]TechAimSoftware|[A-Z]:[\\/]Qt[\\/])'
foreach ($f in ($files | Where-Object { $textExt -contains $_.Extension.ToLower() })) {
    $rel = $f.FullName.Substring($PackageDir.Length).TrimStart('\')
    $body = Get-Content $f.FullName -Raw -ErrorAction SilentlyContinue
    if (-not $body) { continue }
    # The support-bundle script NAMES these words in order to strip them; it is
    # the one file allowed to mention them.
    if ($rel -notlike '*Make-SupportBundle.ps1' -and $rel -notlike '*Verify-Deployment.ps1') {
        $m = $secretRe.Match($body)
        Check (-not $m.Success) "$rel carries no secret-looking assignment" $m.Value
    }
    $d = $devPathRe.Match($body)
    Check (-not $d.Success) "$rel carries no absolute developer path" $d.Value
}

# ---- 7. the shipped configuration ----------------------------------------
$cfg = Join-Path $PackageDir 'config.ini'
Check (Test-Path $cfg) "a configuration template is packaged"
if (Test-Path $cfg) {
    $body = Get-Content $cfg -Raw
    Check ($body -match '(?m)^\s*app_mode\s*=\s*Live\s*$') "app_mode is Live"
    Check ($body -notmatch '(?im)^\s*app_mode\s*=\s*Demo\s*$') "no Demo mode is configured"
    $dm = [regex]::Match($body, '(?m)^\s*developer_mode\s*=\s*(\d)\s*$')
    if ($dm.Success) {
        Check ($dm.Groups[1].Value -eq $ExpectDeveloperMode) `
              "developer_mode is the expected value ($ExpectDeveloperMode)" `
              "found developer_mode=$($dm.Groups[1].Value); pass -ExpectDeveloperMode to change what is required"
    } else {
        Bad "developer_mode is stated explicitly in the configuration"
    }
    # Byte check, deliberately. String.StartsWith([char]0xFEFF) is a trap here:
    # .NET Framework has no StartsWith(char) overload, so PowerShell coerces to
    # StartsWith(String), which is CULTURE-SENSITIVE and treats the zero-width
    # BOM character as ignorable - it returns $true for every string. Verified.
    $cfgBytes = [System.IO.File]::ReadAllBytes($cfg)
    $hasBom = ($cfgBytes.Length -ge 3 -and $cfgBytes[0] -eq 0xEF -and
               $cfgBytes[1] -eq 0xBB -and $cfgBytes[2] -eq 0xBF)
    Check (-not $hasBom) "the configuration has no UTF-8 BOM" `
          "a BOM makes the first INI section header unreadable to QSettings"
    # Machine-specific serial state must never travel inside a package. The
    # remembered adapter belongs in the per-user registry, not in config.ini.
    Check ($body -notmatch '(?im)^\s*(serial_?port|com_?port|portname)\s*=\s*COM\d') `
          "no machine-specific COM port is baked into the configuration"
}

# ---- 8. operator documents + support tooling ------------------------------
Check (Test-Path (Join-Path $PackageDir 'Make-SupportBundle.ps1')) "the support-bundle tool is packaged"
$docs = Join-Path $PackageDir 'docs'
Check (Test-Path $docs) "operator documentation is packaged"

Write-Host ""
Write-Host "=== $script:Pass passed, $script:Fail failed, $script:Warn warnings ==="
if ($script:Fail -gt 0) {
    Write-Host "VERIFICATION FAILED - this package must not be deployed." -ForegroundColor Red
    exit 1
}
Write-Host "VERIFICATION PASSED for the checks above."
Write-Host "This confirms PACKAGE INTEGRITY only. It is not a hardware qualification,"
Write-Host "not a clean-machine test, and not deployment approval."
exit 0
