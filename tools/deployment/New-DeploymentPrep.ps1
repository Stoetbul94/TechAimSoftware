# Tech Aim — build the deployment-preparation output structure.
#
# Assembles dist\deployment-prep\ from artefacts that already exist. It does
# NOT compile, does NOT re-zip an accepted package, and does NOT modify any
# accepted RC. Accepted ZIPs are REFERENCED by name and hash in the manifest;
# only the RC2a package is copied, and it is copied byte-for-byte and then
# re-hashed to prove the copy is faithful.
#
#   powershell -File tools\deployment\New-DeploymentPrep.ps1
#
# Output:
#   dist\deployment-prep\
#       portable\             the RC2a portable folder (verified copy)
#       installer-candidate\  framework-neutral install manifest + README
#       checksums\            SHA256SUMS + per-file .sha256
#       manifests\            release-manifest.json
#       documentation\        operator + deployment documents
#       rollback\             rollback instructions + the rollback package hash
#       support-tools\        verification, support-bundle and drill scripts
[CmdletBinding()]
param(
    [string]$Repo = '',
    [string]$OutRoot = ''
)
$ErrorActionPreference = 'Stop'

if (-not $Repo)    { $Repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot) }
if (-not $OutRoot) { $OutRoot = Join-Path $Repo 'dist\deployment-prep' }
$dist = Join-Path $Repo 'dist'

# ── the accepted artefacts, by name and by ACCEPTED hash ──────────────────
# These values are the authority. If a file on disk does not match, the run
# fails: a deployment package assembled from an unverified artefact is worse
# than no package at all.
$accepted = [ordered]@{
    'TechAim-0.9.0-RC1-FieldTest-Windows-x64.zip'    = '1272F48D2233D3E1E96198D69548D319C68162A216137BB7C3C3D46B1A74145E'
    'TechAim-0.9.0-RC2-FieldTest-Windows-x64.zip'    = 'E3D0DA2907E21E448AF158D601EFC6761015FB2956EECE366F70553EE16E063E'
    'TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip'  = '215C8F0DC89E2E1D5F19CAD6D2B468DA6CED9ADA0735D210AB0D54EC602B165D'
    'TechAim-Rollback-747b9a7-Windows-x64.zip'       = '3051552E78E5868271E1D8CD6DC1430193577FDF6AE4A36595FE3CCB6033C3CB'
}

Write-Host "== Tech Aim deployment preparation =="
Write-Host "   repo   : $Repo"
Write-Host "   output : $OutRoot"

Write-Host "`n-- verifying accepted packages (never rebuilt, never renamed) --"
$verified = [ordered]@{}
foreach ($n in $accepted.Keys) {
    $p = Join-Path $dist $n
    if (-not (Test-Path $p)) { throw "MISSING accepted package: $p" }
    $h = (Get-FileHash $p -Algorithm SHA256).Hash
    if ($h -ne $accepted[$n]) {
        throw "HASH MISMATCH for $n`n  expected $($accepted[$n])`n  found    $h`nRefusing to assemble a deployment package from an unverified artefact."
    }
    $verified[$n] = $h
    Write-Host ("   OK  {0}" -f $n)
}

if (Test-Path $OutRoot) { Remove-Item $OutRoot -Recurse -Force }
foreach ($d in @('portable','installer-candidate','checksums','manifests',
                 'documentation','rollback','support-tools')) {
    New-Item -ItemType Directory -Force (Join-Path $OutRoot $d) | Out-Null
}

# ── portable: a verified copy of the RC2a folder ──────────────────────────
Write-Host "`n-- portable --"
$srcPortable = Join-Path $dist 'TechAim-0.9.0-RC2a-Diagnostic-Windows-x64'
if (-not (Test-Path $srcPortable)) { throw "MISSING extracted RC2a folder: $srcPortable" }
$dstPortable = Join-Path $OutRoot 'portable\TechAim-0.9.0-RC2a'
Copy-Item $srcPortable $dstPortable -Recurse
$exeSrc = (Get-FileHash (Join-Path $srcPortable 'TechAim.exe') -Algorithm SHA256).Hash
$exeDst = (Get-FileHash (Join-Path $dstPortable 'TechAim.exe') -Algorithm SHA256).Hash
if ($exeSrc -ne $exeDst) { throw "The copied executable does not match the source. Copy is not faithful." }
Write-Host "   TechAim.exe SHA-256 verified after copy: $exeDst"

