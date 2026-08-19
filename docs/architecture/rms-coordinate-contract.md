# RMS coordinate contract

What a shot's `x` and `y` actually mean, from the moment the node accepts the
shot to the pixel RMS paints. Traced through the code on 2026-08-19, not
assumed.

---

## 1. The answer, first

```
X unit:                     millimetres
Y unit:                     millimetres
origin:                     target centre (0, 0)
positive X:                 right, as the athlete faces the target
positive Y:                 UP  (see §4 — software convention, not hardware-verified)
transformations before RMS: mm -> hundredths of a mm (qint32) -> mm
                            value-preserving; quantised to 0.01 mm
precision on the wire:      0.01 mm
transformations inside RMS: one divide by the face radius, one Y flip. Nothing else.
```

**PHYSICAL SENSOR ORIENTATION: NOT PHYSICALLY VERIFIED.** See §6.

---

## 2. The path, link by link

| # | Where | Field | Unit | What happens |
|---|---|---|---|---|
| 1 | `CenterPane.qml::calculateShootingSocre(xPoint, yPoint, …)` | `xPoint`, `yPoint` | mm | The acquisition value the node scores from. Also stored as `lastShotXmm` / `lastShotYmm`. |
| 2 | `QualificationController::recordShot(xMm, yMm, …)` | `xMm`, `yMm` | mm | Accepted into the session. |
| 3 | `ShotCore.xHundredthMm` (`src/reliability/events/EventTypes.h`) | `qint32` | 0.01 mm | `qRound(xMm * 100.0)`. The node's durable fixed-point record. |
| 4 | `NodeTelemetryService` (node-telemetry branch) | `rawXMm` | mm | `hundredthMmToMm(v) = v / 100.0`. |
| 5 | Protocol v1 JSON | `rawXMm`, `rawYMm` | mm | `RmsProtocol.cpp` encode/decode. Commented "diagnostics/display only". |
| 6 | `RangeMonitor` | `AcceptedShot.rawXMm` | mm | Stored verbatim. No arithmetic. |
| 7 | `DisplayLaneModel` | `xMm`, `yMm` | mm | Passed to QML **unchanged**, beside the normalised pair. |
| 8 | `TargetGeometry::normalise(spec, xMm, yMm)` | — | fraction | `x / faceRadiusMm`, and `-y / faceRadiusMm`. **The only Y flip in RMS.** |
| 9 | `RmsTargetView.qml` | `x`, `y` | px | `cx + n.x * faceRadiusPx`, `cy + n.y * faceRadiusPx`. |

**A shot at x = 5 mm is displaced by exactly 5 mm of target geometry.** There is
one scale division (step 8) and one pixel multiplication (step 9), and the same
`faceRadius` value governs the rings and the markers, so no double scaling is
possible: if the divisor were wrong the rings would move with the shots.

Verified by test: `tst_target_geometry.cpp` asserts that the fixed-point
round-trip in steps 3–4 is value-preserving, and that a millimetre offset maps
to the matching fraction of the face for every supported standard.

## 3. Positive X

`+x` is to the right of target centre in the drawn face, because
`RmsTargetView` computes `cx + n.x * faceRadius` with no negation, and
`normalise()` does not touch the sign of x.

Whether the acquisition hardware calls "right" positive is a hardware fact, not
a software one. See §6.

## 4. Positive Y — and why RMS flips it

Telemetry `y` is **positive upwards**. Screen `y` is positive downwards. RMS
flips once, in `TargetGeometry::normalise()`, and every renderer downstream
consumes an already-flipped value.

This is not a guess. Four renderers in the shared foundation independently use
the same convention:

| File | Line |
|---|---|
| `IssfTargetCanvas.qml` | `var py = cy - scale * s.y` |
| `FinalsReportTarget.qml` | same form |
| `ShotTargetCanvas.qml` | same form |
| `HeatMapCanvas.qml` | same form |

`cy - scale * y` means a **larger** y produces a **smaller** screen y, i.e.
higher on the screen. RMS follows the family rather than inventing a second
convention, and `TargetSpec::yAxisUp` exists so the answer is one line to change
if the field test contradicts it.

## 5. What RMS does NOT do with these numbers

- It does not score them. There is no function anywhere in `src/rms` or
  `rms/qml` that takes a coordinate and returns a value; see the scan recorded
  in `docs/test/rms-target-geometry-qualification.md`.
- It does not adjust them to agree with the score. If the authoritative score
  and the coordinate disagree, both are shown as received and the disagreement
  is the finding.
- It does not fabricate them. A shot the node accepted but RMS never received
  is counted and stated, never interpolated.

## 6. What is NOT verified

**The physical sensor sign convention.**

Everything above is a *software* contract, established by reading the code that
carries the value. It proves that the number the node accepted is the number RMS
draws. It does **not** prove that a pellet striking physically high produces a
positive `y` at the acquisition layer, because no shot was fired.

That is one measurement, and it is written up as a procedure in
`docs/test/rms-physical-shot-registration-checklist.md`. Until it is performed:

```
PHYSICAL X/Y ORIENTATION: NOT PHYSICALLY VERIFIED
```

If the field test shows the axis inverted, the fix is `yAxisUp = false` in the
affected `TargetSpec` — RMS, not the node, and one line.

## 7. Scaled-distance factor — a node concern, not RMS's

`calculateShootingSocre()` divides the incoming coordinate by
`APPSETTINGS.getMatch_meter() / 10` before scoring, so a 10 m target shot at a
different physical distance still scores correctly. That scaling happens
**inside the node, before the score is decided**, and the coordinate the node
stores and transmits is the unscaled acquisition value.

RMS therefore draws the raw coordinate on the target standard the node reported,
and must not apply a distance factor of its own. Doing so would be a second
transform on a value that has already been dealt with.
