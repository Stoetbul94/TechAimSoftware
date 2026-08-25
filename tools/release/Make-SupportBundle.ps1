# Tech Aim — field-test support bundle.
#
# One command an operator can run at the range when something goes wrong. It
# collects what a diagnosis needs and NOTHING else: no passwords, no tokens, no
# unrelated athletes' sessions, no personal files.
#
#   powershell -File Make-SupportBundle.ps1
#   powershell -File Make-SupportBundle.ps1 -SessionId 3f9c1a22
#
# -SessionId includes that one session's journal. Without it, no session data
# is collected at all — inclusion is opt-in, never automatic.
[CmdletBinding()]
param(
    [string]$SessionId = '',
    [string]$OutDir    = "$env:USERPROFILE\Desktop",
    [int]$LogCount     = 5,
    # SUP-001. The Git commit is baked into the binary at build time and is not
    # reliably readable from outside, so it comes from the field-kit release
    # manifest rather than being scraped. Auto-discovered beside this script,
    # one level up, or in a manifest/ documents/ docs/ folder.
    [string]$ReleaseManifest = ''
)
$ErrorActionPreference = 'Stop'

$stamp = Get-Date -Format 'yyyy-MM-dd-HHmmss'
$name  = "TechAim-Support-$stamp"
$work  = Join-Path $env:TEMP $name
$zip   = Join-Path $OutDir "$name.zip"
if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force $work | Out-Null

$appData = Join-Path $env:LOCALAPPDATA 'TechAim'
$here    = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "== Tech Aim support bundle =="

# ---- release manifest (SUP-001) ------------------------------------------
$manifest = $null
$manifestPath = ''
$candidates = @()
if ($ReleaseManifest) { $candidates += $ReleaseManifest }
foreach ($dir in @($here, (Split-Path $here -Parent))) {
    if (-not $dir) { continue }
    $candidates += (Join-Path $dir '0.9.0-rc1-release-manifest.json')
    $candidates += (Join-Path $dir 'release-manifest.json')
    foreach ($sub in @('manifest', 'documents', 'docs')) {
        $candidates += (Join-Path (Join-Path $dir $sub) '0.9.0-rc1-release-manifest.json')
        $candidates += (Join-Path (Join-Path $dir $sub) 'release-manifest.json')
    }
}
foreach ($c in $candidates) {
    if ($c -and (Test-Path $c)) {
        try { $manifest = Get-Content $c -Raw | ConvertFrom-Json; $manifestPath = $c; break }
        catch { Write-Warning "Manifest at $c could not be parsed: $($_.Exception.Message)" }
    }
}
if (-not $manifest) {
    Write-Warning "No release manifest found. The bundle will report the Git commit as UNKNOWN rather than guess."
}

# ---- release + machine identity ------------------------------------------
$exe = Join-Path $here 'TechAim.exe'
$ver = if (Test-Path $exe) { (Get-Item $exe).VersionInfo.FileVersion } else { 'not found' }
$exeSha = if (Test-Path $exe) { (Get-FileHash $exe -Algorithm SHA256).Hash } else { '' }

# VERIFY, do not assert. If the running binary is not the one the manifest
# describes, the bundle says so - that is exactly when it matters most.
$mProduct   = if ($manifest) { $manifest.product }          else { 'Tech Aim Electronic Target Control' }
$mVersion   = if ($manifest) { $manifest.version }          else { 'UNKNOWN - no manifest' }
$mChannel   = if ($manifest) { $manifest.releaseChannel }   else { 'UNKNOWN - no manifest' }
$mCommit    = if ($manifest) { $manifest.gitCommit }        else { 'UNKNOWN - no manifest' }
$mLimit     = if ($manifest) { $manifest.limitation }       else { 'FIELD TEST - NOT FOR OFFICIAL COMPETITION RESULTS' }
$mAnalytics = if ($manifest) { $manifest.analyticsVersion } else { 'UNKNOWN - no manifest' }
$mExeSha    = if ($manifest) { $manifest.executableSha256 } else { '' }
$shaVerdict =
    if (-not $exeSha)      { 'NOT CHECKED - TechAim.exe not found beside this script' }
    elseif (-not $mExeSha) { 'NOT CHECKED - no manifest hash to compare against' }
    elseif ($exeSha -eq $mExeSha) { 'MATCH - the running binary is the one the manifest describes' }
    else { 'MISMATCH - the binary beside this script is NOT the manifest build; the Git commit above may be wrong' }

