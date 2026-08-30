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
    # Session journals and match records from the test window. 0 = none.
    [int]$RecentHours  = 12,
    # SUP-001. The Git commit is baked into the binary at build time and is not
    # reliably readable from outside, so it comes from the field-kit release
    # manifest rather than being scraped. Auto-discovered beside this script,
    # one level up, or in a manifest/ documents/ docs/ folder.
    [string]$ReleaseManifest = ''
)
$ErrorActionPreference = 'Stop'

$stamp = Get-Date -Format 'yyyy-MM-dd-HHmmss'
# Named after the product it came from, so two products' bundles are not
# indistinguishable in an inbox. Resolved after the manifest is read.
$name  = "Support-$stamp"
$work  = Join-Path $env:TEMP $name
$zip   = Join-Path $OutDir "$name.zip"
if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force $work | Out-Null

# THE DATA ROOT IS TWO LEVELS DEEP, NOT ONE.
#
# Qt resolves AppLocalDataLocation as
#     %LOCALAPPDATA%\<organisationName>\<applicationName>
# so the sessions and journals live in
#     %LOCALAPPDATA%\TechAim\TechAim        (this product)
# and one folder per product line beside it, for any other edition installed.
# and NOT in %LOCALAPPDATA%\TechAim, which is only the vendor folder.
#
# This script looked in the vendor folder. %LOCALAPPDATA%\TechAim\Sessions
# does not exist for either product, so the -RecentHours and -SessionId
# searches walked an absent directory and reported 0 journals every time -
# silently, because -ErrorAction SilentlyContinue turns a missing path into an
# empty result. That is why the RC3F bundles carried no journals, and it would
# have taken the next field test down with it.
#
# EVERY product folder under the vendor root is searched now, so the bundle is
# correct whichever brand produced the data and whichever brand collects it.
$vendorRoot = Join-Path $env:LOCALAPPDATA 'TechAim'
$productRoots = @()
if (Test-Path $vendorRoot) {
    $productRoots = @(Get-ChildItem $vendorRoot -Directory -ErrorAction SilentlyContinue |
                      Where-Object { (Test-Path (Join-Path $_.FullName 'Sessions')) -or
                                     (Test-Path (Join-Path $_.FullName 'Logs')) } |
                      ForEach-Object { $_.FullName })
}
# The vendor root stays in the list so anything written directly there - by an
# older build, or by a future one - is still collected rather than missed.
if (Test-Path $vendorRoot) { $productRoots += $vendorRoot }
# $appData remains the vendor root for the paths that were always correct
# (SupportBundles, config); the per-product roots are used for the data.
$appData = $vendorRoot
$here    = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "== support bundle =="

