import QtQuick 2.15

// 3P FINAL report — miniature target plot (print palette, Phase D4).
// TRUE ISSF 50m-rifle ring geometry (10-ring radius 5.2 mm, 8 mm per ring —
// the same constants the scoring face and coach maps use); shots are plotted
// from the report's STORED X/Y verbatim. The only transform is the linear
// mm -> px fit; no new coordinate calculations. White-page counterpart of
// the coach ShotTargetCanvas.
Item {
    id: root
    property var    shots: []        // [{ x, y, score, number }]  (mm)
    property string title: ""
    property real   side: 220        // square canvas edge
    property real   extentMm: 0      // shared scale across the plots; 0 = auto

    readonly property real tenRingRadiusMm: 5.2
    readonly property real ringStepMm: 8.0
    readonly property real bulletDiameterMm: 5.6

    implicitWidth: side
    implicitHeight: side + 20

    onShotsChanged: cv.requestPaint()
    onExtentMmChanged: cv.requestPaint()

    Text {
        id: cap
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.title + "  (" + (root.shots ? root.shots.length : 0) + ")"
        color: "#5b6270"
        font.pixelSize: 11
        font.bold: true
        font.family: "Segoe UI"
        font.letterSpacing: 0.5
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
            var margin = 6
            var R = Math.min(w, h) / 2 - margin
            var list = root.shots ? root.shots : []

            // Extent: shared (preferred, keeps the three plots comparable) or
            // auto. Always show at least out to the 7 ring, expanding in
            // whole-ring steps to contain the worst shot — dot-vs-ring stays
            // the shot's real score position at every zoom.
            var ext = root.extentMm
            if (ext <= 0) {
                ext = 0
                for (var i = 0; i < list.length; i++)
                    ext = Math.max(ext, Math.sqrt(list[i].x * list[i].x + list[i].y * list[i].y))
            }
            var minExt = root.tenRingRadiusMm + 3 * root.ringStepMm          // 7 ring
            if (ext < minExt) ext = minExt
            else ext = root.tenRingRadiusMm
                      + Math.ceil((ext - root.tenRingRadiusMm) / root.ringStepMm) * root.ringStepMm
            var scale = R / ext

            // face
            ctx.fillStyle = "#fbfbfc"
            ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill()
            ctx.strokeStyle = "#d9dce1"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.stroke()
            // rings at their REAL radii (10 -> 1); only those in view
            for (var k = 10; k >= 1; k--) {
                var rr = (root.tenRingRadiusMm + (10 - k) * root.ringStepMm) * scale
                if (rr > R + 0.5) continue
                ctx.beginPath(); ctx.arc(cx, cy, rr, 0, 2 * Math.PI)
                ctx.strokeStyle = (k >= 9) ? "#b9bec6" : "#dcdfe4"
                ctx.lineWidth = 1; ctx.stroke()
            }
            // bull = the real 10 ring, lightly filled
            ctx.fillStyle = "#e8eaee"
            ctx.beginPath(); ctx.arc(cx, cy, root.tenRingRadiusMm * scale, 0, 2 * Math.PI); ctx.fill()
            ctx.strokeStyle = "#b9bec6"
            ctx.beginPath(); ctx.arc(cx, cy, root.tenRingRadiusMm * scale, 0, 2 * Math.PI); ctx.stroke()
            // crosshair
            ctx.strokeStyle = "#e3e5e9"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy)
            ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R); ctx.stroke()

            // shots at true hole scale (y up on screen); score-band colours
            var dotR = Math.max(2.5, scale * root.bulletDiameterMm / 2)
            for (var j = 0; j < list.length; j++) {
                var s = list[j]
                var px = cx + scale * s.x
                var py = cy - scale * s.y
                ctx.fillStyle = s.score >= 10.5 ? "#1f8a4c"
                              : (s.score >= 10.0 ? "#d9a11a" : "#a80038")
                ctx.globalAlpha = 0.85
                ctx.beginPath(); ctx.arc(px, py, dotR, 0, 2 * Math.PI); ctx.fill()
                ctx.globalAlpha = 1.0
                ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 1; ctx.stroke()
            }
        }
    }
}
