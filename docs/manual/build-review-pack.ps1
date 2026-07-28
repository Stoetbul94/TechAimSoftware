# Tech Aim Manuals PDF Review Pack (review-only tooling).
#
# Builds the six manuals through the normal pipeline, copies them into a
# dedicated review folder, generates a cover page and per-manual dividers,
# and merges everything into one bookmarked review PDF.
#
# Generated output is NEVER committed (manual-preview/ is gitignored) and no
# manual source is modified: provenance is stamped into temporary copies by
# build-manuals.ps1 exactly as in a normal build.
#
#   powershell -File docs\manual\build-review-pack.ps1
#
# Requires: Pandoc, python (with pypdf), and Chrome or Edge.
param(
    [string]$ReviewDir = ''
)

$ErrorActionPreference = 'Stop'
$manualDir = $PSScriptRoot
$repoRoot  = Split-Path -Parent (Split-Path -Parent $manualDir)
$outDir    = Join-Path $manualDir 'output'
if (-not $ReviewDir) {
    $ReviewDir = Join-Path $repoRoot 'manual-preview\TechAim-Manuals-Review'
}
$cssPath = Join-Path $manualDir '_shared\manual-print.css'

function Fail([string]$m) { Write-Host "ERROR: $m" -ForegroundColor Red }

