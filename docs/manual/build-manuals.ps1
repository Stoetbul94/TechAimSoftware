# Tech Aim manual publication pipeline (P0.1).
#
# Markdown is the CONTROLLED SOURCE and is NEVER MODIFIED by this script.
# Provenance is stamped at BUILD TIME into temporary copies:
#
#   docs/manual/*.md  (placeholders)
#     -> stamp-commits.py --out <staging>   (substituted copies)
#     -> pandoc                             (HTML)
#     -> headless Chromium                  (PDF)
#     -> TechAim_Manual_Build_Manifest.json (machine-readable provenance)
#
# Stamping concrete hashes into tracked source is self-defeating: committing
# the stamp changes HEAD, so the documentation source commit is stale the
# instant it is written. Hence the staging directory.
#
# Toolchain decision and its known limits: docs/manual/manual-pdf-toolchain.md
#
#   powershell -File docs\manual\build-manuals.ps1            # html + pdf
#   powershell -File docs\manual\build-manuals.ps1 -Format pdf
#
# Requires: Pandoc and python on PATH. For PDF also Chrome or Edge (Edge
# ships with Windows, so no extra install is normally needed).
#
# Generated output goes to docs/manual/output/ which is gitignored — the
# source plus this script is the committed artefact.
param(
    [ValidateSet('pdf', 'html', 'both')]
    [string]$Format = 'both'
)

$ErrorActionPreference = 'Stop'
$manualDir = $PSScriptRoot
$outDir    = Join-Path $manualDir 'output'
$stageDir  = Join-Path $outDir '_staged'
$cssPath   = Join-Path $manualDir '_shared\manual-print.css'
$stamper   = Join-Path $manualDir 'stamp-commits.py'

# A PDF smaller than this is truncated or blank, not a real manual.
$MinPdfBytes  = 20KB
$MinHtmlBytes = 5KB

function Fail([string]$msg) { Write-Host "ERROR: $msg" -ForegroundColor Red }

# -- tool discovery ------------------------------------------------------
foreach ($tool in @('pandoc', 'python')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Fail "$tool is not installed or not on PATH."
        exit 2
    }
}

$chromium = $null
if ($Format -ne 'html') {
    $candidates = @(
        "${env:ProgramFiles}\Google\Chrome\Application\chrome.exe",
        "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
        "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
        "${env:ProgramFiles}\Microsoft\Edge\Application\msedge.exe"
    )
    $chromium = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $chromium) {
        Fail "No Chromium browser found (Chrome or Edge). PDF output is not possible."
        exit 2
    }
    Write-Host "PDF engine: $chromium"
}

if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

# -- build-time provenance stamping --------------------------------------
# The tracked Markdown is only READ. A stamping failure stops the build; a
# missing Git value is a hard failure inside the stamper, never a stale or
# "unknown" carry-forward.
$env:PYTHONIOENCODING = 'utf-8'
$provJson = & python $stamper --print-json
if ($LASTEXITCODE -ne 0) { Fail "provenance resolution failed"; exit 3 }
$prov = $provJson | ConvertFrom-Json

& python $stamper --out $stageDir
if ($LASTEXITCODE -ne 0) { Fail "stamping failed - build stopped"; exit 3 }

Write-Host ("application baseline commit : {0}" -f $prov.APPLICATION_BASELINE_COMMIT)
Write-Host ("documentation source commit : {0}" -f $prov.DOCUMENTATION_SOURCE_COMMIT)

# German outputs carry _DE_Beta so a beta translation can never be mistaken
# for an approved edition.
$docs = @(
    @{ src='TechAim_Quick_Start_EN.md';     out='TechAim_Quick_Start_EN';     lang='en';
       title='Tech Aim Electronic Target Control - Quick Start Guide' },
    @{ src='TechAim_Operator_Manual_EN.md'; out='TechAim_Operator_Manual_EN'; lang='en';
       title='Tech Aim Electronic Target Control - Operator Manual' },
    @{ src='TechAim_Troubleshooting_EN.md'; out='TechAim_Troubleshooting_EN'; lang='en';
       title='Tech Aim Electronic Target Control - Troubleshooting Guide' },
    @{ src='TechAim_Quick_Start_DE.md';     out='TechAim_Quick_Start_DE_Beta'; lang='de-DE';
       title='Tech Aim Electronic Target Control - Schnellstart (Beta)' },
    @{ src='TechAim_Operator_Manual_DE.md'; out='TechAim_Operator_Manual_DE_Beta'; lang='de-DE';
       title='Tech Aim Electronic Target Control - Bedienungsanleitung (Beta)' },
    @{ src='TechAim_Troubleshooting_DE.md'; out='TechAim_Troubleshooting_DE_Beta'; lang='de-DE';
       title='Tech Aim Electronic Target Control - Fehlersuche (Beta)' }
)

$common = @(
    '--standalone'
    '--toc'
    '--toc-depth=3'
    '--from=gfm+smart'
    '--embed-resources'
    '--resource-path', $stageDir
)
if (Test-Path $cssPath) { $common += @('--css', $cssPath) }

