# Saturday 2026-08-29 — 50 m 3P readiness

Only the two modes the operator will actually use. Written to be useful, not
reassuring.

Rule authority for both: **ISSF Rule Book 2026, Edition 2025 (Second Print
07/2026), effective 1 July 2026**, read from the official PDF on 2026-08-27.

---

## 50 m Rifle 3 Positions — INDOOR QUALIFICATION

### Frozen Saturday configuration

| | |
|---|---|
| **VENUE MODE** | **INDOOR** |
| **PREPARATION / SIGHTING** | **15:00**, with the `30 SECONDS` warning — **VERIFIED** |
| **MATCH** | **1:30:00 (90 minutes)** |
| **COURSE** | **3 × 20** — Kneeling 1–20, Prone 21–40, Standing 41–60 |
| **SCORING** | **INTEGER** (full ring) |
| **MATCH CLOCK** | **CONTINUOUS** — one clock for the whole course |
| **MATCH WARNINGS** | **10 minutes — VERIFIED**, **5 minutes — VERIFIED** (6.11.1.2 e) |
| **POSITION-CHANGE SIGHTERS** | inside the remaining 1:30:00 |
| **OUTDOOR 1:45:00** | **NOT USED FOR THIS COMPETITION** |

### The rule, verbatim (6.11.9.2)

> **Preparation and Sighting time.** Fifteen (15) minutes to fire an unlimited
> number of sighting shots.
>
> **Course of fire.** Twenty (20) shots in each position, in the sequence
> Kneeling, Prone, Standing, in a total time limit of 1hr 45 minutes (105
> minutes) if outdoor range is used. **1 hr 30 minutes (90 minutes) if indoor
> range is used.**
>
> **Scoring.** Full ring (integer) scoring.

"**A total time limit**" — one limit for the whole 3 × 20, including both
position changes and the unlimited prone and standing sighting. There is no
per-position clock in the rule and there must be none in the software.

### Audit

| Rule item | Official | Software | Status |
|---|---|---|---|
| Official shots | 60 | `matchShootCount === 60` gates `is3PMatch` | **PASS** |
| Course | 3 × 20, K → P → S | position breaks at 20 and 40 | **PASS** |
| Preparation and sighting | 15:00 | `m_prepTimeMinutes = 15` | **PASS** |
| Match time, indoor | **90 min** | `getTimeCount(60)` returns `90 * 60` for `game_sub_mode == 1`; start page shows "90 min" | **PASS** |
| Match time, outdoor 105 min | not used | **no 105-minute value exists anywhere in the product** — `105` appears only in a comment | **PASS** |
| Clock continuity | one total limit | `enterPositionTransition()` touches **no** timer symbol and logs "match clock keeps running" | **PASS** |
| Clock not reset at a transition | — | the only site that zeroes elapsed time is the clock row going invisible; its gate is `APPSETTINGS.timer()` and the three finals/training flags — **none of which a position change moves** | **PASS** |
| Sighters excluded from the official count | prone/standing sighters are not official | sighters routed to `globalSlighterModel`, official count from `globalMatchModel` | **PASS** |
| Completion | at 60 official shots | `globalMatchModel.count >= matchShootCount` | **PASS** |
| Scoring | integer | discipline-configured | **PASS** |
| Preparation start | `PREPARATION AND SIGHTING TIME...START` (6.11.1.1 g) | announced | **PASS** |
| Preparation 30 s warning | `30 SECONDS` after 14:30 elapsed (6.11.1.1 i) | announced at 30 s remaining | **PASS** |
| Preparation end | `END OF PREPARATION AND SIGHTING...STOP` (6.11.1.1 j) | announced | **PASS** |
| Match start | `MATCH FIRING...START` (6.11.1.2 a) | announced | **PASS** |
| **Match warning 10 min** | **6.11.1.2 e) — required** | announced once at 10:00 remaining | **PASS** |
| **Match warning 5 min** | **6.11.1.2 e) — required** | announced once at 5:00 remaining | **PASS** |
| Match end | `STOP` (6.11.1.3) | announced at 0:00 | **PASS** |

**Regressions added:** `qualification3p_indoor_must_use_90_minute_continuous_clock`
and `qualification3p_indoor_must_never_start_105_minute_clock` — 28 checks in
`tests/qml`, covering the configured value, the absence of any 105-minute path,
that the position transition touches no timer symbol, and that the clock row's
visibility gate depends on nothing a position change moves.

### Verdict

| | |
|---|---|
| **Rule authority verified?** | **YES** — 6.11.9.2, official Second Print |
| **Software flow matches?** | **YES** on every dimension audited |
| **50 M 3P INDOOR QUALIFICATION** | **PASS** |
| **Physical test status** | **NONE.** 50 m 3P has never been fired with this software |
| **Saturday risk** | **LOW–MEDIUM** — the configuration is verified correct; the risk that remains is that the discipline has no physical record at 50 m |
| **Code change required before Saturday?** | **NO** |

### CRO announcements — were MISSING, now implemented

The qualification path had **no CRO command text of any kind** — only countdown
displays. All seven announcements required by 6.11.1.1 and 6.11.1.2 were
absent. Classified **SEV-2 RULES / RANGE-OFFICER COMMAND GAP** (no competition
state was wrong; the range officer simply was not prompted).

They are now derived from the **existing** clocks: no new timer, and nothing
that starts, stops, pauses or extends a clock, or touches a shot, position or
target mode. Gated so they can never fire in a Final, which owns its own
commands.

`qualification3p_indoor_cro_time_warnings` **executes** the real threshold
logic extracted from `CenterPane.qml`: silence above 10:00, `10 MINUTES` at
exactly 10:00 and once only, `5 MINUTES` at exactly 5:00 and once only, re-armed
for a new session, and silent when the legacy clock is not ours.

**One caveat, stated plainly:** the tests prove the configured value, the
announcement thresholds, and that nothing on the transition or announcement
path touches the clock. They do **not** drive a real 90-minute clock through
two position changes — that needs a long run or a timescale hook the
qualification path does not have.

---

## 50 m Rifle 3 Positions — FINAL

| | |
|---|---|
| **Rule authority verified?** | **YES.** Rule 6.17.3, read section by section from the official PDF |
| **Software flow matches?** | **YES.** Every duration, shot count, elimination point and CRO command phrase checked against the verified text |
| **Automated coverage** | 379 checks in `tests/finals`, including the full 35-shot course driven through the real panel by the real controller, early-completion and two late-transition cases, target-mode sequence, and the standing-transition UI |
| **Known open defects** | Tie-breaking (6.17.3 i) not implemented — cross-lane, impossible on a single target |
| **Physical test status** | **NONE** — DEMO / controller-driven only |
| **50 M 3P FINAL** | **PASS** |
| **Saturday risk** | **LOW for the rule engine; MEDIUM overall**, because it has never run against live hardware |
| **Code change required before Saturday?** | **NO** |

The reported "extra 5-minute Standing sighting block" does not exist in the
engine. What did exist was a presentation gap at the 30-second interval after
the 22:00 STOP; it now reads `NEXT · STANDING SERIES 1 — 5 MATCH SHOTS — WAIT
FOR LOAD`. **That correction is in the working tree, not in RC3E.**

---

## Both modes

| | |
|---|---|
| **50 M 3P INDOOR QUALIFICATION** | **PASS** |
| **50 M 3P FINAL** | **PASS** |

Both are PASS, so the Saturday candidate may be packaged. It has **not** been
packaged — that is a separate, explicit instruction.

Neither mode has ever been fired at 50 m with this software. That is the
largest remaining risk and no amount of automated evidence changes it.
