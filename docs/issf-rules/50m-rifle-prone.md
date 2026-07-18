# 50 m Rifle Prone

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for the shared EST
interruption rules.

## Document status

- **Implementation status:** 🔧 Scoring implemented in legacy QML (decimal). **No
  journal write-path, no reducer coverage confirmed, no recovery restorer** yet.
- **Rules verification status:** 📋 User-supplied ISSF rule extract (edition/page
  not supplied).
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** Partial (scores; not recoverable).

## Event overview

- **Range distance:** 50 m, **outdoor** range. 📋
- **Target type:** Electronic Scoring Targets. 📋
- **Athlete category:** (not specified). ⏳
- **Qualification/final structure:** **No Final.** Ranking is determined from the
  Qualification. 📋 There is **no** Final transition after the 60-shot Prone
  event — do not add one.
- **Scoring method:** **Decimal** scoring. 📋
- **Number of official shots:** **60.** 📋
- **Series/position structure:** Single position (prone). 📋
- **Completion condition:** 60 official shots or match time expiry. 📋

## Scoring rules

- **Decimal** scoring; maximum shot value **10.9**. 📋 / 🔧
- **Official total** = sum of 60 official decimal shots; reducer-owned. 🔧
- **Sighter exclusion:** sighters excluded from the official total. 🔧
- **Corrections:** generic mark-not-delete. 🔧
- **Tie-breaking:** ⏳ not implemented.

## Match phases

1. Athlete call to line (operational). 🔧
2. **Preparation and Sighting** — single combined phase. 📋
3. **Official match.** 📋
4. **Completion** (60th shot or expiry). **No Final.** 📋

No position changes, no Finals stages.

## Timing

| Phase | Duration | Continuous? | Pausable? |
|---|---|---|---|
| Preparation + Sighting | **15 min** 📋 (🔧 `prep_time` default 15) | yes | only via authorised EST incident |
| Official match | **50 min** 📋 (🔧 `getMatchTime()` returns 50 for Prone) | yes | only via authorised EST incident |

> Note: Prone's official match time (**50 min**) is shorter than Air
> Rifle/Pistol (75 min). This is the one timing value that differs among the
> three in-scope disciplines; the restorer must read it from config, not
> hardcode 75.

Recovery/Jury-addition behaviour per shared EST rules; the frozen remaining
value is the active phase's clock.

## Preparation and sighting

- **Duration:** 15 minutes combined. 📋
- **Sighting shots:** **unlimited** during the phase. 📋
- Post-interruption recovery sighting only per shared EST rules, as a distinct
  tagged phase; never affects official count/total/ranking/next-shot number.
  📋 / 🛠

## Official shot sequence

- First official shot: **1**; range 1–60. 📋
- Series: display grouping only. 🔧
- Completion: 60 official shots or expiry; **no Final transition.** 📋

## EST malfunction and interruption rules

Governed by [est-malfunctions.md](est-malfunctions.md). Outdoor 50 m context is
operationally noted but does not change the shared interruption logic. The
over-5-minute / target-move procedure inserts 5-min prep + unlimited recovery
sighting before official shots resume, then restores the **50-minute** official
clock (frozen remaining + any authorised credit).

## Recovery requirements

Journal and restore: athlete; `discipline = Prone50m`; relay/session; firing
point; P&S phase; P&S remaining; match phase; match remaining; sighters; 60
official **decimal** shots; official total; target-face state; next official shot
number; **completion status**; incident state; Jury adjustments;
**reserve-target movement**. Timer anchors interpreted against 15/50-minute
durations. Everything from journal + reducer, never QML-only.

## Journal events

Same set as [10m-air-rifle.md](10m-air-rifle.md#journal-events); generic
`ShotAccepted` (decimal). No `ProneShotAccepted`. EST incident/credit/recovery
events **need implementation** (shared, EST doc §8). 🛠

## Reducer state

`SessionState` with `DisciplineState = QualificationState`; `config.officialShots
= 60`, `matchMs = 50 min`. `positionIndex` fixed (single prone position). ⚠️
Confirm reducer coverage (Phase C).

## Controller or restorer ownership

**ShootingPage discipline restorer** (decimal branch, 50 m). No dedicated C++
controller. Must **not** add any Final transition.

## Manual acceptance tests

Same crash/recover script as Air Rifle, using the **50-minute** match clock, and
an explicit check that **no Final** is offered/entered after shot 60.

## Automated test requirements

As Air Rifle, **plus**:
- **Scoring/flow:** Prone decimal preserved; **no Final carried/entered** after
  60; 50-minute match clock restored correctly (not 75).

## Open questions

1. ⏳ EST 3:00 / 5:00 boundary (shared).
2. ⏳ Outdoor-specific interruption nuances (weather stoppages) — not in supplied
   rules; treat as generic EST/range interruption until confirmed.
3. ⚠️ Confirm reducer assigns `QualificationState` for `Prone50m` and reads
   `matchMs = 50 min`.

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Populated from supplied Prone rules; recorded no-Final constraint and 50-min match clock. | Project owner (📋) | ShootingPage restorer, reducer, tests |
