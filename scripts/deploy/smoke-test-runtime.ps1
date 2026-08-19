<#
.SYNOPSIS
    Launch a deployed SETA runtime the way a customer machine would.

.DESCRIPTION
    check_deployment.py proves the folder is self-contained by reading import
    tables. This proves it by running it:

      * the folder is copied OUT of the repository first, so the artefact stays
        byte-identical and anything the first run writes is visible;
      * PATH is reduced to Windows itself - no Qt, no MinGW, no developer
        directory - which is the condition that produced
        "Qt6Core.dll was not found" on the range laptop;
      * QT_PLUGIN_PATH and the QML import paths are cleared, so Qt cannot fall
        back to the development installation for a missing plugin;
      * the window title is read back, because a process that is alive is not
        the same thing as an application that started;
      * the copy is counted before and after, because a runtime that writes
        into its own program directory will fail under Program Files.

    WHAT THIS DOES NOT PROVE: it runs on the build machine, so the OS already
    has whatever Windows-level runtime a fresh installation might lack, and the
    per-user data directory is this developer's. It is a dependency test, not a
    clean-machine test. Clean-machine validation needs a second machine.

.PARAMETER RuntimeDir
    Deployment folder to test. Default: <repo>\dist\seta-runtime
.PARAMETER Seconds
    How long to let the application settle before reading it. Default 15.
.PARAMETER EvidenceDir
    Where the window capture is written. Default: <repo>\dist\smoke-evidence
.PARAMETER Language
    Seed config.ini with this UI language before launching (e.g. 'de'). The
    catalogues are compiled into the executable, so this exercises the DEPLOYED
    binary's translation resources.
.PARAMETER DemoMode
    Seed config.ini with app_mode=Demo, so the deployed build is checked to
    accept simulated shots without a physical target attached.
#>
[CmdletBinding()]
param(
    [string]$RuntimeDir,
    [int]$Seconds = 15,
    [string]$EvidenceDir,
    [string]$Language,
    [switch]$DemoMode
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $RuntimeDir)  { $RuntimeDir  = Join-Path $repo 'dist\seta-runtime' }
if (-not $EvidenceDir) { $EvidenceDir = Join-Path $repo 'dist\smoke-evidence' }

$script:checks = 0
$script:failures = 0
function Check($ok, $label, $detail = '') {
    $script:checks++
    if ($ok) { Write-Host "PASS  $label" }
    else { $script:failures++; Write-Host "FAIL  $label  $detail" -ForegroundColor Red }
}
function Say($m) { Write-Host "[smoke] $m" }

if (-not (Test-Path (Join-Path $RuntimeDir 'TechAim.exe'))) {
    Write-Host "[smoke] ERROR: no runtime at $RuntimeDir" -ForegroundColor Red; exit 1
}

# ── 1. clean room, outside the repository ─────────────────────────────────
$room = Join-Path $env:TEMP ('seta-smoke-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $room -Force | Out-Null
Copy-Item (Join-Path $RuntimeDir '*') $room -Recurse -Force

# Optional operator configuration. Written HERE and never into the artefact:
# a shipped folder must carry no configuration, and the deployment gate fails
# if it does.
$seeded = @()
if ($Language -or $DemoMode) {
    $ini = "[App_Settings]"
    if ($DemoMode) { $ini += "`r`napp_mode=Demo"; $seeded += 'app_mode=Demo' }
    if ($Language) { $ini += "`r`nui_language=$Language"; $seeded += "ui_language=$Language" }
    [System.IO.File]::WriteAllText((Join-Path $room 'config.ini'), $ini + "`r`n")
    Say "seeded config.ini: $($seeded -join ', ')"
}

$before = @(Get-ChildItem $room -Recurse -File)
Say "clean room: $room ($($before.Count) files)"

# ── 2. a PATH with no development environment on it ───────────────────────
$env:PATH = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\Wbem"
$env:QT_PLUGIN_PATH = ''
$env:QML2_IMPORT_PATH = ''
$env:QML_IMPORT_PATH = ''
$env:QT_FORCE_STDERR_LOGGING = '1'
$qtOnPath = Get-Command qmake -ErrorAction SilentlyContinue
Check ($null -eq $qtOnPath) 'the test PATH has no Qt on it - this is the customer condition'

$errLog = Join-Path $room '..\seta-smoke-stderr.txt'
$outLog = Join-Path $room '..\seta-smoke-stdout.txt'
$proc = Start-Process -FilePath (Join-Path $room 'TechAim.exe') -WorkingDirectory $room `
        -PassThru -RedirectStandardError $errLog -RedirectStandardOutput $outLog
Start-Sleep -Seconds $Seconds

# ── 3. did it actually start ──────────────────────────────────────────────
Check (-not $proc.HasExited) 'the application is still running with no Qt on PATH' `
      "exit code $($proc.ExitCode)"
$title = ''
$live = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
if ($live) { $title = $live.MainWindowTitle }
Check ($title -like '*SETA*') 'a SETA main window exists' "title '$title'"

$stderr = if (Test-Path $errLog) { Get-Content $errLog -Raw } else { '' }
Check ($stderr -notmatch 'could not find or load the Qt platform plugin') `
      'Qt found its platform plugin inside the folder'
Check ($stderr -notmatch 'module "[^"]+" is not installed') `
      'no QML module is missing'
Check ($stderr -match 'brand SETA') 'the running build reports itself as SETA'
if ($Language) {
    Check ($stderr -match "UI language: $Language") `
          "the deployed binary loaded its '$Language' translation catalogue"
    Check ($stderr -notmatch 'could not be loaded') `
          'and no catalogue was missing from the deployed resources'
}
if ($DemoMode) {
    Check ($stderr -match 'Operating mode: Demo') `
          'the deployed build accepts the Demo input source'
}

# ── 4. window capture, as visual evidence ─────────────────────────────────
if ($live -and $live.MainWindowHandle -ne 0) {
    Add-Type -AssemblyName System.Drawing
    if (-not ('SmokeWin' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class SmokeWin {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public struct RECT { public int L, T, R, B; }
}
'@
    }
    # PrintWindow, not a screen grab: the window is captured from its own
    # surface, so nothing in front of it can end up in the evidence.
    $r = New-Object SmokeWin+RECT
    [SmokeWin]::GetWindowRect($live.MainWindowHandle, [ref]$r) | Out-Null
    $w = $r.R - $r.L; $h = $r.B - $r.T
    if ($w -gt 0 -and $h -gt 0) {
        New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
        $bmp = New-Object System.Drawing.Bitmap $w, $h
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [SmokeWin]::PrintWindow($live.MainWindowHandle, $hdc, 2) | Out-Null
        $g.ReleaseHdc($hdc)
        $tag = 'first-launch'
        if ($DemoMode) { $tag += '-demo' }
        if ($Language) { $tag += "-$Language" }
        $shot = Join-Path $EvidenceDir "seta-runtime-$tag.png"
        $bmp.Save($shot, [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $bmp.Dispose()
        Say "window capture: $shot"
    }
}

# ── 5. the program directory is not a scratch directory ───────────────────
$after = @(Get-ChildItem $room -Recurse -File)
$new = @($after | Where-Object { $before.FullName -notcontains $_.FullName })
Check ($new.Count -eq 0) `
      'the first run wrote nothing into the program folder - it can live under Program Files' `
      (($new | ForEach-Object { $_.Name }) -join ', ')

if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
Start-Sleep -Milliseconds 500
Remove-Item $room -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ''
Write-Host ("=== {0} checks, {1} failures ===" -f $script:checks, $script:failures)
if ($script:failures -gt 0) { exit 1 }
