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
    // F9: the completion summary is dismissible (× / Escape / Dismiss) without
    // leaving the completed result — the target + right panel stay; a "View
    // Final Summary" reopener lives in the right panel. New Final / Home leave.
    property bool dismissed: false
    signal viewReportRequested()
    signal newFinalRequested()
    signal homeRequested()
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

    // Completion-panel action dispatch. View Report keeps the summary (the
    // single-lane result IS the report — the shot-by-shot list is in the right
    // panel); New Final / Home leave via ShootingPage (durable session close).
    function doAction(key) {
        if (key === "report")    hud.viewReportRequested()
        else if (key === "new")  hud.newFinalRequested()
        else if (key === "home") hud.homeRequested()
    }

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
            hud.dismissed = false          // a fresh completion always shows
        }
        function onPhaseChanged() {
            // Reset the completion panel when a fresh course starts.
            if (hud.ctl && hud.ctl.running) { hud.completed = false; hud.dismissed = false }
        }
    }

    Timer { id: bannerTimer; interval: 2600; onTriggered: hud.checkpointBanner = "" }

    // ── Transient central flash — ONLY for the major CRO transitions the
    //    athlete must notice (TAKE YOUR POSITIONS / STOP…UNLOAD). Brief
    //    (~1.1 s), auto-dismissing, non-interactive (no MouseArea — never
    //    intercepts a shot). The PERSISTENT command + countdown lives in the
    //    right-hand Finals10mCommandPanel, NOT here (target-first design).
    // F8B: high-contrast central flash. Short label + semantic colour on a dark
    // translucent plate with a red accent border — readable over both the light
    // and black target regions, rifle and pistol faces, zoomed or not (never
    // black text on the light target; never colour alone — the word is always
    // explicit). Non-interactive (no MouseArea), auto-dismissing, a QML
    // projection only — never authoritative and never affects controller timing.
    property string flashText: ""
    property color  flashColor: "#f2f3f5"
    property bool   flashSafety: false
    Timer { id: flashTimer; interval: hud.flashSafety ? 1600 : 1000; onTriggered: hud.flashText = "" }
    function raiseFlash(label, col, safety) {
        hud.flashText = label; hud.flashColor = col; hud.flashSafety = safety
        flashTimer.restart()
    }
    Connections {
        target: hud.ctl
        function onCommandIssued(ev) {
            // Category mapping (docs: F8 command semantics). Safety/stop = red;
            // active firing = green; prep/neutral = light + red accent.
            switch (ev.typeName) {
            case "Stop":                 hud.raiseFlash(qsTr("STOP"), hud._red, true); break
            case "Unload":               hud.raiseFlash(qsTr("STOP · UNLOAD"), hud._red, true); break
            case "StartSeries":
            case "StartSingle":          hud.raiseFlash(qsTr("START · FIRE"), hud._green, false); break
            case "TakeYourPositions":    hud.raiseFlash(qsTr("TAKE YOUR POSITIONS"), hud._txt, false); break
            case "LoadSeries":
            case "LoadSingle":           hud.raiseFlash(qsTr("LOAD"), hud._txt, false); break
            default: break               // LOAD/START details, warnings etc. stay
                                         // in the persistent right-hand panel only
            }
        }
    }
    Rectangle {
        id: flashPlate
        visible: hud.flashText !== ""
        anchors.centerIn: parent
        width: Math.min(hud.width * 0.72, flashText2.implicitWidth + 56)
        height: flashText2.implicitHeight + 30
        radius: 12
        color: Qt.rgba(0, 0, 0, 0.82)
        border.color: hud._red; border.width: 2
        Text {
            id: flashText2
            anchors.centerIn: parent
            text: hud.flashText
            color: hud.flashColor; font.pixelSize: 26; font.bold: true; font.letterSpacing: 1
            horizontalAlignment: Text.AlignHCenter
            width: Math.min(hud.width * 0.64, implicitWidth); wrapMode: Text.WordWrap
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
        property bool blocking: visible
        Connections {
            target: (typeof INCIDENTS !== "undefined") ? INCIDENTS : null
            function onIncidentChanged() {
                incidentWarn.inc = INCIDENTS.activeIncident()
                // Flash DO NOT FIRE once on the transition into a blocking state
                // (safety-critical; longer dwell). The persistent warning banner
                // + command panel keep showing the blocked state afterwards.
                var nowBlocking = incidentWarn.visible
                if (nowBlocking && !incidentWarn.blocking)
                    hud.raiseFlash(qsTr("DO NOT FIRE · RANGE INCIDENT"), hud._red, true)
                incidentWarn.blocking = nowBlocking
            }
        }
        Text {
            id: incText; anchors.centerIn: parent
            text: qsTr("EST INCIDENT — official shots blocked until resolved")
            color: "#ffd7d7"; font.pixelSize: 11; font.bold: true
        }
    }

    // ── Completion result panel (dismissible; Escape closes the card only) ──
    FocusScope {
        visible: hud.completed && !hud.dismissed
        anchors.fill: parent
        focus: visible
        Keys.onEscapePressed: hud.dismissed = true    // dismiss card only —
                                                       // never deletes/closes
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(hud.width * 0.82, 580)
        height: Math.min(hud.height * 0.86, doneCol.implicitHeight + 40)
        radius: 14; color: hud._bg; border.color: hud._red; border.width: 2
        // Close (×) — dismiss the summary, keep the completed result + target.
        Rectangle {
            width: 30; height: 30; radius: 15
            anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 10
            color: closeMouse.containsMouse ? "#3a1420" : "transparent"
            Text { anchors.centerIn: parent; text: "✕"; color: hud._txtSec; font.pixelSize: 16 }
            MouseArea { id: closeMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: hud.dismissed = true }
        }
        Column {
            id: doneCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 20; spacing: 10
            Text {
                width: parent.width - 30
                text: (hud.ctl ? hud.ctl.displayName.toUpperCase() : qsTr("10M FINAL")) + qsTr(" COMPLETE")
                color: hud._red; font.pixelSize: 18; font.bold: true; font.letterSpacing: 1
                elide: Text.ElideRight
            }
            Text {
                text: qsTr("Course complete · %1 · session %2")
                      .arg(hud.ctl ? (typeof userName !== "undefined" && userName ? userName : qsTr("athlete")) : "")
                      .arg(hud.ctl && hud.ctl.sessionId.length >= 8 ? hud.ctl.sessionId.substring(0, 8) : "—")
                color: hud._txtMut; font.pixelSize: 11
            }
            Row {
                spacing: 8
                Text { text: qsTr("24-shot total"); color: hud._txtSec; font.pixelSize: 14
                       anchors.verticalCenter: parent.verticalCenter }
                Text { text: hud.fmt1(hud.ctl ? hud.ctl.cumulativeTotal : 0)
                       color: hud._green; font.pixelSize: 28; font.bold: true }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("   %1 / 24 accepted · %2 missing")
                          .arg(hud.ctl ? hud.ctl.officialShotCount : 0)
                          .arg(hud.ctl ? hud.ctl.missingShotCount : 24)
                    color: hud._txtMut; font.pixelSize: 11
                }
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
                text: qsTr("Single-lane result. Official ranking and placement require Range Management coordination.")
                color: hud._txtMut; font.pixelSize: 9; topPadding: 4
            }
            // ── exit actions (mouse / touch / keyboard) ──────────────────
            Row {
                spacing: 10; topPadding: 6
                Repeater {
                    model: [
                        { key: "report", label: qsTr("View Report"),  accent: false },
                        { key: "new",    label: qsTr("New Final"),     accent: false },
                        { key: "home",   label: qsTr("Home"),          accent: true  }
                    ]
                    Rectangle {
                        width: 132; height: 40; radius: 8
                        color: modelData.accent
                               ? (actMouse.containsMouse ? "#c40034" : "#e8003d")
                               : (actMouse.containsMouse ? "#2f3037" : "#26272c")
                        border.color: modelData.accent ? "transparent" : "#3a3b40"
                        border.width: 1
                        activeFocusOnTab: true
                        Keys.onReturnPressed: hud.doAction(modelData.key)
                        Keys.onEnterPressed: hud.doAction(modelData.key)
                        Rectangle {   // focus ring
                            anchors.fill: parent; anchors.margins: -2; radius: 10
                            color: "transparent"; border.color: hud._amber
                            border.width: 2; visible: parent.activeFocus
                        }
                        Text {
                            anchors.centerIn: parent; text: modelData.label
                            color: "white"; font.pixelSize: 13; font.bold: modelData.accent
                        }
                        MouseArea { id: actMouse; anchors.fill: parent; hoverEnabled: true
                                    onClicked: hud.doAction(modelData.key) }
                    }
                }
            }
        }
    }
    }
}
