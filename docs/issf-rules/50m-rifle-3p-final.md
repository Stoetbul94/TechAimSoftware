# 50 m Rifle 3 Positions — Final

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for shared EST rules.

This discipline is **implemented and recoverable** (M2 persistence, M3
recovery). This file documents its *current, verified* behaviour so future work
does not regress it. The authoritative engineering detail lives in
[docs/3p-finals-discipline.md](../3p-finals-discipline.md) and
[docs/session-reliability-m3-recovery.md](../session-reliability-m3-recovery.md);
this file is the *rules* view.

## Document status

- **Implementation status:** ✅ Implemented end-to-end (`Finals3PController`,
  `FINALS3P`); persistence via `SessionStore`; recovery via M3
  (`loadRecoveredState`). 🔧
- **Rules verification status:** 📋 User-supplied + 🔧 existing behaviour. The
  **Finals-specific EST malfunction allowance** is ⏳ awaiting an official rule.
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** ✅ Yes (single-athlete training mode).

## Event overview

- **Range distance:** 50 m. 🔧
- **Target type:** Electronic Scoring Targets. 🔧
- **Structure:** ISSF **Final**, 35 shots, single-athlete training mode. 🔧
- **Scoring method:** **Decimal only.** 🔧
- **Number of official shots:** **35.** 🔧
- **Series/stage structure:** command-driven stages — Kneeling, Prone, Standing,
  with series and single (elimination) shots. `finalsSeriesIndex` schema is
  **LOCKED**: `0=K, 1=P, 2=S1, 3=S2, 4-8=singles`. 🔧
- **Completion condition:** the 35-shot command sequence completes. 🔧

## Scoring rules

- Decimal; maximum **10.9**. 🔧
- Total and per-series subtotals derived by the reducer / `FinalsReportBuilder`
  (every report value derives there; QML only formats). 🔧
- **The Final is independent of Qualification** — no qualification score is
  carried in. 📋 (mirrors the qualification files' "not carried into the Final").

## Match phases

Command-driven (not free-running clocks):

1. Preparation. 🔧
2. Sighting (per the command sequence). 🔧
3. Official command stages: Kneeling → Prone → Standing, with series then
   single shots on command. 🔧
4. Completion at shot 35. 🔧

Timing is **event-specific command windows and stage timing** — see
[docs/3p-finals-discipline.md](../3p-finals-discipline.md). Do **not** apply the
qualification 15/75/50-minute model here.

## Timing

- Single monotonic clock; `TECHAIM_FINALS_TIMESCALE` accelerates dev runs
  (real timing when unset). 🔧
- Stage/command-window durations owned by `Finals3PController`. 🔧
- **Recovery (M3):** restores the remaining **stage or command-window** time via
  the timer-anchor rebase (`elapsed = lastEventMonoMs - stageClockStartMonoMs`,
  scaled by the timescale). Verified: timer resumes from remaining, not
  restarted. 🔧

## Preparation and sighting

Command-driven within the Final's own sequence; not the qualification combined
15-minute P&S phase. 🔧

## Official shot sequence

- 35 shots across the locked series/single schedule above. 🔧
- Shot numbering and sequence restored on recovery (verified: recovered
  officials keep 1..N, next live shot numbered correctly). 🔧

## EST malfunction and interruption rules

- The **crash/power-loss/termination** recovery path is implemented (M3): the
  Final resumes with stage, command window, shot count/sequence, target face,
  RightPanel, Finals HUD, remaining stage/command-window time, journal sequence,
  and a valid hash chain. 🔧
- ⏳ **Finals-specific EST *allowance* behaviour** (what time credit / prep /
  sighting a Jury grants for a range-caused failure *during a Final*) is **NOT
  supplied.** Finals do **not** inherit the qualification 3/5-minute model. Per
  the task instruction: **STOP and request the official Finals malfunction rule**
  before implementing any Finals EST allowance. Do not invent generic
  one-minute/two-minute Finals behaviour.

> The shared [est-malfunctions.md](est-malfunctions.md) incident *record* and
> Jury-authorisation *mechanism* are generic and may capture a Finals incident,
> but the **allowance amounts/procedure** for Finals remain ⏳.

## Recovery requirements (verified, must not regress)

Restore: stage; command window; shot count; shot sequence; target face;
RightPanel; Finals HUD; remaining stage/command-window time; journal sequence;
journal hash chain. Entry is via the shared `enterFinalsMode(startFresh=false)`
seam → `FINALS3P.resumeFromRecovery` (not a parallel path).

## Journal events

Existing finals events: `StageEntered`, `StageStatusChanged`,
`TargetModeChanged`, `WindowOpened`, `WindowClosed`, `CommandIssued`,
`ShotAccepted`, `SighterAccepted`, `ShotRejected`, `MissingShotRecorded`,
`RecoveryStarted`, `RecoveryCompleted`, `CleanShutdown`. All **existing**. 🔧

## Reducer state

`DisciplineState = Finals3PState { stageId, windowId, shotsInStage }` plus the
generic records/timer. 🔧

## Controller or restorer ownership

**`Finals3PController`** (dedicated C++ controller). This is the reference
implementation of the discipline-restorer contract.

## Manual acceptance tests

The M3 regression drill: start Final → sighters → kneeling match → officials →
hard-kill → recover → verify layout/face/HUD/next-shot/timer continuity → fire
more → validate journal + hash chain. (Executed and passing as of M3.)

## Automated test requirements

Finals harness (`tests/finals/`) including the M3 crash→recover→resume block;
reliability harness recovery blocks. Must stay green.

## Open questions

1. ⏳ Official **Finals EST malfunction allowance** rule (credit/prep/sighting
   for a range failure during a Final).
2. ⏳ ISSF edition/page for the Finals course of fire (to upgrade 🔧 → ✅).

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Documented existing implemented/verified Finals behaviour; flagged Finals EST allowance as awaiting official rule. | Existing behaviour (🔧) + owner (📋) | Finals3PController |
