# SETA Windows runtime — build, deploy, validate

**Scope.** How a SETA Release build becomes a folder that runs by
double-clicking `TechAim.exe` on a machine with no Qt, no MinGW and no
developer environment. That folder is the **input to an installer**; it is not
an installer and this document does not build one.

**What this replaces.** Before this pipeline the Qt runtime was assembled by
hand — most recently an emergency copy of 117 DLLs into `release\` — or by the
hand-maintained allow-list in `tools/release/build-rc1-package.ps1`. Both
answer the question "which DLLs does it need?" from memory. This pipeline asks
the binary.

---

## BUILD

```bash
mingw32-make -f Makefile.Release clean && qmake Seta.pro && mingw32-make -f Makefile.Release -j4
```

Run from the repository root with `C:\Qt\6.5.3\mingw_64\bin` and
`C:\Qt\Tools\mingw1120_64\bin` on PATH.

**The `clean` is not optional for a release build.** `Seta.pro` bakes the commit
in at *qmake* time (`GIT_SHA = $$system(git ... rev-parse --short HEAD)` →
`APP_GIT_SHA`). An incremental build after new commits regenerates the Makefile
with the new define but only recompiles sources whose *timestamps* changed, so
the binary keeps reporting the older commit while every other change is
present. The deployment refuses such a build (see VALIDATE), but the fix is to
build clean, not to argue with the gate.

## DEPLOY

```bash
powershell -ExecutionPolicy Bypass -File scripts\deploy\deploy-seta-release.ps1
```

One command, no arguments, repeatable: it wipes and recreates the output, so a
second run cannot inherit anything from the first. Defaults are
`release\` in, `dist\seta-runtime` out; `-BuildDir`, `-OutDir`, `-QtBin` and
`-MingwBin` override them.

The script, in order:

1. **Refuses the wrong binary.** The product flavour is read out of the
   executable's version resource — a Tech Aim build cannot be shipped through
   the SETA path by being in the right directory.
2. **Refuses a stale binary.** The commit compiled into the executable must be
   the current `HEAD`. This is why the manifest can name a commit at all.
3. **Deploys the Qt runtime with `windeployqt`** — Qt's own tool, so the module
   and QML-import resolution is Qt's answer, not ours. See *Why not a plain
   `windeployqt --release`* below.
4. **Prunes** what that mode adds for developers and what this product never
   loads. Every entry is listed in the script with its reason.
5. **Copies the MinGW runtime** (`libgcc_s_seh-1`, `libstdc++-6`,
   `libwinpthread-1`).
6. **Writes nothing else.** No `config.ini`, no user data, no documents: first
   run must create its own state, in the SETA data namespace.
7. **Writes `deployment-manifest.json` and `SHA256SUMS.txt`** — the executable's
   SHA-256, Qt version, toolchain, commit, dirty flag, file and DLL counts, the
   plugin directories and QML modules present, and a checksum for every
   deployed file so a handoff can be verified byte for byte.
8. **Validates**, and fails the whole run if validation fails.

## VALIDATE

```bash
python tests/release/check_deployment.py dist/seta-runtime
```

Runs automatically as the last step of the deploy; re-runnable on any folder,
including one unpacked from a handoff.

The claim it refuses is *"it launched on my machine"*. A developer machine has
Qt on PATH, so a folder that silently borrows `Qt6Core.dll` from `C:\Qt`
launches there and fails on a range laptop with

> The code execution cannot proceed because Qt6Core.dll was not found.

So it walks the **PE import table** of every `.dll` and `.exe` in the folder and
requires each non-system import to resolve *inside* the folder — the same
question Windows asks, asked before the customer does. An import is accepted as
the operating system's only if it is a known Windows DLL or resolves in
`System32`, and never for the Qt, MinGW or OpenGL families, where a copy found
outside the folder is precisely the defect being hunted. Every import the folder
does not carry is **printed by name**, so "Windows provides it" is a visible
claim rather than a silent allowance.

It also checks: the platform plugin, the QML modules and plugin directories the
application needs, the MinGW runtime, SETA identity in the **version resource**
(not merely somewhere in the file — both product names exist as C++ literals in
every build), the SETA icon actually embedded in the shipped executable, that no
source/test/documentation/build artefact is present, that no athlete, session or
configuration state from the build machine came along, and that the manifest
describes the executable that is really there.

### Runtime smoke test

```bash
powershell -ExecutionPolicy Bypass -File scripts\deploy\smoke-test-runtime.ps1
```

The import walk is static; this one runs the thing. It copies the runtime out of
the repository, reduces `PATH` to Windows itself, clears `QT_PLUGIN_PATH` and
the QML import paths, launches, reads the window title back, greps stderr for
the two failures that matter (missing platform plugin, missing QML module),
captures the window with `PrintWindow` as evidence, and counts the folder before
and after — a runtime that writes into its own program directory cannot live
under `Program Files`.

`-Language de-DE` and `-DemoMode` seed a `config.ini` **in the clean room only**
and then assert what the deployed binary did with it, so the German catalogue
and the simulated-shot input source are checked in the shipped artefact rather
than in a development build:

```bash
powershell -ExecutionPolicy Bypass -File scripts\deploy\smoke-test-runtime.ps1 -Language de-DE -DemoMode
```

### Evidence recorded at `5a62a7b`

| Check | Result |
|---|---|
| `check_deployment.py` | **29 checks, 0 failures** |
| `smoke-test-runtime.ps1` | **7 checks, 0 failures** |
| `smoke-test-runtime.ps1 -Language de-DE -DemoMode` | **10 checks, 0 failures** |
| Launched by the Windows shell (`explorer.exe <exe>`) from `%TEMP%` | window opened; folder unchanged at 539 files |
| Landing page rendered with no Qt on `PATH` | `dist\smoke-evidence\seta-runtime-first-launch.png` |
| German UI + DEMO badge from the deployed binary | `dist\smoke-evidence\seta-runtime-first-launch-demo-de-DE.png` |
| Shell launch | `dist\smoke-evidence\seta-runtime-shell-launch.png` |

Neither the user nor the machine `PATH` on the build machine contains Qt, which
is why the earlier double-click of `release\TechAim.exe` failed with
*"Qt6Core.dll was not found"* — and why the shell launch above is a meaningful
test rather than a formality.

## OUTPUT

`dist\seta-runtime\` — generated, never committed (`/dist/` is in
`.gitignore`).

```
TechAim.exe                  the SETA build
Qt6*.dll, libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
opengl32sw.dll               software OpenGL, for machines with no usable GPU driver
D3Dcompiler_47.dll
platforms\qwindows.dll       without this Qt aborts at startup
imageformats\ tls\ multimedia\ styles\ iconengines\ networkinformation\ generic\
qml\                         Qt's own QML modules (the application's QML is inside the exe)
deployment-manifest.json     what this is, and what it was built from
SHA256SUMS.txt               every file
```

Measured at commit `5a62a7b`: **537 files, 40 DLLs, 112.3 MB**.
For comparison, the earlier hand-made emergency copy in `release\` was 1118
files / 117 DLLs / 264 MB, and the 77 extra DLLs were entirely modules this
application never loads (Qt 3D, Quick 3D, WebView, WebSockets, Designer, Test,
Bluetooth, NFC, Positioning, RemoteObjects, Scxml, TextToSpeech,
DataVisualization…). The deployment is a strict subset: it contains nothing the
emergency copy lacked.

Nothing was removed on a hunch. `Qt6PrintSupport` is absent because the
executable's import table does not reference it — `customprint.cpp` includes the
headers but never constructs a `QPrinter`; PDF output goes through `QPdfWriter`
in QtGui.

## Why not a plain `windeployqt --release`

On this Qt 6.5.3 MinGW installation it ends with

```
Unable to find the platform plugin.
```

and copies nothing. `--verbose 2` shows the cause: it reads every plugin as
`64 bit, MinGW, debug`. The MinGW plugin DLLs ship unstripped, and
`windeployqt`'s debug/release heuristic reads their debug sections as a debug
build. In `--release` mode it then filters out every plugin — including
`platforms\qwindows.dll` — and reports it as missing. `--release --force` fails
the same way.

This installation has **one** set of binaries (there is no `Qt6Cored.dll`), so
`--debug` and `--release` deploy identical files; only the filter differs. The
deployment therefore runs `windeployqt` in the mode this installation accepts
and prunes what that mode adds for developers — chiefly `qmltooling\`, the QML
debugger and profiler plugins. The result is then verified independently by the
import walk, so the choice of mode is not taken on trust.

This is option A of the task's preferred order — the official tool, made to work
— with a repository-owned prune and gate around it, not a reimplementation of
dependency resolution.

## KNOWN LIMITATIONS

- **CLEAN MACHINE: NOT VERIFIED.** Everything above was run on the build
  machine. A bare `PATH` proves the folder does not need Qt; it does not prove
  the folder does not need something else Windows-level that this machine has
  and a fresh Windows installation does not. That requires a second machine and
  is not claimed here.
- **Not code-signed.** `TechAim.exe` and the deployed DLLs carry no
  Authenticode signature, so SmartScreen will warn on first run.
- **Installer not built.** Out of scope for this task; `dist\seta-runtime` is
  its input.
- **`tools/release/build-rc3-seta-package.ps1` still carries its own
  hand-maintained Qt DLL allow-list** as a fallback for the same `windeployqt`
  failure. It is now a second source of truth for the same question and should
  be changed to consume this pipeline's output when the installer work is next
  opened. Not changed here: that is installer work, which this task explicitly
  defers.
- **The smoke test runs as the developer's Windows user**, so the per-user data
  directory (`%LOCALAPPDATA%\TechAim\TechAimSETA`) may already hold sessions and
  a first-run recovery prompt can appear. That is the machine's state, not the
  deployment's: the folder itself is verified to contain no session data, and
  the first run is verified to write nothing into it.
- **Screen-by-screen interaction from the deployment was not driven.** The
  landing page, the event selector, the operating-mode control, the recovery
  dialog and the German catalogue are shown to render from the artefact, and
  the startup log shows `SettingsPage.qml` and `CoachDetailedView.qml` being
  instantiated, so their QML imports resolve. Navigating the DSB screens, the
  Training Lab and a report view by hand from `dist\seta-runtime` remains a
  human step; the automated evidence above does not replace it.
- **The recovery dialog names its discipline in English even in German**
  (`50m Rifle 3 Positions`) because the value is a stored session field. That is
  a translation-coverage matter tracked with the German review, not a
  deployment defect.
- **`opengl32sw.dll` is 19.7 MB of the total** and `multimedia\` (the FFmpeg
  backend, needed for finals audio) another 18.4 MB. Both are kept deliberately:
  the software OpenGL fallback is what keeps the application usable on range
  laptops with no working GPU driver or over remote desktop.
