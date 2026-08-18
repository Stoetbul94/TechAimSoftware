# DSB 2026 — software gap analysis

DSB requirement → current SETA support → gap → required change.

Software statements are from the code on `product/seta` as of this pass; rule
statements cite `dsb-2026-source-register.md`. Ratings:
**SUPPORTED** · **PARTIAL** · **NOT SUPPORTED** · **NEEDS TEST**.

---

## A. Target geometry and scoring

| # | DSB requirement | Authority | Current SETA | Rating | Required change |
|---|---|---|---|---|---|
| A1 | 10 m rifle face: 10-ring 0.5, spacing 2.5, mark 30.5 | S-0.23 | identical ISSF face already drawn and scored | **SUPPORTED** | none |
| A2 | 50 m rifle face: 10.4 / inner 5.0 / spacing 8.0 / mark 112.4 | S-0.23 | identical | **SUPPORTED** | none |
| A3 | 10 m pistol face: 11.5 / inner 5.0 / spacing 8.0 / mark 59.5 | S-0.23 | identical | **SUPPORTED** | none |
| A4 | 50 m pistol face: 50.0 / inner 25.0 / spacing 25.0 / mark 200.0 | S-0.23 | identical | **SUPPORTED** | none |
| A5 | Scheibe 8 — 10 m rapid-fire face, rings 5–10 only | S-0.23 | not implemented | **NOT SUPPORTED** | new target definition; only needed for 2.17 |
| A6 | Scheibe 9 — Klappscheibe 170 × 1370, five falling targets | S-0.23 | not implemented | **NOT SUPPORTED** | new target *and* turning-target hardware; only for 2.16 |
| A7 | Projectile calibre 4.5 mm at 10 m, 5.6 mm at 50 m | S-0.24 | `AppSettings::projectileDiameterMm(range)` returns exactly this | **SUPPORTED** | none |
| A8 | Integer vs decimal per programme | S-C.1, S-DM.5/6/7 | engine carries both; the caller decides per discipline | **SUPPORTED** | none — the mode is now a per-programme profile value. 1.40 still needs Q3a confirmed |

**Conclusion for this section: the scoring engine needs no change for any
confirmed DSB rifle or pistol programme.** That removes the largest risk from
DSB work before it starts.

---

## B. Timing and phases

