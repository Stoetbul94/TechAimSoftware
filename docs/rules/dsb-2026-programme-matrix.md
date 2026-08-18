# DSB 2026 — programme matrix

Source for every cell: `dsb-2026-source-register.md`. Sportordnung des
Deutschen Schützenbundes, **Stand 01.01.2026**.

Nothing here is implemented. No `CompetitionCatalogue` entry exists for any of
it, and none may be added until the status column says CONFIRMED **and** the
open questions that programme depends on are closed.

**Updated 2026-08-18** with the customer rule determinations (S-C.1–S-C.6) and
the DSB Deutsche Meisterschaften 2026 competition table (S-DM.1–S-DM.7). Six
programmes are now READY TO IMPLEMENT; 1.40 waits only on Q3a.

**Reading the timing columns.** The Sportordnung gives two shooting times: one
for *Zuganlagen* (pull/paper systems) and one for *andere Systeme*, marked `*`.
An electronic target is an "anderes System", so **the `*` column is the one an
EST product implements**.

---

## 1. Rifle

### 1.10 — Luftgewehr 10 m

| | |
|---|---|
| Kennzahl | 1.10 (S-0.24) |
| Distance / calibre | 10 m / 4.5 mm |
| Position | stehend |
| Shot counts | **60 / 40 / 20** — all three are official variants |
| Shooting time (paper / **EST**) | 20 → 35 / **30** · 40 → 60 / **50** · 60 → 90 / **75** min (S-1.4) |
| Preparation + sighting | **15 min, unlimited sighters, before the start, NOT part of the shooting time** (S-1.5) |
| Target | **Scheibe Nr. 1** (S-0.24) |
| Olympic competition | yes, for Männer/Frauen (S-0.22) |
| Scoring mode | **DECIMAL** — the DM 2026 table marks 1.10 Zehntel = ja (S-DM.7) |
| Status | **CONFIRMED — READY TO IMPLEMENT** |

### 1.20 — Luftgewehr 3-Stellung 10 m

| | |
|---|---|
| Kennzahl | 1.20 (S-0.24) |
| Distance / calibre | 10 m / 4.5 mm |
| Positions | **kniend → liegend → stehend**; from Herren II / Damen II kneeling may be **sitzend** (S-1.2) |
| Shot counts | **3 × 20** and **3 × 10** — both listed as official (S-0.24); equipment table states 20/20/20 (S-1.3) |
| Per-position time, 30 shots | **kniend 25 · liegend 20 · stehend 30 min** |
| Per-position time, 60 shots | **kniend 35 · liegend 30 · stehend 40 min** |
| Paper vs EST | **identical** — the Sportordnung prints the same figures in both columns for 1.20 (S-1.4). Unlike 1.10, there is **no time reduction on an electronic system** |
| What the per-position time contains | the table's own note: *Einzelzeiten kn/lg/st incl. Probe/Vorbereitung* — the position's sighting/preparation is **inside** that position's time |
| The 15-minute period | taken **once, before the kneeling position**, unlimited sighters, **not** part of the position times (S-1.5) |
| Sighting before prone and standing | **at the shooter's discretion**, out of that position's own time (S-1.5) |
| Equipment change between positions | **not** granted to 1.20 — S-1.2 grants it to KK 3×20, KK 3×40 and 300 m Freigewehr only |
| Target | **Scheibe Nr. 1** |
| Scoring mode | **whole ring** (S-C.1) |
| Position transition | **commanded, not chained**: finish a position → **`POSITION_CHANGE`** → the range officer / control software starts the next position clock (S-C.6) |
| Commands | no ISSF-style script exists in the rule; announcements are configurable and localised (S-C.5) |
| Class → variant | **event configuration, never hardcoded** — the DM runs 60 shots for Schüler 1 / Jugend while regional events also run Schüler 3×10 (S-C.4, S-DM.2) |
| Status | **FULLY SPECIFIED — READY TO IMPLEMENT** |

> **This is not ISSF 3P.** ISSF 50 m 3×20 gives one preparation-and-sighting
> period and one block of match time for the whole course, with changeover
> periods between positions. DSB 1.20 gives **one 15-minute preparation before
> kneeling and then three independent position clocks**, each of which already
> contains that position's own sighting. A timer engine built for ISSF 3P does
> not implement DSB 1.20 by configuration alone.

### 1.40 — KK-Sportgewehr 50 m 3×20

