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
    [int]$LogCount     = 5
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

# ---- release + machine identity ------------------------------------------
$exe = Join-Path $here 'TechAim.exe'
$ver = if (Test-Path $exe) { (Get-Item $exe).VersionInfo.FileVersion } else { 'not found' }
$identity = @"
Tech Aim support bundle
Generated            : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Product              : Tech Aim Electronic Target Control
Version              : 0.9.0-RC1
Channel              : Internal Field Test
Limitation           : FIELD TEST - NOT FOR OFFICIAL COMPETITION RESULTS
Executable           : $exe
Executable version   : $ver
Executable SHA-256   : $(if (Test-Path $exe) { (Get-FileHash $exe -Algorithm SHA256).Hash } else { 'n/a' })
Windows              : $([System.Environment]::OSVersion.VersionString)
Windows product      : $((Get-CimInstance Win32_OperatingSystem).Caption)
Architecture         : $env:PROCESSOR_ARCHITECTURE
Machine              : $env:COMPUTERNAME
AppData root         : $appData
Session id requested : $(if ($SessionId) { $SessionId } else { '(none - no session data collected)' })

The Git commit, build date, Qt version, operating mode and analytics version
are shown in the application under Settings > ABOUT / BUILD. Please copy them
into the field-test checklist alongside this bundle.
"@
[System.IO.File]::WriteAllText((Join-Path $work 'release-identity.txt'), $identity)

# ---- logs ----------------------------------------------------------------
$logSrc = Join-Path $appData 'Logs'
if (Test-Path $logSrc) {
    $logOut = Join-Path $work 'Logs'
    New-Item -ItemType Directory -Force $logOut | Out-Null
    Get-ChildItem $logSrc -File | Sort-Object LastWriteTime -Descending |
        Select-Object -First $LogCount | ForEach-Object { Copy-Item $_.FullName $logOut }
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
