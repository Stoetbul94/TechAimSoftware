# Tech Aim — clean-machine test

## Status: **BLOCKED — PHYSICAL ENVIRONMENT REQUIRED**

This test was **not performed**. It has not passed and it has not failed.

---

## Why it is blocked

All three preferred options were checked on the development machine:

| Option | Available | Evidence |
|---|---|---|
| A second Windows computer | **No** | Only one machine is available to this session |
| An already-enabled Windows Sandbox | **No** | `C:\Windows\System32\WindowsSandbox.exe` **absent** |
| A clean Windows virtual machine | **No** | `vmconnect.exe` absent; no hypervisor present |

The decisive fact: the development machine runs **Windows 11 Home Single
Language** (build 10.0.26200, x64). **Windows Sandbox and Hyper-V are not
available on Home editions at all.** Enabling them is not a configuration
change — it requires a Windows edition upgrade to Pro. That is a host change,
which this phase is not permitted to make, and no amount of scripting works
around it.

Third-party hypervisors were not installed either, and installing one is also a
host change.

**Antivirus was not weakened, disabled or excluded at any point.**

---

## What *was* done instead, and what it is worth

An **isolated-launch test**, which is genuinely useful but is *not* a
clean-machine test and must never be reported as one.

Both the RC2 and RC2a portable folders were launched five times between them
with `PATH` reduced to `%WINDIR%\System32;%WINDIR%` — no Qt, no MinGW, no
repository, no developer tools — from their own working directories. Every
launch started and stayed running, and closed. See
[AppData-and-Upgrade-Safety.md](AppData-and-Upgrade-Safety.md), 35 checks,
0 failures.

**Why that is not sufficient.** A scrubbed `PATH` does not remove registered
DLLs, installed runtimes, registry state or side-by-side assemblies that a Qt
installation may have left on this machine. Only a machine that never had Qt or
a compiler can prove those are not being relied on.

---

## The checklist — for Arnold, on a clean machine

Run on a Windows machine that has **never** had Qt, MinGW, Visual Studio or this
repository installed. **No target hardware connected.** Do not weaken antivirus.

Record Windows edition, version, build and architecture before starting.

| # | Step | Expected | Result |
|---|---|---|---|
| 1 | Copy the ZIP to the clean machine | Transfers | ☐ |
| 2 | `Get-FileHash <zip> -Algorithm SHA256` | `215C8F0DC89E2E1D5F19CAD6D2B468DA6CED9ADA0735D210AB0D54EC602B165D` | ☐ |
| 3 | Extract to `C:\TechAim\RC2a` | Extracts, no error | ☐ |
| 4 | Note any SmartScreen warning | Expected — the package is **unsigned** | ☐ |
| 5 | Run `TechAim.exe` | Starts. **No missing-DLL dialog.** | ☐ |
| 6 | Read the version on screen | `0.9.0-RC2a`, channel *Internal Field Test — Diagnostic* | ☐ |
| 7 | Watch the target state | **TARGET NOT DETECTED** (no hardware connected) | ☐ |
| 8 | Time the scan | Interface stays responsive; **no freeze** | ☐ |
| 9 | Check no Bluetooth port was chosen | No connection claimed on a Bluetooth port | ☐ |
| 10 | Check manual COM controls | Still available and usable | ☐ |
| 11 | Watch the target | **No paper movement** at any point during startup | ☐ |
| 12 | Watch the shot display | **No false shot** appears | ☐ |
| 13 | Check `%LOCALAPPDATA%\TechAim\TechAim` | Created, with `Sessions`, `Logs`, `Reports`, `Settings`, `Exports`, `Backups` | ☐ |
| 14 | Check `%TEMP%` for `tachus_log*.log` | One log created for this launch | ☐ |
| 15 | Close the application | Closes cleanly, no hang, no error dialog | ☐ |
| 16 | Start it again | Starts cleanly; data folder intact | ☐ |
| 17 | Compare the session count with step 13 | Unchanged — **existing sessions not corrupted** | ☐ |
| 18 | Note any UAC prompt | **None expected.** If Windows asks for administrator access, record it — that is a finding. | ☐ |
| 19 | `powershell -File Make-SupportBundle.ps1` | Bundle written to Desktop | ☐ |
| 20 | Open the bundle | Contains `release-identity.txt`, `log-collection.txt`, collected logs, `storage-inventory.txt` | ☐ |

### Record

```
Windows edition   : ____________________  (e.g. Windows 11 Pro 24H2)
Windows version   : ____________________
Windows build     : ____________________
Architecture      : ____________________
Machine had Qt?   : ____________________  (must be NO)
Antivirus         : ____________________  (must NOT be disabled)
Date / tester     : ____________________
Overall result    : PASS / FAIL
```

### Stop criteria

Stop and report immediately if any of these occur:

- a missing-DLL dialog on startup;
- the application requests administrator access;
- the interface freezes while scanning ports;
- **the paper moves at any point during startup**;
- a shot appears without one being fired;
- an existing session is altered or lost.

---

## What a pass here would and would not mean

A pass would confirm the **package is complete and self-contained**: nothing is
silently borrowed from a developer machine.

It would **not** be a hardware qualification, would **not** approve automatic
COM detection (no target is connected), and would **not** be deployment
approval. Those need the physical retest —
[Physical-Qualification-Checklist.md](Physical-Qualification-Checklist.md).
