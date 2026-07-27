# Tech Aim manual publication pipeline (P0.1).
#
# Markdown is the CONTROLLED SOURCE. This script renders it; it never edits it.
# Toolchain decision and its known limits: docs/manual/manual-pdf-toolchain.md
#
#   powershell -File docs\manual\build-manuals.ps1            # html + pdf
#   powershell -File docs\manual\build-manuals.ps1 -Format pdf
#
# Requires: Pandoc on PATH. For PDF also Chrome or Edge (Edge ships with
# Windows, so no extra install is normally needed).
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
$cssPath   = Join-Path $manualDir '_shared\manual-print.css'

# A PDF smaller than this is truncated or blank, not a real manual.
$MinPdfBytes  = 20KB
$MinHtmlBytes = 5KB

function Fail([string]$msg) { Write-Host "ERROR: $msg" -ForegroundColor Red }

# -- tool discovery ------------------------------------------------------
if (-not (Get-Command pandoc -ErrorAction SilentlyContinue)) {
    Fail "Pandoc is not installed or not on PATH. See https://pandoc.org/installing.html"
    exit 2
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
        Fail "Edge normally ships with Windows 10/11; install Chrome or Edge, or use -Format html."
        exit 2
    }
    Write-Host "PDF engine: $chromium"
}

if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

# German outputs carry _DE_Beta so a beta translation can never be mistaken
# for an approved edition.
$docs = @(
    @{ src='TechAim_Quick_Start_EN.md';     out='TechAim_Quick_Start_EN';
       title='Tech Aim Electronic Target Control - Quick Start Guide' },
    @{ src='TechAim_Operator_Manual_EN.md'; out='TechAim_Operator_Manual_EN';
       title='Tech Aim Electronic Target Control - Operator Manual' },
    @{ src='TechAim_Troubleshooting_EN.md'; out='TechAim_Troubleshooting_EN';
       title='Tech Aim Electronic Target Control - Troubleshooting Guide' },
    @{ src='TechAim_Quick_Start_DE.md';     out='TechAim_Quick_Start_DE_Beta';
       title='Tech Aim Electronic Target Control - Schnellstart (Beta)' },
    @{ src='TechAim_Operator_Manual_DE.md'; out='TechAim_Operator_Manual_DE_Beta';
       title='Tech Aim Electronic Target Control - Bedienungsanleitung (Beta)' },
    @{ src='TechAim_Troubleshooting_DE.md'; out='TechAim_Troubleshooting_DE_Beta';
       title='Tech Aim Electronic Target Control - Fehlersuche (Beta)' }
)

$common = @(
    '--standalone'
    '--toc'
    '--toc-depth=3'
    '--from=gfm+smart'
    '--embed-resources'          # images/CSS inline: output is self-contained
    # Image paths in the Markdown are relative to the manual directory. Without
    # this, pandoc silently emits an unresolved <img src> and the diagrams are
    # simply missing from the PDF with no error.
    '--resource-path', $manualDir
)
if (Test-Path $cssPath) { $common += @('--css', $cssPath) }

$results = @()
$failed  = @()

foreach ($d in $docs) {
    $src = Join-Path $manualDir $d.src
    if (-not (Test-Path $src)) {
        Fail "missing source: $($d.src)"; $failed += $d.src; continue
    }

    $htmlPath = Join-Path $outDir ($d.out + '.html')
    $pdfPath  = Join-Path $outDir ($d.out + '.pdf')

    # -- Markdown -> HTML (always: the PDF route renders this HTML) ------
    # pandoc is a NATIVE command: a non-zero exit does not throw, so
    # $LASTEXITCODE must be checked or a failure reports as success.
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

    # Every image the source references must have been RESOLVED and embedded.
    # pandoc emits an unresolved <img src="..."> without failing, which
    # silently drops diagrams from the PDF — guard against that class of bug.
    $srcText  = Get-Content $src -Raw
    $imgCount = ([regex]::Matches($srcText, '!\[[^\]]*\]\([^)]+\)')).Count
    if ($imgCount -gt 0) {
        $htmlText   = Get-Content $htmlPath -Raw
        # <img> only — pandoc's default template carries a <script src> shim
        # for legacy IE which is not an image and never embeds.
        $unresolved = [regex]::Matches($htmlText, '<img[^>]+src="(?!data:)[^"]+"')
        if ($unresolved.Count -gt 0) {
            Fail "$($d.src): $($unresolved.Count) image reference(s) did not embed - e.g. $($unresolved[0].Value)"
            $failed += $d.src; continue
        }
    }

    $pdfSize = 0
    $pages   = 0
    if ($Format -ne 'html') {
        $uri = 'file:///' + ($htmlPath -replace '\\', '/')
        # Isolated profile so a running browser session cannot interfere.
        $profileDir = Join-Path $env:TEMP ("techaim_pdf_" + [System.Guid]::NewGuid().ToString('N'))
        & $chromium --headless --disable-gpu --no-sandbox --no-first-run `
                    --user-data-dir="$profileDir" --no-pdf-header-footer `
                    --print-to-pdf="$pdfPath" $uri 2>$null
        $chromeExit = $LASTEXITCODE

        # Chromium can return before the PDF is flushed to disk. Wait for the
        # file to appear AND for its size to stop changing, so a partially
        # written PDF is never measured or validated.
        $deadline = (Get-Date).AddSeconds(60)
        $lastLen  = -1
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 400
            if (-not (Test-Path $pdfPath)) { continue }
            $len = (Get-Item $pdfPath).Length
            if ($len -gt 0 -and $len -eq $lastLen) { break }   # size settled
            $lastLen = $len
        }
        Remove-Item -Recurse -Force $profileDir -ErrorAction SilentlyContinue

        if ($chromeExit -ne 0) {
            Fail "Chromium exited $chromeExit for $($d.out)"; $failed += $d.src; continue
        }
        if (-not (Test-Path $pdfPath)) {
            Fail "no PDF produced within the timeout: $pdfPath"; $failed += $d.src; continue
        }
        $pdfSize = (Get-Item $pdfPath).Length
        if ($pdfSize -lt $MinPdfBytes) {
            Fail "PDF suspiciously small ($pdfSize bytes) - likely blank or truncated: $pdfPath"
            $failed += $d.src; continue
        }
        # Page count from the PDF page tree (best effort; 0 = not determined).
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
        Status   = 'OK'
    }
    Write-Host ("  built {0}" -f $d.out)
}

Write-Host ''
if ($results) { $results | Format-Table -AutoSize | Out-String | Write-Host }

if ($failed.Count -gt 0) {
    Fail ("FAILED: " + ($failed -join ', '))
    exit 1
}
if ($results.Count -ne $docs.Count) {
    Fail "expected $($docs.Count) documents, produced $($results.Count)"
    exit 1
}

Write-Host "All $($results.Count) documents generated in $outDir" -ForegroundColor Green
Write-Host ''
Write-Host 'STILL REQUIRED - page-by-page HUMAN VISUAL inspection:' -ForegroundColor Yellow
Write-Host '  - clickable table of contents'
Write-Host '  - no clipped headings, no orphan heading at a page foot'
Write-Host '  - no unintended blank pages; tables fit; paths wrap'
Write-Host '  - screenshots and diagrams legible'
Write-Host '  - German umlauts and ss render correctly'
Write-Host '  - readable when printed in grayscale'
Write-Host 'Record findings in docs/manual/manual-pdf-validation.md'
exit 0