| # | DSB requirement | Authority | Current SETA | Rating | Required change |
|---|---|---|---|---|---|
| B1 | Single match clock for a one-position course (1.10, 1.80, 2.10, 2.20) | S-1.4, S-2.5 | `QualificationController::startSession(..., matchMs, prepMs, sighterLimit, ...)` — course parameters are supplied by the caller and the controller enforces nothing rule-specific | **SUPPORTED** | supply DSB values instead of ISSF ones |
| B2 | Preparation + unlimited sighting **before** the start, **outside** the match time | S-1.5, S-2.6 | `beginPreparation()` → `beginSighting()` → `beginOfficialMatch()`; `sighterLimit` already accepts an unlimited value | **SUPPORTED** | none |
| B3 | **Per-position clocks**: 1.20 runs kn/lg/st as three independent times (25/20/30 or 35/30/40) | S-1.4 | the qualification controller has **one** `matchMs`. 3P today is a display/position split over a single ISSF course clock | **NOT SUPPORTED** | a per-position timing model: a list of position times, a current-position index, and per-position expiry |
| B4 | Each 1.20 position time **contains** that position's own sighting | S-1.4 note | phases are global (Preparation → Sighting → OfficialMatch), not per position | **NOT SUPPORTED** | sighting must become re-enterable *inside* a position without leaving the position clock |
| B5 | Sighting before prone and standing is **at the shooter's discretion** | S-1.5 | no notion of optional in-position sighting | **NOT SUPPORTED** | an in-position sighter mode the athlete can enter and leave while the position clock runs |
| B6 | 1.60 runs 120 shots on **one** clock (195/165) despite being three-position | S-1.4 | single clock | **SUPPORTED** | none — but it proves B3 must be *per programme*, not "3 positions ⇒ 3 clocks" |
| B7 | Position order kniend → liegend → stehend, sitzend substitution from Herren II/Damen II | S-1.2 | 3P order is fixed K→P→S; no class substitution | **PARTIAL** | order is right; sitzend is a **label substitution supplied by the event configuration**, never derived from a class inside the engine (S-C.4) |
| B8 | Position transition is **commanded, not chained** — a `POSITION_CHANGE` state sits between position clocks and the next clock is started by the range officer / control software | S-C.6 | 3P transitions run under application control with no operator gate | **NOT SUPPORTED** | add an explicit `POSITION_CHANGE` state that holds until started; no fixed transition interval, stand occupation is the organiser's |
| B9 | Series timing 5 shots / 10 s (2.16) | S-2.2 | no series-exposure timer | **NOT SUPPORTED** | series state machine + target control |
| B10 | 3/7 s rapid mode (2.17) | S-2.3 | not implemented | **NOT SUPPORTED** | turning-target control |
| B11 | 4 × 5 in 150 s and 4 × 5 in 20 s (2.18) | S-2.4 | not implemented | **NOT SUPPORTED** | series-timed course; no turning target strictly required for the 150 s part, but the 20 s part needs exposure control |
| B12 | Organiser-set / recommended times (1.40 30-shot, 2.20 30-shot, Standbelegung) | S-1.5, S-2.5 | all times are caller-supplied | **SUPPORTED** | model the value as *recommended*, not fixed, so an Ausschreibung can override |

---

## C. EST conduct, interruptions and evidence

| # | DSB requirement | Authority | Current SETA | Rating | Required change |
|---|---|---|---|---|---|
| C1 | Shooter monitor shows the scored shot and the rings; shooter may change zoom/full view | S-0.5, S-0.6 | CenterPane shows the shot with true ring geometry; auto-zoom exists | **SUPPORTED** | none |
| C2 | Store **x/y, ring value, deviation from centre, time** for every shot | S-0.5 | `globalMatchModel` stores xmm/ymm/score/time per shot; the session journal persists shots | **SUPPORTED** | expose "deviation from centre" explicitly in the record/print |
| C3 | Shooter may switch PROBE ↔ WETTKAMPF, and back **only while no counted shot has been fired** | S-0.6 | sighter/match mode switching exists (`changeSighterMode`) | **PARTIAL / NEEDS TEST** | verify the one-way lock after the first counted shot, and the option to restrict the switch to officials |
| C4 | Printout of all shots incl. sighters, series and total | S-0.5 | report + PDF export covers match shots and series | **PARTIAL** | confirm sighters appear on the DSB-style printout |
| C5 | Result acknowledgement / protest on leaving the point | S-0.7 | not modelled | **NOT SUPPORTED** | an end-of-session acknowledgement state with a timestamp |
| C6 | Written documentation of **every** interruption and time credit | S-0.12 | `EstIncidentController` records incidents and `grantTimeCredit()` into an append-only journal | **SUPPORTED** | surface it as a printable range report |
| C7 | >3 min interruption → credit; +1 min if in the last 5 minutes | S-0.13 | `grantTimeCredit(durationMs, ...)` takes an arbitrary duration | **PARTIAL** | the +1 min and the "last 5 minutes" condition are DSB-specific and not implemented |
| C8 | Point change or >5 min → credit **+5 min** and unlimited sighters | S-0.13 | `beginRecoveryPreparation()` + recovery sighting exist; target reassignment exists | **PARTIAL** | the DSB-specific +5 min constant and the unlimited-sighter guarantee must be bound to the DSB ruleset |
| C9 | Group/system failure → +5 min, 5 min announced warning, 5 min preparation, unlimited sighters | S-0.16 | recovery preparation exists; the announcement step does not | **PARTIAL** | add the announced-restart step |
| C10 | Non-display protest: no further shot, official records the time, **extra competition shot**, strike the last shot if all register | S-0.17 | not modelled | **NOT SUPPORTED** | an extra-shot workflow with an explicit strike-one rule and an audit entry |
| C11 | Scoring protest admissible only before the next shot, or 3 min for the last shot; rejected = 2-ring deduction | S-0.18 | not modelled | **NOT SUPPORTED** | protest window state + deduction handling |
| C12 | Power failure: shots registered on the target but not visible on the monitor are established and counted | S-0.14 | session journal + recovery replay restores accepted shots | **PARTIAL / NEEDS TEST** | prove that shots accepted by the target but not yet displayed survive a power cut |
| C13 | Evidence pack: LOG printout, central-computer data, control media, range report | S-0.21 | LogFile + journal + support bundle exist | **PARTIAL** | assemble a single DSB-shaped evidence export |
| C14 | **CLEAR LOG only with the classification jury's permission** | S-0.21 | no protected clear-log operation | **NOT SUPPORTED** | make any log clear an authorised, journalled action |
| C15 | Central computer collects all targets and drives rankings/displays | S-0.8 | out of scope for the lane app; the lane already has UDP shot broadcast and `fromServer` hooks | **NOT SUPPORTED (by design)** | this is the RMS product, not the lane app |
| C16 | Wrong-target / crossfire | — | **no DSB rule located** in the parts read | **NEEDS RULE** | none until a rule exists |
| C17 | Tie rules (0.12 Ergebnisgleichheit) | S-0.24 area, not read | not modelled at lane level | **NOT SUPPORTED** | likely RMS, not lane |
| C18 | Range-officer commands | S-2.1, S-2.2, S-C.5 | commands exist for the finals domains | **PARTIAL** | **wording stays out of the rules engine** — configurable, localised announcements only (S-C.5). What the engine needs is the `POSITION_CHANGE` gate, not a script |

