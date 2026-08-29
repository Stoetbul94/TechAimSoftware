# Cross-platform correctness fix register

**Purpose: no Tech Aim build may quietly lose a correctness fix another one
already has.**

Windows Tech Aim, Android Tech Aim and the SETA-branded build are the same
product. They must not independently reimplement a fix, and they must not ship
without one. This register is the checklist that makes that visible.

It is a portability record, not a permission to redesign anything.

---

## How to maintain this (release discipline)

Whenever a release-impacting defect is fixed:

1. **Assign or update the defect ID.** Use the existing ID if the defect is a
   recurrence; a new one only for a genuinely new defect.
2. **Add or update its row here**, with root cause and authoritative fix.
3. **State cross-platform applicability** — shared core or platform-specific.
4. **State the automated regression** that would catch it coming back. A fix
   with no regression is recorded as such, honestly.
5. **State physical validation** separately from code status. They are
   different claims and this document never merges them.
6. **State whether Android and SETA need a retest**, and why.

A fix that is not in this register is a fix the other platforms will lose.

---

## Status vocabulary

| Status | Means |
|---|---|
| `YES` | present and verified in that build's source |
| `NO` | verified absent — that build does not have this fix |
| `PARTIAL` | present but adapted, or present without its regression |
| `N/A` | does not apply to that platform |
| `PHYSICAL PENDING` | the code is in; nobody has fired a rifle at it on that platform |

`PASS` anywhere in this document means **code verified**. Physical status is
always its own column. **Never record DEMO or automated qualification as
physical qualification.**

---

## How the per-branch status below was determined

Not from memory. Each fix was probed by asking the branch itself whether its
authoritative marker is present:

```
git show <branch>:<file> | grep <marker>
```

Branch state at the time of writing:

| Build | Branch | HEAD | Commits behind `feature/rc2e-latency-and-reset` |
|---|---|---|---|
| Windows Tech Aim | `feature/rc2e-latency-and-reset` | current | — |
| Android Tech Aim | `feature/android-tablet` | `b60867c` | **33** |
| SETA OEM | `product/seta` | `f22637f` | **34** |

**A marker probe proves the fix's code is present. It does not prove the fix
behaves correctly on that platform** — for Android in particular, see §
Android carryover.

---

## ⚠ The headline finding

**Android and SETA are missing almost every acquisition correctness fix.**

Both branches predate the RC3 hardening round. Neither carries the seven
acquisition defects' fixes, the QML shot-path fix, the target-readiness fix,
either finals timer fix, the finals mode-isolation fix, the theme layer, or
the new 3P Final panel.

This is not a criticism of those branches — they simply branched earlier. It
is exactly the situation this register exists to make impossible to forget.
**Neither Android nor SETA should be shipped or evaluated until the shared
correctness fixes below are carried across.**

---

## The register

### Acquisition — the RC3 hardening round

These seven were found by the 2026-08-23 four-tablet forensic investigation,
in which one tablet recorded fourteen false 10.8s. They are the reason the
acquisition engine is now frozen.

---

#### ACQ-FLUSH-001 — 10-shot counter reset race

| | |
|---|---|
| **First observed** | RC2g, 2026-08-23 physical test |
| **Symptom** | `ACQUISITION_FAULT` at every 10-shot boundary — twelve times out of twelve |
| **Root cause** | The application asked the target to reset its counter, then judged the resulting `delta=-10` as a fault before the target had confirmed. A ~2.6 s race between request and confirmation |
| **Authoritative fix** | `AcquisitionSequencer` gained an explicit `ResettingCounter` state: the poll judges nothing while a reset it requested is outstanding, and adopts the new baseline only on target confirmation (~100 ms) |
| **Source** | `src/target/AcquisitionDecision.h`, `src/target/AcquisitionSequencer.h`, `ModReader/forms/tachuswidget.cpp` |
| **Regression** | `tests/reliability` — the flush-boundary tests, plus emulator scenarios |
| **Physical** | **PASS** — RC3B (38 shots), RC3C (43 shots, 3 boundary crossings, 0 faults) |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ |
| **Must carry forward** | **YES — critical** |
| **Notes** | Nothing platform-specific. Compiling the same `src/target` gets the fix |

---

#### ACQ-DESYNC-002 — reconnect adopted a mismatched counter