| | |
|---|---|
| Kennzahl | 1.40 (S-0.24) |
| Distance / calibre | 50 m / 5.6 mm |
| Positions | kniend / liegend / stehend |
| Shot count in the competition table | **3 × 20 only** (S-0.24) |
| Shooting time (paper / **EST**) | 60 shots → 120 / **105** min; **30 shots → 70 / 65 min, marked *(Empfehlung)*** (S-1.4) |
| Preparation + sighting | 15 min before kneeling, unlimited sighters, not in the shooting time (S-1.5) |
| Equipment change between positions | **permitted** (S-1.2) |
| Target | **Scheibe Nr. 3** |
| Olympic competition | yes (S-0.22) |
| Scoring mode | **INTEGER for the DM 2026 context** (S-C.7, Q3a closed). Recorded as competition context — not an immutable property of base rule 1.40 |
| Class context | DM 2026: all announced classes, 60 shots, with a final for Herren I / Jun. I m / Damen I / Jun. I w (S-DM.3) |
| Status | 3×20: **CONFIRMED — READY TO IMPLEMENT**. 30-shot row: **NOT an official 3 × 10** (S-C.3) |

> **The 30-shot row does not by itself create a 50 m 3×10 programme.** The
> timing table carries a 30-shot time for 1.40 and labels it a *recommendation*,
> while the competition appendix lists 1.40's shot count as 3 × 20 and nothing
> else. Those two facts are consistent with a half-course a *Veranstalter* may
> schedule, and equally consistent with a class or regional variant. The
> Sportordnung alone does not say, and it does not say whether the 30 shots are
> 3 × 10. **Do not create a `dsb.50m.rifle.3x10` profile on this evidence.**

### 1.60 — KK-Freigewehr 50 m, 120 Schuss

| | |
|---|---|
| Kennzahl | 1.60 (S-0.24) |
| Distance / calibre | 50 m / 5.6 mm |
| Positions | kniend / liegend / stehend |
| Shot count | **120 (3 × 40)** |
| Shooting time (paper / **EST**) | **195 / 165 min** (S-1.4, row "1.60. KK-Freigewehr 3 x 40 Männer") |
| Preparation + sighting | 15 min before kneeling, unlimited sighters, not in the shooting time |
| Equipment change between positions | **permitted** (S-1.2) |
| Target | **Scheibe Nr. 3** — same face as 1.40 |
| Scoring mode | **whole ring** (S-C.1) |
| Class context | DM 2026: all announced classes, 120 shots (S-DM.4) |
| Status | **CONFIRMED — READY TO IMPLEMENT** |

> Note the timing table gives 195/165 as **one time for the whole 120-shot
> course**, not three position times. That is a different timing shape from
> 1.20, from the same rulebook — which is exactly why per-programme timing
> metadata cannot be inferred from "it is a three-position event".

### 1.80 — KK-Liegendkampf 50 m

| | |
|---|---|
| Kennzahl | 1.80 (S-0.24) |
| Distance / calibre | 50 m / 5.6 mm |
| Position | liegend |
| Shot count | **60** |
| Shooting time (paper / **EST**) | **60 / 50 min** (S-1.4) |
| Preparation + sighting | 15 min, unlimited sighters, before the start, not in the shooting time |
| Target | **Scheibe Nr. 3** |
| Scoring mode | **DECIMAL** — the DM 2026 table marks 1.80 Zehntel = ja (S-DM.7) |
| Status | **CONFIRMED — READY TO IMPLEMENT** |

---

## 2. Pistol

### 2.10 — 10 m Luftpistole

| | |
|---|---|
| Distance / calibre | 10 m / 4.5 mm, standing |
| Shot counts | **60 / 40 / 20** |
| Shooting time (paper / **EST**) | 20 → 35 / **30** · 40 → 60 / **50** · 60 → 90 / **75** min (S-2.5) |
| Preparation + sighting | 15 min, unlimited sighters, before the start, not in the shooting time (S-2.6); 2.11.2 repeats that sighters in that period are unlimited (S-2.1) |
| Commands | the shooting director announces shot count and time and starts with **„START"** (S-2.1) |
| Target | **Scheibe Nr. 7** |
| Scoring mode | **whole ring** — DM 2026 Zehntel = nein (S-DM.5) |
| Class context | Schüler 1 = 20; Herren/Damen I–II and Junioren I–II = 40 (LM) / **60 (DM)**; remaining classes 40 (S-DM.5) |
| Status | **CONFIRMED — READY TO IMPLEMENT** |

### 2.16 — 10 m Mehrschüssige Luftpistole

