# Stage 5 — Wind Map controller, recovery dispatch and QML workflow

**Phase:** STAGE 5 · **Implemented on:** `cbacae6` (Stage 4.1 accepted)
**Date:** 2026-07-29
**Status:** implemented; automated evidence complete, **human visual check outstanding**

Stage 5 turns the Wind Map domain (Stages 3–4.1) into a usable capture
programme. It deliberately stops short of analytics and reporting.

---

## 1. What was built

| Area | Delivered |
|---|---|
| Controller | `src/training/WindMapController.{h,cpp}` — `WINDMAP` context property |
| Domain | `WindMapPhase` expanded to the full workflow; transition table |
| Events | `WindMapPhaseChanged` appended to the catalogue (see §2) |
| Projection | `SessionState::wmPhase`, snapshot-serialised (state v6) |
| Catalogue | Wind Map card activated in the Training Lab, 50 m rifle only |
| Setup | `LoginPage.qml` `practiceView === 6` — shot plan + sighters |
| Capture | `WindMapRightPanel.qml` — compass ring, condition entry, progress |
| Review | `WindMapHud.qml` — factual session review + completion |
| Recovery | `RecoveryCoordinator` classification, `main.qml::dispatchRecovery`, `ShootingPage.restoreWindMapSession` |
| Tests | `tst_windmap_controller.cpp` (workflow + resume), `tst_windmap_qml.cpp` (source guards) |

## 2. The Stage 5 finding: the phase had to become durable

Stage 3 declared a coarse four-value phase and **nothing carried it into the
journal**. That is not sufficient for recovery.

Consider a crash in this exact state: sighters fired, `finishSighters()`
called, **no counted shot yet**. Every derivation from the recorded shots —
"the last shot was a sighter, so we were in sighters" — resumes into the wrong
phase, and **the next shot is journalled as a sighter when it should be
counted**. That is a silent data error in the athlete's own record.

So the phase is recorded:

- `WindMapPhaseChanged { fromPhase, toPhase }`, **appended at the END** of the
  `DomainEvent` variant, so no prior variant index or journal hash moves.
- Durability `Sync` / broadcast `Broadcast` / reducer `Mutating` — Sync
  because the phase decides how the next shot is classified.
- Reduced into `SessionState::wmPhase` and serialized inside the existing
  `windMap` object as `phase`.
- The reducer enforces **consistency**, not the transition table: `fromPhase`
  must equal the folded phase (so a reordered or replayed transition is
  refused), `Completed` is terminal, and `PositionReview` is 3P-only.
  Transition **legality** is the controller's, via
  `ta::training::windMapTransitionAllowed`.

`tst_windmap_controller.cpp` case 10 is the direct proof: it crashes in that
exact state, resumes, and asserts the first shot afterwards is **counted**.

### 2.1 What the reducer deliberately does NOT do

It does not re-decide sighter vs counted from the phase. The controller
classifies; the event carries the classification; the reducer records it. That
is the same division the other three Training Lab programmes use, and it keeps
a future workflow change (a sighter mid-block, say) from requiring a reducer
change.

## 3. State version 5 → 6

The `windMap` object gained one key (`phase`). A v5 snapshot genuinely exists
(the committed golden fixture), so the version moves for the same reason it
moved at Stage 4.1 — a reader must be able to tell the two apart. The key is
read optionally; a v1–v5 snapshot restores `wmPhase = 0` (Idle), which is
correct for every snapshot that predates this workflow.

**Golden fixture:** `fixture_finals_clean.jsonl` contains a `StateSnapshot`,
so its bytes changed (7596 → 7608). Before regenerating, the committed
fixture was confirmed to still deserialize (`committed fixture readable`
PASS), replay, classify and produce the expected valid-prefix length — only
the byte-golden differed. It was then regenerated through the harness's own
`--write-fixtures` path, not by hand.

## 4. Controller contract

Phases: `0 Idle · 1 Setup · 2 Sighters · 3 CountedShots · 4 PositionReview
(3P only) · 5 SessionReview · 6 Completed`.

Legal transitions only, and **failure is closed** — an illegal transition is
refused with an operator-facing reason, never clamped to the nearest legal
phase and never silently skipped. `50 m Prone` can never enter
`PositionReview`; `Completed` is terminal.

**Conditions.** Three distinct recorded states, never collapsed:

| State | Meaning | How it is stored |
|---|---|---|
| Measured | a direction and a speed were read | `valid`, `!calm`, degrees + hundredths |
| Calm | calm was **observed** | `valid`, `calm`, no direction |
| No reading | none was taken | `!valid` — never inferred, never back-filled |

Invalid input (negative, NaN, absurd speed) is **refused**, so a bad value can
never enter the record as a silent 0° North.

**Units.** QML passes degrees and **m/s**. The conversion to the stored
hundredths happens only in `metresPerSecondToHundredths`. The raw hundredths
value is never returned to QML and never displayed.

**The immutable association.** At accept time each shot takes a **copy** of
the standing condition. Changing the standing condition afterwards does not
reach back into a recorded shot. A shot recorded with no reading keeps no
reading.

**Sighters** are recorded with their conditions and are **excluded from every
counted statistic**, as approved in the spec review.

## 5. Recovery

`RecoveryCoordinator` now surfaces `wmProgramId` in the programme-id chain, so
`trainingProgramId == "wind_map"` reaches `RecoveryDialog` → `dispatchRecovery`
→ `ShootingPage.restoreWindMapSession` → `WINDMAP.resumeFromRecovery`.

The resume **submits nothing**: no start, no condition, no shot is
re-journalled. It fails **closed** on classification — a competition journal,
another Training programme's journal, or an unsupported discipline is refused
rather than partially applied.

`recoveredMaxExternalId()` carries the duplicate guard past every recovered
shot, so a retransmitted shot cannot be accepted twice after a resume.

## 6. Scope — what Stage 5 did NOT do

Not implemented, by instruction: analytical condition comparisons, coaching
narrative, target-filter analysis view, PDF report, WeatherStation
integration, sight-click or aiming advice, official competition mode. No
scoring, no homepage redesign, no change to any other discipline.

`tst_windmap_qml.cpp` §8 asserts the absence of PDF and analytics wiring so it
cannot creep in unnoticed; §4 asserts the absence of prescriptive and causal
wording — as **phrases**, not single words, because "Tech Aim" contains "aim".

## 7. Evidence

| Harness | Result |
|---|---|
| reliability (QtCore-only) | **1815 checks, 0 failures** |
| training | 567 / 0 |
| finals10m | 143 / 0 |
| 3P finals | 233 / 0 |
| Application build | `qmake && mingw32-make -f Makefile.Release` clean |
| Launch check | `QT_FORCE_STDERR_LOGGING=1` — no Wind Map QML error or warning |

Compiling `WindMapController.cpp` into the `QT = core` reliability harness is
also the proof that the controller carries no QML/GUI dependency.

### 7.1 Visual evidence

**MANUAL-ASSISTED VISUAL CHECK REQUIRED.** Automatic screen capture remains
blocked by endpoint security on this machine (see
`docs/ui/UI_Defect_Register.md` §4); no synthetic-input or antivirus
workaround was attempted. The launch check proves the QML types resolve,
instantiate and produce no errors — it does not prove the layout reads
correctly at every window size. The following need a human at the machine:

- The compass ring at 1366×768, 1600×900, 1920×1080 and 2560×1440.
- The condition entry fields with a touch input.
- The session review table with 40+ shots (scrolling).
- The 3P position-change row.