foreach ($t in @('pandoc', 'python')) {
    if (-not (Get-Command $t -ErrorAction SilentlyContinue)) { Fail "$t not on PATH"; exit 2 }
}
$chromium = @(
    "${env:ProgramFiles}\Google\Chrome\Application\chrome.exe",
    "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
    "${env:ProgramFiles}\Microsoft\Edge\Application\msedge.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $chromium) { Fail 'No Chromium browser found'; exit 2 }

# ── 1. build the manuals through the normal pipeline ────────────────────
Write-Host '== building manuals ==' -ForegroundColor Cyan
& powershell -File (Join-Path $manualDir 'build-manuals.ps1') -Format both
if ($LASTEXITCODE -ne 0) { Fail 'manual generation failed - review pack aborted'; exit 1 }

$manifestSrc = Join-Path $outDir 'TechAim_Manual_Build_Manifest.json'
if (-not (Test-Path $manifestSrc)) { Fail 'build manifest missing'; exit 1 }
$build = Get-Content $manifestSrc -Raw | ConvertFrom-Json

$appCommit  = $build.applicationBaselineCommit
$docCommit  = $build.documentationSourceCommit
$buildStamp = $build.buildTimestamp
$docVersion = $build.documentVersion

$ORDER = @(
    @{ file='TechAim_Quick_Start_EN.pdf';          title='Quick Start Guide';        lang='English';        beta=$false },
    @{ file='TechAim_Operator_Manual_EN.pdf';      title='Operator Manual';          lang='English';        beta=$false },
    @{ file='TechAim_Troubleshooting_EN.pdf';      title='Troubleshooting Guide';    lang='English';        beta=$false },
    @{ file='TechAim_Quick_Start_DE_Beta.pdf';     title='Schnellstart (Beta)';      lang='Deutsch (Beta)'; beta=$true  },
    @{ file='TechAim_Operator_Manual_DE_Beta.pdf'; title='Bedienungsanleitung (Beta)'; lang='Deutsch (Beta)'; beta=$true  },
    @{ file='TechAim_Troubleshooting_DE_Beta.pdf'; title='Fehlersuche (Beta)';       lang='Deutsch (Beta)'; beta=$true  }
)

# ── 2. copy the six PDFs into the review folder ─────────────────────────
if (-not (Test-Path $ReviewDir)) { New-Item -ItemType Directory -Path $ReviewDir -Force | Out-Null }
$missing = @()
foreach ($m in $ORDER) {
    $src = Join-Path $outDir $m.file
    if (-not (Test-Path $src)) { $missing += $m.file; continue }
    Copy-Item $src (Join-Path $ReviewDir $m.file) -Force
}
if ($missing.Count -gt 0) { Fail ("manuals not generated: " + ($missing -join ', ')); exit 1 }
Write-Host "copied 6 manuals -> $ReviewDir"

# ── 3. cover + divider pages ────────────────────────────────────────────
$tmp = Join-Path $env:TEMP ("techaim_review_" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null

function Html-ToPdf([string]$html, [string]$pdf) {
    $htmlFile = [System.IO.Path]::ChangeExtension($pdf, '.html')
    Set-Content -Path $htmlFile -Value $html -Encoding utf8
    $uri  = 'file:///' + ($htmlFile -replace '\\','/')
    $prof = Join-Path $env:TEMP ("techaim_rp_" + [Guid]::NewGuid().ToString('N'))
    & $chromium --headless --disable-gpu --no-sandbox --no-first-run `
                --user-data-dir="$prof" --no-pdf-header-footer `
                --print-to-pdf="$pdf" $uri 2>$null
    $deadline = (Get-Date).AddSeconds(45); $prev = -1
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 300
        if (-not (Test-Path $pdf)) { continue }
        $len = (Get-Item $pdf).Length
        if ($len -gt 0 -and $len -eq $prev) { break }
        $prev = $len
    }
    Remove-Item -Recurse -Force $prof -ErrorAction SilentlyContinue
    return (Test-Path $pdf)
}

$style = @"
<style>
@page { size: A4; margin: 24mm 20mm; }
body { font-family: 'Segoe UI','DejaVu Sans',Arial,sans-serif; color:#14171C; }
.rule { height:6px; background:#C40046; margin:0 0 18px 0; }
h1 { font-size:26pt; margin:0 0 4px 0; letter-spacing:1px; }
h2 { font-size:15pt; color:#5C636E; font-weight:600; margin:0 0 22px 0; }
.warn { border:2px solid #C40046; padding:10px 14px; margin:18px 0; font-size:11pt; }
.beta { border:2px solid #E8A13C; padding:10px 14px; margin:18px 0; font-size:11pt; }
table { border-collapse:collapse; width:100%; font-size:10.5pt; margin-top:14px; }
td { padding:4px 6px; border-bottom:1px solid #D5D9DF; vertical-align:top; }
td.k { color:#5C636E; width:38%; }
ol { font-size:11pt; line-height:1.6; }
.small { font-size:9.5pt; color:#5C636E; margin-top:26px; }
.divtitle { font-size:22pt; margin-top:34vh; }
</style>
"@

# cover
$coverList = ($ORDER | ForEach-Object { "<li>$($_.title) &mdash; $($_.lang)</li>" }) -join "`n"
$coverHtml = @"
<html><head><meta charset="utf-8">$style</head><body>
<div class="rule"></div>
<h1>TECH AIM ELECTRONIC TARGET CONTROL</h1>
<h2>MANUALS REVIEW PACK</h2>
<div class="warn"><b>Internal review only.</b><br>Not approved for external distribution.<br>
Status: <b>GENERATED &mdash; HUMAN VISUAL CHECK REQUIRED</b></div>
<table>
<tr><td class="k">Product version</td><td>0.9.0</td></tr>
<tr><td class="k">Release channel</td><td>Pre-Beta Validation</td></tr>
<tr><td class="k">Document version</td><td>$docVersion</td></tr>
<tr><td class="k">Application baseline commit</td><td><code>$appCommit</code></td></tr>
<tr><td class="k">Documentation source commit</td><td><code>$docCommit</code></td></tr>
<tr><td class="k">Generated</td><td>$buildStamp</td></tr>
<tr><td class="k">Publisher</td><td>JAC SHOOTING SOLUTIONS (PTY) LTD</td></tr>
</table>
<h3 style="margin-top:26px;font-size:12pt;">Included manuals</h3>
<ol>$coverList</ol>
<div class="beta"><b>GERMAN BETA TRANSLATION &mdash; NATIVE TECHNICAL REVIEW REQUIRED</b><br>
The German editions are a beta translation and are not approved. The current
German application interface is <b>incomplete and may display a mixture of
German and English</b>. The German documents are not fully translated.</div>
<div class="small">Screenshots are not yet captured; none are embedded and no
placeholder image is presented as approved artwork. Page-by-page visual
inspection has not been performed.</div>
</body></html>
"@
$coverPdf = Join-Path $tmp 'cover.pdf'
if (-not (Html-ToPdf $coverHtml $coverPdf)) { Fail 'cover page generation failed'; exit 1 }

# dividers
$dividers = @{}
$i = 0
foreach ($m in $ORDER) {
    $i++
    $betaBlock = ''
    if ($m.beta) {
        $betaBlock = '<div class="beta"><b>GERMAN BETA TRANSLATION &mdash; NATIVE TECHNICAL REVIEW REQUIRED</b><br>' +
                     'Not fully translated and not approved. The German application interface is incomplete ' +
                     'and may display a mixture of German and English.</div>'
    }
    $dHtml = @"
<html><head><meta charset="utf-8">$style</head><body>
<div class="rule"></div>
<div class="divtitle"><b>$i. $($m.title)</b></div>
<h2>$($m.lang)</h2>
$betaBlock
<div class="small">Tech Aim Electronic Target Control 0.9.0 &middot; Pre-Beta Validation<br>
Application baseline <code>$appCommit</code> &middot; Documentation source <code>$docCommit</code></div>
</body></html>
"@
    $dPdf = Join-Path $tmp ("div$i.pdf")
    if (-not (Html-ToPdf $dHtml $dPdf)) { Fail "divider $i generation failed"; exit 1 }
    $dividers[$m.file] = $dPdf
}
Write-Host 'cover + 6 dividers generated'

# ── 4. merge with bookmarks (python/pypdf) ──────────────────────────────
$plan = [ordered]@{
    reviewDir = $ReviewDir
    cover     = $coverPdf
    appCommit = $appCommit
    docCommit = $docCommit
    items     = @($ORDER | ForEach-Object {
        [ordered]@{ file = $_.file; title = $_.title; lang = $_.lang;
                    divider = $dividers[$_.file] }
    })
}
$planPath = Join-Path $tmp 'plan.json'
$plan | ConvertTo-Json -Depth 5 | Set-Content -Path $planPath -Encoding utf8

$env:PYTHONIOENCODING = 'utf-8'
& python (Join-Path $manualDir 'merge-review-pack.py') $planPath
if ($LASTEXITCODE -ne 0) { Fail 'merge failed'; exit 1 }

Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
Write-Host ''
Write-Host "Review pack: $ReviewDir" -ForegroundColor Green
Write-Host 'STATUS: GENERATED - HUMAN VISUAL CHECK REQUIRED' -ForegroundColor Yellow
exit 0
