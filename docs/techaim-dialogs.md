# TechAim dialog framework

One modern dialog system for every message, warning, confirmation and
information popup in the application. **There are no native Windows message
boxes left in reachable code** — no `QMessageBox`, no `QtQuick.Dialogs
MessageDialog`, no `StandardButton`.

## Components

- **`TechAimDialog.qml`** — the visual: dark TechAim card (`#1f2026`, 13 px
  radius, hairline accent, layered soft shadow), accent icon disc with a
  clean inline-SVG glyph, title / message / optional "Show Details"
  expander / right-aligned buttons, ~40 % scrim, fade + scale
  0.95 → 1.0 (200 ms in / 180 ms out). Emits `finished(result)`. Never
  instantiated by pages.
- **`TechAimDialogManager.qml`** — the app-facing API. One instance in
  `main.qml` (`id: dialogManager`, z 4000) resolves everywhere via QML
  ancestor scope, exactly like `windowManager`. Dialogs queue — one visible
  at a time.

## API

```qml
dialogManager.showInformation(title, message [, details])
dialogManager.showWarning(title, message [, details])
dialogManager.showError(title, message [, details])
dialogManager.showSuccess(title, message [, details])
dialogManager.showConfirmation(title, message, function(ok) {...}
                               [, okLabel, cancelLabel])   // OK / Cancel
dialogManager.showQuestion(title, message, function(yes) {...}
                           [, yesLabel, noLabel])          // Yes / No
dialogManager.show({ type, title, message, details,
                     buttons: [{ label, result, accent }],
                     defaultResult, cancelResult,
                     autoDismissMs, onResult: function(result) {...} })
```

Types and accents: `info` blue `#2f6fd0` · `warning` amber `#c77700` ·
`error` red `#d0392b` · `success` green `#1f8a4c` · `confirm` maroon
`#a80038` · `question` blue · `custom`.

Keyboard: **Enter** activates `defaultResult` (the accent button),
**Esc** / scrim click produce `cancelResult` (set it to `""` to force an
explicit button press). Focus lands on the card at open. Icons are inline
SVG data-URIs rendered at 2× sourceSize (high-DPI crisp); the shadow is
plain layered rectangles — no GraphicalEffects, portable to
Windows/Linux/Android.

The manager owns **no application logic**: callbacks belong to the caller
and receive the pressed button's result string.

## C++ → dialog bridges

C++ never opens a dialog itself; it emits a signal and `main.qml` routes it:

- `TachusWidget::uiDialogRequested(type, title, message)` — e.g. the
  connection-error path (`showMessage`).
- `CustomPrint::printingNotice(message, timeoutMs)` — the network
  auto-export notice; rendered as an auto-dismissing info dialog. (The
  legacy `TimedMessageBox` blocked the thread ~5 s before writing the PDF;
  the export now proceeds immediately.)

**Pre-QML exception**: the single-instance warning in `main.cpp` fires
before the QML engine exists, so it uses a small frameless TechAim-styled
*widget* dialog (dark card, rounded, maroon button) — still no native
message box.

## Migrated call sites (behaviour preserved verbatim)

| Legacy | Now |
|---|---|
| LeftPanel "You are in Sighter…" ×3 | Match Summary / Match Report / Coach Report Unavailable warnings |
| CenterPane "Match is completed, restart to stimulate" | Match Complete information |
| ShootingPage/RightPanel finish-match `Dialog` | `shootingPage.confirmMatchFinish()` → Finish Match / Cancel confirm. **Also a bug fix**: the legacy dialog's `StandardButton` enum was never in scope (runtime ReferenceError) — its OK/Cancel never rendered. |
| main.qml `ClosePopupDialog` (exit) | Save Match? — Cancel / Discard / Save (Save = default accent; same quit/save actions) |
| LoginPage user-name / distance / COM / hardware / master errors | warnings/errors with actionable copy; invalid-distance still quits, licence path kept commented as before, never-shown auto-feed branch preserved as a silent no-op |
| `TachusWidget::showMessage` QMessageBox | `uiDialogRequested` → error dialog |
| `CustomPrint` TimedMessageBox | `printingNotice` → auto-dismissing info |
| `main.cpp` single-instance QMessageBox | frameless styled widget dialog |

## Known out-of-scope

- **File/folder pickers** (`QFileDialog`, QML `FolderDialog`/`FileDialog`)
  are system UI, not message boxes — unchanged by design.
- **Vendored QModMaster UI** (`ModReader/src/mainwindow.cpp`,
  `ModReader/forms/settingsmodbustcp.cpp`) still contains `QMessageBox`
  calls but is **unreachable**: `mainWin->show()` has always been commented
  out in `main.cpp`, and those dialogs only open from that window's menus.
  Left untouched per the vendored-code policy.
- `ModConnectorDialog` is a connection *form*, not a message box —
  unchanged (candidate for a later visual pass).

## Verification gate

```
grep -rn "QMessageBox|TimedMessageBox|MessageDialog" --include=*.cpp --include=*.h --include=*.qml .
```
must return only comments and the two vendored-unreachable files above.