| | |
|---|---|
| **First observed** | Tablet-02, 2026-08-23 |
| **Symptom** | After a reconnect the coordinate index and the hardware counter disagreed; the application read coordinates that were never measured |
| **Root cause** | Reconnect adopted the target's counter without proving it was consistent with the coordinates already held |
| **Authoritative fix** | `planBaselineAdoption()` proves `priorTotal + baseline == capturedShots` and raises `AdoptionWouldDesync` rather than resuming on a mismatch. A detectable acquisition error is always preferable to a plausible but false score |
| **Source** | `src/target/AcquisitionDecision.h`, `ModReader/forms/tachuswidget.cpp` |
| **Regression** | `tests/reliability`, including a `LegacyTachus` reconstruction proving the old code reached `-1/-1` |
| **Physical** | **PASS** — RC3B 4 reconnects, RC3C 5 reconnects, `baseline == target reports` in every one |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ |
| **Must carry forward** | **YES — critical** |
| **Notes** | Android USB reconnect is a *different event* from a Windows COM reconnect but reaches the same reconciliation. The reconciliation is shared; the reconnect trigger is not — see § Android |

---

#### ACQ-SENTINEL-003 — `-1 / -1` scored as a plausible 10.8

| | |
|---|---|
| **First observed** | Tablet-02, 2026-08-23 — fourteen identical 10.8s |
| **Symptom** | A missing coordinate was returned as `-1 / -1`, which is a valid position 1.4 mm from centre, and scored as a near-perfect shot |
| **Root cause** | A sentinel value indistinguishable from real data, consumed without a validity question |
| **Authoritative fix** | `getXCord`/`getYCord` return `NaN` and raise `ACQ_COORD_INDEX_INVALID`; every consumer asks `coordinateHasValue()` first. All 11 coordinate consumers enumerated and guarded |
| **Source** | `ModReader/forms/tachuswidget.{h,cpp}`, `CenterPane.qml`, `src/bridge/coachreportfeeder.cpp`, `appsettings.cpp` |
| **Regression** | `tests/reliability`, `tests/qml` |
| **Physical** | **PASS** — RC3B and RC3C: 0 sentinel pairs, 0 repeated coordinates, all coordinates distinct |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ + SHARED QML |
| **Must carry forward** | **YES — critical** |
| **Notes** | The QML half (`coordinatesUsable`) must be visually re-validated on Android; the C++ half is automatic |

---

#### ACQ-READ-004 — the coordinate read result was ignored

| | |
|---|---|
| **First observed** | RC3 source audit, 2026-08-24 |
| **Symptom** | A failed Modbus read of a coordinate was not detected; whatever was in the buffer was used |
| **Root cause** | The return code of the coordinate read was not checked |
| **Authoritative fix** | `coordRc < 0` logs `ACQ_COORD_READ_FAILED`, raises a fault and returns before any coordinate is appended |
| **Source** | `ModReader/forms/tachuswidget.cpp` |
| **Regression** | `tests/reliability` |
| **Physical** | **PASS** — RC3B and RC3C: 0 read failures |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ |
| **Must carry forward** | **YES — critical** |

---

#### SERIAL-DEFAULT-005 — wrong serial defaults, and a scan storm

| | |
|---|---|
| **First observed** | Tablet-01, 2026-08-23 — 29 port scans a minute for 74 minutes |
| **Symptom** | The application defaulted to generic serial parameters and scanned continuously instead of connecting |
| **Root cause** | Generic Modbus defaults (9600 / None) rather than the Tech Aim target's profile |
| **Authoritative fix** | The default is the Tech Aim target: **19200 / Even / 8 / 1**. Scans run only while genuinely disconnected |
| **Source** | `ModReader/src/modbuscommsettings.cpp` |
| **Regression** | `tests/reliability` serial assertions |
| **Physical** | **PASS** — RC3B and RC3C: every connect line `19200 Even 8 1`, zero scans while connected |
| **Windows** | YES |
| **Android** | **NO** — and **not directly portable** |
| **SETA** | **NO** |
| **Shared?** | **WINDOWS-SERIAL-SPECIFIC** |
| **Must carry forward** | **YES, as an equivalent** |
| **Notes** | Android has no COM ports. The *value* (19200/Even/8/1) is the target's and carries over; the *mechanism* (QSerialPort enumeration, COM naming, auto-connect scan policy) needs an Android USB-host equivalent. This is the single most platform-divergent item in the register |

---

#### THREAD-MODBUS-006 — unserialised Modbus transport

