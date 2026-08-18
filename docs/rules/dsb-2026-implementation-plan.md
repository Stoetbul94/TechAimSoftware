# DSB 2026 — implementation plan

Ranking of DSB programmes by what the current SETA engine would actually have
to do. The tiers come from `dsb-2026-software-gap-analysis.md`, not from
preference.

---

## Implementation status

| Step | State |
|---|---|
| Catalogue schema + 13 DSB profiles | **done** — `CompetitionCatalogue.qml` |
| Profile / timing authority seam | **done** — the profile supplies preparation, match duration, position mode and rule authority |
| Independent position clocks (1.20 sequencer) | **done** — `Dsb120Controller` (DSB120): preparation, three independent clocks, gated transitions, position-aware recovery |
| 3×40 course engine (120 shots, 3 positions) | **not built** — 1.60 is refused at the start gate |

**The seam.** A selected programme becomes `window.activeProgrammeId`, resolved
once into `activeCompetition`. Every timing site reads
`ShootingPage.authoritativeMatchSeconds()` / `authoritativePrepSeconds()`,
which return the profile's value when it declares one and the legacy
`AppSettings` lookup when it does not. A programme carries authority when it
declares `matchTimeAuthority` — only the federation programmes do, so ISSF and
the practice presets travel the unchanged legacy path. No timing decision
anywhere branches on a ruleset name or a rule number.

**What is conductable today.** Operational: 1.10 (20/40/60), 2.10 (20/40/60),
1.80, 2.20 (60 and the recommended 30), 1.40 — which runs on the existing 50 m
three-position engine with its own 105-minute clock — and **both 1.20 courses**,
3x10 and 3x20, conducted by the DSB120 sequencer. Still refused with a stated
reason: 1.60, a 120-shot three-position course the engine has no course for. A
refused programme can be selected and reviewed; only starting it is blocked, so
nothing is ever run as a different competition.

**The 1.20 sequencer.** One shared 15-minute preparation with unlimited
sighters, then a GATE. Each position is started by an authorised competition-
control action and runs its own clock — 25/20/30 for 3x10, 35/30/40 for 3x20 —
which is never derived from their sum. Kneeling opens in match, because the
shared preparation was its sighting period; prone and standing open in sighting
on a clock that is ALREADY RUNNING, so sighters there cost competition time, and
moving to match does not restart it. The first match shot of a position closes
sighting for that position — refused by the engine, not hidden in the UI. A
completed position arms the next and starts nothing. Shot classification comes
from the competition state; per-position subtotals are folded from the shots
themselves (each carries its position), so a position identity cannot be lost.

**Rule authority is persisted.** A session snapshots the competition
definition it adopted — programme, ruleset and its edition, rule number,
variant, competition context, scoring mode, timing model, target standard,
distance, and the preparation and match durations it actually anchored to —
plus the position sequence and per-position durations the 1.20 sequencer will
need. Recovery reads that snapshot; it never re-resolves the programme against
the catalogue, so a later DSB edition reusing rule 1.10 cannot reinterpret a
2026 match. Two formats carry it, deliberately differently:

| Format | Carries | Absent means |
|---|---|---|
| Session journal (`SessionStarted.ruleAuthority`, state v7) | the live recoverable session's rules | LEGACY |
| Saved match (`.tch` → `Rule_authority`) | which competition produced a saved result | LEGACY |

A session with no competition profile — every ISSF and practice programme —
adopts nothing and writes nothing, so journals recorded before this existed
re-serialise byte-identically and keep their hash chains. Absence is read as
LEGACY explicitly; it is never a reason to reject a session and never a licence
to invent an identity for one.

The journal path covers the migrated qualification disciplines (AR10, AP10,
PRONE50) — i.e. DSB 1.10, 2.10 and 1.80. **DSB 1.40 is not journalled yet**
because 50 m three positions has not been migrated to the qualification seam;
its authority rides in the `.tch` save, and it joins journal recovery when 3P
migrates. That migration, not the format, is the remaining gap.

## Tier 1 — configuration only, current engine

These need a programme definition and DSB timing values. No engine change, no
geometry change, no scoring change.

| Programme | Shots | EST time | Target | Blocked on |
|---|---|---|---|---|
| **1.10 Luftgewehr 10 m** | 20 / 40 / 60 | 30 / 50 / 75 min | Nr. 1 = ISSF 10 m AR | **nothing — decimal, ready** |
| **2.10 10 m Luftpistole** | 20 / 40 / 60 | 30 / 50 / 75 min | Nr. 7 = ISSF 10 m AP | **nothing — whole ring, ready** |
| **1.80 KK-Liegendkampf 50 m** | 60 | 50 min | Nr. 3 = ISSF 50 m R | **nothing — decimal, ready** |
| **2.20 50 m Pistole** | 60 (30) | 90 min (30-shot: *recommended* 55) | Nr. 4 = ISSF 50 m P | **nothing — whole ring, ready** |

