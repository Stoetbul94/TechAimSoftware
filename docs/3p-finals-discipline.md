# 50m Rifle 3P FINAL — workflow map and roadmap (design doc)

Single-athlete **Finals Training Mode** for one TechAim lane unit, implementing
the 50m Rifle 3-Positions Final per the **ISSF Rule Book 2026 Edition 2025,
Second Print 07/2026, effective 1 July 2026**, as a training simulation.

**Everything here is gated on `isFinalsMatch` and is a fully separate domain**
from 3P qualification (`is3PMatch`, `docs/3p-discipline.md`), 50m Prone, the
air disciplines, and the future RMS multi-athlete final. Finals is NOT
implemented by scattering if-statements through qualification code: it has a
dedicated controller (below). Approved for implementation 2026-07-12 with the
clarifications integrated below. [P1] is RESOLVED as **Option B** (missing
expected shots become MissingShot records rendered as DNS/0.0 provisional in
the report layer only; detected-shot models never receive synthetic entries;
`fillUnfiredShotsWithZero` stays false).

## Architecture: dedicated finals controller

```
src/finals/
    Finals3PController.h/.cpp   — QObject state machine, single time authority
    Finals3PTypes.h             — stage/window/command enums, event structs
    Finals3PConfig.h            — configuration (ceremonyMode, flags, timeScale)
```

Exposed to QML as the `FINALS3P` context property. The shooting page **binds**
to the controller; it never owns finals timing logic.

Main API: `startFinal() / skipCeremony() / advanceStage1() /
confirmStage1Advance() / cancelStage1Advance() /
registerShot(xMm, yMm, decimalScore, externalShotId) / abortFinal() /
resetFinal() / pauseTrainingSimulation() / resumeTrainingSimulation()`.
Stage-1 position transitions use the ONE stage-aware `advanceStage1()` action
(Decision 1) — the controller owns legality; there is no public SIGHT/MATCH
toggle. Signals: `phaseChanged / commandIssued / countdownChanged /
shotAccepted / shotRejected / missingShotRecorded / totalsChanged /
advanceConfirmationRequired / transitionRejected / stageCompleted /
finalCompleted`.

### Timer architecture (single time authority)

No web of loosely coordinated QML Timers. The controller owns one **monotonic
clock** (`QElapsedTimer`); a 50–100 ms UI tick recomputes
`remaining = duration − monotonicElapsed` — never a once-per-second integer
decrement (UI stalls must not drift the clock). Exposed: `remainingMs`,
`remainingFormatted`, `elapsedMs`, `isFiringWindowOpen`.
Developer-only accelerated mode `timeScale` (1.0 normal, e.g. 60.0 for tests)
— never exposed in production UI.
RMS hooks so a server can later replace the local time authority:
`startPhaseFromServer(cmd) / stopPhaseFromServer(cmd) /
synchroniseClock(serverTimestamp, offset) / applyAuthoritativeState(snapshot)`.

### Command events

Every CRO command is a **structured event**, not just text:
`FinalsCommandEvent { commandId, commandType, text, issuedAt, effectiveAt,
stage, sequenceNumber, audioCueId }`, with command types
`AthletesToLine, TakeYourPositions, PreparationSightingStart, ThirtySeconds,
Stop, StageOneAnnouncement, MatchFiringStart, FiveMinutes, LoadSeries,
StartSeries, LoadSingle, StartSingle, Unload, ResultsFinal` — essential for
RMS integration and audio playback.

### Audio

Phase A baseline: on-screen command text + system beep + structured
`audioCueId` emitted. The state machine is **decoupled from any voice engine**
via `FinalsAudioService { playCue(cueId), stop(), isAvailable }`; Phase D may
implement prerecorded WAV clips or Qt TextToSpeech. The machine works
identically without voice.

## Scope decisions (locked)

