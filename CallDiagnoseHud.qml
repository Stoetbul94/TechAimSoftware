import QtQuick 2.15

// Call & Diagnose (T2) — call-entry, reveal and summary overlay. The ACTUAL
// impact is never shown until the call is confirmed. During AwaitingCall the
// athlete taps a dedicated call target to place their CALL; on Confirm the
// actual is revealed with the call, a vector between them and the measured
// difference. Measured language only — no technical-cause claims.
Item {
    id: hud
    property var ctl: null                 // CALLDIAG
    property real rangeMm: 30              // call-target half-range (set per discipline)
    signal homeRequested()
    signal newSessionRequested()
    signal exportPdfRequested()

    readonly property bool awaitingCall: ctl && ctl.phase === 3
    readonly property bool revealOpen:   ctl && ctl.phase === 4
    readonly property bool summaryOpen:  ctl && ctl.phase === 5
    // T2.1: reveal view mode — false = Comparison Zoom (auto-fit call+actual),
    // true = Target View (whole face). Reset to Comparison each reveal.
    property bool targetView: false
    onRevealOpenChanged: if (revealOpen) targetView = false

    readonly property color _bg:     "#15171C"
    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _blue:   "#3DA9FC"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    // ── CALL ENTRY + REVEAL ──────────────────────────────────────────────
    Rectangle {
        visible: hud.awaitingCall || hud.revealOpen
        anchors.fill: parent
        color: "#C60F1116"
        MouseArea { anchors.fill: parent }        // modal: the live target is behind

        Column {
            anchors.centerIn: parent; spacing: 12
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: hud.awaitingCall ? "SHOT RECEIVED" : "CALL vs ACTUAL"
                color: _txt; font.pixelSize: 22; font.bold: true; font.letterSpacing: 1
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 520; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                color: _txtSec; font.pixelSize: 13
                text: hud.awaitingCall
                      ? "Mark where you believe the shot landed, then confirm your call."
                      : "CALL (outline) vs ACTUAL (filled). The line shows the difference."
            }

            // the call target face
            Rectangle {
                id: face
                width: 360; height: 360; radius: 8
                color: "#0C0E12"; border.color: _line; border.width: 1
                readonly property real cx: width / 2
                readonly property real cy: height / 2
                readonly property real plotR: width / 2 - 18
                property var rv: hud.revealOpen && hud.ctl ? hud.ctl.revealCurrent() : ({})
                // pending call marker position (mm), set on tap
                property bool hasCall: hud.ctl ? hud.ctl.hasPendingCall : false
                property real callMmX: 0
                property real callMmY: 0
                // adaptive half-range (mm): during Reveal the controller supplies an
                // auto-fit range so neither marker is ever clipped; during call
                // entry a fixed discipline range is used for placement.
                readonly property real activeRange: hud.awaitingCall
                    ? hud.rangeMm
                    : (hud.targetView ? (rv.targetHalfRangeMm || hud.rangeMm)
                                      : (rv.comparisonHalfRangeMm || hud.rangeMm))

                function mmToX(mm) { return cx + (mm / activeRange) * plotR }
                function mmToYpix(mm) { return cy - (mm / activeRange) * plotR }

                // reference rings
                Repeater {
                    model: [1.0, 0.66, 0.33]
                    Rectangle { width: face.plotR * 2 * modelData; height: width; radius: width / 2
                        anchors.centerIn: parent; color: "transparent"; border.color: "#23262d"; border.width: 1 }
                }
                Rectangle { anchors.centerIn: parent; width: face.plotR * 2; height: 1; color: "#1c1f26" }
                Rectangle { anchors.centerIn: parent; width: 1; height: face.plotR * 2; color: "#1c1f26" }
                Text { anchors.top: parent.top; anchors.topMargin: 6; anchors.horizontalCenter: parent.horizontalCenter
                       text: "±" + face.activeRange.toFixed(0) + " mm"; color: _txtMut; font.pixelSize: 9 }
                // off-face indicator (a marker lies beyond the scoring face)
                Text {
                    visible: hud.revealOpen && face.rv.outsideFace === true
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 6; anchors.horizontalCenter: parent.horizontalCenter
                    text: "OUTSIDE NORMAL TARGET FACE"; color: _redHi; font.pixelSize: 9; font.bold: true
                }

                // ACTUAL marker (reveal only) — filled diamond + label
                Item {
                    visible: hud.revealOpen && face.rv.actualXMm !== undefined
                    x: face.mmToX(face.rv.actualXMm !== undefined ? face.rv.actualXMm : 0) - 8
                    y: face.mmToYpix(face.rv.actualYMm !== undefined ? face.rv.actualYMm : 0) - 8
                    width: 16; height: 16
                    Rectangle { anchors.centerIn: parent; width: 13; height: 13; radius: 2; rotation: 45
                                color: _red; border.color: "white"; border.width: 1 }
                }
                // line between call and actual (reveal)
                Canvas {
                    id: vec; anchors.fill: parent; visible: hud.revealOpen && face.rv.actualXMm !== undefined
                    onPaint: {
                        var c = getContext("2d"); c.reset()
                        if (face.rv.actualXMm === undefined) return
                        c.strokeStyle = "#8fa0b5"; c.lineWidth = 1.5; c.setLineDash([4, 3])
                        c.beginPath()
                        c.moveTo(face.mmToX(face.rv.calledXMm), face.mmToYpix(face.rv.calledYMm))
                        c.lineTo(face.mmToX(face.rv.actualXMm), face.mmToYpix(face.rv.actualYMm))
                        c.stroke()
                    }
                    Connections { target: face; function onRvChanged() { vec.requestPaint() } }
                }
                // CALL marker — outlined circle (pending during entry, fixed on reveal)
                Item {
                    visible: hud.revealOpen ? (face.rv.calledXMm !== undefined) : face.hasCall
                    x: face.mmToX(hud.revealOpen ? (face.rv.calledXMm !== undefined ? face.rv.calledXMm : 0) : face.callMmX) - 9
                    y: face.mmToYpix(hud.revealOpen ? (face.rv.calledYMm !== undefined ? face.rv.calledYMm : 0) : face.callMmY) - 9
                    width: 18; height: 18
                    Rectangle { anchors.centerIn: parent; width: 15; height: 15; radius: 8
                                color: "transparent"; border.color: _blue; border.width: 3 }
                }

                // tap to place the call (AwaitingCall only)
                MouseArea {
                    anchors.fill: parent
                    enabled: hud.awaitingCall
                    onClicked: {
                        var mmX = (mouseX - face.cx) / face.plotR * hud.rangeMm
                        var mmY = -(mouseY - face.cy) / face.plotR * hud.rangeMm
                        face.callMmX = mmX; face.callMmY = mmY
                        if (hud.ctl) hud.ctl.submitCall(mmX, mmY)
                    }
                }
            }
            // reveal difference (full sentences + exact-call state)
            Column {
                visible: hud.revealOpen; anchors.horizontalCenter: parent.horizontalCenter; spacing: 2
                property var rv: hud.revealOpen && hud.ctl ? hud.ctl.revealCurrent() : ({})
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       visible: parent.rv.exact === true
                       text: "EXACT CALL — 0.0 mm"; color: _green; font.pixelSize: 16; font.bold: true }
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       visible: parent.rv.exact !== true
                       text: "CALL DIFFERENCE  " + (parent.rv.errorMm !== undefined ? Number(parent.rv.errorMm).toFixed(1) : "—") + " mm"
                             + (parent.rv.errorRingSpacings !== undefined ? "  (~" + Number(parent.rv.errorRingSpacings).toFixed(1) + " ring spacings)" : "")
                       color: _txt; font.pixelSize: 15; font.bold: true }
                Text { anchors.horizontalCenter: parent.horizontalCenter; width: 520; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                       text: (parent.rv.horizontalSentence || ""); color: _txtSec; font.pixelSize: 12 }
                Text { anchors.horizontalCenter: parent.horizontalCenter; width: 520; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                       text: (parent.rv.verticalSentence || ""); color: _txtSec; font.pixelSize: 12 }
            }
            // legend + view toggle
            Row {
                anchors.horizontalCenter: parent.horizontalCenter; spacing: 20
                Row { spacing: 6
                    Rectangle { width: 13; height: 13; radius: 8; color: "transparent"; border.color: _blue; border.width: 3
                                anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "CALL"; color: _txtSec; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                Row { spacing: 6; visible: hud.revealOpen
                    Rectangle { width: 11; height: 11; radius: 2; rotation: 45; color: _red; border.color: "white"; border.width: 1
                                anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "ACTUAL"; color: _txtSec; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                // Comparison Zoom / Target View toggle (touch ≥48px targets)
                Row { spacing: 0; visible: hud.revealOpen
                    Rectangle { width: 120; height: 30; radius: 6; color: !hud.targetView ? _redHi : "transparent"; border.color: _line; border.width: 1
                        Text { anchors.centerIn: parent; text: "Comparison"; color: !hud.targetView ? "white" : _txtSec; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: hud.targetView = false } }
                    Rectangle { width: 110; height: 30; radius: 6; color: hud.targetView ? _redHi : "transparent"; border.color: _line; border.width: 1
                        Text { anchors.centerIn: parent; text: "Target"; color: hud.targetView ? "white" : _txtSec; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: hud.targetView = true } }
                }
            }

            // actions
            Row {
                anchors.horizontalCenter: parent.horizontalCenter; spacing: 12
                // AwaitingCall: Clear + Confirm
                Rectangle {
                    visible: hud.awaitingCall; width: 130; height: 56; radius: 8
                    color: "transparent"; border.color: _line; border.width: 1
                    Text { anchors.centerIn: parent; text: "Clear"; color: _txtSec; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: if (hud.ctl) hud.ctl.clearCall() }
                }
                Rectangle {
                    visible: hud.awaitingCall; width: 200; height: 56; radius: 8
                    property bool ready: hud.ctl && hud.ctl.hasPendingCall
                    color: ready ? (confirmMouse.pressed ? _red : _redHi) : "#2A2C34"
                    Text { anchors.centerIn: parent; text: "CONFIRM CALL"
                           color: parent.ready ? "white" : _txtMut; font.pixelSize: 15; font.bold: true }
                    MouseArea { id: confirmMouse; anchors.fill: parent
                                onClicked: if (hud.ctl && hud.ctl.hasPendingCall) hud.ctl.confirmCall() }
                }
                // Reveal: Continue
                Rectangle {
                    visible: hud.revealOpen; width: 260; height: 56; radius: 8
                    color: contMouse.pressed ? "#A80038" : _red
                    Text { anchors.centerIn: parent
                           text: (hud.ctl && hud.ctl.shotsCompleted >= hud.ctl.shotCount)
                                 ? "CONTINUE" : "CONTINUE TO NEXT SHOT"
                           color: "white"; font.pixelSize: 15; font.bold: true }
                    MouseArea { id: contMouse; anchors.fill: parent
                                onClicked: if (hud.ctl) hud.ctl.continueToNext() }
                }
            }
        }
    }

    // ── SUMMARY ──────────────────────────────────────────────────────────
    Rectangle {
        visible: hud.summaryOpen
        anchors.fill: parent; color: "#EA0F1116"
        MouseArea { anchors.fill: parent }
        Rectangle {
            width: Math.min(1000, parent.width - 40); height: Math.min(760, parent.height - 30)
            anchors.centerIn: parent; color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22; contentHeight: sumCol.implicitHeight; clip: true
                Column {
                    id: sumCol; width: parent.width; spacing: 10
                    property var st: (hud.summaryOpen && hud.ctl) ? hud.ctl.sessionStats(-1) : ({})
                    property var obs: (hud.summaryOpen && hud.ctl) ? hud.ctl.sessionObservations() : []
                    property var shots: (hud.summaryOpen && hud.ctl) ? hud.ctl.shotReviewList() : []
                    property var ins: (hud.summaryOpen && hud.ctl) ? hud.ctl.callInsights() : ({})

                    Text { text: "CALL & DIAGNOSE COMPLETE"; color: _txt; font.pixelSize: 22; font.bold: true }
                    Text { text: "TRAINING SESSION"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
                    Rectangle { width: notOff.implicitWidth + 22; height: 24; radius: 12; color: "#2a0b10"; border.color: _red; border.width: 1
                        Text { id: notOff; anchors.centerIn: parent; text: "Not an official competition result"
                               color: "#ffd0d7"; font.pixelSize: 10; font.bold: true } }

                    // overview cards
                    Grid {
                        width: parent.width; columns: 4; columnSpacing: 10; rowSpacing: 10; topPadding: 6
                        Repeater {
                            model: {
                                var s = sumCol.st
                                if (!s || s.count === undefined) return []
                                return [
                                    { l: "CALLED SHOTS", v: "" + s.count },
                                    { l: "AVG CALL ERROR", v: Number(s.averageError).toFixed(1) + " mm" },
                                    { l: "MEDIAN", v: Number(s.medianError).toFixed(1) + " mm" },
                                    { l: "BEST CALL", v: Number(s.smallestError).toFixed(1) + " mm" },
                                    { l: "LARGEST", v: Number(s.largestError).toFixed(1) + " mm" },
                                    { l: "CONSISTENCY (SD)", v: Number(s.errorStdDev).toFixed(2) },
                                    { l: "X BIAS", v: (Math.abs(s.biasX).toFixed(1)) + (s.biasX >= 0 ? " R" : " L") },
                                    { l: "Y BIAS", v: (Math.abs(s.biasY).toFixed(1)) + (s.biasY >= 0 ? " Hi" : " Lo") }
                                ]
                            }
                            delegate: Rectangle {
                                width: (sumCol.width - 30) / 4; height: 60; radius: 8; color: "#1D2026"; border.color: _line; border.width: 1
                                Column { anchors.fill: parent; anchors.margins: 8; spacing: 3
                                    Text { text: modelData.l; color: _txtMut; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1 }
                                    Text { text: modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 15; font.bold: true } }
                            }
                        }
                    }

                    // ── WHAT YOU SHOULD TAKE FROM THIS SESSION ──────────────
                    Rectangle {
                        visible: sumCol.ins.hasData === true
                        width: parent.width; radius: 8; color: "#12161C"; border.color: _line; border.width: 1
                        height: takeCol.implicitHeight + 24
                        Column {
                            id: takeCol
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12
                            spacing: 8
                            Text { text: "WHAT YOU SHOULD TAKE FROM THIS SESSION"; color: _redHi; font.pixelSize: 12; font.bold: true; font.letterSpacing: 1 }
                            Repeater {
                                model: {
                                    var i = sumCol.ins
                                    if (!i || !i.hasData) return []
                                    var m = []
                                    m.push({ t: "TYPICAL CALL ACCURACY", b: (i.typicalText || "") + "  " + (i.withinText || "") })
                                    m.push({ t: "AVERAGE vs TYPICAL", b: i.averageVsMedian || "" })
                                    var biasStr = (i.biasSentences || []).join("  ") + "  " + (i.biasCaveat || "")
                                    m.push({ t: "DIRECTIONAL BIAS", b: biasStr })
                                    m.push({ t: "CALL CONSISTENCY", b: i.consistencyText || "" })
                                    m.push({ t: "CALL ACCURACY TREND", b: i.trendText || "" })
                                    m.push({ t: "SHOT AWARENESS vs SHOT RESULT", b: i.awarenessText || "" })
                                    return m
                                }
                                delegate: Column { width: takeCol.width; spacing: 2
                                    Text { text: modelData.t; color: _green; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                                    Text { width: parent.width; wrapMode: Text.WordWrap; text: modelData.b; color: _txtSec; font.pixelSize: 12 } }
                            }
                        }
                    }

                    // SHOTS TO REVIEW WITH YOUR COACH
                    Column {
                        width: parent.width; spacing: 3
                        visible: (sumCol.ins.reviewShots || []).length > 0
                        Text { text: "SHOTS TO REVIEW WITH YOUR COACH"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                        Repeater { model: sumCol.ins.reviewShots || []
                            Text { width: sumCol.width; wrapMode: Text.WordWrap
                                   text: "· Shot " + modelData.shotNumber + ": " + modelData.text
                                   color: _txtSec; font.pixelSize: 12 } }
                    }

                    // ACCURACY TREND CHART (shot number vs call difference)
                    Text { text: "CALL DIFFERENCE BY SHOT"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column {
                        width: parent.width; spacing: 3
                        readonly property real maxErr: {
                            var mx = 1
                            for (var i = 0; i < sumCol.shots.length; ++i) mx = Math.max(mx, sumCol.shots[i].errorMm)
                            return mx
                        }
                        Repeater { model: sumCol.shots
                            delegate: Row { spacing: 8; property var s: modelData
                                Text { text: (s.positionName ? s.positionName.substring(0,1) + " " : "") + "#" + s.shotNumber
                                       color: _txtSec; font.pixelSize: 9; width: 40; anchors.verticalCenter: parent.verticalCenter }
                                Rectangle { width: sumCol.width * 0.55; height: 11; radius: 3; color: "#0E1014"; anchors.verticalCenter: parent.verticalCenter
                                    Rectangle { height: parent.height; radius: 3; color: s.isOutlier ? _redHi : _green
                                        width: parent.width * Math.min(1, s.errorMm / parent.parent.parent.maxErr) } }
                                Text { text: Number(s.errorMm).toFixed(1) + " mm"; color: _txt; font.family: "Consolas"; font.pixelSize: 9
                                       anchors.verticalCenter: parent.verticalCenter } } }
                    }

                    Text { text: "OBSERVED"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column { width: parent.width; spacing: 3
                        Repeater { model: sumCol.obs
                            Text { width: sumCol.width; wrapMode: Text.WordWrap; text: "· " + modelData; color: _txtSec; font.pixelSize: 12 } }
                        Text { visible: sumCol.obs.length === 0; text: "Not enough called shots to summarise."
                               color: _txtMut; font.pixelSize: 12 } }

                    // shot review (called vs actual, error) — revealed shots only
                    Text { text: "SHOT REVIEW"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column { width: parent.width; spacing: 3
                        Row { spacing: 8
                            Text { text: "SHOT"; color: _txtMut; font.pixelSize: 9; width: 70 }
                            Text { text: "SCORE"; color: _txtMut; font.pixelSize: 9; width: 60 }
                            Text { text: "CALL ERR"; color: _txtMut; font.pixelSize: 9; width: 70 }
                            Text { text: "X / Y"; color: _txtMut; font.pixelSize: 9; width: 120 }
                            Text { text: "NOTE"; color: _txtMut; font.pixelSize: 9 } }
                        Repeater { model: sumCol.shots
                            delegate: Row { spacing: 8; property var s: modelData
                                Text { width: 70; color: _txt; font.pixelSize: 11; font.bold: true
                                       text: (s.positionName ? s.positionName.substring(0,1) + " " : "") + "#" + s.shotNumber }
                                Text { width: 60; color: _txt; font.family: "Consolas"; font.pixelSize: 11; text: Number(s.actualScore).toFixed(1) }
                                Text { width: 70; color: _txt; font.family: "Consolas"; font.pixelSize: 11; text: Number(s.errorMm).toFixed(1) + " mm" }
                                Text { width: 120; color: _txtSec; font.family: "Consolas"; font.pixelSize: 11
                                       text: Number(s.errorXMm).toFixed(1) + " / " + Number(s.errorYMm).toFixed(1) }
                                Text { color: _txtMut; font.pixelSize: 10; elide: Text.ElideRight; width: 180; text: s.note || "" } } }
                    }

                    Row {
                        width: parent.width; spacing: 10; topPadding: 12
                        Rectangle { width: 180; height: 56; radius: 8; color: pdfMouse.pressed ? "#2A2C34" : "#23252C"; border.color: _line; border.width: 1
                            Row { anchors.centerIn: parent; spacing: 8
                                Text { text: "⭳"; color: _green; font.pixelSize: 18; anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "EXPORT PDF"; color: _txt; font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter } }
                            MouseArea { id: pdfMouse; anchors.fill: parent; onClicked: hud.exportPdfRequested() } }
                        Rectangle { width: 160; height: 56; radius: 8; color: nsMouse.pressed ? "#A80038" : _red
                            Text { anchors.centerIn: parent; text: "NEW SESSION"; color: "white"; font.pixelSize: 14; font.bold: true }
                            MouseArea { id: nsMouse; anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("Start a new session?"),
                                    qsTr("This session is closed and preserved. A new session ID is created."),
                                    function(ok) { if (ok) hud.newSessionRequested() }, qsTr("New Session"), qsTr("Cancel")) } }
                        Rectangle { width: 120; height: 56; radius: 8; color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "Home"; color: _txtSec; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent; onClicked: hud.homeRequested() } }
                    }
                }
            }
        }
    }
}
