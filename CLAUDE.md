# Seta10 / TechAim electronic target software

ISSF electronic target scoring application. Qt Widgets + QML hybrid (C++
backend, QML frontend), Qt 6.5.3 MinGW on Windows, Modbus RTU/TCP to target
hardware via a vendored QModMaster fork (`ModReader/`). Rebranded from
"Tachus"/"Seta" to TechAim.

**Before doing anything else this session:** run `git log --oneline` and read
the last 3-4 commit messages in full — they are the handoff notes from the
previous work. Then read `seta10_ISSF_codebase_analysis.md` (architecture
deep-dive) and `docs/stabilisation-audit.md` (latest cleanup baseline).

**Before touching anything 3P-related** (50m Rifle 3 Positions), read
`docs/3p-discipline.md`. 3P data-model rules, workflow state machine, ISSF
integer-primary display and hard invariants are documented there. Everything
3P-specific is gated on `is3PMatch`.

**The 3P FINAL** (35-shot ISSF final, single-athlete training mode) is a
separate discipline gated on `isFinalsMatch` — decimal-only, command-driven
timing, owned end-to-end by `Finals3PController` (`FINALS3P`). Spec + full
fix history: `docs/3p-finals-discipline.md`. It shares nothing with
`is3PMatch` logic.

**Before modifying any discipline's scoring, timing, shot counting, match
phases, EST malfunction handling, recovery, or resume behaviour:** read
`docs/issf-rules/README.md` and the applicable discipline file under
`docs/issf-rules/`. That directory is the maintained authority for ISSF
discipline requirements. Do not implement behaviour that conflicts with those
files. If the applicable rules file is incomplete, ambiguous, outdated, or
marked *Awaiting official rule confirmation*, **stop and request the missing
official rule** before changing discipline-specific behaviour. When an approved
rule changes, update the rules document, the tests, and the implementation
**together**. Do not put the full rules in CLAUDE.md — the rules docs are the
detailed reference. Cross-discipline EST/interruption rules live in
`docs/issf-rules/est-malfunctions.md`.

**Before changing any Training Lab programme's athlete-facing text, metrics,
thresholds or recommendations** (Technical Blocks, Call & Diagnose, Group
Pattern Coach, Position Transition, Wind Map, and every future programme), read
`docs/research/training-lab-evidence-standard.md` and the applicable
`docs/research/<programme>-evidence.md`. Every athlete-facing claim is
classified in `docs/research/training-lab-evidence-register.md` and enforced by
`tests/docs/check_training_lab_evidence.py`. **Impact data cannot name a
technical cause** — describe the pattern, propose a controlled test, leave
sight/technique decisions to the athlete and coach. Never present a product rule
as research, never assert fatigue from score movement, and never invent coach
approval (there is none).

## TECH AIM UI PROJECT MEMORY — REQUIRED READING

**Before any homepage, theme, branding or UI work**, read all of:

- `docs/project/Current_Project_State.md` — where the product actually is
- `docs/design/TechAim_Design_System.md` — palette, typography, spacing, rules
- `docs/design/Component_Catalogue.md` — component contracts and their status
- `docs/design/Screen_Layout_Rules.md` — responsive and German-expansion rules
- `docs/ui/UI_Decision_Log.md` — accepted decisions and their reasoning
- `docs/ui/UI_Defect_Register.md` — known defects and their evidence
- `docs/ui/Homepage_Acceptance_Checklist.md` — what is proven and what is not
- `docs/ui/Homepage_As_Built.md` — the homepage as implemented, not as designed

**No future UI phase may silently reverse an accepted design decision.** A
decision changes only by adding a new entry that supersedes it; the superseded
entry stays in the log with its status updated. Never edit a decision away.

**No defect may be closed without evidence.** Code changing is not closure. A
defect is fully resolved only with a fixed commit, passing build and tests, a
real application screenshot, and a passing acceptance-checklist line. Where a
screenshot cannot be produced, the correct status is
`RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED` — not `RESOLVED`.

### Workflow for every UI phase

1. Read the project-memory documents above.
2. Identify the affected decision IDs.
3. Identify the affected defect IDs.
4. State, per decision, whether it is **preserved** or **superseded** — and if
   superseded, add the new entry before implementing.
5. Implement the change.
6. Add regression evidence (an automated test where practical; where not,
   record why and require human visual evidence).