| # | Decision |
|---|---|
| 1 | Single athlete, one unit, **full 35-shot program**. No ranking, elimination, tie-breaking or penalties — RMS territory. Elimination points are announced as **information only**. |
| 2 | The **app plays the CRO**: auto-running timers issue the commands with voice/beep cues. |
| 3 | Ceremony simulated, configurable `ceremonyMode: Full | Short | Skip`. |
| 4 | Stage 1: the **athlete switches the target Match↔Sighter on screen**; legal transitions enforced; the software never auto-switches prone/standing sighting back to MATCH without athlete confirmation. |
| 5 | **Decimal scoring only**, start-from-zero, cumulative. |
| 6 | Penalties out of scope until asked. |
| 7 | Entry point: **"FINAL (35)"** event card in the 50m Rifle → 3 Positions flow. |
| 8 | Report: finals-specific data model (below). |
| 9 | Large dedicated shot-time countdown always visible during timed fire; voice counter later. |
| 10 | `stopSeriesWhenAthleteCompletes: true` for training (a real CRO waits for all finalists; the RMS server will own that later). |

## The 35-shot program (shot accounting)

| Shots | Stage | Time authority |
|---|---|---|
| 1–10 | Kneeling match | shared 22:00 Stage-1 clock, athlete-paced |
| 11–20 | Prone match | same 22:00 clock |
| 21–25 | Standing Series 1 | 250 s, on command |
| 26–30 | Standing Series 2 | 250 s, on command |
| 31–35 | Standing Singles 1–5 | 50 s each, on command |

Official numbering is continuous 1–35; **sighting shots never consume official
shot numbers**. Stage enum:

```
FinalsStage { Ceremony, KneelingPrepSight, KneelingMatch, ProneSighting,
              ProneMatch, StandingSighting, StandingSeries1, StandingSeries2,
              StandingSingle1..StandingSingle5, Complete, Aborted }
```

Each accepted match shot carries: `finalShotNumber, stage, position,
seriesIndex, shotWithinStage, decimalScore, xMm, yMm, timestamp,
phaseStartTimestamp, timeUsedInWindow, commandWindowId, targetModeAtReceipt,
accepted/rejected status`. Model roles are finals-specific
(`finalsStageId, finalsStageLabel, finalsShotNumber, finalsWindowId`) — not
overloaded onto the qualification `position` role. Because ListModel role sets
lock at first append, **every finals role is declared in the first inserted
record**.

## State machine

```
IDLE ── startFinal()
CEREMONY (ceremonyMode: Full = intro + hold · Short = hold only · Skip = none)
 ├─ "ATHLETES TO THE LINE", athlete introduction        [Full]
 ├─ "TAKE YOUR POSITIONS" → 30 s kneeling hold          [Full/Short]
 │    holding/aiming allowed; dry firing and safety-flag removal PROHIBITED
 │    (the 30 s hold is NOT part of the 5-minute timer)
KNEELING PREP+SIGHT — 5:00, window SightingOpen
 ├─ "FIVE MINUTES PREPARATION AND SIGHTING TIME…START"
 ├─ 4:30 → "30 SECONDS" · 5:00 → "STOP", window Closed
 └─ software auto-switches target to MATCH (= EST Technical Officer role).
     This is the ONLY automatic Match switch in the final.
STAGE 1 — one continuous 22:00 clock (never paused/restarted by changeovers)
 ├─ "FINALISTS HAVE TWENTY-TWO MINUTES…" → +5 s → "MATCH FIRING START"
 ├─ KneelingMatch  shots 1–10  (max 10, window MatchOpen)
 ├─ athlete: flags in, change to prone, taps SIGHT → ProneSighting (unlimited)
 ├─ athlete taps MATCH → ProneMatch  shots 11–20 (max 10)
 ├─ athlete: change to standing, taps SIGHT → StandingSighting (unlimited,
 │    in whatever time remains)
 ├─ elapsed 17:00 → "FIVE MINUTES" · 21:30 → "THIRTY SECONDS"
 └─ 22:00 → "STOP", window Closed. Standing target must be in MATCH before
     the first commanded series (athlete confirms; prompt if still SIGHT).
STANDING SERIES n = 1, 2
 ├─ gap/announcer period → "FOR THE NEXT COMPETITION SERIES…LOAD" → 5 s →
 │   "START" → window MatchOpen, 250 s countdown
 ├─ close on 5 accepted shots (stopSeriesWhenAthleteCompletes) or 250 s
 └─ "STOP", stage completion stored. After series 2: info notice
     "8th and 7th places would be eliminated here".
STANDING SINGLES k = 31…35
 ├─ "FOR THE NEXT COMPETITION SHOT…LOAD" → 5 s → "START" → one-shot window,
 │   50 s countdown, always-visible
 ├─ close on 1 accepted shot or 50 s → "STOP"
 └─ info notices: after 31 → 6th · 32 → 5th · 33 → 4th · 34 → bronze decided ·
     35 → silver and gold decided (informational only — no rank assumptions)
COMPLETE — "STOP…UNLOAD" → "RESULTS ARE FINAL" → finals report available
```