# ---- release manifest (SUP-001) ------------------------------------------
$manifest = $null
$manifestPath = ''
$candidates = @()
if ($ReleaseManifest) { $candidates += $ReleaseManifest }
foreach ($dir in @($here, (Split-Path $here -Parent))) {
    if (-not $dir) { continue }
    $candidates += (Join-Path $dir '0.9.0-rc1-release-manifest.json')
    $candidates += (Join-Path $dir 'release-manifest.json')
    # The deployment writes deployment-manifest.json beside the executable.
    # Not looking for it is why a bundle taken from a deployed package reported
    # 'UNKNOWN - no manifest' for the version and the commit - the two things
    # the bundle exists to establish.
    $candidates += (Join-Path $dir 'deployment-manifest.json')
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
# The product executable is named by the BRAND, and a manifest may name a
# different one again. Discovered, never assumed: a bundle that cannot find the
# binary reports 'not found' for the version, which is the one field an
# investigation starts from.
$exeName = if ($manifest -and $manifest.executable) { $manifest.executable }
           elseif (Test-Path (Join-Path $here 'TechAim.exe')) { 'TechAim.exe' }
           elseif (Test-Path (Join-Path $here 'SETA.exe'))    { 'SETA.exe' }
           else { 'TechAim.exe' }
$exe = Join-Path $here $exeName
$ver = if (Test-Path $exe) { (Get-Item $exe).VersionInfo.FileVersion } else { 'not found' }
$exeSha = if (Test-Path $exe) { (Get-FileHash $exe -Algorithm SHA256).Hash } else { '' }

# VERIFY, do not assert. If the running binary is not the one the manifest
# describes, the bundle says so - that is exactly when it matters most.
# Read the PRODUCT from the manifest, then from the executable's own version
# resource. Falling back to a hardcoded 'Tech Aim' put another company's product
# name in an operator's support bundle.
$mProduct   = if ($manifest -and $manifest.product) { $manifest.product }
              elseif (Test-Path $exe) { (Get-Item $exe).VersionInfo.ProductName }
              else { 'UNKNOWN - no manifest and no executable' }
$mVersion   = if ($manifest -and $manifest.version) { $manifest.version }
              elseif ($ver -ne 'not found') { "$ver (from the executable)" }
              else { 'UNKNOWN - no manifest' }
$mChannel   = if ($manifest) { $manifest.releaseChannel }   else { 'UNKNOWN - no manifest' }
$mCommit    = if ($manifest) { $manifest.gitCommit }        else { 'UNKNOWN - no manifest' }
$mLimit     = if ($manifest) { $manifest.limitation }       else { 'NOT RECORDED - no manifest' }
$mAnalytics = if ($manifest) { $manifest.analyticsVersion } else { 'UNKNOWN - no manifest' }
$mExeSha    = if ($manifest) { $manifest.executableSha256 } else { '' }
# Now that the product is known, name the bundle after it.
# A product that could not be identified must not become a filename. Run from a
# deployed package the manifest names it; run from anywhere else the bundle is
# still valid - it just says so instead of inventing a name from an error string.
$brandLeaf = if ($mProduct -like 'UNKNOWN*') { 'UnidentifiedBuild' }
             else { ($mProduct -replace '[^A-Za-z0-9]', '') }
if (-not $brandLeaf) { $brandLeaf = 'Support' }
$name = "$brandLeaf-Support-$stamp"
$zip  = Join-Path $OutDir "$name.zip"
$newWork = Join-Path $env:TEMP $name
if ($work -ne $newWork) {
    if (Test-Path $newWork) { Remove-Item $newWork -Recurse -Force }
    Rename-Item $work $newWork
    $work = $newWork
}

$shaVerdict =
    if (-not $exeSha)      { "NOT CHECKED - $exeName not found beside this script" }
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
$mProduct support bundle
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

foreach ($pr in $productRoots) {
    $logSrc = Join-Path $pr 'Logs'
    if (Test-Path $logSrc) {
        Get-ChildItem $logSrc -File | Sort-Object LastWriteTime -Descending |
            Select-Object -First $LogCount |
            ForEach-Object { Copy-Item $_.FullName $logOut -Force; $collected++ }
    }
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

# ---- session records ------------------------------------------------------
# The RC3C physical audit could not check a single acquisition diagnostic, and
# could not answer whether a second motor command occurred, because the bundle
# carried no journal and no match record. Both are THIS application`s own
# competition records - not unrelated user files - so recent ones are now
# collected by default, scoped to the test window and listed in the summary so
# the operator can see exactly what is going out before sending it.
#
# -SessionId still narrows it to one session. -RecentHours 0 collects none.
$sesOut = Join-Path $work 'Session'
New-Item -ItemType Directory -Force $sesOut | Out-Null
$sesNotes = @()

if ($SessionId) {
    # Journals are FILES named session_<stamp>_<id>.jsonl, not directories.
    # Searching only for directories is why -SessionId could still come back
    # empty.
    $hits = @(foreach ($pr in $productRoots) {
                  Get-ChildItem (Join-Path $pr 'Sessions') -Recurse -ErrorAction SilentlyContinue |
                      Where-Object { $_.Name -like "*$SessionId*" }
              })
    foreach ($h in $hits) {
        if ($h.PSIsContainer) { Copy-Item $h.FullName (Join-Path $sesOut $h.Name) -Recurse }
        else                  { Copy-Item $h.FullName $sesOut }
    }
    $sesNotes += "SessionId '$SessionId': $($hits.Count) item(s)"
    if (-not $hits) {
        [System.IO.File]::WriteAllText((Join-Path $sesOut 'NOT-FOUND.txt'),
            "Nothing under Sessions matched '$SessionId'.")
    }
}
elseif ($RecentHours -gt 0) {
    $cut = (Get-Date).AddHours(-$RecentHours)
    $j = @(foreach ($pr in $productRoots) {
               Get-ChildItem (Join-Path $pr 'Sessions') -Recurse -File -Filter '*.jsonl' -ErrorAction SilentlyContinue |
                   Where-Object { $_.LastWriteTime -ge $cut }
           })
    # Two products can hold a journal of the same name; prefix with the
    # product folder so neither silently overwrites the other.
    foreach ($f in $j) {
        $leaf = Split-Path (Split-Path (Split-Path $f.FullName -Parent) -Parent) -Leaf
        Copy-Item $f.FullName (Join-Path $sesOut "$leaf-$($f.Name)") -Force
    }
    $sesNotes += "journals modified in the last $RecentHours h: $($j.Count)"

    # Match records sit beside the executable, not in AppData.
    $t = @(Get-ChildItem $here -File -Filter 'Match_*.tch' -ErrorAction SilentlyContinue |
          Where-Object { $_.LastWriteTime -ge $cut })
    foreach ($f in $t) { Copy-Item $f.FullName $sesOut }
    $sesNotes += "match records (.tch) in the last $RecentHours h: $($t.Count)"
}
else { $sesNotes += 'session data: not collected (-RecentHours 0)' }
$sesNotes += ''
$sesNotes += 'product data roots searched:'
if ($productRoots) { foreach ($pr in $productRoots) { $sesNotes += "  $pr" } }
else               { $sesNotes += '  NONE FOUND - the application has not run on this machine yet' }
[System.IO.File]::WriteAllText((Join-Path $sesOut 'WHAT-WAS-COLLECTED.txt'),
    ($sesNotes -join [Environment]::NewLine))

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
Write-Host "target-communication summary, and the session journals from the last"
Write-Host "$RecentHours hours. Please check the contents before sending it on."
