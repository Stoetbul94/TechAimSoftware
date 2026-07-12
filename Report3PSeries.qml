import QtQuick 2.15

// Per-position detail page of the 3P match report (one page per position).
// Two series tables with shot-by-shot scores and totals, plus a large
// numbered target plot and the position statistics.
Item {
    id: seriesPage
    width: 595
    height: 842

    property int posIndex: 0

    readonly property real faceMm: 154.4
    readonly property real pelletMm: APPSETTINGS.bullet_diameter()
    readonly property var posNames: [qsTr("KNEELING"), qsTr("PRONE"), qsTr("STANDING")]

    // First model index of this position's shots (positions are 20-shot blocks).
    readonly property int baseIndex: posIndex * 20

    function shotScore(i) {
        if (baseIndex + i >= globalMatchModel.count) return -1
        return globalMatchModel.get(baseIndex + i).calculatedscore * 1
    }
    function seriesTotal(s) {   // s: 0 = shots 1-10, 1 = shots 11-20
        var sum = 0
        for (var i = s * 10; i < s * 10 + 10; ++i) {
            var v = shotScore(i)
            if (v >= 0) sum += v
        }
        return sum
    }
    function seriesTotalInt(s) {
        var sum = 0
        for (var i = s * 10; i < s * 10 + 10; ++i) {
            var v = shotScore(i)
            if (v >= 0) sum += Math.floor(v)
        }
        return sum
    }
    function posStats() {
        var xs = [], ys = [], inner = 0, n = 0
        for (var i = 0; i < 20; ++i) {
            if (baseIndex + i >= globalMatchModel.count) break
            var e = globalMatchModel.get(baseIndex + i)
            xs.push(e.xmm * 1); ys.push(e.ymm * 1)
            if (e.calculatedscore * 1 >= rightPanel.star_limit_value_rifle) inner++
            n++
        }
        var g = 0, sx = 0, sy = 0
        for (i = 0; i < n; ++i) {
            sx += xs[i]; sy += ys[i]
            for (var j = i + 1; j < n; ++j) {
                var d = Math.sqrt(Math.pow(xs[i]-xs[j],2) + Math.pow(ys[i]-ys[j],2))
                if (d > g) g = d
            }
        }
        return { group: n > 1 ? g + pelletMm : 0,
                 mpiX: n ? sx/n : 0, mpiY: n ? sy/n : 0, inner: inner }
    }

    function refresh() {
        shotsRepeaterA.model = 0; shotsRepeaterA.model = 10
        shotsRepeaterB.model = 0; shotsRepeaterB.model = 10
        plotRepeater.model = 0;  plotRepeater.model = 20
        statsLine.text = statsText()
        // ISSF 3P: integer primary, decimal in brackets.
        totalAText.text = seriesTotalInt(0) + "  (" + fmt(seriesTotal(0)) + ")"
        totalBText.text = seriesTotalInt(1) + "  (" + fmt(seriesTotal(1)) + ")"
        posTotalText.text = (seriesTotalInt(0) + seriesTotalInt(1))
                          + "  (" + fmt(seriesTotal(0) + seriesTotal(1)) + ")"
    }
    function fmt(v) { return (v * 1).toFixed(1) }
    // Per-shot display: integer primary with the decimal in brackets (ISSF 3P).
    function fmtShot(v) { return Math.floor(v) + " (" + fmt(v) + ")" }
    function statsText() {
        var st = posStats()
        return qsTr("Inner 10s: ") + st.inner
             + qsTr("      Group: ") + st.group.toFixed(1) + qsTr(" mm")
             + qsTr("      MPI: ") + st.mpiX.toFixed(1) + " ; " + st.mpiY.toFixed(1) + qsTr(" mm")
    }

    Rectangle {
        anchors.fill: parent
        color: "white"

        Column {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 30
            spacing: 16

            // ── Header ────────────────────────────────────────────────────
            Item {
                width: parent.width; height: 48
                Column {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        text: posNames[posIndex] + qsTr("  ·  SHOT DETAIL")
                        font.pixelSize: 17; font.bold: true; font.letterSpacing: 2; color: "#111111"
                    }
                    Text {
                        text: qsTr("50m Rifle 3 Positions · ") + userName + (appMode ? "" : qsTr("  —  DEMO"))
                        font.pixelSize: 9; color: appMode ? "#555555" : "#a80038"; font.letterSpacing: 1
                    }
                }
                Image {
                    source: "qrc:/images/logo/techaim_color.png"
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    height: 32; fillMode: Image.PreserveAspectFit
                }
            }
            Rectangle { width: parent.width; height: 2; color: "#a80038" }

            // ── Series tables + target plot ───────────────────────────────
            Row {
                width: parent.width
                spacing: 18

                // Series A
                Column {
                    id: colA
                    width: (parent.width - 36) * 0.27
                    Rectangle {
                        width: parent.width; height: 22; color: "#222222"
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.4; height: parent.height; text: qsTr("SHOT");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            Text { width: parent.width*0.6; height: parent.height; text: qsTr("SCORE"); color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    Repeater {
                        id: shotsRepeaterA
                        model: 10
                        delegate: Rectangle {
                            width: colA.width; height: 24
                            color: index % 2 ? "#f4f4f4" : "white"
                            border.color: "#dddddd"; border.width: 1
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.4; height: parent.height; text: index + 1; font.pixelSize: 11; color: "#555555"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                Text {
                                    width: parent.width*0.6; height: parent.height
                                    text: shotScore(index) >= 0 ? fmtShot(shotScore(index)) : "-"
                                    font.pixelSize: 11; font.bold: true
                                    color: shotScore(index) >= 10 ? "#3a9a3a" : (shotScore(index) >= 9 ? "#111111" : "#a80038")
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                    Rectangle {
                        width: parent.width; height: 28; color: "#f0f0f0"
                        border.color: "#222222"; border.width: 1
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.4; height: parent.height; text: qsTr("S-A"); font.pixelSize: 10; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            Text { id: totalAText; width: parent.width*0.6; height: parent.height; text: "-"; font.pixelSize: 11; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }

                // Series B
                Column {
                    id: colB
                    width: (parent.width - 36) * 0.27
                    Rectangle {
                        width: parent.width; height: 22; color: "#222222"
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.4; height: parent.height; text: qsTr("SHOT");  color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            Text { width: parent.width*0.6; height: parent.height; text: qsTr("SCORE"); color: "white"; font.pixelSize: 9; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    Repeater {
                        id: shotsRepeaterB
                        model: 10
                        delegate: Rectangle {
                            width: colB.width; height: 24
                            color: index % 2 ? "#f4f4f4" : "white"
                            border.color: "#dddddd"; border.width: 1
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.4; height: parent.height; text: index + 11; font.pixelSize: 11; color: "#555555"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                Text {
                                    width: parent.width*0.6; height: parent.height
                                    text: shotScore(index + 10) >= 0 ? fmtShot(shotScore(index + 10)) : "-"
                                    font.pixelSize: 11; font.bold: true
                                    color: shotScore(index + 10) >= 10 ? "#3a9a3a" : (shotScore(index + 10) >= 9 ? "#111111" : "#a80038")
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                    Rectangle {
                        width: parent.width; height: 28; color: "#f0f0f0"
                        border.color: "#222222"; border.width: 1
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.4; height: parent.height; text: qsTr("S-B"); font.pixelSize: 10; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            Text { id: totalBText; width: parent.width*0.6; height: parent.height; text: "-"; font.pixelSize: 11; font.bold: true; color: "#111111"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }

                // Target plot with numbered shots
                Column {
                    width: (parent.width - 36) * 0.46
                    spacing: 8
                    Image {
                        id: bigFace
                        source: "qrc:/images/centerPanel/black_50_Rifle.png"
                        width: parent.width
                        height: width
                        Repeater {
                            id: plotRepeater
                            model: 20
                            delegate: Rectangle {
                                visible: shotScore(index) >= 0
                                width: bigFace.width * (pelletMm / faceMm)
                                height: width
                                radius: width / 2
                                color: "#35c7e8"
                                border.color: "#1a7f99"; border.width: 1
                                x: bigFace.width/2  + (baseIndex + index < globalMatchModel.count ? globalMatchModel.get(baseIndex + index).xmm*1 : 0) * (bigFace.width  / faceMm) - width/2
                                y: bigFace.height/2 - (baseIndex + index < globalMatchModel.count ? globalMatchModel.get(baseIndex + index).ymm*1 : 0) * (bigFace.height / faceMm) - height/2
                                Text {
                                    anchors.centerIn: parent
                                    text: index + 1
                                    font.pixelSize: Math.max(5, parent.width * 0.45)
                                    color: "white"
                                }
                            }
                        }
                    }
                    Rectangle {
                        width: parent.width; height: 30; color: "#a80038"
                        Row {
                            anchors.fill: parent
                            Text { width: parent.width*0.5; height: parent.height; text: qsTr("  POSITION TOTAL"); color: "white"; font.pixelSize: 10; font.bold: true; verticalAlignment: Text.AlignVCenter }
                            Text { id: posTotalText; width: parent.width*0.5; height: parent.height; text: "-"; color: "white"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    Text {
                        id: statsLine
                        width: parent.width
                        text: "-"
                        font.pixelSize: 10; color: "#333333"
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // ── Footer ────────────────────────────────────────────────────────
        Item {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 30
            height: 30
            Text {
                anchors.left: parent.left; anchors.bottom: parent.bottom
                text: qsTr("Page ") + (posIndex + 2) + qsTr(" of 4")
                font.pixelSize: 8; color: "#999999"
            }
            Text {
                anchors.right: parent.right; anchors.bottom: parent.bottom
                text: qsTr("Generated by TechAim Electronic Target · ISSF 2026")
                font.pixelSize: 8; color: "#999999"
            }
        }
    }
}
