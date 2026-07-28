# Manual PDF Validation Record

Document version 1.0 (P0.1) · Application commit `169eef9`
Generated: Pandoc → self-contained HTML → headless Chromium
(`Chrome 141`, `--print-to-pdf`)

> **NO PDF IS APPROVED.**
> Page-by-page **visual** inspection has **not** been performed. PDF page
> rendering is unavailable in this environment (`pdftoppm` / poppler is not
> installed), so no page has been seen. Everything below is **structural**
> validation performed programmatically on the PDF content stream.
>
> Approval requires a human to open each file and work the checklist in
> section 3.

---

## 1. Generation results

All six generated successfully. The build script fails on a non-zero exit, a
missing file, or a file below a minimum-size threshold — it cannot report
success on a failed conversion.

| # | Filename | Pages | Size | Extracted text | Status |
|---|---|---|---|---|---|
| 1 | `TechAim_Quick_Start_EN.pdf` | 6 | 197 KB | 8,860 chars | GENERATED |
| 2 | `TechAim_Operator_Manual_EN.pdf` | 23 | 698 KB | 30,122 chars | GENERATED (includes all 11 diagrams) |
| 3 | `TechAim_Troubleshooting_EN.pdf` | 12 | 241 KB | 18,983 chars | GENERATED |
| 4 | `TechAim_Quick_Start_DE_Beta.pdf` | 6 | 176 KB | 7,726 chars | GENERATED |
| 5 | `TechAim_Operator_Manual_DE_Beta.pdf` | 9 | 450 KB | 15,742 chars | GENERATED |
| 6 | `TechAim_Troubleshooting_DE_Beta.pdf` | 8 | 171 KB | 11,333 chars | GENERATED |

**Total 64 pages, 1.93 MB.** File sizes are reasonable for distribution.

## 2. Structural checks performed (automated)

| Check | Method | Result |
|---|---|---|
| Selectable text | text extraction per page | **PASS** — all six yield substantial text; none is an image-only scan |
| German umlauts / ß | search extracted text for `äöüßÄÖÜ` | **PASS** — present in all three German PDFs, correctly encoded |
| No legacy product identity | search for `Seta Electronic`, `Seeds Electronic`, `Hello World` | **PASS** — absent from all six |
| German beta marking | search for `GERMAN BETA TRANSLATION` | **PASS** — present in all three German PDFs, absent from the English three |
| Non-zero, non-truncated | size threshold in the build script | **PASS** |
| Page count determinable | PDF page-tree parse | **PASS** |

### What these checks do NOT prove

Text extraction confirms the characters are *present and correctly encoded*. It
says nothing about whether they are **visible, positioned correctly, or
unclipped**. A heading pushed off the page edge still extracts fine.

## 3. Human visual checklist — NOT PERFORMED

Every row below is **HUMAN VISUAL CHECK REQUIRED**.

| # | Check | Status |
|---|---|---|
| 3.1 | Tech Aim logo correct and not distorted | NOT PERFORMED |
| 3.2 | Title, document version, application version, language on page 1 | NOT PERFORMED |
| 3.3 | German beta warning prominent on the German PDFs | NOT PERFORMED |
| 3.4 | Page numbers present and correct | NOT PERFORMED |
| 3.5 | Table of contents present and clickable | NOT PERFORMED |
| 3.6 | Internal links work | NOT PERFORMED |
| 3.7 | No clipped headings | NOT PERFORMED |
| 3.8 | No isolated heading at the foot of a page | NOT PERFORMED |
| 3.9 | No unintended blank pages | NOT PERFORMED |
| 3.10 | Diagrams legible at print size, in grayscale, not clipped at a page break | **HUMAN VISUAL CHECK REQUIRED** — NOT PERFORMED. All 11 are generated, committed and confirmed *present* in the PDF; nobody has *looked* at one |
| 3.11 | Screenshots legible | **BLOCKED** — no screenshots exist yet |
| 3.12 | Tables fit the page width | NOT PERFORMED |
| 3.13 | Paths, commands and decision trees wrap rather than run off | NOT PERFORMED |
| 3.14 | German long compounds (`Streukreisdurchmesser`) do not overflow | NOT PERFORMED |
| 3.15 | Readable printed in grayscale | NOT PERFORMED |
| 3.16 | No personal data, no developer paths | PASS (automated) |

## 4. Known limitations of the chosen toolchain

Recorded honestly — see `manual-pdf-toolchain.md` for why the trade was made:

1. **No PDF bookmark outline.** Chromium's print-to-PDF emits none. The
   in-document TOC is present; the reader's sidebar outline will be empty.
2. **Page furniture is CSS-driven.** Page numbers come from a CSS `@page`
   margin box, not from a typesetting engine. **Whether they actually render
   is unverified** — it is check 3.4 above.
3. **Page breaking is CSS-driven**, so orphan control depends on
   `break-after: avoid` rather than proper typesetting. Checks 3.7–3.9 are
   the real test.

## 5. Issues found / corrected in this phase

| Issue | Resolution |
|---|---|
| Build script reported success when the PDF engine failed (native commands do not throw in PowerShell) | Fixed — checks `$LASTEXITCODE`, existence and minimum size |
| Chromium returned before the PDF was flushed, so files appeared missing | Fixed — waits for the file to appear and its size to settle |
| No PDF engine available (no LaTeX) | Resolved — headless Chromium via Chrome/Edge, no install required |
| Pandoc warned about a missing document title | Fixed — explicit `--metadata title` per document |
| **Diagrams silently missing from the PDF.** Pandoc resolved the relative image paths against the working directory, emitted an unresolved `<img src>` and exited 0 — the operator manual published with 11 broken images and no error | Fixed — `--resource-path`. A guard now **fails the build** if any `<img>` did not embed, so this class of silent loss cannot recur |

## 6. Final status

**GENERATED — NOT APPROVED.**

Reproduce with:

```
powershell -File docs\manual\build-manuals.ps1 -Format both
```

Then work section 3 and record the outcome here. **Do not distribute any PDF
until section 3 is complete**, and note that check 3.11 stays blocked until the
screenshots in `TechAim_Manual_Screenshot_Register.md` are captured.
