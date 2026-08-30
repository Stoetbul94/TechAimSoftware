# DSB 2026 — integration audit

Read-only audit performed before any production change, on branch
`feature/seta-dsb-2026-integration`, cut from `product/seta` at `6475190`
(SETA 1.0.0-EVAL2, clean, local == remote).

**The headline finding contradicts the premise of the round.** The brief
assumed historic DSB code written against a stale core and needing rule
re-derivation. That is not what is on this branch. The DSB rule authority is
complete and sourced, the catalogue profiles exist, the 1.20 sequencer exists
and is **already automated-green inside the current SETA reliability harness**
(2 507 checks, 0 failures, ~90 of them DSB). What is missing is **five
integration points in the application shell** — the code that mounts an already
tested engine into the running product.

This is an integration round, not a rule round.

---

## 1. Rule authority (§5)

| | |
|---|---|
| Document | **Sportordnung des Deutschen Schützenbundes** |
| Edition | **Stand 01.01.2026**, 1. Auflage |
| ISBN | 978-3-96416-121-5 |
| Publisher | Deutscher Schützenbund e.V., Wiesbaden |
| Consulted form | official DSB online Sportordnung (the binding online version) |
| Consulted on | 2026-08-18 |
| Repository | `docs/rules/dsb-2026-source-register.md` (149 lines, every claim carries an `S-…` id) |

Supporting documents, all present: `dsb-2026-programme-matrix.md`,
`dsb-2026-software-gap-analysis.md`, `dsb-2026-implementation-plan.md`.

**Rule authority is COMPLETE for every programme this round would enable.** No
`RULE AUTHORITY INCOMPLETE` marker is required for 1.10, 1.20, 1.40, 1.60, 1.80,
2.10 or 2.20. The two programmes that lack it (2.16, 2.17) are blocked on
hardware, not on rules.

**DSB is not ISSF, and the documents say so explicitly.** The matrix records
that ISSF 50 m 3×20 runs one preparation period and one match clock with
changeovers, while DSB 1.20 runs **one 15-minute preparation before kneeling and
then three independent position clocks**, each already containing that position's
own sighting — and states that an ISSF 3P timer engine does not implement DSB
1.20 by configuration. That distinction is preserved in the code (§12 below).

Two negative results are recorded with authority and must be respected:
**10 m 3×15 is not a DSB programme** and **50 m 3×10 is not an official DSB
programme**. Neither may be presented as DSB.

---

## 2. Every German / DSB discipline present (§4)

Thirteen profiles exist in `CompetitionCatalogue.qml`, all currently hidden by a
single gate.

