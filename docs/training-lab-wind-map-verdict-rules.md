# Wind Map — verdict classification rules

**Phase:** STAGE 6.1.2 · **Date:** 2026-07-30
**Status:** specification — implemented by `WindMapVerdictEngine`

These are **Tech Aim training-analysis rules**. They are **not** ISSF rules,
not a medical or scientific diagnosis, and not a statistical significance
test. They exist to decide which plain-language verdict a session earns, and
every one is deterministic and testable.

## 0. Claim classification — read this first

| Threshold | Classification |
|---|---|
| `kMinSamplesMpi` = 3 | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kMinSamplesDispersion` = 5 | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kMinSamplesComparison` = 5 per side | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kOffsetMinimumMm` = 3.0 | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kOffsetRelativeToMeanRadius` = 1.0 × | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kCompactRelativeToReference` = 1.25 × | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kWiderRelativeToReference` = 1.50 × | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| `kWideProvisionalMm` = 40.0 | **PROVISIONAL TECH AIM RULE — COACH REVIEW REQUIRED** (§3.6) |
| `kFragmentedMinConditions` = 3 | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |

**NOT ONE of these is research-validated, and none is an ISSF rule.** The
verified sources in `docs/research/wind-map-feedback-evidence.md` support the
*cautions* this product applies; they validate no number in it.

**No COACH-APPROVED PRODUCT RULE exists yet** — no coach review has taken
place. Until one does, every threshold here is provisional.

Evidence base: `docs/research/wind-map-feedback-evidence.md` — read its §0
warning first.

---

## 1. Audit of the existing analytics output

Before proposing a threshold, what does `WindMapAnalyticsEngine` already
produce that a rule can be built from?

| Available per condition group | Guaranteed when |
|---|---|
| `n` | always |
| `meanScore` | n ≥ 1 |
| `mpiXMm`, `mpiYMm` | **n ≥ 3** (`hasMpi`) |
| `meanRadiusMm`, `groupDiameterMm`, `horizontalSpreadMm`, `verticalSpreadMm`, `scoreStdDev`, `radiusStdDev` | **n ≥ 5** (`hasDispersion`) |
| `ShiftVector.dxMm/dyMm/magnitudeMm` vs the position's reference | **n ≥ 5 on BOTH sides** (`valid`) |

Per position: `reference` (position-specific), `countedShots`, `sighterShots`,
and the three groupings.

**What is NOT available, and therefore may not be used in a rule:** any
inferential statistic, any p-value, any confidence interval, any cross-session
history, any equipment or ammunition data.

**Consequence.** Every rule below is expressible in `n`, `groupDiameterMm`,
`meanRadiusMm`, `horizontalSpreadMm` and `magnitudeMm`. No new mathematics is
introduced, and the accepted formulas are untouched.

---

## 2. Evidence levels

Four product states. They describe **what the sample supports**, not
statistical confidence.

| Level | Condition | What may be said |
|---|---|---|
| **INSUFFICIENT** | the sample threshold for the statistic in question is not met | nothing about the group; state how many more shots are needed |
| **INDICATIVE** | one group has `hasDispersion` (n ≥ 5) but **no second valid group** exists to compare against | describe that group; **no** condition-associated difference |
| **COMPARATIVE** | reference and compared group **both** have n ≥ 5 and both have `hasMpi` | describe an observed difference; causation remains unproven |
| **REPEATED** | the same directional pattern seen across separately completed comparable sessions | **RESERVED — never assigned from one session** |

`REPEATED` is defined and reserved in code; the engine can never emit it, and a
test asserts that.

**These are not confidence intervals.** The UI states this in plain language.

---

## 3. Classification thresholds

Every constant is named, justified, and scale-aware where possible.

### 3.1 Sample thresholds — inherited, unchanged

| Constant | Value | Source |
|---|---|---|
| `kMinSamplesMpi` | 3 | approved Stage 6 |
| `kMinSamplesDispersion` | 5 | approved Stage 6 |
| `kMinSamplesComparison` | 5 per side | approved Stage 6 |

### 3.2 Is a displacement meaningful?

A shift must clear **both** a scale-aware and an absolute bar, so it survives
neither a huge group nor a trivially small one.

| Rule | Value | Why |
|---|---|---|
| `kOffsetRelativeToMeanRadius` | **1.0 ×** the reference group's `meanRadiusMm` | A centre displaced by less than the reference group's own average scatter is inside the noise that group already shows. |
| `kOffsetMinimumMm` | **3.0 mm** | Below this the displacement is small relative to 50 m .22 target geometry and to the athlete's own estimate of the condition. A floor stops a very tight group generating a verdict from a fraction of a millimetre. |

**A shift is MEANINGFUL when `magnitudeMm ≥ max(3.0, 1.0 × referenceMeanRadius)`.**

### 3.3 Is a group compact?

