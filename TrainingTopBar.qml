import QtQuick 2.15

// Training Lab — the SHARED training top bar. UI-WIND-001, UI-TRAIN-001/002/003.
//
// ONE bar for all four Training Lab programmes: Technical Blocks, Call &
// Diagnose, Position Transition and Wind Map. It occupies the same 42 px band
// as the competition `statusStrip`, which is suppressed whenever any Training
// programme is active, so every anchor in ShootingPage still chains from
// statusStrip.bottom and nothing below moves.
//
// WHY ONE BAR. The competition strip's rows each grew their own gate — one
// more `!isXMatch` term as each programme landed — and the terms fell out of
// step. Technical Blocks gated three rows but not the identity row; Call &
// Diagnose and Position Transition gated none of the four. The identity row is
// the one carrying currentGameDisplay/currentmatchDisplay, i.e. "FINAL 35"
// left behind by the last selected event card. Four near-identical bars would
// have re-created the same drift, so the boundary is a single question —
// is any Training Lab programme active — asked once.
//
// THIS BAR READS NO COMPETITION STATE. No Finals controller, phase, stage,
// timer, ceremony, command, score banner or classification; no
// currentGameDisplay, currentmatchDisplay, matchShootCount or
// globalMatchModel. Everything it shows is passed in by the programme that
// owns it.
Item {
    id: bar

    // ── what the owning programme supplies ──────────────────────────────
    property string programmeName: ""     // "Wind Map", "Call & Diagnose", ...
    property string athlete: ""
    property string discipline: ""
    property string positionName: ""      // "" hides the chip (non-3P)
    property string phaseLabel: ""        // the programme's OWN phase
    property string progressValue: ""     // e.g. "12 / 40", "Block 2 of 4"
    property string progressLabel: ""     // e.g. "COUNTED", "SHOTS"
    property bool   phaseActive: false    // true = the shooting phase (accent)
    property bool   demoMode: false

    // Width reserved at the RIGHT edge for the target-connection panel, which
    // floats above this bar at a higher z.
    //
    // Before this existed the panel simply drew over the strip: at 1536 px it
    // spanned x 875..1336 while the centred honesty line spanned roughly
    // 618..918, so "NOT AN OFFICIAL COMPETITION RESULT" was partly covered by
    // the connection status, and the right-hand progress row collided with it
    // outright. Two independently-correct layouts overlapping is still a
    // defect - the fix is to give the panel a reserved area and lay the bar
    // out in what remains, rather than to nudge either one.
    property real rightReserve: 0

    readonly property color _bg:     "#15161a"
    readonly property color _line:   "#3a3b40"
    readonly property color _red:    theme.tokens.accentHover
    readonly property color _green:  "#20C997"
    readonly property color _amber:  "#E0A800"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#9a9ba0"

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
            text: bar.programmeName
            color: bar._txt; font.family: theme.fontFamily
            font.pixelSize: 14; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle { width: 1; height: 18; color: bar._line
                    anchors.verticalCenter: parent.verticalCenter }
        Text {
            text: bar.athlete
            color: bar._txtSec; font.family: theme.fontFamily; font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle { visible: bar.discipline !== ""
                    width: 1; height: 18; color: bar._line
                    anchors.verticalCenter: parent.verticalCenter }
        Text {
            visible: bar.discipline !== ""
            text: bar.discipline
            color: bar._txtSec; font.family: theme.fontFamily; font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        // The TRAINING position. Not a Final stage chip — it is the owning
        // controller's projection, and changing it is that controller's action.
        Rectangle {
            visible: bar.positionName !== ""
            radius: 10; height: 20; width: posText.implicitWidth + 18
            color: "#0d2018"; border.color: bar._green; border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: posText
                anchors.centerIn: parent
                text: bar.positionName.toUpperCase()
                color: bar._green; font.family: theme.fontFamily
                font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
            }
        }
    }

    // ── the honesty line ────────────────────────────────────────────────
    // Present on every Training capture screen at all times. Centred in the
    // area NOT reserved for the connection panel, so it can never be covered.
    Text {
        id: honestyLine
        anchors.verticalCenter: parent.verticalCenter
        x: (parent.width - bar.rightReserve - width) / 2
        text: qsTr("NOT AN OFFICIAL COMPETITION RESULT")
        color: bar._amber; font.family: theme.fontFamily
        font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.5
    }

    // ── the programme's own progress + mode ─────────────────────────────
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 16 + bar.rightReserve
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Text {
            visible: bar.progressValue !== ""
            text: bar.progressValue
            color: bar._txt; font.family: theme.fontFamily
            font.pixelSize: 14; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: bar.progressLabel !== ""
            text: bar.progressLabel
            color: bar._txtSec; font.family: theme.fontFamily
            font.pixelSize: 9; font.letterSpacing: 1.5
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            visible: bar.phaseLabel !== ""
            radius: 4; height: 24; width: phaseT.implicitWidth + 20
            anchors.verticalCenter: parent.verticalCenter
            color: bar.phaseActive ? bar._red : "#8a6d00"
            Text {
                id: phaseT
                anchors.centerIn: parent
                text: bar.phaseLabel
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
        anchors.bottom: parent.bottom; anchors.left: parent.left
        anchors.right: parent.right; height: 1; color: bar._line
    }
}
