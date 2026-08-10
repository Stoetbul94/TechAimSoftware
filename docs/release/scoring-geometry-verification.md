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

### ⏳ Requested: official target-face geometry

Needed in `docs/issf-rules/`, per discipline, from the ISSF rulebook:

- 10-ring diameter and ring-to-ring spacing for the 10 m air rifle, 10 m air
  pistol, 50 m rifle and 50 m pistol target faces
- the projectile diameters assumed for scoring
- whether the "touching counts" convention as implemented is the required one

Once supplied, the rules docs, these tests and the implementation are updated
**together**, and `radOf10Ring = 5.2` can move from *internally consistent* to
*confirmed*.

## Status

| Claim | Status |
|---|---|
| Formula internally consistent across all four disciplines | **VERIFIED (automated)** |
| Monotonic, correct ring hinges, clamp load-bearing | **VERIFIED (automated)** |
| Constants unchanged since this baseline | **GUARDED (automated)** |
| Constants match the ISSF rulebook | ⏳ **AWAITING OFFICIAL RULE** |
| 50 m scoring behaviour in the field | Internally consistent across the 2026-08-10 session; no suspicious result observed |

## Note on the test harness

The text assertions search the whole of `CenterPane.qml` rather than the
extracted function. `extractFunction()` brace-matches without stripping
comments, and this function contains commented-out closing braces (`//    }`),
so the extract terminates early. Each searched string occurs exactly once, so
the file-wide search is unambiguous. The extractor remains correct for
`traceStage()` and `triggerAutoZoom()`, which carry no such comments.
