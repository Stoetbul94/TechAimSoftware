# RC3C — recovered tachus log, read-only audit

**Result: the RC3C session was clean. Every question the original RC3C audit
had to leave open is now closed, including the motor question.**

The RC3C qualification was reported as *"CLEAN WITH MINOR NON-BLOCKING ITEMS"*
rather than proven, for one reason: the tachus log was not collected, so no
acquisition diagnostic could be checked. The operator later found the file and
supplied it.

Evidence (read-only): `D:\tachus_log25082026-204511.log` — 4 449 lines,
20:45:11 → 21:25:06 local, tablet user `Tech Aim 1`, target on COM4
(USB-SERIAL CH340). The window covers the whole session: Open Training
20:54–21:00, 10 m Final 21:01–21:18, save and close to 21:25.

This log is from **RC3C**, so it does **not** contain the UI-LAYOUT-001
instrumentation, which exists only in RC3D. UI-LAYOUT-001 remains open.

## The acquisition baseline

| | |
|---|---|
| Accepted physical shots | **43** — 2 sighters + 12 counted (Open Training), 29 (10 m Final) |
| Distinct coordinate pairs | **43 of 43** |
| Repeated coordinates | **0** |
| `-1 / -1` sentinel pair | **0** |
| `nan` coordinates | **0** |
| Automatic paper feeds | **43 requested, 43 started, 43 completed** — one per shot |
| `ACQUISITION_FAULT` | **0** |
| `ACQ_COORD_READ_FAILED` | **0** |
| `ACQ_COORD_REFUSED_BY_UI` | **0** |
| `COUNTER JUMPED` | **0** |
| `COMMUNICATION INTERRUPTION` | **0** |
| `AdoptionWouldDesync` | **0** |
| `ResetNotConfirmed` | **0** |
| Disconnect / reconnect cycles | **5**, all reconciled |
| `ACQ_COORD_INDEX_INVALID` | 2, both the benign pre-shot report probe |

The two `ACQ_COORD_INDEX_INVALID` lines are at 20:45:16.549, from
`getXMPIForShoot` and `getYMPIForShoot` with `requestedIndex=1 xList=0 yList=0`
— nine minutes before the first shot, match-report cells binding on an empty
session. This is exactly the documented benign exception in
`rc3b-diag-range-start-here.md`, and identical to RC3B. Zero from the scoring
accessors; zero of any kind after the first shot.

## MOTOR-FEED-001 — CLOSED. The operator's recollection was correct.

The open question was: *"a possible extra motor movement around shot 8–10."*
It was recorded as **PHYSICAL OBSERVATION — UNVERIFIED** because nothing in the
kept evidence could settle it. The log settles it.

```
physical shot accepted     43
paper feed requested       43      \
paper feed started         43       >  1 : 1 with accepted shots
paper feed completed       43      /
Send motor movement signal 48      <-- five more
Send motor stop signal     48
```

The five extra movements are **manual feeds**, every one logged with a matching
completion:

```
20:59:07.085  manual feed requested: duration 1.00 s     20:59:08.426  completed
20:59:10.365  manual feed requested: duration 1.00 s     20:59:11.704  completed
20:59:13.074  manual feed requested: duration 1.00 s     20:59:14.414  completed
20:59:15.848  manual feed requested: duration 1.00 s     20:59:17.188  completed
21:00:38.753  manual feed requested: duration 1.00 s     21:00:40.094  completed
```

43 automatic + 5 manual = **48**. The arithmetic closes exactly; there is no
unexplained motor command anywhere in the session.

Two further details confirm the recollection rather than merely permitting it:

- **The timing is right.** Counted shot 10 was accepted at 20:58:59.370 and the
  10-shot flush ran at 20:59:00.796. The first manual feed is at 20:59:07 —
  **six seconds after the shot-10 boundary** — then three more at roughly
  three-second intervals. A fifth follows shot 12 (21:00:32) at 21:00:38.
- **It was not in the Final.** All five are at 20:59 and 21:00, inside Open
  Training. The Final began at 21:01:07. During the Final there were **zero**
  manual feeds and **zero** extra motor commands — 29 shots, 29 feeds.

The operator has since confirmed pressing the feed-paper button "a few times",
which is what the log independently shows.

**Conclusion: correct behaviour, operator-initiated, fully logged. No motor or
paper-feed code change is warranted, and none was made.**

## The 10-shot boundary — three crossings, all correct

