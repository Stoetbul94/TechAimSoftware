import QtQuick 2.15

// Wind Map (Release 2) — the TRAINING top bar. UI-WIND-001.
//
// Occupies the same 42 px band as the competition `statusStrip`, which is
// suppressed for Wind Map. Every anchor in ShootingPage still chains from
// statusStrip.bottom, so nothing below moves.
//
// WHY THIS EXISTS. The competition strip carried no gate of its own and its
// rows gated only on the Finals/Technical-Blocks flags, so a Wind Map session
// inherited whatever the last selected event card had set — "FINAL 35", a
// 0 / 35 counter and the SIGHTING/MATCH stepper. A Training Lab programme
// must never present as an ISSF Final.
//
// THIS BAR BINDS TO WINDMAP AND NOTHING ELSE. It reads no Finals controller,
// no Final phase, stage, timer, ceremony, command or official shot count, and
// no qualification model (globalMatchModel / matchShootCount). Its shot
// progress is the Wind Map controller's own counted-shot progress.
Item {
    id: bar
    property var ctl: null                  // WINDMAP
    property string athlete: ""
    property bool demoMode: false

    readonly property bool inSighters: ctl && ctl.phase === 2
    readonly property bool counting:   ctl && ctl.phase === 3

    readonly property color _bg:     "#15161a"
    readonly property color _line:   "#3a3b40"
    readonly property color _red:    "#C40046"
    readonly property color _green:  "#20C997"
    readonly property color _amber:  "#E0A800"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#9a9ba0"

    // The phase label. Wind Map has exactly two shooting phases; everything
    // else reports itself plainly rather than borrowing a competition word.
    function phaseLabel() {
        if (!ctl) return ""
        if (bar.inSighters) return qsTr("SIGHTERS")
        if (bar.counting)   return qsTr("COUNTED SHOTS")
        return ctl.phaseName.toUpperCase()
    }

    Rectangle { anchors.fill: parent; color: bar._bg }

    // ── identity ────────────────────────────────────────────────────────
    Row {
        anchors.left: parent.left; anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Text {
            text: qsTr("TRAINING LAB")
            color: bar._red; font.family: theme.fontFamily
            font.pixelSize: 11; font.bold: true; font.letterSpacing: 2
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: qsTr("WIND MAP")
            color: bar._txt; font.family: theme.fontFamily
            font.pixelSize: 14; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle { width: 1; height: 18; color: bar._line; anchors.verticalCenter: parent.verticalCenter }
        Text {
            text: bar.athlete
            color: bar._txtSec; font.family: theme.fontFamily; font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle { width: 1; height: 18; color: bar._line; anchors.verticalCenter: parent.verticalCenter }
        Text {
            // The DISCIPLINE, from the controller's own configuration — never
            // from the competition event display.
            text: ctl ? (ctl.threePositions ? qsTr("50m Rifle 3 Positions")
                                            : qsTr("50m Rifle Prone")) : ""
            color: bar._txtSec; font.family: theme.fontFamily; font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        // 3P only: the current TRAINING position and how far through it is.
        // Not a Final stage chip — it is a WindMapController projection and
        // changing it is a WindMapController action.
        Rectangle {
            visible: ctl && ctl.threePositions
            radius: 10; height: 20; width: posText.implicitWidth + 18
            color: "#0d2018"; border.color: bar._green; border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: posText
                anchors.centerIn: parent
                text: ctl ? ctl.positionName.toUpperCase() : ""
                color: bar._green; font.family: theme.fontFamily
                font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
            }
        }
    }

    // ── the honesty line ────────────────────────────────────────────────
    // Present on the capture screen at all times, not only in the review.
    Text {
        anchors.centerIn: parent
        text: qsTr("NOT AN OFFICIAL COMPETITION RESULT")
        color: bar._amber; font.family: theme.fontFamily
        font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.5
    }

    // ── progress + mode ─────────────────────────────────────────────────
    Row {
        anchors.right: parent.right; anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // Wind Map's OWN progress. Sighters are counted as sighters; the
        // counted-shot progress is per position in 3P, matching the panel.
        Text {
            text: bar.inSighters
                  ? (ctl ? (ctl.sighterCount + " " + qsTr("SIGHTERS")) : "")
                  : (ctl ? (ctl.countedShots + " / " + ctl.shotPlan) : "")
            color: bar._txt; font.family: theme.fontFamily
            font.pixelSize: 14; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: !bar.inSighters
            text: qsTr("COUNTED")
            color: bar._txtSec; font.family: theme.fontFamily
            font.pixelSize: 9; font.letterSpacing: 1.5
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            radius: 4; height: 24; width: phaseT.implicitWidth + 20
            anchors.verticalCenter: parent.verticalCenter
            color: bar.counting ? bar._red : "#8a6d00"
            Text {
                id: phaseT
                anchors.centerIn: parent
                text: bar.phaseLabel()
                color: "white"; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
            }
        }
        Rectangle {
            visible: bar.demoMode
            radius: 4; height: 24; width: demoT.implicitWidth + 16
            anchors.verticalCenter: parent.verticalCenter
            color: "transparent"; border.color: bar._red; border.width: 1
            Text {
                id: demoT
                anchors.centerIn: parent
                text: qsTr("DEMO")
                color: bar._red; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
        height: 1; color: bar._line
    }
}