| | |
|---|---|
| **First observed** | RC3 source audit, 2026-08-24 |
| **Symptom** | Concurrent access to the Modbus transport; potential torn reads |
| **Root cause** | The transport was reachable from more than one context without serialisation |
| **Authoritative fix** | A mutex (`m_modbusTransport`) guards every transport call |
| **Source** | `ModReader/src/mainwindow.cpp`, `ModReader/forms/tachuswidget.h` |
| **Regression** | `tests/reliability` |
| **Physical** | **PASS** — no communication interruptions in RC3B or RC3C |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ |
| **Must carry forward** | **YES — critical.** Android's USB transport is asynchronous; if anything, serialisation matters more there |

---

#### LOG-DEFECT-007 — log corruption and dropped entries

| | |
|---|---|
| **First observed** | 2026-08-23 investigation — the log could not be trusted as evidence |
| **Symptom** | Interleaved and dropped log lines under concurrent writers |
| **Root cause** | Unserialised append and a missing per-entry flush |
| **Authoritative fix** | Serialised append with a stable `hh:mm:ss.zzz` stamp and a label per `LogType` |
| **Source** | `ModReader/src/logfile.cpp` |
| **Regression** | `tests/reliability/tst_logging_integrity.cpp` — 3 threads × 400 entries, none interleaved |
| **Physical** | **PASS** — RC3C's 4 449-line log was fully parseable and reconciled to the shot |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ (the log *path* is platform-specific — see § Android) |
| **Must carry forward** | **YES.** Without it, no field evidence from that platform can be trusted |

---

### Shot path, feed and display

---

#### QML-SHOT-001 — ReferenceError aborted the shot handler

| | |
|---|---|
| **First observed** | RC2a |
| **Symptom** | Severity 1: a shot scored but was not counted, saved or reported |
| **Root cause** | A diagnostic in `triggerAutoZoom()` referenced an out-of-scope variable. The exception propagated and skipped `backEndShootCount`, `updateSeriesScore` and `saveMatch`. **Every C++ harness passed** — none of them executes QML |
| **Authoritative fix** | The reference removed; and a QML harness created that *executes* the real functions with a message handler installed |
| **Source** | `CenterPane.qml`, `tests/qml/tst_qml_shot_path.cpp` |
| **Regression** | `tests/qml` |
| **Physical** | **PASS** — RC3B, RC3C |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES — critical.** This is the defect class that C++ tests cannot see. Any platform running this QML needs the QML harness too |

---

#### PAPER-FEED-002 — feed authority and numbering reset

| | |
|---|---|
| **First observed** | RC2e |
| **Symptom** | Feed requests could be issued from more than one place, and shot-numbering resets were not always notified |
| **Root cause** | The feed hook was not positioned as the single authority after coordinate capture |
| **Authoritative fix** | One authority, `onPhysicalShotAccepted`, one call site, after capture; the numbering-reset notification lives inside `resetShootinCount()` so every reset path is covered |
| **Source** | `ModReader/forms/tachuswidget.cpp` |
| **Regression** | `tests/reliability` |
| **Physical** | **PASS** — RC3C: 43 accepted shots, 43 automatic feeds, exactly 1:1 |
| **Windows** | YES |
| **Android** | **YES** (marker present) |
| **SETA** | **YES** (marker present) |
| **Shared?** | SHARED C++ |
| **Must carry forward** | Already present — **verify it still holds after the other acquisition fixes are merged**, because it sits on the same path |

---

#### UI-STATUS-001 — false "Target Not Connected" while connected

| | |
|---|---|
| **First observed** | RC3B physical test, 2026-08-25 |
| **Symptom** | The status panel said the target was not connected while it was connected and accepting shots |
| **Root cause** | Readiness was derived from a stale field rather than published from the acquisition state |
| **Authoritative fix** | `targetReady()` derives from the sequencer state; `publishReadinessIfChanged()` publishes after every poll; the headline is a deterministic `switch` naming every engine state |
| **Source** | `ModReader/forms/tachuswidget.{h,cpp}`, `TargetStatusPanel.qml`, `ShootingPage.qml` |
| **Regression** | `tests/qml`, `tests/reliability` |
| **Physical** | **PASS** — RC3D |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED C++ + SHARED QML |
| **Must carry forward** | **YES** |

---

#### FINALS-TIMER-001 / FINALS-DISPLAY-TIMER-002 — legacy clock in a Final

| | |
|---|---|
| **First observed** | RC3B, 2026-08-25 |
| **Symptom** | A legacy qualification timer started during a Final (001); a frozen `35:00` remained on screen (002) |
| **Root cause** | `gameTimer.start()` and the legacy clock's visibility were not gated on who owns timing |
| **Authoritative fix** | `legacyClockIsOurs` gates every `gameTimer.start()` and every legacy-clock visibility enable. **There must be exactly one competition timing authority** |
| **Source** | `CenterPane.qml` |
| **Regression** | `tests/qml`; and for 3P, `tests/finals` now asserts the displayed clock **is** `remainingFormatted` |
| **Physical** | **PASS** — RC3D |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES** |