| | |
|---|---|
| Shot counts | **60 / 30** |
| Structure | 30 = **6 series**, 60 = **12 series**; each series **5 shots in 10 s** on **5 falling targets** (S-2.2) |
| Zeroing | on a stationary 10 m pistol target, **150 s**, before the competition |
| Sighting series | **one before each round** |
| Preparation | **3 min** (S-2.5) |
| Commands | **LADEN** → 1 minute to prepare → **ACHTUNG 3–2–1–START**; optical signal: time starts when the lamp goes out after 3 s ±1 s and ends when it lights again; ends with **STOP** or the optical signal |
| Scoring | a target counts as hit only if it **falls within** the shooting time; shots before or after are misses |
| Target | **Scheibe Nr. 9 — Klappscheibe, 170 × 1370 mm** |
| Status | **CONFIRMED as a rule**, **NOT IMPLEMENTABLE on the current target** — see the gap analysis |

### 2.17 — 10 m Luftpistole Mehrkampf

| | |
|---|---|
| Shot count | **40** = 20 precision + 20 rapid (S-2.5) |
| Precision | 4 series × 5 shots, **150 s each** |
| Rapid | 4 series × 5 shots in the **3/7 s** mode (3 s ready, 7 s exposure) |
| Sighting | **one sighting series before precision and one before rapid** (S-2.3) |
| Preparation | **3 min before each of the two parts** (S-2.5) |
| Conduct | **as 25 m Pistole (2.40)** (S-2.3) |
| Targets | **Nr. 7 (precision) and Nr. 8 (rapid)** — two different faces in one course |
| Status | **CONFIRMED as a rule**, **NOT IMPLEMENTABLE** — needs turning-target control and a second target face |

### 2.18 — 10 m Luftpistole Standard

| | |
|---|---|
| Shot count | **40** = 20 + 20 |
| Part 1 | 4 series × 5 shots, **150 s each** |
| Part 2 | 4 series × 5 shots, **20 s each** |
| Sighting | **one 5-shot sighting series in 150 s** before the competition (S-2.4) |
| Preparation | **3 min** (S-2.5) |
| Conduct | **as 25 m Standardpistole (2.60)** |
| Target | **Scheibe Nr. 7** — one face throughout |
| Status | **CONFIRMED as a rule**; implementable only with a series-timed exposure model |

### 2.20 — 50 m Pistole

| | |
|---|---|
| Distance / calibre | 50 m / 5.6 mm, standing |
| Shot counts | **60 / 30** (S-2.5) |
| Shooting time, 60 (paper / **EST**) | **105 / 90 min** (S-0.24, S-2.5) |
| Shooting time, 30 | **recommended 65 min (paper) / 55 min (EST)** — printed as *empfohlen* |
| Preparation + sighting | 15 min, unlimited sighters, before the start, not in the shooting time |
| Target | **Scheibe Nr. 4** |
| Scoring mode | **whole ring** — DM 2026 Zehntel = nein (S-DM.6) |
| Class context | DM 2026: all classes, 60 shots (S-DM.6) |
| Status | 60 shots **CONFIRMED — READY TO IMPLEMENT**. 30 shots is a listed shot count whose time is explicitly a **recommendation** — model it as organiser-set, not fixed |

---

## 3. Harald's request matrix — resolved against the Sportordnung

| Request | DSB status | Authority |
|---|---|---|
| **10 m 3×10** | **CONFIRMED** — DSB 1.20, 30 shots, 25/20/30 min per position | S-0.24, S-1.4 |
| **10 m 3×20** | **CONFIRMED** — DSB 1.20, 60 shots, 35/30/40 min per position | S-0.24, S-1.4 |
| **10 m 3×15** | **NOT A DSB PROGRAMME — CLOSED** | Absent from 0.21 and both Teil 1 tables; customer confirms it is not a DSB 2026 national programme and must not be implemented as 1.20 (S-C.2). Possible later as a **custom/local profile with its own authority** |
| **50 m 3×20** | **CONFIRMED** — DSB 1.40, 120 / **105** min | S-0.24, S-1.4 |
| **50 m 3×40** | **CONFIRMED** — DSB 1.60, 120 shots, 195 / **165** min | S-0.24, S-1.4 |
| **50 m 3×10** | **NOT AN OFFICIAL DSB PROGRAMME — CLOSED** | 1.40's competition entry is 3 × 20 only; the 30-shot row is a recommendation (65 min EST) and never states a 3 × 10 split. Must not be labelled official DSB (S-C.3) |

Four of the six are confirmed outright. The other two are now **closed as
negative results**: the Sportordnung does not contain them, and the customer has
confirmed neither may be presented as an official DSB programme. A negative
result with authority behind it is a usable answer.

---

## 4. Target-standard mapping

DSB target dimensions, all in mm, from **0.20 Anhang, Teil 0 S. 62 [p80]**
(S-0.23), against the ISSF geometry the SETA engine already draws and scores.

