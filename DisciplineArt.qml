import QtQuick 2.15
import QtQuick.Shapes

// ─────────────────────────────────────────────────────────────────────────────
// Discipline identification artwork.
//
// One stylised plate per ISSF discipline, drawn as vectors on a 120x64 grid so
// it is crisp at any size and never blurs or stretches the way a raster asset
// does. Deliberately IDENTIFICATION artwork, not a scoring surface: the rings
// here are a visual motif at fixed decorative radii and encode NO ISSF ring
// dimensions. Scoring geometry lives in CenterPane.qml and is
// separately verified (docs/release/scoring-geometry-verification.md).
//
// One visual language across all four so the set reads as a family:
//   · a soft ring motif on the left, weight increasing with distance
//   · the equipment silhouette, muzzle right, sharing the pistol/rifle paths
//     already used by the left pane
//   · a position glyph beneath, which is what actually distinguishes
//     Prone from 3 Positions
//   · brand accent on the focal element, muted steel for the supporting marks
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: art

    // "AR10" | "AP10" | "PRONE50" | "3P50"
    property string discipline: "AR10"
    property color accent:  "#e8003d"
    property color ink:     "#ffffff"
    property color muted:   "#7d8794"

    implicitWidth: 120
    implicitHeight: 64

    readonly property real sx: width  / 120
    readonly property real sy: height / 64

    readonly property bool isPistol: discipline === "AP10"
    readonly property bool is50m:    discipline === "PRONE50" || discipline === "3P50"
    readonly property bool is3P:     discipline === "3P50"

    // Shared equipment silhouettes (same authoring grids as LeftPanel).
    readonly property string pistolPath:
        "M8 11 L8 9 L12 9 L12 11 L20 11 L20 10 L42 10 L42 8 L44 8 L44 10 " +
        "L45 10 L45 12.5 L44 12.5 L29 12.5 L29 14 L43 14 L43 17 L29 17 " +
        "L27 17 L27 20 L24 20 L24 17 L21 17 L20 20 L21 21 L20 23 L21 24 " +
        "L20 26 L19 29 L19 31 L6 31 L5 29 L7 17 L8 11 Z"
    // ISSF MATCH RIFLE, muzzle right. Authoring grid 96x28.
    //
    // The previous silhouette read as a military weapon: straight slab comb,
    // blocky receiver, a squared muzzle device. Those are exactly the cues a
    // target rifle does NOT have, and on a sport-shooting product they were
    // the wrong signal entirely.
    //
    // A match rifle is recognised by a different set of features, and this
    // outline is built from them, left to right:
    //   · deep hooked BUTTPLATE dropping below the stock line
    //   · raised, stepped ADJUSTABLE CHEEK PIECE above the comb
    //   · THUMBHOLE through the grip (the second subpath - an odd-even hole)
    //   · tall REAR APERTURE SIGHT standing well back over the action
    //   · slim, untapered BARREL with no handguard mass around it
    //   · HAND STOP on the accessory rail under the forend
    //   · FRONT SIGHT TUNNEL at the muzzle - a raised ring housing, NOT a
    //     flash hider or suppressor profile
    //
    // Kept as a flat one-colour silhouette so it stays legible at the ~132 px
    // tile width and matches the pistol plate's weight.
    readonly property string riflePath:
        "M1 7 L4 7 L4 10 L9 10 L11 6 L25 6 L27 10 L38 10 L40 8 L43 8 L43 3 " +
        "L49 3 L49 8 L53 8 L55 10 L84 10 L84 5 L91 5 L91 10 L95 10 L95 13 " +
        "L70 13 L70 16 L66 16 L66 19 L60 19 L60 16 L44 16 L42 20 L40 20 " +
        "L40 22 L34 22 L34 20 L32 20 L30 24 L24 25 L20 24 L5 23 L5 26 L1 26 Z " +
        "M31 13 L37 13 L38 16 L36 19 L32 19 L30 16 Z"

    // Position glyphs — the real differentiator between the two 50 m events.
    // 24x16 grid: a simplified athlete seen from the side, muzzle right.
    readonly property string proneGlyph:
        "M2 13 L22 13 M4 12 L7 9 L11 9 L13 11 L20 11 M7 9 L6 12"
    readonly property string kneelGlyph:
        "M3 15 L21 15 M8 15 L8 9 L11 6 L14 6 L16 8 L20 8 M8 12 L5 15"
    readonly property string standGlyph:
        "M4 16 L20 16 M12 16 L12 7 M12 7 L10 4 L14 4 M12 9 L18 8"

    // ── ring motif ───────────────────────────────────────────────────────
    // Decorative only. Heavier and tighter at 50 m, open at 10 m — a visual
    // cue for distance, not a ring specification.
    Item {
        id: rings
        x: 6 * art.sx; y: 10 * art.sy
        width: 44 * art.sx; height: 44 * art.sy

        Repeater {
            model: art.is50m ? 4 : 3
            Rectangle {
                property real f: 1 - index * (art.is50m ? 0.22 : 0.28)
                width: rings.width * f
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "transparent"
                border.color: index === 0 ? art.muted : Qt.rgba(1, 1, 1, 0.18 + index * 0.1)
                border.width: Math.max(1, (index === 0 ? 1.2 : 1) * art.sx * 1.1)
            }
        }
        // Centre — the only brand-accent mark in the ring motif.
        Rectangle {
            anchors.centerIn: parent
            width: rings.width * (art.is50m ? 0.16 : 0.12)
            height: width; radius: width / 2
            color: art.accent
        }
    }

    // ── equipment silhouette ─────────────────────────────────────────────
    VIcon {
        x: (art.isPistol ? 52 : 40) * art.sx
        y: (art.isPistol ? 12 : 14) * art.sy
        width:  (art.isPistol ? 46 : 74) * art.sx
        height: width * ((art.isPistol ? 32 : 28) / (art.isPistol ? 48 : 96))
        viewBoxW: art.isPistol ? 48 : 96
        viewBoxH: art.isPistol ? 32 : 28
        pathData: art.isPistol ? art.pistolPath : art.riflePath
        color: art.ink
        filled: true
        strokeWidth: 0
    }

    // ── position glyphs ──────────────────────────────────────────────────
    // 10 m events carry none: the discipline is already fully identified by
    // the equipment. The 50 m pair is distinguished ONLY by position, so this
    // row is what makes Prone and 3 Positions visually distinct at a glance.
    Row {
        visible: art.is50m
        x: 52 * art.sx
        y: 44 * art.sy
        spacing: 5 * art.sx

        Repeater {
            model: art.is3P ? [art.kneelGlyph, art.proneGlyph, art.standGlyph]
                            : [art.proneGlyph]
            VIcon {
                width: (art.is3P ? 19 : 30) * art.sx
                height: width * (16 / 24)
                viewBoxW: 24; viewBoxH: 16
                pathData: modelData
                color: index === 1 && art.is3P ? art.accent : art.muted
                strokeWidth: 1.8
                filled: false
            }
        }
    }
}
