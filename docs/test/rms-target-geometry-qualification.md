# RMS target geometry + shot registration — qualification

Milestone 4.6. What was checked, what was found, and what remains unproven.

Authority: **ISSF Rule Book 2026, EDITION 2025 (Second Print 07/2026), effective
1 July 2026**, rule 6.3.4 (faces) and rules 7.4.6 / 8.4.4 (ammunition). Every
dimension is cited in
[`rms-target-geometry-source-register.md`](../architecture/rms-target-geometry-source-register.md).
The coordinate path is traced in
[`rms-coordinate-contract.md`](../architecture/rms-coordinate-contract.md).

---

## 1. The question that started this

A shot displayed `8.2` while its plotted position looked like it lay in a
different ring. Three things could have been wrong: the geometry, the
coordinate mapping, or the demo data. All three were checked separately.

**Answer: the geometry and the mapping were sound for three of the four
targets; the fourth was drawing the wrong face entirely; and the demo data was
incoherent by construction.** Details in §6.

## 2. Dimension report — generated from the implementation

Produced by reading the product's own table and comparing it against the
rulebook values typed independently into `tst_target_geometry.cpp`. Not
measured from a screenshot.

| standard | ring | official Ø | implemented Ø | diff | fraction of drawn face |
|---|---|---|---|---|---|
| issf.10m.air-rifle | 10 | 0.5 | 0.5 | 0.000 | 0.0164 |
| issf.10m.air-rifle | 9 | 5.5 | 5.5 | 0.000 | 0.1803 |
| issf.10m.air-rifle | 4 | 30.5 | 30.5 | 0.000 | 1.0000 |
| issf.10m.air-rifle | black | 30.5 | 30.5 | 0.000 | 1.0000 |
| issf.10m.air-rifle | projectile | 4.5 | 4.5 | 0.000 | 0.1475 |
| issf.10m.air-pistol | 10 | 11.5 | 11.5 | 0.000 | 0.1070 |
| issf.10m.air-pistol | 9 | 27.5 | 27.5 | 0.000 | 0.2558 |
| issf.10m.air-pistol | 4 | 107.5 | 107.5 | 0.000 | 1.0000 |
| issf.10m.air-pistol | black | 59.5 | 59.5 | 0.000 | 0.5535 |
| issf.10m.air-pistol | projectile | 4.5 | 4.5 | 0.000 | 0.0419 |
| issf.50m.rifle | 10 | 10.4 | 10.4 | 0.000 | 0.0977 |
| issf.50m.rifle | 9 | 26.4 | 26.4 | 0.000 | 0.2481 |
| issf.50m.rifle | 4 | 106.4 | 106.4 | 0.000 | 1.0000 |
| issf.50m.rifle | black | 112.4 | 112.4 | 0.000 | **1.0564** |
| issf.50m.rifle | projectile | 5.6 | 5.6 | 0.000 | 0.0526 |
| issf.50m.pistol | 10 | 50.0 | 50.0 | 0.000 | 0.1429 |
| issf.50m.pistol | 9 | 100.0 | 100.0 | 0.000 | 0.2857 |
| issf.50m.pistol | 4 | 350.0 | 350.0 | 0.000 | 1.0000 |
| issf.50m.pistol | black | 200.0 | 200.0 | 0.000 | 0.5714 |
| issf.50m.pistol | projectile | 5.6 | 5.6 | 0.000 | 0.0160 |

The 50 m rifle black at **1.0564 of the face** is correct, not an error: rule
6.3.4.2 puts the black at 112.4 mm diameter, past the 4-ring the display crops
to. The paint is clamped to the face; the number is not.

Ratio check — rendered 9-ring/10-ring against official, which catches a wrong
step even if both ends were wrong together:

| standard | rendered | official | difference |
|---|---|---|---|
| issf.10m.air-rifle | 11.000000 | 11.000000 | 0 |
| issf.10m.air-pistol | 2.391304 | 2.391304 | 0 |
| issf.50m.rifle | 2.538462 | 2.538462 | 0 |
| issf.50m.pistol | 2.000000 | 2.000000 | 0 |

## 3. Per-target verdict

| target | ring geometry | black | projectile | verdict |
|---|---|---|---|---|
| 10 m Air Rifle | correct before this milestone | correct | **was arbitrary, now 4.5 mm** | **PASS** |
| 10 m Air Pistol | correct before this milestone | correct | **was arbitrary, now 4.5 mm** | **PASS** |
| 50 m Rifle | correct before this milestone | **was 45.2 mm, now 56.2 mm** | **was arbitrary, now 5.6 mm** | **PASS** |
| 50 m Pistol | **was the RIFLE face, now correct** | **was wrong, now 100 mm** | **was arbitrary, now 5.6 mm** | **PASS** |