7. Update `Homepage_Acceptance_Checklist.md`.
8. Update `UI_Defect_Register.md`.
9. Update `Homepage_As_Built.md` if behaviour changed.
10. Update `Current_Project_State.md`.
11. Commit the documentation with the implementation, or in a focused
    follow-up commit.

Do not describe a concept mockup as the application. Concept files stay stamped
**CONCEPT MOCKUP — NOT CURRENT APPLICATION** and are never cited as evidence.

## RMS — this branch (`feature/rms`)

This worktree is the **Tech Aim Range Management System** product line. Do not
modify SETA, the protected Tech Aim foundation or the frozen RC3a artefacts
from here.

**Before any RMS work, read `docs/architecture/rms-milestone-1-readonly.md`**
(protocol, node identity, duplicate/ordering rules, offline reconciliation,
and the read-only boundary) alongside
`docs/architecture/three-product-architecture.md`.

- **THE TARGET NODE REMAINS AUTHORITATIVE.** The node owns acquisition,
  sequence integrity, scoring, SessionStore, recovery and paper feed. RMS
  observes. **RMS never computes a score** — it transports the node's
  `authoritativeScore`. If RMS disappears mid-match, the match continues.
- **Milestone 1 is READ-ONLY and enforced, not promised.** No command exists in
  protocol v1, `RangeMonitor` has one ingress and no egress, `RmsUdpObserver`
  only binds and reads, and `tests/rms/tst_readonly.cpp` fails if any authored
  RMS file gains a transmit call, a TCP connection or a reference to the node's
  inbound control port 7756. Adding control is a new milestone, not an edit.
- Separate binary: `rms/TechAimRMS.pro` → `TechAimRMS.exe`. Observer core in
  `src/rms/` (QtCore + QtNetwork, no GUI). Dev simulator confined to
  `src/rms/dev/`, gated on `TECHAIM_RMS_DEV_SIMULATOR`, with an on-screen
  `SIMULATED RANGE` badge — never let it read as a real range.
- **Milestone 3 — the range is CONFIGURATION, the nodes are OBSERVATION.**
  `RangeDefinition`/`LaneDefinition` persist in RMS's OWN namespace
  (`<AppLocalDataLocation>/range.json`, org "Tech Aim" / app "Tech Aim RMS") —
  never the node application's AppData. A ten-lane range shows ten lanes with
  two stations on. The join key is `laneId ↔ nodeId` and nothing else, which is
  why a station returning on a new IP and a new bootId lands on its own lane
  with no operator action. `RangeConfigurationService` is the ONLY place the
  configuration changes; a move between lanes is one atomic save.
  See `docs/architecture/rms-milestone-3-range-definition.md`.
- Engineering detail (node/boot/session ids, duplicates, gaps, restarts) lives
  in Lane detail → Diagnostics, not on the operator's lane row. Do not delete
  it and do not put it back on the main list.
- **Milestone 4 — PLANNING is configuration, TELEMETRY is observation.**
  `MatchPlan` (programme + participating lanes + athletes) and `AthleteRegistry`
  persist beside `range.json` in RMS's own namespace. **A plan transmits
  nothing** — saving one records an intention, and the New Match page says so on
  every step. `MatchPlanService` is the only place a plan changes.
  See `docs/architecture/rms-milestone-4-match-planning.md`.
- **Never claim a target loaded a match.** `readiness()` answers two separate
  questions — PLAN COMPLETE (did the operator fill it in) and RANGE READY (are
  the stations answering) — and `targetMatchLoaded` is hard-coded false with a
  note, because no command channel exists.
- **PLANNED and OBSERVED are compared, never merged.** A station reporting a
  different `programmeId` is a mismatch shown on both sides; RMS changes neither
  the plan nor the station. Compared by stable id, never by label.
- `planId` is NOT a node `sessionId` — one plan will enclose several node
  sessions once commands exist. Keep them separate.
- ONLINE/OFFLINE is never persisted; readiness is recomputed from live telemetry
  after every restart.
