import QtQuick 2.15

// THE TARGET FACE. One renderer, used by the small overview card and the
// full-screen view alike.
//
// ═══ IT DRAWS; IT DOES NOT SCORE ═══════════════════════════════════════════
//
// Every score shown here arrived attached to its shot. Nothing in this file
// turns a coordinate into a value, and nothing may be added that does: the
// node is the only scoring authority in the product family.
//
// ═══ COORDINATES ARRIVE ALREADY NORMALISED ═════════════════════════════════
//
// `shots` carry x and y in face-relative units — centre (0,0), face edge at
// radius 1, y already flipped for the screen by TargetGeometry. QML does no
// millimetre arithmetic and does not know which way up the range is, so the
// same shot lands in the same place at every size.
//
// ═══ REDRAW DISCIPLINE ═════════════════════════════════════════════════════
//
// The canvas paints the FACE only, and repaints solely when the standard or
// the size changes. Shot markers are ordinary items positioned by binding, so
// a shot arriving on a twenty-lane range moves one marker rather than
// repainting twenty targets.
Item {
    id: view

    // The observed (or planned) standard. An unrecognised one draws a
    // placeholder — never a different target, which would misplace every
    // marker while looking entirely convincing.
    property string targetStandardId: ""
    // [{ x, y, score, sequence, last, offFace, innerTen, presentation }]
    property var shots: []
    property bool showRingNumbers: true
    property bool showLastShotLabel: true
    // A lane whose station is offline still shows its last known face, visibly
    // stale rather than blank.
    property bool stale: false

    // FIT THE WHOLE SCORING REGION, not just the cropped tile.
    //
    // The compact lane tile crops at the 4 ring on purpose - most of a match
    // happens near the middle and a tile that drew rings 1-3 would waste its
    // area. But the first physical test put two perfectly valid shots (3.4 and
    // 4.3) outside that crop, where they rendered as edge arrows and read as
    // errors. A detail view sets this true and gets the whole card.
    property bool fitToScoringRegion: false

    readonly property var spec: TARGETGEO.specFor(targetStandardId)
    readonly property bool supported: spec && spec.supported === true
    // How much bigger the scoring region is than the drawn face. Dividing the
    // face by this is the WHOLE fix: shot positions are already fractions of
    // the face, so shrinking the face inside the same widget moves every shot
    // and every ring correctly with no second mapping to keep in step.
    readonly property real scoringFactor:
        (fitToScoringRegion && spec && spec.scoringRadiusFraction !== undefined)
            ? spec.scoringRadiusFraction : 1.0
    readonly property real faceRadius:
        (Math.min(width, height) / 2 - 4) / scoringFactor
    readonly property real cx: width / 2
    readonly property real cy: height / 2
    // THE PROJECTILE, AT ITS TRUE PHYSICAL SIZE. Not a fixed pixel dot: a
    // 4.5 mm pellet is 15% of a 10 m air rifle face and 1.6% of a 50 m pistol
    // face, and one number cannot be right for both. `projectileRadiusFraction`
    // comes from the ammunition rules via TargetGeometry.
    readonly property real projectileRadius:
        (spec && spec.projectileRadiusFraction !== undefined)
            ? spec.projectileRadiusFraction * faceRadius : 0
    // Below this a hole stops being visible at all on a small lane tile. It is
    // a legibility floor, not a geometry claim.
    readonly property real minMarkerPx: 1.6

    Theme { id: theme }

    onTargetStandardIdChanged: face.requestPaint()
    onWidthChanged: face.requestPaint()
    onHeightChanged: face.requestPaint()

    // ── the printed face ────────────────────────────────────────────────
    Canvas {
        id: face
        anchors.fill: parent
        antialiasing: true
        visible: view.supported
        opacity: view.stale ? 0.45 : 1.0

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            if (!view.supported)
                return

            var R = view.faceRadius
            if (R <= 0)
                return
            var cx = view.cx, cy = view.cy
            var s = view.spec

            // cream face
            ctx.fillStyle = "#f4efe4"
            ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill()

            // Black aiming mark. On the 50 m rifle the official black (112.4 mm
            // diameter) is WIDER than the cropped face, so the paint is clamped
            // to the face while the geometry keeps the true value. Clamping the
            // brush is presentation; clamping the number would be a lie.
            ctx.fillStyle = "#141414"
            ctx.beginPath()
            ctx.arc(cx, cy, Math.min(1.0, s.blackRadiusFraction) * R, 0, 2 * Math.PI)
            ctx.fill()

            // The cropped face is drawn above. When fitting to the scoring
            // region, the rings OUTSIDE it are drawn first, on the paper the
            // card actually has, so a low shot lands on rings instead of on
            // nothing.
            if (view.fitToScoringRegion && s.scoringRings) {
                var outer = s.scoringRings
                var SR = R * view.scoringFactor
                // The card beyond the cropped face: same cream, so the face
                // and its surround read as one piece of paper.
                ctx.fillStyle = "#efe9dd"
                ctx.beginPath(); ctx.arc(cx, cy, SR, 0, 2 * Math.PI); ctx.fill()
                ctx.fillStyle = "#f4efe4"
                ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill()
                ctx.fillStyle = "#141414"
                ctx.beginPath()
                ctx.arc(cx, cy, Math.min(1.0, s.blackRadiusFraction) * R, 0, 2 * Math.PI)
                ctx.fill()
                for (var k = 0; k < outer.length; ++k) {
                    if (!outer[k].outsideDrawnFace)
                        continue
                    var ro = outer[k].fraction * SR
                    if (ro <= 0.5)
                        continue
                    ctx.beginPath(); ctx.arc(cx, cy, ro, 0, 2 * Math.PI)
                    ctx.strokeStyle = "rgba(35,35,35,0.40)"
                    ctx.lineWidth = 1
                    ctx.stroke()
                }
            }

            // ring edges, at their true fractions of the face
            var rings = s.rings ? s.rings : []
            for (var i = 0; i < rings.length; ++i) {
                var r = rings[i].fraction * R
                if (r <= 0.5)
                    continue
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI)
                ctx.strokeStyle = rings[i].inBlack ? "rgba(255,255,255,0.80)"
                                                   : "rgba(35,35,35,0.55)"
                ctx.lineWidth = 1
                ctx.stroke()
            }

            // The inner ten, where the rules define one by dimension. 10 m Air
            // Rifle defines it by gauge outcome instead, so it has none to draw.
            if (s.hasInnerTen === true && s.innerTenRadiusFraction * R > 1.5) {
                ctx.beginPath()
                ctx.arc(cx, cy, s.innerTenRadiusFraction * R, 0, 2 * Math.PI)
                ctx.strokeStyle = "rgba(255,255,255,0.55)"
                ctx.lineWidth = 1
                ctx.stroke()
            }

            if (!view.showRingNumbers || R < 60)
                return
            ctx.font = "bold " + Math.max(8, Math.round(R * 0.075)) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            for (var j = 0; j < rings.length; ++j) {
                var ring = rings[j].ring
                if (ring >= 10)
                    continue
                // midway between this ring edge and the next one in
                var nextFraction = (j + 1 < rings.length) ? rings[j + 1].fraction : 0
                var band = (rings[j].fraction + nextFraction) / 2 * R
                if (band < 6)
                    continue
                ctx.fillStyle = (band <= Math.min(1.0, s.blackRadiusFraction) * R) ? "#f4efe4" : "#333333"
                ctx.fillText(String(ring), cx - band, cy)
                ctx.fillText(String(ring), cx + band, cy)
            }
        }
    }

    // ── unsupported / unknown standard ──────────────────────────────────
    Rectangle {
        anchors.centerIn: parent
        width: view.faceRadius * 2
        height: width
        radius: width / 2
        visible: !view.supported
        color: "transparent"
        border.width: 1
        border.color: theme.borderColor

        Column {
            anchors.centerIn: parent
            spacing: 4
            width: parent.width * 0.8
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: view.targetStandardId.length > 0
                      ? "Target standard not supported"
                      : "No target standard reported"
                font.family: theme.fontFamily
                font.pixelSize: Math.max(9, view.faceRadius * 0.10)
                color: theme.textSecondary
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideMiddle
                visible: view.targetStandardId.length > 0 && view.faceRadius > 50
                text: view.targetStandardId
                font.family: theme.fontFamily
                font.pixelSize: Math.max(8, view.faceRadius * 0.075)
                color: theme.statusDisconnected
            }
        }
    }

    // ── shot markers ────────────────────────────────────────────────────
    //
    // THREE DIFFERENT THINGS, DRAWN DIFFERENTLY, so none can be mistaken for
    // another:
    //
    //   ·  the SHOT CENTRE      the measured x/y, a small solid point
    //   ○  the PROJECTILE       the real hole, at its true physical diameter
    //   ◎  the LATEST-SHOT HALO a selection cue, outside the hole, brand red
    //
    // The halo is deliberately larger than any pellet so its outer edge cannot
    // be read as the edge of the hole. ISSF scores by the OUTWARD GAUGE — the
    // pellet's edge, not its centre — so showing the true footprint is what
    // makes a displayed score legible to a human at all.
    Repeater {
        model: view.supported ? view.shots : []

        delegate: Item {
            // Bound, not painted: one shot arriving moves one marker.
            x: view.cx + modelData.x * view.faceRadius
            y: view.cy + modelData.y * view.faceRadius
            z: modelData.last ? 2 : 1

            readonly property bool off: modelData.offFace === true

            // FUTURE ADJUDICATION. A shot may one day carry a presentation
            // status (disputed, annulled, cross-fire, penalty). The marker
            // style changes; the RAW COORDINATE never does, and no shot is
            // ever removed. Nothing sets this today.
            readonly property string presentation:
                modelData.presentation !== undefined ? modelData.presentation : "NORMAL"

            // The hole, at true physical size. The floor is a legibility
            // minimum for small cards, not a geometry claim: above it the
            // radius is exactly projectileRadiusFraction of the face.
            Rectangle {
                id: hole
                visible: !parent.off
                anchors.centerIn: parent
                readonly property real r: Math.max(view.minMarkerPx, view.projectileRadius)
                width: r * 2
                height: r * 2
                radius: r
                // Translucent, with a thin rim: on a 10 m air rifle face the
                // holes genuinely overlap, and a solid fill would turn a
                // twenty-shot group into one blob with no rings behind it.
                color: Qt.rgba(0.07, 0.06, 0.05, 0.72)
                border.width: Math.max(1, r * 0.11)
                border.color: "#efe7d6"
                opacity: view.stale ? 0.55 : 1.0
            }

            // The measured centre. Always drawn, always small, never the same
            // shape as the hole.
            Rectangle {
                visible: !parent.off
                anchors.centerIn: parent
                readonly property real r: Math.max(0.75, hole.r * 0.26)
                width: r * 2
                height: r * 2
                radius: r
                color: modelData.last ? theme.brandPrimary : "#efe7d6"
                opacity: view.stale ? 0.55 : 1.0
            }

            // Latest shot: a halo well clear of the hole.
            Rectangle {
                visible: modelData.last && !parent.off
                anchors.centerIn: parent
                readonly property real r: hole.r + Math.max(3, view.faceRadius * 0.022)
                width: r * 2
                height: r * 2
                radius: r
                color: "transparent"
                border.width: Math.max(1.2, view.faceRadius * 0.006)
                border.color: theme.brandPrimary
                opacity: view.stale ? 0.4 : 0.95
            }

            // ── OFF THE PRINTED FACE ────────────────────────────────────
            //
            // Drawn as an OUTWARD CHEVRON on the rim, never as a hole. A hole
            // painted on the rim would say the shot struck the outer ring; a
            // chevron says "it went that way, past the edge". The true
            // coordinate is untouched in the model - only this symbol sits at
            // the rim.
            Canvas {
                visible: parent.off
                anchors.centerIn: parent
                width: Math.max(9, view.faceRadius * 0.07)
                height: width
                rotation: Math.atan2(modelData.y, modelData.x) * 180 / Math.PI
                opacity: view.stale ? 0.55 : 1.0
                onPaint: {
                    var c = getContext("2d")
                    c.reset()
                    c.beginPath()
                    c.moveTo(width * 0.15, height * 0.12)
                    c.lineTo(width * 0.85, height * 0.5)
                    c.lineTo(width * 0.15, height * 0.88)
                    c.strokeStyle = theme.brandAccent
                    c.lineWidth = Math.max(1.5, width * 0.16)
                    c.lineJoin = "round"
                    c.lineCap = "round"
                    c.stroke()
                }
            }
        }
    }

    // ── DEVELOPMENT GEOMETRY OVERLAY ────────────────────────────────────
    //
    // Off unless TECHAIM_RMS_GEOMETRY_OVERLAY=1. It exists so a qualification
    // screenshot can state its own scale instead of being measured by eye, and
    // it must never appear in a normal display.
    //
    // A LOADER, not a hidden Column: QML evaluates bindings whether or not an
    // item is visible, so a merely invisible overlay still ran its arithmetic
    // on every lane and logged type errors for lanes with no shots or no
    // supported face. Inactive means not instantiated, which is the only
    // version of "off" worth having in a shipped build.
    Loader {
        active: RMS_GEOMETRY_OVERLAY && view.supported && view.faceRadius > 80
        anchors { left: parent.left; bottom: parent.bottom; margins: 6 }
        sourceComponent: Column {
        spacing: 1

        Text {
            text: "DEV GEOMETRY"
            font.family: theme.fontFamily
            font.pixelSize: 9
            font.letterSpacing: 1.0
            color: theme.brandAccent
        }
        Text {
            text: view.targetStandardId
            font.family: theme.fontFamily
            font.pixelSize: 9
            color: theme.textSecondary
        }
        Text {
            text: "face " + view.spec.faceRadiusMm.toFixed(2) + " mm  ·  "
                  + (view.faceRadius / view.spec.faceRadiusMm).toFixed(3) + " px/mm"
            font.family: theme.fontFamily
            font.pixelSize: 9
            color: theme.textSecondary
        }
        Text {
            text: "projectile " + view.spec.projectileDiameterMm.toFixed(1) + " mm  ·  "
                  + (view.projectileRadius * 2).toFixed(1) + " px"
            font.family: theme.fontFamily
            font.pixelSize: 9
            color: theme.textSecondary
        }
        Text {
            visible: view.shots.length > 0
            readonly property var s: view.shots.length > 0
                                     ? view.shots[view.shots.length - 1] : null
            text: s ? ("last  x=" + (s.xMm !== undefined ? s.xMm.toFixed(2) : "?")
                       + " mm  y=" + (s.yMm !== undefined ? s.yMm.toFixed(2) : "?")
                       + " mm  score=" + s.score)
                    : ""
            font.family: theme.fontFamily
            font.pixelSize: 9
            color: theme.textSecondary
        }
        }
    }

    // The newest shot's score, from the node, beside the face.
    Text {
        visible: view.supported && view.showLastShotLabel && view.shots.length > 0
                 && view.faceRadius > 70
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: view.shots.length > 0
              ? view.shots[view.shots.length - 1].score : ""
        font.family: theme.fontFamily
        font.pixelSize: Math.max(11, view.faceRadius * 0.12)
        font.weight: Font.DemiBold
        color: theme.textPrimary
    }
}