---

#### FINAL-TCH-TIME-001 — Final shot times not persisted

| | |
|---|---|
| **First observed** | RC3C evidence — all 29 rows of the Final `.tch` carried `-1` |
| **Symptom** | Per-shot times were absent from the persisted Final record |
| **Root cause** | The 10 m Final's accepted-shot router did not append the time the controller had measured |
| **Authoritative fix** | The router appends `rec.timeSec` and the acceptance timestamp, skipped during recovery replay so a resumed session does not double-append |
| **Source** | `ShootingPage.qml` |
| **Regression** | `tests/qml` |
| **Physical** | **PHYSICAL PENDING** — fixed after RC3C; not yet re-tested on a target |
| **Windows** | YES |
| **Android** | YES (marker present) |
| **SETA** | YES (marker present) |
| **Shared?** | SHARED QML |
| **Must carry forward** | Present — verify |

---

#### FINALS-TCH-SIGHTER-001 — 10 m Final `.tch` mixes sighters and official shots

| | |
|---|---|
| **First observed** | RC3C reconciliation, 2026-08-26 |
| **Symptom** | `Match_25082026-210107.tch` holds 29 rows — the Final's sighters and its official shots, with nothing marking which is which |
| **Root cause** | `changeSighterMode` never runs during a 10 m Final (the controller owns phase), so the swap that separates sighters from the match record never happens |
| **Authoritative fix** | **NOT FIXED — OPEN.** Recorded so it is not lost |
| **Source** | `ShootingPage.qml`, `appsettings.cpp` (`.tch` writer) |
| **Regression** | none yet |
| **Physical** | N/A — a persistence-format issue |
| **Windows** | **OPEN** |
| **Android** | **OPEN** (inherits) |
| **SETA** | **OPEN** (inherits) |
| **Shared?** | SHARED QML + SHARED persistence |
| **Must carry forward** | **YES — as an open item.** It must be handled by the 10 m Finals report builder (F6), not by feeding those rows to the qualification tabs |
| **Notes** | The 3P Final does **not** have this problem: its shot record carries explicit `isSighter`, `finalsPosition` and `finalsShotNumber`, so phase ownership is stored, not inferred from order |

---

#### UI-LAYOUT-001 — status/header collision

| | |
|---|---|
| **First observed** | RC3B, 2026-08-25 |
| **Symptom** | The target-connection panel overlapped the header content |
| **Root cause** | Not established — the compact panel's geometry at 1280×800 was never measured |
| **Authoritative fix** | **NOT FIXED — instrumented only.** `TargetStatusPanel` logs its geometry under developer mode |
| **Source** | `ShootingPage.qml` |
| **Regression** | none |
| **Physical** | **OPEN** — needs compact geometry captured at 1280×800 |
| **Windows** | **OPEN** |
| **Android** | **OPEN** — and higher risk: Android tablets vary in density and safe-area insets |
| **SETA** | **OPEN** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES — as an open item** |

---

### Finals discipline isolation

---

#### FINALS-3P-MIX-001 — 10 m Finals state leaked into the 3P Final

| | |
|---|---|
| **First observed** | DEMO, RC3D, 2026-08-26 |
| **Symptom** | A 50 m 3P Final rendered the 3P shell with the 10 m Final's right panel on top: "10m Air Rifle Final", `0 / 24 shots`, Series 1 / Series 2 / Singles, two clocks 33 s apart, two enabled shot routers |
| **Root cause** | `enterFinalsMode()` set `isFinalsMatch` and cleared `is3PMatch` but never cleared `isFinals10mMatch`. Every other mode-entry function cleared all three. A 10 m Final earlier in the same run left the flag true |
| **Authoritative fix** | The 3P entry releases `isFinals10mMatch`; the same omission in `enterQualificationMode()`'s recovery branch fixed with it. **Rule: a mode-entry function owns every discipline flag, not just its own** |
| **Source** | `ShootingPage.qml` |
| **Regression** | `tests/qml` — all seven entry points, plus the presentation matrix. Verified against the defect: reintroducing the line fails 2 checks |
| **Physical** | **PHYSICAL PENDING** — found and fixed in DEMO |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES** |

---

#### FINALS-3P-PANEL-001 — the 3P Final had no panel of its own

