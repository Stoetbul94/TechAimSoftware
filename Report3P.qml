import QtQuick 2.15

// ISSF 50m Rifle 3 Positions match report — A4 page (595x842 @ 72dpi),
// grabbed to image by MatchReport.qml and written to PDF by CUSTOMPRINT.
// Sighters are excluded by construction: globalMatchModel holds match shots only.
Item {
    id: reportRoot
    width: 595
    height: 842

    readonly property real faceMm: 154.4          // 50m rifle target face width in mm
    readonly property real pelletMm: APPSETTINGS.bullet_diameter()
    readonly property var posNames: [qsTr("KNEELING"), qsTr("PRONE"), qsTr("STANDING")]

    // Recomputed when the report is opened (MatchReport.onVisibleChanged calls refresh()).
    property var stats: [null, null, null]
    property var seriesTotals: []
    property var seriesTotalsInt: []
    property var bestShot: ({ score: 0, num: 0, pos: 0 })
    property var worstShot: ({ score: 99, num: 0, pos: 0 })
    property real grandDec: 0
    property int grandInt: 0

    function refresh() {
        var newStats = []
        var gDec = 0, gInt = 0
        var best = { score: -1, num: 0, pos: 0 }
        var worst = { score: 99, num: 0, pos: 0 }
        for (var p = 0; p < 3; ++p)
            newStats.push({ dec: 0, intg: 0, inner: 0, n: 0, time: 0,
                            sA: 0, sB: 0, sAi: 0, sBi: 0, group: 0, mpiX: 0, mpiY: 0,
                            c10: 0, c9: 0, c8: 0, cLow: 0, xs: [], ys: [] })
        for (var i = 0; i < globalMatchModel.count; ++i) {
            var e = globalMatchModel.get(i)
            var p2 = e.position === undefined || e.position < 0 ? 0 : e.position
            var st = newStats[p2]
            var s = e.calculatedscore * 1
            st.dec += s
            st.intg += Math.floor(s)
            if (s >= rightPanel.star_limit_value_rifle) st.inner++
            if (st.n < 10) { st.sA += s; st.sAi += Math.floor(s) }
            else           { st.sB += s; st.sBi += Math.floor(s) }
            if (s >= 10) st.c10++
            else if (s >= 9) st.c9++
            else if (s >= 8) st.c8++
            else st.cLow++
            st.time += e.timeComsumed * 1
            st.xs.push(e.xmm * 1)
            st.ys.push(e.ymm * 1)
            st.n++
            if (s > best.score)  best  = { score: s, num: i + 1, pos: p2 }
            if (s < worst.score) worst = { score: s, num: i + 1, pos: p2 }
            gDec += s
            gInt += Math.floor(s)
        }
        for (p = 0; p < 3; ++p) {
            var t = newStats[p]
            var g = 0, sx = 0, sy = 0
            for (i = 0; i < t.n; ++i) {
                sx += t.xs[i]; sy += t.ys[i]
                for (var j = i + 1; j < t.n; ++j) {
                    var d = Math.sqrt(Math.pow(t.xs[i]-t.xs[j], 2) + Math.pow(t.ys[i]-t.ys[j], 2))
                    if (d > g) g = d
                }
            }
            t.group = t.n > 1 ? g + pelletMm : 0
            t.mpiX = t.n > 0 ? sx / t.n : 0
            t.mpiY = t.n > 0 ? sy / t.n : 0
        }
        var sTotals = [], sTotalsInt = []
        for (var k = 0; k < 6; ++k) {
            var sum = 0, sumInt = 0
            for (i = k * 10; i < Math.min((k + 1) * 10, globalMatchModel.count); ++i) {
                var sv = globalMatchModel.get(i).calculatedscore * 1
                sum += sv
                sumInt += Math.floor(sv)
            }
            sTotals.push(sum)
            sTotalsInt.push(sumInt)
        }
        stats = newStats
        seriesTotals = sTotals
        seriesTotalsInt = sTotalsInt
        bestShot = best
        worstShot = worst
        grandDec = gDec
        grandInt = gInt
        targetsRepeater.model = 0
        targetsRepeater.model = 3
    }

    function fmt(v, dp) { return (v * 1).toFixed(dp === undefined ? 1 : dp) }
    function bestSeriesIdx() {
        var bi = 0
        for (var i = 1; i < seriesTotals.length; ++i)
            if (seriesTotals[i] > seriesTotals[bi]) bi = i
        return bi
    }
    function worstSeriesIdx() {
        var wi = 0
        for (var i = 1; i < seriesTotals.length; ++i)
            if (seriesTotals[i] < seriesTotals[wi]) wi = i
        return wi
    }

    Rectangle {
        anchors.fill: parent
        color: "white"

        Column {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 16

            // ── Header ────────────────────────────────────────────────────
            Item {
                width: parent.width; height: 54
                Column {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text { text: qsTr("MATCH REPORT"); font.pixelSize: 21; font.bold: true; font.letterSpacing: 2; color: "#111111" }
                    Text {
                        text: qsTr("50m Rifle 3 Positions · ISSF 3x20 Qualification") + (appMode ? "" : qsTr("   —  DEMO"))
                        font.pixelSize: 10; color: appMode ? "#555555" : "#a80038"; font.letterSpacing: 1
                    }
                }
                Image {
                    source: "qrc:/images/logo/techaim_color.png"
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    height: 52; fillMode: Image.PreserveAspectFit
                }
            }
            Rectangle { width: parent.width; height: 2; color: "#a80038" }

            // ── Meta line ─────────────────────────────────────────────────
            Row {
                width: parent.width; spacing: 22
                Text { text: qsTr("Shooter: ") + userName; font.pixelSize: 10; font.bold: true; color: "#111111" }
                Text { text: qsTr("Date: ") + new Date().toLocaleString(Qt.locale(""), "yyyy-MM-dd  hh:mm"); font.pixelSize: 10; color: "#333333" }
                Text { text: qsTr("Shots: ") + globalMatchModel.count + "/60"; font.pixelSize: 10; color: "#333333" }
                Text { text: qsTr("Match time used: ") + (totalTime/60).toFixed(1) + qsTr(" min"); font.pixelSize: 10; color: "#333333" }
            }

            // ── Position result table ─────────────────────────────────────
            Column {
                width: parent.width
                Rectangle {
                    width: parent.width; height: 20; color: "#222222"
                    Row {
                        anchors.fill: parent
                        Text { width: parent.width*0.16; text: qsTr("POSITION");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.10; text: qsTr("SERIES A");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.10; text: qsTr("SERIES B");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.15; text: qsTr("SUBTOTAL");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.11; text: qsTr("INNER 10s"); color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.13; text: qsTr("GROUP mm");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.15; text: qsTr("MPI mm");    color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.10; text: qsTr("s/SHOT");    color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                    }
                }
                Repeater {
                    model: 3
                    delegate: Rectangle {
                        width: parent.width; height: 30
                        color: index % 2 ? "#f4f4f4" : "white"
                        border.color: "#dddddd"; border.width: 1
                        property var st: stats[index]
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.16; text: posNames[index]; font.pixelSize: 10; font.bold: true; color: "#a80038"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.10; text: st ? st.sAi + " (" + fmt(st.sA) + ")" : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.10; text: st ? st.sBi + " (" + fmt(st.sB) + ")" : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.15; text: st ? st.intg + " (" + fmt(st.dec) + ")" : "-"; font.pixelSize: 10; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.11; text: st ? st.inner : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.13; text: st && st.n > 1 ? fmt(st.group) : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.15; text: st && st.n > 0 ? fmt(st.mpiX) + " ; " + fmt(st.mpiY) : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { width: parent.width*0.10; text: st && st.n > 0 ? fmt(st.time / st.n, 0) : "-"; font.pixelSize: 10; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        }
                    }
                }
                Rectangle {
                    width: parent.width; height: 32; color: "#a80038"
                    Row {
                        anchors.fill: parent
                        Text { width: parent.width*0.16; text: qsTr("TOTAL"); color: "white"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.20; text: ""; height: parent.height }
                        Text { width: parent.width*0.15; text: grandInt + " (" + fmt(grandDec) + ")"; color: "white"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.11; text: stats[0] ? (stats[0].inner + stats[1].inner + stats[2].inner) : ""; color: "white"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        Text { width: parent.width*0.38; text: ""; height: parent.height }
                    }
                }
            }

            // ── Series trend ──────────────────────────────────────────────
            Column {
                width: parent.width; spacing: 3
                Text { text: qsTr("SERIES TREND"); font.pixelSize: 9; font.bold: true; font.letterSpacing: 2; color: "#555555" }
                Row {
                    width: parent.width; spacing: 6
                    Repeater {
                        model: 6
                        delegate: Rectangle {
                            width: (reportRoot.width - 60 - 30) / 6; height: 40
                            color: index === bestSeriesIdx() ? "#e7f5e7" : (index === worstSeriesIdx() ? "#fdeaea" : "#f4f4f4")
                            border.color: index === bestSeriesIdx() ? "#3a9a3a" : (index === worstSeriesIdx() ? "#a80038" : "#dddddd")
                            Column {
                                anchors.centerIn: parent; spacing: 1
                                Text {
                                    text: "S" + (index+1) + " · " + posNames[Math.floor(index/2)].charAt(0)
                                    font.pixelSize: 7; color: "#777777"; anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: seriesTotals.length > index
                                          ? seriesTotalsInt[index] + " (" + fmt(seriesTotals[index]) + ")" : "-"
                                    font.pixelSize: 10; font.bold: true; color: "#111111"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
            }

            // ── Per-position targets ──────────────────────────────────────
            Column {
                width: parent.width; spacing: 3
                Text { text: qsTr("GROUPING BY POSITION"); font.pixelSize: 9; font.bold: true; font.letterSpacing: 2; color: "#555555" }
                Row {
                    width: parent.width; spacing: 10
                    Repeater {
                        id: targetsRepeater
                        model: 3
                        delegate: Column {
                            spacing: 2
                            property int posIndex: index
                            Image {
                                id: faceImg
                                source: "qrc:/images/centerPanel/black_50_Rifle.png"
                                width: (reportRoot.width - 60 - 20) / 3
                                height: width
                                Repeater {
                                    model: globalMatchModel
                                    delegate: Rectangle {
                                        visible: model.position === posIndex
                                        width: faceImg.width * (pelletMm / faceMm)
                                        height: width
                                        radius: width / 2
                                        color: "#35c7e8"
                                        x: faceImg.width/2  + (model.xmm*1) * (faceImg.width  / faceMm) - width/2
                                        y: faceImg.height/2 - (model.ymm*1) * (faceImg.height / faceMm) - height/2
                                    }
                                }
                            }
                            Row {
                                width: faceImg.width
                                Text { width: parent.width*0.6; text: posNames[posIndex]; font.pixelSize: 9; font.bold: true; color: "#a80038" }
                                Text {
                                    width: parent.width*0.4
                                    text: stats[posIndex] ? stats[posIndex].intg + " (" + fmt(stats[posIndex].dec) + ")" : "-"
                                    font.pixelSize: 9; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }
                }
            }

            // ── Distribution + notable shots ──────────────────────────────
            Column {
                width: parent.width; spacing: 2
                Text { text: qsTr("SCORE DISTRIBUTION  (10s / 9s / 8s / below)"); font.pixelSize: 9; font.bold: true; font.letterSpacing: 2; color: "#555555" }
                Row {
                    width: parent.width; spacing: 18
                    Repeater {
                        model: 3
                        delegate: Text {
                            text: posNames[index].charAt(0) + ":  " + (stats[index] ? (stats[index].c10 + " / " + stats[index].c9 + " / " + stats[index].c8 + " / " + stats[index].cLow) : "-")
                            font.pixelSize: 10; color: "#111111"
                        }
                    }
                    Text {
                        text: qsTr("Best: ") + Math.floor(bestShot.score) + " (" + fmt(bestShot.score) + ") #" + bestShot.num + " " + posNames[bestShot.pos].charAt(0)
                              + qsTr("   Worst: ") + Math.floor(worstShot.score) + " (" + fmt(worstShot.score) + ") #" + worstShot.num + " " + posNames[worstShot.pos].charAt(0)
                        font.pixelSize: 10; font.bold: true; color: "#111111"
                    }
                }
            }

        }

        // ── Footer (pinned to the page bottom) ────────────────────────────
        Item {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 26
            height: 40
            Column {
                anchors.left: parent.left; anchors.bottom: parent.bottom; spacing: 2
                Rectangle { width: 170; height: 1; color: "#111111" }
                Text { text: qsTr("Range Officer signature"); font.pixelSize: 8; color: "#555555" }
            }
            Text {
                anchors.right: parent.right; anchors.bottom: parent.bottom
                text: qsTr("Generated by TechAim Electronic Target · ISSF 2026")
                font.pixelSize: 8; color: "#999999"
            }
        }
    }
}
