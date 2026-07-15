# 3P FINAL — Phase-D plan (plan only, no code yet)

Prerequisite: Phase C complete and verified (done — 140 checks, 0 failures).
Phase D is the report + presentation-polish phase. No controller state-machine
changes; the report consumes what Phases B/C already persist.

## 1. FinalsReportData

A finals-specific report assembly (NO qualification assumptions — no six
10-shot series, no integer-primary totals, no 60-shot completion rules):
- source: `globalMatchModel` finals rows (official + sighters), controller
  `stageStatuses()` / `stageSubtotals()` / `missingShots()` /
  `cumulativeTotal` / `commandEvents()`, and the session journal.
- sections: athlete · event (FINAL 35) · date/time · completion status ·
  cumulative decimal total · Kneeling subtotal (10 expected) · Prone subtotal
  (10 expected) · Series 1/2 subtotals (5 expected) · singles 31-35
  individually · MissingShot list rendered as "Shot N — DNS / Not fired —
  0.0 provisional" ([P1]=B) · rejected/out-of-window incident list ·
  shot-by-shot table (official number, decimal score, direction, time used in
  window) · per-stage target plots (reuse the existing plot components) ·
  stage timeline from the journal/command events.
- assembly in a small C++ helper (`src/finals/Finals3PReportData.h/.cpp`,
  QtCore-only where possible, unit-testable like Finals3PShotRecord).

## 2. FinalsReportView

Pure-content QML view (same architecture as SummaryReportView /
MatchReportView): `implicitHeight`, intent signals, `exportPdf()` owning the
grab via the existing CUSTOMPRINT/PDFEXPORT path. Decimal-only formatting.

## 3. Report-window integration

Host FinalsReportView in the existing floating Report window as a
finals-only tab (visible when the session was a final), or its own
FloatingWindow registered as "finalsReport" — decide by whichever keeps the
Report window's qualification tabs untouched. Wire the bottom bar's
finals-complete state ("VIEW REPORT") to windowManager.

## 4. Audio service choice

Implement `FinalsAudioService` behind the existing `audioCueId` events:
- Option 1 (recommended): pre-recorded WAV clips per commandType via
  QSoundEffect — deterministic, offline, no extra Qt module; ship
  clips in qrc (`sounds/finals/<cueId>.wav`), beep fallback when a clip is
  missing.
- Option 2: Qt TextToSpeech — zero assets but adds the QtSpeech module and
  per-machine voice variability (verify the MinGW kit ships it).
The state machine stays decoupled either way (design doc §Audio).

## 5. Ceremony polish

- Voice cues for AthletesToLine/TakeYourPositions via the audio service.
- Configurable intro length; athlete-name announcement (userName).
- Optional on-screen introduction card during the Full ceremony intro.

## Commit plan

D1 report data assembly + tests · D2 FinalsReportView + window integration ·
D3 audio service + clips · D4 ceremony polish + docs. Restart-recovery UI
remains deferred (journal already sufficient).