| DSB Scheibe | Used by | Ø 10-ring | Inner ten | Ring spacing | Aiming mark | ISSF equivalent | Classification |
|---|---|---|---|---|---|---|---|
| **Nr. 1** | 1.10, 1.20 | 0.5 | 0.5 | 2.5 | 30.5 | ISSF 10 m Air Rifle | **CONFIRMED SAME STANDARD** |
| **Nr. 3** | 1.40, 1.60, 1.80 | 10.4 | 5.0 | 8.0 | 112.4 | ISSF 50 m Rifle | **CONFIRMED SAME STANDARD** |
| **Nr. 4** | 2.20 | 50.0 | 25.0 | 25.0 | 200.0 | ISSF 50 m Pistol | **CONFIRMED SAME STANDARD** |
| **Nr. 7** | 2.10, 2.18, 2.17 (precision) | 11.5 | 5.0 | 8.0 | 59.5 | ISSF 10 m Air Pistol | **CONFIRMED SAME STANDARD** |
| **Nr. 8** | 2.17 (rapid part) | 22.0 | 11.0 | 13.25 | 154.5, rings 5–10 only | — | **DIFFERENT TARGET** |
| **Nr. 9** | 2.16 | 59.5 / 40.0, **Klappscheibe**, 170 × 1370 | — | — | — | — | **DIFFERENT TARGET** |

**This is the single most valuable finding in this research.** The four faces
that matter for 1.10, 1.20, 1.40, 1.60, 1.80, 2.10 and 2.20 are dimensionally
identical to the ISSF faces already implemented and already scored by
`CenterPane.qml::calculateShootingSocre()`. A DSB programme on any of them
needs **no geometry change, no new ring table and no scoring change** — it is a
programme definition, not a scoring problem.

Scheibe 8 and Scheibe 9 are genuinely different and are the only two that would
require new target work. Both belong to programmes (2.16, 2.17) that are
blocked on hardware anyway.

**No target geometry was modified by this task.**

---

## 5. Scoring mode

**Resolved per programme. It is a property of the programme *and* the
competition, not of the discipline** — the DM 2026 competition table carries an
explicit `Zehntel` column (S-DM.1).

| Programme | Scoring mode | Authority |
|---|---|---|
| **1.10** Luftgewehr 10 m | **DECIMAL** | S-DM.7 |
| **1.20** Luftgewehr 3-Stellung | **whole ring** | S-C.1 |
| **1.40** KK-Sportgewehr 50 m 3×20 | **INTEGER** (DM 2026 context) | S-C.7 |
| **1.60** KK-Freigewehr 3×40 | **whole ring** | S-C.1 |
| **1.80** KK-Liegendkampf | **DECIMAL** | S-DM.7 |
| **2.10** 10 m Luftpistole | **whole ring** | S-DM.5 |
| **2.20** 50 m Pistole | **whole ring** | S-DM.6 |

Nothing here was inferred from ISSF. Note that the pattern does **not** match
ISSF: DSB scores 10 m air rifle and 50 m prone in tenths but the three-position
rifle events in whole rings, and both 10 m and 50 m pistol in whole rings.

**Q3a is closed** (S-C.7): 1.40 is integer in the DM 2026 context. It is stored
as `competitionContext → scoringMode`, so a different context can carry a
different mode without touching the base rule.

## 6. Class / competition context — the four layers

These must stay separate in any future data model, because DSB varies behaviour
at every one of them:

| Layer | Example from the material above | Where it lives today |
|---|---|---|
| **BASE RULE** | 1.20 is kniend → liegend → stehend | the rule number |
| **PROGRAMME VARIANT** | 1.20 as 3×10 *or* 3×20, with different position times | the Wettkampfschüsse column |
| **COMPETITION CONTEXT** | 1.40's 30-shot time is an *Empfehlung*; 2.20's 30-shot time likewise; stand-occupation times are set by the *Veranstalter* | Ausschreibung — **outside the Sportordnung** |
| **ATHLETE CLASS** | from Herren II / Damen II, kneeling may be **sitzend** in three-position events | 1.6, and 0.7 (not yet read) |

Future profile metadata will therefore need at least: `rulesetId`,
`ruleNumber` (Kennzahl), `programmeVariant` (shot count), `positionSequence`
with a per-class substitution rule, `perPositionTimeMin[]` **or** a single
`matchTimeMin`, `preparationMin` with a flag for whether it is inside or
outside the shooting time, `sighterPolicy` per position, `scoringMode`,
`timeSource` (rule-fixed vs organiser-recommended), and `athleteClasses`.

**No class rule is hardcoded by this task, and none may be.** S-C.4 is explicit:
the DM runs 1.20 at 60 shots for Schüler 1 / Jugend while regional competitions
also run Schüler 3 × 10, so the shot-count variant is **event configuration**. A
profile may declare which variants exist; it may not decide from a class which
one applies.