### Firing-window gating

Explicit window states `Closed | SightingOpen | MatchOpen`:
- **SightingOpen** → append to `globalSlighterModel`; official total unaffected.
- **MatchOpen** → validate stage limit, append to `globalMatchModel`, update
  cumulative decimal total.
- **Closed** → reject official registration, log, show
  "SHOT OUTSIDE FIRING WINDOW".
A unique local shot/event id guards against the same raw event being appended
twice on hardware retry.

### Extra-shot blocking

Hard limits: Kneeling 10 · Prone 10 · each series 5 · each single window 1.
An extra shot is **never silently ignored**: it is excluded from the official
model, logged as `ShotRejected { reason: StageShotLimitReached, stage,
rawShot, timestamp }`, surfaced as a clear warning, with raw coordinates
retained for diagnostics.

### [P1 — RESOLVED: Option B] Unfired shots at window expiry

`fillUnfiredShotsWithZero: false` (default, and Phase B ships with it off).
When a timed stage expires: close the window, preserve the actual shots fired,
mark the stage incomplete, and record `MissingShot { expectedShotNumber,
stage, reason: TimeExpired }` — **no synthetic shot records are inserted into
`globalMatchModel`**. The report may display
"Shot 24 — DNS / Not fired — 0.0 provisional", but the raw model always
distinguishes a missing shot from a detected one. Final behaviour to be
confirmed against the rule interpretation and desired training feel.

## Display rules

Finals UI is clearly distinct from qualification: **FINAL 35** event label ·
decimal-only scores · continuous shot number 1–35 · current stage + position ·
target mode (SIGHTER/MATCH) · command banner · large remaining-time display ·
firing-window status · stage subtotal · cumulative total · phase stepper:

```
PREP · KNEEL · PRONE · STAND SIGHT · S1 · S2 · 31 · 32 · 33 · 34 · 35 · DONE
```

Standing singles 31–35 are **normal commanded final shots** — the label
"SHOOT-OFF" is reserved for future tie-breaking shots (RMS phase) and must not
be used for the single-shot progression.

## Report

`FinalsReportData` (finals-specific — none of the qualification assumptions:
no six 10-shot series, no integer-primary totals, no qualification position
grouping, no 60-shot completion rules; visual components may be reused):
athlete · event · date/time · completion status · cumulative decimal total ·
Kneeling subtotal (10 expected) · Prone subtotal (10 expected) · Series 1 and
Series 2 subtotals (5 expected each) · singles 31–35 individually · missing
shots · rejected/out-of-window incidents · shot-by-shot time used · target
plots · stage timeline.

## Persistence and recovery (lands with Phase B)

After each accepted shot and phase transition, persist: `isFinalsMatch`,
current stage, stage start time, accepted shots, sighting shots, command
sequence, target mode, cumulative total, missing-shot records, completion
status. On restart: offer **Resume Final** or explicitly mark the interrupted
final abandoned — never silently restart at zero while retaining old shots.

## Roadmap