| | |
|---|---|
| **First observed** | 2026-08-26, after the isolation fix |
| **Symptom** | After isolation the 3P Final fell back to the qualification `RightPanel` — a 60-shot series structure (S1–S6) in a 35-shot Final |
| **Root cause** | No 3P equivalent of `Finals10mRightPanel` had ever been built |
| **Authoritative fix** | `Finals3PRightPanel.qml`, driven exclusively by `FINALS3P`: position, stage, authoritative clock, command, last **official** shot with its owning position, per-position subtotals, overall total, progress as *n* / 35 |
| **Source** | `Finals3PRightPanel.qml`, `ShootingPage.qml`, `qml.qrc` |
| **Regression** | `tests/finals` — 71 checks driving the full course through the real panel bound to a real controller |
| **Physical** | **PHYSICAL PENDING** |
| **Windows** | YES |
| **Android** | **NO** — file absent |
| **SETA** | **NO** — file absent |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES** |
| **Notes** | Two defects were found *inside this panel* by driving it, and both are portable traps: `finalsPosition` is an int **role** (0 K / 1 P / 2 S), not a display string; and `stageSubtotals()` is a `Q_INVOKABLE`, so a binding on it never re-evaluates. Any platform reimplementing this panel will hit both |

---

### Presentation

---

#### UI-THEME-001 — System / Light / Dark appearance

| | |
|---|---|
| **First observed** | Feature request, 2026-08-26 |
| **Symptom** | n/a — new capability |
| **Root cause** | n/a |
| **Authoritative fix** | Every semantic token in `DesignTokens.qml` resolves through one `isLight`; the preference persists per user and defaults to dark. Presentation only |
| **Source** | `src/ui/theme/DesignTokens.qml`, `Theme.qml`, `LoginPage.qml`, `Header.qml`, `appsettings.{h,cpp}` |
| **Regression** | `tests/qml`, including measured WCAG contrast on the light palette |
| **Physical** | N/A |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED QML + SHARED C++ (the persistence) |
| **Must carry forward** | **YES, with adaptation.** `QSettings` organisation scope maps to the Windows registry and to Android shared preferences — the API is the same, the store is not. Android must also decide whether "System" follows the OS dark setting, which Qt reports via `Application.styleHints.colorScheme` on both |

---

#### UI-THEME-LOGO-001 — white logo on the light header

| | |
|---|---|
| **First observed** | 2026-08-26, looking at the rendered light theme |
| **Symptom** | The header mark was white on a near-white strip — effectively invisible |
| **Root cause** | `Header.qml` used `theme.logoWhite` unconditionally |
| **Authoritative fix** | `theme.isLight ? theme.logoColor : theme.logoWhite` |
| **Source** | `Header.qml` |
| **Regression** | `tests/qml` asserts the binding — but **would not have caught the original**; that needed a person looking at a picture |
| **Physical** | N/A |
| **Windows** | YES |
| **Android** | **NO** |
| **SETA** | **NO** — and SETA has its own mark, so this needs a SETA-specific light/dark asset decision |
| **Shared?** | SHARED QML, **brand-specific assets** |
| **Must carry forward** | **YES** |

---

---

#### FINALS3P-FLOW-001 — ISSF 2026 22-minute state-flow (audited, no defect found)

| | |
|---|---|
| **First observed** | Reported from operator DEMO, 2026-08-27 |
| **Symptom (reported)** | After the 22-minute block expired, "another approximately 5-minute Standing sighting/preparation block, followed by additional states" |
| **Root cause** | **None found in the state flow.** The audit drove the exact scenario against the real controller: one continuous 22:00 clock already spans KneelingMatch, ProneSighting, ProneMatch and StandingSighting, and its STOP is followed by a 30 s transition and a 250 s (4:10) Standing Series 1 - not a sighting period. Exactly one preparation/sighting period exists in the whole Final. The 4:10 series window immediately after the STOP, with the position still STANDING, is the closest match to what was described |
| **Authoritative behaviour** | `docs/rules/50M-3P-FINAL-STATE-FLOW-ISSF-2026.md` - Rule 6.17.3, 35 shots, one 22:00 block, warnings at 17:00 and 21:30, STOP, 30 s, 250 s + 250 s + 5 x 50 s |
| **Fix** | **No competition logic changed.** Added the normative specification (which did not exist) and 43 regression checks that fail on any build inserting a second sighting period, restarting the clock on a position change, letting the shot count end the block, or adding compensation time |
| **Source** | `docs/rules/50M-3P-FINAL-STATE-FLOW-ISSF-2026.{md,json}`, `tests/finals/tst_finals3p.cpp` |
| **Test** | `runFlow001Regressions` - early completion, two late-transition cases, target-mode sequence, 35-shot invariant |
| **Physical** | **PHYSICAL PENDING** - DEMO/controller-driven only |
| **Windows** | AUDITED - no defect |
| **Android** | **NO** - the branch predates the whole 3P Final work |
| **SETA** | **NO** - same |
| **Shared?** | SHARED C++ (controller + config) |
| **Must carry forward** | **YES.** The state flow AND its regressions must travel together; a port that takes the controller without the tests can reintroduce the old format silently |
| **Notes** | Two source questions remain open: which print of Edition 2025 governs (this audit used First Print 12/2025; `Finals3PConfig.h` cites Second Print 07/2026 - the durations agree), and Rule 6.17.3's tie-breaking provisions, which were not supplied and are therefore not implemented or inferred |

