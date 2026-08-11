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
    // ISSF MATCH RIFLE, muzzle right. Authoring grid 126x30.
    //
    // Traced from docs/ui/issf-match-rifle-reference.png, which is the visual
    // authority for this silhouette. The reference was measured column by
    // column and every landmark below is a real coordinate off that image,
    // rescaled 1212x290 px -> 126x30 grid units (aspect preserved to 0.5%).
    //
    // Three properties of the reference are what make it read as a TARGET
    // rifle rather than a service weapon, and each one is load-bearing here:
    //
    //  1. CURVES, not facets. The previous outline was straight segments end
    //     to end, which is what made it angular and chunky. Every organic edge
    //     below - the butt underside, the grip, the buttplate hook, the forend
    //     taper - is a cubic. Only the machined parts (receiver, barrel,
    //     sights) are straight, because on the real rifle they are.
    //
    //  2. The CHEEK PIECE and BUTTPLATE are SEPARATE SUBPATHS with real air
    //     around them. Drawn as part of the stock outline they disappear into
    //     a generic rifle profile. Here the cheek piece floats above the comb
    //     on two posts, and the buttplate stands off the butt on two prongs
    //     and hooks well below the stock line. They are the two features an
    //     onlooker actually uses to identify a match rifle.
    //
    //  3. A SLENDER BARREL. Measured off the rendered path, not off these
    //     coordinates: exposed barrel (forend nose -> muzzle) is 34.5 of 126
    //     units = 27.4% of overall length. The plain section is 1.95 units
    //     across - 17:1 over the exposed length - after the production-size
    //     pass thickened it from the 1.6 it was traced at, because 1.6 units
    //     is 1.17 device px at the 92 px the left pane draws and the barrel
    //     rendered as a grey line rather than a barrel. Short, thick barrels
    //     are the single strongest military cue and the reference has none.
    //     See UI-DEC-013 and UI-DEC-014.
    //
    // Supporting cues, left to right: hooked buttplate below the stock line;
    // floating cheek piece; thumb-notch behind the grip; trigger guard (an
    // odd-even hole); tall rear aperture sight with its diopter eyepiece
    // standing back over the action; free-floating barrel with an air gap
    // above the forend; clean forend tapering to a blunt nose; raised front
    // sight tunnel on a post - never a muzzle device.
    //
    // Fill rule is odd-even (VIcon's default), so the separate pieces must
    // never OVERLAP the main outline or they cancel to holes. They do not:
    // the gaps the design calls for are what guarantees it. The one genuine
    // hole - the trigger guard - is a nested subpath.
    readonly property string riflePath:
        // ── main body: stock, action, barrel, forend ──────────────────────
        "M9.2 16.6 C9.2 15.9 9.9 15.5 10.8 15.5 L16.2 15.5 " +
        // cheek-piece posts rising off the comb
        "L16.2 12.75 L18 12.75 L18 15.5 L19.3 15.5 L19.3 12.75 " +
        "L21.1 12.75 L21.1 15.5 L22.6 15.5 " +
        // comb sweeping up to the action
        "C25.8 14.4 29.8 11.4 35.5 9.9 L35.5 7.2 " +
        // rear aperture sight. The mount is deliberately NARROW and the sight
        // rail thin: a diopter that sits on a wide solid pedestal reads as a
        // telescopic sight, which is the one optic a match rifle never wears.
        "L34.4 7.2 L34.4 5.8 L37 5.8 L37 5.35 L35.4 5.35 L35.4 4.5 " +
        // diopter eyepiece: knurled cap, waisted neck, then the sight body
        "L33.3 4.5 L33.3 5.05 L31.4 5.05 L31.4 2.7 L33.3 2.7 L33.3 3.4 " +
        "L35.4 3.4 L35.4 1.8 L35.9 1.8 L35.9 0.2 L37.5 0.2 L37.5 1.8 " +
        "L42.6 1.8 L42.6 5.35 L40 5.35 L40 5.8 L43.5 5.8 L43.5 7.2 " +
        // receiver top, step down onto the barrel, barrel to the muzzle sleeve.
        // The barrel is 1.95 units deep, not the 1.6 it was traced at: at the
        // 92 px the left pane draws, 1.6 units is 1.17 device px and the barrel
        // rendered as a grey line. It is thickened UPWARD only so the
        // free-float gap below it is not narrowed. Still 17:1 over the exposed
        // length - slender, which UI-DEC-013 requires - but now solid.
        "L49.5 7.2 L51 7.55 L112.5 7.55 L112.5 7.3 L121.6 7.3 " +
        // front sight: post, then the tunnel overhanging it both sides
        "L121.6 5.5 L120 5.5 L120 3.9 C120 3.2 120.7 3.2 121.4 3.2 " +
        "L124.2 3.2 C124.9 3.2 125.6 3.2 125.6 3.9 L125.6 5.5 L124.6 5.5 " +
        "L124.6 7.3 L126 7.3 L126 9.95 L112.5 9.95 L112.5 9.5 L52 9.5 " +
        // fold under the barrel into the free-float gap, then the forend top
        "C51.4 9.6 51.2 10 51.6 10.35 C54 10.5 56 10.85 58.5 10.88 " +
        "L86 10.88 C87.5 10.86 88.4 10.4 89 10.1 " +
        // squared forend nose carrying the accessory rail, then the underside
        "L91.4 10.15 C91.9 10.2 92 10.6 92 11.2 L92 12.6 " +
        "C92 13.05 91.6 13.25 90.9 13.3 C90 13.35 89.4 13.32 88.9 13.28 " +
        "C88.2 13.35 87.7 13.7 86.9 14.05 C80 14.6 70 15.2 58 16.05 " +
        "L48 16.1 C46.4 16.1 45.4 17 44.2 18.2 L40.6 18.45 L36.9 18.75 " +
        // grip: front face, palm swell, and the rear lobe under the thumb
        "C36.3 20.5 35.8 23 35.6 25.4 C35.7 26.3 34.9 26.55 33.9 26.55 " +
        "L31.6 26.5 C29.6 26.4 27.9 25.2 27.15 22.9 " +
        "C26.9 22.1 27 21.1 27.6 20.55 C28.8 19.9 30.4 19 31.35 17.4 " +
        // thumb notch closes, then its roof runs back over the thumb shelf
        "C31.6 16.9 31.5 16.4 31.15 16.05 C30.8 15.4 30.2 14.85 29.4 14.82 " +
        "C28.6 14.85 27.9 15.05 27.2 15.4 " +
        // The long concave butt underside, in exactly TWO cubics. It was three,
        // and the extra join put a visible crease in the middle of the sweep -
        // the one place on the whole outline where a stock must look poured.
        // The single crease that IS in the reference, where the flat underside
        // breaks upward into the grip, is the join at 22.3 and is deliberate.
        "C25.6 16.15 23.6 18 22.3 20.5 " +
        "C19.8 20.85 14.2 23.9 10.2 23.6 " +
        "C9.6 23.5 9.25 23.15 9.25 22.5 L9.25 21.9 " +
        // two prongs carrying the buttplate rail
        "L6.5 21.9 L6.5 20.5 L9.25 20.35 L9.25 19.1 " +
        "L6.5 18.95 L6.5 17.55 L9.25 17.4 Z " +
        // ── adjustable cheek piece: a separate plate floating on its rail ──
        "M10.35 11.05 C10.4 10.6 10.9 10.2 11.6 9.85 " +
        "C12.5 9.3 13.6 8.6 14.6 8.2 C15.1 8.05 15.6 8.05 16.2 8.05 " +
        "L22.6 8.1 C24.2 8.2 25.6 8.5 26.6 8.85 " +
        "C27.9 9 28.9 9.15 29.7 9.28 C28.8 9.9 27.7 10.6 26.4 11.25 " +
        "L22.85 11.3 L22 12.45 L14.2 12.45 L13.45 11.35 L11.1 11.35 " +
        "C10.65 11.35 10.35 11.25 10.35 11.05 Z " +
        // ── hooked buttplate: separate, stood off the butt, hooked below ───
        // Straight parallel faces, because a bowed rear edge turns the plate
        // into a crescent and it stops reading as a flat adjustable plate.
        "M5.7 14.4 L5.7 26.9 C5.6 28.3 4.6 29.4 3.05 29.8 " +
        "C2.1 29.98 1.05 29.95 0.2 29.65 C0.6 29 1.25 28.45 1.95 28.05 " +
        "C2.45 27.75 2.75 27.3 2.8 26.5 L2.8 16 " +
        "C2.75 15.7 2.55 15.55 2.2 15.5 L1.9 13.4 L3.35 13.85 Z " +
        // ── holes: trigger guard ───────────────────────────────────────────
        "M37.4 15.6 C37.4 15.05 37.8 14.85 38.4 14.85 L42.3 14.85 " +
        "C42.9 14.85 43.25 15.15 43.25 15.75 L43.25 17.35 " +
        "C43.25 17.85 42.9 18.05 42.3 18.05 L38.4 18.05 " +
        "C37.8 18.05 37.4 17.75 37.4 17.15 Z"
        // The two forend ventilation slots that used to close this path are
        // GONE. They were 0.9 units deep, which is 0.66 device px at the size
        // the left pane draws: they could never resolve, and all they
        // contributed was a band of grey down the middle of the forend. The
        // forend now reads as one clean competition profile. They are not an
        // identifying feature of a match rifle; the buttplate, cheek piece,
        // diopter, front tunnel and barrel are, and those are all kept.

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
    // The rifle box is wider and shallower than the pistol's because the
    // reference silhouette is 4.2:1. It starts further left than the old 96x28
    // rifle did: the butt now carries the buttplate hook and the muzzle now
    // carries a real barrel, so the drawing needs the extra length. The butt
    // overlapping the ring motif is deliberate and matches the reference.
    VIcon {
        x: (art.isPistol ? 52 : 34) * art.sx
        y: (art.isPistol ? 12 : 15) * art.sy
        width:  (art.isPistol ? 46 : 84) * art.sx
        height: width * ((art.isPistol ? 32 : 30) / (art.isPistol ? 48 : 126))
        viewBoxW: art.isPistol ? 48 : 126
        viewBoxH: art.isPistol ? 32 : 30
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
