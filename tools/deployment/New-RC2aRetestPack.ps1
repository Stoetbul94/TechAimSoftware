# Tech Aim — assemble the RC2a physical retest pack.
#
#   powershell -File tools\deployment\New-RC2aRetestPack.ps1
#
# RC2a is NEVER rebuilt. The accepted ZIP is verified against its accepted hash
# and then copied byte-for-byte; the copy is re-hashed to prove it is faithful.
# If the hash does not match, the run fails and nothing is assembled.
[CmdletBinding()]
param([string]$Repo = '', [string]$OutDir = '')
$ErrorActionPreference = 'Stop'

if (-not $Repo)   { $Repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot) }
if (-not $OutDir) { $OutDir = Join-Path $Repo 'dist\RC2a-Physical-Retest-Pack' }
$dist = Join-Path $Repo 'dist'

$RC2A_NAME = 'TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip'
$RC2A_SHA  = '215C8F0DC89E2E1D5F19CAD6D2B468DA6CED9ADA0735D210AB0D54EC602B165D'
$RB_NAME   = 'TechAim-Rollback-747b9a7-Windows-x64.zip'
$RB_SHA    = '3051552E78E5868271E1D8CD6DC1430193577FDF6AE4A36595FE3CCB6033C3CB'
$RC2_NAME  = 'TechAim-0.9.0-RC2-FieldTest-Windows-x64.zip'
$RC2_SHA   = 'E3D0DA2907E21E448AF158D601EFC6761015FB2956EECE366F70553EE16E063E'

Write-Host "== RC2a physical retest pack =="

# ---- 1. verify the accepted artefacts BEFORE touching anything -----------
$src = Join-Path $dist $RC2A_NAME
if (-not (Test-Path $src)) { throw "MISSING: $src" }
$h = (Get-FileHash $src -Algorithm SHA256).Hash
if ($h -ne $RC2A_SHA) {
    throw "RC2a HASH MISMATCH`n  expected $RC2A_SHA`n  found    $h`nRefusing to assemble a retest pack around an unverified package."
}
Write-Host "   RC2a verified: $h"

if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir 'checklists') | Out-Null

# ---- 2. copy RC2a byte-for-byte and re-verify the COPY -------------------
Copy-Item $src (Join-Path $OutDir $RC2A_NAME)
$copyHash = (Get-FileHash (Join-Path $OutDir $RC2A_NAME) -Algorithm SHA256).Hash
if ($copyHash -ne $RC2A_SHA) { throw "The copied RC2a ZIP does not match. Copy is not faithful." }
Write-Host "   copy verified: $copyHash"
"$RC2A_SHA *$RC2A_NAME" | Set-Content (Join-Path $OutDir "$RC2A_NAME.sha256") -Encoding utf8

# ---- 3. checklists (the version-controlled originals) --------------------
$docs = Join-Path $Repo 'docs'
foreach ($d in @('deployment\Physical-Qualification-Checklist.md',
                 'deployment\Field-Test-Notice.md',
                 'deployment\Operator-Guide.md',
                 'release\0.9.0-known-limitations.md',
                 'release\0.9.0-rc2a-diagnostic.md',
                 'release\0.9.0-rc1-rollback.md')) {
    $p = Join-Path $docs $d
    # Windows PowerShell 5.1's Join-Path takes only two path arguments.
    if (Test-Path $p) { Copy-Item $p (Join-Path (Join-Path $OutDir 'checklists') (Split-Path $d -Leaf)) }
    else { Write-Warning "not found, not packed: $d" }
}

# ---- 4. the one-page START HERE -----------------------------------------
@"
TECH AIM 0.9.0-RC2a - PHYSICAL RETEST PACK
==========================================

Three shots. One sighter, two counted. About 20 minutes.

WHAT THIS DECIDES
-----------------
Whether SERIAL-AUTO-001 (automatic COM detection) can be closed. Deployment
preparation is finished and waiting on this result.


1. VERIFY THE DOWNLOAD
----------------------
    Get-FileHash .\$RC2A_NAME -Algorithm SHA256

Must be exactly:

    $RC2A_SHA

If it differs, STOP. Do not extract it.


2. EXTRACT
----------
Extract to:      C:\TechAim\RC2a\
Do NOT overwrite C:\TechAim\RC1\  or  C:\TechAim\RC2\