### Rule authority

Windows, Android and SETA must use the SAME normative rules and state-flow
documents. A platform branch may not retain an older competition format.

Master index: [../rules/RULE-AUTHORITY-INDEX.md](../rules/RULE-AUTHORITY-INDEX.md)

| Discipline | Authority | Windows | Android | SETA |
|---|---|---|---|---|
| 50 m 3P Final | ISSF 6.17.3, Second Print 07/2026 (verified 2026-08-27) | **PASS** | **NO** | **NO** |
| 10 m AR/AP Final | ISSF 6.17.2, same print (verified) | **PASS** | PARTIAL | PARTIAL |
| 50 m 3P Qualification | ISSF events programme + 6.11.1.1 | **PARTIAL** | unknown | unknown |
| DSB 1.20 / 1.40 / 1.60 | DSB Sportordnung | not on this branch | — | **implemented on `product/seta`, NOT audited** |

Nothing was ported in this round.

---

#### UI-LASTSHOT-DWELL-001 — the last shot of a position clears too fast

| | |
|---|---|
| **First observed** | Live 50 m 3P Qualification, three tablets, 2026-08-29 |
| **Symptom** | At the last official shot of a position (qualification shots 20 and 40) the target face clears before the athlete or operator can inspect the shot |
| **Measured** | **1.63-2.03 s**, mean 1.84 s, from `qml-marker-added` to `3P: position change` - consistent across all three tablets and both boundaries |
| **Root cause** | `ShootingPage.qml::enterPositionTransition()` clears `globalModelOfData` and repopulates it with only the new position's sighters. Triggered by the `positionWatch` 500 ms poll at official shot 20/40. The ~1.8 s is paper-feed duration plus one poll interval - **incidental, not a designed dwell**. Nothing currently holds the last shot on purpose |
| **Competition impact** | **NONE.** State, target mode, clock, persistence and paper feed are all correct and complete before the face clears. Presentation only |
| **Fix** | **NOT FIXED - RC3F is frozen.** Recommended: a 2.5 s presentation-only hold of the last marker and score with a POSITION COMPLETE caption. Must hold a snapshot, never the live model; a shot arriving during the hold is routed by the NEW state; the CRO command area must update immediately |
| **Source** | `ShootingPage.qml`, `CenterPane.qml` |
| **Test** | none yet |
| **Physical** | **OBSERVED on 3 tablets** |
| **Windows** | **OPEN** |
| **Android** | **NO** - branch predates the 3P work |
| **SETA** | **NO** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES** |
| **Notes** | Qualification shot 60 and the Final's position boundaries are NOT affected: completion does not clear the face, and the Final waits for the athlete to advance |

---

#### CRO-ORDER-001 / CRO-REPEAT-002 — qualification announcement order and repetition

| | |
|---|---|
| **First observed** | Live 50 m 3P Qualification, all three tablets, 2026-08-29 |
| **Symptom** | (001) `END OF PREPARATION AND SIGHTING...STOP` is announced ~138 ms **after** `MATCH FIRING...START`. (002) `MATCH FIRING...START` is announced again at each position change - three times per session |
| **Rule** | 6.11.1.1 j) then 6.11.1.2 a): STOP, a ~30 s pause for the target reset, **then** MATCH FIRING. One MATCH FIRING per match |
| **Root cause** | The sighting timer's expiry path calls the transition - which announces MATCH FIRING - before the line that announces the end of preparation; and `changedToMatchMode()` announces MATCH FIRING whenever the athlete resumes after a position change |
| **Competition impact** | **NONE** - announcements only. No state, clock, shot or target-mode effect |
| **Fix** | **NOT FIXED - RC3F is frozen.** Batch with UI-LASTSHOT-DWELL-001 |
| **Source** | `CenterPane.qml` |
| **Physical** | **OBSERVED on 3 tablets** |
| **Windows** | **OPEN** |
| **Android** | **NO** |
| **SETA** | **NO** |
| **Shared?** | SHARED QML |
| **Must carry forward** | **YES** |
| **Notes** | Introduced by RC3F's own CRO announcement work. The 10- and 5-minute warnings were **NOT EXERCISED** - all three athletes finished before the 10-minute mark (T3 by 44 s) |

