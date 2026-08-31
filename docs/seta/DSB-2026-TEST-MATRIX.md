# DSB 2026 — test matrix

Measured on branch `feature/seta-dsb-2026-integration`, after the port.
Every cell below is a run result, not an intention.

**DSB automated: PASS. DSB physical: PENDING.** No DSB programme has been fired
on a physical target. Tech Aim's RC3F field evidence is Tech Aim's; it says
nothing about DSB conduct and is not claimed here.

## Suite totals

| Suite | Result |
|---|---|
| Reliability (incl. `tst_dsb120`) | **2 507 / 0** |
| QML shot path + selector | **299 / 0**, 0 deferred |
| 3P Finals | **240 / 0** |
| 10 m Finals | **229 / 0** |
| Training Lab | **568 / 0** |
| Manuals | **1 409 / 0** |
| Project memory | **216 / 0** |
| Training Lab evidence | **903 / 0** |
| Windows icon | **7 / 0** |
| Support-bundle scope (SUP-002, added at EVAL4) | **26 / 0** |
| **Total** | **6 418 / 0** |

Two checks are environmental and were not counted as product results:
`check_generated_manuals.py` needs a manual build that has not been run in this
worktree, and `check_deployment_package.py` fails looking for a
`0.9.0-RC2g-DIAG` directory that does not exist — a stale path predating this
work and unrelated to DSB.

Approximately **186 assertions are DSB-specific**: 84 `DSB-120-001` and 27
`RULE-AUTH-001` in reliability, and in the QML harness 23 `DSB-SEAM-001`,
15 `DSB-160-001`, 15 `DSB-CAT-001`, 13 `DSB-120-002`, 5 `DSB-REPORT-001` and
4 `DSB-TIMING-001`.

---

## Per-state matrix

Legend: **P** pass · **NI** not implemented · **NA** not applicable ·
**ENG** covered but limited by the shared 50 m three-position engine.

| State | 1.10 | 1.20 (3×10/3×20) | 1.40 | 1.60 | 1.80 | 2.10 | 2.20 |
|---|---|---|---|---|---|---|---|
| START | P | P | P | P | P | P | P |
| PREP | P | **P** — one shared 15 min | P | P | P | P | P |
| SIGHTING | P | **P** — per position | P | P | P | P | P |
| MATCH | P | P | P | P | P | P | P |
| SHOT COUNT BOUNDARY | P | **P** — 11th match shot in a position refused | P | **P** — 40/80 from the course | P | P | P |
| POSITION TRANSITION | NA | **P** — gated, arms without starting | P | P | NA | NA | NA |
| TIMER | P | **P** — independent clocks | P | P | P | P | P |
| SCORING | P | P | P | P | P | P | P |
| TARGET MODE | P | P | P | P | P | P | P |
| PERSISTENCE | P | **P** — journalled | authority in `.tch` | authority in `.tch` | P | P | P |
| RECONNECT | P | **P** | P | P | P | P | P |
| RECOVERY | P | **P** — A–J | **ENG** | **ENG** | P | P | P |
| REPORT | P | P | P | P | P | P | P |
| COMPLETION | P | P | P | P | P | P | P |

**2.16, 2.17, 2.18: NOT IMPLEMENTED.** Falling targets (Scheibe Nr. 9),
turning targets and a second target face (Nr. 8) are hardware the product does
not have. They have no catalogue entry, so they cannot be selected.

---

## The timer rules, asserted (§12)

The DSB 1.20 clock model is the single largest way this could have gone wrong,
and it is the most heavily asserted:

- kneeling starts on its **own** 35 minutes, *"undiminished by the preparation
  period that preceded it"*
- preparation ends **at the gate** with kneeling armed — the kneeling clock does
  not start with it
- **nothing is running at the gate**; ten minutes there changes nothing
- only the **armed** position may start — prone cannot jump the queue
- completing a position **arms** the next and starts nothing
- kneeling opens **in match**; prone and standing open in sighting on a clock
  that is already running, and entering match does not restart it
- **a `SINGLE_MATCH_CLOCK` three-position course is refused by this sequencer** —
  three positions is not what activates it

That last one is the ISSF-contamination guard: it is what keeps DSB 1.40 and
1.60, which are also DSB and also three positions, on the single-clock 50 m
engine where their rule puts them.