Windows may warn that the file came from another computer. Expected - this
software is NOT code-signed. Continue only because the checksum matched.


3. SET DIAGNOSTIC MODE
----------------------
Edit C:\TechAim\RC2a\config.ini:

    app_mode=Live
    developer_mode=1

developer_mode=1 records the timestamps this test needs. Set it back to 0
when you have finished.

LEAVE BLUETOOTH ON. That is the condition RC2 failed under.


4. RUN THE TEST
---------------
Follow checklists\Physical-Qualification-Checklist.md, tests A to J:

    A. Target connected before startup
    B. Confirm automatic connection
    C. Confirm no manual port selection was needed
    D. Confirm no paper movement during startup
    E. Fire ONE sighter
    F. Fire ONE counted shot
    G. Disconnect and reconnect
    H. Fire ONE counted shot
    I. Restart and confirm the remembered connection
    J. Generate the support bundle

Fill in the observation sheet as you go, especially the delay in seconds and
whether the view zoomed.


5. SUPPORT BUNDLE
-----------------
From C:\TechAim\RC2a\ :

    powershell -File Make-SupportBundle.ps1 -Diagnostic

It lands on your Desktop. Check it contains shot-pipeline-stamps.txt and that
the file is NOT empty, then send it with the completed checklist.


STOP THE TEST IMMEDIATELY IF
----------------------------
  * the paper moves when no shot was fired
  * a shot appears that nobody fired
  * the software connects to a Bluetooth port
  * the interface freezes for more than about 30 seconds
  * the software crashes or closes on its own
  * the shot count is wrong

Do not keep firing to see if it settles. Three shots is the whole test.


ROLLBACK
--------
If RC2a is worse than RC2, just run C:\TechAim\RC2\TechAim.exe instead.
Nothing needs uninstalling and NO SESSION DATA IS LOST - your data lives in
%LOCALAPPDATA%\TechAim\TechAim and is untouched by switching folders.

  RC2      $RC2_NAME
           $RC2_SHA

  older    $RB_NAME
           $RB_SHA

Neither is copied into this pack, so there is exactly one of each and it
cannot drift. Both are in dist\ .


AFTERWARDS
----------
  1. Set developer_mode=0 in C:\TechAim\RC2a\config.ini
  2. Send the support bundle and the completed checklist
  3. Do NOT delete C:\TechAim\RC2a\ - the logs are still useful


WHAT A PASS MEANS
-----------------
It closes SERIAL-AUTO-001 and confirms paper feed and reconnect for ONE
discipline on ONE machine. It is NOT approval for final deployment and it
does NOT qualify any other discipline.
"@ | Set-Content (Join-Path $OutDir 'START-HERE.txt') -Encoding utf8

# ---- 5. pack manifest ----------------------------------------------------
Push-Location $Repo
$commit = (& git rev-parse HEAD).Trim()
Pop-Location
[ordered]@{
    schema         = 'techaim.retest-pack/1'
    purpose        = 'RC2a physical re-test of SERIAL-AUTO-001, paper feed, reconnect and restart'
    sourceCommit   = $commit
    totalShots     = 3
    package        = @{ name = $RC2A_NAME; sha256 = $RC2A_SHA; rebuilt = $false
                        note = 'Copied byte-for-byte from the accepted artefact and re-hashed after copying.' }
    referencedOnly = @(@{ name = $RC2_NAME; sha256 = $RC2_SHA;  role = 'rollback (recent)' },
                       @{ name = $RB_NAME;  sha256 = $RB_SHA;   role = 'rollback (747b9a7)' })
    developerMode  = '1 for this test only; return to 0 afterwards'
    testStatus     = 'PENDING - not yet performed'
    closes         = 'SERIAL-AUTO-001 (only on a pass)'
    doesNotClose   = @('clean-machine test', 'any discipline other than the one exercised',
                       'final deployment approval')
} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutDir 'pack-manifest.json') -Encoding utf8

Write-Host ""
Write-Host "Pack: $OutDir"
Get-ChildItem $OutDir -Recurse -File |
    ForEach-Object { "  {0,-58} {1,8:N0} KB" -f $_.FullName.Substring($OutDir.Length).TrimStart('\'), ($_.Length/1KB) }
Write-Host ""
Write-Host "RC2a was NOT rebuilt. Accepted hash re-verified before and after copying."
