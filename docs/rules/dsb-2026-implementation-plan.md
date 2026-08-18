# DSB 2026 — implementation plan

Ranking of DSB programmes by what the current SETA engine would actually have
to do. Nothing here is implemented; no catalogue entry exists. The tiers come
from `dsb-2026-software-gap-analysis.md`, not from preference.

---

## Tier 1 — configuration only, current engine

These need a programme definition and DSB timing values. No engine change, no
geometry change, no scoring change.

| Programme | Shots | EST time | Target | Blocked on |
|---|---|---|---|---|
| **1.10 Luftgewehr 10 m** | 20 / 40 / 60 | 30 / 50 / 75 min | Nr. 1 = ISSF 10 m AR | Q3 (scoring mode), Q4 (which classes shoot which variant) |
| **2.10 10 m Luftpistole** | 20 / 40 / 60 | 30 / 50 / 75 min | Nr. 7 = ISSF 10 m AP | Q3, Q4 |
| **1.80 KK-Liegendkampf 50 m** | 60 | 50 min | Nr. 3 = ISSF 50 m R | Q3, Q4 |
| **2.20 50 m Pistole** | 60 (30) | 90 min (30-shot: *recommended* 55) | Nr. 4 = ISSF 50 m P | Q3, Q4 |

All four share the same preparation shape: **15 min with unlimited sighters
before the start, outside the shooting time** — which the qualification
controller already models as Preparation → Sighting → OfficialMatch.

## Tier 2 — programme / timer changes

| Programme | Shots | EST time | What is new |
|---|---|---|---|
| **1.40 KK-Sportgewehr 50 m 3×20** | 60 | 105 min total | three positions on **one** clock; position display and per-position shot counting; equipment change permitted between positions |
| **1.60 KK-Freigewehr 50 m 3×40** | 120 | 165 min total | as 1.40 but 40 shots per position; the longest session the product would run |
| **1.20 Luftgewehr 3-Stellung** | 3×10 / 3×20 | **25/20/30** · **35/30/40** per position | **three independent position clocks**, each containing that position's own sighting, after one shared 15-minute preparation before kneeling |

## Tier 3 — needs hardware behaviour the product does not have

| Programme | Why |
|---|---|
| **2.18 10 m LP Standard** | 4 × 5 in 150 s then 4 × 5 in 20 s — series-timed exposure |
| **2.17 10 m LP Mehrkampf** | 3/7 s rapid mode **and** a second target face (Scheibe 8) |
| **2.16 10 m Mehrschüssige LP** | five **falling targets**, 5 shots in 10 s, LADEN / ACHTUNG 3-2-1-START signalling, Klappscheibe 170 × 1370 (Scheibe 9) |

---

## Recommended first implementation: **1.20 Luftgewehr 3-Stellung**

The audit supports this, and for a better reason than "the customer asked":

- It is **fully specified** by the Sportordnung — order, both shot counts, all
  six position times, the preparation rule and the sighting rule are confirmed
  facts with citations, and its per-position times are the **same for paper and
  electronic**, so there is no ambiguity about which column to implement.
- It uses **Scheibe Nr. 1**, dimensionally identical to the ISSF 10 m air-rifle
  face already scored. No geometry or scoring risk at all.
- It is the **only confirmed programme that needs the per-position timing
  model**, so building it delivers the one genuine engine capability DSB
  requires — and 1.40 and 1.60 then follow as configuration on top of a model
  that already understands positions.
- Its 3×10 variant is short (30 shots) and therefore cheap to test end to end.

The counter-argument is real and should be stated: 1.10 or 2.10 would be
*faster* to ship, because they are configuration only. If the goal is "a DSB
programme running this month", start there. If the goal is "the DSB rifle
family", 1.20 is the load-bearing one and doing it first avoids building the
position model twice.

### What must be answered before 1.20 is written

| | Question | Why it blocks |
|---|---|---|
| **Q6** | Do the three position clocks run automatically back to back, or does a range command start each? | It is the difference between an automatic sequencer and a command-driven state machine — the same difference that separates the qualification engine from the finals engine today |
| **Q3** | Integer or decimal, per competition level | Changes every displayed and recorded value |
| **Q4** | Which classes shoot 3×10 vs 3×20; when kneeling becomes **sitzend** | Programme availability and position labelling |
| **Q5** | Rifle range-officer command wording | Only if Q6 says commanded |

Q6 is the one that decides the shape of the code. **1.20 should not be written
before Q6 is answered**, because guessing it wrong means rewriting the timer.

---

## Sequencing

1. **Obtain the missing authority** — Q1, Q2, Q3, Q4, Q6 (see the source
   register). Q1 and Q2 are customer questions; Q3, Q4 and Q6 are DSB document
   questions that further reading of 0.7, Teil 15 and the DM-Ausschreibung
   should settle.
2. **Extend the catalogue schema** with the DSB fields listed at the end of the
   programme matrix — *schema first, no entries*. The `rulesetId: "dsb"` seam
   and the selector's automatic DSB visibility already exist and are tested.
3. **Tier 1 programmes** as catalogue entries once Q3 and Q4 are closed. These
   prove the DSB rule set end to end — selector, session, report — with zero
   engine risk.
4. **Per-position timing model** (gap B3–B5), behind its own tests, with 1.20
   as its first consumer.
5. **1.40 and 1.60** as configuration on the position model.
6. **DSB interruption and protest conduct** (C7–C11, C14) bound to the DSB rule
   set, reusing `EstIncidentController`.
7. **Tier 3** only if SETA supplies turning-target hardware.

## What must not happen

- No `dsb.*` catalogue entry before its row in the programme matrix says
  CONFIRMED **and** its blocking questions are closed. A profile whose
  `authorityStatus` is not confirmed must never be presented as an official
  competition.
- No **10 m 3×15** and no **50 m 3×10** profile on current evidence. Neither
  exists in the Sportordnung; inventing them would put a fabricated competition
  in front of an athlete.
- No change to `calculateShootingSocre()`, to the ring geometry or to the
  projectile diameters — the research found no DSB requirement for any of them.
- No reuse of the ISSF 3P timing shape for DSB 1.20. They are different rules
  that happen to share a position order.
