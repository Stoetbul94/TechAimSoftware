# 50 m 3P Final — operator state-flow review, 2026-08-27

**The transition the operator questioned, captured from the REAL panel driven
by the REAL controller.** Rule authority: ISSF Rule Book 2026, Edition 2025
(Second Print 07/2026), effective 1 July 2026, Rule 6.17.3 — verified from the
official PDF.

Scenario: Kneeling 10 fired early, prone sighter, Prone 10 fired early, change
to Standing with most of the block left, standing sighter, then the shared
22-minute clock allowed to expire on its own. `TECHAIM_FINALS_TIMESCALE=60`.
No state assigned directly; no timer expiry bypassed.

| POINT | CONTROLLER STATE | UI POSITION | UI PHASE | UI COMMAND | UI TIMER | TARGET | UI SERIES/RANGE |
| A. Standing sighting (time remaining) | `StandingSighting` | STANDING | STANDING · SIGHTING | MATCH FIRING START | **21:59** | SIGHTING | SIGHTERS |
| B. FIVE MINUTES warning | `StandingSighting` | STANDING | STANDING · SIGHTING | FIVE MINUTES | **04:59** | SIGHTING | SIGHTERS |
| D. THIRTY SECONDS warning | `StandingSighting` | STANDING | STANDING · SIGHTING | THIRTY SECONDS | **00:30** | SIGHTING | SIGHTERS |
| E/F. 22:00 STOP -> 30-second interval | `StandingSeries1` | STANDING | STANDING · SERIES 1 | STOP | **00:30** | MATCH | STANDING · SERIES 1 (21·25) |
| G. LOAD | `StandingSeries1` | STANDING | STANDING · SERIES 1 | FOR THE NEXT COMPETITION SERIES·LOAD | **00:05** | MATCH | STANDING · SERIES 1 (21·25) |
| I. STANDING SERIES 1 firing | `StandingSeries1` | STANDING | STANDING · SERIES 1 | START | **04:10** | MATCH | STANDING · SERIES 1 (21·25) |
| J. after Series 1 complete | `StandingSeries2` | STANDING | STANDING · SERIES 2 | STOP | **00:15** | MATCH | STANDING · SERIES 2 (26·30) |

## Reading it

- **A** — Standing sighting shows **21:59**, the shared clock. Not a fresh
  05:00. The rule (6.17.3 a, e) gives the athlete "unlimited sighting shots in
  any time remaining", and that is what this is.
- **B** — `FIVE MINUTES` at 04:59 remaining, i.e. 17:00 elapsed (6.17.3 f).
- **D** — `THIRTY SECONDS` at 00:30 remaining, i.e. 21:30 elapsed (6.17.3 f).
- **E/F** — at the 22:00 `STOP` the stage becomes **STANDING · SERIES 1** and
  the target is already **MATCH**. The 00:30 countdown is the rule's 30-second
  interval before `FOR THE NEXT COMPETITION SERIES...LOAD` (6.17.3 g).
- **G** — `FOR THE NEXT COMPETITION SERIES…LOAD`, 00:05 to `START`.
- **I** — Series 1 firing at **04:10** (250 s), shot range 21–25, target MATCH.
- **J** — Series 2 follows with its own 00:15 announcer interval.

## The presentation defect, and the correction

The phase label was already correct at every point — it never says SIGHTING
after the STOP. What was missing is that at **E/F** a *new 00:30 countdown
appears while the command still reads `STOP`*, with nothing on screen saying
what that countdown belongs to. An operator who has just watched a sighting
period end can reasonably read a fresh countdown as another preparation block.

**Correction applied (presentation only):** the panel now shows, whenever the
firing window is shut,

> `NEXT · STANDING SERIES 1 — 5 MATCH SHOTS — WAIT FOR LOAD`

and the equivalent for Series 2 and each single shot. It clears the moment
firing starts. It decides no competition rule — it reads the stage and window
the controller already publishes.

Locked by `runStandingTransitionUi` in `tests/finals/tst_finals3p.cpp`, which
asserts that after the 22:00 STOP the panel names Series 1, says MATCH and
WAIT FOR LOAD, shows 04:10 and the range 21–25, and that **no field on the
panel contains the word SIGHTING or PREPARATION** once Series 1 is firing.

## Verdict

| | |
|---|---|
| 50 m 3P Final **rule engine** | **PASS** — every duration and command verified against 6.17.3 |
| 50 m 3P Final **operator presentation** | **PASS after correction** — SEV-3, the interval countdown was unlabelled |
