# FINALS3P-FLOW-001 — 22-minute state-flow audit, 2026-08-27

**Result: the reported extra ~5-minute Standing sighting period could not be
reproduced. The implemented flow already matches the ISSF 2026 rule as
supplied. No competition logic was changed.**

## What was reported

> The athlete completed Kneeling and Prone and was already in Standing firing
> sighting shots within the original 22-minute period. When the 22-minute
> period expired, Tech Aim then entered another approximately 5-minute Standing
> sighting/preparation block, followed by additional states.

## What the audit found

The durations in `src/finals/Finals3PConfig.h` were already correct:

| | |
|---|---|
| `prepSightMs` | 300000 — 5:00, the ONLY preparation/sighting period |
| `stage1Ms` | **1320000 — 22:00** |
| `stage1Warn1Ms` | 1020000 — FIVE MINUTES at 17:00 elapsed |
| `stage1Warn2Ms` | 1290000 — THIRTY SECONDS at 21:30 elapsed |
| `preSeriesGapMs` | 30000 — the transition after STOP |
| `seriesMs` | 250000 — 4:10 |
| `singleMs` | 50000 |

And both sighting stages share the block's clock rather than opening one of
their own (`Finals3PController.cpp`, `enterStage`):

```cpp
case Stage::ProneSighting:
case Stage::StandingSighting:
    ++m_windowId;
    setWindow(WindowState::SightingOpen);
    m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;   // shared clock
    break;
```

The announcement text was already the rule's:
`FINALISTS HAVE TWENTY-TWO MINUTES TO FIRE TEN SHOTS IN EACH OF THE KNEELING
AND PRONE POSITIONS AND PREPARE FOR THE STANDING POSITION`.

## The scenario, driven against the real controller

Kneeling 10 fired immediately, prone sighter, prone 10 fired immediately,
change to Standing — then nothing, until the clock ran out on its own.
`TECHAIM_FINALS_TIMESCALE=60`, so 22:00 of competition time is 22 s of wall
time. No state was assigned directly and no timer expiry was bypassed.

```
=== FINALS3P-FLOW-001 TRACE  (timeScale 60) ===
wallMs    stage                  position  target    off    remain   window
1         Ceremony               KNEELING  SIGHTING  0      00:20    closed    <- STAGE CHANGE
895       KneelingPrepSight      KNEELING  SIGHTING  0      05:00    sighting  <- STAGE CHANGE
5913      KneelingMatch          KNEELING  MATCH     0      00:05    closed    <- STAGE CHANGE
6036      KneelingMatch          KNEELING  MATCH     0      22:00    match     kneeling window open
6041      KneelingMatch          KNEELING  MATCH     10     22:00    match     kneeling 10 fired EARLY
6042      ProneSighting          PRONE     SIGHTING  10     22:00    sighting  <- STAGE CHANGE
6042      ProneSighting          PRONE     SIGHTING  10     22:00    sighting  prone sighter
6043      ProneMatch             PRONE     MATCH     10     22:00    match     <- STAGE CHANGE
6048      ProneMatch             PRONE     MATCH     20     22:00    match     prone 10 fired EARLY
6049      StandingSighting       STANDING  SIGHTING  20     22:00    sighting  <- STAGE CHANGE
6049      StandingSighting       STANDING  SIGHTING  20     22:00    sighting  entered STANDING SIGHTING
   >> remaining on the shared clock at this point: 22:00
6049      StandingSighting       STANDING  SIGHTING  20     22:00    sighting  standing sighter
28086     StandingSeries1        STANDING  MATCH     20     00:30    closed    <- STAGE CHANGE
28086     StandingSeries1        STANDING  MATCH     20     00:30    closed    after the shared clock expired
28724     StandingSeries1        STANDING  MATCH     20     04:10    match     SERIES 1 firing window open
   >> series 1 window duration shown: 04:10

--- command sequence as issued ---
   AthletesToLine
   TakeYourPositions
   PreparationSightingStart
   ThirtySeconds
   Stop
   StageOneAnnouncement
   MatchFiringStart
   FiveMinutes
   ThirtySeconds
   Stop
   LoadSeries
   StartSeries
=== END TRACE ===
```

Read the `wallMs` column: the shared clock starts at 6036 and Standing Series 1
is reached at 28086 — **22 050 ms, or 22:00 at scale 60** — with
`StandingSighting` holding `remain` on that same clock throughout. After the
STOP comes a 00:30 transition and then a **04:10** series window.

There is exactly one `PreparationSightingStart` in the whole sequence.

## The most likely explanation for what was seen

`04:10` immediately after the 22:00 STOP, with the position still STANDING, is
the closest thing in the flow to "another approximately 5-minute Standing
block" — and it is **Standing Series 1**, the first scoring series, not a
sighting period.

## Conclusion

| Claim | Finding |
|---|---|
| One continuous 22:00 clock across K / P-sight / P / S-sight | **CONFIRMED** |
| Finishing early does not end the block | **CONFIRMED** |
| A second ~5:00 sighting period after the STOP | **NOT REPRODUCED** |
| 7-minute or 9-minute changeover remnants | **ABSENT** from the source |
| 45-shot or 15+15+15 remnants | **ABSENT** from the source |
| Exactly one `PreparationSightingStart` per Final | **CONFIRMED** |

**No fix was applied, because no defect was found in the state flow.** Changing
a state machine that already implements the rule would itself be the
competition-critical regression.

What was added instead: the normative specification
([docs/rules/50M-3P-FINAL-STATE-FLOW-ISSF-2026.md](../rules/50M-3P-FINAL-STATE-FLOW-ISSF-2026.md)),
which did not exist, and **43 regression checks** that would fail on any build
that inserts a second sighting period, restarts the clock on a position change,
lets the shot count end the block, or adds compensation time.

## Still open — needed to close this properly

1. **Which build was tested?** RC3D had no 3P Final panel at all: the Final
   fell back to the qualification `RightPanel`, whose series structure and
   labelling are a 60-shot qualification model. RC3E is the first build with
   the 3P panel. If the observation was made on RC3D, the panel is the likely
   explanation and RC3E may already resolve it.
2. **A screenshot at the moment the extra block appeared**, and the
   `tachus_log` for that session. The log records every state transition and
   command with timestamps, which settles it in minutes.
3. **Which print of the rulebook governs.** This audit used the project
   owner's *First Print 12/2025, effective 1 January 2026*;
   `Finals3PConfig.h` has always cited *Second Print 07/2026, effective
   1 July 2026*. The durations agree, so nothing is blocked — but the
   discrepancy is unresolved.
4. **Rule 6.17.3 tie-breaking provisions**, which were not supplied. Nothing
   about ties is implemented or inferred.