## Portability validation matrix

`PASS` = code verified present. Physical status is the separate column in each
entry above and is **never** implied by this table.

| Fix / feature | Windows | Android | SETA |
|---|---|---|---|
| ACQ-FLUSH-001 | PASS + **PHYSICAL PASS (RC3F, 3 tablets, 385 shots)** | **NO — must carry** | **NO — must carry** |
| ACQ-DESYNC-002 | PASS | **NO — must carry** | **NO — must carry** |
| ACQ-SENTINEL-003 | PASS + **PHYSICAL PASS (RC3F, 385/385 distinct)** | **NO — must carry** | **NO — must carry** |
| ACQ-READ-004 | PASS | **NO — must carry** | **NO — must carry** |
| SERIAL-DEFAULT-005 | PASS + **PHYSICAL PASS (RC3F, 3 COM ports)** | **NO — needs USB equivalent** | **NO — must carry** |
| THREAD-MODBUS-006 | PASS | **NO — must carry** | **NO — must carry** |
| LOG-DEFECT-007 | PASS | **NO — must carry** | **NO — must carry** |
| QML-SHOT-001 | PASS | **NO — must carry** | **NO — must carry** |
| PAPER-FEED-002 | PASS + **PHYSICAL PASS (RC3F, 385 feeds 1:1)** | PASS | PASS |
| UI-STATUS-001 | PASS | **NO — must carry** | **NO — must carry** |
| FINALS-TIMER-001 | PASS | **NO — must carry** | **NO — must carry** |
| FINALS-DISPLAY-TIMER-002 | PASS | **NO — must carry** | **NO — must carry** |
| FINAL-TCH-TIME-001 | PASS | PASS | PASS |
| FINALS-TCH-SIGHTER-001 | **OPEN** | **OPEN** | **OPEN** |
| UI-LAYOUT-001 | **OPEN** | **OPEN** | **OPEN** |
| FINALS-3P-MIX-001 | PASS | **NO — must carry** | **NO — must carry** |
| FINALS-3P-PANEL-001 | PASS | **NO — must carry** | **NO — must carry** |
| UI-THEME-001 | PASS | **NO — must carry** | **NO — must carry** |
| UI-THEME-LOGO-001 | PASS | **NO — must carry** | **NO — must carry** |
| FINALS3P-FLOW-001 | AUDITED — no defect | **NO — must carry** | **NO — must carry** |
| UI-LASTSHOT-DWELL-001 | **OPEN** — observed on 3 tablets | **NO — must carry** | **NO — must carry** |
| CRO-ORDER-001 / CRO-REPEAT-002 | **OPEN** — observed on 3 tablets | **NO — must carry** | **NO — must carry** |
| 3P Finals UI | PASS (DEMO) | **NO** | **NO** |
| 10 m Finals | PASS | PARTIAL | PARTIAL |
| Reports | PASS (3P) · **10 m MISSING (F6)** | inherits | inherits |
| Reconnect | PASS + PHYSICAL PASS | **NO** | **NO** |
| Paper feed | PASS + PHYSICAL PASS | PASS | PASS |
| Theme / Settings | PASS | **NO** | **NO** |

**Android has never been physically tested against a target as part of this
programme. SETA has never been physically tested as part of this programme.
Neither may be recorded as physically passed on the strength of Windows
evidence.**

---

## Android carryover

### Categories

| Category | Meaning | Fixes |
|---|---|---|
| **SHARED C++** | applies automatically if the same code is compiled | ACQ-FLUSH-001, ACQ-DESYNC-002, ACQ-READ-004, THREAD-MODBUS-006, LOG-DEFECT-007 (logic), ACQ-SENTINEL-003 (C++ half) |
| **SHARED QML** | should apply, **must be visually validated on Android** | QML-SHOT-001, UI-STATUS-001, FINALS-TIMER-001/002, FINALS-3P-MIX-001, FINALS-3P-PANEL-001, UI-THEME-001, UI-THEME-LOGO-001, ACQ-SENTINEL-003 (QML half) |
| **WINDOWS-SERIAL-SPECIFIC** | needs an Android USB equivalent | SERIAL-DEFAULT-005, and the auto-connect/scan policy |
| **WINDOWS-FILESYSTEM-SPECIFIC** | needs an Android storage equivalent | log path (`%TEMP%`), `.tch` location, journal root, PDF export path, support-bundle collection |
| **ANDROID LIFECYCLE-SPECIFIC** | no Windows counterpart | background/foreground, process death, orientation, session recovery |

