# Coach Report — Technical Summary

Offline shooting-performance analytics for the TechAim/Seta electronic-target
app: real match shots in → a full coaching report out, shown in three views and
exportable to PDF. Everything is deterministic, offline, and needs no network,
AI, or hardware to compute.

---

## 1. Architecture

Three layers, with a strict one-way dependency (QML → bridge → engine). The
analytics engine never depends on Qt.

```
 TachusWidget (game-mode match history)
        │  getScore / getXCord / getYCord / getTime
        ▼
 CoachReportFeeder            src/bridge/coachreportfeeder.*   [QObject: COACHFEED]
        │  resolve discipline, read arrays
        ▼
 tachusshotbuilder           src/bridge/tachusshotbuilder.*   (PURE C++, Qt-free)
        │  arrays → std::vector<ShotAnalyticsData>
        ▼
 CoachAnalyticsEngine::analyze   src/analytics/*              (PURE C++, Qt-free)
        │  → CoachReportData   (the single computed object)
        ▼
 CoachReportBridge           src/bridge/coachreportbridge.*   [QObject: COACHREPORT]
        │  marshals CoachReportData ↔ QVariant (coachreportvariant.*, QtCore only)
        ▼
 QML views  (Dashboard / Detailed / Print)  ──► PdfExporter  [QObject: PDFEXPORT]
```

Context properties registered in `main.cpp`: `COACHREPORT` (bridge),
`COACHFEED` (feeder), `PDFEXPORT` (exporter) — alongside the pre-existing
`MODREADER`, `APPSETTINGS`, `CUSTOMPRINT`.

**Invariant:** the engine (`src/analytics/`) and the shot builder
(`src/bridge/tachusshotbuilder.*`) are Qt-free and compile/unit-test with plain
`g++`. All Qt lives in `src/bridge/coachreport*` + `pdfexporter.*` + QML.

---

## 2. Analytics engine (`src/analytics/`)

Pure C++ (`std::vector`/`std::string`/`<cmath>`), namespace
`techaim::analytics`, class `CoachAnalyticsEngine` (static methods). Input is
`std::vector<ShotAnalyticsData>` (score, mm coordinates, timestamp, position,
validity/sighter/competition flags); output is one `CoachReportData`.

Sections (built in `analyze()`, each honest about availability — never
fabricates values for missing coordinates/timing or low samples):

1. Executive Summary — totals, average, SD, consistency, group geometry, trend
2. Score Distribution — quality bands, streaks, histogram
3. Trend — rolling averages, halves/thirds, regression, deterioration flag
4. Heat Map — density grid + raw coordinate stats (mm)
5. Timing — intervals, rhythm, rushed/delayed, decision/recovery times
6. Position Analysis — each position analysed **independently** (never merged),
   with per-position geometry/trend/recovery/timing + cross-position comparison
7. Recovery — behaviour after poor shots; pattern classification
8. Fatigue — inferred fatigue *signal* (not a diagnosis); pattern + index
9. Training Priorities — ranked, categorical synthesis over sections 1–8
10. Coach Conclusion — grounded coach-facing text (templated from real numbers)
11. Coach Diary — manual human context (engine leaves it empty)

Coordinates are millimetres throughout; pixel/ISSF-ring conversion happens only
in QML. The structure is **frozen** — no maths changes without a confirmed bug.

**Tests (scratchpad, run with `g++`):** ~660 unit checks across
`test_coach2–10` (phases 4–13), `test_position` (70, position analysis),
`test_shotbuilder`, `test_disciplines`, `test_bridge` (marshalling +
single-source, needs Qt6Core), `test_pdf` (PDF output, headless). All green,
warning-free under `-Wall -Wextra`.

---

## 3. Single source of truth

`CoachAnalyticsEngine::analyze()` runs **exactly once per match**, inside
`CoachReportBridge::analyze()`, which stores one `m_report` (the
`CoachReportData`) and one `m_shots` (the raw shots). Every view reads the same
objects via `COACHREPORT.report` and `COACHREPORT.shots()`; no view recomputes a
metric — QML only formats.