# ── documentation ─────────────────────────────────────────────────────────
Write-Host "`n-- documentation --"
$docOut = Join-Path $OutRoot 'documentation'
$docSets = @(
    @{ From = 'docs\deployment'; Filter = '*.md' },
    @{ From = 'docs\release';    Filter = '0.9.0-*.md' }
)
foreach ($s in $docSets) {
    $from = Join-Path $Repo $s.From
    if (Test-Path $from) {
        Get-ChildItem $from -Filter $s.Filter -File | ForEach-Object {
            Copy-Item $_.FullName (Join-Path $docOut $_.Name) -Force
        }
    }
}
Write-Host "   documents: $(@(Get-ChildItem $docOut -File).Count)"

# ── support tools ─────────────────────────────────────────────────────────
Write-Host "`n-- support tools --"
$toolOut = Join-Path $OutRoot 'support-tools'
foreach ($t in @('Verify-Deployment.ps1','Make-SupportBundle.ps1','Test-AppDataUpgrade.ps1')) {
    Copy-Item (Join-Path $PSScriptRoot $t) $toolOut -Force
}
# The support tool must also sit BESIDE the executable: it reads the manifest,
# config.ini and TechAim.exe from its own directory to report release identity.
Copy-Item (Join-Path $PSScriptRoot 'Make-SupportBundle.ps1') $dstPortable -Force

# ── rollback ──────────────────────────────────────────────────────────────
$rbOut = Join-Path $OutRoot 'rollback'
$rbName = 'TechAim-Rollback-747b9a7-Windows-x64.zip'
@"
Rollback package
================

File    : $rbName
SHA-256 : $($verified[$rbName])
Commit  : 747b9a7 (the last build before the RC serial and paper-feed work)

The package is NOT copied here. It stays in dist\ so there is exactly one copy
and it cannot drift. Verify it before use:

    Get-FileHash "<path>\$rbName" -Algorithm SHA256

The full procedure is in documentation\0.9.0-rc1-rollback.md.
Rolling back does NOT delete sessions, journals or reports - user data lives in
%LOCALAPPDATA%\TechAim\TechAim and is untouched by swapping program folders.
"@ | Set-Content (Join-Path $rbOut 'ROLLBACK.txt') -Encoding utf8

# ── installer candidate (framework-neutral) ───────────────────────────────
$icOut = Join-Path $OutRoot 'installer-candidate'
$installManifest = [ordered]@{
    schema             = 'techaim.install-manifest/1'
    note               = 'FRAMEWORK-NEUTRAL. Describes WHAT an installer must do. No installer is built, and none is chosen.'
    productName        = 'Tech Aim Electronic Target Control'
    publisher          = 'Tech Aim'
    publisherSigned    = $false
    codeSigningStatus  = 'NOT CONFIGURED - no certificate, no signtool step. Any installer produced now would be UNSIGNED.'
    architecture       = 'x64'
    defaultInstallPath = '%LOCALAPPDATA%\Programs\TechAim'
    perMachineInstall  = $false
    requiresElevation  = $false
    elevationRationale = 'Per-user install into LOCALAPPDATA needs no administrator rights. A per-machine install into Program Files would, and is not proposed.'
    startMenuShortcut  = 'Tech Aim'
    desktopShortcut    = $false
    userDataPath       = '%LOCALAPPDATA%\TechAim\TechAim'
    userDataOnUninstall = 'PRESERVE - never delete sessions, journals or reports'
    userDataOnUpgrade   = 'PRESERVE - the data path does not change between versions'
    registryScope      = 'HKCU\Software\TechAim\TechAim'
    fileAssociations   = @()
    services           = @()
    firewallRules      = @()
    autoUpdate         = 'NONE'
}
$installManifest | ConvertTo-Json -Depth 6 |
    Set-Content (Join-Path $icOut 'install-manifest.json') -Encoding utf8
@"
Installer candidate
===================

NO INSTALLER IS BUILT HERE, and no installer framework has been chosen.

install-manifest.json states what an installer would have to do, in a form that
does not commit to WiX, NSIS, Inno Setup or MSIX. Options and the trade-offs
are compared in documentation\Installer-Options.md.

The supported deployment method today is the portable ZIP.

Any installer produced from this manifest as it stands would be UNSIGNED. It
must not be described as code-signed, and no claim may be made about Windows
SmartScreen reputation.
"@ | Set-Content (Join-Path $icOut 'README.txt') -Encoding utf8

# ── manifests ─────────────────────────────────────────────────────────────
Write-Host "`n-- manifest --"
Push-Location $Repo
$commit  = (& git rev-parse HEAD).Trim()
$branch  = (& git rev-parse --abbrev-ref HEAD).Trim()
$treeDirty = [bool]((& git status --porcelain) | Where-Object { $_ })
Pop-Location

