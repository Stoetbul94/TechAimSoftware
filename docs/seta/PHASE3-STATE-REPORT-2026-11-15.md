# Phase 3 — engineering state report and implementation plan

**Date:** 2026-11-15 · **Status:** review only — no implementation in this step.
Legend: **[V]** verified in code, tests or a run · **[A]** assumption / not yet measured.

---

## 1. Repository state

| | |
|---|---|
| Path | `C:\Users\User\Downloads\TechAimSoftware-SETA` (git worktree of `Stoetbul94/TechAimSoftware`) [V] |
| Branch | `feature/seta-athlete-mode` (from SETA EVAL4 `9e3acd5`) [V] |
| HEAD | `b730d1c` — Phase 2 audit [V] |
| Working tree | clean; nothing pushed [V] |
| Related, untouched | Tech Aim fix branch `fix/techaim-recovery-port` (`c20b913`, worktree `Downloads\TechAim-RecoveryPort`); seta10 checkout on `release/techaim-1.0.0` with unrelated pre-existing dirty files [V] |
| `feature/training-lab` | **already fully merged** into this branch (ancestor of HEAD, 0 commits ahead) — nothing to recover [V] |

**Test baseline** at `441460f` (the only later commit, `b730d1c`, changes a
comment and a document) [V]:

| Suite | Result |
|---|---|
| reliability | **2840 / 0** |
| qml | **299 / 0** |
| finals (3P Final) | 240 / 0 |
| finals10m | 229 / 0 |
| training | 568 / 0 |
| project memory | 228 / 0 |

The earlier figure "2,790 reliability" (quoted in the brief) is from before the
recovery work; the suite has grown to 2840 with the recovery and DSB tests.

---

## 2. UI architecture — what exists, what can be reused

| Layer | Component | Reuse |
|---|---|---|
| Shell | `main.qml` → `Header`, `LoginPage`, `ShootingPage`, `WindowManager` (Coach + Incident windows), `TechAimDialogManager`, `RecoveryDialog` [V] | keep |
| Home | `LoginPage` (operator page) + **`AthleteHome`** overlay (athlete mode, PIN, DSB/ISSF/Training tabs, `HomeDisplaySettings`) [V] | **extend** — becomes the three-function athlete screen |
| Selection data | `CompetitionCatalogue.qml` — 13 DSB profiles (authoritative times), ISSF + practice presets (legacy table) [V] | keep; practice moves onto the profile seam |
| Match | `ShootingPage` + `CenterPane` (target, scoring, clock) + `RightPanel` (shots, series, totals) + `LeftPanel` [V] | keep; athlete-mode presentation layer only |
| Domain (C++) | `QualificationController` (QUAL, journal, recovery), `Dsb120Controller`, `Finals3P/10m`, Training Lab controllers (`TRAINING`, `CALLDIAG`, `POSTRANS`, `WINDMAP`), `EstIncidentController`, frozen `CoachAnalyticsEngine` [V] | keep — no second engine |
| Acquisition | `TachusWidget` + `src/target` (sequencer, counter integrity), 100 ms poll [V] | **do not touch** |
| Reports | `ReportWindow` (Summary / Match / 3P), `FinalsReportView`, `Finals10mReportView`, Coach views, Training Lab report views [V] | keep; finish localisation |

**Lazy loading:** effectively none [V]. `ShootingPage` instantiates **every**
mode's HUD, right panel and report view at startup — Finals, 10 m Finals, DSB
1.20, all four Training Lab programmes, the report window — and `WindowManager`
creates the Coach and Incident windows at startup. Only `WindMapAnalysisView`
and `PdfPage` use a `Loader`.

---

## 3. Performance assessment

**Measured (dev laptop, Demo, 2026-11-14)** [V]: ~257 MB working set; window in
3.5–4.2 s; idle CPU ≈ 12–18 % of one core; athlete mode adds ~2 MB — within
run-to-run noise.

**Risks found in code** [V]:

| # | Finding | Impact |
|---|---|---|
| P-1 | All mode-specific QML (≈ 20 heavy components) built at startup and kept alive | memory + startup; every component's bindings evaluate even when hidden |
| P-2 | `Dsb120Hud` timer: 250 ms, `running: true` **always**, calls `DSB120.remainingMs()` even with no DSB match | small constant CPU |
| P-3 | `TachusWidget` poll every 100 ms | **required** (acquisition) — not to be changed |
| P-4 | Idle ≈ 15 % of a core | source not yet attributed [A]: candidates are scene-graph repaints (pulsing status panel, clock text) and P-2 |

**Not yet measured** [A]: RAM during shooting, per-thread CPU (GUI vs
acquisition), screen-switch latency, memory after repeated transitions, any
behaviour on the **real SETA Windows hardware** (I have only the dev laptop).

**Method:** a PowerShell measurement script (working set, private bytes,
per-thread CPU via performance counters, time-to-window, 50× home↔match↔report
transitions with memory sampled each round) plus a developer-mode log line per
screen switch. Same script runs unchanged on SETA hardware.
**Thresholds are set from that baseline**, not invented: proposal — no
measurable growth (< 2 %) after 50 transitions; idle CPU and startup no worse
than baseline; shot-to-screen latency unchanged.

