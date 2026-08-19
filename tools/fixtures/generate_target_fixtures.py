# -*- coding: utf-8 -*-
"""Generate CORRELATED target-display fixtures for RMS.

WHY THIS EXISTS
---------------
RMS must never turn a coordinate into a score. But a display whose demo data
draws coordinates and scores from independent random sequences produces
screenshots that look wrong even when the renderer is perfect - an 8.2 printed
beside a hole in the 9 ring proves nothing except that the demo was incoherent.

So the correlation is done HERE, outside the RMS runtime, once, and frozen into
a JSON file. RMS consumes that file as OPAQUE AUTHORITATIVE INPUT: it reads an
x, a y and a score, and it does not know or care that they agree.

Production RMS does not link, import or execute any of this.

THE SCORING RULE REPRODUCED HERE
--------------------------------
Read from the trusted engine, `CenterPane.qml::calculateShootingSocre()`, which
carries one formula per discipline. All four have the same shape:

    score = 9 + (ringStepRadius + tenRingRadius + pelletRadius - r) / ringStepRadius

which is ISSF outward-gauge decimal scoring: a shot scores exactly k when the
PELLET EDGE touches the k-ring edge, and decimals interpolate linearly across a
ring band.

Every constant below was checked against BOTH the trusted engine and the
official rulebook (ISSF Rule Book 2026, EDITION 2025 Second Print 07/2026,
rule 6.3.4 for faces, 7.4.6 / 8.4.4 for ammunition). They agree; see
docs/architecture/rms-target-geometry-source-register.md.

    python tools/fixtures/generate_target_fixtures.py
"""
import json
import math
import os

# targetStandardId -> (tenRingRadiusMm, ringStepRadiusMm, projectileDiameterMm)
# Radii, derived from the official DIAMETERS, written out so the halving is
# visible rather than assumed.
STANDARDS = {
    "issf.10m.air-rifle":  (0.5 / 2,   5.0 / 2,  4.5),
    "issf.10m.air-pistol": (11.5 / 2, 16.0 / 2,  4.5),
    "issf.50m.rifle":      (10.4 / 2, 16.0 / 2,  5.6),
    "issf.50m.pistol":     (50.0 / 2, 50.0 / 2,  5.6),
}

# ISSF decimal scoring tops out at 10.9; a shot off the card scores 0.
MAX_DECIMAL = 10.9


def score_for(standard, x_mm, y_mm):
    """The trusted engine's formula, applied outside RMS."""
    ten_r, step_r, projectile = STANDARDS[standard]
    pellet_r = projectile / 2.0
    r = math.hypot(x_mm, y_mm)
    s = 9.0 + ((step_r + ten_r + pellet_r) - r) / step_r
    s = min(s, MAX_DECIMAL)
    return round(max(s, 0.0), 1)


def radius_for_score(standard, score):
    """The inverse, used only to PLACE a fixture at a chosen ring boundary.

    This is fixture authoring, not scoring: it answers "where is the 9 ring
    boundary" so a test can put a shot exactly there.
    """
    ten_r, step_r, projectile = STANDARDS[standard]
    return (step_r + ten_r + projectile / 2.0) - (score - 9.0) * step_r


