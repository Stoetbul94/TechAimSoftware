# Scoring geometry — offline verification

`CenterPane.qml::calculateShootingSocre()` is the most correctness-critical
function in the codebase and had **no test coverage at all** before this pass.
Verified offline on 2026-08-10. **No scoring behaviour was changed.**

---

## The shared formula

Every discipline uses one shape:

```
score = 9 + (ringSpacing + r10 + rPellet − radius) / ringSpacing
```

followed by a clamp to the ISSF decimal maximum:

```qml
if (calculatedSccore >= 11) calculatedSccore = 10.9
```

## Constants in the shipped source

| Discipline | Ring spacing | 10-ring radius | Projectile radius |
|---|---|---|---|
| 10 m Air Pistol | 8 mm | 5.75 mm | `bullet_diameter()/2` (2.25 mm) |
| 10 m Air Rifle | 2.5 mm | 0.25 mm | 2.25 mm |
| 50 m Pistol | 25 mm | 25 mm | 2.8 mm |
| 50 m Rifle | 8 mm | **5.2 mm** | 2.8 mm |

Each pair is now asserted against the real file, so a silent edit to a ring
size fails the harness.

## What is verified (24 checks, `tests/qml`)

Properties that must hold **whatever the constants are** — a rulebook cannot
change these, so they are safe to assert offline:

1. **A projectile edge touching the 10-ring scores exactly 10.0.** True for all
   four disciplines, to 1e-9. This is the ISSF "touching counts" convention and
   the hinge the whole formula turns on.
2. **One ring further out scores exactly 9.0.** Ring width and score step have
   not diverged.
3. **Score decreases strictly with radius** — swept 0…60 mm in 0.05 mm steps
   for every discipline. No plateau, no inversion, never rewards a worse shot.
4. **A dead-centre shot computes ≥ 11.0 in every discipline**, so the clamp is
   load-bearing rather than defensive decoration. Removing it would emit 11.0,
   which is not a legal ISSF score.
5. The clamp and its 10.9 target exist in the shipped source.

## What is NOT verified, and why

**No check asserts that a constant matches the ISSF rulebook.**

`docs/issf-rules/` is the maintained authority for discipline requirements, and
it **does not document target-face geometry for any discipline** — not ring
diameters, not ring spacing, not projectile diameters.
`docs/issf-rules/50m-rifle-prone.md` specifies decimal scoring and a 10.9
maximum, and says nothing about the target face.

Per `CLAUDE.md`, when the applicable rules file is incomplete the correct
action is to **request the missing official rule**, not to encode a value from
memory and call it verified. Writing `5.2` into a test because it looks right
would manufacture the confirmation the open item is asking for.

### ✅ Official authority obtained (2026-08-15)

**AUTHORITY: ISSF Rule Book 2026, Edition 2025, Second Print 07/2026,
effective 1 July 2026.** Rule sections: **6.3.2** Electronic Scoring Target
Requirements · **6.3.3** ISSF Target Standards · **6.3.4.2** 50 m Rifle Target ·
**6.3.4.3** 10 m Air Rifle Target · **6.3.4.6** 10 m Air Pistol Target · **7.4**
rifle ammunition · **8.4** pistol ammunition.

The constants were audited against those values rather than copied into them.
Ring spacing is derived: consecutive official ring DIAMETERS differ by a fixed
step, so the radius step the formula uses is half that.

| Discipline | Official | Source constant (`CenterPane.qml`) | Value | Result |
|---|---|---|---|---|
| 10 m Air Rifle | 10-ring ⌀ 0.5 mm → r 0.25 | `radOf10Ring` | 0.25 | **PASS** |
| 10 m Air Rifle | ⌀ step 5.0 mm → radius step 2.5 | `r2rDis` | 2.5 | **PASS** |
| 10 m Air Rifle | outer ring ⌀ 45.5 | `gameRatio` numerator | 45.5 | **PASS** |
| 10 m Air Pistol | 10-ring ⌀ 11.5 mm → r 5.75 | `radOf10Ring` | 5.75 | **PASS** |
| 10 m Air Pistol | ⌀ step 16.0 mm → radius step 8.0 | `r2rDis` | 8 | **PASS** |
| 10 m Air Pistol | outer ring ⌀ 155.5 | `gameRatio` numerator | 155.5 | **PASS** |
| 50 m Rifle (Prone and 3P) | 10-ring ⌀ 10.4 mm → r 5.2 | `radOf10Ring` | 5.2 | **PASS** |
| 50 m Rifle | ⌀ step 16.0 mm → radius step 8.0 | `r2rDis` | 8 | **PASS** |
| 50 m Rifle | outer ring ⌀ 154.4 | `gameRatio` numerator | 154.4 | **PASS** |
| All | decimal maximum 10.9 | `calculatedSccore >= 11 → 10.9` | 10.9 | **PASS** |