---

## 4. Localisation assessment

**Current catalogue** [V]: 1196 entries; **73 open** = 58 vendored QModMaster
form strings (window never shown) + 15 neutral units (` mm`, `X:` …).
Translated and gated since Phase 1: athlete flow, command banners, Range
Incident window (107), qualification reports (59 + header/tab/card labels).

**Reconciling "223 vs 59"** [V]: the 59 were the qualification-report strings
that were *in* the catalogue; they are now translated. The rest of the old
figure is report text that **is not wrapped in `qsTr()` at all**, so no
catalogue can reach it. By literal-text count:

| View | bare text lines (approx., regex) |
|---|---|
| FinalsReportView | 25 |
| Finals10mReportView | 21 |
| CallDiagnoseReportView | 20 |
| TrainingReportView | 18 |
| CoachPrintView | 17 (+ "Generated …" built in JS) |
| PositionTransitionReportView | 17 |
| MatchReportInfo | 12 |
| CoachDetailedView / CoachDashboardView | 7 / 5 |

The exact count is produced by the gate in WP1 (strings assembled in JS are
not caught by a line regex).

**Dynamic English found** [V]:

| Seen as | Source | Fix |
|---|---|---|
| "10m Air Rifle" (recovery dialog) | `RecoveryCoordinator.cpp:23` returns an English label | QML maps the stable discipline **id** to a `qsTr` label; C++ ids unchanged |
| "Clean" (recovery dialog) | `RecoveryTypes.h:30` / `JournalValidator.h:48` | same: id → `qsTr("Intakt")` etc. |
| "10M AIR RIFLE / MATCH-60" (match header) | catalogue display keys — **0 catalogue entries** | extraction aid + translation, or the AthleteHome naming |
| "Sun 2026-11-15", "15 Nov" (reports) | `Qt.formatDateTime(…,"ddd …")` / `"d MMM yyyy"` — C locale | format with the selected UI locale |

Constraint kept: logic never branches on display text (QML-LANG-001); C++
machine strings stay English and stable (journal, logs, RMS).

---

## 5. Practice workflow assessment

**Today** [V]: ten fixed tiles (Free / 10 / 20 / 30 / 40 per weapon); shot
counts come from the legacy event index (0–5), so other counts are impossible;
times come from `config.ini` (15 / 30 / 45 / 60) and **free practice runs a
silent 90-minute clock**; nothing to choose on screen; tiles now show the real
time (PRACTICE-TIME-001) but the athlete cannot change it.

**Reusable foundation** [V]: the competition-profile seam
(`applySetaProgramme` → `activeCompetition` → `applyCompetitionProfile` /
`authoritativeMatchSeconds`) already runs arbitrary shot counts with their own
clock (DSB 1.60 = 120 shots). Practice shots are already journalled by QUAL, so
recovery works for practice too. The QUAL controller already refuses shots
beyond the cap.

**Gaps** [V/A]:
- an **untimed** session does not exist — `authoritativeMatchSeconds() == 0`
  currently means "blocked" [V]; "unlimited" needs an explicit no-clock mode;
- behaviour at time expiry for practice (refuse / allow-and-mark) is not
  defined [A — to verify];
- a custom practice duration must survive a crash: the session authority must
  carry it (the journal already stores `ruleAuthority`) [V seam, A detail];
- practice must be visibly **non-official** everywhere (tile, header, report).

---

## 6. Training Lab assessment

| Programme | Controller | State [V] |
|---|---|---|
| Technical Blocks | `TRAINING` | complete, frozen, PDF, recovery |
| Call & Diagnose | `CALLDIAG` | complete, PDF, recovery |
| Position Transition (3P) | `POSTRANS` | complete, PDF, recovery |
| Wind Map (50 m) | `WINDMAP` | complete, analysis view, PDF, recovery |

Harness: **training 568 / 0**. Evidence standard enforced
(`docs/research/*`, `check_training_lab_evidence.py`): impact data may not name
a technical cause; no aim-trace / hold-stability claims (the target measures
only impact x/y, score and time) — consistent with the brief.

Already available from real shot data: group centre (MPI), dispersion, H/V
spread, series and position comparison, time between shots, score trend
(coach engine + Training Lab views). **Not** available on the athlete screen
yet; reports **not localised** (see §4).

Integration risks: Training Lab HUDs/panels are among the always-loaded
components (P-1); making them lazy must not lose their shot projection
(`Connections` to controller `shotAccepted`) [A — must audit each].

---

## 7. Implementation plan

Change control per `CLAUDE.md`: UI decisions logged before building (UI-DEC-019+),
evidence for every closed defect, docs + tests with each package. One commit per
package; nothing pushed.

### P0 — stability, measurement, localisation

