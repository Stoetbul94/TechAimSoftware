import QtQuick 2.15

// 10m FINAL — right-hand command / countdown / last-shot panel (F7).
//
// The persistent CRO command and countdown live HERE, in the right-hand
// information column — never over the target. Pure presentation over FINALS10M
// (the sole authority): no value is recalculated in QML. Keeps the target a
// clean visual workspace (target-first design). Shared by Air Rifle and Air
// Pistol — only the discipline name / target face / positioning interval
// differ, all resolved inside FINALS10M.
Item {
    id: panel
    property var ctl                 // FINALS10M
    // True while an unresolved EST incident blocks official shots. Refreshed by
    // the parent on INCIDENTS.incidentChanged (kept out of this leaf component).
    property bool blocked: false

    readonly property color _bg:     "#1f2026"
    readonly property color _bgAlt:  "#24262e"
    readonly property color _red:    "#e8003d"
    readonly property color _txt:    "#f2f3f5"
    readonly property color _txtSec: "#c9ced6"
    readonly property color _txtMut: "#9aa0aa"
    readonly property color _amber:  "#e8a13d"
    readonly property color _green:  "#43b581"

    readonly property int _win: ctl ? ctl.windowState : 0        // 0 closed 1 sight 2 match
    readonly property bool _firing: ctl ? ctl.isFiringWindowOpen : false
    readonly property bool _complete: ctl ? (ctl.stageId === 6) : false  // Stage::Complete

    // One deterministic status: colour + short wording (never colour alone).
    readonly property color statusColor:
        panel.blocked ? _red
      : _complete    ? _green
      : _firing      ? _green
      : (ctl && ctl.running) ? _amber
      : _txtMut
    readonly property string statusWord:
        panel.blocked ? qsTr("FIRING BLOCKED — EST INCIDENT")
      : _complete    ? qsTr("SINGLE-LANE RESULT COMPLETE")
      : _win === 2   ? qsTr("MATCH WINDOW OPEN — FIRE")
      : _win === 1   ? qsTr("SIGHTING OPEN — FIRE")
      : (ctl && ctl.running) ? qsTr("STAND BY — HOLD")
      : qsTr("WAITING")

    function fmt1(v) { return (v === undefined || v === null || v < 0) ? "—" : Number(v).toFixed(1) }

    height: col.implicitHeight + 24

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: panel._bg
        border.color: panel.blocked ? panel._red : "transparent"
        border.width: panel.blocked ? 2 : 0
    }

    Column {
        id: col
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        // discipline name (the only visible AR/AP text difference besides the
        // target face) — keeps the athlete's discipline unambiguous now that the
        // on-target strip is gone.
        Text {
            width: parent.width
            text: panel.ctl ? panel.ctl.displayName : qsTr("10m Final")
            color: panel._red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
            elide: Text.ElideRight
        }

        // stage + step dots
        Row {
            width: parent.width
            Text {
                text: panel.ctl ? panel.ctl.stageLabel : ""
                color: panel._txt; font.pixelSize: 14; font.bold: true
                elide: Text.ElideRight
                width: parent.width - stepRow.width - 8
                anchors.verticalCenter: parent.verticalCenter
            }
            Row {
                id: stepRow
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Repeater {
                    model: panel.ctl ? panel.ctl.stepLabels() : []
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: (panel.ctl && panel.ctl.stepIndex === index) ? panel._red : panel._bgAlt
                    }
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: "#33343c" }

        // big command (one clean completion line — not the raw ISSF command)
        Text {
            width: parent.width
            text: panel.blocked
                  ? qsTr("OFFICIAL SHOTS BLOCKED")
                  : panel._complete
                    ? qsTr("FINAL COMPLETE")
                    : (panel.ctl && panel.ctl.commandText.length ? panel.ctl.commandText : qsTr("—"))
            color: panel._txt
            font.pixelSize: 18; font.bold: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // completed subtitle (avoids any official-placement wording)
        Text {
            visible: panel._complete
            width: parent.width
            text: qsTr("Course complete · %1 shot positions completed")
                  .arg(panel.ctl ? panel.ctl.maximumMatchShots : 24)
            color: panel._txtMut; font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }

        // countdown (kept visible during a live firing window)
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: panel.ctl && panel.ctl.remainingMs > 0 && !panel._complete
            text: panel.ctl ? panel.ctl.remainingFormatted : "00:00"
            color: panel.statusColor
            font.pixelSize: 40; font.bold: true
        }

        // window-state line (text + colour, not colour alone)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            Rectangle {
                width: 10; height: 10; radius: 5
                color: panel.statusColor
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: panel.statusWord
                color: panel.statusColor
                font.pixelSize: 12; font.bold: true; font.letterSpacing: 1
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Rectangle { width: parent.width; height: 1; color: "#33343c" }

        // last OFFICIAL shot (authoritative — from FINALS10M; — when none, so a
        // sighter is never shown as the Final's last official result).
        Row {
            width: parent.width
            Column {
                width: parent.width * 0.5
                Text { text: qsTr("LAST OFFICIAL SHOT"); color: panel._txtMut; font.pixelSize: 9; font.letterSpacing: 1 }
                Text {
                    text: (panel.ctl && panel.ctl.lastShotScore >= 0)
                          ? panel.fmt1(panel.ctl.lastShotScore) : "—"
                    color: panel._green; font.pixelSize: 22; font.bold: true
                }
            }
            Column {
                width: parent.width * 0.5
                Text { text: qsTr("SHOT · TIME"); color: panel._txtMut; font.pixelSize: 9; font.letterSpacing: 1
                       anchors.right: parent.right }
                Text {
                    anchors.right: parent.right
                    text: (!panel.ctl || panel.ctl.lastShotScore < 0) ? "—"
                          : ("#" + panel.ctl.lastShotNumber + " · " + panel.ctl.lastShotTimeSec + "s")
                    color: panel._txtSec; font.pixelSize: 14; font.bold: true
                }
            }
        }
    }
}