### Needs separate Android validation

- **USB permissions** — Android requires explicit user grant per device; there
  is no Windows equivalent. A denied or revoked permission must present as a
  clear target-status state, not as a silent failure.
- **USB reconnect** — cable events arrive as Android intents, not as a COM
  enumeration change. `ACQ-DESYNC-002`'s reconciliation is shared, but what
  *triggers* a reconnect is not.
- **Foreground / background** — a backgrounded app must not lose acquisition
  state or silently stop polling. The monotonic finals clock and
  `TECHAIM_FINALS_TIMESCALE` behaviour under process suspension are untested.
- **Android lifecycle** — process death mid-Final. Session recovery exists
  (`M3`), but has never been exercised against an Android kill.
- **Storage paths** — `StoragePaths` resolves the journal root; Android
  scoped storage changes what is writable and what survives uninstall.
- **PDF / export paths** — sharing a report off an Android tablet is a
  different flow from writing a file.
- **Screen scaling** — Qt density handling differs; the token layer is
  density-independent but pixel sizes in QML are not.
- **1280×800 tablet layouts** — the operator's resolution. Windows evidence at
  this size is itself incomplete (see UI-LAYOUT-001).
- **Theme persistence** — `QSettings` maps to shared preferences; verify the
  default is dark for existing users and that "System" follows the Android
  dark setting.
- **Settings reachability** — the start-page Settings entry must be reachable
  and hit-target-sized on a touch screen.
- **Orientation** — the shooting page assumes landscape. Rotation behaviour
  during a Final is undefined and should probably be locked.
- **Session recovery** — combined with lifecycle above: the highest-risk
  Android-specific area, because it interacts with acquisition state.

---

## SETA build carryover

The SETA build must share the same correctness logic. **Only branding and
configuration should differ.**

| Fix | Automatic via shared code | Must verify in the SETA build |
|---|---|---|
| ACQ-FLUSH-001 … ACQ-READ-004 | yes, once merged | acquisition behaviour end to end |
| SERIAL-DEFAULT-005 | yes | the serial profile **and** that a SETA `config.ini` does not override it back to a generic default |
| THREAD-MODBUS-006 | yes | — |
| LOG-DEFECT-007 | yes | log path and support-bundle collection under SETA branding |
| PAPER-FEED-002 | yes | one feed per accepted shot |
| Shot-10 transitions | yes | a 10/20/30 boundary crossing |
| Coordinate validity | yes | — |
| Finals mode isolation | yes | 10 m Final → Home → 3P Final in one run |
| 3P Finals UI/controller | yes, once merged | the panel renders under SETA branding |
| 10 m Finals controller | yes | — |
| Reports | yes | branded header/footer, and the 10 m Finals report gap (F6) |
| Theme / Settings | yes | SETA-specific logo assets for light chrome (UI-THEME-LOGO-001) |
| Software / build identity | **no — brand-specific** | version string, `APP_GIT_SHA`, product identity, icon |

**An OEM or brand package must not fork correctness logic.** If SETA needs
different behaviour in a correctness path, that is a shared-core change with a
configuration switch — not a divergent copy.

---

## Source-of-truth principle

**Tech Aim Windows, Tech Aim Android and SETA OEM must not independently
reimplement known correctness fixes.**

The source of truth is:

- **shared C++** — `src/target`, `src/finals`, `src/finals10m`,
  `src/training`, `src/reliability`, `src/analytics`, `ModReader`
- **shared QML** — the shooting page, the finals panels and HUDs, the token
  layer, the dialog framework
- **shared session and report models** — the journal, the `.tch` record, the
  report builders

Differences belong in:

- **BrandPackage** — names, marks, colours, product identity
- **platform adapters** — serial vs USB host, storage paths, lifecycle
- **configuration** — `config.ini`, build flavours

A fix that lands anywhere other than the shared core is a fix the other
platforms will lose. That is the failure this register exists to prevent, and
the matrix above shows it has already happened once, at scale.

---

*Machine-readable companion: `cross-platform-fix-register.json`. The Markdown
document above is the human-readable authority; the JSON is a convenience for
tooling and must be regenerated, not diverged.*
