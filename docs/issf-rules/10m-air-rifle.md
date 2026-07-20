# 10 m Air Rifle — Qualification

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for the shared EST
interruption rules referenced below.

## Document status

- **Implementation status:** ✅ Persistence (B1) · ✅ Reducer/replay (Phase C:
  `TimerStarted` anchors, frozen-remaining derivation) · ✅ **Recovery
  implemented (Phase D1)** — a crashed AR10 session resumes via the shared
  qualification restorer: AR10 layout, decimal scoring, sighters + officials
  restored through the live projection path, frozen 75-min clock rebased (never
  restarted), next official continues the sequence, duplicate hardware identity
  refused. ⏳ **EST malfunction workflow pending (Phase E)** — normal crash
  recovery only; Jury credits/allowances are not implemented.
- **Rules verification status:** 📋 User-supplied ISSF rule extract (edition/page
  not supplied).
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** Scoring ✅ · Persistence ✅ · Recovery restore ❌ (Phase D).

## Durability boundary (Phase B1)

An AR10 shot is **not** added to the visible UI models until its
`SighterAccepted`/`ShotAccepted` event has been durably submitted to
`SessionStore`. Flow: raw measurement → `calculateShootingSocre` (decimal) →
`ShootingPage.onPointAddedToSeries` (AR10 branch) → `QUAL.submitSighter/
submitOfficial` → `SessionStore` (never-refuse-to-score) → reducer →
`QUAL.shotAccepted` signal → the existing `rightPanel.addToSeries` projection.
There is **one** acceptance route; if the submit is refused (cap / duplicate
`externalId` / wrong phase) nothing is projected. Duplicate prevention:
`CenterPane.backEndShootCount` dedupes repeated hardware deliveries upstream; a
per-session monotonic `qualShotSeq` is carried as the event `externalId` for
reducer-level dedup and future resume seeding (details:
[../session-reliability-phaseB-qualification.md](../session-reliability-phaseB-qualification.md)).

## Event overview

- **Range distance:** 10 m, enclosed range. 📋
- **Target type:** Electronic Scoring Targets (EST). 📋
- **Athlete category:** (not specified in supplied rules). ⏳
- **Qualification/final structure:** Qualification event. The **qualification
  score is not carried into the Final.** 📋
- **Scoring method:** **Decimal** qualification scoring. 📋
- **Number of official shots:** **60.** 📋
- **Series/position structure:** No positions (standing only). Series structure
  for display is a TechAim presentation concern. 🔧
- **Completion condition:** 60 official shots recorded, or match time expiry. 📋

## Scoring rules

- **Decimal** scoring; maximum shot value **10.9**. 📋 / 🔧 (`scoreTenths`
  0–109; the reducer already validates `scoreTenths` and derives totals).
- **Official total** = sum of the 60 official decimal shots. Reducer-owned
  (`SessionState.totalTenths`, derived from records — spec §8). 🔧
- **Sighter exclusion:** sighters (`shotNumber == 0`) never contribute to the
  official total. 🔧 (already enforced by `SighterAccepted` vs `ShotAccepted`).
- **Corrections:** generic `ShotInvalidated` / `ShotRescored` mark records; they
  never delete them. 🔧
- **Tie-breaking:** ⏳ not required for single-lane training; not implemented.

## Match phases

Exact sequence for this discipline:

1. Athlete call to line (operational; not journalled as a phase). 🔧
2. **Preparation and Sighting** — single combined phase. 📋
3. **Official match.** 📋
4. Completion (60th shot or time expiry). 📋

There are **no** position changes and **no** Finals stages for this discipline.

## Timing

| Phase | Duration | Start | Stop | Continuous? | Pausable? |
|---|---|---|---|---|---|
| Preparation + Sighting | **15 min** 📋 (🔧 `prep_time` default 15) | phase start command | timer expiry or athlete starts match | yes | only via authorised EST incident |
| Official match | **75 min** 📋 (🔧 `getMatchTime()` returns 75) | official match start | 60th shot or expiry | yes | only via authorised EST incident |

