# 50 m Rifle Prone

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for the shared EST
interruption rules.

## Document status

- **Implementation status:** ✅ Persistence (B3, decimal) · ✅ Reducer/replay
  (Phase C timer anchors) · ✅ **Recovery implemented (Phase D3)** — a crashed
  Prone session resumes via the shared qualification restorer: Prone-only
  layout (never 3P mode, position transitions, a Finals controller, or a
  post-60 Final), decimal totals, target face + shot sequence restored, and
  the frozen **50-minute** clock rebased (never 75/90, never restarted). The
  60/60-crash edge resumes as complete and refuses a 61st official. ⏳ **EST
  malfunction workflow pending (Phase E).**
- **Rules verification status:** 📋 User-supplied ISSF rule extract (edition/page
  not supplied).
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** Scoring ✅ · Persistence ✅ · Recovery restore ❌ (Phase D).

## Durability boundary (Phase B3)

Same durable route as [Air Rifle](10m-air-rifle.md#durability-boundary-phase-b1),
extended to Prone via the shared `QUAL` seam (gate: 50m rifle, sub-mode 0). Prone
uses **decimal** scoring (no full-ring floor). The journal identifies
`Discipline::Prone50m` — never generic 50m or 3P — and contains **no** finals
command-window or 3P position events. Verified: a Prone journal replays into
`Prone50m` with decimal `scoreTenths` and `config.matchMs = 3_000_000` (50 min).

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
| 2026-07-19 | **B3: persistence implemented.** 50m Prone live scoring journalled via `QUAL` (decimal, 50-min clock, no Final, no 3P transition); tests + manual verified (journal discipline=PRONE50, no finals/3P events, crash→restart detected as 50m Rifle Prone). Recovery restore = Phase D. | Implementation (🛠) | ShootingPage gate, tst_qualification |
| 2026-07-20 | **D3: recovery implemented.** Prone resumes via the shared restorer (manual: officials 1–3 decimal restored, live shot 4 = 9.6, total 41.9, 50-min clock 49:53→resumed 49:47, chain VALID, no finals/3P events; 60/60 crash refuses a 61st). | Implementation (🛠) | main.qml dispatcher, tst_qualification |
