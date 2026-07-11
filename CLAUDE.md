# Seta10 / TechAim electronic target software

ISSF electronic target scoring application. Qt Widgets + QML hybrid (C++ backend, QML frontend), Modbus RTU/TCP to target hardware via a vendored QModMaster fork (`ModReader/`). Originally "Tachus"/"Seta", rebranding to TechAim (see brand assets in `images/logo/`).

**Before doing anything else this session:** run `git log --oneline` and read the last 3-4 commit messages in full (`git log -4 -p` or `git show <hash>`). They document what was done and *why* in detail - this isn't boilerplate, it's the actual handoff notes from the previous work. Then read `seta10_ISSF_codebase_analysis.md` in the repo root for the full architecture writeup and findings.

**Before touching anything 3P-related** (50m Rifle 3 Positions), read
`docs/3p-discipline.md` first. 3P behaves differently from every other
discipline — its data-model rules (which shot store to read), workflow state
machine, ISSF integer-primary display format, and hard invariants are all
documented there, along with the full fix history. Everything 3P-specific is
gated on `is3PMatch`; other disciplines must remain untouched by 3P logic.

## Architecture, in short

- `ModReader/qModMaster.pro` is `include()`d directly into `Seta.pro` - one binary, not two.
- `TachusWidget` (`ModReader/forms/tachuswidget.cpp/h`) is the bridge between Modbus hardware data and QML, exposed as the `MODREADER` context property in `main.cpp`. `AppSettings` is `APPSETTINGS`, `CustomPrint` is `CUSTOMPRINT`.
- The actual ISSF ring-geometry-to-score math lives in QML, not C++: `CenterPane.qml::calculateShootingSocre()`. This is the single most correctness-critical function in the codebase.
- `Theme.qml` (new) holds the TechAim brand colors, extracted directly from the logo SVGs - not guessed. Instantiated once in `main.qml` as `Theme { id: theme }`, visible to every descendant page via QML ancestor-scope property lookup (same mechanism `main.qml`'s `isDefaultIcon`/`gameRange`/`gameMode` already use - deliberately not a QML singleton/module, since that pattern has never been used anywhere in this codebase).

## Known constraints from the previous session

- Everything was done in a sandboxed environment with **no Qt installed** - nothing here has ever been compiled or visually tested. Every edit was verified by careful direct reading (brace-by-brace, cross-referencing property names) rather than a build. **If Qt is available locally, building this project for the first time would be genuinely valuable** - it would be the first real compiler verification any of this has had.
- This codebase uses CRLF line endings throughout (Windows-authored), except `ModReader/src/modbusadapter.cpp/h` which were already pure LF before any of this work started (vendored from a Linux-originated source) - match whichever convention a file already uses.
- `git log` only shows real history from this work forward. The original upstream commit (`cfae758`, "Initial commit", 2019-01-01) is a single squashed snapshot - there's no earlier evolutionary history to mine.
- `origin` remote is `https://github.com/Stoetbul94/TechAimSoftware.git`. `upstream` is the original source repo. The previous session could not push (no credentials in that sandbox) - this should work normally from a local environment with your own git auth.

## Pending decisions (need the user's input, don't guess)

1. **25m Pistol disciplines** - currently unimplemented anywhere (confirmed: every `gameRange` check in the codebase is strictly `10` or `50`). Need to know which specific events are in scope (Rapid Fire / Standard / Sport / Women's aren't identical) before starting.
2. **ISSF Finals format** - start-from-zero scoring, per-shot countdown timing, progressive elimination - not implemented anywhere. Need to know if it's in scope for this pass.
3. **50m Rifle's `radOf10Ring=5.2` constant** in `calculateShootingSocre()` - couldn't be verified against available rule text. Needs the official rulebook or a physical calibration test.

## UI rebrand status

Done: `Theme.qml` foundation, `Header.qml` (first page, proof of direction). Still on old Tachus/Seta branding and ad-hoc hardcoded colors: `LoginPage.qml`, `ShootingPage.qml`, `SettingsPage.qml`, `CenterPane.qml`, `RightPanel.qml`, `MatchReport.qml`, `MatchReportInfo.qml`, `SummaryPage.qml`, `LeftPanel.qml`, and others - same `theme.xxx` pattern as `Header.qml`, same approach. `LoginPage.qml` is next (most visible screen after Header), but it's ~2000 lines with real functional coupling (licence validation, Modbus connection state, game-mode persistence all wired into the visual tree) - go carefully, and the user has said they want to review direction page-by-page rather than a single large batch.

`isDefaultIcon` (the old Tachus/Seta brand toggle, declared in `main.qml`) is still used in 6+ places outside `Header.qml` (`LoginPage.qml`, `PdfPage.qml`, `PdfSeriesPage.qml`, `RightPanel.qml`) - replacing those is part of the page-by-page work, not done yet.
