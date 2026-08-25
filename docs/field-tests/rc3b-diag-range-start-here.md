# RC3B-DIAG — start here

**This build is INTERNAL.** It is not the SETA evaluation package and must not
be sent as one. `developer_mode=1` is deliberate: it exists to be diagnosed.

## Before you shoot

1. Unzip the whole folder to the tablet. Do not run it from inside the ZIP.
2. Double-click `TechAim.exe`. It needs no installer, no Qt and nothing on
   PATH.
3. Check the first line of the log or the About screen reads
   **`0.9.0-RC3B-DIAG`**. If it says anything else, you are running the wrong
   build — stop.
4. Plug the target in **before** starting a session, and confirm the connect
   line in the log shows **19200 / Even / 8 / 1**. If it shows 9600 or None,
   stop and report it: that is the defect this build is meant to have fixed.

## What to shoot

`docs/rc3b-diag-physical-qualification-plan.md` — four tests, in order, on
**one tablet first**:

1. **Training, 30–60 shots** — must cross shot 10, 20 and 30.
2. **Reconnect** — unplug the USB mid-session, plug it back in.
3. **50 m 3P Final** — sighters, counted, a position change, an Incident
   Report raised and resolved.
4. **Restart and recovery** — restart the application on a live session.

If you have a tablet free before the range slot, the plan also lists six
**bench scenarios** you can run against `target_emulator.exe` with no target
hardware at all. Crossing a 10-shot boundary for the first time at the range is
avoidable.

## AFTER EVERY TEST — collect the evidence

**Double-click `Collect-Logs.cmd`.** It writes a dated ZIP to the Desktop.

Do this after **every** test, pass or fail. The last field investigation could
not explain part of what happened on one tablet because the logs for the
failure window were never collected — they live in `%TEMP%`, and `%TEMP%` gets
cleaned.

The bundle contains the application logs, the configuration and the build
manifest. It contains **no** passwords, no personal files and no other
athlete's sessions. Session data is opt-in: to include one, run
`powershell -File Make-SupportBundle.ps1 -SessionId <id>` instead.

## "It worked" is not a result

A session that finishes on screen is not a pass. After each test, open the
newest `tachus_log*.log` in the bundle and confirm it contains **none** of:

| Look for | Means |
|---|---|
| `ACQUISITION_FAULT` | acquisition stopped — the shot-10 defect, or a real lost shot |
| `ACQ_COORD_INDEX_INVALID` | a shot was requested that had no measured coordinate |
| `ACQ_COORD_READ_FAILED` | the target was asked for a coordinate and did not give one |
| `ACQ_COORD_REFUSED_BY_UI` | the display refused a shot the backend had accepted |
| `COUNTER JUMPED` | the counter moved by more than one between polls |
| `counter was N, target reports M` where N ≠ M | a reconnect adopted a mismatched counter |
| repeated identical coordinates | the 2026-08-23 corruption returning |
| more than a handful of `auto-connect` lines | the reconnect loop is back |

Report what you find, including "nothing". A clean log is the result.
