# 50 m 3P Final — which rule governs, and what it says

**Decision: ISSF Rule Book 2026, Edition 2025 (Second Print 07/2026), effective
1 July 2026, Rule 6.17.3 is the authority.** The First Print 12/2025 is
superseded. Sources: [ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md](ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md).

## First Print vs Second Print — the 3P Final flow is IDENTICAL

The state flow written on 2026-08-27 from the project owner's First Print
extract was compared line by line against the Second Print text read from the
PDF. **Every duration, every shot count, every command phrase and every
elimination point is the same.**

| Element | First Print (as supplied) | Second Print (verified) | Same? |
|---|---|---|---|
| Preparation and sighting | 5:00, kneeling, unlimited sighters | 5:00, kneeling, unlimited sighters | ✅ |
| 30-second hold before it | yes | yes | ✅ |
| Warning inside prep | 30 SECONDS | `30 SECONDS` | ✅ |
| Combined match block | 22:00 | twenty-two (22) minutes | ✅ |
| Inside the block | K10, change, prone sighters, P10, change, standing sighters | same | ✅ |
| Warnings | 17:00 → FIVE MINUTES; 21:30 → THIRTY SECONDS | After 17 min "FIVE MINUTES"; after 21 min 30 s "THIRTY SECONDS" | ✅ |
| Block end | STOP at 22:00 | after 22 min "STOP" | ✅ |
| Interval before series | 30 s | After 30 seconds | ✅ |
| LOAD → START | 5 s | After 5 secs | ✅ |
| Standing series | 2 × 5 shots, 250 s each | two 5-shot series, 250 seconds each | ✅ |
| Elimination after series 2 | 8th and 7th | two lowest, 30 shots total, 8th and 7th | ✅ |
| Singles | 31–35, 50 s each | single shots, 50 seconds | ✅ |
| Elimination per single | 6th, 5th, 4th, bronze, gold/silver | after 32→5th, 33→4th, 34→3rd, 35→2nd and 1st | ✅ |
| Total | 35 | "a total of 35 shots in the Final" | ✅ |
| Scoring | decimal | tenth-ring (decimal) | ✅ |

**Consequence: no implementation change was required by the print correction.**
The software was already correct under both prints. What changed is the
*authority* the documentation cites, which is now the current one and carries
the ✅ tag.

## The rule, verbatim where it matters

> The Final consists of 10 MATCH shots in each of the Kneeling and Prone
> positions, fired in that order, **in a total time of twenty-two (22)
> minutes**. When finished firing Match shots in the Prone position, athletes
> must change to the Standing position and **may fire unlimited sighting shots
> in any time remaining**. Athletes are responsible for changing their targets
> from Match to Sighters when changing positions.
> — 6.17.3 a)

> Finalists must fire ten (10) Match shots in the Kneeling position, insert
> their safety-flags, change to the Prone position and fire unlimited sighting
> shots, then fire ten (10) Match shots. After finishing those ten shots they
> must insert safety flags and change to the Standing position. They may then
> fire unlimited sighting-shots **in the time remaining before the CRO commands
> "STOP" at the end of the 22 minute Match Firing time.**
> — 6.17.3 e)

> After seventeen (17) minutes, the CRO will announce, "FIVE MINUTES". After 21
> minutes and 30 seconds, the CRO will announce, "THIRTY SECONDS". After
> twenty-two minutes, the CRO will command, "STOP".
> — 6.17.3 f)

> After 30 seconds. the CRO will command "FOR THE NEXT COMPETITION
> SERIES...LOAD". After 5 secs., "START". Athletes have 250 seconds to fire
> each 5-shot MATCH series.
> — 6.17.3 g)

**This settles the reported defect: there is no preparation or sighting period
after the 22-minute STOP. The rule goes STOP → 30 s → LOAD → 5 s → START →
250 s.** That 250-second (4:10) series is the first thing after the STOP, and
is the only candidate for what was read as "another approximately 5-minute
Standing block".

## Tie-breaking — 6.17.3 i), now recorded

> If there is a tie for the lowest ranking athlete to be eliminated, the tied
> athletes will fire an additional tie-breaking shot(s) until the tie is broken.
> … If there are two athletes tied at the end of the second 5-shot series in the
> Standing position, they are both eliminated and the tie will be broken by
> countback as follows: 1) The highest total score in the second Standing
> 5-shot series; 2) The highest total score in the first Standing 5-shot series;
> 3) The highest total value final shot of the prone series; etc. If there are
> more than two athletes tied after the second 5-shot series, then there will be
> tie-breaking shots to decide the two athletes eliminated.

**Tech Aim implements none of this, and correctly so.** Every clause depends on
comparing athletes across lanes. A single-target application has no such
knowledge and must not fabricate a ranking. What it can do — and does — is show
the structural elimination checkpoint in the conditional voice
("8th and 7th places *would be* eliminated here").

**Requires Range Management coordination before any of it can be implemented:**
cross-lane ranking, identifying which athletes are tied, ordering a tie-breaking
shot, and countback across series. Recorded, not attempted.

## Software conformance

Every value in `src/finals/Finals3PConfig.h` was checked against the verified
text: `prepSightMs` 300000, `holdMs` 30000, `prepWarnMs` 270000, `stage1Ms`
**1320000**, `stage1Warn1Ms` **1020000**, `stage1Warn2Ms` **1290000**,
`preSeriesGapMs` 30000, `loadDelayMs` 5000, `seriesMs` **250000**, `singleMs`
**50000**, kneeling 10, prone 10, series 5.

**All match. The command strings in `Finals3PController::issueCommand` are the
rule's own wording, verbatim.**

The header comment in `Finals3PConfig.h` already cited *Second Print 07/2026,
effective 1 July 2026* — it was right all along, and the First Print reference
introduced on 2026-08-27 was the outlier. That is now corrected.