| WP | Content | Files (main) | Acceptance |
|---|---|---|---|
| **WP0 Baseline** | measurement script + dev-mode switch timing log; record laptop baseline; same script for SETA hardware | `tools/perf/measure.ps1` (new), `docs/seta/perf-baseline-*.md` | numbers recorded, script runs on SETA hardware unchanged |
| **WP1a Dynamic labels** | recovery dialog id→label; journal status labels; match header codes; locale-aware dates | `RecoveryDialog.qml`, `CenterPane/Header` display keys, report date sites | German run shows no English in these places; QML test |
| **WP1b Report localisation** | wrap Finals, 10 m Finals, Coach, Training Lab report text in `qsTr`; translate with the glossary; render each A4 page in DE + EN and check for clipping | the 8 report views above, `techaim_de_DE.ts`, glossary | every page renders in both languages without clipping (uirender evidence); language switch changes reports |
| **WP1c Localisation gate** | test: no user-visible bare literal in product QML (allowlist for neutral values) + catalogue open set == allowlist | `tests/qml` | gate fails on any new untranslated string |
| **WP2 Low-risk performance** | gate `Dsb120Hud` timer to DSB mode; then lazy-load mode-specific HUD/panel/report components **one at a time**, each after auditing its `Connections`/state | `Dsb120Hud.qml`, `ShootingPage.qml`, `WindowManager.qml` | before/after measured; shot reception and recovery tests unchanged; no component that projects shots is unloaded while its mode runs |

**P0 progress (2026-11-15):** WP0 done (`056eede`). WP1a/b/c done — every
product string translated, gate `tests/docs/check_localisation.py` 240 / 0,
Training Lab + 10 m Final reports rendered EN + DE without clipping (3P Final,
Coach and Match Summary: human visual check). Engine- and controller-supplied
sentences stay English by design (see `docs/german-translation-review.md`).
WP2 first increment done: Wind Map analysis + three Training Lab PDF
renderers created on demand (PERF-003/004), −4.4 MB at start-up; hand check of
those flows still open, and demo clicks in Technical Blocks record no shot
(pre-existing, open).

### P1 — athlete experience

| WP | Content | Acceptance |
|---|---|---|
| **WP3 Three-function athlete screen** | Freies Training / Wettkampf (DSB + ISSF) / Trainingslabor; replaces the three tabs (UI-DEC-019 supersedes part of 018) | first-time user starts practice in ≤ 3 taps; DE/EN; screenshots |
| **WP4 Practice builder** | discipline (only lane-compatible), shots (∞/10/20/30/40/60/custom), time (∞/10/20/30/45/60/90/custom); five types incl. combined (first limit wins) and competition simulation (= a verified profile, labelled "Simulation"); summary before start; remembers last settings; untimed mode; defined handling of shots at/after a limit; custom limits survive recovery | tests 6–12 of the brief; free practice has **no** clock by default; DSB/ISSF timings untouched (pinned tests) |
| **WP5 Athlete match screen** | large score / series / total / shots left / time left / sighting-vs-match; admin functions behind the operator | readable at target-side distance (screenshot check); no change to scoring |

### P2 — Training Lab

| WP | Content | Acceptance |
|---|---|---|
| **WP6** | Trainingslabor entry launches the four existing programmes with athlete-sized setup; analysis loaded only on demand; bilingual reports (via WP1b) | same data source as match (no second store); evidence-standard checks pass |

### P3 — qualification

End-to-end Windows run, **physical SETA target run** (still never done for
DSB), German usability review with SETA, performance on SETA hardware,
recovery drills, release-readiness report, EVAL5 installer.

**Blocked, untouched by Phase 3:** ISSF F1 (50 m Pistol), F2 (3P indoor /
outdoor), F3 (Third Print) — until the official sources are recorded.

---

## 8. Risk register

| Risk | Area | Mitigation |
|---|---|---|
| Lazy-loading a panel that carries a `Connections` handler drops shot projection or state | **shot reception** | audit each component; keep `Connections` in always-alive owners; shot-path tests + hand run per component |
| A practice profile is mistaken for an official one | **timing / integrity** | separate authority value (`PRACTICE`), never `RULE`; labels on tile, header, report; tests |
| "Untimed" implemented as 0 seconds collides with the "blocked" meaning of 0 | **timing** | explicit no-clock flag; test that 0 still blocks official profiles |
| Custom practice time lost on recovery | **recovery** | carried in the session authority; double-crash test like RECOVERY-CLOCK-001 |
| Translating machine strings breaks journal / RMS / logic | **data** | translate at presentation only; C++ ids unchanged; QML-LANG-001 gate |
| German text clips A4 report pages | **reports** | render every page DE + EN before claiming done |
| Changing DSB / ISSF timing by accident | **rules** | existing pinned tests + new gate on catalogue times; F1/F2 untouched |
| Performance claims from a laptop | **performance** | measure on SETA hardware before stating thresholds as met |
| UI rework disturbs acquisition thread | **hardware** | no change to `TachusWidget` / `src/target`; UI work is QML only |

---

## 9. What this step did not do

No code change; no performance improvement claimed; no measurement on SETA
hardware (not available here); no translation added.