function Get-Sha256([string]$path) {
    (Get-FileHash -Path $path -Algorithm SHA256).Hash.ToLower()
}

# Chromium can still be flushing a PDF after it exits, and the LAST document
# has no following work to absorb that delay. Wait until the file can be
# opened exclusively AND two consecutive hashes agree, so the recorded hash
# always matches the delivered file.
function Wait-Settled([string]$path, [int]$timeoutSec = 60) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    $prev = $null
    while ((Get-Date) -lt $deadline) {
        try {
            $fs = [System.IO.File]::Open($path, 'Open', 'Read', 'None')
            $fs.Close()
        } catch { Start-Sleep -Milliseconds 250; continue }   # still locked
        $h = Get-Sha256 $path
        if ($prev -and $h -eq $prev) { return $h }
        $prev = $h
        Start-Sleep -Milliseconds 250
    }
    return $null
}

$results  = @()
$manifest = @()
$failed   = @()

foreach ($d in $docs) {
    # Render from the STAGED copy, never from the tracked source.
    $src = Join-Path $stageDir $d.src
    if (-not (Test-Path $src)) {
        Fail "missing staged source: $($d.src)"; $failed += $d.src; continue
    }

    $htmlPath = Join-Path $outDir ($d.out + '.html')
    $pdfPath  = Join-Path $outDir ($d.out + '.pdf')

    pandoc $src @common '--metadata' "title=$($d.title)" '-o' $htmlPath
    if ($LASTEXITCODE -ne 0) {
        Fail "pandoc failed for $($d.src) (exit $LASTEXITCODE)"; $failed += $d.src; continue
    }
    if (-not (Test-Path $htmlPath)) {
        Fail "pandoc reported success but produced no file: $htmlPath"; $failed += $d.src; continue
    }
    $htmlSize = (Get-Item $htmlPath).Length
    if ($htmlSize -lt $MinHtmlBytes) {
        Fail "HTML suspiciously small ($htmlSize bytes): $htmlPath"; $failed += $d.src; continue
    }

    # No placeholder may survive into published output.
    $htmlText = Get-Content $htmlPath -Raw
    if ($htmlText -match '\{\{[A-Z_]+\}\}') {
        Fail "$($d.out): unresolved placeholder in generated HTML"; $failed += $d.src; continue
    }
    # Every referenced image must have embedded.
    $unresolved = [regex]::Matches($htmlText, '<img[^>]+src="(?!data:)[^"]+"')
    if ($unresolved.Count -gt 0) {
        Fail "$($d.src): $($unresolved.Count) image(s) did not embed - e.g. $($unresolved[0].Value)"
        $failed += $d.src; continue
    }

    $pdfSize = 0; $pages = 0
    if ($Format -ne 'html') {
        $uri = 'file:///' + ($htmlPath -replace '\\', '/')
        $profileDir = Join-Path $env:TEMP ("techaim_pdf_" + [System.Guid]::NewGuid().ToString('N'))
        & $chromium --headless --disable-gpu --no-sandbox --no-first-run `
                    --user-data-dir="$profileDir" --no-pdf-header-footer `
                    --print-to-pdf="$pdfPath" $uri 2>$null
        $chromeExit = $LASTEXITCODE

        # Chromium can return before the PDF is flushed; wait for the file to
        # appear AND its size to settle so a partial file is never measured.
        $deadline = (Get-Date).AddSeconds(60); $lastLen = -1
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 400
            if (-not (Test-Path $pdfPath)) { continue }
            $len = (Get-Item $pdfPath).Length
            if ($len -gt 0 -and $len -eq $lastLen) { break }
            $lastLen = $len
        }
        Remove-Item -Recurse -Force $profileDir -ErrorAction SilentlyContinue

        if ($chromeExit -ne 0) { Fail "Chromium exited $chromeExit for $($d.out)"; $failed += $d.src; continue }
        if (-not (Test-Path $pdfPath)) { Fail "no PDF produced within the timeout: $pdfPath"; $failed += $d.src; continue }
        $pdfSize = (Get-Item $pdfPath).Length
        if ($pdfSize -lt $MinPdfBytes) {
            Fail "PDF suspiciously small ($pdfSize bytes): $pdfPath"; $failed += $d.src; continue
        }
        try {
            $bytes = [System.IO.File]::ReadAllBytes($pdfPath)
            $latin = [System.Text.Encoding]::GetEncoding(28591).GetString($bytes)
            $pages = ([regex]::Matches($latin, '/Type\s*/Page[^s]')).Count
        } catch { $pages = 0 }
    }

    $results += [pscustomobject]@{
        Document = $d.out
        HTML_KB  = [math]::Round($htmlSize / 1KB)
        PDF_KB   = if ($pdfSize) { [math]::Round($pdfSize / 1KB) } else { '-' }
        Pages    = if ($pages)   { $pages } else { '-' }
        Status   = 'GENERATED'
    }

    $entry = [ordered]@{
        product                   = 'Tech Aim Electronic Target Control'
        productVersion            = '0.9.0'
        releaseChannel            = 'Pre-Beta Validation'
        documentVersion           = $prov.DOCUMENT_VERSION
        applicationBaselineCommit = $prov.APPLICATION_BASELINE_COMMIT_FULL
        documentationSourceCommit = $prov.DOCUMENTATION_SOURCE_COMMIT_FULL
        buildTimestamp            = $prov.DOCUMENT_BUILD_TIMESTAMP
        language                  = $d.lang
        sourceDocument            = $d.src
        filename                  = ($d.out + '.html')
        format                    = 'html'
        pageCount                 = $null
        fileSizeBytes             = 0
        sha256                    = ''
        generationTool            = 'pandoc'
        visualValidationStatus    = 'HUMAN VISUAL CHECK REQUIRED'
    }
    $manifest += $entry

    if ($Format -ne 'html') {
        $manifest += [ordered]@{
            product                   = 'Tech Aim Electronic Target Control'
            productVersion            = '0.9.0'
            releaseChannel            = 'Pre-Beta Validation'
            documentVersion           = $prov.DOCUMENT_VERSION
            applicationBaselineCommit = $prov.APPLICATION_BASELINE_COMMIT_FULL
            documentationSourceCommit = $prov.DOCUMENTATION_SOURCE_COMMIT_FULL
            buildTimestamp            = $prov.DOCUMENT_BUILD_TIMESTAMP
            language                  = $d.lang
            sourceDocument            = $d.src
            filename                  = ($d.out + '.pdf')
            format                    = 'pdf'
            pageCount                 = $pages
            fileSizeBytes             = 0
            sha256                    = ''
            generationTool            = ('pandoc + headless Chromium (' + (Split-Path $chromium -Leaf) + ')')
            visualValidationStatus    = 'HUMAN VISUAL CHECK REQUIRED'
        }
    }

    Write-Host ("  built {0}" -f $d.out)
}

