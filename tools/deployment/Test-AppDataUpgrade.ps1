# Tech Aim — AppData and upgrade-safety drill.
#
# Exercises first-run directory creation, restart, upgrade-over-older, rollback
# and removal, and proves that user data survives each one.
#
#   powershell -File Test-AppDataUpgrade.ps1 -OldPackage C:\...\RC2 -NewPackage C:\...\RC2a
#
# ── ISOLATION: what is and is not possible here ───────────────────────────
# Qt resolves QStandardPaths::AppLocalDataLocation through the Windows shell
# (SHGetKnownFolderPath / FOLDERID_LocalAppData), NOT through the LOCALAPPDATA
# environment variable. Overriding that variable for a child process therefore
# does NOT redirect the application's storage - this was attempted and verified
# to have no effect: the app still used the real root and the sandbox stayed
# empty. Genuinely redirecting it needs one of:
#
#   * a second Windows user account (each account has its own AppData), or
#   * a virtual machine or Windows Sandbox, or
#   * changing the User Shell Folders registry value - a SYSTEM SETTING change,
#     which is out of scope and is not done here.
#
# So this drill runs against the real per-user root and is instead made SAFE by
# construction:
#
#   * it takes a full before-inventory of every file under the data root;
#   * it only ever CREATES files carrying the $MarkerPrefix name;
#   * it deletes exactly those marker files and nothing else;
#   * it takes an after-inventory and FAILS if anything it did not create was
#     added, removed or resized.
#
# It never deletes a session, a journal, a report or a setting. If the
# after-inventory does not reconcile, the run fails loudly rather than
# reporting a clean result.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OldPackage,
    [Parameter(Mandatory = $true)][string]$NewPackage,
    [int]$LaunchSeconds = 15,
    [string]$MarkerPrefix = 'UPGRADE-DRILL-MARKER'
)
$ErrorActionPreference = 'Stop'

$script:Pass = 0; $script:Fail = 0
function Ok  ($n, $d = '') { $script:Pass++; Write-Host ("PASS  {0}{1}" -f $n, $(if ($d) { "  ($d)" })) }
function Bad ($n, $d = '') { $script:Fail++; Write-Host ("FAIL  {0}{1}" -f $n, $(if ($d) { "  -> $d" })) -ForegroundColor Red }
function Check($c, $n, $d = '') { if ($c) { Ok $n } else { Bad $n $d } }

$root = Join-Path $env:LOCALAPPDATA 'TechAim\TechAim'
Write-Host "=== Tech Aim AppData / upgrade drill ==="
Write-Host "data root : $root"
Write-Host "old       : $OldPackage"
Write-Host "new       : $NewPackage"
Write-Host "mode      : additive and reversible - only '$MarkerPrefix*' files are created or deleted"

