# Tech Aim documentation capture launcher (J.1A).
#
# Starts the REAL TechAim.exe against an ISOLATED data root and an isolated
# runtime directory, then refuses to hand back a usable session unless every
# safety gate passes.
#
#   powershell -File scripts\docs\start-documentation-capture.ps1
#   powershell -File scripts\docs\start-documentation-capture.ps1 -KeepOpen
#
# Paths are resolved from the script location; nothing is hardcoded to a
# particular user or checkout.
param(
    [switch]$KeepOpen,          # leave the app running for capture work
    [int]$TimeoutSeconds = 60
)

$ErrorActionPreference = 'Stop'
$scriptDir = $PSScriptRoot
$repoRoot  = (Resolve-Path (Join-Path $scriptDir '..\..')).Path
$exeSrc    = Join-Path $repoRoot 'release\TechAim.exe'
$captureBase = Join-Path $repoRoot 'manual-preview'
$runtimeDir  = Join-Path $captureBase 'capture-runtime'
$dataRoot    = Join-Path $captureBase 'capture-data-root'
$logPath     = Join-Path $captureBase ('capture-session-' +
                (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')

$script:log = @()
function Say([string]$m, [string]$c = 'Gray') {
    $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m
    $script:log += $line
    Write-Host $line -ForegroundColor $c
}
function Die([string]$m) {
    Say "REFUSED: $m" 'Red'
    if (-not (Test-Path $captureBase)) { New-Item -ItemType Directory -Path $captureBase -Force | Out-Null }
    $script:log | Set-Content -Path $logPath -Encoding utf8
    Say "log: $logPath"
    exit 1
}

# ── 1. executable present ───────────────────────────────────────────────
if (-not (Test-Path $exeSrc)) { Die "release\TechAim.exe not found. Build first." }
Say "executable: $exeSrc"

# ── 2. embedded identity ────────────────────────────────────────────────
$vi = (Get-Item $exeSrc).VersionInfo
Say ("version resource: {0} / {1} / {2}" -f $vi.ProductName, $vi.FileVersion, $vi.CompanyName)
if ($vi.ProductName -notlike '*Tech Aim*') { Die "unexpected ProductName: $($vi.ProductName)" }
$exeSha = (Get-FileHash $exeSrc -Algorithm SHA256).Hash.Substring(0,16).ToLower()
$appCommit = (& git -C $repoRoot log -1 --format=%h -- '*.cpp' '*.h' '*.qml' '*.pro' '*.pri' '*.rc' '*.qrc' 'src/' 'ModReader/' 'translations/' 'images/').Trim()
Say "executable sha256[0:16]: $exeSha"
Say "application baseline commit: $appCommit"

# ── 3. isolated runtime (NO real .tch / athlete / report data) ──────────
if (Test-Path $runtimeDir) { Remove-Item -Recurse -Force $runtimeDir }
New-Item -ItemType Directory -Path $runtimeDir -Force | Out-Null

# Only what is needed to RUN: the executable, Qt runtime DLLs and plugin
# folders. Explicitly never .tch match records, reports or journals.
$copied = 0
Get-ChildItem (Join-Path $repoRoot 'release') -File |
  Where-Object { $_.Extension -in '.exe','.dll' -and $_.Name -ne 'Seta.exe' } |
  ForEach-Object { Copy-Item $_.FullName $runtimeDir; $copied++ }
# Copy EVERY Qt deployment subdirectory. An earlier revision listed folder
# names by hand and missed release\qml\, so QtQuick failed to load and the
# application exited during QML startup. Directories under release/ are Qt
# deployment artefacts; user data lives in files, which are filtered above.
Get-ChildItem (Join-Path $repoRoot 'release') -Directory |
  ForEach-Object { Copy-Item $_.FullName $runtimeDir -Recurse -Force }
Say "runtime prepared: $copied binaries + plugin folders -> $runtimeDir"

# Prove no real match data leaked into the runtime.
$leaked = Get-ChildItem $runtimeDir -Recurse -File -Include *.tch,*.jsonl,*.csv -ErrorAction SilentlyContinue
if ($leaked) { Die "real data leaked into the capture runtime: $($leaked[0].Name)" }
Say "verified: no .tch / .jsonl / .csv in the capture runtime"
if (Test-Path (Join-Path $runtimeDir 'Seta.exe')) { Die "stale Seta.exe present in the capture runtime" }

# ── 4/6. capture-specific config: Demo + English ────────────────────────
@"
[shot_count_and_timer]
timer=yes

[App_Settings]
app_mode=Demo
ui_language=en
is_single_decimal=1
"@ | Set-Content -Path (Join-Path $runtimeDir 'config.ini') -Encoding utf8
Say "capture config.ini written (app_mode=Demo, ui_language=en)"

# ── 5. production baseline BEFORE launch ────────────────────────────────
$prodRoot = Join-Path $env:LOCALAPPDATA 'TechAim\TechAim'
function Measure-Root([string]$p) {
    if (-not (Test-Path $p)) { return [pscustomobject]@{ Files=0; Bytes=0; Newest=$null } }
    $f = Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue
    [pscustomobject]@{
        Files  = ($f | Measure-Object).Count
        Bytes  = ($f | Measure-Object -Property Length -Sum).Sum
        Newest = ($f | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
    }
}
$before = Measure-Root $prodRoot
Say ("production root BEFORE: {0} files, {1} bytes, newest {2}" -f $before.Files, $before.Bytes, $before.Newest)

# ── 7. launch with the capture flags ────────────────────────────────────
$exe = Join-Path $runtimeDir 'TechAim.exe'
$args = @('--documentation-capture', '--data-root', $dataRoot)

# Qt writes its compiled-QML disk cache under QStandardPaths::CacheLocation,
# which sits inside the production AppData tree and is NOT routed through
# StoragePaths — so it changed the production root even though no session
# data did. Redirect it into the capture profile so the capture is genuinely
# zero-touch. The gate below stays strict rather than being relaxed to pass.
$prevQmlCache = $env:QML_DISK_CACHE_PATH
$env:QML_DISK_CACHE_PATH = Join-Path $dataRoot 'qmlcache'
New-Item -ItemType Directory -Path $env:QML_DISK_CACHE_PATH -Force | Out-Null
Say "Qt QML disk cache redirected into the capture profile"

Say "launching: TechAim.exe $($args -join ' ')"
$p = Start-Process -FilePath $exe -WorkingDirectory $runtimeDir -ArgumentList $args -PassThru
$env:QML_DISK_CACHE_PATH = $prevQmlCache
Say "PID: $($p.Id)"

# ── 9. wait for the real window ─────────────────────────────────────────
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class CapW {
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
 public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
"@ -ErrorAction SilentlyContinue

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$win = $null
while ((Get-Date) -lt $deadline -and -not $win) {
    Start-Sleep -Milliseconds 700
    if ($p.HasExited) { Die "the application exited during startup (exit $($p.ExitCode)) - check the capture flags" }
    $found = New-Object System.Collections.ArrayList
    $cb = [CapW+EnumWindowsProc]{
        param($h,$l)
        $q = 0; [CapW]::GetWindowThreadProcessId($h,[ref]$q) | Out-Null
        if ($q -eq $p.Id -and [CapW]::IsWindowVisible($h)) {
            $sb = New-Object System.Text.StringBuilder 512
            [CapW]::GetWindowText($h,$sb,512) | Out-Null
            $r = New-Object CapW+RECT; [CapW]::GetWindowRect($h,[ref]$r) | Out-Null
            if ($sb.Length -gt 0 -and ($r.R-$r.L) -gt 400) {
                [void]$found.Add([pscustomobject]@{ H=$h; T=$sb.ToString(); W=($r.R-$r.L); Ht=($r.B-$r.T) })
            }
        }
        return $true
    }
    [CapW]::EnumWindows($cb,[IntPtr]::Zero) | Out-Null
    if ($found.Count -gt 0) { $win = $found[0] }
}
if (-not $win) { Die "no application window appeared within ${TimeoutSeconds}s" }
Say ("window: '{0}' {1}x{2}" -f $win.T, $win.W, $win.Ht)

# ── 10/11. title gate ───────────────────────────────────────────────────
$expected = 'Tech Aim Electronic Target Control'
if ($win.T -ne $expected) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Die "window title is '$($win.T)', expected exactly '$expected'"
}
foreach ($bad in 'SETA','TACHUS','Seta','Seeds') {
    if ($win.T -match "(^|\W)$bad(\W|$)") {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        Die "legacy branding '$bad' present in the window title"
    }
}
Say "title gate PASSED: exactly '$expected'" 'Green'

# ── 14. session log + handback ──────────────────────────────────────────
$after = Measure-Root $prodRoot
Say ("production root AFTER launch: {0} files, {1} bytes" -f $after.Files, $after.Bytes)
if ($after.Files -ne $before.Files -or $after.Bytes -ne $before.Bytes) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Die "the production data root CHANGED during capture startup"
}
Say "production root unchanged" 'Green'
if (-not (Test-Path (Join-Path $dataRoot '.techaim-documentation-capture'))) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Die "capture profile marker missing from $dataRoot"
}
Say "capture marker present in the isolated root" 'Green'

$script:log | Set-Content -Path $logPath -Encoding utf8
Say "capture session log: $logPath"

if ($KeepOpen) {
    Say "application left running (PID $($p.Id)) for capture work." 'Yellow'
    Write-Output $p.Id
} else {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    Say "application closed"
}
exit 0
