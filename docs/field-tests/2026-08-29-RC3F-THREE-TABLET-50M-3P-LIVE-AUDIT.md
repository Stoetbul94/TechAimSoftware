# RC3F Three-Tablet Live 50 m 3P Audit

**2026-08-29 — three tablets, three athletes, live 50 m targets, ISSF 3P Indoor
Qualification and 3P Final. Read-only forensic audit. No production code was
changed.**

**Headline: the acquisition engine and both competition state machines behaved
correctly on all three tablets. 385 accepted shots, 385 distinct coordinates,
385 paper feeds, zero acquisition faults. Two announcement-ordering defects
were found in RC3F's own new CRO code, both presentation-only.**

## Build identity

| | Tablet 1 (Arnold) | Tablet 2 (Bernard) | Tablet 3 (Chantelle) |
|---|---|---|---|
| `TechAim.exe` SHA-256 | **MATCH** | **MATCH** | **MATCH** |
| `.zip` SHA-256 | **MATCH** | **MATCH** | **MATCH** |
| Version | 0.9.0-RC3F-DIAG | 0.9.0-RC3F-DIAG | 0.9.0-RC3F-DIAG |
| Commit | 39df782 | 39df782 | 39df782 |
| `app_mode` / `developer_mode` | Live / 1 | Live / 1 | Live / 1 |
| Serial | 19200 Even 8 1, COM3 | 19200 Even 8 1, COM5 | 19200 Even 8 1, COM7 |
| **Verdict** | **VERIFIED** | **VERIFIED** | **VERIFIED** |

Expected `F27A6743F9114B60B6BAD8B919A995E51E47A7DC86F726B4CDA2BBFB82796856`
(exe) and `864A5F5711C1365D64CDB3C269BCD7F92CCFF2327244539F7611A0FB948617B3`
(zip) — both matched byte-for-byte on all three, in both the extracted folder
and the `staging` copy. All three ran the same binary.

`is_single_decimal=1` is the **coordinate divider** (tenths of a millimetre from
the hardware), not a scoring mode. Correctly configured.

## Evidence inventory

Each folder carries the full RC3F package, its manifest, its ZIP, the extracted
runtime, and the session artefacts:

| Tablet | Logs | `.tch` | Reports |
|---|---|---|---|
| 1 | `tachus_log29082026-081712.log` (25 402 lines, 08:17→12:38) | `Match_29082026-090700.tch` (67 rows), `Match_29082026-120038.tch` (46) | `Arnold_Match.pdf`, `summary_report.pdf`, `finals_report.pdf` |
| 2 | three logs: `081536` (13 677), `104421` (3 398), `113915` (6 284) | `…090700.tch` (72), `…120039.tch` (49) | `Bernard_Match.pdf`, `summary_report.pdf`, `finals_report.pdf` |
| 3 | `tachus_log29082026-081358.log` (26 102 lines, 08:13→12:41) | `…090700.tch` (74), `…120037.tch` (47) | `Chantelle_Match.pdf`, `summary_report.pdf`, `finals_report.pdf` |

**Not collected: the finals session journals** (`finals_session*.jsonl`). The 3P
Final's internal stage transitions are journalled, not written to the tachus
log, so the Final's stage-by-stage timing **cannot be reconstructed from this
evidence**. That is the single largest gap in the set.

**Tablet 2 restarted the application twice** (three logs). Nothing in the logs
indicates a crash; the restarts sit between sessions.

The report PDFs are image-based (`grabToImage` → PDF), so no text could be
extracted from them.

## Acquisition — all three tablets

| | T1 | T2 | T3 |
|---|---|---|---|
| Accepted physical shots | **123** | **131** | **131** |
| Distinct coordinates | **123** | **131** | **131** |
| Repeated coordinates | 0 | 0 | 0 |
| `ACQUISITION_FAULT` | **0** | **0** | **0** |
| `ACQ_COORD_READ_FAILED` | 0 | 0 | 0 |
| `ACQ_COORD_REFUSED_BY_UI` | 0 | 0 | 0 |
| `COUNTER JUMPED` | 0 | 0 | 0 |
| `AdoptionWouldDesync` | 0 | 0 | 0 |
| `COMMUNICATION INTERRUPTION` | 0 | 0 | 0 |
| `ACQ_COORD_INDEX_INVALID` | 2 | 6 | 2 |
| Auto feed requested / started / completed | 123/123/123 | 131/131/131 | 131/131/131 |
| Manual feeds | **0** | **0** | **0** |
| Motor move / stop | 123/123 | 131/131 | 131/131 |