Write-Host ''
if ($results) { $results | Format-Table -AutoSize | Out-String | Write-Host }

if ($failed.Count -gt 0) { Fail ("FAILED: " + ($failed -join ', ')); exit 1 }
if ($results.Count -ne $docs.Count) {
    Fail "expected $($docs.Count) documents, produced $($results.Count)"; exit 1
}

# -- final measurement pass ---------------------------------------------
# Size and SHA-256 are computed HERE, after every document is finished.
# Measuring inside the loop raced Chromium's final flush: a file could be
# hashed while still being written, yielding a manifest hash that did not
# match the delivered file. Hashing once at the end removes the race.
foreach ($entry in $manifest) {
    $fp = Join-Path $outDir $entry.filename
    if (-not (Test-Path $fp)) { Fail "manifest target missing: $fp"; exit 1 }
    $settled = Wait-Settled $fp
    if (-not $settled) { Fail "output never settled (still being written?): $fp"; exit 1 }
    $entry.fileSizeBytes = (Get-Item $fp).Length
    $entry.sha256        = $settled
    if ($entry.fileSizeBytes -le 0) { Fail "empty output: $fp"; exit 1 }
}

# -- manifest ------------------------------------------------------------
$manifestPath = Join-Path $outDir 'TechAim_Manual_Build_Manifest.json'
$manifestDoc = [ordered]@{
    schema                    = 'techaim.manual-build-manifest/1'
    product                   = 'Tech Aim Electronic Target Control'
    productVersion            = '0.9.0'
    releaseChannel            = 'Pre-Beta Validation'
    documentVersion           = $prov.DOCUMENT_VERSION
    applicationBaselineCommit = $prov.APPLICATION_BASELINE_COMMIT_FULL
    documentationSourceCommit = $prov.DOCUMENTATION_SOURCE_COMMIT_FULL
    buildTimestamp            = $prov.DOCUMENT_BUILD_TIMESTAMP
    overallStatus             = 'GENERATED - HUMAN VISUAL CHECK REQUIRED'
    screenshotsEmbedded       = $false
    screenshotNote            = 'No screenshots exist. None are embedded; none are placeholders.'
    diagramsEmbedded          = 11
    documents                 = $manifest
}
$manifestDoc | ConvertTo-Json -Depth 6 |
    Set-Content -Path $manifestPath -Encoding utf8
Write-Host "manifest: $manifestPath"

# The staging directory is build scratch, not output.
Remove-Item -Recurse -Force $stageDir -ErrorAction SilentlyContinue

Write-Host "All $($results.Count) documents generated in $outDir" -ForegroundColor Green
Write-Host ''
Write-Host 'STATUS: GENERATED - HUMAN VISUAL CHECK REQUIRED' -ForegroundColor Yellow
Write-Host 'Page-by-page inspection has NOT been performed. Still required:'
Write-Host '  - clickable table of contents'
Write-Host '  - no clipped headings, no orphan heading at a page foot'
Write-Host '  - no unintended blank pages; tables fit; paths wrap'
Write-Host '  - diagrams legible; German umlauts and ss render'
Write-Host '  - readable when printed in grayscale'
Write-Host 'Record findings in docs/manual/manual-pdf-validation.md'
exit 0
