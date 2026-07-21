import QtQuick 2.15

// 10m Air Rifle / Air Pistol FINAL — self-contained HUD (F2).
//
// Pure presentation over FINALS10M (the single-athlete 10m final controller).
// Contains NO scoring, timing or transition logic; every value and event comes
// from `ctl`. Deliberately independent of the 3P FinalsHud and the 3P finals
// models so the two finals never share fragile state. The elimination points
// are single-athlete COURSE CHECKPOINTS only — this HUD never displays a
// finishing place, medal or rank (those need the future competition
// coordinator's authoritative opponent data).
Item {
    id: hud

    property var ctl                      // FINALS10M
    property bool developerMode: false

    // Transient checkpoint banner state.
    property string checkpointBanner: ""
    property bool completed: false
    // Snapshots taken at completion — a bare ctl.checkpointTotals() /
    // ctl.seriesSubtotals() in a delegate binding is a plain function call with
    // no notify signal, so it would capture an empty map at delegate creation
    // and never re-evaluate. Snapshot into these properties instead.
    property var checkpointSnapshot: ({})
    property var seriesSnapshot: []
    property var reportGroup: ({})       // {mpiX, mpiY, extent, avgSec, n}

    readonly property color _bg:      "#1b1c22"
    readonly property color _bgAlt:   "#24262e"
    readonly property color _red:     "#e8003d"
    readonly property color _txt:     "#f2f3f5"
    readonly property color _txtSec:  "#c9ced6"
    readonly property color _txtMut:  "#9aa0aa"
    readonly property color _amber:   "#e8a13d"
    readonly property color _green:   "#43b581"

    function fmt1(v) { return (v === undefined || v === null) ? "0.0" : Number(v).toFixed(1) }

    // Group geometry / MPI / mean shot time over the accepted official shots
    // (millimetre coordinates). Single-athlete training analysis only.
    function computeGroup(recs) {
        var n = 0, sx = 0, sy = 0, st = 0
        var xs = [], ys = []
        for (var i = 0; i < recs.length; ++i) {
            var r = recs[i]
            if (r.sighter === true) continue
            var x = Number(r.xmm), y = Number(r.ymm)
            if (isNaN(x) || isNaN(y)) continue
            xs.push(x); ys.push(y); sx += x; sy += y
            st += Number(r.timeSec || 0); ++n
        }
        if (n === 0) return { n: 0 }
        var mpiX = sx / n, mpiY = sy / n
        // Group extent = max pairwise distance (largest spread).
        var extent = 0
        for (var a = 0; a < xs.length; ++a)
            for (var b = a + 1; b < xs.length; ++b) {
                var dx = xs[a] - xs[b], dy = ys[a] - ys[b]
                var d = Math.sqrt(dx*dx + dy*dy)
                if (d > extent) extent = d
            }
        return { n: n, mpiX: mpiX, mpiY: mpiY, extent: extent, avgSec: st / n }
    }

    Connections {
        target: hud.ctl
        function onCheckpointReached(shotNumber, label, total) {
            hud.checkpointBanner = label + "   ·   " + hud.fmt1(total)
            bannerTimer.restart()
        }
        function onFinalCompleted() {
            if (hud.ctl) {
                hud.checkpointSnapshot = hud.ctl.checkpointTotals()
                hud.seriesSnapshot = hud.ctl.seriesSubtotals()
                hud.reportGroup = hud.computeGroup(hud.ctl.officialShotRecords())
            }
            hud.completed = true
        }
        function onPhaseChanged() {
            // Reset the completion panel if a fresh course is started.
            if (hud.ctl && hud.ctl.running) hud.completed = false
        }
    }

    Timer { id: bannerTimer; interval: 2600; onTriggered: hud.checkpointBanner = "" }

    // ── Transient central flash — ONLY for the major CRO transitions the
    //    athlete must notice (TAKE YOUR POSITIONS / STOP…UNLOAD). Brief
    //    (~1.1 s), auto-dismissing, non-interactive (no MouseArea — never
    //    intercepts a shot). The PERSISTENT command + countdown lives in the
    //    right-hand Finals10mCommandPanel, NOT here (target-first design).
    property string flashText: ""
    Timer { id: flashTimer; interval: 1100; onTriggered: hud.flashText = "" }
    Connections {
        target: hud.ctl
        function onCommandIssued(ev) {
            var t = ev.typeName
            if (t === "TakeYourPositions" || t === "Stop" || t === "Unload") {
                hud.flashText = ev.text
                flashTimer.restart()
            }
        }
    }
    Rectangle {
        visible: hud.flashText !== ""
        anchors.centerIn: parent
        width: Math.min(hud.width * 0.7, flashText2.implicitWidth + 48)
        height: flashText2.implicitHeight + 28
        radius: 12
        color: Qt.rgba(0, 0, 0, 0.78)
        Text {
            id: flashText2
            anchors.centerIn: parent
            text: hud.flashText
            color: hud._txt; font.pixelSize: 22; font.bold: true
            horizontalAlignment: Text.AlignHCenter
            width: Math.min(hud.width * 0.62, implicitWidth); wrapMode: Text.WordWrap
        }
    }

    // ── Checkpoint banner (transient, top toast — never over the aim) ───
    Rectangle {
        visible: hud.checkpointBanner !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 8
        width: cpText.implicitWidth + 28; height: 30; radius: 15
        color: hud._amber
        Text {
            id: cpText; anchors.centerIn: parent; text: hud.checkpointBanner
            color: "#20130a"; font.pixelSize: 12; font.bold: true
        }
    }

    // ── Unresolved EST incident warning ─────────────────────────────────
    Rectangle {
        id: incidentWarn
        property var inc: (typeof INCIDENTS !== "undefined") ? INCIDENTS.activeIncident() : ({})
        visible: inc && inc.statusKey !== undefined
                 && inc.statusKey !== "resolved" && inc.statusKey !== "cancelled"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: hud.height * 0.30
        width: incText.implicitWidth + 28; height: 30; radius: 6
        color: "#7a1d1d"
        Connections {
            target: (typeof INCIDENTS !== "undefined") ? INCIDENTS : null
            function onIncidentChanged() { incidentWarn.inc = INCIDENTS.activeIncident() }
        }
        Text {
            id: incText; anchors.centerIn: parent
            text: qsTr("EST INCIDENT — official shots blocked until resolved")
            color: "#ffd7d7"; font.pixelSize: 11; font.bold: true
        }
    }

    // ── Completion result panel ─────────────────────────────────────────
    Rectangle {
        visible: hud.completed
        anchors.centerIn: parent
        width: Math.min(hud.width * 0.82, 560)
        height: Math.min(hud.height * 0.82, doneCol.implicitHeight + 40)
        radius: 14; color: hud._bg; border.color: hud._red; border.width: 2
        Column {
            id: doneCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 20; spacing: 10
            Text {
                text: qsTr("FINAL TRAINING COMPLETE")
                color: hud._red; font.pixelSize: 18; font.bold: true; font.letterSpacing: 1
            }
            Text {
                text: (hud.ctl ? hud.ctl.displayName : "") + qsTr("  ·  single-athlete course")
                color: hud._txtMut; font.pixelSize: 11
            }
            Row {
                spacing: 8
                Text { text: qsTr("24-shot total"); color: hud._txtSec; font.pixelSize: 14
                       anchors.verticalCenter: parent.verticalCenter }
                Text { text: hud.fmt1(hud.ctl ? hud.ctl.cumulativeTotal : 0)
                       color: hud._green; font.pixelSize: 28; font.bold: true }
            }
            // series breakdown
            Text { text: qsTr("SERIES"); color: hud._txtMut; font.pixelSize: 9; font.letterSpacing: 2 }
            Row {
                spacing: 20
                Repeater {
                    model: hud.seriesSnapshot
                    Column {
                        visible: modelData.stageId === 2 || modelData.stageId === 3
                        Text {
                            text: modelData.stageId === 2 ? qsTr("Series 1")
                                : modelData.stageId === 3 ? qsTr("Series 2") : ""
                            color: hud._txtMut; font.pixelSize: 10
                        }
                        Text { text: hud.fmt1(modelData.subtotal); color: hud._txt
                               font.pixelSize: 16; font.bold: true }
                    }
                }
                Column {
                    Text { text: qsTr("14 singles"); color: hud._txtMut; font.pixelSize: 10 }
                    Text {
                        text: {
                            var subs = hud.seriesSnapshot; var s = 0
                            for (var i = 0; i < subs.length; ++i)
                                if (subs[i].stageId >= 4) s += subs[i].subtotal
                            return hud.fmt1(s)
                        }
                        color: hud._txt; font.pixelSize: 16; font.bold: true
                    }
                }
            }
            // checkpoint totals
            Text { text: qsTr("COURSE CHECKPOINTS"); color: hud._txtMut; font.pixelSize: 9; font.letterSpacing: 2 }
            Grid {
                columns: 7; columnSpacing: 10; rowSpacing: 4
                Repeater {
                    model: [12, 14, 16, 18, 20, 22, 24]
                    Column {
                        Text { text: "S" + modelData; color: hud._txtMut; font.pixelSize: 9
                               anchors.horizontalCenter: parent.horizontalCenter }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: {
                                var v = hud.checkpointSnapshot[String(modelData)]
                                return v === undefined ? "—" : hud.fmt1(v)
                            }
                            color: modelData >= 22 ? hud._amber : hud._txtSec
                            font.pixelSize: 12; font.bold: modelData >= 22
                        }
                    }
                }
            }
            // group geometry / MPI / mean shot time
            Text { text: qsTr("GROUP ANALYSIS"); color: hud._txtMut; font.pixelSize: 9; font.letterSpacing: 2 }
            Row {
                spacing: 24
                visible: hud.reportGroup && hud.reportGroup.n > 0
                Column {
                    Text { text: qsTr("MPI (mm)"); color: hud._txtMut; font.pixelSize: 10 }
                    Text {
                        text: hud.reportGroup.n > 0
                            ? (Number(hud.reportGroup.mpiX).toFixed(1) + ", " + Number(hud.reportGroup.mpiY).toFixed(1))
                            : "—"
                        color: hud._txt; font.pixelSize: 14; font.bold: true
                    }
                }
                Column {
                    Text { text: qsTr("Group extent (mm)"); color: hud._txtMut; font.pixelSize: 10 }
                    Text { text: hud.reportGroup.n > 0 ? Number(hud.reportGroup.extent).toFixed(1) : "—"
                           color: hud._txt; font.pixelSize: 14; font.bold: true }
                }
                Column {
                    Text { text: qsTr("Mean shot time (s)"); color: hud._txtMut; font.pixelSize: 10 }
                    Text { text: hud.reportGroup.n > 0 ? Number(hud.reportGroup.avgSec).toFixed(1) : "—"
                           color: hud._txt; font.pixelSize: 14; font.bold: true }
                }
                Column {
                    Text { text: qsTr("Shots fired"); color: hud._txtMut; font.pixelSize: 10 }
                    Text { text: hud.reportGroup.n + " / 24"; color: hud._txt; font.pixelSize: 14; font.bold: true }
                }
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: qsTr("Single-athlete training result. Rank, elimination place and medals require competition-level ranking data (future range coordinator).")
                color: hud._txtMut; font.pixelSize: 9; topPadding: 4
            }
        }
    }
}