| Phase | Deliverable |
|---|---|
| **A** | Skeleton only — **no shot scoring changes**: FINAL (35) event entry, finals session flag, dedicated controller, complete state machine, phase transitions, countdown framework, command events, command banner, phase stepper, beep fallback, dry-run controls, abort/reset, RMS `FromServer` hooks, developer `timeScale`. Tests verify: state sequence, command order, warning timing, 5-s LOAD→START delay, 22-min warnings, two 250-s series, five 50-s singles, completion, no qualification-timer usage, no shot totals affected. |
| **B** | Stage-1 shot handling: window gating into the models, 10+10 enforcement, SIGHT/MATCH legality, decimal totals, persistence/recovery, [P1] left disabled. |
| **C** | Standing series + singles shot handling with the LOAD/START/STOP cadence. |
| **D** | Finals report + audio implementation (WAV clips or TextToSpeech behind FinalsAudioService) + ceremony polish. |
| **E** | RMS (separate software): ranking, eliminations, tie-breaking, penalties, range-wide control. |

Phase A commit plan on `feature/3p-finals` (no Phase B work mixed in):
**A1** finals types, controller and configuration · **A2** event entry and
finals UI skeleton · **A3** timers, commands, stepper and accelerated test
mode · **A4** Phase-A test suite and documentation update.

## Phase A — IMPLEMENTED (commits A1 `07b8527`, A2+A3 `c707611`, A4)

What exists and is verified:

- `src/finals/` controller domain registered as `FINALS3P`; the **FINAL (35)**
  event card appears only in the 50m Rifle → 3 Positions flow; selecting it
  hands the session to the finals controller — the qualification prep/sighting
  machinery and timers never run (`isFinalsMatch` forces `is3PMatch` false).
- Finals panel on the shooting page: stepper, command banner, large countdown,
  window/target-mode status, next official shot number, and dry-run controls
  (SIGHT/MATCH, SKIP CEREMONY, PAUSE/RESUME, ABORT, RESET). In demo mode a
  click on the target drives `FINALS3P.simulateShot()` — the real acceptance
  path with **no scoring and no model writes** (every Phase-A shot event is
  flagged `simulated`; structurally the controller has no access to
  `globalMatchModel`/`globalSlighterModel` or any qualification timer).
- **Acceptance test suite: `tests/finals/` — 70 checks, 0 failures.**
  Standalone console harness (`qmake finals_tests.pro`, run with the Qt bin
  directory on PATH). Runs the controller end-to-end at
  `TECHAIM_FINALS_TIMESCALE=60` and asserts: the exact command-event ordering
  (all 39 commands from ATHLETES TO THE LINE to RESULTS ARE FINAL) and exact
  stage-entry ordering; ceremony Full/Short/Skip; the 30 s hold separate from
  the 5:00 prep timer; prep 4:30 warning; auto-MATCH only at prep end; the
  continuous 22:00 Stage-1 clock with 17:00/21:30 warnings; athlete
  SIGHT/MATCH legality (legal and illegal transitions); both 250 s series and
  all five 50 s singles with the 5 s LOAD→START delay each; continuous
  numbering 1–35 with sighters unnumbered; per-stage limits + rejection events
  (11th shot, out-of-window); elimination info notices in sequence; final
  STOP→UNLOAD→RESULTS ARE FINAL order; pause freezing remaining time; abort →
  Aborted; reset → Idle; repeated/invalid commands harmless; RMS hook stubs
  callable without state damage; no synthetic shots at window expiry
  ([P1] disabled).
- Timing assertions use a tolerance of a few controller ticks (50 ms real ×
  timeScale), since scaled time advances in tick-sized steps; ordering
  assertions are exact.

Phase B starts from here: window gating into the real shot models, decimal
totals, persistence/recovery — [P1] to be reconfirmed first.
**Phase-B implementation plan: `docs/3p-finals-phase-b-plan.md`** (approved
plan required before Phase-B code; the Stage-1 transition mechanism and [P1]
are the two review gates).

## Phase A — implementation summary (approved)