| Programme | Rule | Shots | Scoring | Timing model | Engine | Status |
|---|---|---|---|---|---|---|
| `dsb.10m.air-rifle.lg20` | 1.10 | 20 | DECIMAL | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-rifle.lg40` | 1.10 | 40 | DECIMAL | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-rifle.lg60` | 1.10 | 60 | DECIMAL | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-rifle.3x10` | 1.20 | 30 | INTEGER | **INDEPENDENT_POSITION_CLOCKS** | **Dsb120Controller** | engine green, **NOT MOUNTED** |
| `dsb.10m.air-rifle.3x20` | 1.20 | 60 | INTEGER | **INDEPENDENT_POSITION_CLOCKS** | **Dsb120Controller** | engine green, **NOT MOUNTED** |
| `dsb.50m.rifle.3x20` | 1.40 | 60 | INTEGER | SINGLE_MATCH_CLOCK | 50 m 3-position engine | WORKING, **not journalled** |
| `dsb.50m.rifle.3x40` | 1.60 | 120 | whole ring | SINGLE_MATCH_CLOCK | 50 m 3-position engine | WORKING, **not journalled** |
| `dsb.50m.rifle.prone60` | 1.80 | 60 | DECIMAL | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-pistol.lp20` | 2.10 | 20 | whole ring | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-pistol.lp40` | 2.10 | 40 | whole ring | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.10m.air-pistol.lp60` | 2.10 | 60 | whole ring | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.50m.pistol.p60` | 2.20 | 60 | whole ring | SINGLE_MATCH_CLOCK | qualification seam | **WORKING, journalled** |
| `dsb.50m.pistol.p30` | 2.20 | 30 | whole ring | SINGLE_MATCH_CLOCK | qualification seam | organiser-set time |

Not implemented and correctly so: **2.16** (Mehrschüssige LP — falling targets,
Scheibe Nr. 9) and **2.17** (LP Mehrkampf — turning targets, two faces).
Both need target hardware the product does not have. **2.18** (LP Standard)
needs a series-timed exposure model. These are hardware gaps, not integration
gaps, and this round does not change that.

---

## 3. Target geometry (§14)

The DSB faces used by every enabled programme — **Nr. 1, Nr. 3, Nr. 4, Nr. 7** —
are dimensionally identical to the ISSF faces already drawn and scored by
`CenterPane.qml::calculateShootingSocre()`. A DSB programme on those faces is a
programme definition, not a scoring problem. **No geometry change is required
and none was made.** Scheibe Nr. 8 and Nr. 9 genuinely differ and belong only to
the two hardware-blocked programmes.

Scoring mode is a property of the programme *and* the competition, carried per
profile (`scoringMode`), not inferred from the discipline.

---

## 4. Dsb120Controller trace (§7)

`src/dsb/Dsb120Controller.{h,cpp}` — 8 945 + 22 159 bytes.

**Course:** idle → `startPreparation()` → one shared 15-minute preparation with
unlimited sighters → **GATE** with kneeling *armed* → `startPosition()` →
position runs its own clock → `endPosition()` → next position *armed*, nothing
running → … → `finishMatch()`.

Kneeling opens **in match**, because the shared preparation *was* its sighting
period. Prone and standing open in sighting on a clock that is **already
running**, so sighters there cost competition time and `enterMatchPhase()` does
not restart it. The first match shot of a position closes sighting for that
position, **refused by the engine**, not hidden in the UI.

Per-position durations are 25/20/30 (3×10) and 35/30/40 (3×20), **never derived
from their sum**.

**Verified by test, not by reading** — the harness asserts, among ~90 DSB checks:
kneeling starts on its own 35 minutes "undiminished by the preparation period
that preceded it"; the gate arms but does not start; only the armed position may
start; ten minutes at the gate changes nothing; an eleventh match shot in a
position is refused; and a `SINGLE_MATCH_CLOCK` three-position course (1.40 /
1.60) is **refused by this sequencer** — three positions is not what activates it.

That last assertion is the ISSF-contamination guard the brief asks for in §12,
and it already exists.

---

## 5. Stale-core classification (§8)

| Interaction | Class | Evidence |
|---|---|---|
| `AcquisitionSequencer`, shot acceptance, coordinate validation | **COMPATIBLE** | the controller contains **zero** references to acquisition, Modbus, serial, Tachus, paper feed or motor code. It receives `submitShot(xMm, yMm, score, …)` — an already-accepted, already-scored shot |
| Counter baseline / reset / reconnect | **COMPATIBLE** | owned entirely by the shared path; DSB never sees a counter |
| Modbus read path | **COMPATIBLE** | not referenced |
| Paper feed | **COMPATIBLE** | not referenced; feed is driven by the shared accepted-shot path |
| Session journal / reducer | **COMPATIBLE** | depends on `SessionStore` + `RecoveryCoordinator` — the modern path. `Dsb120StepRecorded` is a registered domain event with serializer and registry rows |
| shotRole / position / series | **COMPATIBLE** | classification comes from competition state at acceptance; per-position subtotals fold from shots that each carry their position |
| Shot timing persistence | **COMPATIBLE** | journal path shared |
| Rule authority persistence | **COMPATIBLE** | `SessionStarted.ruleAuthority` state v7 + `.tch` `Rule_authority`; absence reads as LEGACY |
| `main.cpp` DSB120 context property | **MISSING INTEGRATION** | not registered — QML cannot see `DSB120` |
| `ShootingPage.qml` DSB integration | **MISSING INTEGRATION** | every DSB hook removed by the v1.0 carry-forward: `isDsb120Match`, the shot submission branch, the HUD mount, `restoreDsb120Session` |
| `Seta.pro` | **MISSING INTEGRATION** | sources compile only under `CONFIG+=dsb` |
| `qml.qrc` | **MISSING INTEGRATION** | `Dsb120Hud.qml` not packaged |
| `CompetitionCatalogue.qml` gate | **MISSING INTEGRATION** | `dsbAvailable: false` hides all 13 profiles all-or-nothing |
| 1.40 / 1.60 journal recovery | **MISSING INTEGRATION** | 50 m three positions has not migrated to the qualification seam; authority rides in `.tch` only |

**No `STALE API` or `STALE BEHAVIOUR` row was found.** The controller compiles
and passes against today's `SessionStore` and `RecoveryCoordinator` — it is
built and run by the current reliability harness on every run. The port is
shell wiring, not adaptation.

### A live defect found by this audit

`main.qml:525` calls `shootingPage.restoreDsb120Session(sessionId)` after
`RecoveryDialog.qml:34` maps `INDEPENDENT_POSITION_CLOCKS` → `"DSB120"`.
**`ShootingPage.qml` no longer defines that function.** The call is a dangling
reference on the DSB recovery path: unreachable today only because the selector
hides DSB, and reachable the moment it is unhidden. It must be restored as part
of the port, not after it.

---

## 6. Tech Aim historical defect protection (§9)

| Defect | DSB status | Why |
|---|---|---|
| ACQ-FLUSH-001 | **PROTECTED BY SHARED CORE** | DSB never opens the acquisition path |
| ACQ-DESYNC-002 | **PROTECTED BY SHARED CORE** | counter reconciliation is not visible to DSB |
| ACQ-SENTINEL-003 | **PROTECTED BY SHARED CORE** | sentinel rejection happens before `submitShot` |
| ACQ-READ-004 | **PROTECTED BY SHARED CORE** | checked reads are in the shared Modbus path |
| SERIAL-DEFAULT-005 | **NOT APPLICABLE** | DSB holds no serial configuration |
| THREAD-MODBUS-006 | **PROTECTED BY SHARED CORE** | DSB runs on the GUI thread and issues no Modbus |
| QML-SHOT-001 | **NEEDS DSB-SPECIFIC TEST** | the shot branch is re-added to `ShootingPage.qml` by this port and must be covered by the QML harness |
| PAPER-FEED-002 | **NEEDS DSB-SPECIFIC TEST** | one accepted shot must still yield exactly one feed with the DSB branch present |
| UI-STATUS-001 | **NEEDS DSB-SPECIFIC TEST** | the DSB HUD replaces panels that normally carry connection state |
| FINALS-TIMER-001 / -002 | **NOT APPLICABLE** | finals clocks are a different controller; the harness already asserts DSB120 refuses a single-clock course |
| FINAL-TCH-TIME-001 | **PROTECTED BY SHARED CORE** | shot time is measured and persisted by the shared path |
| shotRole persistence | **PROTECTED BY SHARED CORE** | classification supplied at acceptance from competition state |
| Support/log collection | **NEEDS VERIFICATION** | the EVAL2 collector fix must still collect a DSB journal |

---

## 7. German language (§17)

`translations/techaim_de_DE.ts` — **1 131 messages, 239 unfinished, 0 vanished**
(79 % translated). The unfinished set is the audit's German deliverable: it is
recorded, **not fabricated**. No German string was invented by this audit, and
none will be invented by the port. Terminology for DSB itself comes from the
Sportordnung and the rule documents, which already carry the authoritative terms
(*kniend*, *liegend*, *stehend*, *Probe*, *Vorbereitung*, *Zuganlagen*).

---

## 8. Teiler (§18)

`BrandPackage.cpp`: `showTeilerMetric = false` for Tech Aim, **`true` for SETA**
— the product decision, unchanged. Teiler is rendered in `MatchReportInfo.qml`,
`MatchReportView.qml`, `RightPanel.qml`, `SummaryReportView.qml`. **The Teiler
calculation is not touched by this round.** Whether DSB reports should show
Teiler is a product question for the DSB report work, not a calculation change.

---

## 9. Scope of the port

Five integration points, in dependency order:

1. `Seta.pro` — compile `src/dsb` unconditionally
2. `main.cpp` — register the `DSB120` context property
3. `qml.qrc` — package `Dsb120Hud.qml`
4. `ShootingPage.qml` — restore `isDsb120Match`, the shot submission branch, the HUD mount and `restoreDsb120Session` (closing the dangling call)
5. `CompetitionCatalogue.qml` — replace the all-or-nothing `dsbAvailable` gate with **per-programme** gating, so a mode is exposed only when its own evidence exists

Everything else — rules, profiles, controller, journal events, recovery mapping,
`.tch` authority, tests — is already present and green.

## 10. What must not change

Rules, scoring geometry, the Teiler calculation, the acquisition path, the ISSF
disciplines, EVAL2's package, and Tech Aim. 2.16, 2.17 and 2.18 stay
unimplemented for stated hardware reasons.

---

## 11. Correction — what the audit got wrong

§5 of this document concluded: *"No `STALE API` or `STALE BEHAVIOUR` row was
found. The port is shell wiring, not adaptation."* **That was wrong**, and
running the test suite with the DSB deferral removed proved it.

The conclusion was drawn from the controller, which was sound. But the audit
inferred from a healthy controller that the *page* was healthy too, and it was
not. The v1.0 carry-forward had removed three things from `ShootingPage.qml`
that DSB depends on, and the deferral was reporting all three as DEFERRED
rather than FAILED because their assertion ids begin with `DSB-`:

| Finding | Class | Consequence had it shipped |
|---|---|---|
| `authoritativeMatchSeconds()` / `authoritativePrepSeconds()` removed; **seven** sites reading the legacy shot-count table directly | **STALE BEHAVIOUR** | every federation programme would have run on a legacy duration while claiming its own — DSB 1.10 60-shot on the ISSF time instead of its 75-minute EST time, silently |
| `p3Course` removed; the 20/40 position boundaries hardcoded again | **STALE BEHAVIOUR** | DSB 1.60 3×40 would have transitioned after 20 kneeling shots and then never transitioned again |
| the engine-boundary gate on `profileNeedsUnbuiltEngine` removed from `beginPreparationPhase()` | **MISSING INTEGRATION** | a server start command could begin a programme the engine cannot conduct; only the operator path was blocked |

And one **STALE API** the port had to navigate rather than restore:
`centerPanel.suppressLegacyClock`, an imperative flag the entering mode pushed,
no longer exists. The current core derives `legacyClockIsOurs` inside
`CenterPane` instead, because a pushed flag can be left stale by a mode that
forgets to clear it. Restoring the old flag would have reintroduced exactly what
was fixed, so DSB joined the derived property.

**The lesson for the deferral mechanism.** The deferral was honest in intent —
it never silently passed anything, it printed `DEFER` and counted separately.
But it matched on the `DSB-` id prefix, so it swallowed assertions about
*shared* code that DSB merely happens to exercise. A deferral scoped to
"assertions about DSB availability" would have caught this; one scoped to
"assertions whose id starts with DSB" did not. That is why it was **deleted**
rather than narrowed.

All five are fixed, and the suite that found them now reports **299 checks, 0
failures, 0 deferred**.
