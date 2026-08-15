# Tech Aim — field support bundle (v2).
#
# One command an operator runs when something goes wrong. It collects what a
# diagnosis needs and NOTHING else: no passwords, no tokens, no unrelated
# athletes' sessions, no personal documents, no source code.
#
#   Normal deployment:
#     powershell -File Make-SupportBundle.ps1
#
#   RC2a diagnostic field test (captures the correlated shot timestamps):
#     powershell -File Make-SupportBundle.ps1 -Diagnostic
#
#   With one specific session (opt-in, never automatic):
#     powershell -File Make-SupportBundle.ps1 -SessionId 3f9c1a22
#
# ── What changed from v1, and why ──────────────────────────────────────────
# SUP-002. v1 read $env:LOCALAPPDATA\TechAim, one level ABOVE the real data
# root. Qt's AppLocalDataLocation is <LOCALAPPDATA>\<organisation>\<application>
# and Tech Aim sets both to "TechAim", so the root is ...\TechAim\TechAim.
# Every collection path in v1 therefore pointed at a directory that does not
# exist and the bundle shipped with no logs, no sessions and no crash data.
# Verified on the development machine: ...\TechAim\Logs absent,
# ...\TechAim\TechAim\Logs present.
#
# SUP-003. Operational logging does NOT go to the AppData Logs directory. The
# application's active logger writes %TEMP%\tachus_log<ddMMyyyy-hhmmss>.log.
# That is the file that proved SERIAL-AUTO-001, and v1 collected none of it.
# Both locations are now collected, and the report says which one had content.
# The logger's location is a RUNTIME concern and is deliberately NOT changed
# here - it is recorded in docs/deployment/Deployment-Audit.md as finding
# LOG-001 for a future runtime branch.
[CmdletBinding()]
param(
    [string]$SessionId = '',
    [string]$OutDir    = "$env:USERPROFILE\Desktop",
    [int]$LogCount     = 5,
    # Diagnostic mode widens log collection and extracts the correlated
    # shot-pipeline stamps that RC2a emits when developer_mode=1.
    [switch]$Diagnostic,
    # Historical logs are EXCLUDED by default. A support bundle is about what
    # just happened; sweeping months of prior sessions off an operator's machine
    # is neither useful nor proportionate. Raise it deliberately if asked to.
    [int]$MaxLogAgeDays = 7,
    [string]$ReleaseManifest = ''
)
$ErrorActionPreference = 'Stop'

$stamp = Get-Date -Format 'yyyy-MM-dd-HHmmss'
$name  = if ($Diagnostic) { "TechAim-Diagnostic-$stamp" } else { "TechAim-Support-$stamp" }
$work  = Join-Path $env:TEMP $name
$zip   = Join-Path $OutDir "$name.zip"
if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force $work | Out-Null

# SUP-002: the REAL root is <LOCALAPPDATA>\TechAim\TechAim.
$appData = Join-Path $env:LOCALAPPDATA 'TechAim\TechAim'
$here    = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($Diagnostic) { $LogCount = [Math]::Max($LogCount, 20) }

Write-Host "== Tech Aim support bundle$(if ($Diagnostic) { ' - DIAGNOSTIC' }) =="

