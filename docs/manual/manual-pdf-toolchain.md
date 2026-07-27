# Manual PDF Toolchain — Decision Record

Document version 1.0 (P0.1) · Application commit `169eef9`

## Decision

**Pandoc (Markdown → self-contained HTML) + headless Chromium (HTML → PDF).**

Chromium is invoked through whichever of Chrome or Microsoft Edge is present.
**Neither requires installation on a Windows machine** — Edge ships with
Windows 10/11, so any operator or build agent can reproduce the manuals with
Pandoc as the only added dependency.

## Options evaluated

| Option | A4 | Clickable TOC | Selectable text | Bookmarks | Umlauts | Windows install burden | Verdict |
|---|---|---|---|---|---|---|---|
| Pandoc + **headless Chromium** | yes | yes (in-page anchors) | yes | no | yes | **none** — Edge is preinstalled | **CHOSEN** |
| Pandoc + MiKTeX | yes | yes | yes | yes | needs font setup | ~500 MB, on-demand package fetching | rejected |
| Pandoc + TeX Live | yes | yes | yes | yes | good with XeLaTeX | 2–7 GB | rejected |
| Pandoc + wkhtmltopdf | yes | partial | yes | limited | yes | separate install; unmaintained (Qt 4 WebKit) | rejected |
| Pandoc + WeasyPrint | yes | yes | yes | yes | yes | Python + GTK native deps, awkward on Windows | rejected |

### Why headless Chromium

1. **Zero install burden.** The deciding factor. A LaTeX distribution is
   hundreds of megabytes to several gigabytes, and MiKTeX's on-demand package
   installation makes builds non-deterministic on a fresh machine. Edge is
   already on every supported Windows target.
2. **Predictable Windows behaviour.** The same engine renders the HTML that was
   already reviewed, so PDF output matches the HTML output.
3. **Font coverage.** The browser resolves system fonts, so German umlauts and
   ß render without the font configuration a LaTeX route needs.
4. **Screenshots.** Raster images embed at native resolution with no
   `\includegraphics` sizing work.

### What this choice costs

Recorded honestly, because these are real limitations:

- **No PDF bookmark tree.** Chromium's print-to-PDF does not emit outline
  bookmarks. The in-document table of contents is clickable, but the reader's
  sidebar outline will be empty. LaTeX would give bookmarks; it was rejected on
  install burden. **Revisit if bookmarks become a requirement.**
- **No native page headers/footers with document metadata.** `--print-to-pdf`
  supports only a default header/footer, which is disabled. Page numbers and
  the running document title are instead supplied by CSS
  (`@page` / `position: fixed`) in the print stylesheet.
- **Page breaking is CSS-driven**, so orphan-heading control depends on
  `break-after: avoid`, not on LaTeX's typesetting.

## Reproducing

```
powershell -File docs\manual\build-manuals.ps1 -Format pdf
powershell -File docs\manual\build-manuals.ps1 -Format both
```

Requirements: **Pandoc** on `PATH`; **Chrome or Edge** installed (Edge is
present by default on Windows).

Output goes to `docs/manual/output/`, which is gitignored — the Markdown source
plus this script is the committed artefact, not the generated PDFs.

## Failure behaviour

The script must never report success on failure. It:

- checks `$LASTEXITCODE` after every external invocation (native commands do
  **not** throw in PowerShell — an earlier revision used `try/catch` and would
  have reported a failed PDF build as successful);
- verifies each output file exists **and** is non-zero;
- applies a minimum-size sanity threshold, so a truncated or blank PDF is
  caught rather than counted;
- prints a per-document result table and exits non-zero if anything failed.