## 4. Automated test matrix

`tests/rms/tst_target_geometry.cpp`, run by `rms_tests.exe`.

| § | check | result |
|---|---|---|
| A | geometry definitions exist, and only the four qualified ones | PASS |
| B | ring dimensions match the source register | PASS |
| C | diameter/radius conversion, asserted directly | PASS |
| D | centre maps to the exact centre, in fraction and in pixels | PASS |
| E/F | +x right, −x left, symmetric | PASS |
| G/H | +y **up**, −y down — the flip happens once | PASS |
| I | four diagonals in the four correct quadrants | PASS |
| J | equal radial distance stays equal in all four | PASS |
| K | one scale for x and y — the face stays round | PASS |
| L | scale invariance at 160/300/600/1000 px | PASS |
| M | card and full-screen positions are identical by construction | PASS |
| N | projectile radius follows the physical calibre | PASS |
| O | unknown standard places nothing and draws no face | PASS |
| P | off-face: true radius preserved, only the drawn point moves | PASS |
| Q | correlated fixtures — every score matches its coordinate | PASS |
| R | no coordinate→score function in the RMS tree | PASS (§5) |
| S | protocol score stays opaque and authoritative | PASS |
| T | competition state cannot reach geometry | PASS |

Ring-boundary registration is checked for rings 4–10 on **both** axes for all
four standards: a shot at the ring radius normalises to exactly the fraction
the renderer draws that ring at.

Harness total after this milestone: **1129 checks, 0 failures** (was 924).

## 5. No-scoring scan

Run over `src/rms`, `rms/qml`, `rms/main.cpp` after implementation:

- `calculateScore|scoreFrom|ringScore|decimalScore|scoreForRadius|ringValueFor|toScore|scoreOf` — **no hits**
- geometry functions (`hypot`, `sqrt`) appearing near a score — **no hits**
- every `authoritativeScore =` assignment, reviewed by hand:
  - `RmsProtocol.cpp` — decoded from the wire
  - `SimulatedRange.cpp` ×2 — the simulated NODE supplying its own score, from
    a correlated fixture or the chaos sequence

```
RMS COORDINATE→SCORE IMPLEMENTATION: NONE
```

Every radius in the RMS tree is used to **paint a ring**, **decide whether a
shot is on the face**, or **size a hole**. None produces a value.

## 6. Root cause of the reported mismatch

Three contributing causes, in order of importance:

**1. The demo data was uncorrelated (the dominant cause).**
`SimulatedRange::emitShot()` drew `rawXMm`, `rawYMm` and `authoritativeScore`
from three separate draws of the same LCG. A shot's coordinate and its score
had no relationship whatever. Any screenshot from that demo was guaranteed to
show scores that disagreed with positions, no matter how correct the renderer
was. Fixed by playing **correlated fixtures** in the field-test scenario; the
uncorrelated generator is kept for the network-chaos tests that assert its exact
numbers.

**2. The projectile was not drawn (the reason it looked wrong even so).**
The marker was a fixed 3% of the face radius — an arbitrary dot with no
relation to the calibre. **ISSF scores by the OUTWARD GAUGE: the pellet's
edge, not its centre.** On a 10 m air rifle face this is not a subtlety:

- ten ring radius: **0.25 mm**
- pellet radius: **2.25 mm**
- a shot scoring exactly **10.0** has its centre **2.5 mm** from the middle —
  ten times the ten-ring radius, and outside the ten ring altogether.

So on a correct display, a 10.0 on air rifle *looks* well out of the ten ring
if you only draw the centre. Drawing the true 4.5 mm hole makes the score
legible: its inner edge touches the ring the score names. This alone accounts
for most of the "that can't be right" reaction.

**3. One target really was wrong.** `issf.50m.pistol` carried a copy of the
50 m rifle row, so every 50 m pistol shot was plotted at about **4.8×** its true
radius from centre. That is a genuine renderer defect and it is fixed.

## 7. Before and after

`docs/img/rms_geometry_50m_pistol_BEFORE.png` — lane 6, 50 m Pistol, captured
during milestone 4.5, drawn on a 50 m **rifle** face (53.2 mm face radius).

`docs/img/rms_geometry_50m_pistol.png` — the same lane after this milestone:
175 mm face radius, black to the 7 ring, 5.6 mm holes, the dev overlay stating
its own scale.

