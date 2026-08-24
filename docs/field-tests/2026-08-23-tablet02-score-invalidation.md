# Tablet-02, 2026-08-23 — fourteen persisted scores declared INVALID

**Status: these fourteen records are not shooting results and must never be
used as any.** The trigger events behind them were genuine. The coordinates and
scores stored against them were produced by a software defect and describe
nothing that happened on the target.

The original journals are **preserved byte-for-byte** as forensic evidence and
must not be edited, renamed, migrated or normalised. This document is the
invalidation; the data stays exactly as it was written.

- Evidence (read-only): `C:\TechAim\RC3A-Physical-Test-Evidence\Tablet-02\TechAim\Sessions\`
- Athlete as recorded: `Bernard` / `bernard`
- Build that produced them: `0.9.0-RC2g-DIAG` (not RC3a, despite the evidence
  folder's name — see *Provenance* below)
- Defect: **ACQ-DESYNC-002**, with **ACQ-SENTINEL-003** as the reason it was
  silent

---

## What is genuine and what is not

| | |
|---|---|
| The athlete fired a real shot | **GENUINE** — the target counter advanced by exactly one for each of these records |
| The shot was detected | **GENUINE** — each record is a real detection event, correctly counted |
| The coordinates stored | **INVALID** — `x = -100`, `y = -100` hundredth-mm on every one of them |
| The score stored | **INVALID** — `scoreTenths = 108` (10.8) on every one of them |
| The direction stored | **INVALID** — `22500` centi-degrees, the diagonal of (-1, -1) |

The software did **not** fabricate extra trigger events, and it did not lose
count. It attached a fabricated coordinate to a real shot.

## Why the value is always 10.8

After the reconnect, the application asked for a coordinate index one past the
end of the arrays it held. `getXCord()`/`getYCord()` answered `-1` for an index
they did not have. −1.00 mm on both axes is 1.414 mm from centre, which on a
50 m rifle target scores **10.8**. An internal indexing error therefore became
a plausible competition score with no error reported anywhere.

The backend had decoded the real coordinates correctly. The log proves it:

```
tachus_log23082026-150736.log
  15:19:23.305  SHOTTRACE nosession/12 decoded x=-3.9 y=1     <- real measurement
  15:19:23.313  live match new formula xpoint -1 yPoint -1    <- what scoring received
  15:19:23.316  50 m game for rifle -> calculate score 10.823233304703363
