import QtQuick 2.15

// A real ISSF-style scoring target: cream face, black bull, white numbered
// score rings at true mm radii for the discipline, with each shot plotted from
// its mm coordinates and coloured by score. Display only (COACHREPORT.shots()).
Item {
    id: root
    property var    shots: []          // [{ x, y, decimalScore }]  (mm)
    property string title: ""
    property real   side: 250
    property string discipline: "rifle50"   // rifle50 | airrifle10 | airpistol10

    implicitWidth: side
    implicitHeight: side

    onShotsChanged: cv.requestPaint()
    onDisciplineChanged: cv.requestPaint()

    // Ring geometry (outer radius of ring k, mm) = ten + (10 - k) * step.
    function spec() {
        if (discipline === "airrifle10")  return { ten: 0.25, step: 2.5, ringMin: 4, blackRing: 4 }
        if (discipline === "airpistol10") return { ten: 5.75, step: 8.0, ringMin: 4, blackRing: 7 }
        return { ten: 5.2, step: 8.0, ringMin: 4, blackRing: 5 }   // rifle50 (default)
    }

    Canvas {
        id: cv
        anchors.fill: parent
        antialiasing: true
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var w = width, h = height, cx = w / 2, cy = h / 2
            var margin = 6
            var Rpx = Math.min(w, h) / 2 - margin
            var sp = root.spec()
            function ringR(k) { return sp.ten + (10 - k) * sp.step }   // mm
            var outerMm = ringR(sp.ringMin)
            var scale = Rpx / outerMm
            var blackMm = ringR(sp.blackRing)

            // cream face
            ctx.fillStyle = "#f4efe4"
            ctx.beginPath(); ctx.arc(cx, cy, Rpx, 0, 2 * Math.PI); ctx.fill()
            // black bull
            ctx.fillStyle = "#111111"
            ctx.beginPath(); ctx.arc(cx, cy, blackMm * scale, 0, 2 * Math.PI); ctx.fill()

            // ring lines
            for (var k = sp.ringMin; k <= 10; k++) {
                var rp = ringR(k) * scale
                ctx.beginPath(); ctx.arc(cx, cy, rp, 0, 2 * Math.PI)
                ctx.strokeStyle = (ringR(k) <= blackMm) ? "rgba(255,255,255,0.85)" : "rgba(30,30,30,0.55)"
                ctx.lineWidth = 1; ctx.stroke()
            }
            // inner 10 dot ring
            ctx.strokeStyle = "rgba(255,255,255,0.9)"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.arc(cx, cy, ringR(10) * scale, 0, 2 * Math.PI); ctx.stroke()

            // ring numbers along the horizontal axis (both sides)
            ctx.font = "bold 11px sans-serif"
            ctx.textAlign = "center"; ctx.textBaseline = "middle"
            for (var n = sp.ringMin; n <= 9; n++) {
                var bandMm = ringR(n) - sp.step * 0.5     // midpoint of the ring band
                var bx = bandMm * scale
                ctx.fillStyle = (bandMm <= blackMm) ? "#f4efe4" : "#333333"
                ctx.fillText(String(n), cx - bx, cy)
                ctx.fillText(String(n), cx + bx, cy)
            }

            // shots (y up), coloured by score
            var list = root.shots ? root.shots : []
            for (var j = 0; j < list.length; j++) {
                var s = list[j]
                var px = cx + scale * s.x
                var py = cy - scale * s.y
                var sc = s.decimalScore
                ctx.fillStyle = sc >= 10.5 ? "#2e9e5b" : (sc >= 10.0 ? "#e0a020" : "#d0392b")
                ctx.globalAlpha = 0.92
                ctx.beginPath(); ctx.arc(px, py, 5, 0, 2 * Math.PI); ctx.fill()
                ctx.globalAlpha = 1.0
                ctx.strokeStyle = "rgba(0,0,0,0.55)"; ctx.lineWidth = 1; ctx.stroke()
            }
        }
    }
}