| Item | Implemented as |
|---|---|
| Files added | `src/finals/Finals3PTypes.h`, `Finals3PConfig.h`, `Finals3PController.h/.cpp`; `tests/finals/finals_tests.pro`, `tst_finals3p.cpp`; QML surface in `LoginPage.qml` (FINAL (35) card), `main.qml` (routing + `FINALS3P` registration in `main.cpp`), `ShootingPage.qml` (finals panel), `CenterPane.qml` (dry-run click gate) |
| Controller API | `startFinal / skipCeremony / setTargetMode / registerShot / simulateShot / abortFinal / resetFinal / pauseTrainingSimulation / resumeTrainingSimulation / commandEvents / stepLabels` + RMS stubs `startPhaseFromServer / stopPhaseFromServer / synchroniseClock / applyAuthoritativeState` |
| State enum | `FinalsStage { Idle, Ceremony, KneelingPrepSight, KneelingMatch, ProneSighting, ProneMatch, StandingSighting, StandingSeries1, StandingSeries2, StandingSingle1..5, Complete, Aborted }` |
| Command-event enum | `CommandType { AthletesToLine, TakeYourPositions, PreparationSightingStart, ThirtySeconds, Stop, StageOneAnnouncement, MatchFiringStart, FiveMinutes, LoadSeries, StartSeries, LoadSingle, StartSingle, Unload, ResultsFinal, InfoNotice }` — every command emitted as a structured map (id, type, text, issuedAt, stage, sequence, audioCueId) |
| Timer authority | One monotonic `QElapsedTimer`, pause-aware, `timeScale`-scaled; 50 ms tick recomputes `remaining = duration − monotonicElapsed`; the 22:00 Stage-1 clock is a single anchor shared by all Stage-1 sub-stages |
| Target-mode rules | Athlete `setTargetMode` legal only for K_MATCH→P_SIGHT, P_SIGHT→P_MATCH, P_MATCH→S_SIGHT, S_SIGHT→ready-MATCH; illegal changes ignored + logged; the ONLY automatic MATCH switch is the EST-officer reset at prep end |
| Firing-window rules | `Closed / SightingOpen / MatchOpen`; shots only accepted in open windows; per-stage limits (10/10/5/5/1) reject with `StageShotLimitReached`; single windows close on their 1 shot, series on the 5th (config `stopSeriesWhenAthleteCompletes`) |
| Accelerated test mode | `timeScale` (Idle-only setter; env `TECHAIM_FINALS_TIMESCALE`); never in production UI |
| Tests | `tests/finals/` — **70 checks, 0 failures**, exact command-event and stage-entry ordering plus timing (tolerance = a few 50 ms ticks × timeScale) |
| Deferred | [P1] unfired-shot behaviour (disabled); FinalsAudioService voice (beep baseline only); persistence/recovery; all shot-model writes and scoring (Phase B); ranking/elimination/ties/penalties (RMS) |
| Commits | `55b448a` doc corrections · `07b8527` A1 types/controller/config · `c707611` A2+A3 event entry + UI skeleton · `8ac7365` A4 test suite + docs |

## Finals HUD (Option A — approved, implemented HUD1–HUD4)

Progressive-disclosure presentation layer replacing the Phase-A/B panel; pure
QML over `FINALS3P` (controller/router/models/journal/tests untouched).
Components (each takes `ctl`, mock-injectable): `FinalsHud` (shell) ·
`FinalsTopStrip` (only permanent element, ~6% height clamp 38–46 px: stage
identity · compact progress · countdown + window LED; collapses <640 px) ·
`FinalsProgressIndicator` (PREP|K|P|S|S1|S2|SINGLES|DONE, singles expand to
31–35 in phase; green done / maroon current / grey future / amber "!" missing
/ red aborted) · `FinalsCommandOverlay` (transient LOAD/START/STOP/UNLOAD/
RESULTS ARE FINAL etc., fade-in→hold ~2 s (4.5 s results)→fade-out, single
instance restarts cleanly) · `FinalsPerformanceBlock` (bottom-right, official
fire only: shots n/N, STAGE, TOTAL — decimal) · `FinalsAdvanceControl`
(contextual pill from `advanceLabel`; inline non-modal confirmation) ·
`FinalsIncidentToast` (slide/fade, severity edge bar) ·
`FinalsDeveloperDrawer` (SKIP CEREMONY/PAUSE/ABORT/RESET behind
`APPSETTINGS.getDeveloperMode()`, config.ini `developer_mode=1`, default OFF).
State harness + screenshot capture: `tests/finals/hud_harness.qml` (mock
controller, saves `docs/img/finals-hud-*.png`: prep, kneeling match, series
START, single with incident toast).

