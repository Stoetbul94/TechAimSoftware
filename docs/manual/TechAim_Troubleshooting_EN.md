# Tech Aim Electronic Target Control — Troubleshooting Guide

Product version 0.9.0 · Release channel: Pre-Beta Validation
Document version 1.1 ({{DOCUMENT_VERSION}}) · Language: English (controlled master edition)
Built {{DOCUMENT_BUILD_TIMESTAMP}} · Application baseline commit `{{APPLICATION_BASELINE_COMMIT}}` · Documentation source commit `{{DOCUMENTATION_SOURCE_COMMIT}}`
Publisher: JAC SHOOTING SOLUTIONS (PTY) LTD

**Status: Pre-Beta Documentation. Internal evaluation — not for public
distribution.**

> **Never disable Microsoft Defender, SmartScreen or Windows Firewall** to
> work around a problem. No procedure in this guide asks you to. Any temporary
> diagnostic bypass requires explicit engineering approval and must never
> appear in normal operator instructions.

**Information to send with any report:** version + **Commit:** + **Built:**
(Settings ▸ ABOUT / BUILD), operating mode, discipline and programme, what you
did / expected / observed, and the exported report if relevant. Do not send
athlete personal data that is not needed.

---

## Contents

- [1. Application](#1-application)
- [2. Target connection](#2-target-connection)
- [3. Scoring and coordinates](#3-scoring-and-coordinates)
- [4. Training Lab](#4-training-lab)
- [5. Reports](#5-reports)
- [6. Recovery](#6-recovery)
- [7. Windows and security](#7-windows-and-security)
- [8. Decision trees](#8-decision-trees)

---

## 1. Application

### 1.1 `TechAim.exe` does not open

**WHAT IT MEANS** — the process failed to start or exited before showing a
window.

**LIKELY CAUSES** — another instance already running; missing Qt runtime or
platform plugin; incomplete copy.

**CHECK FIRST** — is Tech Aim already running (taskbar / Task Manager)?

**CORRECTIVE STEPS**
1. Close any running instance and retry.
2. Confirm the Qt runtime DLLs and the `platforms` plugin folder sit beside
   the executable.
3. Confirm `Qt6Multimedia.dll` and the `multimedia` plugin folder are present
   — finals audio needs them.

**DO NOT** — copy the `.exe` alone to another machine; disable security
software.

**ESCALATE WHEN** — it fails on a correctly deployed folder.

**[WINDOWS RC1 DEPENDENT]** — installer-related causes.

### 1.2 Application opens and immediately closes

**LIKELY CAUSES** — storage location not writable; single-instance lock;
missing plugin.

**CHECK FIRST** — can the current Windows user write to the local application
data folder?

**CORRECTIVE STEPS** — Tech Aim reports a storage failure with **Retry** or
**Exit** rather than closing silently; if it closes with no message, capture
the diagnostic log from the Windows temporary folder and escalate.

### 1.3 "Tech Aim is already running"

**WHAT IT MEANS** — single-instance protection. Only one instance may use the
session data at a time.

**CORRECTIVE STEPS**
1. Switch to the running instance.
2. If none is visible, end any leftover process, then relaunch.

**NOTE** — this build also blocks against a **legacy** `Seta.exe` instance, on
purpose: two builds writing the same session store would corrupt it. If the
message appears with no Tech Aim window, check for an old `Seta.exe` running.

**DO NOT** — run two copies against the same data to "get around" it.

### 1.4 Restart Now does not reopen the application

**LIKELY CAUSES** — the relaunch was blocked, or the first instance had not
released its lock.

**CORRECTIVE STEPS** — wait a few seconds and start `TechAim.exe` manually;
confirm the mode changed as intended.

**NOTE** — restart relaunches **the same executable that is running**, so a
renamed executable cannot break it.

### 1.5 Wrong language / missing or English text in German

**WHAT IT MEANS** — usually **not** a fault. German is a partial beta
translation; untranslated text intentionally falls back to English.

**CHECK FIRST** — Settings ▸ **LANGUAGE**.

**CORRECTIVE STEPS**
1. Reselect the language; restart if a **Restart required** note appears.
2. If the whole interface is English after choosing Deutsch, the catalogue
   failed to load — escalate with the startup log.

**EXPECT** — mixed-language screens in German. See
`TechAim_German_Translation_Status.md`.

**DO NOT** — hand-edit `config.ini` to a language code that is not offered;
unknown codes fall back to English.

### 1.6 Clipped or overlapping text

**LIKELY CAUSES** — long German compounds; small window.

**CORRECTIVE STEPS** — enlarge or maximise the window; note the exact screen,
language and window size and report it.

**Known risk:** German layout has **not** been visually verified.
**[GERMAN REVIEW REQUIRED]**

### 1.7 Blank screen / appears frozen / crashed

**CORRECTIVE STEPS**
1. Wait — PDF export and report rendering can take a moment.
2. If unresponsive, end the process, restart, and expect the **recovery**
   prompt (see [section 6](#6-recovery)).
3. Preserve the diagnostic log before shooting again.

**DO NOT** — delete session data to clear a crash; it is what recovery needs.

### 1.8 Old session information remains after Home

**WHAT IT MEANS** — a defect. Clean Home must reset counters and session
state.

**CORRECTIVE STEPS** — restart the application, confirm the previous session
is closed and not offered for recovery, and report it with the programme name
and steps.

**Note:** a stale-counter defect of exactly this kind was found and fixed in
Position Transition. If it reappears anywhere, report it — do not work around
it.

---

## 2. Target connection

**[PHYSICAL TARGET DEPENDENT]** — this whole section needs confirmation
against a real target.

### 2.0 The Target Connection panel — what each state means

Tech Aim shows one **Target Connection** panel, on the Start session screen and
again on the shooting screen. It is the authoritative statement of the target
link. It reads the live connection — it never shows a remembered value.

Below the state it shows the **device description reported by Windows and the
port actually in use**, for example `USB Serial Port · COM7`. The description
is whatever Windows calls your adapter; different adapters report different
names, and the port number can change (see 2.1.1).

| State | Meaning | Can you shoot? |
|---|---|---|
| `SEARCHING FOR TARGET…` | Scanning the serial ports. | No |
| `TARGET FOUND` | A target was identified; connecting. | No |
| `SELECT TARGET PORT` | More than one device could be the target. Choose the port. | No |
| `SYNCHRONIZING…` | Connected; reading the target's shot counter before trusting it. | Not yet |
| `READY` | Identified, connected, and synchronized. | **Yes** |
| `RECONNECTING…` | The link dropped. Rediscovery is running automatically. | **No** |
| `TARGET DISCONNECTED` | The link is gone. Reconnect the USB cable. | **No** |
| `TARGET ACQUISITION ERROR` | The target and the software disagree about the shot count. | **NO — STOP SHOOTING** |

**`SYNCHRONIZING` is normal.** It appears at every connection and after each
reconnection, and it is deliberate: the software reads the target's own counter
rather than assuming it. On the Start session screen it is also the expected
resting state — the shot counter is only read once a session is running.

**Bluetooth ports are never chosen automatically.** Windows lists
`Standard Serial over Bluetooth link (COMx)` ports on most laptops; Tech Aim
rejects them before opening them, so they cannot be mistaken for a target.

### 2.1 `TARGET DISCONNECTED` or `RECONNECTING…`

**LIKELY CAUSES** — cable unseated, target powered down, converter fault, port
taken by another program.

**CHECK FIRST** — is the target powered, and is the cable seated?

**CORRECTIVE STEPS**
1. Reconnect the USB cable. Rediscovery runs by itself — you do **not** need to
   select a port by hand, and you do not need to restart the application.
2. Watch the panel: `RECONNECTING…` → `SYNCHRONIZING…` → `READY`.
3. Close any other program holding the port.
4. If it does not recover, power-cycle the target, then restart Tech Aim.

**WHILE THE LINK IS DOWN** no shot is accepted and no paper feed is issued.
A shot fired during an outage is not recorded — wait for `READY`.

**ESCALATE WHEN** — the adapter is present in Windows Device Manager and free,
but the panel never reaches `READY`.

#### 2.1.1 The COM port number changed after reconnecting

**WHAT IT MEANS** — normal Windows behaviour. Unplugging and replugging a USB
adapter can make Windows assign it a different port number (for example COM7
becomes COM8). Tech Aim rediscovers the target by its device identity, so it
follows the change automatically and displays the new port.

**WHAT TO DO** — nothing. Confirm the panel shows the new port and `READY`.

**DO NOT** type the old port into the manual fallback field to "correct" it.

### 2.1.2 `TARGET ACQUISITION ERROR`

**STOP SHOOTING.**

**WHAT IT MEANS** — the target's shot counter moved by an amount the software
cannot account for (it jumped, or it went backwards). Rather than guess, Tech
Aim stops accepting shots and tells you. Shots may have been missed.

**WHY IT DOES NOT CLEAR ITSELF** — deliberately. A later normal-looking reading
does not prove the missing shots were recorded, so the warning stays until the
session is dealt with.

**CORRECTIVE STEPS**
1. Stop shooting and tell the range officer.
2. Record the time and the last shot you are certain was recorded.
3. Check the target connection, then restart the session.
4. Keep the diagnostic log — it identifies the exact shot where the counts
   diverged.

### 2.1.3 The manual port fallback field

The field labelled **TARGET CONNECTION — MANUAL PORT FALLBACK** on the Start
session screen is **not** the target status. It is a manual override for the
rare case where automatic discovery cannot decide.

It may still show a previously used port. That is not a fault and does not mean
the target is connected on that port. **The Target Connection panel is the
authority** — always read the state and port from the panel.

### 2.1.4 "Target Not Ready" when pressing Start Practice

**WHAT IT MEANS** — a Live session needs a working target, and there is not one
yet. The message includes the current target state and port, which is the same
state shown in the Target Connection panel.

**CORRECTIVE STEPS** — resolve the state named in the message using the table
in 2.0, wait for `READY`, then press Start Practice again.

**In Demo / Simulation mode** no target is required and this message does not
appear.

### 2.2 No shot received

Use [Decision tree A](#a--no-shot-received).

**Most common non-faults**
- **Demo mode selected** — physical shots are rejected by design.
- **Wrong phase** — shots during **POSITION SETUP** are deliberately ignored.
- **Session already complete** — no further shots are accepted.

### 2.3 Shots arrive late, duplicated, or unexpectedly

**LIKELY CAUSES** — communication retries, electrical noise, a target
registering a non-shot event.

**CORRECTIVE STEPS** — note the exact time and sequence; check cabling and
earthing; capture the log; do not attempt to delete a shot from the record.

**DO NOT** — edit the session record. It is append-only on purpose.

### 2.4 Mode rejects the input (Demo shot in Live, or physical shot in Demo)

**WHAT IT MEANS** — working as designed. Tech Aim refuses shots whose source
does not match the operating mode, so a demonstration cannot be mistaken for a
real result.

**CORRECTIVE STEPS** — set the correct mode in Settings and restart.

---

## 3. Scoring and coordinates

**[PHYSICAL TARGET DEPENDENT]**

### 3.1 Shot appears on the wrong side / axis reversed

**LIKELY CAUSES** — target coordinate orientation or units differ from what is
configured.

**CHECK FIRST** — fire a deliberate, clearly off-centre shot in a known
direction and compare.

**CORRECTIVE STEPS** — record the physical impact position and the displayed
position and escalate. **Do not** compensate by aiming off.

**ESCALATE WHEN** — the mapping is consistently mirrored or rotated.

### 3.2 Score differs from expected / centre shot is not centred

**CHECK FIRST** — the discipline, and whether decimal scoring is expected.

**KNOWN LIMITATION** — the 50 m Rifle 10-ring radius awaits official
confirmation or physical calibration. Treat 50 m absolute values as
provisional and report measured discrepancies with physical evidence.

### 3.3 Sighter counted as a match shot, or a counted shot shown as a sighter

**WHAT IT MEANS** — a defect if genuinely mis-classified. Sighters are
excluded from counted results by design.

**CORRECTIVE STEPS** — record the exact sequence and phase and escalate;
export the report as evidence before starting another session.

### 3.4 Unexpected zero or missing shot number

**CHECK FIRST** — whether the red `000` counter is showing during a **Training
Lab** session — see [4.2](#42-red-000-counter-appears-during-training).

---

## 4. Training Lab

### 4.1 A match timer appears during Training

**WHAT IT MEANS** — a defect. Competition countdown overlays are suppressed in
all Training Lab programmes.

**CORRECTIVE STEPS** — note the programme and screen and report it. Training
timing (setup time, ready-to-first-shot, cadence) is shown in the programme's
own panels, not as a match clock.

### 4.2 Red `000` counter appears during Training

**WHAT IT MEANS** — a defect, and on a fresh screen it looks like a phantom
"zero" shot. Training programmes count their own work in the right panel.

**CORRECTIVE STEPS** — report with the programme and screen.

### 4.3 Training programme not available

**LIKELY CAUSE** — discipline gating. **Position Transition is offered only
for 50 m Rifle Three Positions.**

**CORRECTIVE STEPS** — return Home, select the correct discipline, reopen the
Training Lab.

### 4.4 START BLOCK / POSITION READY / START VERIFICATION unavailable

**WHAT IT MEANS** — usually the workflow is not in the phase that offers that
action.

**CHECK FIRST** — the right panel's current phase.

**CORRECTIVE STEPS** — complete the current phase first. In Position
Transition the order is **POSITION SETUP → POSITION READY → (sighters) →
START VERIFICATION**.

### 4.5 Call cannot be confirmed / actual impact reveals too early

**BY DESIGN** — only one shot may be unresolved at a time; the next shot is
refused until the current call is confirmed. The actual impact is stored
hidden and revealed only after **CONFIRM CALL**.

**IF THE ACTUAL REVEALS BEFORE THE CALL IS CONFIRMED** — that is a defect that
invalidates the exercise. Report it immediately with the shot number.

### 4.6 Sighters appear to be included in metrics

**WHAT IT MEANS** — a defect. Sighters are excluded everywhere.

**CORRECTIVE STEPS** — export the report as evidence, note sighter and counted
shot counts, and escalate.

### 4.7 No rhythm shown

**BY DESIGN** — a rhythm classification needs at least **three** counted shots
and timing on **every** shot. Tech Aim shows nothing rather than an unreliable
label.

### 4.8 Group Pattern says insufficient data / gives an unexpected pattern

**BY DESIGN** — pattern analysis needs about five counted shots.

For an unexpected pattern: read the **evidence** and **confidence** shown with
it. The description is a measurement of shape, not a judgement of technique,
and several causes can produce the same shape.

### 4.9 Position results appear mixed together

**WHAT IT MEANS** — a defect. Kneeling, prone and standing are kept separate.

**CORRECTIVE STEPS** — report with the sequence and repeat configuration.

### 4.10 Training session offered for recovery after a clean Home

**WHAT IT MEANS** — a defect. A cleanly closed session must never be offered.

**CORRECTIVE STEPS** — **Discard** only if you are certain it was completed
and exported; otherwise preserve it and escalate.

---

## 5. Reports

### 5.1 EXPORT PDF does nothing / PDF cannot be created

Use [Decision tree C](#c--pdf-will-not-export).

**CHECK FIRST** — is the destination writable, and is a file of the same name
already open in a PDF viewer?

**CORRECTIVE STEPS**
1. Close the file if it is open elsewhere.
2. Export to a folder you certainly own (for example Documents).
3. Avoid `\ / : * ? " < > |` in names.
4. Confirm free disk space.

### 5.2 Report opens blank / logo missing / text clipped

**CORRECTIVE STEPS** — note the report type and page, and attach the PDF when
reporting.

**Known risk:** PDF page rendering has **not** been visually verified in this
build, and additional content was recently added to the Position Transition
comparison page. Report overflow rather than working around it.

### 5.3 German umlauts missing / wrong report language

**Known risk:** German PDF output has **not** been verified.
**[GERMAN REVIEW REQUIRED]**

**CORRECTIVE STEPS** — confirm the UI language, re-export, and attach both the
PDF and a screenshot of the on-screen report.

### 5.4 Report marked Demo unexpectedly

**WHAT IT MEANS** — the session ran in **Demo**. The marking is correct and
must not be removed.

**DO NOT** — edit an exported PDF to remove a Demo marking or the *"Not an
official competition result"* statement.

---

## 6. Recovery

### 6.1 Unfinished session not found

Use [Decision tree B](#b--session-will-not-resume).

**LIKELY CAUSES** — the session was cleanly completed (correct: it will not be
offered); a different Windows user; a different data location.

### 6.2 Wrong session offered / duplicate candidate

**CORRECTIVE STEPS** — check athlete, discipline, mode, phase, shots and saved
time in the dialog before resuming. If they do not match, do **not** resume —
escalate.

### 6.3 Recovered shot count or phase incorrect

**WHAT IT MEANS** — serious. Preserve everything and escalate.

**DO NOT** — continue shooting into a session you believe is wrong; the record
is the evidence.

### 6.4 Recovery fails validation

**WHAT IT MEANS** — the record could not be confirmed intact (for example the
file was truncated by a power cut). Tech Aim reports this rather than silently
loading a damaged session.

**CORRECTIVE STEPS** — do not delete anything; escalate so the record can be
examined.

### 6.5 A completed session is offered for recovery

**WHAT IT MEANS** — a defect. Report it before discarding.

---

## 7. Windows and security

**[WINDOWS RC1 DEPENDENT] — this section is incomplete by design.**

Defender/SmartScreen warnings, "publisher cannot be verified", firewall
prompts, missing DLL, Qt platform plugin errors, multimedia plugin errors,
blocked installation, administrator rights, failed upgrade and failed
uninstall depend on the installer and signing pipeline, which do not exist
yet. They must be completed and validated before external handoff.

> **Under no circumstances** will this guide instruct an operator to disable
> Defender, SmartScreen or the Firewall.

---

## 8. Decision trees

### A — No shot received

```
1. Live or Demo?
   Demo  -> physical shots are rejected by design. Switch to Live, restart.
   Live  -> continue.

2. Is the target connection indicator healthy?
   No    -> section 2.1 (power, cable, COM port).
   Yes   -> continue.

3. Correct discipline selected?
   No    -> Home, reselect, restart the session.
   Yes   -> continue.

4. Is the workflow in a phase that ACCEPTS shots?
   POSITION SETUP        -> shots are ignored BY DESIGN. Select POSITION READY,
                            then START VERIFICATION.
   Session complete      -> no further shots accepted. Start a new session.
   Awaiting a call (C&D) -> confirm the call first.
   Incident unresolved   -> official shots are blocked. Resolve the incident.
   Otherwise             -> continue.

5. Is the target powered and did it register the shot itself?
   No    -> range/equipment fault, not software.
   Yes   -> continue.

6. Is the COM/network configuration correct and the port free?
   No    -> correct it, reconnect.
   Yes   -> continue.

7. Does reconnect (or power-cycle + restart) restore operation?
   Yes   -> resume; report the interruption.
   No    -> ESCALATE.

8. Collect: version + commit, operating mode, discipline, phase,
   COM/network settings, diagnostic log, time of the missing shot.
```

### B — Session will not resume

```
1. Was the session cleanly completed (Home from the summary)?
   Yes -> CORRECT behaviour. Completed sessions are never offered.
   No  -> continue.

2. Was the application closed with Keep for Recovery?
   No / force-closed -> recovery should still apply; continue.

3. Is a recovery candidate shown at startup?
   Yes -> verify athlete, discipline, mode, phase, shots, saved time,
          then Resume Match / Resume Training.
   No  -> continue.

4. Does the session record validate?
   Reported as failing -> do NOT delete anything. ESCALATE.
   No candidate at all -> continue.

5. Same Windows user account and same machine as the original session?
   No  -> session data is per-user and local. Use the original account.
   Yes -> continue.

6. Is the application using the expected data location?
   Confirm the startup log's storage root.

7. Preserve for support: the startup log, the storage root path,
   the athlete/discipline/approximate time, and whether the session
   had been exported.
```

### C — PDF will not export

```
1. Is the destination folder writable by this Windows user?
   No  -> export to Documents instead.
   Yes -> continue.

2. Is a file of the same name already open in a PDF viewer?
   Yes -> close it and retry (Windows locks open files).
   No  -> continue.

3. Does the filename contain \ / : * ? " < > | ?
   Yes -> rename and retry.
   No  -> continue.

4. Is the report itself complete on screen?
   No  -> the session may be incomplete; finish it first.
   Yes -> continue.

5. Is there sufficient free disk space?
   No  -> free space and retry.
   Yes -> continue.

6. Does a DIFFERENT report export successfully?
   Yes -> the problem is specific to that report type. Report which one.
   No  -> the problem is export-wide. Report that.

7. Collect: report type, destination path, exact error text,
   version + commit, operating mode, diagnostic log.
```

### D — Application will not start

**[WINDOWS RC1 DEPENDENT] — complete after packaging.**

```
1. Is TechAim.exe present and complete (not the .exe alone)?
2. Does Windows show a security message?          [RC1]
3. Is a required DLL missing (Qt6*, Qt6Multimedia)?
4. Is the Qt platforms plugin folder present?
5. Is another instance already running (including a legacy Seta.exe)?
6. Did an update partially complete?               [RC1]
7. What do the file properties show (version, publisher)?
```

---

## Related documents

`TechAim_Quick_Start_EN.md` · `TechAim_Operator_Manual_EN.md` ·
`TechAim_German_Translation_Status.md` ·
`TechAim_Manual_Validation_Checklist.md`