**No legacy clock runs underneath.** `CenterPane.legacyClockIsOurs` now excludes
DSB 1.20 alongside the Finals and Training Lab. The comment on that property
records why it exists: a reconnect during a Final used to start a second,
invisible clock. DSB 1.20 had exactly that exposure.

---

## Recovery (§20), asserted end to end

All pass, from `tst_dsb120`:

| Scenario | Assertion |
|---|---|
| restart during preparation | comes back **at the gate** with kneeling armed, not mid-clock |
| restart during kneeling match | restored as kneeling match, 7 of 20 still kneeling's |
| — its clock | the **kneeling** clock at its frozen remaining, never a fresh 35 minutes |
| restart at the position change | comes back **at the gate** — prone armed, nothing running |
| restart during prone sighting | restored as prone sighting; **the sighter is not promoted to a match shot** |
| — its clock | prone's 30 minutes |
| restart during prone match | prone match at 7 of 20 restores as prone match at 7 of 20 |
| restart during standing | standing sighting/match restored with two positions behind it |

No duplicate shot, no missing shot, no position restart, no timer restart, no
shot-count reset.

### The one recovery gap, named where it belongs

**DSB 1.40 and 1.60 have no journal recovery.** An interrupted 60- or 120-shot
50 m course cannot be resumed. Their adopted rule authority *does* persist in
the saved `.tch`, and `RULE-AUTH-001` asserts a recovered 1.40 is still DSB 1.40
and never a generic 60-shot 50 m match.

This is a **50 m three-position engine** limitation, not a DSB one: ISSF 3×20
runs on the same engine and has the same gap. It closes when 3P migrates to the
qualification seam, for ISSF and DSB together. That is why 1.40 and 1.60 are
offered rather than withheld — withholding them would report the limitation as
DSB's when it is not.

---

## Acquisition, feed and shot role (§10, §11, §21)

`Dsb120Controller` contains **zero** references to acquisition, Modbus, serial,
Tachus, paper feed or motor code. It receives `submitShot(xMm, yMm, score, …)`
— a coordinate the shared path has already validated, reconciled and scored.
DSB decides what an accepted shot **means**; it never decides whether one is
real, and there is no second acquisition engine.

Paper feed is therefore driven by the shared accepted-shot path, unchanged: one
accepted shot, one feed. No DSB code issues a motor command.

Shot role is supplied **at acceptance** from competition state — sighter vs
match and which position — never inferred later from shot order. Per-position
subtotals fold from shots that each carry their own position, so a position
identity cannot be lost.

---

## Reports and PDF (§22, §23)

| | |
|---|---|
| Report required | yes, and produced from persisted session data |
| `DSB-REPORT-001` | 5 assertions: rule, variant, context, scoring mode and target standard all reach the sheet |
| Re-scoring | **none** — the report reads what was journalled |
| Teiler | SETA's product decision (`showTeilerMetric = true`), **unchanged by this round** |
| Branding | SETA, no Tech Aim report header |
| Dedicated DSB report layout | **not claimed** — DSB sessions use the SETA qualification report, which carries the rule authority |

A DSB-specific report structure has not been built and is not asserted to exist.

---

## German (§17)

`translations/techaim_de_DE.ts`: **1 131 messages, 239 unfinished, 0 vanished.**
No German string was fabricated by this round. DSB terminology in the selector
comes from the Sportordnung via the catalogue — *Luftgewehr*,
*Luftgewehr 3-Stellung*, *Luftpistole*, *KK-Liegendkampf*,
*KK-Sportgewehr 3x20*, *50 m Pistole* — and renders correctly.

---

## Visual evidence (§34)

`docs/seta/evidence/dsb-selector-eval3.png`, rendered offline from the **real**
`CompetitionCatalogue` by `tools/uirender`. It shows the DSB 2026 rule set
offered beside ISSF and the practice presets, the German discipline names, and
DSB 1.20 offering 3×10 and 3×20 both labelled *Official course*. SETA blue
(`#25B0E6`); no Tech Aim maroon; no missing QML type. The step-2 list is cut by
the specimen panel's fixed height and carries a scrollbar — that is the scene's
framing, not clipping in the application.

Application-level screenshots of a conducted DSB session have **not** been
produced. Status for those: *human visual check required*.