**385 accepted shots, 385 distinct coordinates, 385 feeds, 385 motor commands.**
Every coordinate unique; no shot fed twice and none unfed. The
`ACQ_COORD_INDEX_INVALID` counts are exactly two per application launch — the
documented benign pre-shot report probe (Tablet 2 launched three times, hence
six).

## Shot reconciliation

The backend tags a shot `(counted)` or `(sighter)` from the **legacy** sighter
flag. During a 3P position change QML routes shots to the sighter model but
deliberately does **not** call `MODREADER.changeSighterMode()` (it races the
polling worker), so position-change sighters carry the `counted` tag. Reading
the tag alone would overcount officials. Split by position instead:

| | T1 | T2 | T3 |
|---|---|---|---|
| Pre-match preparation sighters (`sighter` tag) | 10 | 10 | 10 |
| Kneeling block | 20 = **20 official** + 0 | 20 = **20** + 0 | 20 = **20** + 0 |
| Prone block | 24 = **20 official** + 4 sighters | 30 = **20** + 10 | 26 = **20** + 6 |
| Standing block | 23 = **20 official** + 3 sighters | 22 = **20** + 2 | 28 = **20** + 8 |
| **Qualification official** | **60** | **60** | **60** |
| Qualification `.tch` rows | 67 | 72 | 74 |
| Final accepted | 46 | 49 | 47 |
| Final `.tch` rows | 46 | 49 | 47 |
| **Total accepted** | 10+67+46 = **123** ✓ | 10+72+49 = **131** ✓ | 10+74+47 = **131** ✓ |

**Every shot on every tablet is accounted for.** No missing shot, no duplicate,
no unexplained coordinate.

Both `.tch` files hold officials **and** position-change sighters
undifferentiated — the same class as `FINALS-TCH-SIGHTER-001`, now confirmed
for 3P qualification as well as the 10 m Final. Harmless to scoring (the
official count comes from `globalMatchModel`), but a report or reload that
treats `.tch` rows as officials would be wrong.

## Reconnect

| Tablet | Event | Reconciliation |
|---|---|---|
| 1 | 08:17:19 LOST → 08:28:17 RESTORED (COM3, 317 attempts) | pre-session setup; baseline 0, target 0, coords 0 |
| 2 | 08:15:42 LOST → 08:29:20 RESTORED (COM5, 394 attempts) | pre-session setup |
| 3 | 08:14:06 LOST → 08:29:42 RESTORED (COM7, 451 attempts) | pre-session setup |
| **3** | **09:37:48 LOST → 09:37:51 RESTORED (2.3 s, 2 attempts)** | **mid-qualification**, 20 s into Prone sighting: *baseline was 0, target reports 0, coordinates held 20* |
| 3 | 12:40:42 LOST, no RESTORED | end of session, after the last shot |

**Tablet 3's mid-qualification reconnect is the only one that exercised the
reconnect path under competition conditions.** `baseline == target reports`, 20
coordinates held (the completed Kneeling block), and the session continued to a
correct 60-shot completion. No duplicate shot, no missing shot, no baseline
jump, no timer restart, no position change, no Finals10m activation.

## Qualification — rule conformance

Rule 6.11.9.2 and 6.11.1.1/6.11.1.2, ISSF Rule Book 2026 Edition 2025 (Second
Print 07/2026).

