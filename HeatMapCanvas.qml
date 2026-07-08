import QtQuick 2.15

// Kernel-density heat map of shot fall (SIUS/Meyton style): additive Gaussian
// blobs accumulated per shot, then recoloured with a jet colormap on a navy
// field. Pure visualisation of COACHREPORT.shots() (mm) — no analytics.
// `theme` resolves via main.qml's context.
Item {
    id: root
    property var    shots: []        // [{ x, y, decimalScore }]  (mm)
    property string title: ""
    property real   side: 240
    property real   extentMm: 0      // shared scale across maps; 0 = auto-fit

    implicitWidth: side
    implicitHeight: side + 22

    onShotsChanged: cv.requestPaint()
    onExtentMmChanged: cv.requestPaint()

    Text {
        id: cap
        anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter
        text: root.title + "  (" + (root.shots ? root.shots.length : 0) + ")"
        color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
    }

    Canvas {
        id: cv
        anchors.top: cap.bottom; anchors.topMargin: 4; anchors.horizontalCenter: parent.horizontalCenter
        width: root.side; height: root.side
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var w = width, h = height, cx = w / 2, cy = h / 2
            var list = root.shots ? root.shots : []

            // Accumulate grayscale density on black (additive blobs).
            ctx.fillStyle = "#000000"; ctx.fillRect(0, 0, w, h)

            if (list.length > 0) {
                var margin = 10
                var R = Math.min(w, h) / 2 - margin
                var ext = root.extentMm
                if (ext <= 0) {
                    ext = 8
                    for (var i = 0; i < list.length; i++)
                        ext = Math.max(ext, Math.sqrt(list[i].x * list[i].x + list[i].y * list[i].y))
                    ext *= 1.15
                }
                if (ext <= 0) ext = 1
                var scale = R / ext
                var rad = Math.min(w, h) * 0.16        // KDE bandwidth (px)

                ctx.globalCompositeOperation = "lighter"
                for (var j = 0; j < list.length; j++) {
                    var px = cx + scale * list[j].x
                    var py = cy - scale * list[j].y     // +y = high
                    var g = ctx.createRadialGradient(px, py, 0, px, py, rad)
                    g.addColorStop(0.0, "rgba(255,255,255,0.5)")
                    g.addColorStop(1.0, "rgba(255,255,255,0.0)")
                    ctx.fillStyle = g
                    ctx.beginPath(); ctx.arc(px, py, rad, 0, 2 * Math.PI); ctx.fill()
                }
                ctx.globalCompositeOperation = "source-over"

                // Recolour: intensity (normalised to peak) -> jet on navy.
                var img = ctx.getImageData(0, 0, w, h)
                var d = img.data
                var maxv = 1
                for (var p = 0; p < d.length; p += 4) if (d[p] > maxv) maxv = d[p]
                for (var q = 0; q < d.length; q += 4) {
                    var t = d[q] / maxv
                    if (t < 0.10) { d[q] = 10; d[q + 1] = 10; d[q + 2] = 60 }   // navy field
                    else {
                        t = Math.pow(t, 0.85)
                        d[q]     = Math.max(0, Math.min(1, 1.5 - Math.abs(4 * t - 3))) * 255
                        d[q + 1] = Math.max(0, Math.min(1, 1.5 - Math.abs(4 * t - 2))) * 255
                        d[q + 2] = Math.max(0, Math.min(1, 1.5 - Math.abs(4 * t - 1))) * 255
                    }
                    d[q + 3] = 255
                }
                ctx.putImageData(img, 0, 0)
            } else {
                ctx.fillStyle = "#0a0a3c"; ctx.fillRect(0, 0, w, h)
            }

            // crosshair
            ctx.strokeStyle = "rgba(255,255,255,0.22)"; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(cx, 4); ctx.lineTo(cx, h - 4)
            ctx.moveTo(4, cy); ctx.lineTo(w - 4, cy); ctx.stroke()

            if (list.length === 0) {
                ctx.fillStyle = "rgba(255,255,255,0.5)"
                ctx.fillText("No coordinate data", cx - 42, cy + 18)
            }
        }
    }
}