- **3P FINALS ELIMINATION — RMS NEVER DECIDES IT.** Elimination is determined by
  `Finals3PController` on the node. RMS must never infer it from rank, score,
  shot count, how many athletes are left, translated text, or another athlete
  disappearing. `CompetitionStatus` (ACTIVE/WAITING/FINISHED/ELIMINATED) is a
  THIRD axis, independent of node health and target health — an eliminated
  athlete's station is normally perfectly healthy, and the lane is never
  removed. Protocol v1 carries no such field, so every real station reads
  UNKNOWN; carrying it is a deliberate v2 bump, never a widened v1.
  Requirement + v2 field list:
  `docs/architecture/rms-finals-elimination-display.md`.
- **Milestone 4.5 — the display SHOWS, it does not DECIDE.** `TargetGeometry`
  maps `xMm`/`yMm` onto a face; it never maps a position back to a value, and
  there is no coordinate→score function anywhere in the RMS tree. The big
  number is the STATION's `totalScore`; RMS's own sum of what it received
  appears only inside the unseen-shot warning, labelled as such. An unsupported
  `targetStandardId` draws a placeholder, never a substitute face. Telemetry y
  is up-positive and is flipped exactly once, in `normalise()`. Shot history is
  bounded at 30 impacts for display only. `DisplayController.laneOrder()` is the
  single ordered set behind previous/next/rotation — and it is exposed as the
  NOTIFYING property `laneOrderList`, because a `Q_INVOKABLE` cannot drive a
  live QML model. See `docs/architecture/rms-target-display-mvp.md`; the future
  spectator/TV client shape is in `rms-spectator-client-design.md`.
- **Target geometry is taken from the RULEBOOK, not from another renderer.**
  `TargetGeometry` stores official **DIAMETERS** (ISSF Rule Book 2026 rule
  6.3.4) and converts to radii, because a diameter used as a radius is the
  classic 2x error. Mirroring `IssfTargetCanvas.qml` is how RMS came to draw a
  50 m *rifle* face for 50 m pistol — that renderer has no pistol entry and
  falls through to its rifle default. Two foundation correction candidates are
  recorded in the source register and were deliberately NOT applied from this
  branch. The projectile is drawn at its true calibre (4.5 mm at 10 m, 5.6 mm at
  50 m, rules 7.4.6 / 8.4.4): ISSF scores by the OUTWARD GAUGE, so on a 10 m air
  rifle face a 10.0 has its centre ten times the ten-ring radius out, and a
  display that draws only a dot makes correct scoring look broken.
  Qualification: `docs/test/rms-target-geometry-qualification.md`; coordinate
  path: `docs/architecture/rms-coordinate-contract.md`.
- **The visual demo uses CORRELATED FIXTURES, not the chaos simulator.** The
  development scenario draws coordinate and score independently — fine for
  ordering/outage tests, useless as a picture. `--demo-range` plays fixtures
  generated outside the product by `tools/fixtures/generate_target_fixtures.py`;
  RMS reads them as opaque authoritative values.
- **Milestone 4.7 — a PHYSICAL LANE IS NOT A DEVICE IDENTITY.** The mapping is
  `laneId ↔ nodeId` and nothing else. IP, MAC, COM port, `bootId`, the laneId a
  station reports, and DISCOVERY ORDER are diagnostics only, and each has a test
  proving it cannot move a lane. `StationCode` turns a nodeId into a readable
  `E222-403F` for humans — deterministic, never persisted, never a key, and
  collision-resolved across the whole set at once. Commissioning is one tablet
  at a time; `assignNodeToLane` REFUSES an occupied lane, so replacing a station
  is clear-then-assign, two deliberate acts. A wiped tablet returns as a NEW
  unassigned station. `FieldTestRecorder` is an append-only JSONL diary of what
  RMS saw (not SessionStore, not adjudication) that records TRANSITIONS, never
  heartbeats; `FieldTestService` answers the preflight and writes the evidence
  bundle. The verdict is `OBSERVATION PREFLIGHT COMPLETE`, never "RANGE READY" —
  RMS cannot certify a station. A DEMO bundle is stamped simulated everywhere.
  `IDENTIFY_STATION` is documented and deliberately NOT built: it is a command,
  and improvising one would create an unaudited control path.
  See `docs/architecture/rms-field-test-instrumentation.md` and the range-day
  procedure `docs/test/rms-first-multilane-field-test.md`.