## Correctness/workflow fix phase (FIX1-FIX5, post-HUD live test)

**Part-1 audit result (one demo click, end to end):** click px -> mm in
`CenterPane.onClicked` (centre-origin, +x right, +y high; mm-per-px from the
face size / `shootingMianRect`) -> `uxShoot(mm)` (50m passthrough) -> backend
list -> `getXCord/getYCord(count)` -> `calculateShootingSocre` (shared engine;
sets `lastShotXmm/Ymm`, emits `pointAddedToSeries(angle, radius, score)`) ->
finals branch -> `registerShot(xmm, ymm, score, seq)` -> accepted record ->
router -> models -> display. The mm record was always correct; the DISPLAY was
wrong because the target overlay plots via the POLAR roles
(`mapToPosition(direction, score)`) and the finals record carried the decimal
score in `score` instead of the engine's polar radius.

Fixes (each finals-gated; qualification untouched):
- **FIX1 `825ebf1`** — router injects the engine's polar radius into `score`
  (shots now land exactly where clicked, same pipeline demo/live; xmm/ymm stay
  the canonical mm record, centre 0,0/+x right/+y high). Duplicate identity
  redefined: a session-scoped strictly-increasing detection sequence
  (`finalsShotSeq`, one increment per processed detection event) replaces
  `MODREADER.getShootCount()` (which does not advance on demo clicks and
  caused false DuplicateShot). The controller's per-window guard still rejects
  replayed ids; a new firing window resets the guard. Finals session start
  resets `backEndShootCount`/`sligterMode`/`matchFinished`/right-panel state.
- **FIX2 `869846b`** — HARD BLOCK (user decision, supersedes the Phase-B
  confirmation flow): K/P Match cannot advance below 10 accepted shots
  (`transitionRejected: StageIncomplete`); `confirmStage1Advance()` inert;
  developer bypass `devForceAdvanceStage1()` (drawer FORCE ADVANCE).
  StandingSighting has no manual action — the controller sets Match/Ready at
  the 22:00 STOP. Bottom-bar API: `primaryActionVisible/Enabled/Label` +
  `executePrimaryAction()`. Completion UX: stage label flips to
  `KNEELING/PRONE · COMPLETE` on shot 10/20; post-completion shots reject with
  athlete wording `KNEELING COMPLETE — CHANGE TO PRONE` (green guidance
  toast); `STAGE SHOT LIMIT REACHED` stays diagnostic-only.
- **FIX3 `a669112`** — qualification match clock, sighting countdown, and
  sighter-corner indicator hidden in finals; the HUD strip
  (`FINALS3P.remainingFormatted`) is the only time source. Face filtering:
  clean face per firing window (already in place); **singles = option A**
  (each single window starts clean; history stays in the right panel/models).
- **FIX4 `f05d7c4`** — right panel fed from the SAME accepted records
  (`finalsOnShotAccepted`): SN column shows official 1-35 (sighters "S",
  excluded from totals), TOTAL card shows controller cumulative + stage
  subtotal, S1-S6 qualification grouping hidden, series header names the
  finals group (K/P/S1/S2/SINGLES/DONE). Dev-mode single-source assertions:
  official rows + sum in `globalMatchModel` == `officialShotCount` /
  `cumulativeTotal`. Bottom action bar repurposed finals-only.