| Rule | Value | Why |
|---|---|---|
| `kCompactRelativeToReference` | compared `groupDiameterMm` ≤ **1.25 ×** reference `groupDiameterMm` | "Compact" is relative to what this athlete shot in the reference condition — never an absolute target standard, which would vary by athlete and position. |

### 3.4 Is a group wider?

| Rule | Value | Why |
|---|---|---|
| `kWiderRelativeToReference` | compared `groupDiameterMm` ≥ **1.50 ×** reference `groupDiameterMm` | A half-again increase is visible on the plot and unlikely to be reversed by one or two shots at these sample sizes. |

The gap between 1.25 and 1.50 is deliberate: a group in it is neither compact
nor wider, and produces **no** dispersion claim.

### 3.5 Are groups similar?

| Rule | Value |
|---|---|
| centres | shift is **not** meaningful (§3.2) for every compared group |
| sizes | every compared `groupDiameterMm` within **1.25 ×** of the smallest |

Reported as *"this session did not establish a pattern"* — **never** as proof
that wind had no effect.

### 3.6 Are groups wide across all conditions?

**DECISION (Stage 6.1.3): Option A — retained internally, provisional, with
relative athlete-facing wording.**

| Rule | Value | Status |
|---|---|---|
| `kWideProvisionalMm` | every valid group's `groupDiameterMm` ≥ **40.0 mm** | **PROVISIONAL TECH AIM RULE — COACH REVIEW REQUIRED** |

**Reasoning for the decision.** Verification found **no source supporting a
millimetre figure for "wide" at 50 m**, and S1 records that the small-bore
evidence base is thin (4 studies). Removing the category outright (Option B)
would also remove a genuinely useful referral — when every group is wide, the
most helpful thing the software can say is *"no condition explains this, look
at technique with the right tool"*. So the rule is kept and its **claim** is
weakened instead of its usefulness:

- the constant is **named** `Provisional` so no reader mistakes it for settled;
- the headline reads *"Groups remained relatively wide across the recorded
  conditions"* — the wording the brief prefers;
- the interpretation says *"No single recorded condition separates these
  groups from one another"* rather than judging the shooting;
- **the athlete's group is never called poor, bad or inadequate** — asserted
  by test;
- it refers to Group Pattern Coach rather than diagnosing a fault.

If coach review rejects the figure, only the constant changes; the wording
already makes no claim that depends on it.

### 3.7 Fragmented data

| Rule | Value | Why |
|---|---|---|
| `kFragmentedMinConditions` | ≥ **3** distinct conditions | |
| and | **no** condition reaches `kMinSamplesComparison` (5) | The athlete recorded diligently but spread the shots too thin to compare anything. This is a *data-quality* verdict, distinct from plain insufficiency. |

### 3.8 When no rule fits

If thresholds are met but no category's rule is satisfied, the engine emits
**`NoValidComparison`** — the metrics are presented and no category is forced.
This is required behaviour, not a fallback bug.

---

## 4. Verdict priority

At most one **primary** verdict is shown; the rest are secondary.

1. `InsufficientSample` — a data-quality warning outranks any analysis
2. `FragmentedData`
3. `CompactButOffset` — the most actionable valid comparison
4. `WiderUnderCondition`
5. `WideAcrossConditions`
6. `SimilarAcrossConditions`
7. `PositionSpecificDifference` — session scope, 3P only
8. `NoValidComparison`

---

## 5. Scope

Every verdict declares `Session`, `Position`, `Condition` or `CrossSession`
(reserved). A `Session`-scoped verdict is **never** shown as a position
result — the defect UI-WIND-006 exists for.

For 3P, positions are analysed independently and a position comparison is
`Session`-scoped, labelled *SESSION-LEVEL POSITION COMPARISON*. Kneeling is
compared with Kneeling **across sessions**, which is why that comparison is
reserved to the cross-session feature.

---

## 6. Wording rules

**Permitted:** *observed*, *recorded*, *measured*, *while this condition was
recorded*, *observed alongside*, *may mean*, *worth testing*.

**Prohibited, enforced by test:** *caused*, *because of*, *due to*, *proves*,
*significant*, *confidence*, sight clicks, aim-off, hold left/right, any
predicted correction, any diagnosis of a technique fault.

---

## 7. Relative wind direction

`directionDegrees` is the **authoritative recorded compass value and is never
mutated**. If a session records the optional `firingDirectionDegrees`, the
engine *derives* an athlete-relative label (headwind, left-to-right crosswind,
…) as an additional field.

Sessions without it — which is every session recorded so far — display
*"Relative wind direction unavailable — firing direction was not recorded."*
The field is **optional**, and no existing journal needs it to be read.

---

## 8. Cross-session foundation

Defined, not built. Two sessions are comparable only when athlete, discipline,
position, distance and analytics version match. Equipment, ammunition, range
and firing direction are recorded when available and shown as caveats.

`REPEATED` stays unassignable until a feature actually evaluates separate
sessions.
