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
    readonly property string riflePath:
        "M4 8 L11 8 L14 5 L23 5 L25 8 L31 8 L33 3 L37 3 L37 8 L46 9 L54 11 " +
        "L88 11 L88 8 L94 8 L94 15 L88 15 L88 13 L54 13 L50 14 L48 14 L48 22 " +
        "L45 22 L45 14 L41 14 L39 17 L37 17 L37 14 L34 14 L34 21 L28 21 L28 14 " +
        "L11 14 L8 15 L9 16 L8 17 L8 20 L4 20 Z"

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
