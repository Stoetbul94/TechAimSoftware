import QtQuick 2.15

// 3P FINAL HUD — athlete ADVANCE control. Contextual pill shown ONLY while the
// controller offers a legal Stage-1 transition (advanceLabel non-empty); QML
// never infers legality. Disappears after a successful transition (label
// empties / phase changes). The under-limit confirmation is a compact inline
// row, never a modal.
Item {
    id: adv
    property var ctl

    width: childrenRect.width
    height: 30

    property bool confirming: false
    property string prompt: ""

    Connections {
        target: adv.ctl
        function onAdvanceConfirmationRequired(fired, limit) {
            adv.prompt = (limit - fired) + " SHOTS UNFIRED — CHANGE ANYWAY?"
            adv.confirming = true
        }
        function onPhaseChanged() { adv.confirming = false }
    }

    // The single stage-aware action (Decision 1).
    Rectangle {
        visible: adv.ctl && adv.ctl.advanceLabel !== "" && !adv.confirming
        width: visible ? pillTxt.implicitWidth + 30 : 0
        height: 30; radius: 15
        color: pillMA.pressed ? "#8a002f" : "#cca80038"
        border.color: "#d44a6a"
        border.width: 1
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        Text {
            id: pillTxt; anchors.centerIn: parent
            text: adv.ctl ? adv.ctl.advanceLabel : ""
            color: "white"; font.family: "Segoe UI"; font.pixelSize: 12; font.bold: true
            font.letterSpacing: 1
        }
        MouseArea { id: pillMA; anchors.fill: parent; onClicked: adv.ctl.advanceStage1() }
    }

    // Compact inline confirmation (no modal, no full-screen block).
    Row {
        visible: adv.confirming
        spacing: 8
        Rectangle {
            width: promptTxt.implicitWidth + 20; height: 30; radius: 15
            color: "#c614151a"; border.color: "#e8b93d"; border.width: 1
            Text { id: promptTxt; anchors.centerIn: parent; text: adv.prompt
                   color: "#e8b93d"; font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true }
        }
        Rectangle {
            width: okTxt.implicitWidth + 24; height: 30; radius: 15
            color: okMA.pressed ? "#8a002f" : "#a80038"
            Text { id: okTxt; anchors.centerIn: parent; text: "CONFIRM"
                   color: "white"; font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true }
            MouseArea { id: okMA; anchors.fill: parent
                        onClicked: { adv.confirming = false; adv.ctl.confirmStage1Advance() } }
        }
        Rectangle {
            width: noTxt.implicitWidth + 24; height: 30; radius: 15
            color: noMA.pressed ? "#2a2b30" : "#26272c"; border.color: "#3a3b42"; border.width: 1
            Text { id: noTxt; anchors.centerIn: parent; text: "CANCEL"
                   color: "#d7d8dd"; font.family: "Segoe UI"; font.pixelSize: 11 }
            MouseArea { id: noMA; anchors.fill: parent
                        onClicked: { adv.confirming = false; adv.ctl.cancelStage1Advance() } }
        }
    }
}