| | T1 | T2 | T3 |
|---|---|---|---|
| `is3P = true`, prep 900 s | ✓ | ✓ | ✓ |
| Preparation start | 09:07:00 | 09:07:00 | 09:06:59 |
| `MATCH FIRING...START` | 09:22:17 | 09:22:09 | 09:22:15 |
| Prep duration | 15:16 | 15:09 | 15:16 |
| Position changes | exactly **2** | **2** | **2** |
| Order | K → P → S | K → P → S | K → P → S |
| Last official shot | 10:39:00 | 10:31:39 | 10:41:31 |
| Elapsed inside the 90 min | 76:43 | 69:30 | 79:16 |
| Match window would end | 10:52:17 | 10:52:09 | 10:52:15 |

All three completed 60 official shots **inside one 90-minute window**. Had the
clock restarted at either position change, the elapsed times would not fit a
single continuous window in the way they do; nothing in any log shows a match
timer restart, and `3P: position change → … (sighting, match clock keeps
running)` appears at every transition.

**QUALIFICATION: PASS on all three tablets.**

## Final — rule conformance

Each tablet started the Final at 12:00:37–12:00:39 and produced a
`finals_report.pdf` and a Final `.tch`. Accepted shots 46 / 49 / 47, consistent
with 35 official plus 11 / 14 / 12 sighters.

**However — the Final's stage transitions are journalled, not logged to the
tachus log, and the journals were not collected.** The 22-minute block, its
warnings, the STOP, the 30-second interval, the two 250-second series and the
five 50-second singles **cannot be verified from this evidence set**.

**FINAL: PARTIAL — completed on all three tablets, internal state flow NOT
PROVEN from the collected evidence.**

## Historical regression matrix

| Defect | T1 | T2 | T3 | Overall |
|---|---|---|---|---|
| ACQ-FLUSH-001 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** (0 faults across 385 shots and 19 flush boundaries) |
| ACQ-DESYNC-002 | NOT EXERCISED (pre-session only) | NOT EXERCISED | **NOT OBSERVED** (mid-match reconnect reconciled) | **PHYSICALLY CLEAR on T3** |
| ACQ-SENTINEL-003 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** (385/385 distinct) |
| ACQ-READ-004 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** |
| SERIAL-DEFAULT-005 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** (19200/Even/8/1 on all) |
| THREAD-MODBUS-006 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** |
| QML-SHOT-001 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** |
| PAPER-FEED-002 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** (1:1 on 385) |
| UI-STATUS-001 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** |
| FINALS-TIMER-001 / -002 | NOT PROVABLE | NOT PROVABLE | NOT PROVABLE | journals not collected |
| UI-LAYOUT-001 | NOT PROVABLE | NOT PROVABLE | NOT PROVABLE | no screenshots |
| FINAL-TCH-TIME-001 | NOT PROVABLE | NOT PROVABLE | NOT PROVABLE | 3P Final `.tch`, not 10 m |
| FINALS-TCH-SIGHTER-001 | **OBSERVED** | **OBSERVED** | **OBSERVED** | `.tch` mixes officials and sighters in **both** disciplines |
| FINALS-3P-MIX-001 | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | no 10 m contamination in any log |
| FINALS3P-FLOW-001 | NOT PROVABLE | NOT PROVABLE | NOT PROVABLE | journals not collected |
| Qualification 90-min continuity | NOT OBSERVED | NOT OBSERVED | NOT OBSERVED | **PHYSICALLY CLEAR** |
| Qualification CRO commands | **DEFECTS FOUND** | **DEFECTS FOUND** | **DEFECTS FOUND** | see below |

---

## UI-LASTSHOT-DWELL-001 — CONFIRMED, and measured

**Severity: SEV-3. Presentation only. Competition behaviour correct.**

### Measured dwell

Time from the last shot's marker appearing to the target face being cleared by
the position transition:

| Boundary | T1 | T2 | T3 |
|---|---|---|---|
| Kneeling → Prone (official shot 20) | **1.92 s** | **1.63 s** | **1.90 s** |
| Prone → Standing (official shot 40) | **2.03 s** | **1.82 s** | **1.73 s** |

**Mean 1.84 s, range 1.63–2.03 s.** The operator's report is objectively
correct: the last shot of a position is visible for under two seconds.

### The trace (Tablet 1, shot 20)

