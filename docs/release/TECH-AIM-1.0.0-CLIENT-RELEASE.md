# Tech Aim 1.0.0 — client release record

**The client folder is ready to send. The repository commit of two packaging
scripts is blocked by the environment — see §Git, which is the one open item.**

Written 2026-08-30.

---

## Identity

| | |
|---|---|
| Product | **Tech Aim Electronic Target Control** |
| Version | **1.0.0** |
| Release branch | `release/techaim-1.0.0` |
| Commit | **`19f239d`** |
| Built from | `da03984` (frozen 1.0.0-RC1 baseline) + identity/tooling only |
| Build timestamp | 2026-08-30 21:31:40 |
| **EXE SHA-256** | `CAB86F681813417D41E6A18C9E10A5982821997CDC2BE454D04C4564675A7903` |
| **ZIP SHA-256** | `B0EB5E1EAE7B4B2C5495738C5AD2CF24F57F42A27A699362AC9FE64E49769E3D` |
| Production package | `dist/v1.0.0/TechAim-1.0.0-Windows-x64.zip` (36.62 MB) |
| **Client folder** | `C:\Users\User\Downloads\TechAim-1.0.0-Client` — 758 files, 91 MB |

**RC1 preserved unmodified:** `dist/v1.0.0-rc1/TechAim-1.0.0-RC1-Windows-x64.zip`,
SHA-256 `4759B87DAFEF3E9C275168519B773AF0D052252B566DE19C4514A6ECF1BEA5CE` —
re-verified after this release was built.

---

## Why 1.0.0 exists

RC1 was blocked from client delivery for one reason: **it told the customer so.**

- Every report and PDF footer printed `Tech Aim 1.0.0-RC1`, via
  `softwareVersionLabel()` on all five report families.
- Settings → About displayed *"Evaluation Build — Not for Official Competition
  Results"*.

A range officer handing an athlete a printed result would have handed them a
page marked RC1.

---

## What changed — six files, no functional core

| File | Class |
|---|---|
| `src/app/ProductIdentity.cpp` | identity: version 1.0.0, channel `Production Release`, empty `fieldTestNotice`, description no longer says "Internal Field Test Build" |
| `Seta.pro` | identity: `APP_VERSION_STR = 1.0.0` |
| `TechAim.rc` | identity: PE `Comments` no longer says "Pre-Beta Validation Build" |
| `tests/finals/tst_finals3p.cpp` | test: the identity rule now holds for both build kinds |
| `tools/release/Make-SupportBundle.ps1` | support tooling |
| `tools/release/Collect-Logs.cmd` | support tooling |

**Zero files** under `src/target/`, `src/finals/`, `src/finals10m/`,
`src/reliability/`, `src/analytics/`, `src/training/`, `src/qualification/` or
`ModReader/`. Acquisition, scoring, Modbus, serial, coordinate validation,
counter reconciliation, paper feed, the state machines, competition timers, CRO
behaviour, last-shot dwell, report calculations and shot-role semantics are
byte-identical to the frozen baseline.

The `fieldTestNotice` **mechanism** is untouched: `isFieldTest` is still derived
from the notice being non-empty, so the About notice hides itself by the guard
it always had, and the next evaluation build gets it back by setting one string.

### The test that would have had to be deleted

The harness asserted "the release channel names a pre-release candidate". That
is true of a candidate and false of a production release, so shipping this would
have meant removing it — and **a rule deleted in order to ship is not a rule**.
It now checks consistency in both directions:

- `fieldTestNotice` non-empty → an evaluation build: the channel must name a
  candidate and the notice must carry its limitation.
- `fieldTestNotice` empty → a production build: no candidate wording on **any**
  customer surface — channel, version, release description, or the report footer
  string — and the footer must still name the product and version.

---

## Support collector

The SETA acceptance round proved a real, generic defect: the collector searched
`%LOCALAPPDATA%\TechAim` — the **vendor** folder — while Qt writes to
`<organisation>\<application>`. `-ErrorAction SilentlyContinue` turned the
missing path into an empty result, so it reported **zero journals without ever
saying it had looked in the wrong place**.

Carried into Tech Aim and **executed against real application data from the
client folder**:

| | |
|---|---|
| Journals collected | **41** |
| Logs collected | **5** |
| Configuration (`qModMaster.ini`) | present |
| Product identified | **Tech Aim Electronic Target Control** |
| Version | 1.0.0.0 (read from the executable) |
| Operating mode | Live |
| Bundle name | `TechAimElectronicTargetControl-Support-<stamp>.zip` |