$modeLine = 'UNKNOWN'
$cfgProbe = Join-Path $here 'config.ini'
if (Test-Path $cfgProbe) {
    $mm = (Get-Content $cfgProbe | Select-String -Pattern '^\s*app_mode\s*=\s*(.+)$')
    if ($mm) { $modeLine = $mm.Matches[0].Groups[1].Value.Trim() }
}
$identity = @"
Tech Aim support bundle
Generated            : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Product              : $mProduct
Version              : $mVersion
Release channel      : $mChannel
Git commit           : $mCommit
Analytics version    : $mAnalytics
Limitation           : $mLimit
Operating mode       : $modeLine
Executable           : $exe
Executable version   : $ver
Executable SHA-256   : $(if ($exeSha) { $exeSha } else { 'n/a' })
Manifest SHA-256     : $(if ($mExeSha) { $mExeSha } else { '(none)' })
Binary vs manifest   : $shaVerdict
Manifest file        : $(if ($manifestPath) { $manifestPath } else { '(none found)' })
Windows              : $([System.Environment]::OSVersion.VersionString)
Windows product      : $((Get-CimInstance Win32_OperatingSystem).Caption)
Architecture         : $env:PROCESSOR_ARCHITECTURE
Machine              : $env:COMPUTERNAME
AppData root         : $appData
Session id requested : $(if ($SessionId) { $SessionId } else { '(none - no session data collected)' })

The Git commit above comes from the field-kit release manifest and is checked
against the executable SHA-256 - see "Binary vs manifest". The build date and
Qt version are shown in the application under Settings > ABOUT / BUILD.
"@
[System.IO.File]::WriteAllText((Join-Path $work 'release-identity.txt'), $identity)

# ---- logs ----------------------------------------------------------------
# TWO locations, and the second one is the one that matters.
#
# This used to collect AppData\TechAim\Logs only. That folder is EMPTY on
# every tablet in the 2026-08-23 evidence set - all four of them - because the
# application writes its log through LogFile, which uses
# QStandardPaths::TempLocation. So the bundle gathered configuration and
# identity and no log at all, and the investigation into the repeated 10.8
# had nothing to read for the window in which it happened.
#
# %TEMP% is also cleaned by Windows, so a log not collected soon is a log
# gone. Collect after every test.
$logOut = Join-Path $work 'Logs'
New-Item -ItemType Directory -Force $logOut | Out-Null
$collected = 0

$logSrc = Join-Path $appData 'Logs'
if (Test-Path $logSrc) {
    Get-ChildItem $logSrc -File | Sort-Object LastWriteTime -Descending |
        Select-Object -First $LogCount |
        ForEach-Object { Copy-Item $_.FullName $logOut; $collected++ }
}

# The application log itself.
Get-ChildItem $env:TEMP -Filter 'tachus_log*.log' -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First $LogCount |
    ForEach-Object { Copy-Item $_.FullName $logOut; $collected++ }

# The serial parameters the application actually used. SERIAL-DEFAULT-005 was
# a wrong baud and parity for an hour on two tablets; this is how that is seen
# afterwards. No secrets live in this file.
$qmm = Join-Path $env:TEMP 'qModMaster.ini'
if (Test-Path $qmm) { Copy-Item $qmm (Join-Path $work 'qModMaster.ini') }

