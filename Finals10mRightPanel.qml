import QtQuick 2.15

// 10m FINAL — right-hand information column (F7). Replaces the qualification
// RightPanel while isFinals10mMatch, so no qualification series structure
// (S1–S6), stale SERIES-1 heading, zero totals or START MATCH control appear in
// a Final. Composes: the persistent command/countdown/last-shot panel (top),
// the Final-specific shot history (middle), and the score summary (bottom).
// Every value comes from FINALS10M (the sole authority); the shot list is the
// accepted-shot projection (appended on the durable shotAccepted signal), never
// inferred from target dots. Shared by Air Rifle and Air Pistol.
Item {
    id: rp
    property var ctl                 // FINALS10M

    readonly property color _bg:     "#1f2026"
    readonly property color _bgAlt:  "#191a1f"
    readonly property color _red:    "#e8003d"
    readonly property color _txt:    "#f2f3f5"
    readonly property color _txtSec: "#c9ced6"
    readonly property color _txtMut: "#9aa0aa"
    readonly property color _amber:  "#e8a13d"
    readonly property color _green:  "#43b581"

    property bool blocked: false
    function refreshBlocked() {
        rp.blocked = (rp.ctl && rp.ctl.officialsBlockedNow()) === true
    }

    // F9: mirrors the completion card's dismissed state so a "View Final
    // Summary" reopener can appear after the operator dismisses the card.
    property bool summaryDismissed: false
    signal reopenSummaryRequested()

    function fmt1(v) { return (v === undefined || v === null || v < 0) ? "0.0" : Number(v).toFixed(1) }

    // Accepted-shot projection (durable). Rebuilt on recovery because
    // loadRecoveredState re-emits shotAccepted for every recovered shot.
    ListModel { id: shotModel }

    // Heading follows the current Final stage (never a stale series).
    readonly property string historyHeading: {
        if (!ctl) return qsTr("SHOTS")
        // techaim::finals10m::Stage: Idle 0, Presentation 1, PrepSighting 2,
        // Series1 3, Series2 4, Singles 5, Complete 6, Aborted 7.
        switch (ctl.stageId) {
        case 3: return qsTr("SERIES 1")
        case 4: return qsTr("SERIES 2")
        case 5: return qsTr("SINGLE SHOTS")
        case 6: return qsTr("FINAL — ALL SHOTS")
        default: return qsTr("SIGHTERS")   // Idle / Presentation / PrepSighting
        }
    }

    Connections {
        target: rp.ctl
        function onShotAccepted(shot) {
            shotModel.append({
                num: shot.finalShotNumber === undefined ? 0 : shot.finalShotNumber,
                score: shot.calculatedscore === undefined ? "" : String(shot.calculatedscore),
                sighter: shot.sighter === true,
                timeSec: shot.timeSec === undefined ? 0 : shot.timeSec,
                seriesIndex: shot.seriesIndex === undefined ? -1 : shot.seriesIndex
            })
            shotList.positionViewAtEnd()
        }
        function onPhaseChanged()   { rp.refreshBlocked() }
        function onShotCountsChanged() { rp.refreshBlocked() }
    }
    Connections {
        target: (typeof INCIDENTS !== "undefined") ? INCIDENTS : null
        function onIncidentChanged() { rp.refreshBlocked() }
    }
    // Cleared and refilled by the parent on fresh start / recovery via reset().
    function reset() { shotModel.clear(); rp.refreshBlocked() }

    // ── F9: View Final Summary reopener (after the completion card is
    //    dismissed) — a small bar at the top of the right column ───────────
    Rectangle {
        id: reopenBar
        visible: rp.ctl && rp.ctl.complete && rp.summaryDismissed
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: parent.top; anchors.margins: 10
        height: visible ? 34 : 0
        radius: 8; color: reopenMouse.containsMouse ? "#2f3037" : "#26272c"
        border.color: rp._red; border.width: 1
        Text {
            anchors.centerIn: parent
            text: qsTr("▸ View Final Summary")
            color: rp._txt; font.pixelSize: 12; font.bold: true
        }
        MouseArea { id: reopenMouse; anchors.fill: parent; hoverEnabled: true
                    onClicked: rp.reopenSummaryRequested() }
    }

    // ── command / countdown / last shot ─────────────────────────────────
    Finals10mCommandPanel {
        id: cmd
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: reopenBar.visible ? reopenBar.bottom : parent.top
        anchors.margins: 10
        ctl: rp.ctl
        blocked: rp.blocked
    }

    // ── score summary (bottom, fixed) ───────────────────────────────────
    Rectangle {
        id: summary
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        height: sumCol.implicitHeight + 20
        radius: 10; color: rp._bg
        Column {
            id: sumCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 10; spacing: 8
            Row {
                width: parent.width
                Text { text: qsTr("SCORE SUMMARY"); color: rp._txtMut; font.pixelSize: 9; font.letterSpacing: 2
                       anchors.verticalCenter: parent.verticalCenter }
                Item { width: parent.width - 200; height: 1 }
                Text {
                    text: (rp.ctl ? rp.ctl.officialShotCount : 0) + " / " + (rp.ctl ? rp.ctl.maximumMatchShots : 24)
                          + qsTr(" shots")
                    color: rp._txtSec; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Row {
                width: parent.width
                Repeater {
                    model: [
                        { k: qsTr("Series 1"), v: rp.ctl ? rp.ctl.series1Subtotal : 0 },
                        { k: qsTr("Series 2"), v: rp.ctl ? rp.ctl.series2Subtotal : 0 },
                        { k: qsTr("Singles"),  v: rp.ctl ? rp.ctl.singlesSubtotal : 0 }
                    ]
                    Column {
                        width: parent.width / 4
                        Text { text: modelData.k; color: rp._txtMut; font.pixelSize: 10 }
                        Text { text: rp.fmt1(modelData.v); color: rp._txt; font.pixelSize: 15; font.bold: true }
                    }
                }
                Column {
                    width: parent.width / 4
                    Text { text: qsTr("TOTAL"); color: rp._txtMut; font.pixelSize: 10; font.bold: true }
                    Text { text: rp.fmt1(rp.ctl ? rp.ctl.cumulativeTotal : 0)
                           color: rp._green; font.pixelSize: 17; font.bold: true }
                }
            }
        }
    }

    // ── shot history (middle, flexible) ─────────────────────────────────
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: cmd.bottom; anchors.bottom: summary.top
        anchors.margins: 10
        radius: 10; color: rp._bg
        Column {
            anchors.fill: parent; anchors.margins: 10; spacing: 6
            Item {
                width: parent.width; height: 18
                Text {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                    text: rp.historyHeading; color: rp._txt
                    font.pixelSize: 13; font.bold: true; font.letterSpacing: 1
                }
                Row {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    spacing: 14
                    Text { text: qsTr("Score"); color: rp._txtMut; font.pixelSize: 10 }
                    Text { text: qsTr("Time"); color: rp._txtMut; font.pixelSize: 10 }
                }
            }
            Rectangle { width: parent.width; height: 1; color: "#33343c" }
            ListView {
                id: shotList
                width: parent.width
                height: parent.height - 30
                clip: true
                model: shotModel
                delegate: Row {
                    width: shotList.width
                    height: 24
                    // checkpoint positions 12/14/16/18/20/22/24 subtly marked
                    readonly property bool isCheckpoint:
                        !model.sighter && (model.num === 12 || model.num === 14 || model.num === 16
                            || model.num === 18 || model.num === 20 || model.num === 22 || model.num === 24)
                    Text {
                        width: parent.width * 0.5
                        text: model.sighter ? qsTr("S")
                              : ("#" + model.num + (isCheckpoint ? "  ·  ckpt" : ""))
                        color: model.sighter ? rp._txtMut : (isCheckpoint ? rp._amber : rp._txtSec)
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        width: parent.width * 0.28
                        text: model.score
                        color: model.sighter ? rp._txtMut : rp._txt
                        font.pixelSize: 13; font.bold: !model.sighter
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        width: parent.width * 0.22
                        text: model.timeSec + "s"
                        color: rp._txtMut; font.pixelSize: 11
                        horizontalAlignment: Text.AlignRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
