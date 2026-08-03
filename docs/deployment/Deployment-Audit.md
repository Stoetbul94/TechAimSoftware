# Tech Aim — deployment and packaging audit

**Branch:** `release/0.9.0-deployment-prep` · **Source commit:** `4151620`
**Date:** 2026-08-03 · **Scope:** audit only — no runtime code was changed.

This is Part 1 of the deployment-preparation phase. Everything below was read
from the repository or measured on the development machine. Where something
could not be measured, it says so.

---

## 1. Packaging scripts

| Script | Role |
|---|---|
| `tools/release/build-rc1-package.ps1` | The **only** package builder. Despite the name it builds RC1, RC2, RC2a and the rollback package — `-SourceRepo` points it at a worktree, `-PackageName` renames the output, so the rollback build and the RC build cannot diverge in *how* they were produced. |
| `tools/deployment/New-DeploymentPrep.ps1` | **New.** Assembles `dist\deployment-prep\` from artefacts that already exist. Builds nothing. |
| `tools/deployment/Verify-Deployment.ps1` | **New.** Verifies one extracted package against a manifest. |
| `tools/deployment/Test-AppDataUpgrade.ps1` | **New.** First-run / restart / upgrade / rollback / removal drill. |
| `tools/deployment/Make-SupportBundle.ps1` | **New (v2).** Corrects two defects in the shipped v1 — see §10. |
| `tests/release/check_deployment_package.py` | Repository-side deployment audit, 85 checks. |

**Finding PKG-001 (cosmetic).** The builder is still named `build-rc1-package.ps1`
while producing RC2a. The name is now misleading. Renaming it is a low-risk
follow-up but touches release tooling mid-flight, so it is recorded rather than
done during a preparation phase.

## 2. Portable ZIP process

1. Verify `release/TechAim.exe` exists (fails loudly if not built).
2. Clean the output folder and ZIP.
3. Copy the binary.
4. Write a **fresh** `config.ini` — never the developer's, so no local mode or
   machine value can leak.
5. Copy field-test documents, licence and third-party notices.
6. Copy the support-bundle tool.
7. Deploy the Qt runtime (see §3).
8. Ensure Qt Multimedia DLL + plugins (loaded at runtime for finals audio, so
   not always visible in the import graph).
9. Scrub `*.pdb`, `*.ilk`, `*.exp`, `*.lib`, `vc_redist*.exe`.
10. Zip, hash, write `<name>.zip.sha256`.

The package copies an **explicit allow-list**, never `release/` wholesale —
`release/` also holds object files, moc/qrc output and the developer config.

## 3. windeployqt behaviour and the fallback

**windeployqt fails on this machine.** It reports *"Unable to find the platform
plugin"* even though `plugins/platforms/qwindows.dll` is present — from every
working directory, and with `--plugindir` given explicitly.

The builder therefore runs windeployqt **first**, records the outcome, and when
the platform plugin is absent afterwards falls back to an explicit allow-list.
The package prints which path was used (`deployed by: …`). Every RC so far was
built through the **fallback**.

This is a deviation and is documented as one. It is safe because the allow-list
is not a guess: the Qt module set is exactly what windeployqt itself resolved
("To be deployed"), and `tests/release/check_deployment_package.py` verifies
every shipped top-level directory against a documented reason.

**Finding PKG-002.** The root cause of the windeployqt failure is unknown. It
should be diagnosed before a production release so deployment does not depend
indefinitely on a hand-maintained list. Not a blocker for the physical retest.

## 4. DLL allow-list

Qt: Core, Gui, Qml, QmlModels, QmlWorkerScript, Quick, QuickControls2,
QuickControls2Impl, QuickTemplates2, QuickShapes, QuickDialogs2,
QuickDialogs2Utils, QuickDialogs2QuickImpl, QuickLayouts, Widgets, Network,
SerialPort, Xml, Svg, Multimedia, MultimediaQuick, Charts, ChartsQml, OpenGL,
OpenGLWidgets, Concurrent.

Compiler runtime: `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`.
Without these the application does not start on a machine that never had a
compiler — i.e. every real deployment machine. `Qt6OpenGLWidgets.dll` was
missing from an earlier package and was caught by the audit, not by launching.

## 5. Qt plugins included

`platforms` (qwindows.dll — no window without it), `imageformats`, `iconengines`,
`styles`, `multimedia` (finals audio), `tls`, `networkinformation`, `generic`.

QML modules: `QtQuick`, `QtQml`, `QtCharts`, and **only** the
`Qt5Compat/GraphicalEffects` submodule. `QtQuick/VirtualKeyboard` is deleted
after deployment — roughly 1500 files the application never imports.

## 6. Executable path

`TechAim.exe` at the **root** of the portable folder. The Qt runtime, plugin
directories and `config.ini` sit beside it. The application is started by
running it directly; there is no launcher and no shortcut in the ZIP.

## 7. config.ini creation and loading

- **Created** by the package builder as a fresh literal (`app_mode=Live`,
  `developer_mode=0`, `is_single_decimal=1`, motor times), written UTF-8
  **without BOM**. A BOM makes the first INI section header unreadable to
  `QSettings`, which is why the audit checks for it explicitly.
- **Loaded** at `main.cpp:321` — `new AppSettings("config.ini")`, a **relative**
  path resolved against the process working directory.

**Finding CFG-001 (operational).** Because the path is relative, starting
`TechAim.exe` with a different working directory (for example from a Start Menu
shortcut with the wrong "Start in" field) silently loads **no** configuration
and falls back to defaults. This has not caused a field failure — operators
launch from the folder — but any future installer or shortcut **must** set the
working directory to the install folder. Recorded in the install manifest as a
requirement. Not fixed here: changing the resolution rule is a runtime change.

## 8. AppData location

Root: `QStandardPaths::AppLocalDataLocation` — on Windows
`%LOCALAPPDATA%\<organisation>\<application>`. Both names are `TechAim`
(`ProductIdentity.cpp:23`, `main.cpp:148-149`), so the real root is:

```
C:\Users\<user>\AppData\Local\TechAim\TechAim
```

**Measured on the development machine**, the tree is exactly as specified:
`Sessions\{Current,Archive,Corrupt}`, `Backups`, `Reports`, `Exports`, `Logs`,
`Settings`, `SupportBundles`, `DerivedIndexes`, plus Qt's own `cache`.

`StoragePaths` is the single owner of these paths. It never uses the executable
directory and never the process working directory.

Three **stale sibling roots** exist on the development machine from earlier
names and other applications: `TechAim Electronic Target`,
`TechAimLaneSimulator`, `TechAimRangeManager`. They are not created by the
current build. The uninstall guide names them so an operator can decide.

## 9. Logs location — two of them

| Writer | Location | Content |
|---|---|---|
| `StoragePaths::logsDirectory()` | `…\TechAim\TechAim\Logs` | **empty — 0 files measured** |
| `LogFile` singleton (`logfile.h:26`) | `%TEMP%\tachus_log<ddMMyyyy-hhmmss>.log` | **all operational logging — 88 files measured** |

**Finding LOG-001 (runtime — recorded, NOT fixed).** The storage layer creates
an AppData `Logs` directory that nothing writes to, while the active logger
writes to the Windows TEMP directory. Consequences:

- `%TEMP%` is cleared by Windows Disk Cleanup and Storage Sense, so diagnostic
  evidence can disappear before anyone collects it;
- a new log file is created **per launch**, so 88 accumulated on one machine;
- any tool that looks only in AppData finds nothing — which is exactly what
  happened to the support bundle (§10).

This is a runtime defect. Per the phase rules it is **recorded and left alone**;
the support tool was adapted to reality instead. It belongs in a runtime branch
after the physical retest.

## 10. Support-bundle tool

**Finding SUP-002 (support tooling — FIXED in v2).** v1 set
`$appData = $env:LOCALAPPDATA\TechAim` — one level **above** the real root. Every
collection path (`Logs`, `Sessions`, `SupportBundles`) therefore pointed at a
directory that does not exist. Verified: `…\TechAim\Logs` **absent**,
`…\TechAim\TechAim\Logs` **present**. The bundle shipped with no logs, no
sessions and no crash data — only the identity file and a sanitized config.

**Finding SUP-003 (support tooling — FIXED in v2).** Even at the corrected path
the AppData `Logs` directory is empty (LOG-001), so the bundle would *still*
have collected nothing. v2 collects **both** locations and states which one had
content.

Also corrected in v2: the known-limitations filename was pinned to the RC1 name;
the manifest search did not know the neutral `release-manifest.json` name.

Support tooling is explicitly in scope for this branch, so v2 is a fix, not a
deferral. The v1 script under `tools/release/` is left untouched because it is
part of the accepted RC1/RC2 kits.

## 11. Report-export location

`PdfExporter::exportToFile` (`src/bridge/pdfexporter.cpp:47`) defaults to
`QStandardPaths::DocumentsLocation` — the user's **Documents** folder — not the
AppData `Reports` directory. This is deliberate and correct for an operator
saving a report to hand over; `Reports`/`Exports` under AppData remain available
to the storage layer. Noted so the uninstall guide can tell an operator where
their exported PDFs actually are.

## 12. Rollback process

`docs/release/0.9.0-rc1-rollback.md`, with a real rollback package built from
commit `747b9a7` by the same builder:
`TechAim-Rollback-747b9a7-Windows-x64.zip`, SHA-256
`3051552E78E5868271E1D8CD6DC1430193577FDF6AE4A36595FE3CCB6033C3CB` — re-verified
during this audit.

Rollback is a folder swap. User data is untouched, and the upgrade drill proves
it (§ Part 5 results).

## 13. Installer framework

**None exists.** A repository-wide search for NSIS (`.nsi`), WiX, Inno Setup
(`.iss`), MSIX/AppX manifests and installer build steps returned no match; the
only hits were clangd index caches and the words "design"/"assign". `Seta.pro`
has no install target beyond qmake defaults.

The supported deployment method is the **portable ZIP**. Options are compared in
[Installer-Options.md](Installer-Options.md); a framework-neutral install
manifest is generated into `dist\deployment-prep\installer-candidate\`.

## 14. Code signing

**Not configured.** No certificate, no `signtool` step, no `codesign`, no
Authenticode metadata in the build. Every binary and package produced so far is
**unsigned**.

Consequences that must not be glossed over:

- Windows SmartScreen will warn on first run of a downloaded package.
- No claim of code-signing or SmartScreen reputation may be made about any
  Tech Aim artefact, including any future installer.
- Signing needs an OV or EV code-signing certificate, which is a **procurement**
  decision, not a build change.

## 15. Crash dumps

**Not captured.** No `MiniDumpWriteDump`, no `SetUnhandledExceptionFilter`, no
Breakpad or Crashpad. `StoragePaths::supportBundlesDirectory()` exists and the
support bundle looks there, but nothing writes crash data into it — measured: 0
files.

A crash today leaves only the Windows Application event log and whatever the
`%TEMP%` log captured before the process died. Adding a crash handler is a
runtime change and is out of scope for this branch.

## 16. Upgrades and AppData

Upgrade preserves user data. **Proven, not assumed** — see
[AppData-and-Upgrade-Safety.md](AppData-and-Upgrade-Safety.md): 35 checks, 0
failures, with the data root reconciling at exactly 142 files before and after.

The mechanism is structural: the data root is derived from the Windows shell,
not from the program folder, so replacing or deleting the program folder cannot
reach it. No package contains a `.jsonl`, and the verification script fails if
one ever does.

---

## Findings summary

| ID | Area | Severity | Status |
|---|---|---|---|
| PKG-001 | Builder script name says RC1 but builds RC2a | Cosmetic | Recorded |
| PKG-002 | windeployqt platform-plugin failure, root cause unknown | Low | Recorded |
| CFG-001 | `config.ini` is loaded by relative path | Medium | Recorded; install manifest requires a working directory |
| LOG-001 | Operational log goes to `%TEMP%`, AppData `Logs` unused | **Medium** | **Recorded — runtime, not fixed here** |
| SUP-002 | Support bundle read the wrong AppData level, collected nothing | **High** | **Fixed** in `Make-SupportBundle.ps1` v2 |
| SUP-003 | Support bundle did not collect the active `%TEMP%` log | **High** | **Fixed** in v2 |

No runtime source file was modified in this branch. LOG-001 and CFG-001 are
runtime concerns and were deliberately left for a post-retest branch, so the
binary Arnold tests in five days is byte-identical to the accepted RC2a.
