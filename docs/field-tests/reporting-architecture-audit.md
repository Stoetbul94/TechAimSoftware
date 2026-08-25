# Reporting architecture audit — what exists, what does not

Audit only. Nothing in this document was implemented in this round; §5 of the
brief asks for the audit before the implementation, and the implementation is a
larger piece of work than it looks from the outside.

## The matrix

| Mode | Session persisted | Report model | Report view | PDF writer | Report button | Auto trigger | Status |
|---|---|---|---|---|---|---|---|
| Open Practice / Unlimited | `.tch` + journal | legacy `globalMatchModel` + `MODREADER` | `SummaryReportView`, `MatchReportView` | `createSummryPdf()`, `createPdf()` | yes | no | **WORKS** — RC3C produced `summary_report.pdf` (2 pp) and `untitled.pdf` (3 pp) at 21:00 |
| Fixed Training (Training Lab) | journal | controller | `TrainingReportView` | `createTrainingPdf(path)` | yes | no | **WORKS** |
| Call & Diagnose | journal | `CallDiagnoseController` | `CallDiagnoseReportView` | `createTrainingPdf(path)` | yes | no | **WORKS** |
| Position Transition | journal | controller | `PositionTransitionReportView` | `createTrainingPdf(path)` | yes | no | **WORKS** |
| Qualification / Match | `.tch` + journal | legacy models | `SummaryReportView`, `MatchReportView` | `createSummryPdf()`, `createPdf()` | yes | no | **WORKS** |
| **50 m 3P Final** | journal | `FINALS3P.buildReport()` | `FinalsReportView` (4 pp) | `createFinalsPdf()` | yes, via `ReportWindow.finalsMode` | no | **WORKS** |
| **10 m AR / AP Final** | journal | **none** | **none** | **none** | **falls through to the qualification tabs** | no | **MISSING** |
| Coach Report | analytics engine | `COACHREPORT` | `CoachReportWindow` | `PdfExporter` | yes | no | WORKS |

## The gap, precisely

Three independent facts, each read from source:

1. `ReportWindow.qml:21` — `finalsMode` is `shootingPage.isFinalsMatch`. That is
   the **3P** flag. `isFinals10mMatch` is not consulted, so a 10 m Final opens
   the **qualification** Summary/Match tabs, whose comment three lines above
   says those tabs "carry qualification assumptions that must never be fed
   finals data".
2. `FinalsReportView.qml:47` — every value comes from `FINALS3P.buildReport(meta)`.
   There is no other source.
3. `Finals10mController` has **no `buildReport()`**. Its `onReportRequested()`
   handler in `ShootingPage.qml` logs
   `"FINALS10M: report requested (F6 pending)"` — the gap is already known and
   tracked as F6; it is not a regression.

So the RC3C 10 m Final produced an excellent completion dialog and no report,
exactly as observed.

## What building it actually involves

Not a QML screen. The 3P finals report works because
`FinalsReportBuilder` (`src/finals/`) is a QtCore-only assembler where **every
reported value is derived**, and QML only formats. A 10 m equivalent needs the
same:

1. `Finals10mController::buildReport()` returning an immutable report map —
   identity, athlete, discipline, course, sighters separated from official
   shots, per-shot score and time, Series 1 / Series 2 / singles, the 24-shot
   checkpoints, accepted/missing counts, MPI, group extent, mean shot time,
   incidents, LIVE TARGET indication, and the single-lane caveat.
2. A report view shaped for a 24-shot course — the 3P view's 35-shot,
   three-position structure cannot represent it and must not be bent to.
3. `ReportWindow.finalsMode` extended to either finals family, choosing the
   view by discipline.
4. PDF wiring through the existing `createFinalsPdf()` path.
5. Round-trip tests: persist → reload → identical score, coordinate, time,
   timestamp.

**Estimate: a full round of its own.** Attempting it in the same round as the
physical-defect fixes would produce a half-built report and a worse gate.

## Shared vs discipline-specific — for whoever builds it

**Shared** (put in a common builder/view): identity, athlete, discipline, date,
session id, software version and build, sighters-vs-official split, shot
sequence with score and time, accepted/missing, MPI, group extent, mean shot
time, incidents, LIVE TARGET SESSION badge, single-lane caveat, footer
traceability.

**10 m only**: Series 1, Series 2, 14 singles, the 24-shot course checkpoints
(S12…S24).

**3P only**: position, position transitions, per-position series, elimination
and ranking semantics, the 35-shot course model.

**Do not** give 10 m a position field or 3P a "14 singles" row to make one
template serve both.

## Open Practice reporting — already works, wording is the open question

RC3C produced both PDFs. What was not verified is whether they are **labelled**
as practice rather than as a competition result, and whether an unlimited
session is broken into 10-shot series with a valid short final series
(27 shots → 10 / 10 / 7). Those are checks against the existing renderer, not
new infrastructure — worth doing before the SETA build, and cheap.

## Recommendation

Order: 10 m Finals report builder + view (own round) → Open Practice report
labelling and series-breakdown check → reload-consistency tests across both.
None of it should introduce a second scoring engine in the PDF layer; the
analytics engine and `calculateShootingSocre` remain the only sources.