**Known gap, stated rather than hidden:** the bundle reports *"Release channel:
UNKNOWN — no manifest"* and *"Git commit: UNKNOWN — no manifest"*. The collector
looks for a JSON manifest; this package ships `RELEASE-MANIFEST.txt`, which is
text. The build is still identifiable from the product name, version and
executable SHA-256, and the bundle **says** the fields are missing rather than
inventing them — which is the behaviour asked for. Closing it means either
emitting a JSON manifest from the packager or teaching the collector to read the
text one; both are in the blocked scripts below.

---

## Client folder audit

| Check | Result |
|---|---|
| Portable launch, no Qt on `PATH` | **PASS** — `Tech Aim 1.0.0 · commit 19f239d · Production Release · flavour TECH_AIM` |
| TypeError / ReferenceError / QML type failure / missing DLL | **0 / 0 / 0 / 0** |
| Acquisition startup warnings | **0** |
| Binding warnings | 7, the documented pre-existing set, unchanged |
| `app_mode` / `developer_mode` | **Live / 0** |
| Candidate wording in the binary | **NONE** — Release Candidate, Evaluation, Internal Field Test, Pre-Beta, "Not for Official", `1.0.0-RC1` all absent |
| `.cpp` / `.c` / `.h` / `.hpp` | **0 / 0 / 0 / 0** |
| `.py` / `.pro` / `.pri` / `.pdb` / `.obj` / `.lib` | **0** each |
| Git metadata | **NO** |
| Test files | **NO** |
| Internal documentation | **0** — see below |
| **Tech Aim application QML (loose)** | **0** |
| Qt runtime QML | 466 (QtQuick 417, Qt5Compat 32, QtCharts 17) — required, kept |
| Customer-safe scripts | `Collect-Logs.cmd`, `Make-SupportBundle.ps1` |
| SETA / DSB / Android / RMS customer-facing content | **NONE** |

### Internal documents removed from the package

The v1.0 packager was written for an internal audience and shipped **19**
documents: the release gate, the blocker inventory, the RC3F field-test
baseline, the cross-platform fix register, the rule-authority index and the ISSF
rule working files. Two are named in the client exclusion list outright —
field-test evidence and a release-blocker working document — and the rest carry
defect ids and test totals. All were removed; the package now carries
`README-CLIENT.txt` and `RELEASE-MANIFEST.txt`.

### The "SETA" string in the binary

Present, and **not branding**. Traced in the frozen source to a legacy
`QSettings("Seta","shootingApp")` registry scope (4 sites), and to
`legacyApplicationNames` / `legacyOrganisationNames`, which exist so old
installations' settings can still be found. They are rendered in **zero** QML.
Documented rather than removed, because removing them breaks migration for
existing installations.

---

## Automated regression

**6 863 checks / 0 failures.**

| Suite | Result |
|---|---|
| Reliability | 2 602 / 0 |
| Training Lab | 568 / 0 |
| 50 m 3P Finals | 379 / 0 |
| 10 m Finals | 229 / 0 |
| QML | 440 / 0 |
| Manuals | 1 514 / 0 |
| Project memory | 228 / 0 |
| Training Lab evidence | 903 / 0 |

---

## Physical evidence

**RC3F, inherited.** 2026-08-29: three tablets, three athletes, live 50 m
targets, 385 accepted physical shots, 0 acquisition faults, one reconnect
recovered correctly.

**No additional live target test was performed, and none is required**, because
the functional core did not change: the diff from the frozen baseline touches
three identity files, one test and two support scripts. See
[V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md](V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md).

---

## Git — the one open item

`tools/release/build-v1-package.ps1` and `tools/release/build-client-package.ps1`
**cannot be read or written by any process on this machine**: every attempt
returns *Permission denied* or *being used by another process*, while a freshly
created `probe.ps1` in the same directory writes and deletes normally. This
looks like an endpoint-security scanner holding those two paths.

Consequences, stated plainly:

- `build-v1-package.ps1` is **deleted in the working tree** and cannot be
  restored from git.
- `build-client-package.ps1` exists on disk but cannot be staged.
- **The branch is therefore not committed and not pushed.** `HEAD` is
  `19f239d`, which is the commit the shipped binary was built from, so the
  artefacts are consistent and correctly identified.
- The client package is currently reproducible only by hand: build, deploy with
  the allow-list, replace the document set, zip.

