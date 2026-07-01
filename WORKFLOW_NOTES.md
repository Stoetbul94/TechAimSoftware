# TechAim — System Workflow & Feature Notes

> Living document. Updated as features are verified in code and in the running app.
> Intended as a reference for PDF documentation, onboarding, and future development.

---

## 1. Range Management / Network Share System

### What it does
Each lane PC running TechAim writes live score data to a **shared network folder** after
every shot. A separate range-master PC reads those files to aggregate scores across all lanes.
Communication is two-way:

| Direction | Mechanism |
|---|---|
| Lane → Range master | CSV files written to the shared folder after each shot and at session end |
| Range master → Lane | Command files dropped into the shared folder; the app watches for them via a file watcher |

### How to configure it (as of current build)
1. On the **Login / Start session** screen, find the **NETWORK SHARE** card in the left panel.
2. The toggle (right side of card) enables/disables live sharing. Default: **ON**.
3. Click the **left side of the card** (cloud icon / path area) to open a folder picker.
4. Navigate to the shared network folder and click **Select Folder**.
5. The path is now saved to `userDetails_seta.txt` next to the exe and will persist across restarts.

### Files written to the shared folder
- Per-shot CSV: written after every shot received from the target hardware
- Session summary CSV: written at end of session
- PDF report: saved to the same folder when a match report is generated

### What was fixed (session 2026-07-01)
- `saveNameAndPort()` was being called with 2 arguments — the third (network path) was
  silently dropped, meaning the path was cleared to empty every time a session was started.
  Now passes `netowrk_path_text.text` as the third argument.
- The toggle started as `false` in QML but `true` in C++ (`m_isServerNetworkEnabled = true`
  by default). Now starts `true` to match.
- No UI existed to set the path. Added `FolderDialog` — clicking the card's info area opens
  a native Windows folder browser.

### Known limitation
The range-master PC software (which reads the CSVs) is a **separate application** not in
this codebase. The TechAim side is complete; the other side must be verified independently.

---

## 2. Crash Recovery / Session Restore

### What it does
TechAim automatically saves match data to a `.tch` file (XML) next to the exe during a
session. If the app crashes or the computer loses power mid-competition, the data can be
recovered on restart.

### Save triggers — when is data written to disk?
| Trigger | Function called |
|---|---|
| Session start (Start session button clicked) | `saveMatch(true)` — creates a new `.tch` file |
| Every shot received from target hardware | `saveMatch()` — overwrites the file with all shots so far |
| Hardware disconnected mid-session | `saveMatch()` — saves current state immediately |
| App close (Save & Quit dialog) | `saveMatch()` — final save before exit |

So in practice: **the file is updated after every single shot**. If the power cuts
between two shots, at most one shot is lost.

### File naming
| Setting | Filename |
|---|---|
| Overwrite mode (default) | `Match_AthleteName.tch` |
| Timestamped mode | `Match_DDMMYYYY-HHMMSS.tch` |

Files are saved in the **same folder as the exe** (e.g. `seta10\release\`).

### How to restore after a crash
1. Restart the computer and open TechAim.
2. On the **Login / Start session** screen, click **"Load saved session"** (bottom-left button).
3. A file browser opens, **already filtered to `.tch` files only** — you cannot accidentally
   open a wrong file type.
4. Navigate to the folder where the exe lives (e.g. `seta10\release\`) and select the
   `.tch` file for the athlete's session.
5. The app restores: athlete name, game mode, event type, and all shot coordinates with
   timestamps.
6. Continue the competition from where it left off.

### Known limitation / improvement needed
The file browser opens in `C:\Users\<name>\` (Windows home folder) rather than in the
exe's directory where `.tch` files are saved. The user must navigate manually.
**Recommendation:** set `getLoadFileLocation()` to the exe directory on first run, or
default the dialog to `QCoreApplication::applicationDirPath()`.

### What is NOT saved (lost on hard crash)
- The **scoring mode** (Sighter vs Match) is restored via `game_is_sighter_mode`
- The **in-memory shot list** held by `TachusWidget` is rebuilt from the `.tch` file on load
- `autoSaveMatchScore()` exists as a function but its body is **empty** — it was never
  implemented. This has no real impact because `saveMatch()` is already called per-shot.

---

## 3. Discipline Selector (PISTOL / RIFLE)

- Two-button selector at the top of the event list, each showing the discipline name and
  available distances ("10 m · 50 m").
- Selecting PISTOL sets `gameMode = 0`; RIFLE sets `gameMode = 1`.
- The event list, SELECTED PROFILE, SCORING (Integer/Decimal), and DISTANCE tiles all
  update automatically when the discipline changes.
- State is persisted via `APPSETTINGS.setGameMode()` on every change.

---

## 4. Event Selection

- Events are grouped into: Official ISSF Match, Training Sessions, Free Practice.
- Each card shows: shot count badge, event name, ruleset + shot count + time metadata.
- Selected event is highlighted in red; `gameEvent` index is persisted via
  `APPSETTINGS.setGameEvent()`.
- The SELECTED PROFILE and info tiles (SHOT PLAN, SCORING, PREP, MATCH, DISTANCE, RULES)
  update when a different event is selected.

---

## 5. Pending / Not Yet Implemented

| Feature | Status |
|---|---|
| 25 m Pistol disciplines | Not started — needs user input on which events are in scope |
| ISSF Finals format (start-from-zero, countdown, elimination) | Not started — needs user input |
| 50m Rifle `radOf10Ring = 5.2` constant verification | Needs rulebook or calibration test |
| Load session dialog defaulting to exe directory | Usability improvement needed |
| Remaining page rebrands (ShootingPage, SettingsPage, CenterPane, etc.) | In progress — page by page |