- **FIX5** — suite extended to **104 checks, 0 failures**: hard block,
  primary-action states, completion labels/wording, duplicate identity
  (replay rejected without altering totals; same coords + new id accepted;
  window-scoped guard reset), xmm/ymm passthrough exactness, auto Match/Ready
  at STOP, officialShotCount, unlimited standing sighters.

**Live validation (user, Part 12):** start FINAL 35 -> three known clicks land
exactly where clicked -> 10 kneeling shots (right panel rows 1-10, totals
match HUD; bar flips to CHANGE TO PRONE — SIGHTERS; 11th shot rejected with
completion wording, totals unchanged) -> prone sighters (marked S, excluded)
-> START PRONE MATCH (face clears) -> shots 11-20 -> CHANGE TO STANDING —
SIGHTERS -> unlimited standing sighters on the continuous 22:00 clock, no
qualification clock/START MATCH interference -> at STOP the target goes
Match/Ready for Series 1.

## Phase C — implemented (commits C1 `22277c0` · C2 `ae422ef` · C3 `31aca14` · C4)

Standing series/singles command cycles, 250 s/50 s windows, exact numbering
(21-25 / 26-30 / 31-35), MissingShot records, elimination info events and
per-window duplicate reset were already implemented and test-covered in
Phases A/B and the FIX phase; Phase C added the persistence and verification
the spec requires:

- **Stage status** per official stage (NotStarted/InProgress/Complete/
  Incomplete/Aborted), persisted in the controller (`stageStatus(id)` /
  `stageStatuses()` + `stageStatusChanged`), journalled, never downgraded
  once finished, and never inferred from the current stage after a
  transition. Complete lands the moment the limit is reached; Incomplete
  accompanies the MissingShot records at expiry; Aborted marks a final
  aborted mid-stage.
- **Per-stage decimal subtotals** persisted (`stageSubtotals()`) — accepted
  official shots only; the Phase-D report's data source.
- **`stageShotCapacity`** property: the controller owns expected-shot
  capacity; the HUD performance block formats `n / N` from it and gained a
  NEXT-official-shot row (series/stage-1 fire only).
- **Journal** now covers, in controller order: every CRO command,
  windowOpened/windowClosed (windowId), accepted shots, rejected shots,
  MissingShot, stageStatus verdicts, stage entries, final start/completion.
- **Tests: 140 checks, 0 failures** — completion-driven (full 35, all stages
  Complete, no MissingShots) AND timeout-driven (stage-1 expiry verdicts,
  series-1 expiry with MissingShot 23-25/TimeExpired, single-31/33-35
  expiries, progression to Complete only after stage 35, STOP once per
  window (9), ending STOP→UNLOAD→RESULTS ARE FINAL, cumulative exactness,
  journal command order == controller order).

**Schema note (flagged):** the Phase-C brief mentioned `finalsSeriesIndex`
1/2 for the standing series; the approved Phase-B values (0=K, 1=P, 2=S1,
3=S2, 4-8=singles) are retained — renumbering is a one-line change if wanted.

**Right panel:** the accepted FIX4 architecture is unchanged (same accepted
records, SN from `finalsShotNumber`, sighters "S", controller totals, finals
group header, no qualification S1-S6 assumptions). Stage completion verdicts
surface via the HUD progress indicator (green/amber) and the status API/
journal; no second shot store exists.

## Phase D1 — Finals report data model (implemented)

`src/finals/FinalsReportData.h` + `FinalsReportBuilder.h/.cpp` (QtCore only,
unit-testable): the finals-specific report assembler from the Report section
above. No qualification `SummaryReport`/`MatchReport` assumptions.

- **Immutable output**: `FinalsReportData { summary; stages[9]; officialShots
  (35 rows, real + provisional, ordered 1-35); missingShots; incidents;
  timeline; completionStatus }` — rebuilt from the same stored state, the
  result is byte-identical (asserted in tests).
- **[P1 = Option B] at the report layer**: MissingShot records become
  provisional `0.0` DNS rows merged into the 1-35 shot table (flagged
  `provisional: true`); real detected rows are never synthesised.
