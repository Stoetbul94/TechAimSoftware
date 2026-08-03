# Tech Aim — diagnostics appendix

Technical detail deliberately kept out of the
[Operator guide](Operator-Guide.md). For engineers and for anyone reading a
support bundle.

---

## 1. Target detection — how selection actually works

Enumeration is `QSerialPortInfo::availablePorts()`. Each port yields port name,
system location, manufacturer, description, serial number, and vendor/product
identifiers **where the driver supplies them**.

Qt 6 removed `QSerialPortInfo::isBusy()`, and on Windows it was never reliable
(several drivers always reported false). Busy state is therefore recorded as
**unknown** rather than guessed, and never disqualifies a port.

**Rejection** happens on metadata alone — no port is opened during discovery,
which is why nothing can block on a Bluetooth device:

- Bluetooth: `bluetooth`, `rfcomm`, `bthenum` in description **and** manufacturer
- virtual ports and modems

**Scoring** among survivors:

| Signal | Score |
|---|---|
| Matches the remembered adapter fingerprint | +1000 |
| Known USB-serial bridge (CH340/CH341, CP210x, FT232/FTDI, PL2303) | +100 |
| Vendor + product identifier present | +50 |
| Serial number present | +10 |
| Reported busy | −25 |

**Outcomes:** `AutoConnect` (one confident candidate), `NeedsUserChoice`
(several), `NoCandidates` (none). Only `AutoConnect` yields a port name; the
other two return an **empty** string by contract.

### The startup order — SERIAL-AUTO-001

The RC2 field failure was not in ranking or rejection. The selector was wired
*after* a speculative connection with an empty port name, and `ModbusCommSettings`
substituted the stored port, so the connection "succeeded" and the selector
never ran.

RC2a inverts it: **decide, then connect.** An empty port name means run the
selector; an empty answer returns failure rather than falling through. The
stored port is a *candidate*, used only if it still appears in the current
enumeration, so a stale port from another machine is discarded.

Regression tests 35–43 in `tests/reliability/tst_target_hardware.cpp` pin the
**order**, which is the gap the original tests left — they all called the
selector directly and none asserted when the application calls it.

### Fingerprint storage

`HKCU\Software\TechAim\TechAim`, group `TargetDevice`. Written **only** after
communication is confirmed, and only if it still matches the selected device.
Per-user and per-machine — never packaged, never copied between machines.

## 2. Serial link

Modbus RTU, 9600 baud, 8 data bits, no parity, 1 stop bit, RTS disabled. The
paper-feed motor is register **8196**, value **32768** — the same command the
manual feed button has always used. No new motor protocol was invented and no
arbitrary command is ever sent to a device that has not been identified.

## 3. Paper feed

`PaperFeedCoordinator` is the single authority. The hook sits at the hardware
acceptance site — after protocol validation, the duplicate guard and coordinate
storage — so demo and UI shots (which go through `uxShoot()`) never reach it.

Checks, in order: **replay first**, then live mode, target connected, duplicate
identity (512-entry recent set), duration. Duration is sanitised: NaN → 0,
negative → 0, > 30 s → clamped to 30, 0 = disabled. A queue plus a busy flag
prevents re-entrancy.

Per-command duration is passed with each request, so a sighter feeds for the
sighter duration. RC1 stored a sighter value but passed only the match value.

`isAppDemoMode` is **inverted-named** — it means *is live*. Reading it the other
way would drive the physical motor from demo clicks.

## 4. Shot-pipeline stamps (RC2a diagnostic)

Emitted only when `developer_mode=1`. All share one session tag and one
monotonic clock, so differences are directly comparable.

| Stamp | Side | Boundary |
|---|---|---|
| `decoded` | C++ | frame validated, coordinates stored |
| `emit-shootCountChanged` | C++ | notification leaving the backend |
| `qml-notified` | QML | interface received it |
| `qml-scored` | QML | ISSF ring score computed |
| `qml-marker-added` | QML | impact added to the series |
| `zoom-requested` | QML | auto-zoom entry reached |
| `zoom-started` | QML | zoom animation began |
| `emit-returned` | C++ | control back in the backend |
| `feed-hook-enter` / `feed-hook-exit` | C++ | paper-feed decision window |

Reading them: `emit-shootCountChanged` → `qml-notified` is signal delivery;
`qml-notified` → `qml-marker-added` is scoring plus draw. `zoom-requested`
without `zoom-started` means the zoom was **suppressed by a gate**, not that it
failed.

Extract them with:

```
powershell -File Make-SupportBundle.ps1 -Diagnostic
```

## 5. Auto-zoom — why it is still UNVERIFIED

`CenterPane.qml`: `autoZoomOn: true`, `autoZoomTarget: 2.4`, `autoZoomHold: 4`
(≈4 × 450 ms). `triggerAutoZoom(px, py)` is called from the shot path, gated by:

```qml
trainingHidesImpact = shootingPage.isTrainingMatch && !TRAINING.showImpacts
```

The RC2 physical test was a Training programme. If impacts were hidden, the
zoom was **suppressed by design** — the athlete is meant not to see the shot —
and there is no defect.

It was therefore not "fixed": the intended behaviour is clear, but the code
path is **not** demonstrably failing, and loosening that gate would risk
revealing impacts a Training programme is meant to withhold. Test D of the
physical retest settles it — `zoom-requested` without `zoom-started` in a mode
where impacts are *visible* would be a real defect.

## 6. Storage layout

Root: `QStandardPaths::AppLocalDataLocation` =
`%LOCALAPPDATA%\TechAim\TechAim` (organisation and application are both
`TechAim`). `StoragePaths` is the sole owner; never the executable directory,
never the process working directory.

```
Sessions\Current     live session journals (append-only JSONL, hash-chained)
Sessions\Archive     completed sessions, month-partitioned
Sessions\Corrupt     journals that failed validation, preserved not deleted
Backups  Reports  Exports  Logs  Settings  SupportBundles  DerivedIndexes
```

Session files are `session_<utcCompact>_<uuid8>.jsonl`. The finals journal keeps
its historic name `finals_session.jsonl` inside `Sessions\Current`.

## 7. Logging — LOG-001

Two locations, only one of them used:

- `…\TechAim\TechAim\Logs` — created by the storage layer, **empty**.
- `%TEMP%\tachus_log<ddMMyyyy-hhmmss>.log` — the actual operational log, one
  file **per launch**.

This is a known runtime defect (LOG-001 in
[Deployment-Audit.md](Deployment-Audit.md)). It is not fixed in the
deployment-preparation branch, because that would change the binary Arnold is
about to test. The support bundle collects **both** locations and reports which
had content.

`%TEMP%` is cleared by Disk Cleanup and Storage Sense — collect bundles promptly.

## 8. What is not instrumented

- **No crash dumps.** No `MiniDumpWriteDump`, no unhandled-exception filter, no
  Breakpad/Crashpad. A crash leaves only the Windows event log and whatever the
  `%TEMP%` log captured first.
- **No telemetry.** Nothing is transmitted anywhere. Every diagnostic is a local
  file an operator chooses to send.
- **No code signing.** Binaries and packages are unsigned; SmartScreen will warn
  on a downloaded package.

## 9. Configuration resolution — CFG-001

`main.cpp:321` constructs `AppSettings("config.ini")` — a **relative** path,
resolved against the process working directory. Launching from a different
working directory silently loads defaults instead of the shipped configuration.

Any shortcut or installer **must** set the working directory to the install
folder. Recorded as `requiresWorkingDirectory` in the install manifest.
