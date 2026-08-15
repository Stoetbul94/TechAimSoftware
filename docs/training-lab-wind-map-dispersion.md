# Wind Map — sample-size-safe dispersion comparison

**Defect:** EVID-WM-001 · **Status:** CORRECTED 2026-07-30 ·
**Analytics version:** `windmap-analytics-v2`

How Wind Map decides that one group of shots is more or less dispersed than
another, why the previous method was unsound, and what is now display-only.

---

## 1. The defect

Wind Map classified a condition as **compact** or **wider** by comparing
`groupDiameterMm` — the **extreme spread**, the largest centre-to-centre
distance between any two shots in the group.

Extreme spread is an **order statistic**: it is determined entirely by the two
most distant shots and discards every other shot. Its expected value increases
with sample count, because a larger sample gives more opportunity to contain a
distant pair.

Wind Map compares groups whose sizes need only each reach `kMinSamplesComparison`
(5). A 20-shot condition and a 5-shot reference are a normal, expected pairing —
an athlete simply shoots more under the conditions that persist. So a verdict
could turn partly on **how many shots were fired under a condition** rather than
on how the shots were placed.

### A correction to the original write-up

The governance audit illustrated this with the control-chart constant `d2(n)`,
quoting an expected ratio of `d2(20)/d2(5) ≈ 1.61` and noting it exceeds the
1.50 "wider" threshold.

**That figure was misapplied.** `d2` is the expected **range of a
one-dimensional normal sample**. Wind Map's group diameter is the **maximum
pairwise distance among two-dimensional points**, a different statistic with a
different distribution. The 1.61 figure is therefore an illustration of the
direction of the effect, **not a derivation of its magnitude for this metric**,
and it is not repeated as one anywhere in the product.

The qualitative defect stands on its own: extreme spread depends on two shots
and grows with sample count. No exact 2-D estimator is claimed. If a magnitude
is ever needed, it must come from a derivation or a simulation for the actual
maximum-pairwise-distance metric.

**Demonstrated instead, deterministically.** `tests/reliability/tst_windmap_dispersion.cpp`
case 5 builds two groups with **identical sample SD** at n = 5 and n = 20, and
asserts that the 20-shot group's extreme spread exceeds **1.25×** the 5-shot
group's — enough to flip the compact classification — from sample size alone.
That is a measured property of the constructed data, not an appeal to `d2`.

---

## 2. The classification metric

**Radial RMS dispersion**, about the group's own centre, in millimetres:

```
radialRmsMm = sqrt( SUM( (x - meanX)^2 + (y - meanY)^2 ) / (n - 1) )
```

Equivalently the square root of the trace of the sample covariance matrix.

| Property | Value |
|---|---|
| **Units** | millimetres |
| **Denominator** | `n - 1` (sample, not population) |
| **Minimum sample** | `kMinSamplesDispersion` = 5. Undefined below n = 2 and unstable just above it; the shared floor keeps it at 5 in practice. |
| **Uses every shot** | Yes — this is the point. Interior shots contribute, so adding them cannot inflate it. |
| **Computed in** | `WindMapAnalyticsEngine::statsFor` — one place, pure QtCore |

Because the x and y sample standard deviations share the same denominator:

```
radialRmsMm^2  ==  horizontalSdMm^2 + verticalSdMm^2
```

exactly. This identity is **asserted by test** (case 0, to 1e-6), not assumed.

### Also exposed

| Field | Meaning | Role |
|---|---|---|
| `radialRmsMm` | as above | **classification** |
| `horizontalSdMm` | sample SD of x (n−1) | supporting detail |
| `verticalSdMm` | sample SD of y (n−1) | supporting detail |
| `meanRadiusMm` | mean distance from the group's own centre | offset scaling |
| `groupDiameterMm` | extreme spread | **display only** |
| `horizontalSpreadMm` / `verticalSpreadMm` | max − min ranges | **display only** |

All are computed centrally in C++. **QML and report code compute none of them**
and must never compare the display-only fields between groups.

---

## 3. Why extreme spread stays

It is removed from **classification**, not from the product. An athlete asking
"how big was that group?" is asking a reasonable question, and "the two
outermost shots were 31 mm apart" answers it in a way a radial RMS figure does
not. It remains on screen, in the group tables, and in the descriptive part of
the compact-but-offset verdict — labelled as descriptive.

What it may no longer do is decide anything.

---

## 4. Classification rules