$cfg = Get-Content (Join-Path $dstPortable 'config.ini') -Raw
$devMode = ([regex]::Match($cfg, '(?m)^\s*developer_mode\s*=\s*(\d)')).Groups[1].Value
$appMode = ([regex]::Match($cfg, '(?m)^\s*app_mode\s*=\s*(\w+)')).Groups[1].Value

$manifest = [ordered]@{
    schema             = 'techaim.release-manifest/1'
    product            = 'Tech Aim Electronic Target Control'
    publisher          = 'Tech Aim'
    releaseFamily      = '0.9.0'
    version            = '0.9.0-RC2a'
    releaseChannel     = 'Internal Field Test - Diagnostic'
    buildType          = 'Release (MinGW, qmake)'
    architecture       = 'x64'
    qtVersion          = '6.5.3 (MinGW 11.2.0)'
    gitCommit          = $commit
    gitBranch          = $branch
    workingTreeDirty   = $treeDirty
    operatingMode      = $appMode
    developerMode      = $devMode
    developerModeExpectation = @{
        'rc2a-diagnostic-field-test' = '1 - set by the operator for the physical retest only'
        'rc3-and-later-deployment'   = '0 - REQUIRED'
        'packagedDefault'            = '0'
    }
    analyticsVersion   = 'windmap-analytics-v2'
    executableSha256   = $exeDst
    packages           = $verified
    packageSha256      = $verified['TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip']
    rollbackPackage    = @{ name = $rbName; sha256 = $verified[$rbName]; commit = '747b9a7' }
    supportToolVersion = 'Make-SupportBundle v2 (SUP-002 / SUP-003 corrected)'
    codeSigning        = 'NOT CONFIGURED - binaries and any future installer are UNSIGNED'
    crashDumps         = 'NOT CAPTURED - no MiniDump or crash handler is installed'
    installer          = 'NONE - portable ZIP is the supported deployment method'
    limitation         = 'FIELD TEST - NOT FOR OFFICIAL COMPETITION RESULTS'
    physicalTestStatus = 'PENDING - SERIAL-AUTO-001 fixed in code, awaiting physical re-test'
    cleanMachineStatus = 'BLOCKED - requires a second machine or VM; Windows Sandbox is unavailable on Windows 11 Home'
    deploymentStatus   = 'NOT APPROVED FOR FINAL DEPLOYMENT - AWAITING RC2a PHYSICAL COM AND RECONNECT TEST'
    knownLimitations   = 'documentation\0.9.0-known-limitations.md'
}
$mfPath = Join-Path $OutRoot 'manifests\release-manifest.json'
$manifest | ConvertTo-Json -Depth 6 | Set-Content $mfPath -Encoding utf8
# The support-bundle tool reads a manifest beside the executable.
Copy-Item $mfPath (Join-Path $dstPortable 'release-manifest.json') -Force
Write-Host "   $mfPath"

# ── checksums ─────────────────────────────────────────────────────────────
Write-Host "`n-- checksums --"
$ckOut = Join-Path $OutRoot 'checksums'
$lines = @('# Tech Aim 0.9.0 deployment preparation - SHA-256',
           "# generated for commit $commit", '')
foreach ($n in $verified.Keys) { $lines += ("{0} *{1}" -f $verified[$n], $n) }
$lines += ''
$lines += '# extracted portable payload'
$lines += ("{0} *portable/TechAim-0.9.0-RC2a/TechAim.exe" -f $exeDst)
$lines -join "`n" | Set-Content (Join-Path $ckOut 'SHA256SUMS.txt') -Encoding utf8
foreach ($n in $verified.Keys) {
    "$($verified[$n]) *$n" | Set-Content (Join-Path $ckOut "$n.sha256") -Encoding utf8
}
@"
How to verify
=============

    Get-FileHash <file> -Algorithm SHA256

Compare the Hash column with SHA256SUMS.txt. Any difference means the file is
not the accepted artefact - do not deploy it, and do not "just re-download".
Report the mismatch.
"@ | Set-Content (Join-Path $ckOut 'HOW-TO-VERIFY.txt') -Encoding utf8

Write-Host ""
Write-Host "Deployment preparation assembled at: $OutRoot"
Write-Host "Accepted RC packages were verified and NOT modified."
Write-Host ""
Write-Host "Next: powershell -File tools\deployment\Verify-Deployment.ps1 ``"
Write-Host "        -PackageDir '$dstPortable' ``"
Write-Host "        -ExpectDeveloperMode 0 -ExpectVersion '0.9.0-RC2a'"
