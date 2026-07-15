import QtQuick 2.15

// A target-face shot map: concentric rings + bull, with each shot plotted from
// its mm coordinates and coloured by score. Display only — the coordinates come
// straight from COACHREPORT.shots(). `theme` resolves via main.qml's context.
Item {
    id: root
    property var    shots: []        // [{ x, y, decimalScore }]  (mm)
    property string title: ""
    property real   side: 240        // square canvas edge
    property real   extentMm: 0      // shared scale across targets; 0 = auto-fit
    // TRUE ISSF ring geometry (mm). When both are set (> 0), rings are drawn
    // at their real radii — a dot's position against the rings then IS its
    // real score position on the card, exactly like the target face. Unset,
    // the legacy decorative even spacing is kept (rings carry no meaning).
    property real   tenRingRadiusMm: 0   // radius of the 10 ring
    property real   ringStepMm: 0        // radial step per ring (10 -> 1)
    // True hole size: when set, dots are drawn at bullet scale so group
    // tightness (and overlap) reads honestly. 0 = legacy fixed 5 px dots.
    property real   bulletDiameterMm: 0

    implicitWidth: side
    implicitHeight: side + 22

    onShotsChanged: cv.requestPaint()
    onExtentMmChanged: cv.requestPaint()
    onTenRingRadiusMmChanged: cv.requestPaint()
    onRingStepMmChanged: cv.requestPaint()

    Text {
        id: cap
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.title + "  (" + (root.shots ? root.shots.length : 0) + ")"
        color: theme.textSecondary
        font.pixelSize: 13
        font.family: theme.fontFamily
    }

    Canvas {
        id: cv
        anchors.top: cap.bottom
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.side
        height: root.side
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var w = width, h = height, cx = w / 2, cy = h / 2
            var margin = 8
            var R = Math.min(w, h) / 2 - margin
            var list = root.shots ? root.shots : []
            var trueRings = root.tenRingRadiusMm > 0 && root.ringStepMm > 0

            // fit scale (shared or auto)
            var ext = root.extentMm
            if (ext <= 0) {
                ext = 8
                for (var i = 0; i < list.length; i++)
                    ext = Math.max(ext, Math.sqrt(list[i].x * list[i].x + list[i].y * list[i].y))
                ext *= 1.15
            }
            if (ext <= 0) ext = 1
            // True geometry: always show at least out to the 7 ring so a tight
            // group reads as tight against real rings (never a zoomed-in blob
            // that looks spread), and snap the view to the ring containing the
            // worst shot so ring positions stay honest at every zoom.
            if (trueRings) {
                var minExt = root.tenRingRadiusMm + 3 * root.ringStepMm     // 7 ring
                if (ext < minExt) ext = minExt
                else ext = root.tenRingRadiusMm
                          + Math.ceil((ext - root.tenRingRadiusMm) / root.ringStepMm) * root.ringStepMm
            }

            // target face
            ctx.fillStyle = "#141416"
            ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill()
            var scale = R / ext
            if (trueRings) {
                // rings at their REAL radii (10 -> 1); only those in view
                for (var k = 10; k >= 1; k--) {
                    var rr = (root.tenRingRadiusMm + (10 - k) * root.ringStepMm) * scale
                    if (rr > R + 0.5) continue
                    ctx.beginPath(); ctx.arc(cx, cy, rr, 0, 2 * Math.PI)
                    ctx.strokeStyle = (k >= 9) ? "#3d3d44" : "#28282c"
                    ctx.lineWidth = 1; ctx.stroke()
                }
                // bull = the real 10 ring
                ctx.fillStyle = "#050506"
                ctx.beginPath(); ctx.arc(cx, cy, root.tenRingRadiusMm * scale, 0, 2 * Math.PI); ctx.fill()
            } else {
                // legacy decorative rings (no geometry supplied)
                for (var k2 = 10; k2 >= 1; k2--) {
                    ctx.beginPath(); ctx.arc(cx, cy, R * k2 / 10, 0, 2 * Math.PI)
                    ctx.strokeStyle = (k2 <= 3) ? "#3d3d44" : "#28282c"
                    ctx.lineWidth = 1; ctx.stroke()
                }
                ctx.fillStyle = "#050506"
                ctx.beginPath(); ctx.arc(cx, cy, R * 0.18, 0, 2 * Math.PI); ctx.fill()
            }
            // crosshair
            ctx.strokeStyle = "#2a2a2e"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy)
            ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R); ctx.stroke()

            // shots (y up on screen); colour by score band. With a bullet
            // diameter, dots are drawn at true hole scale (min 2.5 px so a
            // zoomed-out view stays readable).
            var dotR = 5
            if (root.bulletDiameterMm > 0)
                dotR = Math.max(2.5, scale * root.bulletDiameterMm / 2)
            for (var j = 0; j < list.length; j++) {
                var s = list[j]
                var px = cx + scale * s.x
                var py = cy - scale * s.y
                var sc = s.decimalScore
                ctx.fillStyle = sc >= 10.5 ? "#37c871" : (sc >= 10.0 ? "#e0b020" : "#e0503a")
                ctx.globalAlpha = 0.9
                ctx.beginPath(); ctx.arc(px, py, dotR, 0, 2 * Math.PI); ctx.fill()
                ctx.globalAlpha = 1.0
                ctx.strokeStyle = "#0d0d0f"; ctx.lineWidth = 1; ctx.stroke()
            }
        }
    }
}