- **Recovery behaviour:** on crash, the active phase clock freezes at the last
  valid journalled state; `remainingCompetitionTime` is recomputed from the
  journal (see [est-malfunctions.md §5](est-malfunctions.md#5-the-five-distinct-timerallowance-concepts)).
  The unavailable interval must not count against the athlete.
- **Jury-authorised additions:** per shared EST rules only.

## Preparation and sighting

- **Duration:** 15 minutes (combined preparation + sighting). 📋
- **Sighting shots:** **unlimited** during that phase. 📋
- **When permitted:** only within the P&S phase (and within an authorised
  post-interruption recovery-sighting phase). 📋
- **When it ends:** at P&S timer expiry or when the athlete begins the official
  match. 🔧
- **Post-interruption sighting:** granted only per shared EST rules (over 5 min
  or target move), as a **distinct** recovery-sighting phase. 📋
- **Distinguishing recovery sighters:** tagged via the recovery-sighting events;
  they never affect official count/total/ranking/next-shot number.
  🛠 (see EST doc §5).

## Official shot sequence

- **First official shot:** number **1**. 📋
- **Shot-number range:** 1–60. 📋
- **Series structure:** display-only grouping (TechAim). 🔧
- **Transition rules:** official match begins after P&S; no position changes.
- **Completion:** 60 official shots or time expiry. 📋

## EST malfunction and interruption rules

Governed by [est-malfunctions.md](est-malfunctions.md). Discipline-specific
effects:

- The interrupted phase is either **P&S** or **official match**; the frozen
  remaining value is that phase's clock.
- Over-5-minute / target-move recovery inserts the **5-minute prep + unlimited
  recovery-sighting** phases *before* official shots resume, then restores the
  **official** match clock (frozen remaining + any authorised credit). 📋
- Clearly separated per EST doc §5: (1) restored competition time, (2)
  interruption duration, (3) Jury credit, (4) additional prep, (5) additional
  sighting.

## Recovery requirements

State that must be journalled and restored:

- athlete; `discipline = AirRifle10m`; relay/session; firing point
- current phase (P&S vs official)
- P&S remaining time; match remaining time (as timer anchors, not a single
  ambiguous value)
- all sighting shots (decimal); all official shots (decimal)
- official shot count 0→60; total score
- target-face state (rebuilt from the shot records via replay)
- next official shot number
- incident state (open/unresolved EST incident, if any)
- Jury adjustments (credits/allowances as journalled events)
- target move (where applicable)

Timer anchors required: `stageClockStartMonoMs` and `lastEventMonoMs` for the
active phase (as already used by Finals), interpreted against the 15/75-minute
durations above.

Official/sighter separation, next-shot calculation, and continuation must come
**exclusively** from the journal + reducer — never from a QML-only model.

## Journal events

| Event | Status |
|---|---|
| `SessionStarted` (discipline `AR10`, config officialShots/matchMs) | ✅ produced (B1) |
| `PreparationStarted`, `SightingStarted`, `OfficialMatchStarted` | ✅ produced (B1) |
| `SighterAccepted`, `ShotAccepted` (decimal `scoreTenths`, coords, `externalId`) | ✅ produced (B1) |
| `MatchCompleted`, `SessionClosed` / `CleanShutdown` | ✅ produced (B1, on clean finish) |
| `TimerStarted` / `TimerExpired` (explicit phase-clock anchors) | ⏳ Phase D (match/prep durations currently carried in `config`; explicit timer anchors added when UI restore needs them) |
| EST incident + credit + recovery-phase events | ⏳ Phase E (see EST doc §8) |

No `AirRifleShotAccepted` — the generic `ShotAccepted` carries all required
data. 🛠

## Reducer state

Rebuildable from existing `SessionState`: `phase`, `sighters`, `officials`,
`totalTenths`, `timer`, `currentStageId` (single stage), `config`
(`officialShots=60`, `matchMs=75min`). `DisciplineState` = `QualificationState`.
⚠️ Confirm the reducer sets `QualificationState` for `AirRifle10m` and that the
P&S/match phase transitions are covered (Phase C of the plan).

## Controller or restorer ownership

**ShootingPage discipline restorer** (there is no dedicated C++ controller for
qualification; the discipline runs in `ShootingPage.qml` + Modbus backend). The
restorer owns mode/range/scoring-mode selection, phase, timers, shot models,
target face, shot numbering, official/sighter split, HUD, and live continuation.

## Manual acceptance tests

Start P&S → sighters → hard-kill → recover → confirm phase + remaining time →
begin official match → official shots → hard-kill → recover → confirm shot list,
face, score, timer → one more official shot → confirm next-shot numbering →
validate journal + hash chain → close cleanly. Plus one simulated EST incident
(record scope/duration → rule recommendation → Jury confirm → apply → prep/sight
→ resume → verify timer + count).

## Automated test requirements

- **Persistence:** session creation, phase events, sighters, official shots,
  decimal format, exact shot numbering, timer anchors, clean close, crash
  without clean close, hash-chain continuity.
- **Recovery:** 0 shots, sighting phase, first official, mid-match, shot 59,
  shot 60, crash immediately before/after journal append, snapshot+tail,
  journal-without-snapshot, torn final line, degraded-but-recoverable,
  duplicate-event prevention, continued firing after recovery.
- **Scoring:** decimal values preserved, sighters excluded from total, count
  capped at 60, recovered next-shot number correct, **no qualification score
  carried into a Final.**
- **Interruption:** per EST doc boundaries.

## Open questions

1. ⏳ EST 3:00 / 5:00 boundary interpretation (shared).
2. ⏳ Athlete category / any relay structure for single-lane training.
3. ⚠️ Confirm reducer already assigns `QualificationState` for `AirRifle10m`.

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Populated from supplied AR10 rules; recorded scoring-only implementation status. | Project owner (📋) | ShootingPage restorer, reducer, tests |
| 2026-07-19 | **B1: persistence implemented.** AR10 live scoring journalled via `QUAL`/`SessionStore`; durability boundary + duplicate prevention wired; tests + manual scoring/crash verified (journal Clean, AR10 detected as unfinished). Recovery restore remains Phase D. | Implementation (🛠) | QualificationController, ShootingPage, main.qml wiring, tst_qualification |
| 2026-07-20 | **D1: recovery implemented.** Shared qualification restorer (`enterQualificationMode(config, startFresh)` + `restoreQualificationSession`); QUAL resume adopts the journal (no new session); recovered shots projected through the live `addToSeries` path; frozen 75-min clock rebased; manual drill verified (officials 1–4 → live shot 5, timer 74:52→resumed, chain VALID, no re-anchor). Fixed a latent null `m_matchSavedFile` crash for non-LoginPage entry. | Implementation (🛠) | ShootingPage/CenterPane restorer, main.qml dispatcher, RecoveryDialog, appsettings |
