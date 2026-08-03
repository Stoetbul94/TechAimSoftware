# Tech Aim 0.9.0-RC2a — physical qualification checklist

**Total shots: 3.** One sighter, two counted.

This is the test that decides whether SERIAL-AUTO-001 can be closed. Everything
else in the deployment-preparation phase is waiting on it.

---

## Before you start

| | |
|---|---|
| Package | `TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip` |
| SHA-256 | `215C8F0DC89E2E1D5F19CAD6D2B468DA6CED9ADA0735D210AB0D54EC602B165D` |
| Extract to | `C:\TechAim\RC2a\` |
| Do **not** overwrite | `C:\TechAim\RC1\`, `C:\TechAim\RC2\` |

**Verify the download first:**

```bash
Get-FileHash "C:\TechAim\TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip" -Algorithm SHA256
```

It must match exactly. If it does not, stop.

**Set diagnostic mode.** Open `C:\TechAim\RC2a\config.ini` and set:

```ini
app_mode=Live
developer_mode=1
```

`developer_mode=1` is what emits the correlated timestamps. `app_mode` stays
`Live`. This is the **only** sanctioned use of `developer_mode=1`.

**Leave Bluetooth ON.** That is the condition RC2 failed under. Turning it off
would test the wrong thing.

Record before starting:

```
Date / time     : ______________________
Windows edition : ______________________
Bluetooth       : ON  (required)
Target model    : ______________________
Discipline      : ______________________
```

---

## Test sequence

### A. Target connected before startup

1. Connect the target's USB cable. Wait for Windows to finish recognising it.
2. Start `TechAim.exe`.

| Check | Expected | Result |
|---|---|---|
| Target state shown | `SCANNING` → `TARGET DETECTED` → `TARGET CONNECTED` | ☐ |
| Port used | The target's real port (record it) | ☐ |

Port: ______________

### B. Automatic connection

| Check | Expected | Result |
|---|---|---|
| A Bluetooth port was **not** chosen | No Bluetooth port opened or claimed | ☐ |
| The connection came up without help | Connected within a few seconds | ☐ |

### C. No manual selection

| Check | Expected | Result |
|---|---|---|
| Did you have to pick a port? | **No** | ☐ |

If you had to select the port manually, **A–C have FAILED**. Record it and
continue to D — the remaining tests are still informative.

### D. No paper movement during startup

| Check | Expected | Result |
|---|---|---|
| Paper during scan and connect | **Does not move** | ☐ |
| A shot appeared without one being fired | **No** | ☐ |

### E. One sighter

Fire **one sighter shot**.

| Check | Expected | Result |
|---|---|---|
| Shot registered | Yes | ☐ |
| Impact drawn on screen | Yes | ☐ |
| Delay before it appeared | Record seconds | ______ s |
| View zoomed to the shot | Record Yes / No | ☐ |
| Paper fed | **Exactly once** | ☐ |
| Feed duration looked like the **sighter** setting | Yes / No | ☐ |

### F. One counted shot

Fire **one counted shot**.

| Check | Expected | Result |
|---|---|---|
| Shot registered and scored | Yes | ☐ |
| Delay before it appeared | Record seconds | ______ s |
| View zoomed to the shot | Record Yes / No | ☐ |
| Paper fed | **Exactly once** | ☐ |

**Important for the zoom check:** if the discipline is a **Training** programme
with impacts hidden, the zoom is suppressed **on purpose** and that is not a
defect. If you can, run E and F in a mode where impacts are **visible** — that
is the only setting in which a missing zoom means something.

Visibility mode used: ______________________

### G. Disconnect and reconnect

1. Unplug the target's USB cable.
2. Wait about 10 seconds.
3. Plug it back in.

| Check | Expected | Result |
|---|---|---|
| Disconnection noticed | Target state changes | ☐ |
| Reconnection | Reconnects, or reconnects after **Rescan** | ☐ |
| Manual selection needed? | Record Yes / No | ☐ |
| Paper moved during reconnection | **No** | ☐ |
| A false shot appeared | **No** | ☐ |

### H. One counted shot after reconnect

Fire **one counted shot**.

| Check | Expected | Result |
|---|---|---|
| Shot registered and scored | Yes | ☐ |
| Paper fed | **Exactly once** | ☐ |
| Shot count is correct overall | 2 counted + 1 sighter | ☐ |

### I. Restart and remembered connection

1. Close the application.
2. Start it again, target still connected.

| Check | Expected | Result |
|---|---|---|
| Reconnected automatically | Yes | ☐ |
| Same port as test A | Yes / No | ☐ |
| Faster than the first start | Record impression | ☐ |
| Manual selection needed? | **No** | ☐ |

### J. Support bundle

```bash
powershell -File Make-SupportBundle.ps1 -Diagnostic
```

| Check | Expected | Result |
|---|---|---|
| Bundle created on the Desktop | Yes | ☐ |
| `release-identity.txt` shows `0.9.0-RC2a` | Yes | ☐ |
| It shows `Developer mode : 1` | Yes | ☐ |
| `shot-pipeline-stamps.txt` present and **not empty** | Yes | ☐ |
| `log-collection.txt` reports collected logs | Yes | ☐ |

Send the bundle. **Do not delete `C:\TechAim\RC2a\` afterwards** — if a question
comes up, the logs are still there.

---

## Latency and zoom observation sheet

| Shot | Type | Delay seen (s) | Zoomed? | Paper fed once? | Notes |
|---|---|---|---|---|---|
| 1 | Sighter | | | | |
| 2 | Counted | | | | |
| 3 | Counted (after reconnect) | | | | |

Anything else you noticed: _______________________________________________

---

## Stop-test criteria

Stop immediately, record what happened, and collect a support bundle if:

- **the paper moves when no shot was fired** — motor safety;
- **a shot appears that nobody fired** — scoring integrity;
- the application connects to a **Bluetooth** port;
- the interface freezes for more than about 30 seconds;
- the application crashes or closes on its own;
- **the shot count is wrong** at any point.

Do not continue firing to "see if it settles". Three shots is the whole test.

---

## Afterwards

1. Set `developer_mode=0` in `C:\TechAim\RC2a\config.ini`.
2. Send the support bundle and this completed sheet.

## Rollback

If RC2a is worse than RC2 for any reason, stop using it and run
`C:\TechAim\RC2\TechAim.exe` instead. Nothing needs uninstalling and **no
session data is lost** — your data lives in
`%LOCALAPPDATA%\TechAim\TechAim` and is not touched by switching folders.

Older still: `TechAim-Rollback-747b9a7-Windows-x64.zip`, SHA-256
`3051552E78E5868271E1D8CD6DC1430193577FDF6AE4A36595FE3CCB6033C3CB`.

---

## What a pass means

A pass closes **SERIAL-AUTO-001** and confirms paper feed and reconnect for
**one discipline on one machine**.

It is **not** approval for final deployment, and it does **not** qualify any
other discipline. The next step is RC3, rebuilt from the approved commit with
`developer_mode=0` — see
[0.9.0-promotion-plan.md](../release/0.9.0-promotion-plan.md).