- **The field-test package.** `scripts/deploy/deploy-rms-fieldtest.ps1` builds
  `dist/TechAimRMS-FieldTest-M4_5/` (+ ZIP) — a self-contained runtime that
  double-clicks on a machine with no Qt. It reuses the method documented in
  SETA's `deploy-seta-release.ps1` (windeployqt in the mode this Qt install
  accepts, then a prune), and `tests/release/check_rms_deployment.py` gates it
  by walking the PE import table of every binary: nothing may resolve outside
  the folder, and no source, test or state may ship. **LIVE is the default
  mode**; `--demo-range` runs the scripted field-test demonstration and
  `--simulate` the development one. A demo writes to its own profile
  (`<AppLocalDataLocation>/field-test-demo/`) which LIVE never opens, so
  `--reset-demo` cannot touch a configured range. The demo's terminal states
  are development injections stamped SIMULATED — the script declares them at
  fixed times and never derives them from a score, rank or shot count.
- Design notes, written and deliberately NOT built:
  `docs/architecture/rms-incident-model-design.md` (raw observed shot vs
  adjudicated result — never destroy the raw one) and
  `rms-command-boundary-design.md` (legacy UDP 7756 stays outside RMS).
- Harness: `tests/rms/rms_tests.pro` (`QT = core network`, no platform plugin
  needed). Currently **1301 checks, 0 failures**.
- Protocol/state must use the stable `programmeId` (plus `rulesetId`,
  `targetStandardId`) from `CompetitionCatalogue.qml`. Display text is derived
  FROM the id; nothing is ever looked up BY a label (QML-LANG-001).

## Architecture, in short

- `ModReader/qModMaster.pro` is `include()`d directly into `Seta.pro` — one
  binary. The vendored QModMaster window is never shown (`mainWin->show()`
  commented out); its dialogs/settings are unreachable.
- `TachusWidget` (`ModReader/forms/tachuswidget.*`) bridges Modbus data to
  QML as `MODREADER`. Other context properties: `APPSETTINGS`, `CUSTOMPRINT`,
  `COACHREPORT`, `COACHFEED`, `PDFEXPORT`, `FINALS3P`, `FINALSAUDIO`.
- **Scoring**: the ISSF ring-geometry-to-score math lives in QML —
  `CenterPane.qml::calculateShootingSocre()`. The single most
  correctness-critical function in the codebase. Do not touch casually.
- **Shot record**: `globalMatchModel` (ShootingPage) is the authoritative
  full-match record (xmm/ymm/calculatedscore/timeComsumed/position per shot).
  `globalModelOfData` is the per-window/per-position DISPLAY buffer only.
  The C++ backend coordinate lists swap between sighter/match storage on
  every mode change — they are NOT a match record (the coach feed reads
  globalMatchModel for exactly this reason).
- **Finals domain** (`src/finals/`): `Finals3PController` state machine
  (single monotonic clock, `TECHAIM_FINALS_TIMESCALE` for accelerated runs),
  `FinalsReportBuilder` (QtCore-only report assembler — every report value
  derives HERE, QML only formats), `FinalsAudioService` (WAV per command
  cue, beep fallback), journal `finals_session.jsonl`.
  `finalsSeriesIndex` schema is LOCKED: 0=K, 1=P, 2=S1, 3=S2, 4-8=singles.
- **Report system** (Tech Aim Report System): shared components
  (`ReportHeader/Footer`, `SectionTitle`, `MetricCard`) on white A4 pages;
  Summary + Match tabs in the floating `ReportWindow`; finals get their own
  4-page `FinalsReportView` (Finals tab replaces the qualification tabs in
  finals mode). PDF export = `grabToImage` pages → `CUSTOMPRINT.create*Pdf`
  (harvest `result.image` INSIDE each grab callback — results are only valid
  there). Coach print/PDF uses `PdfExporter`.
- **Floating windows**: `FloatingWindow` shell + `WindowManager` registry
  (`windowManager.openReport()/openCoach()/openFinalsReport()`).
- **Coach analytics**: frozen pure-C++ engine (`src/analytics/`), QML bridge
  `COACHREPORT`; fed from `globalMatchModel` via
  `ShootingPage.feedCoachReport()` (real coords + per-shot positions), with
  transfer assertions. Shot maps draw TRUE ISSF ring geometry.