- **Controller is the only source** (`Finals3PController`): it now retains
  the pre-router accepted official records (`officialShotRecords()`),
  rejection records (`rejectionRecords()`), and `sighterCount()` — cleared
  in `startFinal()`, surviving `Complete` so the report outlives the final.
  The retained copies keep the decimal `score` and real `direction`; the QML
  router's polar-display overrides (`score` = polar radius) never reach them.
- **`registerShot` gained an optional 5th arg** `direction` (default 0.0);
  the ShootingPage finals branch passes the engine angle so controller-side
  records carry the true direction. Existing call compatibility unchanged.
- **QML entry point**: `FINALS3P.buildReport(meta)` returns the QVariant
  report (`summary` / `stages` / `shots` / `incidents` / `timeline` /
  `completionStatus`); `meta` may carry athlete/eventName/dateTime.
- **completionStatus verdict**: ABORTED if any stage Aborted, else INCOMPLETE
  if any Incomplete, else COMPLETE when all nine official stages are
  Complete (else IN PROGRESS).
- **`finalsSeriesIndex` schema locked** (0=K, 1=P, 2=S1, 3=S2, 4-8=singles) —
  carried through to report rows unchanged; UI may display human-facing
  series numbering independently.
- **Tests: 155 checks, 0 failures** — adds D1 assembly checks on both the
  completion-driven run (35 real rows, 9× Complete, timeline == command
  history, meta passthrough) and the timeout-driven run (11 real + 24
  provisional DNS rows ordered 1-35, per-stage fired/subtotal/status,
  incidents == rejections, rebuild-identity).

## Phase D2 + D3 — Finals report view, window integration, PDF (implemented)

**FinalsReportView.qml (D2)** — pure-content view in the Tech Aim Report
System style (ReportHeader/SectionTitle/MetricCard/ReportFooter), fed
exclusively by `FINALS3P.buildReport()`. Three A4 pages: (1) result summary
+ completion banner + stage breakdown (decimal subtotals, persisted status
verdicts, TOTAL row); (2) shot-by-shot 1-35 with provisional DNS rows
highlighted amber ("DNS — not fired (0.0 provisional)"); (3) incidents +
command timeline (row caps stated on the page and pointed at
`finals_session.jsonl`, never silent). Decimal-only. Human-facing stage
names (Standing Series 1/2, Standing Single 31-35) are mapped from the
LOCKED `finalsSeriesIndex` schema for display only.

**Report-window integration (D3)** — the floating Report window gains a
Finals tab (tab id 2). In finals mode the Summary/Match tabs are hidden
entirely (they carry qualification assumptions and must never be fed finals
data) and `prepare()` pins the window to the Finals tab whatever was
requested; the Coach Report footer button hides on the finals tab.
`windowManager.openFinalsReport()` is the intent. The report is rebuilt
from controller state on every open (`onAboutToOpen`/`onTabChanged` →
`finalsView.refresh()`).

**VIEW REPORT (D3)** — after RESULTS ARE FINAL the controller's contextual
primary action becomes `VIEW REPORT →` (visible+enabled in
`Stage::Complete`); `executePrimaryAction()` emits the new
`reportRequested()` signal and the ShootingPage router opens the finals
report tab. The controller stays UI-independent. The kiosk `onPrintPDF`
auto-export is finals-gated: in finals mode it opens the finals report
instead of driving the qualification Match auto-export.

**Finals PDF (D3)** — `CUSTOMPRINT.createFinalsPdf()`: same writer
behaviour as the redesigned summary path (each grabbed A4 page fitted
aspect-true and centred, one image per PDF page), default filename
`finals_report.pdf`, cancel-safe, emits `saveComplete()`. The view's
`exportPdf()` refreshes the report, grabs the three pages and calls it —
the existing grabToImage → CUSTOMPRINT path, no qualification writer
touched.

**Tests: 157 checks, 0 failures** — adds the D3 completion-action checks
(VIEW REPORT label/visibility/enabled after Complete; executePrimaryAction
emits reportRequested exactly once and leaves the state machine intact).
