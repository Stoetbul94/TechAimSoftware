# 10m Finals — future Competition Coordinator contract

**Status:** design contract only (no implementation). Defines the boundary
between the **implemented single-athlete lane** (`Finals10mController`, F1–F6)
and the **future `Finals10mCompetitionCoordinator`** — the Range Management
milestone that will run an 8-athlete elimination final.

Rules authority: [issf-rules/10m-finals-shared.md](issf-rules/10m-finals-shared.md)
(✅ verified ISSF 2026, rule 6.17.2). Architecture:
[10m-finals-architecture.md](10m-finals-architecture.md).

---

## Why a two-layer split

The verified rules describe an **8-athlete** final with live cross-athlete
ranking, eliminations (after shots 12/14/16/18/20/22/24), tie-break shoot-offs
and official placements. This app is **single-lane** hardware. The approved
architecture (see the scope decision) is therefore two layers:

| Layer | What it is | Status |
|---|---|---|
| **Lane** — `Finals10mController` (`FINALS10M`) | One athlete on one TechAim target firing the full 24-shot course of fire; decimal scoring; command-driven timing; journalled, replayable, recoverable. | ✅ Implemented (F1–F6) |
| **Coordinator** — `Finals10mCompetitionCoordinator` | Coordinates eight athletes/lanes: synchronized commands, live ranking, eliminations, ties, shoot-offs, official placements. | ⏳ Future (this doc) |

The lane **never** claims official rank, elimination place, medal, victory, a
tie against other athletes, or a shoot-off requirement. Its elimination points
are single-athlete **course checkpoints** only.

---

## Ownership boundary

### The Coordinator owns (future)
- competition ID; the eight finalists and their identities
- lane / firing-point assignments (R1-A-B-C-D-E-F-G-H-R2)
- synchronized **stage authority** (all lanes enter a stage together)
- global **command authority** (one CRO command stream to all lanes)
- per-athlete **totals** and the **live ranking**
- **elimination boundaries** (which place drops out after shots 12…24)
- **tie detection** at a boundary and **shoot-off authority**
- **official placement** (1st…8th) and **result confirmation** ("RESULTS ARE
  FINAL", medals)

### The Lane owns (implemented)
- the athlete's **session** (identity, discipline, lane/target id)
- **raw shot reception** from the target hardware (Modbus)
- **durable shot submission** (SessionStore → journal → reducer)
- the athlete's **score projection** (integer-tenths total, decimal display)
- the **target face** rendering
- **local hardware / EST status** for that lane
- **EST incident state** for that lane (Phase-E workflow)
- **lane recovery** (crash → replay → resume, F3/F5)

---

## The seam (how the coordinator attaches later — no lane rewrite)

The lane already exposes the hooks a coordinator needs; **the accepted shot
history never changes shape** when the coordinator arrives (the scope decision's
key requirement). Concretely:

1. **Command authority.** `Finals10mController` owns its clock today. The RMS
   stubs on the sibling `Finals3PController`
   (`startPhaseFromServer` / `stopPhaseFromServer` / `synchroniseClock` /
   `applyAuthoritativeState`) are the template: the coordinator becomes the
   command/clock authority and the lane follows. Add the same four
   `*FromServer` entry points to `Finals10mController`; when a coordinator is
   present, `enterStage`/`openCommandWindow` defer to coordinator commands
   instead of the local timeline. **No event schema change** — the lane still
   journals `CommandIssued` / `WindowOpened` / `TimerStarted` as now.

2. **Score export.** The lane's authoritative per-shot record is already
   integer `scoreTenths` in the reducer (`SessionState.officials`). The
   coordinator reads each lane's `cumulativeTotal` / `officialShotCount` (already
   exposed) or the reducer totals to compute ranking. The lane broadcasts via
   the existing UDP/`fromServer` hooks (same mechanism the qualification lane
   already has). **Ranking is derived by the coordinator; the lane stores no
   rank.**

3. **Elimination / placement events (coordinator-side only).** The two events
   reserved in the architecture — `FinalsShootOffOpened` and
   `FinalsPlacementDecided` — are **coordinator** concerns and are added to the
   shared event catalogue **when the coordinator is built**, not now. They
   attach placement to an athlete *without touching the lane's accepted shot
   history*: a placement references athlete + boundary shot number + total, and
   a shoot-off is a separate firing window whose shots are tallied apart from
   the 24-shot match total (reserved `seriesIndex = -2`). The lane's journal is
   unaffected.

4. **Shoot-off firing on a lane.** When the coordinator authorises a shoot-off
   for tied athletes, it opens a shoot-off window **on those lanes only**. The
   lane fires it through the same `registerShot` path but under a
   coordinator-flagged window so the shot is journalled as a tie-break (not a
   25th match shot). The lane's `maximumMatchShots` cap (24) is unchanged; the
   shoot-off shot is a distinct window class.

---

## Invariants the coordinator must preserve

- **Accepted shot history is append-only and immutable.** The coordinator may
  attach ranking/placement *around* it; it must never rewrite, renumber or
  delete a lane's accepted shots.
- **Fixed-point integer tenths remain authoritative.** Ranking and tie
  comparisons operate on integer `scoreTenths` (exact ties, no float epsilon).
- **Decimal for both disciplines.** Air Pistol final ranking is decimal — the
  AP-qualification integer path must never leak in (guarded by tests today).
- **The lane keeps working stand-alone.** Removing the coordinator returns the
  lane to the single-athlete training mode with no code change.
- **Authority model.** As with Phase-E EST incidents, the app measures and
  displays but the coordinator's authoritative decisions (placement, shoot-off,
  results-final) are explicit, journalled actions — never silently inferred.

---

## Explicitly NOT in scope until the coordinator milestone

Eight-lane network transport, official ranking/elimination/medal placement,
tie detection against other athletes, shoot-offs against other athletes,
simulated opponents, Shadow Shooting, and CS Cloud import. See the scope
decision and [10m-finals-architecture.md §0](10m-finals-architecture.md).

---

## Open items to resolve before building the coordinator

1. **Detailed command timings** — the ISSF HQ "Commands and Announcements for
   Finals" document (rulebook timings are guidelines) is still required for
   competition-accurate synchronized commands.
2. **Finals-specific EST vs generic Phase-E** — whether the coordinator drives
   the reserve-target / 2-min-restart procedure through the generic incident
   workflow or a finals-specific path.
3. **2026 "live aiming"** — confirm any procedural effect on the synchronized
   command stream.