```

The scoring mathematics was correct throughout. Only its input was wrong.

## The affected records

**14 records across 3 sessions.** Session-relative sequence numbers are given so
each can be located without altering the file.

### `session_20260823T124831_4449b052.jsonl` — FINAL3P, FINAL 35 — 9 records

| seq | time (UTC) | event | shot | stage | stored score | stored x,y |
|----:|---|---|---:|---:|---:|---|
| 35 | 12:59:50.853 | ShotAccepted | 5 | 3 | 10.8 | −100, −100 |
| 36 | 13:00:07.931 | ShotAccepted | 6 | 3 | 10.8 | −100, −100 |
| 37 | 13:00:23.372 | ShotAccepted | 7 | 3 | 10.8 | −100, −100 |
| 38 | 13:00:43.835 | ShotAccepted | 8 | 3 | 10.8 | −100, −100 |
| 39 | 13:01:52.447 | ShotAccepted | 9 | 3 | 10.8 | −100, −100 |
| 41 | 13:02:29.206 | ShotAccepted | 10 | 3 | 10.8 | −100, −100 |
| 45 | 13:04:35.610 | SighterAccepted | — | 4 | 10.8 | −100, −100 |
| 51 | 13:06:49.663 | SighterAccepted | — | 4 | 10.8 | −100, −100 |
| 52 | 13:07:12.861 | SighterAccepted | — | 4 | 10.8 | −100, −100 |

This session is the one filed under `Sessions/Corrupt/`. Its hash chain and
sequence numbering are continuous end to end; it was quarantined for a reason
not visible in the file as collected.

### `session_20260823T130813_9c988399.jsonl` — FINAL3P, FINAL 35 — 3 records

| seq | time (UTC) | event | shot | stage | stored score | stored x,y |
|----:|---|---|---:|---:|---:|---|
| 20 | 13:13:33.468 | SighterAccepted | — | 2 | 10.8 | −100, −100 |
| 22 | 13:13:53.625 | SighterAccepted | — | 2 | 10.8 | −100, −100 |
| 31 | 13:14:27.480 | ShotAccepted | 1 | 3 | 10.8 | −100, −100 |

### `session_20260823T131510_6506fc2d.jsonl` — PRONE50, 60 — 2 records

| seq | time (UTC) | event | shot | stage | stored score | stored x,y |
|----:|---|---|---:|---:|---:|---|
| 16 | 13:19:23.316 | ShotAccepted | 11 | 1 | 10.8 | −100, −100 |
| 17 | 13:19:40.397 | ShotAccepted | 12 | 1 | 10.8 | −100, −100 |

**This third session matters disproportionately.** It is a plain 50 m Prone
qualification. There is no finals code in it, no incident report anywhere in the
session, and the corruption begins at shot 11 — immediately after the 10-shot
flush and the replug. It is the proof that the defect was never
finals-specific and never caused by the Incident Report.

## What is NOT affected

- Shots 1–4 of `4449b052` and every sighter before 12:57:57 UTC: genuine,
  correctly measured, correctly scored.
- Sighters 1–9 of `9c988399` (13:09:18 – 13:11:59 UTC): genuine.
- Shots 1–10 of `6506fc2d` (13:15:38 – 13:18:33 UTC): genuine.
- **Tablet-01 and Tablet-04 hold no corrupted records at all.** Every reconnect
  on those tablets adopted a counter that agreed (0 → 0), so the desynchronisation
  never started. Their acquisition stoppages are ACQ-FLUSH-001 and cost time,
  not data.

## Where the corruption starts, in each session

The onset is not approximate. In all three sessions the last genuine shot and
the first corrupted one bracket a single logged reconnect that adopted a
**mismatched** counter (log times are UTC+2):

| session | last genuine | reconnect adopting `target reports 1` | first corrupted |
|---|---|---|---|
| `4449b052` | 12:55:36 UTC | `14:57:57.959` log | 12:59:50 UTC |
| `9c988399` | 13:11:59 UTC | `15:12:48.172` log | 13:13:33 UTC |
| `6506fc2d` | 13:18:33 UTC | `15:19:07.128` log | 13:19:23 UTC |

Three reconnects with a mismatched counter; three corrupted sessions. No other
tablet logged such a reconnect and no other tablet has a corrupted record.

## Incident Report

`4449b052` contains two EST incidents, the second with the operator's own words:
`reason: "registering 10.8 constantly"`. The first was raised at 12:58:31 UTC —
**34 seconds after** the reconnect that caused the desynchronisation.

The Incident Report is a **witness, not a cause**. It journals events and pauses
the competition clock; it touches no counter, no Modbus register and no
acquisition state. `6506fc2d` shows the identical corruption with no incident
report in the session at all.

## Handling

1. **Do not use these fourteen records as shooting results** — not for scores,
   averages, group analysis, coaching feedback or any report.
2. **Do not edit, correct or remove them.** They are the evidence that the
   defect existed and the reference the regression test is built from.
3. Any report already produced from these sessions carries invalid totals for
   the affected shots and should be withdrawn rather than corrected — the true
   coordinates were never recorded and cannot be recovered.
4. The shot the target counted while the application was offline (one per
   affected reconnect) was never captured at all. It is not among the fourteen;
   it is simply absent.

## Provenance note

The evidence folder is named `RC3A-Physical-Test-Evidence`, but every session
journal in it reports `appVersion: 0.9.0-RC2g-DIAG`. RC3a is `0.9.0-RC3a-SETA`
(commit `488d506`, tag `techaim-v0.9.0-rc3a-seta-eval`). The folder name is
historically inaccurate and is **left unchanged** — renaming evidence after the
fact is worse than a wrong label. The defects were confirmed present in the
RC3a source as well, so the findings apply to it.

Tablet-03 is not part of this test: its logs are dated 2026-07-18, it holds no
sessions, and it lost the target link within ten seconds of each launch.

## Fixed by

- `ACQ-DESYNC-002` — the reconnect proves the relationship between the counter
  and the captured coordinates instead of assigning it; the shot number is the
  count of coordinates actually held.
- `ACQ-SENTINEL-003` — an index the arrays do not hold can no longer return a
  number that looks like a coordinate.

Regression: `tests/reliability/tst_acquisition_integrity.cpp`, TEST E, which
reconstructs this exact sequence — ten shots, the flush, the replug, the target
reporting 1, and the genuine next shot carrying the coordinates the log proves
the backend decoded (−3.9, 1.0).
