# Tech Aim manual PDF/HTML generation (P0 Phase J).
#
# Markdown is the CONTROLLED SOURCE. This script renders it; it never edits it.
# Generated PDFs are internal drafts during P0 and are NOT committed
# (docs/manual/output/ is gitignored) — the source plus this script is the
# reproducible artefact.
#
#   powershell -File docs\manual\build-manuals.ps1
#   powershell -File docs\manual\build-manuals.ps1 -Format html
#
# Requires Pandoc (https://pandoc.org). A PDF engine (wkhtmltopdf or a LaTeX
# engine) is required for -Format pdf; HTML needs Pandoc only.
param(
    [ValidateSet('pdf', 'html', 'both')]
    [string]$Format = 'both'
)

$ErrorActionPreference = 'Stop'
$manualDir = $PSScriptRoot
$outDir    = Join-Path $manualDir 'output'
$logo      = Join-Path (Split-Path -Parent (Split-Path -Parent $manualDir)) 'images\logo\techaim_color.png'

if (-not (Get-Command pandoc -ErrorAction SilentlyContinue)) {
    Write-Error "Pandoc is not installed or not on PATH. See https://pandoc.org/installing.html"
    exit 2
}
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

# source basename -> output basename. German outputs carry _DE_Beta so a
# beta translation can never be mistaken for an approved edition.
$docs = @(
    @{ src = 'TechAim_Quick_Start_EN.md';     out = 'TechAim_Quick_Start_EN';
       title = 'Tech Aim Electronic Target Control - Quick Start Guide' },
    @{ src = 'TechAim_Operator_Manual_EN.md'; out = 'TechAim_Operator_Manual_EN';
       title = 'Tech Aim Electronic Target Control - Operator Manual' },
    @{ src = 'TechAim_Troubleshooting_EN.md'; out = 'TechAim_Troubleshooting_EN';
       title = 'Tech Aim Electronic Target Control - Troubleshooting Guide' },
    @{ src = 'TechAim_Quick_Start_DE.md';     out = 'TechAim_Quick_Start_DE_Beta';
       title = 'Tech Aim Electronic Target Control - Schnellstart (Beta)' },
    @{ src = 'TechAim_Operator_Manual_DE.md'; out = 'TechAim_Operator_Manual_DE_Beta';
       title = 'Tech Aim Electronic Target Control - Bedienungsanleitung (Beta)' },
    @{ src = 'TechAim_Troubleshooting_DE.md'; out = 'TechAim_Troubleshooting_DE_Beta';
       title = 'Tech Aim Electronic Target Control - Fehlersuche (Beta)' }
)

$common = @(
    '--standalone'
    '--toc'                       # automated table of contents
    '--toc-depth=3'
    '--number-sections'
    '--from=gfm+smart'
    '-V', 'papersize=a4'
    '-V', 'geometry:margin=2.2cm'
    '-V', 'linkcolor=black'       # readable when printed in grayscale
    '-V', 'colorlinks=true'
)

$failed = @()
foreach ($d in $docs) {
    $src = Join-Path $manualDir $d.src
    if (-not (Test-Path $src)) { Write-Warning "missing source: $($d.src)"; continue }

    if ($Format -eq 'html' -or $Format -eq 'both') {
        $dst = Join-Path $outDir ($d.out + '.html')
        Write-Host "HTML -> $dst"
        # pandoc is a native command: a non-zero exit does NOT throw, so
        # $LASTEXITCODE must be checked or failures would report as success.
        pandoc $src @common '--metadata' "title=$($d.title)" '--embed-resources' '-o' $dst
        if ($LASTEXITCODE -ne 0) { $failed += "$($d.src) (html)" }
    }

    if ($Format -eq 'pdf' -or $Format -eq 'both') {
        $dst = Join-Path $outDir ($d.out + '.pdf')
        Write-Host "PDF  -> $dst"
        # DejaVu covers German umlauts and ß; a font lacking them would
        # silently drop glyphs, which is exactly the failure being guarded.
        pandoc $src @common '--metadata' "title=$($d.title)" '-V' 'mainfont=DejaVu Sans' '-o' $dst
        if ($LASTEXITCODE -ne 0) { $failed += "$($d.src) (pdf)" }
    }
}

Write-Host ''
if ($failed.Count -gt 0) {
    Write-Warning "Failed: $($failed -join ', ')"
    Write-Warning "For PDF output install a PDF engine (wkhtmltopdf, or TeX for pdflatex/xelatex)."
    exit 1
}
Write-Host "Done. Output in $outDir"
Write-Host ""
Write-Host "MANUAL CHECK REQUIRED on generated PDFs:" -ForegroundColor Yellow
Write-Host "  - clickable TOC and bookmarks"
Write-Host "  - selectable text; screenshots legible"
Write-Host "  - no clipped content, no orphan headings, no unintended blank pages"
Write-Host "  - German umlauts and ss render correctly"
Write-Host "  - readable when printed in grayscale"