All four are **REASONED PRODUCT RULE — COACH REVIEW REQUIRED**. None is
research-validated; none is an ISSF rule.

| Rule | Threshold | Applied to |
|---|---|---|
| Comparatively compact | compared ≤ **1.25 ×** reference | radial RMS |
| Clearly wider | compared ≥ **1.50 ×** reference | radial RMS |
| Indeterminate | > 1.25 and < 1.50 | **no verdict is forced** |
| Meaningful centre offset | `max(3.0 mm, 1.0 × reference mean radius)` | shift magnitude vs mean radius |

The **ratios are unchanged** from the previous implementation — only the metric
they are applied to. That was deliberate: changing both at once would have made
the effect of the correction impossible to isolate.

### Categories affected

Migrated off extreme spread: **CompactButOffset**, **WiderUnderCondition**,
**SimilarAcrossConditions**, **WideAcrossConditions**, and —
**extending the brief by one** — **PositionSpecificDifference**, which compares
dispersion across 3P positions whose shot counts routinely differ and is
therefore the same bias class.

---

## 5. Elevated dispersion across all conditions

The superseded rule fired when every group's **extreme spread** reached
**40 mm**. That constant is **gone from the code**. Its value survives only as
history in `training-lab-wind-map-verdict-rules.md`, so it cannot quietly keep
classifying athletes under a new name.

The replacement:

```
elevated  ⇔  radialRmsMm  ≥  1.50 × ringSpacingMm
```

— 12.0 mm at 50 m rifle (ring spacing 8.0 mm), resolved through
`TargetGeometry` so it scales with the discipline instead of being a bare
millimetre figure. The same principle Group Pattern Coach already uses.

It is the **weakest rule in the set**, because when every group is wide there is
no within-session reference to scale against, so some absolute-ish bar is
unavoidable. It is therefore treated as follows:

- the athlete-facing headline is **"Dispersion remained elevated across the
  recorded conditions"**;
- the verdict's own limitations tell the athlete the rule is **provisional and
  awaiting coach review**;
- it refers to Group Pattern Coach rather than issuing a judgement;
- a test asserts it never says **poor, bad, weak, unacceptable or inadequate**.

---

## 6. Backward compatibility

**Nothing stored changed.** Shot coordinates, journal events, immutable wind
snapshots and the reducer state are untouched, and case 16 of the dispersion
suite compares every stored field before and after an analysis and requires them
byte-identical. Existing sessions replay and resume exactly as before.

What changed is the method that **interprets** them. So every analysis carries:

| Field | Value |
|---|---|
| `analyticsVersion` | `windmap-analytics-v2` |
| `ringSpacingMm` | the geometry the elevated-dispersion rule used |

Both are projected into the analysis model's `session` block, so a screen or a
report can state which method produced its verdicts. A report produced by the
earlier extreme-spread classification must never be presented as though it used
the corrected one.

**Version history**

| Version | Classification metric |
|---|---|
| *(unstamped, pre-2026-07-30)* | extreme spread (`groupDiameterMm`) — **superseded, biased by sample size** |
| `windmap-analytics-v2` | radial RMS dispersion |

---

## 7. Tests

`tests/reliability/tst_windmap_dispersion.cpp` — deterministic, geometrically
controlled data only (evenly spaced lines, points on circles, cluster
constructions). No random data. A seeded Monte Carlo study may be added as
supporting evidence but may never replace these boundary cases.

| # | Case |
|---|---|
| 0 | The estimator: circle mean radius, `R·sqrt(n/(n−1))`, the trace identity, the version stamp |
| 1–4 | Equivalent dispersion at 5v5, 5v10, 5v20, 20v5 → **no wider verdict** |
| 5 | Equal radial RMS, extreme-spread ratio > 1.25 → old rule fails, new rule correct |
| 6 | Equal extreme spread, different radial RMS → correctly separated |
| 7 | Ten added interior shots → no wider verdict |
| 8 | A genuinely 1.6× wider radial distribution → verdict fires, cites `radialRmsMm` |
| 9–12 | Ratio boundaries: exactly 1.25, just above, just below 1.50, exactly 1.50 |
| 13 | Both sides must meet the sample minimum |
| 14 | 3P positions stay separate; the verdict is scoped to the position |
| 15 | Sighters excluded |
| 16 | Stored shot data byte-identical after analysis |
| 17 | Elevated-dispersion wording, provisional limitation, no judgement words |