if ($collected -eq 0) {
    [System.IO.File]::WriteAllText((Join-Path $logOut 'NO-LOGS-FOUND.txt'),
        "No application log was found in either location:`n" +
        "  $logSrc`n" +
        "  $env:TEMP\tachus_log*.log`n`n" +
        "If the application ran on this machine, %TEMP% may have been cleaned.`n" +
        "Say so in the report - an empty Logs folder is a finding, not a blank.")
}

# ---- sanitized configuration ---------------------------------------------
# Whole-line drop of anything that looks like a secret. A key is reported as
# present-but-removed rather than silently omitted, so a reader can tell the
# difference between "no such setting" and "withheld".
$cfgSrc = Join-Path $here 'config.ini'
if (Test-Path $cfgSrc) {
    $clean = Get-Content $cfgSrc | ForEach-Object {
        if ($_ -match '(?i)(pass|pwd|secret|token|key|licence|license|serial)\s*=') {
            ($_ -replace '=.*', '= [REMOVED FROM SUPPORT BUNDLE]')
        } else { $_ }
    }
    $clean | Set-Content (Join-Path $work 'config.sanitized.ini') -Encoding utf8
}

# ---- target communication summary ----------------------------------------
$commLines = @()
if (Test-Path $logSrc) {
    $commLines = Get-ChildItem $logSrc -File | Sort-Object LastWriteTime -Descending |
        Select-Object -First $LogCount | ForEach-Object { Get-Content $_.FullName } |
        Select-String -Pattern 'target|modbus|COM\d|connect|disconnect|protocol|malformed' |
        Select-Object -Last 400 | ForEach-Object { $_.Line }
}
$commText = if ($commLines.Count) { $commLines -join "`n" } else { 'No target communication lines found in the collected logs.' }
[System.IO.File]::WriteAllText((Join-Path $work 'target-communication.txt'), $commText)

# ---- ONE session journal, only when explicitly asked for -----------------
if ($SessionId) {
    $sesOut = Join-Path $work 'Session'
    New-Item -ItemType Directory -Force $sesOut | Out-Null
    $hits = Get-ChildItem (Join-Path $appData 'Sessions') -Recurse -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -like "*$SessionId*" }
    foreach ($h in $hits) { Copy-Item $h.FullName (Join-Path $sesOut $h.Name) -Recurse }
    if (-not $hits) {
        [System.IO.File]::WriteAllText((Join-Path $sesOut 'NOT-FOUND.txt'),
            "No session directory matched '$SessionId'.")
    }
}

# ---- crash information ---------------------------------------------------
$crashSrc = Join-Path $appData 'SupportBundles'
if (Test-Path $crashSrc) {
    $c = Get-ChildItem $crashSrc -File -ErrorAction SilentlyContinue |
         Sort-Object LastWriteTime -Descending | Select-Object -First 3
    if ($c) {
        $crashOut = Join-Path $work 'Crash'
        New-Item -ItemType Directory -Force $crashOut | Out-Null
        $c | ForEach-Object { Copy-Item $_.FullName $crashOut }
    }
}

# ---- known limitations ---------------------------------------------------
$lim = Join-Path $here 'docs\0.9.0-rc1-known-limitations.md'
if (Test-Path $lim) { Copy-Item $lim (Join-Path $work 'known-limitations.md') }

# ---- the manifest the identity was read from -----------------------------
# Shipped so a reader can check the claim rather than take it on trust.
if ($manifestPath) { Copy-Item $manifestPath (Join-Path $work 'release-manifest.json') }

# ---- zip -----------------------------------------------------------------
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $work '*') -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $work -Recurse -Force

Write-Host ""
Write-Host "Support bundle : $zip"
Write-Host "SHA-256        : $((Get-FileHash $zip -Algorithm SHA256).Hash)"
Write-Host ""
Write-Host "It contains logs, release identity, a sanitized configuration and a"
Write-Host "target-communication summary. Session data is included ONLY when you"
Write-Host "pass -SessionId. Please check the contents before sending it on."