---

## D. What the four confirmed rifle programmes actually need

| Programme | Geometry | Scoring | Timing | Verdict |
|---|---|---|---|---|
| **1.10** 10 m LG, 20/40/60 | reuse | reuse (**decimal**) | single clock, DSB minutes | **Configuration only** |
| **1.80** 50 m KK-Liegend, 60 | reuse | reuse (**decimal**) | single clock, 50 min EST | **Configuration only** |
| **1.60** 50 m KK-Freigewehr 3×40 | reuse | reuse | single 165 min clock over three positions | **Configuration + position display** |
| **1.40** 50 m KK-Sportgewehr 3×20 | reuse | reuse | single 105 min clock over three positions | **Configuration + position display** |
| **1.20** 10 m LG 3-Stellung 3×10 / 3×20 | reuse | reuse (**whole ring**) | **three independent position clocks, each containing its own sighting, separated by a commanded `POSITION_CHANGE`** | **Real engine work (B3–B5, B8)** |

And for pistol: **2.10** and **2.20** are configuration only; **2.16**, **2.17**
and **2.18** are a different product capability (series exposure, turning
targets, a second face) and are not near-term.

---

## E. The honest summary

The scoring engine, the target geometry and the projectile handling are
**already DSB-correct** for every confirmed programme — the faces are
dimensionally identical to the ISSF ones the product already scores. Nothing in
the DSB material read here asks for a change to `calculateShootingSocre()`.

The real work is in three places, in this order:

1. **Per-position timing with a commanded transition (B3–B5, B8)** — required
   by 1.20 and by nothing else in the confirmed set. This is the one genuine
   engine change, and B8 is now specified rather than open.
2. **DSB-specific interruption and protest conduct (C7–C11, C14)** — a
   rules-bound layer over the existing incident controller, not a rewrite.
3. **Series-timed / turning-target courses (B9–B11)** — a hardware-dependent
   capability the product does not have and should not pretend to.
