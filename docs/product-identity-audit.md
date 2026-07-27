# Product Identity Audit (P0 Phase A)

**Baseline:** branch `feature/training-lab`, HEAD `e709bb9`, origin in sync,
no tracked uncommitted changes, `config.ini` gitignored.

**Method:** case-insensitive repository sweep for `seta` / `SETA` / `seeds` /
`Seta.exe` plus the identity, storage, single-instance, installer and metadata
terms listed in the P0 brief. Build output (`release/`, `debug/`), the vendored
`ModReader/3rdparty/`, generated `ui_*.h` / `moc_*` files and the background
task's worktree under `.claude/worktrees/` are excluded from counts.

**No global search-and-replace was performed.** Every hit below is classified
individually.

---

## Classification key

| Class | Meaning | Action |
|---|---|---|
| **A** | User-facing product identity | Change to Tech Aim now |
| **B** | Technical identity with data/upgrade consequences | Migrate carefully |
| **C** | Valid SETA hardware/supplier reference | **Retain unchanged** |
| **D** | Internal legacy symbol, not user-visible | Leave unless clearly safe |
| **E** | Future SETA OEM configuration | Document only |

---

## Headline findings

Three findings materially change the P0 plan:

1. **The application data identity is already Tech Aim.** `main.cpp:135-136`
   has set `organizationName` and `applicationName` to `TechAim` since the M0
   reliability work. `QStandardPaths::AppLocalDataLocation` therefore already
   resolves to `%LOCALAPPDATA%\TechAim\TechAim\`, and every journal, snapshot
   and recovery candidate ever written by a reliability-layer build is already
   in the Tech Aim root. **There is no Seta-rooted session store to migrate.**
   See "Phase E reassessment" below.

2. **Restart is executable-name agnostic.** `main.cpp:376` relaunches via
   `QCoreApplication::applicationFilePath()` — the process re-launches *itself*
   by resolved path. Renaming the executable requires **no change** to the
   restart/mode-switch flow, and no code anywhere hardcodes `Seta.exe`.

3. **The main window has no product title.** `main.qml:11` is
   `title: qsTr("Hello World")`. This is the taskbar and window-caption string
   — the single most visible identity defect in the build, and release-blocking.

---

## A — User-facing product identity (change now)

| Location | Current | Notes |
|---|---|---|
| `main.qml:11` | `title: qsTr("Hello World")` | Window + taskbar caption. Blocking. |
| `Seta.pro` (filename) | target → `Seta.exe` | qmake derives `TARGET` from the project filename. |
| `Seta.pro:7` | `QMAKE_TARGET_PRODUCT = "SETA"` | Feeds the Windows version resource product name. |
| `Seta.pro:6` | `VERSION = 4.0` | Disagrees with `APP_VERSION_STR = 0.9.0`. Two version truths. |
| `SummaryReportView.qml:275` | `softwareVersion: "Seta 4.0"` | Printed on the summary report. |
| `CoachPrintView.qml:420` | `softwareVersion: "Seta 4.0"` | Printed on the coach report. |
| `FinalsReportView.qml:437,542,613,783` | `softwareVersion: "Seta 4.0"` | Printed on all four finals report pages. |
| `src/finals/FinalsReportBuilder.cpp:172` | `"Seta 4.0"` fallback | Report builder default. |
| `customprint.cpp:234,242` | `:/images/logo/seta.png` | SETA logo drawn into printed output. |
| `SettingsPage.qml:477` | `"TechAim " + version` | Prose should read `Tech Aim`. |

**Not found (already correct):** PDF document metadata. `customprint.cpp`
already sets `setTitle`/`setCreator` to *Tech Aim Electronic Target Control*
on every writer. No change required.

## B — Technical identity with data/upgrade consequences

| Location | Current | Consequence |
|---|---|---|
| `main.cpp:161` | `QDir::temp()/"tachus_seta.lock"` | Single-instance identifier. Renaming without recognising the legacy name would let a legacy build and a Tech Aim build run concurrently against the same data. |
| `appsettings.cpp:302,315,327,339` | `QSettings("Seta", "shootingApp")` | Registry-backed EULA acceptance + last load-file path, under `HKCU\Software\Seta\shootingApp`. |
| `appsettings.cpp:298,312,324,336` | `QSettings("Tachus", "shootingApp")` | Same keys for the older Tachus brand; selected by `m_brandName`. |
| `logfile.h:26` | `%TEMP%/tachus_log*.log` | Diagnostic log filename prefix; referenced by support instructions. |
| `main.cpp:300` | `%TEMP%/qModMaster.ini` | Vendored Modbus tool settings. Harmless, unreachable UI. |

## C — Valid SETA hardware/supplier references (RETAIN)

These name the **SETA range-hardware / lane-server integration**, not the
product. They must not be renamed.

| Location | Symbol |
|---|---|
| `appsettings.{h,cpp}` | `getSetaServerPath` / `setSetaServerPath` / `addSetaServerPathToWatcher` |
| `appsettings.{h,cpp}` | `checkForSetaServerSettingFile`, `checkForSetaLaneConcrolFile` |
| `appsettings.{h,cpp}` | `uploadSetaServerSettings`, `uploadSetaServerSettingsCSV`, `m_setaServerPath` |
| `appsettings.cpp:962-968` | `setSetaLaneShootDataFilePath`, `setSetaLaneScoreSummaryFilePath`, `setSetaLaneEachScoreDataFilePath`, `setSetaLaneStatusPath` |
| `appsettings.cpp:1100,1135` | `matchDetailsSetaModification` signal |
| `LoginPage.qml:153` | `onMatchDetailsSetaModification` handler |

**Judgement call — EULA artwork.** `images/loginPage/End User Agreement
SETA-1.png` / `-2.png` (referenced `LoginPage.qml:2214,2220`) are rendered
images of a SETA-era end-user agreement. These are **legal documents, not
branding**, and cannot be corrected by renaming a file. Flagged for legal
review; see "Deferred / needs a human decision".

## D — Internal legacy symbols (leave for now)

`m_brandName = "seta"` (`appsettings.cpp:37`) with the `tachus`/`seta`
switch at lines 296-342; `receiverTachus.*`, `TachusWidget`,
`src/bridge/tachusshotbuilder.*`, the `MODREADER` context property. These are
internal C++ symbol names with no user-visible surface. Renaming them is a
large mechanical change with real regression risk and zero identity benefit —
explicitly out of P0 scope.

`m_brandName` is, however, the vestigial ancestor of a build-flavour switch
and is superseded by the Phase B `ProductIdentity` / `BuildFlavour` design.

## E — Future SETA OEM configuration (document only)

The future OEM edition needs, and must obtain from configuration rather than
from a forked codebase: display name, executable base name, publisher,
organisation, application id, logo asset set, colour theme, default language.
The Phase B `ProductIdentity` structure carries exactly these fields, and
`BuildFlavour::SetaOem` is reserved as a compile-time value with no runtime
selector, no blue theme and no separate executable output.

---

## Phase E reassessment — what actually needs migrating

Because org/app identity has been `TechAim` since M0, the elaborate
Seta→Tech Aim data migration described in the brief has **no source data to
migrate for session storage**. Claiming otherwise would be inventing a
migration for a directory that was never written.

What genuinely carries legacy state:

| Item | Legacy location | Status |
|---|---|---|
| Session journals / snapshots / recovery | already `%LOCALAPPDATA%\TechAim\TechAim\` | **No migration needed** |
| Pre-M0 journals in the process CWD | handled since M0 by `StoragePaths::migrateLegacyJournals()` → `Sessions/Archive/Legacy` | **Already implemented** |
| EULA accepted flag, last load-file path | `HKCU\Software\{Seta,Tachus}\shootingApp` | Needs read-through fallback |
| `config.ini` (app_mode, COM port, network share) | beside the executable | Path unchanged by the rename |
| Single-instance lock | `%TEMP%\tachus_seta.lock` | Needs a Tech Aim id + legacy detection |

The correct scope is therefore a **narrow settings-compatibility shim plus a
single-instance migration**, not a session-data migration. Sessions, journals,
snapshots and recovery candidates are untouched by the identity change, which
is also why no journal can fail hash validation as a result of it.

---

## Deferred / needs a human decision

1. **EULA artwork** still shows a SETA-era agreement naming a different
   entity. Replacing it is a legal question — the publisher is now JAC
   SHOOTING SOLUTIONS (PTY) LTD — not an engineering one. Must be resolved
   before any public beta.
2. **`VERSION = 4.0` vs `APP_VERSION_STR = 0.9.0`.** Two disagreeing version
   truths. P0 standardises the Windows version resource on 0.9.0.
3. **Registry key ownership.** Whether to write new Tech Aim registry values
   or keep reading the legacy ones indefinitely is a support-policy decision.
   P0 implements read-legacy / write-TechAim.