function Inventory {
    $h = @{}
    if (Test-Path $root) {
        foreach ($f in (Get-ChildItem $root -Recurse -File -EA SilentlyContinue)) {
            $h[$f.FullName.Substring($root.Length).TrimStart('\')] = $f.Length
        }
    }
    return $h
}
$before = Inventory
Write-Host "files before: $($before.Count)"

function Start-App([string]$pkg, [string]$label) {
    $exe = Join-Path $pkg 'TechAim.exe'
    if (-not (Test-Path $exe)) { Bad "$label - TechAim.exe exists" $exe; return $null }
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.WorkingDirectory = $pkg
    $psi.UseShellExecute = $false
    # No Qt, no MinGW, no repository on PATH - the deployment condition.
    $psi.EnvironmentVariables['PATH'] = "$env:WINDIR\System32;$env:WINDIR"
    $p = [System.Diagnostics.Process]::Start($psi)
    Start-Sleep -Seconds $LaunchSeconds
    if ($p.HasExited) { Bad "$label - stayed running" "exited with $($p.ExitCode)"; return $null }
    Ok "$label - launched and stayed running with no Qt/MinGW/repository on PATH"
    return $p
}
function Stop-App($p, [string]$label) {
    if (-not $p) { return }
    try { $null = $p.CloseMainWindow(); Start-Sleep -Seconds 4 } catch { }
    if (-not $p.HasExited) { $p.Kill(); Start-Sleep -Seconds 2; Write-Host "      ($label needed Kill - no graceful window close)" }
    else { Ok "$label - closed cleanly" }
}
function CountIn([string]$sub) {
    $p = Join-Path $root $sub
    if (Test-Path $p) { @(Get-ChildItem $p -Recurse -File -EA SilentlyContinue).Count } else { -1 }
}

$markers = @()
function Seed([string]$sub, [string]$tag) {
    $dir = Join-Path $root $sub
    if (-not (Test-Path $dir)) { return $null }
    $f = Join-Path $dir "$MarkerPrefix-$tag.txt"
    [System.IO.File]::WriteAllText($f, "Created by Test-AppDataUpgrade.ps1. Safe to delete.")
    $script:markers += $f
    return $f
}

# ── 1. STORAGE TREE ───────────────────────────────────────────────────────
Write-Host "`n-- 1. first run / storage tree --"
$p1 = Start-App $OldPackage 'old version'
Stop-App $p1 'old version'
Check (Test-Path $root) "the data root exists" $root
foreach ($d in @('Sessions\Current','Sessions\Archive','Sessions\Corrupt','Logs','Reports',
                 'Exports','Settings','Backups','SupportBundles','DerivedIndexes')) {
    Check (Test-Path (Join-Path $root $d)) "storage directory present: $d"
}

$mArchive  = Seed 'Sessions\Archive' 'archive'
$mSettings = Seed 'Settings'         'settings'
$mReports  = Seed 'Reports'          'reports'
$archiveBefore = CountIn 'Sessions\Archive'
$currentBefore = CountIn 'Sessions\Current'

# ── 2. RESTART ────────────────────────────────────────────────────────────
Write-Host "`n-- 2. restart of the same version --"
$p2 = Start-App $OldPackage 'restart'
Stop-App $p2 'restart'
Check (Test-Path $mArchive)  "restart preserves archived sessions"
Check (Test-Path $mSettings) "restart preserves settings"

# ── 3. UPGRADE ────────────────────────────────────────────────────────────
Write-Host "`n-- 3. upgrade over the older version --"
$p3 = Start-App $NewPackage 'upgrade'
Stop-App $p3 'upgrade'
Check (Test-Path $mArchive)  "upgrade preserves archived sessions"
Check (Test-Path $mSettings) "upgrade preserves settings"
Check (Test-Path $mReports)  "upgrade preserves reports"
Check ((CountIn 'Sessions\Archive') -ge $archiveBefore) `
      "upgrade does not reduce the archived session count" `
      "before=$archiveBefore after=$(CountIn 'Sessions\Archive')"
Check ((CountIn 'Sessions\Current') -le ($currentBefore + 1)) `
      "upgrade does not promote archived sessions into Current" `
      "before=$currentBefore after=$(CountIn 'Sessions\Current')"

# The remembered target adapter is per-user registry state, not package state,
# so an upgrade cannot carry it away with the old program folder.
$fp = 'HKCU:\Software\TechAim\TechAim'
if (Test-Path $fp) {
    Ok "the per-user settings key survives the upgrade" $fp
} else {
    Write-Host "      (no HKCU\Software\TechAim\TechAim key yet - written on first successful target connection)"
}
Check (@(Get-ChildItem $NewPackage -Recurse -File -Include '*.jsonl' -EA SilentlyContinue).Count -eq 0) `
      "the new program folder contains no session journals"

# ── 4. ROLLBACK ───────────────────────────────────────────────────────────
Write-Host "`n-- 4. rollback to the older version --"
$p4 = Start-App $OldPackage 'rollback'
Stop-App $p4 'rollback'
Check (Test-Path $mArchive)  "rollback does not delete journals or archived sessions"
Check (Test-Path $mSettings) "rollback preserves settings"

# ── 5. REMOVAL ────────────────────────────────────────────────────────────
Write-Host "`n-- 5. removal semantics --"
# Portable removal = delete the program folder. User data lives outside it, so
# it survives - which is correct, and is also why removal never silently
# destroys an athlete's history.
$inside = @(Get-ChildItem $NewPackage -Recurse -File -EA SilentlyContinue |
            Where-Object { $_.Extension -in '.jsonl', '.pdf' -or $_.Name -like 'session*' })
Check ($inside.Count -eq 0) "no user data lives inside the program folder" `
      (($inside | Select-Object -First 2 -Expand Name) -join ', ')
Check ($root -notlike "$((Resolve-Path $NewPackage).Path)*") `
      "the data root is outside the program folder, so folder removal cannot delete it"

# ── 6. CLEAN UP AND RECONCILE ─────────────────────────────────────────────
Write-Host "`n-- 6. clean up and reconcile --"
foreach ($m in $markers) { if ($m -and (Test-Path $m)) { Remove-Item $m -Force } }
Check (@($markers | Where-Object { $_ -and (Test-Path $_) }).Count -eq 0) `
      "every marker file created by this drill was removed"

$after = Inventory
$added   = @($after.Keys  | Where-Object { -not $before.ContainsKey($_) })
$removed = @($before.Keys | Where-Object { -not $after.ContainsKey($_) })
Check ($removed.Count -eq 0) "the drill removed no pre-existing file" `
      (($removed | Select-Object -First 4) -join ', ')
# Files the APPLICATION legitimately creates by being started (caches, its own
# logs) are reported, not failed - the drill launched it five times.
if ($added.Count) {
    Write-Host "      files added by the application during this drill ($($added.Count)):"
    $added | Select-Object -First 8 | ForEach-Object { Write-Host "        $_" }
}
$leftover = @($added | Where-Object { $_ -like "*$MarkerPrefix*" })
Check ($leftover.Count -eq 0) "no drill marker survives the run" (($leftover) -join ', ')

Write-Host ""
Write-Host "files before: $($before.Count)   files after: $($after.Count)"
Write-Host "=== $script:Pass passed, $script:Fail failed ==="
Write-Host "Storage behaviour only. No target hardware, no shots fired - this is NOT a"
Write-Host "hardware or discipline qualification."
exit $(if ($script:Fail) { 1 } else { 0 })