```
09:33:31.544  SHOTTRACE 20 decoded x=-12.2 y=-2.9
09:33:31.555  scored 9.43
09:33:31.591  SHOTTRACE 20 qml-marker-added        <- the shot becomes visible
09:33:31.593  update shootdat file -> count 20
09:33:31.594  physical shot accepted: seq 20 (counted)
09:33:31.594  paper feed requested
09:33:32.983  paper feed completed
09:33:32.985  ACQ_RESET issued
09:33:33.509  3P: position change -> PRONE          <- the face is cleared
                                                       DWELL = 1.918 s
```

### Root cause

`ShootingPage.qml::enterPositionTransition()` calls
`globalModelOfData.clear()` and repopulates the buffer with only the sighters
for the **new** position. The target face is bound to that buffer, so every
marker for the completed position disappears at once.

It is triggered by the `positionWatch` `Timer` (500 ms poll) when
`globalMatchModel.count` reaches 20 or 40. The observed ~1.8 s is therefore
the paper-feed duration (~1.39 s) plus up to one 500 ms poll interval — an
incidental delay, not a designed dwell. **Nothing in the product currently
holds the last shot on purpose.**

### What is and is not affected

| Boundary | Mechanism | Affected? |
|---|---|---|
| Qualification shot 20, 40 | `positionWatch` → `enterPositionTransition()` clears the buffer | **YES — measured** |
| Qualification shot 60 | `matchCompleteWatch` → `changedToMatchFinish()`; the face is **not** cleared | **NO** |
| Final shots 10, 20 | `Finals3PController` marks the stage COMPLETE and waits for the **athlete** to advance | **NO** — the athlete controls when it moves |
| Final shots 25, 30 | series boundary, controller-driven | **NOT PROVEN** (journals not collected) |
| Final shots 31–35 | single-shot boundaries | **NOT PROVEN** |
| Final shot 35 | completion | **NOT PROVEN** |

**The root cause is position-transition-specific in the qualification.** It is
not "every new series clears the presentation" and it is not the target-mode
change: `changedToMatchMode()` also clears `globalModelOfData`, but that runs
when the athlete chooses to resume, not automatically.

### Answers to the state-versus-presentation question

| Question | Answer | Evidence |
|---|---|---|
| Competition state correct? | **YES** | 60 official on all three; 2 transitions; correct order |
| Target mode correct? | **YES** | MATCH → SIGHTING at each transition, SIGHTING → MATCH on resume |
| Match clock correct? | **YES** | one continuous 90-minute window on all three |
| Shot persistence correct? | **YES** | 385/385 accounted, `.tch` written, feed 1:1 |
| Only visual presentation too fast? | **YES** | the shot is persisted, counted and fed before the face clears |

**The controller is right and the picture goes away too soon.** No state
machine, clock or acquisition change is warranted.

### UX options

| Option | Rules safety | Clarity | Complexity | Stale-visual risk |
|---|---|---|---|---|
| A — no change | safe | poor: under 2 s | none | none |
| B — 2.0 s hold | safe | modest gain over 1.8 s | low | low |
| **C — 2.5 s hold** | **safe** | **clear gain** | **low** | **low** |
| D — 3.0 s hold | safe | good | low | starts to feel frozen |
| E — hold + "POSITION COMPLETE" overlay | safe | best | medium | low |
| F — manual acknowledgement | **REJECT** | — | medium | **blocks the workflow; an athlete on the clock must never wait for a tap** |

**Recommended: option C+E — a 2.5-second visual hold of the last shot's marker
and score, with a "KNEELING COMPLETE / NEXT: PRONE SIGHTING" caption.**

2.5 s is chosen because the measured dwell is already ~1.8 s: the perceived
gain must be real, and 2.5 s adds about 40 % without the screen feeling
stalled. It is also comfortably shorter than the fastest observed
shot-to-shot interval, so it cannot still be on screen when the next shot
arrives.

### Design constraints — non-negotiable

1. **The hold is presentation only.** The controller advances, the clock runs,
   the target mode changes, persistence completes and the feed fires exactly as
   they do now.
2. **A shot arriving during the hold is interpreted by the NEW state.** The
   held card must never be consulted for routing. The safest implementation
   holds a *snapshot* (score, marker position, position name) in a separate
   overlay, not the live model.
