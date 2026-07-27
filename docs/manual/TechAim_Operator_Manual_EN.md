# Tech Aim Electronic Target Control — Operator Manual

Product version 0.9.0 · Release channel: Pre-Beta Validation
Document version 1.0 (P0-J) · Language: English (controlled master edition)
Published 2026-07-27 · Application commit `3741980`
Publisher: JAC SHOOTING SOLUTIONS (PTY) LTD

**Status: Pre-Beta Documentation. Internal evaluation — not for public
distribution.**

> **Verification notice.** This manual is written from the current source
> code, controllers, reachable QML screens and passing tests. Sections marked
> **[MANUAL VALIDATION REQUIRED]** have **not** been performed against a
> running build. Sections marked **[WINDOWS RC1 DEPENDENT]** or **[PHYSICAL
> HARDWARE DEPENDENT]** cannot be completed yet. Per-procedure status is in
> `TechAim_Manual_Validation_Checklist.md`.

**Audience labels:** 🎯 Athlete · 🧑‍🏫 Coach · 🛠 Range Operator

---

## Contents

- [Part 1 — Product overview](#part-1--product-overview)
- [Part 2 — Getting started](#part-2--getting-started)
- [Part 3 — Main screen](#part-3--main-screen)
- [Part 4 — Operating modes](#part-4--operating-modes)
- [Part 5 — Disciplines and events](#part-5--disciplines-and-events)
- [Part 6 — Open Practice](#part-6--open-practice)
- [Part 7 — Training Lab overview](#part-7--training-lab-overview)
- [Part 8 — Technical Blocks](#part-8--technical-blocks)
- [Part 9 — Call & Diagnose](#part-9--call--diagnose)
- [Part 10 — Group Pattern Coach](#part-10--group-pattern-coach)
- [Part 11 — Position Transition](#part-11--position-transition)
- [Part 12 — Reports and PDF export](#part-12--reports-and-pdf-export)
- [Part 13 — Session lifecycle](#part-13--session-lifecycle)
- [Part 14 — Recovery](#part-14--recovery)
- [Part 15 — Range incidents](#part-15--range-incidents)
- [Part 16 — Settings](#part-16--settings)
- [Part 17 — Data and privacy](#part-17--data-and-privacy)
- [Part 18 — Updates and beta feedback](#part-18--updates-and-beta-feedback)
- [Part 19 — Troubleshooting index](#part-19--troubleshooting-index)
- [Part 20 — Glossary](#part-20--glossary)

---

## Part 1 — Product overview

### What Tech Aim is

Tech Aim Electronic Target Control records, scores and analyses shots from an
electronic target. It supports competition-style qualification and final
courses, free practice, and a **Training Lab** of structured measured
exercises.

### Current supported scope

**Competition / practice**
- 10 m Air Rifle, 10 m Air Pistol, 50 m Rifle Prone, 50 m Rifle Three
  Positions qualification workflows
- 10 m Air Rifle Final, 10 m Air Pistol Final, 50 m Rifle 3P Final
- Open Practice

**Training Lab**
- Technical Blocks
- Call & Diagnose
- Group Pattern Coach (an *analysis layer*, not a standalone programme)
- Position Transition (**50 m Rifle Three Positions only**)

**Foundation**
- Live / Demo operating mode, session journaling, replay, crash recovery,
  Range Incident workflow, reports and PDF export, English/German language

### Not included in this build

Wind Map · First Shot & Re-entry · Consistency Chain · SCATT · Shadow
Shooting · cloud services · Android · Range Management coordination · the
SETA blue OEM theme · 25 m Pistol events.

Anything not listed under *supported scope* is not present. Full list:
`docs/pre-beta-feature-scope.md`.

### Beta limitations

1. German is a **partial beta translation** (~100 of 583 strings); the rest
   render in English, so German sessions are mixed-language.
2. German layout and German PDF output are **not visually verified**.
3. The end-user agreement artwork still shows a **SETA-era document** naming a
   different entity. **Legal blocker for public release.**
4. There is **no application icon**; Windows shows its default.
5. 25 m Pistol is unimplemented; the 50 m Rifle 10-ring radius awaits rulebook
   confirmation; the licence-expiry check is disabled.

### Safe operation

- Range safety procedure always overrides the software.
- Never present a **Demo** result as a physical or official result.
- Training reports carry *"Not an official competition result"* — keep it.
- Tech Aim reports **measured** patterns and timing. It does **not** establish
  the technical cause.

### Software identity vs SETA hardware references

The **software product** is **Tech Aim**, published by JAC SHOOTING SOLUTIONS
(PTY) LTD. The executable is `TechAim.exe`.

**"SETA" also names the electronics supplier and the lane hardware
integration.** Where you see SETA in a hardware, lane-server or file-share
context, it correctly refers to that supplier — not to this software. Both can
legitimately appear in the same system.

---

## Part 2 — Getting started

🛠 **Range Operator**

### System requirements

Windows 10/11 PC; an electronic target and its connection for Live operation.
Demo operation needs no target.

**[WINDOWS RC1 DEPENDENT]** — installer, prerequisites and minimum
specification are finalised with the installer.

### First launch

1. Run `TechAim.exe`.
2. Confirm the title bar reads **Tech Aim Electronic Target Control**.
3. Open **Settings ▸ ABOUT / BUILD** and confirm the version, **Commit:** and
   **Built:** values match the build you intended to deploy.

**Expected result:** the home screen appears; only one instance runs.

**[MANUAL VALIDATION REQUIRED]** — title bar and About screen appearance.

### Application identity

| Where | Shows |
|---|---|
| Window title / taskbar | Tech Aim Electronic Target Control |
| Settings ▸ ABOUT / BUILD | product name, version, channel, publisher, commit, build time |
| Windows file properties | Company, description, `TechAim.exe`, 0.9.0.0 |

**VERIFIED AUTOMATICALLY** — version resource and startup identity.

### Language, operating mode, navigation, closing

Language and mode: see [Part 16](#part-16--settings) and
[Part 4](#part-4--operating-modes). Home screen and navigation:
[Part 3](#part-3--main-screen). Close via the window close control; use
**Home** first to close a session cleanly ([Part 13](#part-13--session-lifecycle)).

---

## Part 3 — Main screen

🎯🧑‍🏫🛠

The shooting screen has three regions:

| # | Region | Contents |
|---|---|---|
| 1 | Header | athlete, discipline, connection status |
| 2 | Centre | target face, shot markers, group/MPI overlays, completion summary |
| 3 | Right panel | session status and the primary action button |

In Training Lab sessions the right panel is headed **TRAINING LAB** with the
programme name (**Technical Blocks**, **Call & Diagnose**, **Position
Transition**) beneath, and shows **Sighters fired** plus programme status. It
also states *"Results are shown in the main view."*

### Competition overlays are suppressed during Training

Training Lab screens **must not** show the match countdown clock, the sighter
countdown, or the red `000` raw shot counter. These are competition overlays
and are deliberately gated off for all three Training programmes.

If one appears during Training, that is a defect — see *Timer appears during
Training* and *Red 000 appears during Training* in the troubleshooting guide.

**VERIFIED AUTOMATICALLY** (gating) · **[MANUAL VALIDATION REQUIRED]** (visual).

**Screenshots:** SS-01…SS-04 — see `TechAim_Manual_Screenshot_Register.md`.

---

## Part 4 — Operating modes

🛠

### Live target

**PURPOSE** — score real shots from the physical electronic target.

**WHEN TO USE IT** — all real shooting, all results that will be shown to
anyone as real.

**BEFORE YOU START** — target powered and connected.

**EXPECTED RESULT** — startup log and Settings show the Live mode; physical
shots are accepted. **Simulated input is rejected.**

### Demo / simulation

**PURPOSE** — operate the software without a target, using generated shots.

**WHEN TO USE IT** — learning the software, demonstrations, testing a
workflow, capturing screenshots.

**LIMITATIONS** — Demo results are **not physical results** and must never be
presented as official. **Physical-target input is rejected in Demo.**

### Why the mode is enforced

Tech Aim gates shots by source: Live rejects simulated input and Demo rejects
physical input. A demonstration therefore cannot be mistaken for a real
session, and a real session cannot be quietly padded with generated shots.

### Changing mode

Change it in **Settings ▸ OPERATING MODE** (shows **Current mode:**, with
**Live target** and **Demo / simulation**). Applying it uses **Restart Now**
or **Restart Later**. Restart relaunches the same executable.

**COMMON MISTAKES** — leaving Demo on before a real session; exporting a Demo
report and presenting it as a result.

**[MANUAL VALIDATION REQUIRED]** — the restart round trip.

---

## Part 5 — Disciplines and events

🎯🛠

| Discipline | Qualification | Final |
|---|---|---|
| 10 m Air Rifle | yes | yes |
| 10 m Air Pistol | yes | yes |
| 50 m Rifle Prone | yes | — |
| 50 m Rifle Three Positions | yes | yes (3P Final) |

For each discipline: select it on the home screen, choose the event or
programme, fire sighters, then counted shots; the session ends per the course;
results and reports follow in [Part 12](#part-12--reports-and-pdf-export).

**Important limitations**
- Discipline selection gates the Training Lab: **Position Transition appears
  only for 50 m Rifle Three Positions.**
- 25 m Pistol events are **not implemented**.
- The 50 m Rifle 10-ring radius awaits official confirmation or physical
  calibration; treat 50 m absolute scores as provisional.
- Demo results are never official.

**[MANUAL VALIDATION REQUIRED]** — per-discipline event lists and score
displays. **[PHYSICAL HARDWARE DEPENDENT]** — scoring against known impacts.

---

## Part 6 — Open Practice

🎯

**PURPOSE** — shoot without a fixed competition course.

**HOW IT DIFFERS**

| | Course | Structured exercise | Official |
|---|---|---|---|
| Official match | fixed | no | yes |
| Open Practice | free | no | no |
| Training Lab | free | **yes** | no |

Open Practice gives freedom; the Training Lab adds *structure and measurement*.

**[MANUAL VALIDATION REQUIRED]** — configuration options and end-of-session
behaviour.

---

## Part 7 — Training Lab overview

🎯🧑‍🏫

The Training Lab provides **structured measured exercises**, not just a course
of fire. Each programme defines what to do, when shots count, and what is
measured.

| Programme | Trains | Available for |
|---|---|---|
| Technical Blocks | working on one technical element in short blocks | supported disciplines |
| Call & Diagnose | shot **awareness** — calling where the shot went | supported disciplines |
| Position Transition | setting up and settling into each position | **50 m 3P only** |

**Group Pattern Coach is not a separate programme.** It is an analysis layer
that appears inside the others as **GROUP PATTERN INSIGHTS**.

### The single most important statement in this manual

> **Tech Aim reports measured patterns and timing.
> It does not automatically prove the technical cause.**

A wide group is a measurement. *Why* it was wide — position, hold, trigger,
follow-through, equipment, conditions — is a coaching judgement the software
does not make. Every Training summary is written to respect that boundary, and
so should any coaching conversation built on it.

### Sighters

Across every programme, **sighters are excluded from counted metrics.** They
never enter averages, group diameter, MPI, spread or cadence.

**VERIFIED AUTOMATICALLY.**

---

## Part 8 — Technical Blocks

🎯🧑‍🏫

**PURPOSE** — shoot several short blocks while concentrating on one technical
part of your process. After each block Tech Aim reveals the measured group and
lets you record a note.

**BEFORE YOU START** — discipline selected; a technical focus in mind.

### Visibility modes

| Mode | Shows |
|---|---|
| **Full Hidden** | nothing until review |
| **Group Only** | shot positions without scores |
| **Impact Only** | impacts without scores |

Hiding the score during the block is the point: it keeps attention on the
process rather than the number.

### STEPS

1. Configure the programme (blocks, shots per block, focus, visibility).
2. Fire sighters if you want them. They are excluded from results.
3. Select **START BLOCK** in the right panel.
4. Fire the block. Progress shows as **Shot 0 of N**, counting only counted
   shots.
5. At block end, **Block Review** opens.
6. Read the measured result, add a note under **ATHLETE NOTE**, select
   **Save Note**.
7. Select **CONTINUE TO BLOCK …**, or **End Training** to finish.

**EXPECTED RESULT** — each block produces its own reviewable result; the final
summary compares blocks (**BLOCK COMPARISON**, **AVERAGE SCORE BY BLOCK**,
**GROUP SIZE BY BLOCK**).

### What is measured

| Metric | Meaning |
|---|---|
| Average score | mean of counted shots |
| MPI | mean point of impact (mm) |
| Group diameter | extreme spread (mm) |
| Horizontal / vertical spread | how the group is distributed on each axis |
| Average shot interval (cadence) | mean time **between consecutive counted shots** |
| Timing variation | how much that interval varied |

> **Cadence is measured between shots.** It is the interval from one counted
> shot to the next — not a timestamp, and not the time from the start signal
> to the first shot. A block fired at a steady three-second rhythm reports an
> average interval of about three seconds.
>
> **Note for anyone comparing against earlier builds:** this was previously
> computed incorrectly (it averaged absolute timestamps). Historic "average
> shot time" figures from older builds are not comparable.

**COMMON MISTAKES** — expecting sighters in the metrics (they are excluded);
reading a *measured* pattern as a proven cause.

**TROUBLESHOOTING** — *Start Block unavailable*, *Sighters included in
metrics*, *Group Pattern says insufficient data*.

**RELATED** — [Part 10](#part-10--group-pattern-coach),
[Part 12](#part-12--reports-and-pdf-export).

**VERIFIED AUTOMATICALLY** (metrics, sighter exclusion, cadence) ·
**[MANUAL VALIDATION REQUIRED]** (screen flow).

---

## Part 9 — Call & Diagnose

🎯🧑‍🏫

**PURPOSE** — train **shot awareness**: can the athlete say where the shot
went before seeing it?

**This measures awareness, not accuracy.** An athlete can shoot a modest score
with excellent calling, or a good score with poor calling. The second is the
more urgent coaching problem — it means the result is not yet under
conscious control.

### STEPS

1. Fire the shot.
2. **The actual impact stays hidden.**
3. Mark where you believe the shot went (the **call**).
4. Select **CONFIRM CALL**.
5. Tech Aim reveals **CALL** and **ACTUAL** together with the difference.
6. Review the measured difference (**Target View** / **Comparison** zoom).
7. Select **CONTINUE TO NEXT SHOT**.

**EXPECTED RESULT** — one shot is resolved at a time. The next shot is refused
until the current call is confirmed, so a call can never be attributed to the
wrong shot.

### What is shown

| Element | Meaning |
|---|---|
| Call marker | where the athlete said the shot went |
| Actual marker | where it actually was |
| Connecting vector | direction and size of the error |
| **CALL DIFFERENCE** | radial call error (mm) |
| **HORIZONTAL** / **VERTICAL** | the error split by axis |
| **EXACT CALL — 0.0 mm** | call and actual coincide |
| **OUTSIDE NORMAL TARGET FACE** | actual impact outside the normal face |

The summary reports typical call accuracy, directional bias, shots worth
reviewing (**SHOTS TO REVIEW WITH YOUR COACH**), and observations.

> **Median vs average.** A single wild call distorts an average. The typical
> (median) figure describes normal calling; compare the two — a large gap
> means outliers, not consistently poor calling.

### Warning — call bias is not a sight adjustment

> **A directional call bias is not automatically a sight-adjustment
> recommendation.** It says the athlete's *perception* is offset. Adjusting
> sights to compensate can entrench the perception error. Treat it as a
> coaching observation.

**COMMON MISTAKES** — treating call error as a scoring fault; adjusting sights
from call bias; calling after glancing at the target.

**VERIFIED AUTOMATICALLY** (one-pending-shot rule, hidden actual, analytics) ·
**[MANUAL VALIDATION REQUIRED]** (marker placement, zoom).

---

## Part 10 — Group Pattern Coach

🧑‍🏫

**Group Pattern Coach is an analysis layer, not a homepage programme.** It
appears as **GROUP PATTERN INSIGHTS** inside the Training programmes.

### Supported measured descriptions

tight centred group · tight offset group · wide group · horizontal string ·
vertical string · diagonal string · two clusters · progressive drift · group
expansion or contraction · isolated outlier

Each carries **evidence** (the measurement behind it) and a **confidence**
level. A **primary** description may be accompanied by secondary ones.

### Sample size

Pattern analysis needs enough shots (about five counted shots). Below that it
reports insufficient data rather than guessing — a "pattern" in three shots is
usually noise.

### Why no definite cause is stated

Several different causes produce the same measured pattern. A vertical string
can come from breathing, hold, position settling, follow-through or
conditions. Naming one would be a guess presented as a finding. Tech Aim
therefore states the pattern and its evidence, and leaves cause to the coach —
which is why the output includes discussion prompts (**Coach discussion:**)
rather than instructions.

### 3P position separation

In 50 m 3P, positions are analysed **separately**. Kneeling, prone and
standing have different stability characteristics; pooling them would produce
a meaningless composite.

**Examples** must use synthetic or authorised Demo data. Do not publish real
athlete data without permission.

**VERIFIED AUTOMATICALLY** (analyzer + thresholds).

---

## Part 11 — Position Transition

🎯🧑‍🏫

**Available for 50 m Rifle Three Positions only.**

**PURPOSE** — measure how well the athlete builds each position and settles
into it: how long setup takes, how long until the first counted shot, and how
the early group and rhythm behave.

### Workflow

| Phase | What happens |
|---|---|
| **POSITION SETUP** | build the position. **Shots are ignored.** Optional **SETUP CHECKLIST**. **Setup time** runs. |
| **POSITION READY** | you declare the position built. Timing to the first counted shot starts. |
| Sighters | optional. Excluded from results. |
| **START VERIFICATION** | begins the counted block. |
| Verification | fire N counted shots; **Shot 0 of N**. |
| **Position Review** | measured result for that position. |
| **BEGIN TRANSITION TO …** | move to the next position. |
| Session Summary | **POSITION TRANSITION COMPLETE** |

**Shots fired during POSITION SETUP are deliberately ignored** — you are
building the position, not shooting it. This is not a fault.

### Configuration

Sequence (Kneeling / Prone / Standing and shorter presets), repeats,
verification-shot count, checklist mode, technical focus.

### The timers — each measures something different

| Timer | From → to | Tells you |
|---|---|---|
| Setup / transition time | phase start → **POSITION READY** | how long building the position took |
| Sighter duration | Ready → last sighter | time spent confirming |
| Ready → first counted shot | **POSITION READY** → first counted shot | how long until committing (**includes the sighter phase**) |
| Verification duration | Ready → last counted shot | length of the counted block |
| Average shot interval | between consecutive counted shots | working rhythm |
| Cadence variation | spread of those intervals | how even the rhythm was |

> **Ready → first counted shot includes the sighter phase.** If sighters were
> fired, that time is inside this figure. Compare it only against sessions
> with a comparable sighter policy.

### Rhythm classification — exactly as implemented

Computed from the variation of shot-to-shot intervals relative to their
average (coefficient of variation), needing timing on every shot and at least
three counted shots:

| Label | Meaning |
|---|---|
| **Steady** | intervals close to constant (CV below 0.20) |
| **Variable** | moderate variation (0.20 – 0.40) |
| **Inconsistent** | large variation (0.40 and above) |

> **A rhythm label alone does not prove good or bad technique.** "Steady" can
> mean well-controlled or rushed-and-uncommitted; "Inconsistent" can mean
> disrupted or appropriately patient. Read it with the group and the context.

If there are fewer than three counted shots, or any shot lacks timing, **no
rhythm label is shown** rather than an unreliable one.

### Position separation

Kneeling, prone and standing metrics are kept **separate**.

> **Compare each position against itself across repeats**, not against another
> position. Positions have different stability demands; a wider standing group
> than prone is expected, not a finding.

### Session Summary

**POSITION TRANSITION COMPLETE** contains:

- Overview: positions, sequence, counted shots, sighters, duration
- **SESSION HIGHLIGHTS** — fastest and slowest setup, tightest group, best
  average, steadiest rhythm
- **POSITIONS** — one card per position/repeat with a mini group plot (counted
  shots, first shot highlighted, MPI), setup time, sighters, ready→first shot,
  first-shot result, average, group diameter, MPI X/Y, average shot interval,
  a rhythm badge, comparison bars against the session maxima, and the group
  pattern reading
- **WHAT YOU SHOULD TAKE FROM THIS SESSION** — observations drawn only from
  real measurements
- **EXPORT PDF**, **NEW SESSION**, **Home**

### Clean Home

Selecting **Home** closes the session durably and clears the UI. Reopening
must **not** show stale counters or suggest a session is still in progress.

**VERIFIED AUTOMATICALLY** (phases, ignored setup shots, sighter exclusion,
timers, rhythm classifier, per-position separation, clean-Home lifecycle,
recovery) · **[MANUAL VALIDATION REQUIRED]** (summary layout and PDF).

---

## Part 12 — Reports and PDF export

🧑‍🏫🛠

### Reachable reports

| Report | From |
|---|---|
| Technical Blocks | Training summary |
| Call & Diagnose | Training summary |
| Position Transition | Training summary |
| Qualification / match summary | match completion |
| 10 m Final | final completion |
| 3P Final | final completion |
| Coach report | report window |
| Incident report | incident workflow **[MANUAL VALIDATION REQUIRED]** |

### Exporting

1. Open the summary or report view.
2. Select **EXPORT PDF**.
3. Confirm the destination.

**EXPECTED RESULT** — a Tech Aim–branded A4 PDF with the enlarged logo,
athlete and session metadata, the software attribution (**Tech Aim 0.9.0**),
page numbers, and — for training reports — *"Not an official competition
result"*.

**Competition reports must not carry the training disclaimer**, and training
reports must always carry it.

**IF IT FAILS** — see *PDF will not export* in the troubleshooting guide.

**[MANUAL VALIDATION REQUIRED]** — filenames, default folder, page rendering,
German PDF output (umlauts, wrapping, overflow).

---

## Part 13 — Session lifecycle

🛠

| Stage | Meaning |
|---|---|
| New session | a new session identity is created |
| Active | shots are being recorded |
| Completed | the course finished or the athlete ended it |
| Closed | the session is durably closed and the UI reset |

**Home** on a completed summary closes cleanly. **NEW SESSION** closes the
current session and starts a fresh one with a new identity.

On closing with an active session, Tech Aim offers **Save and Close** or
**Keep for Recovery** (with **Cancel** to go back).

> **Avoid duplicate sessions.** Always finish with **Home** or **NEW
> SESSION**. Force-closing leaves the session unfinished and it will be
> offered for recovery.

**What is stored:** the append-only session record — shots, phases, notes,
timing. **What is not:** video, audio, biometric data.

**VERIFIED AUTOMATICALLY** (clean-close lifecycle for Training programmes).

---

## Part 14 — Recovery

🛠

> **Tech Aim records session progress as it happens, so an interrupted session
> can normally be resumed safely.**

### When it applies

Power failure, application crash, Windows restart, or force-close during an
active session.

### STEPS

1. Restart `TechAim.exe`.
2. If an unfinished session is found, the recovery dialog appears
   (**Unfinished Match Found** / **Recovery Wizard**), showing athlete,
   discipline, mode, phase, shots and when it was saved.
3. Select **Resume Match** / **Resume Training** to continue, or **Discard**.

**EXPECTED RESULT** — the resumed session continues at the correct phase with
the correct shot count. Sighters stay sighters; counted shots stay counted.

### Rules that must always hold

- A **cleanly completed** session is **never** offered as unfinished.
- An unfinished session never appears as completed.
- Recovery never invents or drops shots.

### Integrity, in plain language

Each recorded step is chained to the one before it, so if a file were
truncated by a power cut or altered, Tech Aim can tell where the trustworthy
record ends instead of silently loading a damaged session. If a session fails
this check it is reported rather than quietly used.

**Report immediately** if a completed session is offered for recovery, a
resumed shot count is wrong, or a recovery fails validation. Preserve the data
and see the troubleshooting guide.

**VERIFIED AUTOMATICALLY** (detection, resume, completed-session exclusion).

---

## Part 15 — Range incidents

🛠

**PURPOSE** — record an interruption caused by equipment or the range.

**WHEN** — the electronic target or range prevents normal shooting.

### Categories present in the application

Target not registering · Continuous target fault · Scoring computer failure ·
Target network failure · Application crash · Computer crash · Power failure ·
Communication failure · Target move · Other range-caused failure

Scope: **Individual firing point**, **Selected firing points**, or **Entire
relay / range**.

States include **Awaiting Jury decision** and awaiting official resume. While
an incident is unresolved, official shots are blocked and the screen shows
**DO NOT FIRE — RANGE INCIDENT**.

**STEPS** — open the incident workflow, choose the category and scope, add a
note, save.

> **Tech Aim records the incident. It does not decide the competition
> outcome.** Rule decisions (re-shoots, time allowances, disqualification)
> belong to the Jury.

**[MANUAL VALIDATION REQUIRED]** — dialog flow and incident export.

---

## Part 16 — Settings

🛠

| Setting | Notes |
|---|---|
| **LANGUAGE** | English / Deutsch (Beta). Persists across restarts. |
| **OPERATING MODE** | **Live target** / **Demo / simulation**; shows **Current mode:** |
| **MOTOR FEED (SECONDS)** | paper-feed timing |
| **Background Color**, **Pellet Color** | target display appearance |
| **ABOUT / BUILD** | product, version, channel, publisher, **Commit:**, **Built:** |

Actions: **Save**, **Cancel**, and **Restart Now** / **Restart Later** where a
restart is needed.

Language and brand are independent: **selecting German never changes the logo,
theme, executable name, publisher or data location**, and never changes the
operating mode.

**VERIFIED AUTOMATICALLY** (language persistence, fallback, brand
independence) · **[MANUAL VALIDATION REQUIRED]** (other controls).

---

## Part 17 — Data and privacy

🛠

| Data | Location |
|---|---|
| Session records, recovery data | Windows local application data, under a **TechAim** folder |
| Exported PDFs | the folder chosen at export |
| Diagnostic log | the Windows temporary folder |
| Settings, mode, language | `config.ini` beside the executable |

**Exact paths are confirmed at Windows RC1. [WINDOWS RC1 DEPENDENT]**

- **Uninstall** does not remove exported reports; treat application data as
  removable and back up anything you must keep.
- **Back up exported PDFs** — they are the durable record.
- **Athlete notes are stored verbatim** in the session record and appear in
  reports. Do not enter medical, personal or sensitive information.
- **Support bundles:** send only version, mode, discipline, the description of
  the problem, and the relevant report. Do not send athlete personal data that
  is not needed.

---

## Part 18 — Updates and beta feedback

🛠

Version format: `0.9.0` plus a release channel (**Pre-Beta Validation**) and a
build commit. Check them in **Settings ▸ ABOUT / BUILD**; the commit
identifies the exact source the build came from.

**[WINDOWS RC1 DEPENDENT]** — installation, update, rollback and uninstall
procedures are defined with the installer and must be completed before
external handoff.

**Feedback should include:** version and commit, operating mode, discipline
and programme, what you did / expected / observed, and the exported report if
relevant.

---

## Part 19 — Troubleshooting index

Full guide: `TechAim_Troubleshooting_EN.md`.

| Area | Typical symptoms |
|---|---|
| Application | will not open, closes immediately, already running, wrong language, clipped text, frozen, crashed |
| Target connection | offline, no shot received, late/duplicate/unexpected shot, COM port, mode-rejected input |
| Scoring / coordinates | shot on wrong side, axis reversed, unexpected score, sighter/counted confusion |
| Training Lab | programme unavailable, timer or red `000` during Training, action unavailable, call cannot be confirmed, sighters in metrics, no rhythm, insufficient pattern data, mixed positions |
| Reports | export does nothing, PDF not created, blank report, missing logo, clipped text, missing umlauts, permission denied |
| Recovery | session not found, wrong session, wrong shot count or phase, validation failure, completed session offered |
| Windows / security | **[WINDOWS RC1 DEPENDENT]** |

---

## Part 20 — Glossary

| Term | Meaning |
|---|---|
| **Decimal score** | score to one decimal place, from exact impact geometry |
| **Sighter** | practice shot; never included in counted results |
| **Counted shot** | a shot recorded and scored as part of the session |
| **MPI** | mean point of impact — the average centre of the group |
| **Group diameter** | extreme spread; widest distance across the group |
| **Horizontal / vertical spread** | distribution on each axis |
| **Call** | where the athlete believes the shot went |
| **Actual impact** | where the target measured it |
| **Radial error** | straight-line distance between call and actual |
| **Cadence** | average interval **between consecutive counted shots** |
| **Rhythm** | how even that cadence was — Steady / Variable / Inconsistent |
| **Verification shot** | a counted shot in a Position Transition block |
| **Transition** | moving from one shooting position to the next |
| **Demo** | simulated shots, no physical target; never an official result |
| **Live** | physical target input |
| **Recovery** | resuming an interrupted session from its recorded progress |
| **Journal** | the append-only record of everything that happened |
| **Incident** | a recorded equipment or range interruption |
| **Group pattern** | a measured description of group shape |
| **Confidence** | how strongly the measurement supports a pattern |

---

## Related documents

`TechAim_Quick_Start_EN.md` · `TechAim_Troubleshooting_EN.md` ·
`TechAim_German_Translation_Status.md` ·
`TechAim_Manual_Screenshot_Register.md` ·
`TechAim_Manual_Validation_Checklist.md` ·
`TechAim_Manual_Review_Findings.md`
