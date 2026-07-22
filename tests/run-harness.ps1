# Safe test-harness launcher (T1.1 orphan-process prevention).
#
# The finals10m/3P harnesses link Qt Multimedia and need the GUI platform
# plugin even as console tests; a wrapper shell that times out can orphan the
# child, which then blocks on a "no Qt platform plugin" dialog. This launcher
# runs ONE harness at a time with the correct environment, a child-level
# timeout, tree-kill on expiry, captured stdout, the real exit code, and an
# orphan check. Usage:
#   powershell -File tests\run-harness.ps1 -Harness training   # or reliability|finals10m|finals
param(
    [Parameter(Mandatory=$true)][ValidateSet('training','reliability','finals10m','finals')]
    [string]$Harness,
    [int]$TimeoutSec = 180
)
$root = Split-Path -Parent $PSScriptRoot
$exeNames = @{ training='training_tests'; reliability='reliability_tests';
               finals10m='finals10m_tests'; finals='finals_tests' }
$name = $exeNames[$Harness]
$exe  = Join-Path $root "tests\$Harness\release\$name.exe"
if (-not (Test-Path $exe)) { Write-Error "Not built: $exe"; exit 2 }

# Pre-check: no stale copy of THIS harness (never kills unrelated processes).
Get-Process $name -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Warning "Killing stale $name (pid $($_.Id))"; Stop-Process -Id $_.Id -Force
}

$env:Path = "C:\Qt\6.5.3\mingw_64\bin;" + $env:Path
$env:QT_QPA_PLATFORM = "offscreen"
$out = Join-Path $env:TEMP "$name-out.txt"

$p = Start-Process -FilePath $exe -RedirectStandardOutput $out `
        -RedirectStandardError (Join-Path $env:TEMP "$name-err.txt") `
        -PassThru -NoNewWindow
if (-not $p.WaitForExit($TimeoutSec * 1000)) {
    Write-Warning "Timeout after $TimeoutSec s - killing process tree"
    & taskkill /PID $p.Id /T /F | Out-Null
    Get-Content $out -Tail 5
    exit 3
}
Get-Content $out | Select-String -Pattern '=== \d+ checks, \d+ failures ===' | ForEach-Object { $_.Line }
# Post-check: confirm no orphan of this harness remains.
if (Get-Process $name -ErrorAction SilentlyContinue) {
    Write-Error "Orphan $name still running"; exit 4
}
exit $p.ExitCode