Root cause: the geometry table was mirrored from `IssfTargetCanvas.qml`, whose
`rifle50` entry is the **default** branch for anything that is not 10 m. There
is no pistol entry there to mirror. The table is now taken from the rulebook.

The test that would have failed before:
`"50 m PISTOL is not the 50 m RIFLE face — the ten rings differ"`.

## 8. Visual evidence

All at 1366×768 in the real application, demo mode, with the development
geometry overlay on (`TECHAIM_RMS_GEOMETRY_OVERLAY=1`) so each image states its
own scale.

| image | what it proves |
|---|---|
| `rms_geometry_10m_rifle.png` | 4.5 mm pellet at 15% of the face — the holes dominate, as they do on a real card |
| `rms_geometry_10m_pistol.png` | face 53.75 mm, 3.656 px/mm, projectile 16.5 px; last shot x = −16.00 mm scoring 9.0, its inner edge exactly on the 13.75 mm 9-ring |
| `rms_geometry_50m_rifle.png` | whole face black (official black exceeds the cropped face), 5.6 mm holes |
| `rms_geometry_50m_pistol.png` | face 175.00 mm, black to the 7 ring, last shot y = **+27.80 mm drawn ABOVE centre**, scoring 10.0 with its lower edge on the 25 mm ten ring |

The air pistol and 50 m pistol images are the two worth reading closely: each
shows a shot whose **pellet edge sits exactly on the ring line its score
names**, with the millimetres printed beside it.

## 9. Resolutions

- **1366×768** — verified in the real application; all four captures above.
- **1920×1080 / 2560×1440** — this machine's desktop is smaller, so a window
  cannot reach those sizes. Unchanged from milestone 4.5: layout can be
  rendered offscreen but glyphs do not rasterise there, so it is not visual
  evidence.

**The geometry tests are resolution-independent** and pass without any monitor:
they assert normalised relationships, and scale invariance is checked at four
sizes. So:

```
GEOMETRY PASS:        yes, at every size
VISUAL MONITOR PASS:  1366×768 only
```

## 10. What is NOT proven

- **PHYSICAL TARGET TEST: NOT TESTED.** No shot was fired. Everything here is a
  software contract plus a rulebook.
- **PHYSICAL X/Y ORIENTATION: NOT PHYSICALLY VERIFIED.** The +y-is-up
  convention is taken from four foundation renderers that agree with each
  other; no hardware confirmed it. Procedure:
  [`rms-physical-shot-registration-checklist.md`](rms-physical-shot-registration-checklist.md).
- **20-lane performance** was not run: the development simulator is capped at
  six lanes and raising the cap would disturb tests that assert its exact
  output. Six lanes × 30 markers were exercised. Structurally, the face is a
  `Canvas` repainted only when the standard or the size changes, and shot
  markers are bound `Item`s, so a new shot moves one marker rather than
  repainting a target; no scoring call occurs during render because none
  exists.

## 11. Foundation correction candidates — NOT changed here

Recorded for a separate foundation task, per the milestone instruction not to
modify the foundation from an RMS branch:

1. `IssfTargetCanvas.qml` has **no 50 m pistol face**; its `rifle50` entry is
   the default for everything that is not 10 m, so the node application draws a
   rifle face for 50 m pistol too.
2. `IssfTargetCanvas.qml` puts the 50 m rifle black at the 5-ring (45.2 mm);
   rule 6.3.4.2 says 112.4 mm diameter (56.2 mm radius).

Neither affects scoring.

## 12. Scoring engine — examined, not modified

`CenterPane.qml::calculateShootingSocre()` was read to establish the fixture
formula. All four of its branches were checked against the rulebook:

| discipline | `radOf10Ring` | official 10-ring radius | verdict |
|---|---|---|---|
| 10 m air rifle | 0.25 | 0.25 | correct |
| 10 m air pistol | 5.75 | 5.75 | correct |
| 50 m rifle | **5.2** | **5.2** | correct |
| 50 m pistol | 25 | 25.0 | correct |

Its ring steps (2.5 / 8 / 8 / 25 mm radius) and its projectile selector
(`rangeMeters == 10 ? 4.5 : 5.6`, rules 7.4.6 / 8.4.4) are also correct.

```
SCORING ENGINE DEFECT FOUND: NO
AUTHORITATIVE SCORING MODIFIED: NO
```

This also closes CLAUDE.md's long-standing open item: **50 m rifle
`radOf10Ring = 5.2` is a RADIUS and it is confirmed correct** by rule 6.3.4.2's
10.4 mm diameter. No physical calibration is needed for that number.