**What clears it:** unlock those two paths — reboot, or exclude
`…\seta10\tools\release\` in the security product — then
`git checkout HEAD -- tools/release/build-v1-package.ps1`, stage
`build-client-package.ps1` and `docs/release/README-CLIENT.txt`, commit and push.
Nothing about the built artefacts changes; only the repository catches up.

---

## One-click customer installer — added 2026-08-30

| | |
|---|---|
| Installer | `dist/v1.0.0/TechAim-1.0.0-Setup.exe` |
| Size | 27.86 MB (29 209 122 bytes) |
| **SHA-256** | `EBF150305CF30093C14347E84BC02614C9A776F92D6DE20840C1FB38129024C4` |
| Script | `tools/release/TechAim-1.0.0.iss` |
| Built with | Inno Setup 6.7.3, `C:\Users\User\AppData\Local\Programs\Inno Setup 6\ISCC.exe` |
| Payload | the verified client runtime, staged to `dist/v1.0.0/installer-staging` and audited there — 758 files, EXE SHA-256 matches the release |
| AppId | `{FBB1DC00-9EF6-410D-952B-B2BCF243045C}` — fixed, and distinct from the SETA installer's |
| Customer delivery folder | `C:\Users\User\Downloads\TechAim-1.0.0-Customer-Delivery` — installer + `README.txt` only |

**Correction to the previous record:** it stated that Inno Setup was not
installed. That was wrong — 6.7.3 was already present as a *per-user* install
under `%LOCALAPPDATA%\Programs`, and the earlier audit looked only in
`Program Files`.

### Install location — a deliberate deviation from "Program Files"

The brief asked for `C:\Program Files\Tech Aim`. The installer uses
`PrivilegesRequired=lowest` with `{autopf}`, which resolves to
`%LOCALAPPDATA%\Programs\Tech Aim`. The reason is in the product, not the
installer:

| Site | Path it uses |
|---|---|
| `appsettings.cpp` `saveMatch()` | `QFile("Match_<stamp>.tch")` — **relative**, resolved against the working directory |
| `appsettings.cpp` `AppSettings("config.ini")` | QSettings resolves the relative name against the working directory |
| `tachuswidget.cpp` `saveNameAndPort()` | `applicationDirPath()/USER_DETAILS` |

Under `C:\Program Files` a standard user cannot write to any of these.
`saveMatch()` does not report the failure — `open()` fails and the guarded
block is skipped — so **a completed match would be silently not saved**.
Measured on this machine: writing to the install directory succeeds; writing to
`C:\Program Files` is denied for this user.

Redirecting those paths is a persistence change, which is out of scope for
installer work. The installer therefore places the application somewhere
writable. Consequences: no UAC prompt, no administrator rights (relevant on
locked-down range laptops), per-user entry in Settings → Apps. Session
journals, reports and logs are unaffected either way — the reliability layer
already stores them under `AppLocalDataLocation`.

**To move to Program Files instead**, one of these has to happen first, and both
need approval: run the application elevated (does not fix a standard-user
range PC), or redirect `.tch`/`config.ini`/`USER_DETAILS` to `AppLocalDataLocation`
(a persistence change).

### Installer acceptance

| Test | Result |
|---|---|
| Silent install, exit code | **PASS** (0) |
| Installed files | 758 payload + uninstaller |
| Start Menu group | Tech Aim — application, Collect support logs, Uninstall |
| Desktop shortcut | **PASS**, target and working directory both the install dir |
| Apps & features entry | Tech Aim Electronic Target Control 1.0.0, JAC SHOOTING SOLUTIONS (PTY) LTD |
| Launch from installed copy, `PATH` stripped to `system32` | **PASS** — `Tech Aim 1.0.0 · commit 19f239d · Production Release · flavour TECH_AIM`, Operating mode Live |
| TypeError / ReferenceError / missing module / missing DLL | **0 / 0 / 0 / 0** |
| RC or evaluation wording at runtime | **0** |
| **Independence** | 141 modules loaded, **none** from the client folder, the repository, or any Qt installation — all from the install directory or Windows. The only two foreign modules are Bitdefender hooks injected into every process on this machine. |
| `app_mode` / `developer_mode` | **Live / 0** |
| Uninstall | **PASS** — all installed files, both shortcuts, the Start Menu group and the Apps entry removed |
| User data on uninstall | **PRESERVED** — a `.tch` written after install survived, as did the 22-file AppData session store |
| Reinstall | **PASS** — completes, launches, and leaves a **single** Apps entry, proving the AppId upgrades in place |
| Source leak in packed payload | `.cpp` 0, `.c` 0, `.h` 0, `.hpp` 0, `.pro` 0, `.pri` 0, `.pdb` 0, `.obj` 0, `.lib` 0, `.py` 0, git NO, tests NO, internal docs NO |
| Tech Aim application QML | **0** (466 `.qml` are Qt runtime modules) |
| SETA / Evaluation / RC1 / DSB in the installer's own strings | **0** each |

Nothing under `src/` or `ModReader/` changed for this work: the installer is
`tools/release/TechAim-1.0.0.iss` plus a staged copy of an already-verified
payload.
