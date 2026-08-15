# uirender — offline UI evidence renderer

Loads a QML scene file, renders it offscreen and saves a PNG, so a layout
claim can be **shown** rather than asserted. Used to produce the screenshots
filed against defects in `docs/ui/UI_Defect_Register.md`.

The scenes render **in place**. Nothing has to be copied to the repository
root first — that was an older workaround and is no longer needed.

## Build

Qt 6.5.3 MinGW, in-source (build artifacts are gitignored):

```bash
export PATH="/c/Qt/6.5.3/mingw_64/bin:/c/Qt/Tools/mingw1120_64/bin:$PATH"
cd tools/uirender && qmake uirender.pro && mingw32-make -f Makefile.Release
```

## Run

From the repository root, with the Qt `bin` on `PATH`:

```bash
./tools/uirender/release/uirender.exe tools/uirender/scene_disciplines.qml out.png 620 300
```

No environment variables are required — the tool defaults itself to the
offscreen platform. The working directory does not matter; scene paths are
resolved relative to the scene file, not to the shell.

The bundled scenes and the sizes they were designed at:

| Scene                  | Size       | What it evidences                                     |
| ---------------------- | ---------- | ----------------------------------------------------- |
| `scene_disciplines.qml`| `620 300`  | The four discipline plates as the left pane draws them |
| `scene_leftpanel.qml`  | `460 760`  | UI-TRAIN-001 — the badge reads the active programme    |
| `scene_topbar.qml`     | `1536 42`  | The honesty line sits clear of the connection panel    |

Success is the `saved <path> (WxH)` line **and** exit status 0. A `WARN` line
means the PNG was written but is not the evidence that was asked for.

## Writing a new scene

Scenes live in this directory and reach the application's root-level
components — `DisciplineArt`, `VIcon`, `LeftPanel`, `TrainingTopBar` — with a
relative directory import:

```qml
import QtQuick 2.15
import "../.."                  // repository root
```

Without that import the type is not in scope and the tool stops with
`QML ERROR ... <Type> is not a type`. The import is relative to the scene
file, so a scene moved out of this directory must have its import adjusted.

`main.cpp` injects minimal stand-ins for the ambient context properties the
real application provides (`APPSETTINGS`, `MODREADER`, `loginPage`, `theme`).
A scene that reaches for a property no stub provides will render, but that
binding will be undefined — extend the stubs rather than working around it in
the scene.

## Gotchas, and what the tool now does about them

**Headless crash.** `QQuickView::show()` + `grabWindow()` crashes with no
platform plugin, and `QT_QUICK_BACKEND=software` applies the desktop's DPI
scaling so the PNG is not the requested size. The tool defaults
`QT_QPA_PLATFORM=offscreen` and `QT_ENABLE_HIGHDPI_SCALING=0` when they are
unset; setting either explicitly still wins. If the saved image does not match
the requested size the tool says so and exits non-zero.

**Empty boxes instead of text.** The offscreen platform plugin carries no font
database — Qt no longer ships fonts, so `QFontDatabase::families()` comes back
empty and every glyph renders as tofu. The tool registers Segoe UI (falling
back to Arial, then DejaVu Sans) from the system font directory at startup and
draws with whichever family resolved. If none is found it prints
`WARN no usable font found` rather than silently producing unreadable
evidence.

## Evidence status

Renderer output is **automated evidence**. Per `CLAUDE.md`, a PNG from this
tool supports a defect status of
`RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED`; it is not a
screenshot of the running application and does not by itself close a defect.