- Both the target plots and the report counts use the identical filter
  `isValid && isCompetitionShot && !isSighter`, matching the engine's prepared
  set — so plotted shots always equal the report's `competitionShotCount`.
- Switching views is a visibility toggle (`coachViewMode`); it never triggers a
  new analysis. `analyze*` is invoked only by Flip Y, Re-run, the Match-Summary
  "Coach Report" button, and the DEMO buttons.
- Re-analysis preserves any human-entered Coach Diary.

---

## 4. Three report views

Opened from the Match Summary (or DEMO buttons); toggled by `coachViewMode` in
`main.qml`; all read the one `COACHREPORT`.

- **Dashboard** (`CoachDashboardPage.qml`, mode 0, default) — light, quick coach
  overview: ISSF numbered targets (`IssfTargetCanvas.qml`, real mm ring radii
  per discipline), score-distribution donut, trend line, by-position table, and
  a compact **Key Coach Insights** card (limiting position, fatigue level,
  recovery rating, dominant error direction, main training focus).
- **Detailed** (`CoachReportPage.qml`, mode 1) — dark, full diagnostic: KDE
  density heat map (`HeatMapCanvas.qml`), timing, and structured Recovery /
  Fatigue cards with coach-facing headings + the engine's own evidence bullets,
  plus training priorities, conclusion, diary.
- **Print** (`CoachPrintPage.qml`, mode 2) — light A4-style layout (all sections
  in order) with an **editable Coach Diary** (equipment/sight/ammunition/
  weather/wind/lighting/physical/mental/coach/athlete notes, next-focus
  checkboxes, free-text focus), saved via `COACHREPORT.setCoachDiary`.

Shared display components: `CoachReportCard.qml`, `ShotTargetCanvas.qml`,
`HeatMapCanvas.qml`, `IssfTargetCanvas.qml`.

---

## 5. PDF export path

`PdfExporter` (`src/bridge/pdfexporter.*`, context `PDFEXPORT`). From the Print
toolbar, **⤓ Export PDF**:

1. QML grabs each visible Print section to an image
   (`content.children` → `item.grabToImage` at 2× for crispness) →
   `PDFEXPORT.addSection(result.image)`.
2. `composeToFile()` lays the section images onto **A4 portrait** pages
   (`QPdfWriter`, 12 mm margins), **packing** sections and starting a new page
   only when the next section would not fit — so a page break never cuts a
   section. A section taller than one page is scaled to fit rather than split.
3. Filename `TechAim_CoachReport_<Athlete>_<Discipline>_<Date>.pdf` (sanitised,
   date hyphens kept), offered via a save dialog pre-filled in Documents; a
   "PDF exported" flash confirms.

Export lays out already-rendered images only — it triggers no analysis and uses
the currently loaded report + saved diary, so the PDF matches the Print view.
`composeToFile` is dialog-free and unit-tested headless (valid multi-page
`%PDF`, including an over-tall section).

---

## 6. Known temporary dev aids (remove after live hardware testing)

Kept intentionally to allow validation without the range hardware:

- **DEMO PRONE / DEMO 3P** buttons — `main.qml`; `COACHFEED.analyzeDemoMatch(kind)`
  → `CoachReportFeeder::makeDemoArrays` (fixed golden-angle-scatter sample data).
- **DEBUG — first-test only** card — `CoachReportPage.qml` / `CoachPrintPage.qml`;
  backed by `CoachReportFeeder::debugInfo()` and the `m_dbg*` capture fields.
- The feeder's `coordinatesFlipY` toggle + `debugInfo` are dev conveniences; the
  y-orientation default and the 3P equal-thirds position fallback
  (`tachusshotbuilder` — see its header) still need confirming on real shots.

When removing: delete the DEMO buttons/`analyzeDemoMatch`/`makeDemoArrays`, the
DEBUG cards/`debugInfo`/`m_dbg*`, and switch the report entry point to the real
Match-Summary "Coach Report" button only.