- **Dialogs**: ONE framework — `TechAimDialog` + `TechAimDialogManager`
  (`dialogManager`, ancestor scope like `windowManager`). No native
  QMessageBox / QtQuick MessageDialog anywhere reachable. Guide + migration
  table: `docs/techaim-dialogs.md`. C++ requests dialogs via signals
  (`MODREADER.uiDialogRequested`, `CUSTOMPRINT.printingNotice`).
- `Theme.qml` holds TechAim brand colors (instantiated once in main.qml as
  `theme`, ancestor-scope lookup — deliberately not a singleton).
- **Scope-resolution gotcha**: main.qml declares `gameRange` but NOT
  `gameMode` — always use `loginPage.gameMode` (0 = pistol, 1 = rifle).
  More gotchas (ListModel role locking, model resets in signal handlers,
  changeSighterMode races): see `docs/3p-discipline.md`.

## Build & test (local, verified)

- Qt 6.5.3 MinGW: `C:/Qt/6.5.3/mingw_64`, tools `C:/Qt/Tools/mingw1120_64`.
- `qmake Seta.pro && mingw32-make -f Makefile.Release` (qmake is
  warning-free). Force qrc regen after QML edits:
  `rm -f release/qrc_qml.cpp release/qrc_qml.o`. Kill Seta.exe before
  relinking. Launch-verify with `QT_FORCE_STDERR_LOGGING=1` and read stderr.
- Deployed `release/` needs `Qt6Multimedia.dll` + `multimedia/` plugins
  (finals audio).
- **Finals harness**: `tests/finals/finals_tests.pro` — standalone console
  binary, run with Qt bin on PATH. Currently **189 checks, 0 failures**.
  The Qt multimedia backend hard-exits at teardown; the harness fflushes
  per check so output is never lost.
- **Headless harness runs** (finals10m + 3P link Qt Multimedia → they pull in
  the Qt GUI **platform plugin**). Always run them with **both** the Qt `bin`
  on PATH **and** `QT_QPA_PLATFORM=offscreen`, e.g.
  `QT_QPA_PLATFORM=offscreen ./release/finals_tests.exe`. Without the platform
  plugin these console tests pop a modal *"no Qt platform plugin could be
  initialized"* dialog that **blocks the process and yields an empty output
  file** — never infer PASS from exit code alone; always capture the
  `=== N checks, M failures ===` line. (reliability is QtCore-only and needs
  neither.) Current baselines: finals10m **143/0**, reliability **864/0**,
  3P finals **189/0**.
- Diagnostic logging is gated behind `APPSETTINGS.getDeveloperMode()` —
  production runs are near-silent; dev mode restores per-shot traces.

## Branch structure (as of the stabilisation baseline)

`main` ← PR #11 `feature/3p-finals` (finals report redesign) ← PR #12
`feature/techaim-dialogs` (dialog framework) ← `chore/stabilisation`
(S1-S8 cleanup baseline). Merge in that order, then branch
**`feature/3p-training`** (next planned feature) off the result.
Stale: PR #8 (`feature/right-panel-telemetry`) predates the merged
floating-windows work — review/close manually; local
`feature/floating-windows` tracks a deleted remote.

## Known deferred work

- 25m Pistol disciplines: unimplemented; which events are in scope needs the
  user's decision.
- ~~50m Rifle `radOf10Ring = 5.2`: needs official rulebook confirmation.~~
  **CLOSED (M4.6).** ISSF Rule Book 2026 rule 6.3.4.2 gives the 50 m rifle 10
  ring as **10.4 mm DIAMETER**, so the 5.2 mm RADIUS is correct. All four
  `calculateShootingSocre()` branches were checked against the rulebook and all
  four are right. See `docs/architecture/rms-target-geometry-source-register.md`.
- Licence-expiry check: DISABLED (commented in LoginPage, rewritten against
  the dialog framework). Re-enabling needs separate approval + a licence
  test fixture (`MODREADER.isValidLicence()` path).
- UI backlog: ModConnectorDialog restyle; remaining rebrand pages
  (SettingsPage, PdfPage, `isDefaultIcon` still used in ~6 places).
- C++ qDebug/qInfo audit + QLoggingCategory channels (QML side done in S4).
- Range Management System (RMS): future separate app (SIUS Data/Rank model);
  lane app already has UDP shot broadcast + fromServer hooks; finals report
  header reserves Lane/Target ID.