# ---- release manifest -----------------------------------------------------
# The Git commit is baked into the binary at build time and cannot be scraped
# reliably from outside, so it is read from the shipped manifest and then
# CHECKED against the executable hash rather than taken on trust.
$manifest = $null; $manifestPath = ''
$candidates = @()
if ($ReleaseManifest) { $candidates += $ReleaseManifest }
foreach ($dir in @($here, (Split-Path $here -Parent))) {
    if (-not $dir) { continue }
    $candidates += (Join-Path $dir 'release-manifest.json')
    foreach ($sub in @('manifests', 'manifest', 'documents', 'docs')) {
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
    Write-Warning "No release manifest found. The bundle reports the Git commit as UNKNOWN rather than guessing."
}

$exe    = Join-Path $here 'TechAim.exe'
$ver    = if (Test-Path $exe) { (Get-Item $exe).VersionInfo.FileVersion } else { 'not found' }
$exeSha = if (Test-Path $exe) { (Get-FileHash $exe -Algorithm SHA256).Hash } else { '' }

$mProduct   = if ($manifest) { $manifest.product }          else { 'Tech Aim Electronic Target Control' }
$mVersion   = if ($manifest) { $manifest.version }          else { 'UNKNOWN - no manifest' }
$mChannel   = if ($manifest) { $manifest.releaseChannel }   else { 'UNKNOWN - no manifest' }
$mCommit    = if ($manifest) { $manifest.gitCommit }        else { 'UNKNOWN - no manifest' }
$mLimit     = if ($manifest) { $manifest.limitation }       else { 'FIELD TEST - NOT FOR OFFICIAL COMPETITION RESULTS' }
$mAnalytics = if ($manifest) { $manifest.analyticsVersion } else { 'UNKNOWN - no manifest' }
$mExeSha    = if ($manifest) { $manifest.executableSha256 } else { '' }
$shaVerdict =
    if (-not $exeSha)             { 'NOT CHECKED - TechAim.exe not found beside this script' }
    elseif (-not $mExeSha)        { 'NOT CHECKED - no manifest hash to compare against' }
    elseif ($exeSha -eq $mExeSha) { 'MATCH - the running binary is the one the manifest describes' }
    else { 'MISMATCH - the binary beside this script is NOT the manifest build; the Git commit above may be wrong' }

$modeLine = 'UNKNOWN'; $devMode = 'UNKNOWN'
$cfgProbe = Join-Path $here 'config.ini'
if (Test-Path $cfgProbe) {
    $cfgBody = Get-Content $cfgProbe -Raw
    $mm = [regex]::Match($cfgBody, '(?m)^\s*app_mode\s*=\s*(.+?)\s*$')
    if ($mm.Success) { $modeLine = $mm.Groups[1].Value }
    $dm = [regex]::Match($cfgBody, '(?m)^\s*developer_mode\s*=\s*(\d)\s*$')
    if ($dm.Success) { $devMode = $dm.Groups[1].Value }
}

# ---- logs, from BOTH locations (SUP-003) ---------------------------------
$logOut = Join-Path $work 'Logs'
New-Item -ItemType Directory -Force $logOut | Out-Null

$cutoff = (Get-Date).AddDays(-$MaxLogAgeDays)

$appDataLogDir = Join-Path $appData 'Logs'
$appDataLogs = @()
if (Test-Path $appDataLogDir) {
    $appDataLogs = @(Get-ChildItem $appDataLogDir -File -EA SilentlyContinue |
                     Where-Object { $_.LastWriteTime -ge $cutoff } |
                     Sort-Object LastWriteTime -Descending | Select-Object -First $LogCount)
    foreach ($f in $appDataLogs) { Copy-Item $f.FullName (Join-Path $logOut ("appdata-" + $f.Name)) }
}
# The active logger. This is where the evidence actually is.
$tempAll  = @(Get-ChildItem $env:TEMP -Filter 'tachus_log*.log' -File -EA SilentlyContinue)
$tempLogs = @($tempAll | Where-Object { $_.LastWriteTime -ge $cutoff } |
              Sort-Object LastWriteTime -Descending | Select-Object -First $LogCount)
foreach ($f in $tempLogs) { Copy-Item $f.FullName (Join-Path $logOut $f.Name) }
$excludedOld = @($tempAll | Where-Object { $_.LastWriteTime -lt $cutoff }).Count

$logReport = @"
Log collection
--------------
AppData log directory : $appDataLogDir
  exists              : $(Test-Path $appDataLogDir)
  files collected     : $($appDataLogs.Count)

Active logger location: $env:TEMP\tachus_log*.log
  files present       : $($tempAll.Count)
  files collected     : $($tempLogs.Count)
  newest              : $(if ($tempLogs.Count) { $tempLogs[0].Name + '  ' + $tempLogs[0].LastWriteTime } else { '(none)' })

Age limit             : $MaxLogAgeDays days (cutoff $($cutoff.ToString('yyyy-MM-dd HH:mm')))
  older logs EXCLUDED : $excludedOld
Historical logs are excluded by default. Pass -MaxLogAgeDays to widen it if a
diagnosis genuinely needs older material.

The application's operational log is the tachus_log file in the Windows TEMP
directory. The AppData Logs directory is created by the storage layer but the
current logger does not write to it. Both are collected so a reader can see
which one had content. Windows Disk Cleanup and Storage Sense can delete TEMP
files, so collect a bundle SOON after a problem occurs.
"@
[System.IO.File]::WriteAllText((Join-Path $work 'log-collection.txt'), $logReport)

$allLogText = @()
foreach ($f in ($tempLogs + $appDataLogs)) { $allLogText += (Get-Content $f.FullName -EA SilentlyContinue) }

# ---- target communication summary ----------------------------------------
$commLines = @($allLogText | Select-String -Pattern 'target|modbus|COM\d|connect|disconnect|protocol|malformed|scan|candidate|remembered|bluetooth' |
                Select-Object -Last 600 | ForEach-Object { $_.Line })
$commText = if ($commLines.Count) { $commLines -join "`n" } else { 'No target communication lines found in the collected logs.' }
[System.IO.File]::WriteAllText((Join-Path $work 'target-communication.txt'), $commText)

# ---- diagnostic: correlated shot-pipeline stamps -------------------------
$traceCount = 0
if ($Diagnostic) {
    $traceLines = @($allLogText | Select-String -Pattern 'decoded|emit-shootCountChanged|emit-returned|qml-notified|qml-scored|qml-marker-added|zoom-requested|zoom-started|feed-hook-enter|feed-hook-exit' |
                    ForEach-Object { $_.Line })
    $traceCount = $traceLines.Count
    $header = if ($traceCount -eq 0) {
@"
NO SHOT-PIPELINE STAMPS FOUND.

The stamps are emitted only when developer_mode=1. This package reports
developer_mode=$devMode. If that is 0, set it to 1 in config.ini, restart the
application, repeat the shots and collect the bundle again.
"@
    } else {
@"
Correlated shot-pipeline stamps ($traceCount lines).

Boundaries, in order: decoded -> emit-shootCountChanged -> qml-notified ->
qml-scored -> qml-marker-added -> zoom-requested -> zoom-started ->
emit-returned -> feed-hook-enter -> feed-hook-exit.

All stamps share one session tag and one monotonic clock, so the differences
between them are directly comparable and locate where any delay occurs.
"@
    }
    [System.IO.File]::WriteAllText((Join-Path $work 'shot-pipeline-stamps.txt'),
                                   ($header + "`n`n" + ($traceLines -join "`n")))
}

# ---- release + machine identity ------------------------------------------
$identity = @"
Tech Aim support bundle
Bundle type          : $(if ($Diagnostic) { 'DIAGNOSTIC (RC2a physical field test)' } else { 'NORMAL DEPLOYMENT' })
Generated            : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Product              : $mProduct
Version              : $mVersion
Release channel      : $mChannel
Git commit           : $mCommit
Analytics version    : $mAnalytics
Limitation           : $mLimit
Operating mode       : $modeLine
Developer mode       : $devMode
Executable           : $exe
Executable version   : $ver
Executable SHA-256   : $(if ($exeSha) { $exeSha } else { 'n/a' })
Manifest SHA-256     : $(if ($mExeSha) { $mExeSha } else { '(none)' })
Binary vs manifest   : $shaVerdict
Manifest file        : $(if ($manifestPath) { $manifestPath } else { '(none found)' })
Windows              : $([System.Environment]::OSVersion.VersionString)
Windows product      : $((Get-CimInstance Win32_OperatingSystem).Caption)
Windows build        : $((Get-CimInstance Win32_OperatingSystem).Version)
Architecture         : $env:PROCESSOR_ARCHITECTURE
Machine              : $env:COMPUTERNAME
Data root            : $appData
Data root exists     : $(Test-Path $appData)
Logs collected       : $($appDataLogs.Count) from AppData, $($tempLogs.Count) from TEMP
Shot stamps found    : $(if ($Diagnostic) { $traceCount } else { '(normal bundle - not collected)' })
Session id requested : $(if ($SessionId) { $SessionId } else { '(none - no session data collected)' })

The Git commit comes from the shipped release manifest and is checked against
the executable SHA-256 - see "Binary vs manifest". Build date and Qt version
are shown in the application under Settings > ABOUT / BUILD.
"@
[System.IO.File]::WriteAllText((Join-Path $work 'release-identity.txt'), $identity)

# ---- sanitized configuration ---------------------------------------------
# Whole-line drop of anything that looks like a secret. A key is reported as
# present-but-removed rather than silently omitted, so a reader can tell the
# difference between "no such setting" and "withheld".
if (Test-Path $cfgProbe) {
    $clean = Get-Content $cfgProbe | ForEach-Object {
        if ($_ -match '(?i)(pass|pwd|secret|token|key|licence|license|serial)\s*=') {
            ($_ -replace '=.*', '= [REMOVED FROM SUPPORT BUNDLE]')
        } else { $_ }
    }
    $clean | Set-Content (Join-Path $work 'config.sanitized.ini') -Encoding utf8
}

# ---- storage inventory (counts only - never athlete content) -------------
$inv = @("Storage inventory (file counts only - no session content is read)","")
foreach ($d in @('Sessions\Current','Sessions\Archive','Sessions\Corrupt','Reports','Exports',
                 'Logs','Settings','Backups','SupportBundles','DerivedIndexes')) {
    $p = Join-Path $appData $d
    $n = if (Test-Path $p) { @(Get-ChildItem $p -File -Recurse -EA SilentlyContinue).Count } else { 'ABSENT' }
    $inv += ("{0,-24} {1}" -f $d, $n)
}
[System.IO.File]::WriteAllText((Join-Path $work 'storage-inventory.txt'), ($inv -join "`n"))

# ---- ONE session journal, only when explicitly asked for -----------------
if ($SessionId) {
    $sesOut = Join-Path $work 'Session'
    New-Item -ItemType Directory -Force $sesOut | Out-Null
    $hits = @(Get-ChildItem (Join-Path $appData 'Sessions') -Recurse -File -EA SilentlyContinue |
              Where-Object { $_.Name -like "*$SessionId*" })
    foreach ($h in $hits) { Copy-Item $h.FullName $sesOut }
    if (-not $hits) {
        [System.IO.File]::WriteAllText((Join-Path $sesOut 'NOT-FOUND.txt'),
            "No session file matched '$SessionId'. Nothing else was collected.")
    }
}

# ---- known limitations ----------------------------------------------------
foreach ($cand in @('docs\0.9.0-known-limitations.md',
                    'docs\0.9.0-rc2-known-limitations.md',
                    'docs\0.9.0-rc1-known-limitations.md')) {
    $p = Join-Path $here $cand
    if (Test-Path $p) { Copy-Item $p (Join-Path $work 'known-limitations.md'); break }
}
if ($manifestPath) { Copy-Item $manifestPath (Join-Path $work 'release-manifest.json') }

# ---- zip ------------------------------------------------------------------
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Force $OutDir | Out-Null }
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $work '*') -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $work -Recurse -Force

Write-Host ""
Write-Host "Support bundle : $zip"
Write-Host "SHA-256        : $((Get-FileHash $zip -Algorithm SHA256).Hash)"
Write-Host ""
Write-Host "Contents: release identity, log-collection report, collected logs, a"
Write-Host "target-communication summary, a sanitized configuration and a storage"
if ($Diagnostic) { Write-Host "inventory, plus the correlated shot-pipeline stamps." }
else { Write-Host "inventory." }
Write-Host "Session data is included ONLY when you pass -SessionId."
Write-Host "Please check the contents before sending it on."
