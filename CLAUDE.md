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
  binary, run with Qt bin on PATH. Currently **178 checks, 0 failures**.
  The Qt multimedia backend hard-exits at teardown; the harness fflushes
  per check so output is never lost.
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
- 50m Rifle `radOf10Ring = 5.2` in `calculateShootingSocre()`: needs official
  rulebook confirmation or physical calibration.
- Licence-expiry check: DISABLED (commented in LoginPage, rewritten against
  the dialog framework). Re-enabling needs separate approval + a licence
  test fixture (`MODREADER.isValidLicence()` path).
- UI backlog: ModConnectorDialog restyle; remaining rebrand pages
  (SettingsPage, PdfPage, `isDefaultIcon` still used in ~6 places).
- C++ qDebug/qInfo audit + QLoggingCategory channels (QML side done in S4).
- Range Management System (RMS): future separate app (SIUS Data/Rank model);
  lane app already has UDP shot broadcast + fromServer hooks; finals report
  header reserves Lane/Target ID.
