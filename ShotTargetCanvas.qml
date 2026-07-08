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

    implicitWidth: side
    implicitHeight: side + 22

    onShotsChanged: cv.requestPaint()
    onExtentMmChanged: cv.requestPaint()

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

            // fit scale (shared or auto)
            var ext = root.extentMm
            if (ext <= 0) {
                ext = 8
                for (var i = 0; i < list.length; i++)
                    ext = Math.max(ext, Math.sqrt(list[i].x * list[i].x + list[i].y * list[i].y))
                ext *= 1.15
            }
            if (ext <= 0) ext = 1

            // target face
            ctx.fillStyle = "#141416"
            ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill()
            // rings (10 -> centre); inner rings a touch brighter
            for (var k = 10; k >= 1; k--) {
                ctx.beginPath(); ctx.arc(cx, cy, R * k / 10, 0, 2 * Math.PI)
                ctx.strokeStyle = (k <= 3) ? "#3d3d44" : "#28282c"
                ctx.lineWidth = 1; ctx.stroke()
            }
            // bull
            ctx.fillStyle = "#050506"
            ctx.beginPath(); ctx.arc(cx, cy, R * 0.18, 0, 2 * Math.PI); ctx.fill()
            // crosshair
            ctx.strokeStyle = "#2a2a2e"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy)
            ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R); ctx.stroke()

            // shots (y up on screen); colour by score band
            var scale = R / ext
            for (var j = 0; j < list.length; j++) {
                var s = list[j]
                var px = cx + scale * s.x
                var py = cy - scale * s.y
                var sc = s.decimalScore
                ctx.fillStyle = sc >= 10.5 ? "#37c871" : (sc >= 10.0 ? "#e0b020" : "#e0503a")
                ctx.globalAlpha = 0.9
                ctx.beginPath(); ctx.arc(px, py, 5, 0, 2 * Math.PI); ctx.fill()
                ctx.globalAlpha = 1.0
                ctx.strokeStyle = "#0d0d0f"; ctx.lineWidth = 1; ctx.stroke()
            }
        }
    }
}
