# 50m Rifle 3 Positions (3P) — rules, workflow, and invariants

The 3P discipline (ISSF 3x20 qualification: Kneeling → Prone → Standing,
20 record shots each, 60 total) behaves differently from every other
discipline in this app. This document records how it works, the rules that
were established while building it, and the invariants that must not be
broken. **Everything here is gated on `is3PMatch` — other disciplines
(10m Air Pistol/Rifle, 50m Pistol, 50m Rifle Prone) are untouched by these
rules and keep their original behaviour.**

## Discipline detection

`ShootingPage.is3PMatch` is set in `beginPreparationPhase()`:
rifle (`gameMode === 1`) · 50m range · `gameSubMode === 1` ·
`matchShootCount === 60`. Restored/saved sessions bypass this and run as
non-3P.

## Match workflow (state machine)

1. **Preparation/sighting** — `beginPreparationPhase()` → `sligterMode =
   true`, 15-min prep countdown. Sighter shots go to `globalSlighterModel`
   (with a `position` role) and number from 1 on a clean face.
2. **Record fire** — play → `changedToMatchMode()`: the display buffer is
   refilled with only the current position's record shots,
   `sligterMode = false`.
3. **Position rollover** — `positionWatch` (a declarative 500ms polling
   Timer) fires `enterPositionTransition()` after the 20th and 40th record
   shots: the athlete gets a sighting break for the next position, then
   presses play. The transition must NEVER be triggered imperatively from
   the shot-processing handler (model resets under live delegates crash the
   app natively) and must NOT call `MODREADER.changeSighterMode()` mid-match
   (C++ list swaps race the 100ms polling worker → heap corruption).
4. **Position tracking** — `p3Position` is *derived*, never assigned:
   `globalMatchModel.count < 20 ? 0 : (< 40 ? 1 : 2)`.
5. **Match complete** — `matchCompleteWatch` (500ms poll) calls
   `changedToMatchFinish()` at 60 record shots; review mode then jumps the
   table/face to the final series (`Qt.callLater(showLastSeriesForReview)`).
6. **Shot cap** — two synchronous guards stop anything past shot 60
   (the 500ms watcher poll leaves a race window that once registered a
   61st shot): the demo click path checks `globalMatchModel.count >=
   matchShootCount` before reaching the backend, and
   `onPointAddedToSeries` drops over-limit shots (also covers late live
   hardware reports). Free practice (`matchShootCount === -1`) is exempt.

## Data-model rules (the single most important section)

Three shot stores exist; 3P correctness is entirely about reading the
right one:

| Store | Contents | 3P behaviour |
|---|---|---|
| `globalMatchModel` | Every record shot, in order, with roles `calculatedscore`, `xmm`, `ymm`, `position` (0=K 1=P 2=S), `direction`, `timestamp`, `timeComsumed` | **Source of truth.** Grows 1→60 through the whole match. |
| `globalModelOfData` | The current target-face display buffer | **Resets at every position change** (clean face per position). Holds ≤20 shots. Equals the full record only for non-3P. |
| C++ `TachusWidget` lists (`getScore(i)`, `getXCord(i)`) | Backend copies | `getXCord/getYCord` index the **combined sighter+match** list while `getScore` is match-only — in 3P they drift out of alignment after each position's sighting break. **Never feed analytics from them.** |

Rules that follow:

- **Any cross-position aggregate must read `globalMatchModel`**: series
  totals (S1–S6 cards), grand totals (`updateGrandTotal`), the Summary's
  position/series breakdowns, the Match report pages, and the Coach feed
  (`ShootingPage.feedCoachReport()` → `COACHREPORT.analyzeShots`, which
  carries the real per-shot `position`).
- **Live display pages the buffer** (that's the clean-face-per-position
  design); **post-match review pages the full record**
  (`rightPanel.pagingModel`, gated on `shootingPage.matchFinished`), and
  the target-face overlay, selected-shot marker and score bubble follow
  the same gate. Anything indexing `currentShootIndex` must use
  `pagingModel` + a range guard — in review the index is match-absolute
  (0–59) and an out-of-range `ListModel.get()` throws a TypeError that
  silently aborts the whole binding/handler.
- **Continuous shot numbering (1–60)** is derived from data, not state:
  `snBase = sligterMode ? 0 : globalMatchModel.count - pagingModel.count`.
  During record fire the buffer holds exactly the current position's
  record shots, so the difference is the completed positions' shots
  (K 0, P 20, S 40); non-3P and review both yield 0 by construction.
  Series mapping: **Kneeling = S1,S2 · Prone = S3,S4 · Standing = S5,S6**.

## Display rules (ISSF)

- **3P scores are integer-primary with the decimal in brackets** —
  `10 (10.4)`, `565 (599.6)` — on *every* surface: live shot table, LAST
  SHOT card, TOTAL card, S1–S6 series cards, Match report (result sheet,
  per-position shot pages, series trend), Summary (Total Score card,
  Position Breakdown, Series Breakdown). All gated on `is3PMatch`;
  every other discipline stays decimal-primary.
- Inner-10 thresholds: rifle ≥ 10.2, pistol ≥ 10.4
  (`rightPanel.star_limit_value_*`).
- 3P uses the 154.4mm 50m-rifle face; ring/score maths in
  `CenterPane.calculateShootingSocre()` is shared with 50m Rifle Prone and
  was **not** modified by any 3P work.

## QML gotchas confirmed while building this (keep in mind)

- `main.qml` declares `gameRange` but **not** `gameMode` — `gameMode`
  lives on `loginPage`. A bare `gameMode` read is a ReferenceError that
  only fires once the code path executes (e.g. loops over shots), silently
  aborting the binding and freezing its last value.
- ListModel role sets lock at first append — every model that copies
  entries between models must carry the same roles (incl. `position`).
- Never reassign Repeater/ListView models from inside a shot-processing
  signal handler; defer with `Qt.callLater` or a polling watcher.

## Fix history (commits on `feature/floating-windows`)

| Commit | Fix |
|---|---|
| `e4ec9bb` | S1–S6 series cards read the full record (were resetting per position) |
| `9cb2e61` | Coach fed from `globalMatchModel` (C++ feed had sighter-polluted coords + equal-thirds position guess) |
| `7f51bdd` / `ffc9776` | Integer-primary display, live panel + all reports |
| `9c661f4` | Post-match review pages all 6 series; face follows the table |
| `ccb34b3` | Phantom review marker (stale buffer read) + TOTAL wiped per position (`updateGrandTotal` from full record) |
| `d049b53` | Frozen "0 shots · ★ 0" cards / blank Inner 10 (bare `gameMode` scope bug) |
| `d5343d9` / `da45e61` / `b6ecdaa` | Continuous 1–60 numbering (final form: data-derived `snBase`) + 60-shot cap |