`delta=-10` is the exact condition that produced `ACQUISITION_FAULT` twelve
times out of twelve on RC2g. It occurred three times here and faulted zero
times. The shot-20 crossing in the Final, in full:

```
21:14:11.757  rawCounter=10  baseline=9   delta=1     ACQUIRING          captured=19  priorTotal=10
21:14:13.283  rawCounter=0   baseline=10  delta=-10   RESETTING_COUNTER  captured=20  priorTotal=10
21:14:13.398  rawCounter=0   baseline=0   delta=0     ACQUIRING          captured=20  priorTotal=20
```

115 ms from application-requested reset to target-confirmed baseline move. The
2.6-second race is gone. The other two crossings (20:59:00 in training,
21:09:47 in the Final) show the same three-line shape and the same ~100 ms
confirmation.

### The invariant held on every poll

`priorTotal + baseline == capturedShots` was checked mechanically across every
diagnostic row in the file:

```
rows checked: 1403   violations: 0
```

## Reconnects — five, every one reconciled

| Time | Event | Reconciliation |
|---|---|---|
| 20:45:17 → 20:53:04 | LOST at launch, 225 attempts | baseline 0, target 0, coords 0 |
| 20:57:04 → 20:57:12 | LOST after 3 failed reads, 5 attempts | baseline 8, target **8**, coords 8 |
| 21:05:57 → 21:06:03 | LOST after 3 failed reads, 4 attempts | baseline 2, target **2**, coords 2 |
| 21:06:08 → 21:06:31 | LOST after 3 failed reads, 12 attempts | baseline 2, target **2**, coords 2 |
| 21:20:34 → 21:20:40 | LOST after 3 failed reads, 4 attempts | baseline 9, target **9**, coords 29 |

`baseline == target reports` in every case — no reconnect adopted a mismatched
counter, which is the Tablet-02 failure. The last of these is the interesting
one: `baseline 9 + priorTotal 20 == 29 coordinates held`, the invariant holding
across a reconnect two 10-shot flushes into a Final.

A sixth `LOST` at 21:20:43 has no matching `RESTORED`. The last shot was
accepted at 21:18:02 and the session was saved and closed at 21:25, so
**nothing was lost** — this is the target being disconnected at the end of the
session. Acquisition was correctly suspended for the remaining 4½ minutes.

## SERIAL-DEFAULT-005 held

Every connect line reads `COM4 19200 Even 8 1 Disable 1`. Never 9600, never
`None`.

Five `auto-connect` lines — exactly one per reconnect, none while connected.
The Tablet-01 signature of 2026-08-23 (29 scans a minute for 74 minutes) is
absent.

## Reading note — the `(sighter)` tag during a Final

All 29 Final shots are logged `(sighter)`. This is not a defect and does not
affect scoring: `Finals10mController` owns finals phase and shot bookkeeping,
so the legacy `MODREADER` sighter/counted flag is never switched during a
Final and stays where the session left it. The tag in a tachus log therefore
describes the **legacy acquisition mode**, not the finals phase.

This is the same class of reading trap as TRAINING-LIMIT-001: a log total
counts every physical acceptance, regardless of what the owning controller
calls it. Worth knowing when reading a log; not a defect.

## Minor, non-blocking

| ID | Observation | Status |
|---|---|---|
| **LOG-WORDING-001** | The launch-time disconnect logs `target link LOST after 0 consecutive failed reads`, which reads as a contradiction. It is the initial state before the port is open, not a read failure. Cosmetic wording only. | OPEN — cosmetic |

## What this changes

| Previously | Now |
|---|---|
| RC3C: *"acquisition diagnostics were not checked at all"* | Checked in full — clean |
| Motor question: **PHYSICAL OBSERVATION — UNVERIFIED** | **CLOSED** — 43 automatic + 5 manual, operator-initiated |
| RC3C: *"CLEAN WITH MINOR NON-BLOCKING ITEMS"* | **CLEAN** — one cosmetic log-wording item |

Two independent physical sessions now show the same result: RC3B (38 shots,
4 reconnects) and RC3C (43 shots, 5 reconnects, 3 flush boundaries), with zero
acquisition faults and zero corruption signatures between them.

This does not change the cross-discipline physical status. RC3C exercised
10 m Air Rifle Open Training and the 10 m Final; every other row of the
propagation matrix remains **PHYSICAL PENDING**.

**No code was changed in this audit. The evidence file was read, not moved,
renamed or modified.**