`radOf10Ring = 5.2` for 50 m is therefore **CONFIRMED**, not merely internally
consistent: it is half the official 10.4 mm 10-ring diameter.

**50 m Rifle 3 Positions shares the 50 m Rifle target**, so it is covered by the
same row and needs no separate constant.

### The edge-touch interpretation, stated explicitly

Every discipline uses one formula:

```
score = 9 + ( (r10 + spacing + rPellet) - d ) / spacing
```

where `d` is the shot centre's radial distance in mm. Setting `score = 10`
gives `d = r10 + rPellet`: a shot whose **projectile edge just touches the
10-ring line scores exactly 10.0**. The same algebra one ring out gives exactly
9.0. So the implementation encodes the convention that a projectile touching
the higher ring is awarded the higher ring.

That convention is **assumed, not proven** from the material to hand. The
rulebook sections above fix the ring dimensions, the decimal scoring areas and
the calibres; they do not, in the extract available here, state the EST edge
rule in the form the formula uses. Classified narrowly as **EST EDGE-
INTERPRETATION CONFIRMATION PENDING**. The geometry itself is not pending.

### ✅ FIXED (SCORING-CAL-001) — projectile diameter is now discipline-bound

`radOfPallet` is `APPSETTINGS.bullet_diameter()/2`, and `bullet_diameter()`
returns a single process-wide value read once at startup from
`config.ini [App_Settings] bullet_size`, defaulting to **5.6**. It is never
written per discipline. The **deployed `release/config.ini` does not contain
the key at all**, so a 10 m session scores with a 5.6 mm projectile instead of
the official 4.5 mm:

| Discipline | Official rPellet | As deployed | 10.0 boundary official | As deployed | Error |
|---|---|---|---|---|---|
| 10 m Air Rifle | 2.25 | 2.8 | d = 2.50 mm | d = 3.05 mm | **+0.22 score/shot** |
| 10 m Air Pistol | 2.25 | 2.8 | d = 8.00 mm | d = 8.55 mm | **+0.069 score/shot** |

50 m Rifle is unaffected: 5.6 mm is the correct calibre there, which is why the
default was chosen and why the defect is invisible at 50 m. The scoring
harness hardcodes the correct per-discipline radii, so it passes while the
shipped configuration does not — the tests and the deployment disagree.

**Fixed.** `AppSettings::projectileDiameterMm(rangeMeters)` is now the
authority — 10 m returns 4.5, 50 m returns 5.6 (ISSF Rule Book 2026 §7.4,
§8.4). Every `radOfPallet`, the shot-marker scale and the group-size
calculation in `CenterPane.qml` take it; the process-wide
`bullet_diameter()` no longer appears there. An explicitly configured
`bullet_size` still wins, so a deliberate non-ISSF calibre remains
configurable — it is simply no longer the silent default.

**Historical sessions are rescored, not migrated.** A saved `.tch` stores
`x_data`, `y_data`, `time` and `time_stamp` — **no score**. A reopened or
resumed qualification session is therefore rescored from coordinates, so a
10 m session saved before this fix will read LOWER (correctly) when reopened
after it. No migration was invented; the behaviour is recorded here.

## Status

| Claim | Status |
|---|---|
| Ring geometry matches the ISSF rulebook | **CONFIRMED** — ISSF Rule Book 2026, sections above |
| 50 m `radOf10Ring = 5.2` | **CONFIRMED** — half of the official 10.4 mm |
| Formula internally consistent across all four disciplines | **VERIFIED (automated)** |
| Monotonic, correct ring hinges, clamp load-bearing | **VERIFIED (automated)** |
| Constants unchanged since this baseline | **GUARDED (automated)** |
| EST edge-touch interpretation | ⏳ **INTERPRETATION PENDING** |
| Projectile diameter bound to discipline | **CONFIRMED** — SCORING-CAL-001, ISSF §7.4 / §8.4 |

## Note on the test harness

The text assertions search the whole of `CenterPane.qml` rather than the
extracted function. `extractFunction()` brace-matches without stripping
comments, and this function contains commented-out closing braces (`//    }`),
so the extract terminates early. Each searched string occurs exactly once, so
the file-wide search is unambiguous. The extractor remains correct for
`traceStage()` and `triggerAutoZoom()`, which carry no such comments.