def build(standard):
    ten_r, step_r, projectile = STANDARDS[standard]
    face_r = ten_r + 6.0 * step_r          # the 4 ring, which RMS draws to
    shots = []

    def add(x, y, note):
        shots.append({
            "x": round(x, 3),
            "y": round(y, 3),
            "score": score_for(standard, x, y),
            "note": note,
        })

    # § centre
    add(0.0, 0.0, "dead centre")

    # § axes - both directions, at the 10, 9 and 8 boundaries
    for value in (10.0, 9.0, 8.0):
        r = radius_for_score(standard, value)
        add(r, 0.0, "+x on the %d boundary" % value)
        add(-r, 0.0, "-x on the %d boundary" % value)
        add(0.0, r, "+y on the %d boundary" % value)
        add(0.0, -r, "-y on the %d boundary" % value)

    # § diagonals at one equal radius - four shots, one distance
    r = radius_for_score(standard, 9.0)
    d = r / math.sqrt(2.0)
    for sx, sy, name in ((1, 1, "+x+y"), (1, -1, "+x-y"),
                         (-1, 1, "-x+y"), (-1, -1, "-x-y")):
        add(sx * d, sy * d, "diagonal %s at the 9 radius" % name)

    # § a plausible group, so a screenshot looks like shooting rather than a
    #   test pattern. Deterministic, seeded by nothing - these are literals.
    spread = step_r * 0.55
    for i, (fx, fy) in enumerate([(0.35, 0.62), (-0.48, 0.21), (0.12, -0.71),
                                  (0.66, -0.14), (-0.27, -0.55), (-0.61, 0.44),
                                  (0.05, 0.33), (0.44, 0.05), (-0.13, 0.08),
                                  (0.22, -0.29)]):
        add(fx * spread * 2.0, fy * spread * 2.0, "group shot %d" % (i + 1))

    # § off the printed face - a genuine cross-fire, kept as a true coordinate
    add(face_r * 1.18, face_r * 0.35, "off the drawn face")

    return {
        "targetStandardId": standard,
        "tenRingRadiusMm": ten_r,
        "ringStepRadiusMm": step_r,
        "projectileDiameterMm": projectile,
        "faceRadiusMm": round(face_r, 3),
        "shots": shots,
    }


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.dirname(os.path.dirname(here))
    out = os.path.join(repo, "tests", "rms", "fixtures", "target-shot-fixtures.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)

    doc = {
        "_comment": [
            "CORRELATED FIXTURE DATA - TEST/DEMO AUTHORITATIVE INPUT.",
            "Scores are supplied, not computed by RMS. Generated by",
            "tools/fixtures/generate_target_fixtures.py from the trusted",
            "engine's formula and the official ISSF Rule Book 2026 geometry.",
            "RMS reads x, y and score and correlates nothing.",
        ],
        "ruleSource": ("ISSF Rule Book 2026, EDITION 2025 (Second Print 07/2026), "
                       "effective 1 July 2026; rule 6.3.4, 7.4.6, 8.4.4"),
        "scoringSource": "CenterPane.qml::calculateShootingSocre() (node, trusted)",
        "standards": [build(s) for s in sorted(STANDARDS)],
    }
    with open(out, "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")
    total = sum(len(s["shots"]) for s in doc["standards"])
    print("wrote %s" % out)

    # The same data, frozen as a compiled constant. The JSON is for humans to
    # review; the header is what the demo and the tests actually consume, so
    # neither depends on finding a file at runtime and both see identical
    # numbers.
    hdr = os.path.join(repo, "src", "rms", "dev", "TargetShotFixtures.h")
    lines = [
        "#ifndef TA_RMS_DEV_TARGETSHOTFIXTURES_H",
        "#define TA_RMS_DEV_TARGETSHOTFIXTURES_H",
        "",
        "// GENERATED FILE - do not edit by hand.",
        "//   python tools/fixtures/generate_target_fixtures.py",
        "//",
        "// CORRELATED FIXTURE DATA - TEST/DEMO AUTHORITATIVE INPUT.",
        "// Each shot carries a coordinate AND the score that belongs to it. The",
        "// correlation was done by the generator, outside the RMS runtime, from",
        "// the trusted engine's formula and the official ISSF Rule Book 2026",
        "// geometry. RMS reads these as opaque authoritative values and derives",
        "// nothing from them: there is no coordinate-to-score function in this",
        "// product and this header does not introduce one.",
        "//",
        "// Rule source: " + doc["ruleSource"],
        "// Scoring source: " + doc["scoringSource"],
        "",
        "namespace ta { namespace rms { namespace dev {",
        "",
        "struct FixtureShot { double xMm; double yMm; double score; };",
        "struct FixtureTarget {",
        "    const char* targetStandardId;",
        "    const FixtureShot* shots;",
        "    int shotCount;",
        "};",
        "",
    ]
    for st in doc["standards"]:
        sym = st["targetStandardId"].replace(".", "_").replace("-", "_")
        lines.append("// %s - face radius %.3f mm, projectile %.1f mm"
                     % (st["targetStandardId"], st["faceRadiusMm"],
                        st["projectileDiameterMm"]))
        lines.append("inline const FixtureShot kFixtures_%s[] = {" % sym)
        for sh in st["shots"]:
            lines.append("    { %9.3f, %9.3f, %4.1f },   // %s"
                         % (sh["x"], sh["y"], sh["score"], sh["note"]))
        lines.append("};")
        lines.append("")
    lines.append("inline const FixtureTarget kFixtureTargets[] = {")
    for st in doc["standards"]:
        sym = st["targetStandardId"].replace(".", "_").replace("-", "_")
        lines.append('    { "%s", kFixtures_%s, %d },'
                     % (st["targetStandardId"], sym, len(st["shots"])))
    lines.append("};")
    lines.append("inline constexpr int kFixtureTargetCount = %d;" % len(doc["standards"]))
    lines.append("")
    lines.append("} } }  // namespace ta::rms::dev")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    with open(hdr, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("wrote %s" % hdr)
    print("%d standards, %d fixture shots" % (len(doc["standards"]), total))


if __name__ == "__main__":
    main()
