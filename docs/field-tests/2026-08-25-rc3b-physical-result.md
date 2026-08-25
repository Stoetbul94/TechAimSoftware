# RC3B-DIAG — first physical test, 2026-08-25

**Result: the acquisition engine passed. Three UI/state defects were found, two
of them fixed in the narrow hardening round that followed.**

Evidence (read-only): `C:\Users\User\Downloads\rc3b-test` —
`tachus_log25082026-132657.log` (4 322 lines, 13:26:57 → 14:05:41) and one
screenshot at 14:01:34. Tablet user `Tech Aim 1`, target on COM4.

## The acquisition baseline

| | |
|---|---|
| Accepted physical shots | **38** — 3 sighters, 23 counted (10 m air rifle), 12 (Call & Diagnose) |
| Distinct coordinates | **38 of 38** |
| Paper feeds | **38 started, 38 completed** — one per shot |
| Disconnect / reconnect cycles | **4**, all reconciled |
| `ACQUISITION_FAULT` | **0** |
| `ACQ_COORD_READ_FAILED` | **0** |
| `ACQ_COORD_REFUSED_BY_UI` | **0** |
| `COUNTER JUMPED` | **0** |
| `COMMUNICATION INTERRUPTION` | **0** |
| repeated coordinates | **0** |
| `-1 / -1` sentinel pair | **0** |
| `ACQ_COORD_INDEX_INVALID` | 2, both benign — see the acceptance note below |

### ACQ-FLUSH-001 did not recur, and the log shows why

The 10-shot boundary, caught in full at 13:52:45–13:52:47:

```
13:52:45.733  rawCounter=10 baseline=9  delta=1    ACQUIRING          captured=19 priorTotal=10
13:52:47.288  rawCounter=0  baseline=10 delta=-10  RESETTING_COUNTER  captured=20 priorTotal=10
13:52:47.388  rawCounter=0  baseline=0  delta=0    ACQUIRING          captured=20 priorTotal=20
```

`delta=-10` is the exact condition that produced `ACQUISITION_FAULT` twelve
times out of twelve on RC2g. The poll judged nothing because it knew the
application had asked for the reset, and **100 ms later** the target confirmed
zero and the baseline moved. The 2.6-second race is gone.

### ACQ-DESYNC-002 did not recur, under the exact Tablet-02 conditions

```
13:52:45  shot 20 accepted                                  (20 coordinates held)
13:52:58  target link LOST after 3 consecutive failed reads  (operator unplugged)
13:53:10  target link RESTORED on COM4 after 7 attempt(s);
          baseline was 0, target reports 0, coordinates held 20
13:53:10  SYNCHRONIZED baseline=0 captured=20 priorTotal=20
13:53:58  shot 21   x=1.0  y=2.5                             (real coordinates)
```

That is the state that produced fourteen false 10.8s on Tablet-02.

### SERIAL-DEFAULT-005 held

Scans ran only while genuinely disconnected. Between 13:43–13:52 and
13:58–14:05, connected and shooting: **zero scans**. Tablet-01 on 2026-08-23
ran 29 scans a minute for 74 minutes.

## SHOTCOUNT-001 — 23 counted shots, not ~21. Not a defect.

The operator recalled about 21; the log has 23. The sequencer's own counters
settle it: `captured` advances 16, 17, 18, 19, 20 — strictly one per shot — and
the identity `priorTotal + baseline == captured` holds at every step
(10+9=19, 10+10=20, 20+0=20). The reconnect added nothing; no shot was accepted
twice; all 23 coordinates are distinct and each had exactly one paper feed.

**Conclusion: the athlete fired 23. Recall, not software.**

## TRAINING-LIMIT-001 — 12 shots against "11 called shots". Not a defect.

`CallDiagnoseController::registerShot` routes strictly by phase:

- phase 1 **Sighters** → `acceptSighter`
- phase 2 **AwaitingShot** → `acceptActualShot`
- phase 3/4/5 (AwaitingCall / Reveal / **Complete**) →
  `shotRejected("ResolveCurrentShotFirst")`, refused

A Call & Diagnose programme **begins in the sighter phase**. Sighter shots are
physically real — they reach acquisition, score, feed paper and appear in the
tachus log — but they are not *called* shots. One sighter plus eleven called
shots is twelve physical acceptances, which is exactly what the log holds.

The boundary is enforced: on the last call the phase moves to 5 (Complete) and
every further shot is refused by the controller.

**No change required.** The label "11 called shots" counts called shots and is
accurate. What differs is the tachus log, which counts every physical
acceptance regardless of programme phase — so a log total will legitimately
exceed a programme total by the number of sighters. Worth knowing when reading
a log; not a defect in either.

*Not confirmable from this evidence set: the exact sighter/called split. The
session journal would show it and was not copied into the evidence folder.
Collect `TechAim\Sessions` next time — `Collect-Logs.cmd -SessionId <id>` does
it.*

## Acceptance-criteria correction

The original criterion — "zero `ACQ_COORD_INDEX_INVALID`" — was unachievable.
Two are emitted a few seconds after every launch, from `getXMPIForShoot` and
`getYMPIForShoot` with `requestedIndex=1 xList=0 yList=0`: match-report cells
binding on an empty session. The guard behaved correctly — invalid value, dash
in the cell, and deliberately **no** acquisition fault, because nothing is being
acquired when a report view lays itself out.

`rc3b-diag-range-start-here.md` now distinguishes a benign pre-shot report probe
from an invalid coordinate during active acquisition, and the integrity
criteria themselves are unchanged: zero from the scoring accessors, zero of any
kind after the first shot, zero failed reads, zero rejected genuine shots.

## Defects found

| ID | Severity | Status |
|---|---|---|
| **UI-STATUS-001** | display integrity | **FIXED** — see the hardening round |
| **FINALS-TIMER-001** | state integrity | **FIXED** |
| **UI-LAYOUT-001** | display | **OPEN — not reproduced**, needs the shooting page driven |
| SHOTTRACE session tag | evidence quality | **DEFERRED**, documented |

## Physical status

One clean session on one tablet, in two disciplines. That is the first physical
evidence any of the seven acquisition fixes holds — it is not qualification
across all disciplines, and every row of the cross-discipline matrix remains
**PHYSICAL PENDING**.
