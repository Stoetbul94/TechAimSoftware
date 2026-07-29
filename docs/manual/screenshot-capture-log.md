# Screenshot Capture Log — Phase J.1

**Outcome: GUI capture capability PROVEN. Screenshot set BLOCKED on two
defects that must be resolved before any capture can be used.**

No screenshot was captured for the manuals. No mock, no AI-generated image and
no fabricated application state was produced.

---

## 1. Capture environment

| Item | Value |
|---|---|
| Executable path | `release/TechAim.exe` |
| Executable embedded SHA | `21b40db` — matches the documented application baseline |
| Repository HEAD at probe time | `5d63b1b` |
| Windows session | interactive, session id 1, `explorer.exe` present |
| Primary display | **1536 × 960** — the register's required capture resolution |
| Application window | `1536 × 912`, visible, maximised on demand |
| app_mode during probe | `Demo` |
| UI language | `en` |
| Data root used | `C:\Users\User\AppData\Local\TechAim\TechAim` — **the real one** (see blocker 2) |
| Synthetic athlete | **not achievable** — see blocker 2 |
| Capture date | 2026-07-28 |
| Automation method | PowerShell + Win32 `EnumWindows` / `ShowWindow` / `SetForegroundWindow` / `PrintWindow`, rendering to `System.Drawing.Bitmap` → PNG |

## 2. Capability: PROVEN

Each step was executed against the real application, not asserted:

| Capability | Result |
|---|---|
| Launch `TechAim.exe` | **YES** |
| Application creates a visible top-level window | **YES** — `1536 × 912` |
| Enumerate and identify the window | **YES** — via `EnumWindows` + PID match |
| Force show / maximise / foreground | **YES** — `ShowWindow(SW_MAXIMIZE)`, `SetForegroundWindow` |
| Capture the window as PNG | **YES** — `PrintWindow` returned true; 159 160-byte PNG produced |
| Visually inspect the capture | **YES** — the Start-session screen was read back and examined |
| Read application state from the capture | **YES** — Demo badge, COM7 Connected, event list, Open Practice, Training Lab all legible |

**Conclusion: this environment CAN drive and photograph the real
application.** Clicking controls and driving full workflows was not attempted,
because the two blockers below make any resulting image unusable.

### First observation timing

On the very first probe the window reported `IsWindowVisible = false` after
8 s. It becomes visible with a longer settle (~20 s). Any capture script must
poll for visibility rather than sleep a fixed interval.

## 3. BLOCKER 1 — legacy SETA identity in the window title

**Actual window title observed:**

```
SETA - Tech Aim Electronic Target Control
```

**Cause — `main.qml:440`, inside `Component.onCompleted`:**

```qml
title = isDefaultIcon ? "TACHUS" : "SETA"
```

This **imperative assignment destroys the declarative binding** established at
`main.qml:13` (`title: PRODUCT.fullProductName`). Qt then appends the
application display name, producing the observed string.

The P0 identity work fixed the declarative binding and did not detect this
override 427 lines later. The window title and the Windows taskbar entry
therefore still present **SETA** as the software product.

**Why this blocks every screenshot:** the screenshot register rejects outright
any capture showing *"Old Seta / Seeds software identity"*. The title bar and
taskbar appear in window-level captures, so the entire set would be rejected.

**Severity: High.** One-line fix (delete the assignment, or route it through
`PRODUCT`), but this phase explicitly forbids silently changing application
code. **Recorded for approval, not fixed.**

**Status: BLOCKED — APPLICATION DEFECT, FIX APPROVAL REQUIRED.**

## 4. BLOCKER 2 — real athlete data cannot be isolated

The captured home screen shows a **real athlete name** in the ATHLETE
selector — not synthetic data.

Source: real `.tch` match records in the `release/` directory
(`Match_21072026-*.tch` and similar), which populate the athlete list.

### Isolation attempts

| Approach | Result |
|---|---|
| Redirect `LOCALAPPDATA` for the child process | **FAILED** — Qt resolves `AppLocalDataLocation` through the Windows shell API, not the environment variable. Startup log confirmed the real root was still used. |
| `StoragePaths::setRootOverrideForTesting()` | **Not reachable** — a C++ test-only API with no command-line or config exposure. |
| Copied Release directory without `.tch` files | **Would fix the athlete list only.** The session store is resolved from the organisation/application name, so captures would still write into the real archive. |

**Consequence:** a clean synthetic capture environment cannot be created
without either a code change (data-root override) or moving/backing up
Arnold's real data. Neither is permitted in this phase.

**Status: BLOCKED — REAL DATA CANNOT BE ISOLATED.**

## 5. Non-contamination confirmed

| Check | Before | After |
|---|---|---|
| Data-root file count | 123 | **123** |
| Data-root byte total | 3 151 485 | **3 151 485** |
| Sessions created by probes | — | **none** |
| Orphan `TechAim.exe` processes | — | **none** |

The application was launched only to the Start-session screen; no session was
started, and nothing was written. **No user data was created, modified or
deleted.**

Pre-existing and untouched: one unfinished session in `Sessions\Current`
(`session_20260727T180053_*.jsonl`) from earlier work. It predates this phase
and was deliberately left alone; it will still be offered for recovery.

## 6. Also observed (recorded, not acted on)

- The EULA overlay is gated on `!isEulaAccepted() || !isValidLicence()`, and
  the startup log shows `isValidLicence ... false`. Any fresh profile would
  therefore land on the **SETA-era EULA screen**, which this phase forbids
  opening or capturing. Reinforces the existing
  `LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA` blocker.
- The Start-session screen shows `COM7 · Connected`, so a target interface is
  present on this machine. Not exercised.

## 7. What is required to unblock

1. **Approve the `main.qml:440` title fix** (remove the legacy override so the
   `PRODUCT.fullProductName` binding survives). One line; needs a rebuild, and
   the resulting executable SHA becomes the new application baseline.
2. **Decide the isolation strategy**, one of:
   - add a supported data-root override (command line or `config.ini`) — a
     code change requiring approval; or
   - authorise a temporary backup-and-restore of the real data root; or
   - accept a copied Release directory *and* explicitly accept that synthetic
     Demo sessions will be written into the real session archive.
3. Once both are settled, the capture automation proven in section 2 can run
   the register end to end.

Until then every register entry stays at its blocked status. No screenshot
has been captured, and none is claimed.
