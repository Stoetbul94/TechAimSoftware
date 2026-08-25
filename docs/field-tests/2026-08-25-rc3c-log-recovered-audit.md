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
| Accepted physical shots | **43** — 2 sighters + 12 counted (Open Training), 29 (10 m Final). Fully reconciled against both `.tch` records below |
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

## SHOT COUNT RECONCILED — 43 accepted = 41 persisted + 2 explained sighters

The log records **43** accepted physical shots; the two `.tch` records hold
**41**. Every one of the 43 was matched by coordinate against both records.

```
tch1 (Match_25082026-205444.tch)   12
tch2 (Match_25082026-210107.tch)   29
NEITHER                             2
                                   --
                                   43
```

The two unmatched events are **not** the first rows of either record.
`Match_25082026-205444.tch` `data_0` is x=2.1 y=-0.6 at 20:55:24, which is
accepted event **03**. Events 01 and 02 precede it:

| # | Time | Coordinate | Seq | Score | In tch1 | In tch2 |
|---|---|---|---|---|---|---|
| **01** | **20:54:52.976** | **x=5.6 y=14.8** | **1 (sighter)** | 4.67 | no | no |
| **02** | **20:55:08.291** | **x=1 y=-6.1** | **2 (sighter)** | 8.53 | no | no |
| 03 | 20:55:24.739 | x=2.1 y=-0.6 | 1 (counted) | 10.13 | **yes** — `data_0` | no |

Events 04–14 map one-to-one onto `tch1` `data_1`…`data_11`; events 15–43 map
one-to-one onto `tch2` `data_0`…`data_28`. No duplicates, no gaps, no
reordering.

### What the two events are

**Sighters fired inside the official preparation-and-sighting phase of the
session that became `Match_25082026-205444.tch`.** The log shows the whole
sequence:

```
20:54:44.497  QUAL: AR10 session journalling started (session_20260825T185444_41...)
20:54:44.470  beginPreparationPhase: prep seconds = 900
20:54:44.477  startPreparationCountdown: totalSighterTime=900
20:54:52.976  physical shot accepted: seq 1 (sighter)      <- event 01
20:55:08.291  physical shot accepted: seq 2 (sighter)      <- event 02
20:55:14.271  removeSetaLaneShootDataFile
20:55:14.342  paper feed: shot numbering reset - 2 remembered identities cleared
20:55:14.352  changeSighterMode: lists reset, leaving sighter mode
20:55:24.739  physical shot accepted: seq 1 (counted)      <- event 03, tch1 data_0
```

The journal name `session_20260825T185444` is 18:54:44 **UTC** = 20:54:44
local, the same instant as the `.tch` name `205444`. Both sighters are inside
that session, 8 and 24 seconds after it started.

Against the categories asked for:

| Candidate explanation | Verdict |
|---|---|
| Before a session officially started | **No** — the session started at 20:54:44.497, both sighters follow it |
| After a session ended | **No** |
| Sighters belonging to another session | **No** — same session; only two sessions were journalled all evening |
| Another short or unsaved session | **No** — no third session start appears anywhere in the log |
| Fired while navigating or testing | **No** — fired inside a running 900-second sighting countdown |
| **Accepted but not persisted** | **Yes — and by design.** See below |

### Why they are not in the `.tch`, from source

`changeSighterMode(false)` does not discard the sighters — it **swaps** the
lists (`tachuswidget.cpp:1683`):

```cpp
temp = m_xCordList;                    // the 2 sighter coordinates
m_xCordList = m_xCordList_gameMode;    // the empty match list becomes live
m_xCordList_sighterMode = temp;        // the 2 sighters are parked, not destroyed
```

The `.tch` writer reads the **live** list — `appsettings.cpp:239` calls
`getXCord(i+1)`, which returns `m_xCordList.at(index-1)`. After the swap the
live list is the match list, which then accumulated exactly the 12 counted
shots. Hence 12 rows, not 14.

**This is correct behaviour.** A match record must not contain sighters. The
log message "lists reset" is loose wording for a swap plus a counter reset;
nothing was lost.

### Paper feeds and other storage, for those two events

Both received a full automatic feed, requested, started and completed:

```
event 01   20:54:52.977 requested -> 20:54:52.977 started -> 20:54:54.324 completed
event 02   20:55:08.292 requested -> 20:55:08.292 started -> 20:55:09.640 completed
```

Correct — a sighter is a real shot on paper and must advance it. These two are
part of the 43 automatic feeds, so the 1:1 feed accounting is unchanged.

Where their coordinates went:

| Store | Held? |
|---|---|
| `m_xCordList_sighterMode` / `m_yCordList_sighterMode` | yes, until process exit — swapped in, not cleared |
| SETA lane shoot-data file | written, then removed by `removeSetaLaneShootDataFile()` at 20:55:14.271 |
| `Match_25082026-205444.tch` | no — by design |
| Session journal `session_20260825T185444_41….jsonl` | **cannot be determined** — no `.jsonl` was collected into the RC3C evidence |
| The tachus log itself | yes — both coordinates, scores and feeds are fully recorded |

The journal is the one store that could still hold them and it was not
collected, because `Collect-Logs.cmd` was not run. It may still exist on the
tablet under `AppData\Local\TechAim\TechAim\Sessions`. This does not affect the
reconciliation — the two events are accounted for either way — but it is the
same evidence gap already recorded against RC3C.

### The asymmetry, and one item for the deferred reporting round

`changeSighterMode` was called **once** in the whole session, at 20:55:14. It
was never called during the Final, because `Finals10mController` owns finals
phase — so no swap ever happened there, the live list accumulated all 29
events, and all 29 were written to `Match_25082026-210107.tch`.

That means **`Match_25082026-210107.tch` contains the Final's sighters mixed in
with its official shots**, with nothing in the file marking which is which. The
exact split cannot be determined from the collected evidence; the journal would
resolve it.

This is not a scoring defect and nothing acquired it wrongly. It matters for
the deferred 10 m Finals reporting round, because `ReportWindow.finalsMode`
currently tests only `isFinalsMatch` (the 3P flag), so a 10 m Final opens the
**qualification** Summary/Match tabs — fed from these 29 rows, which would
present sighters as match shots. Recorded here as input to that round, not
fixed here.

| ID | Observation | Status |
|---|---|---|
| **FINALS-TCH-SIGHTER-001** | The 10 m Final `.tch` holds sighters and official shots undifferentiated, because `changeSighterMode` never runs in a Final. Harmless today; must be handled by the 10 m Finals report builder (F6) rather than by feeding the qualification tabs. | **OPEN — for the reporting round** |

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
| Shot count: 43 accepted vs 41 persisted, unexplained | **RECONCILED** — the 2 are training sighters, excluded from the match record by design |
| RC3C: *"CLEAN WITH MINOR NON-BLOCKING ITEMS"* | **CLEAN** — one cosmetic log-wording item |

Two independent physical sessions now show the same result: RC3B (38 shots,
4 reconnects) and RC3C (43 shots, 5 reconnects, 3 flush boundaries), with zero
acquisition faults and zero corruption signatures between them.

This does not change the cross-discipline physical status. RC3C exercised
10 m Air Rifle Open Training and the 10 m Final; every other row of the
propagation matrix remains **PHYSICAL PENDING**.

**No code was changed in this audit. The evidence file was read, not moved,
renamed or modified.**