3. **The CRO command / state area must update immediately.** A held card must
   never delay `STOP`, `MATCH FIRING`, `LOAD` or `START`. Hold the last-shot
   card and marker; never freeze the whole right panel.
4. **No competition time may be added anywhere.**

The operator's need is most likely **both** the score number and the marker —
the trace shows the marker and the score become visible at the same instant and
disappear at the same instant, so holding one without the other would only half
solve it.

---

## Two defects found in RC3F's own CRO announcements

These are in code added for RC3F and are **presentation-only**. Both appear on
all three tablets.

### CRO-ORDER-001 — the preparation STOP is announced *after* MATCH FIRING

```
09:22:17.112  CRO: MATCH FIRING...START
09:22:17.250  CRO: END OF PREPARATION AND SIGHTING...STOP     <- 138 ms LATER
```

Rule 6.11.1.1 j) then 6.11.1.2 a): `END OF PREPARATION AND SIGHTING...STOP`,
a ~30 second pause for the target reset, **then** `MATCH FIRING...START`. The
software emits them in the wrong order and with no pause between them, because
the sighting timer's expiry path calls the transition (which announces MATCH
FIRING) *before* the line that announces the end of preparation.

**SEV-3.** No competition state is wrong; the announcements are misleading.

### CRO-REPEAT-002 — `MATCH FIRING...START` announced at every position change

It appears three times per tablet: once correctly at the start of the match,
then again at each position change (T1 09:42:14 and 10:20:51), because
`changedToMatchMode()` announces it whenever the athlete resumes match firing.
The rule has one `MATCH FIRING...START` for the match.

**SEV-3.** Harmless operationally, incorrect as a CRO command.

### The 10- and 5-minute warnings were NOT EXERCISED

They never fired on any tablet — **and correctly so**. All three athletes
finished the course before the 10-minute mark:

| | match ends | 10-min warning due | last shot | |
|---|---|---|---|---|
| T1 | 10:52:17 | 10:42:17 | 10:39:00 | finished 3:17 early |
| T2 | 10:52:09 | 10:42:09 | 10:31:39 | finished 10:30 early |
| T3 | 10:52:15 | 10:42:15 | 10:41:31 | finished **44 s** early |

The regression proves the threshold logic; the field did not reach it. Recorded
as **NOT EXERCISED**, not as evidence of correctness. Tablet 3 came within 44
seconds.

---

## Physical qualification status

| | Status |
|---|---|
| **50 m 3P Indoor Qualification** | **PHYSICALLY PASSED ON 3 TABLETS** — 60 official shots each, correct course and order, one continuous 90-minute clock, full shot reconciliation, zero acquisition faults |
| **50 m 3P Final** | **PHYSICALLY COMPLETED ON 3 TABLETS; INTERNAL STATE FLOW NOT PROVEN** — the finals journals were not collected |
| Acquisition engine (RC3 hardening) | **PHYSICALLY PASSED** — 385 shots, 385 coordinates, 385 feeds, 0 faults |

## RC3F freeze decision

**KEEP RC3F FROZEN.**

Nothing found is a competition defect. `UI-LASTSHOT-DWELL-001` is SEV-3 comfort;
`CRO-ORDER-001` and `CRO-REPEAT-002` are SEV-3 announcement wording and order.
None affects a score, a clock, a shot count or a state transition. There is no
case for an RC3G before these are reviewed and batched together.

## Remaining open items

1. **Collect the finals session journals** (`Sessions\…\finals_session*.jsonl`)
   from the next Final. Without them the 3P Final's 22-minute block, warnings,
   STOP, interval, series and singles cannot be verified in the field — the
   single largest gap in this evidence set. `Collect-Logs.cmd -SessionId <id>`
   gathers them.
2. `UI-LASTSHOT-DWELL-001` — implement option C+E when the freeze lifts.
3. `CRO-ORDER-001`, `CRO-REPEAT-002` — fix together with 2.
4. `FINALS-TCH-SIGHTER-001` — now confirmed in 3P qualification as well.
5. No screenshots were taken, so `UI-LAYOUT-001` remains unprovable.
