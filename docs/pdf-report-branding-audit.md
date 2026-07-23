# PDF report branding audit (T3.1)

Inventory of every user-facing PDF/export path reachable in the Release app,
the logo asset/scale each uses, and its migration status. All reports render on
white A4 pages and use the approved light-background wordmark
`images/logo/techaim_color.png` (3163×973 px, aspect ≈ 3.251:1). No unrelated
organisation logo is used anywhere.

Target header logo scale (T3.1): ~24–31 % of the printable page width. The A4
page views are 794 px wide with 40 px margins (~714 px printable), so a header
logo **height 60 px** → width ≈ 195 px ≈ **24.6 %** of the page (≈ 2.5× the old
size). Standardised via `PdfBrandingMetrics.js` (`logoHeaderHeight`,
`logoFooterHeight`, `logoWidth()`), imported by every report chrome.

| Report | Component | Export entry | Logo asset | Old header logo | Page numbers | Shared chrome | Status |
|---|---|---|---|---|---|---|---|
| Technical Blocks | `TrainingReportView.qml` | `ShootingPage.exportTrainingPdf` → `CUSTOMPRINT.createTrainingPdf` | techaim_color | h24 (footer h12) | yes (Page X of Y) | own chrome | **enlarged** → `logoHeaderHeight` |
| Call & Diagnose | `CallDiagnoseReportView.qml` | `ShootingPage.exportCallDiagnosePdf` → `createTrainingPdf` | techaim_color | h24 (footer h12) | yes | own chrome | **enlarged** |
| Match (qualification) | `MatchReportView.qml` | `CUSTOMPRINT.createPdf` | techaim_color | via `ReportHeader` h26 | via `ReportFooter` | shared `ReportHeader/Footer` | **enlarged** (shared) |
| Match summary | `SummaryReportView.qml` | `CUSTOMPRINT.createSummryPdf` | techaim_color | via `ReportHeader` h26 | via `ReportFooter` | shared | **enlarged** (shared) |
| 10m Final | `FinalsReportView.qml` | `CUSTOMPRINT.createFinalsPdf` | techaim_color | h30 (own header row) | own | partial | **enlarged** |
| 3P Final | `Report3P.qml` / `Report3PSeries.qml` / `PdfSeriesPage.qml` | finals report grab path | techaim_color | h40 / h32 / h20 | own | own | **enlarged** |
| Coach report | `CoachPrintView.qml` | `PDFEXPORT`/`PdfExporter` grab | techaim_color | via `ReportHeader` h26 | via `ReportFooter` | shared | **enlarged** (shared) |
| Qualification pages | `PdfPage.qml` | match report grab | techaim_color | via `ReportHeader` | via `ReportFooter` | shared | **enlarged** (shared) |

Shared components `ReportHeader.qml` / `ReportFooter.qml` cover Match, Summary,
Coach and the qualification pages, so a single change there enlarges all of
them consistently. The standalone Training / Call & Diagnose / Finals / 3P
views are updated to the same `logoHeaderHeight` constant.

Metadata: `createTrainingPdf` sets Title/Creator + an XMP packet (Tech Aim,
Electronic Target Training Analysis) with no filesystem paths. The competition
creators (`createPdf` / `createSummryPdf` / `createFinalsPdf`) gain
Title/Creator too (no Debug/Demo/dev paths in metadata; Demo status stays in
the visible report body via the operating-mode field).

Not modified: no test-only or obsolete report prototypes are reachable in the
Release UI; none were changed.
