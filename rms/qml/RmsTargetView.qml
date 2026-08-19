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

    readonly property var spec: TARGETGEO.specFor(targetStandardId)
    readonly property bool supported: spec && spec.supported === true
    readonly property real faceRadius: Math.min(width, height) / 2 - 4
    readonly property real cx: width / 2
    readonly property real cy: height / 2
    // Markers scale with the face so a small card is not a smear of dots and a
    // full-screen face is not a scatter of pinpricks.
    readonly property real markerRadius: Math.max(2.0, faceRadius * 0.030)

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

            // black aiming mark
            ctx.fillStyle = "#141414"
            ctx.beginPath()
            ctx.arc(cx, cy, s.blackRadiusFraction * R, 0, 2 * Math.PI)
            ctx.fill()

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
                ctx.fillStyle = (band <= s.blackRadiusFraction * R) ? "#f4efe4" : "#333333"
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
    Repeater {
        model: view.supported ? view.shots : []

        delegate: Item {
            // Bound, not painted: one shot arriving moves one marker.
            x: view.cx + modelData.x * view.faceRadius - width / 2
            y: view.cy + modelData.y * view.faceRadius - height / 2
            width: marker.width
            height: marker.height
            z: modelData.last ? 2 : 1

            // FUTURE ADJUDICATION. A shot may one day carry a presentation
            // status (disputed, annulled, cross-fire, penalty). The marker
            // style changes; the RAW COORDINATE never does, and no shot is
            // ever removed. Nothing sets this today.
            readonly property string presentation:
                modelData.presentation !== undefined ? modelData.presentation : "NORMAL"

            Rectangle {
                id: marker
                readonly property real r: modelData.last ? view.markerRadius * 1.35
                                                         : view.markerRadius
                width: r * 2
                height: r * 2
                radius: r
                // Not colour alone: the last shot is larger and ringed, so the
                // distinction survives a projector, a bright range and a
                // colour-blind operator.
                color: modelData.last ? theme.brandPrimary : "#1a1a1a"
                border.width: modelData.last ? 0 : Math.max(1, view.markerRadius * 0.35)
                border.color: "#f4efe4"
                opacity: view.stale ? 0.5 : 1.0
            }

            // The newest shot gets a halo as well as size.
            Rectangle {
                visible: modelData.last
                anchors.centerIn: marker
                width: marker.width * 2.4
                height: width
                radius: width / 2
                color: "transparent"
                border.width: Math.max(1, view.markerRadius * 0.35)
                border.color: theme.brandPrimary
                opacity: view.stale ? 0.4 : 0.9
            }

            // A shot that landed off the printed face is held at the edge and
            // marked, never dropped — a cross-fire is exactly what an operator
            // needs to see.
            Rectangle {
                visible: modelData.offFace === true
                anchors.centerIn: marker
                width: marker.width * 3.0
                height: width
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: theme.brandAccent
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
