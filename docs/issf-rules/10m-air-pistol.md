# 10 m Air Pistol — Qualification

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for the shared EST
interruption rules.

## Document status

- **Implementation status:** ✅ **Persistence implemented (Phase B2).** Air
  Pistol live scoring is journalled through `QUAL` → `SessionStore` with
  **integer/full-ring** scores. ⚠️ **Recovery UI (restoration) NOT implemented**
  — a crashed AP10 session is detected/preserved by the recovery dialog, but
  rebuilding the AP10 UI on resume is **Phase D**. Reducer coverage: ✅ (generic
  `QualificationState`).
- **Rules verification status:** 📋 User-supplied ISSF rule extract (edition/page
  not supplied).
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** Scoring ✅ · Persistence ✅ · Recovery restore ❌ (Phase D).

## Durability boundary + integer scoring (Phase B2)

Same durable route as [Air Rifle](10m-air-rifle.md#durability-boundary-phase-b1),
extended to AP10 via the shared `QUAL` seam. The **only** AP10-specific
difference: the `ShootingPage.onPointAddedToSeries` AP10 branch **floors** the
decimal ring position to the integer ring value (`Math.floor`) *before* it is
submitted, so the reducer total is integer-based (a 10.x measurement is
persisted as `scoreTenths = 100`, i.e. integer 10 — never 104/109). AR10 stays
decimal. The full-ring rule lives in QML; the reliability engine remains
scoring-agnostic (it stores whatever tenths it is given). Verified: an AP10
journal's official `scoreTenths` are all multiples of 10 and replay into
`Discipline::AirPistol10m`, distinct from an AR10 session.

## Event overview

- **Range distance:** 10 m, enclosed range. 📋
- **Target type:** Electronic Scoring Targets. 📋
- **Athlete category:** (not specified). ⏳
- **Qualification/final structure:** Qualification event. **Qualification score
  is not carried into the Final.** 📋
- **Scoring method:** **Integer / full-ring** qualification scoring. 📋
- **Number of official shots:** **60.** 📋
- **Series/position structure:** No positions. 📋
- **Completion condition:** 60 official shots or match time expiry. 📋

## Scoring rules

- **Integer** scoring; maximum shot value **10** (full-ring; no decimal). 📋
- ⚠️ **CRITICAL — do NOT reuse the decimal Air Rifle presentation/aggregation
  path for Air Pistol.** Air Pistol qualification scores and totals are integers.
  The reliability layer stores raw `scoreTenths` internally, but the discipline
  **presentation and official aggregation must be integer** for AP10.
  🔧 (legacy uses `gameMode == 0` → pistol → integer display, e.g.
  `getSeriesTotalNonDecimal`, `totalScoreWithoutDecimal`).
- **Official total** = sum of the 60 official **integer** shot values. 📋
- **Sighter exclusion:** sighters excluded from total. 🔧
- **Corrections:** generic mark-not-delete. 🔧
- **Tie-breaking:** ⏳ not implemented.

> 🛠 **Implementation decision (storage vs presentation):** Events may still
> carry `scoreTenths` for a uniform shot payload, but for AP10 the **integer
> ring value is authoritative for scoring** and the reducer/presentation must
> aggregate integers. Tests must explicitly assert decimal↔integer separation
> between AR10 and AP10 (a decimal 10.9 must never appear in an AP10 total).

## Match phases

1. Athlete call to line (operational). 🔧
2. **Preparation and Sighting** — single combined phase. 📋
3. **Official match.** 📋
4. Completion (60th shot or expiry). 📋

No position changes, no Finals stages.

## Timing

| Phase | Duration | Continuous? | Pausable? |
|---|---|---|---|
| Preparation + Sighting | **15 min** 📋 (🔧 `prep_time` default 15) | yes | only via authorised EST incident |
| Official match | **75 min** 📋 (🔧 `getMatchTime()` returns 75 for pistol) | yes | only via authorised EST incident |

Recovery/Jury-addition behaviour identical to the shared EST rules; the frozen
remaining value is the active phase's clock.

## Preparation and sighting

- **Duration:** 15 minutes combined. 📋
- **Sighting shots:** **unlimited** during the phase. 📋
- Post-interruption recovery sighting only per shared EST rules, as a distinct,
  tagged phase; recovery sighters are **integer**-scored and never affect
  official count/total/ranking/next-shot number. 📋 / 🛠

## Official shot sequence

- First official shot: **1**; range 1–60. 📋
- Series: display grouping only. 🔧
- Completion: 60 official shots or expiry. 📋

## EST malfunction and interruption rules

Governed by [est-malfunctions.md](est-malfunctions.md). Same structure as Air
Rifle; only the **scoring type differs (integer)**. The over-5-minute / move
procedure inserts 5-min prep + unlimited recovery sighting before official shots
resume, then restores the 75-minute official clock (frozen remaining + any
authorised credit).

## Recovery requirements

Journal and restore: athlete; `discipline = AirPistol10m`; relay/session; firing
point; phase; P&S remaining; match remaining; sighters; **integer** official
shots; official count 0→60; **integer** total; target face; next official shot
number; incident state; Jury adjustments; target move. Timer anchors as per Air
Rifle. Everything from journal + reducer, never QML-only.

## Journal events

Same set as [10m-air-rifle.md](10m-air-rifle.md#journal-events). Generic
`ShotAccepted` carries the shot; **no** `AirPistolShotAccepted`. The
integer-vs-decimal distinction is a **scoring/presentation** property of the
discipline, not a separate event. 🛠

## Reducer state

`SessionState` with `DisciplineState = QualificationState`; `config.officialShots
= 60`, `matchMs = 75 min`. ⚠️ Confirm the reducer/presentation aggregates AP10 as
integer (Phase C). The generic reducer stores `scoreTenths`; the **discipline
restorer + report path** must present/aggregate integer for AP10.

## Controller or restorer ownership

**ShootingPage discipline restorer** (integer-scoring branch). No dedicated C++
controller.

## Manual acceptance tests

Same crash/recover script as Air Rifle, **plus** an explicit check that scores
and totals are integer throughout recovery (no decimal leakage).

## Automated test requirements

As Air Rifle, **plus**:
- **Scoring:** Air Pistol **integer** preserved through persist → crash →
  recover; decimal/integer separation asserted against AR10; total is integer;
  count capped at 60; recovered next-shot correct.

## Open questions

1. ⏳ EST 3:00 / 5:00 boundary (shared).
2. ⚠️ Confirm where AP10 integer aggregation must live so recovery reproduces
   integer totals deterministically (reducer vs discipline restorer/report).
3. ⏳ Athlete category / relay structure.

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Populated from supplied AP10 rules; flagged mandatory integer/decimal separation from AR10. | Project owner (📋) | ShootingPage restorer, reducer/report, tests |
| 2026-07-19 | **B2: persistence implemented.** AP10 live scoring journalled via `QUAL` with integer/full-ring scores (QML floors before submit); tests + manual verified (journal `scoreTenths` all ×10, discipline AP10, crash→restart detected as 10m Air Pistol). Recovery restore = Phase D. | Implementation (🛠) | ShootingPage wiring, tst_qualification |