All four share the same preparation shape: **15 min with unlimited sighters
before the start, outside the shooting time** — which the qualification
controller already models as Preparation → Sighting → OfficialMatch.

## Tier 2 — programme / timer changes

| Programme | Shots | EST time | What is new |
|---|---|---|---|
| **1.40 KK-Sportgewehr 50 m 3×20** | 60 | 105 min total | three positions on **one** clock; position display and per-position shot counting; equipment change permitted. Integer scoring (DM 2026) |
| **1.60 KK-Freigewehr 50 m 3×40** | 120 | 165 min total | as 1.40 but 40 shots per position; the longest session the product would run |
| **1.20 Luftgewehr 3-Stellung** | 3×10 / 3×20 | **25/20/30** · **35/30/40** per position | **three independent position clocks** separated by a commanded **`POSITION_CHANGE`**, each containing that position's own sighting, after one shared 15-minute preparation before kneeling. Whole-ring scoring |

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

### The blocking questions are now answered

| | Question | Answer | Effect on the design |
|---|---|---|---|
| **Q6** | Automatic or commanded position clocks? | **Commanded.** Finish a position, enter **`POSITION_CHANGE`**, the range officer or control software starts the next clock. No fixed transition interval; stand occupation is the organiser's (S-C.6) | The timer becomes a **gated sequencer**, not an auto-chain — closer to the finals controller's command model than to the qualification controller's single clock |
| **Q3** | Integer or decimal? | **Whole ring** for 1.20 (S-C.1) | No decimal display, storage or printing for this programme |
| **Q4** | Which class shoots which variant? | **Not an engine decision.** The DM runs 60 shots for Schüler 1 / Jugend while regional events also run Schüler 3 × 10, so the variant is **event configuration** (S-C.4) | The profile declares that 3×10 and 3×20 both exist; the event selects one. No class-to-shot-count table inside the engine |
| **Q5** | Command wording? | **No ISSF-style script exists in the rule.** Announcements are configurable and localised and stay **outside** the rules engine (S-C.5) | The engine needs a transition *gate*, not a script |

**1.20 is now fully specified and may be written.**

---

## Sequencing

1. **All blocking questions are closed.** Q3a resolved integer for 1.40 in the
   DM 2026 context (S-C.7). Q7 (finals per programme) and Q8 (central-computer
   interface) stay open and block nothing in Tier 1 or Tier 2.
2. **Extend the catalogue schema** with the DSB fields listed at the end of the
   programme matrix — *schema first, no entries*. The `rulesetId: "dsb"` seam
   and the selector's automatic DSB visibility already exist and are tested.
3. **Tier 1 programmes** as catalogue entries — 1.10, 1.80, 2.10 and 2.20 are
   ready now. These prove the DSB rule set end to end — selector, session,
   report — with zero engine risk.
4. **Per-position timing model** (gap B3–B5), behind its own tests, with 1.20
   as its first consumer. *Done — `Dsb120Controller`.*
5. **1.60** and **1.40** as configuration — both are single-master-clock
   programmes and do not depend on the 1.20 position model. *1.40 done. 1.60
   turned out to need more than configuration: a 120-shot three-position course
   has no engine, so it is refused rather than run as something else.*
6. **DSB interruption and protest conduct** (C7–C11, C14) bound to the DSB rule
   set, reusing `EstIncidentController`.
7. **Tier 3** only if SETA supplies turning-target hardware.

## What must not happen

- No `dsb.*` catalogue entry before its row in the programme matrix says
  CONFIRMED **and** its blocking questions are closed. A profile whose
  `authorityStatus` is not confirmed must never be presented as an official
  competition.
- No **10 m 3×15** and no **50 m 3×10** profile as a DSB programme. Both are
  now closed negatives (S-C.2, S-C.3). 3×15 may exist later only as a
  **custom/local profile carrying its own authority and labelled as such** —
  never under `rulesetId: "dsb"`.
- No **class-to-shot-count** rule inside the engine (S-C.4).
- No **command script** inside the rules engine (S-C.5).
- No change to `calculateShootingSocre()`, to the ring geometry or to the
  projectile diameters — the research found no DSB requirement for any of them.
- No reuse of the ISSF 3P timing shape for DSB 1.20. They are different rules
  that happen to share a position order.
