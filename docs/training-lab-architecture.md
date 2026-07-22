# Training Lab — Architecture & Pre-implementation Audit (T1)

Source of truth: *TechAim Training System — Refined Research Specification v0.2
(2026-07-20)*. T1 delivers one complete programme — **Technical Blocks** — as a
vertical slice. Training Lab is a **separate domain** from competition and must
never present as an official result.

## 1. Audit — what exists and how Training reuses it

| Subsystem | Location | Training decision |
|---|---|---|
| Homepage / event selection | `LoginPage.qml` | **Modify**: replace the four fixed 10/20/30/40 rows with three categories (Official ISSF Match / Open Practice / Training Lab). Official ISSF area unchanged. |
| Practice events (10/20/30/40) | `LoginPage.qml` `getGameEventText`, `ShootingPage.setCurrentGameType` | **Fold** into one **Open Practice** setup (presets + custom). No behaviour change to the underlying practice run. |
| Scoring / coordinate mapping / target geometry | `CenterPane.qml::calculateShootingSocre` | **Reuse verbatim** — Training shots flow through the SAME detection+scoring pipeline; only registration differs (as finals do). Do NOT reimplement. |
| Group / MPI / spread / timing analytics | `src/analytics/` (frozen pure C++ engine) + `CoachReportData` | **Reuse** for block metrics via a narrow adapter; do NOT recompute geometry in QML. |
| Durable journal + hash chain | `src/reliability/` `SessionStore`, `JournalWriter`, serializer, validator | **Reuse** — Training events are added to the shared `DomainEvent` catalogue and journalled through `SessionStore` (same hash chain, degraded queue, recovery). Do NOT duplicate journal hashing. |
| Reducer / replay | `SessionReducer` → `SessionState`; `ReplayEngine` | **Extend** — fold Training events into a `TrainingState` sub-struct on `SessionState` (finals pattern). Replay/recovery reused. |
| Recovery | `RecoveryCoordinator` | **Extend** — classify Training sessions distinctly (a Training `sessionKind`), so a Training session is NOT offered as an unfinished *competition* candidate. |
| Live/Demo input-source gate | `ta::mode::sourceAllowed`, `OperatingModeService` (OPMODE) | **Reuse** — the Training controller applies the same gate before `TrainingShotAccepted`; wrong source → no event, no block progress. |
| Operating-mode metadata | `SessionStarted.operatingMode` | **Reuse** — Training session stamps its start mode; authoritative for the session. |
| Report components | `ReportHeader/Footer`, `SectionTitle`, `MetricCard` | **Reuse** for a Training-specific report (own terminology). |

## 2. Isolation — what is Training-only

New domain under `src/training/` (owns ALL Training state; qualification/Final
controllers never own it):

```
src/training/
  TrainingProgramTypes.h        program id, position, visibility, focus, config + per-discipline defaults, validation
  TrainingBlockMetrics.h        pure block-metrics adapter over the analytics engine (MPI/radius/diameter/H-V spread/score SD/timing)
  TrainingProgramController.*    context property TRAINING — state machine, source gate, visibility projection, journaling, recovery, comparison
qml/training/ (registered in qml.qrc)
  TrainingLabPage.qml           discipline-aware catalogue (AVAILABLE / COMING NEXT / PLANNED)
  TechnicalBlocksSetup.qml      setup panel
  TechnicalBlocksSession.qml    active block + visibility overlay (controller-owned projection)
  TechnicalBlocksBlockReview.qml block review + one athlete note
  TechnicalBlocksSummary.qml    final block comparison
  TrainingReport.qml            Training-specific report ("Not an official competition result")
tests/training/
  training_tests.pro, tst_training.cpp
```

Shared-catalogue additions (backward-compatible, appended at the END so no
existing `std::variant` index / journal hash moves; only written when a
Training session runs, so old journals re-serialise byte-identically):

- `EventTypes.h`: 8 Training events.
- `DomainEvent.h`: append the 8 alternatives.
- `EventSerializer.cpp`: write/read each.
- `SessionState.h`: `TrainingState` sub-struct + `sessionKind`.
- `SessionReducer.cpp`: fold Training events.

## 3. Event ownership (Training-specific semantics)

`TrainingSessionStarted, TrainingBlockStarted, TrainingShotAccepted,
TrainingBlockCompleted, TrainingNoteSaved, TrainingNextBlockStarted,
TrainingCompleted, TrainingSessionClosed`.

A Training shot reuses measured shot data (x, y, scoreTenths, timestamp,
source, discipline, position) but is journalled as a **Training** shot — never
as `ShotAccepted`/`SighterAccepted`. `sessionKind = Training` and
`programId = technical_blocks` distinguish it everywhere.

## 4. Recovery ownership

Training recovery is classified separately: `RecoveryCandidate` carries the
`sessionKind`; a Training session is offered only as a Training resume and
never opens a competition firing window. Duplicate-lifecycle guards
(`TrainingBlockCompleted`/`TrainingCompleted` once) mirror the finals
completion guards.

## 5. Report ownership

Own report view + terminology ("TRAINING SESSION", "Not an official
competition result"). Never "Match completed / Final / Official score /
Ranking / Placement / Medal / Elimination".

## 6. Future interfaces (defined, NOT implemented in T1)

- **Call & Diagnose (T2)**: `TrainingShotAccepted` reserves an optional
  `calledX/calledY` (athlete shot-call) — unused in T1.
- **Group Pattern Coach**: block metrics + per-shot coordinates are stored so a
  future pattern classifier can run **after a completed block or ≥5 valid
  shots**. T1 stores the data and defines the pattern taxonomy in comments; it
  ships **no diagnostic engine** and never diagnoses from one shot.

## 7. Must NOT change (accepted competition behaviour)
Qualification/Final/3P rules & scoring, target geometry, incident authority,
competition recovery semantics, Live/Demo semantics, build identity, command
flash, Final completion workflow, operating-mode selector, comms protocol,
production audio, RMS contract.
