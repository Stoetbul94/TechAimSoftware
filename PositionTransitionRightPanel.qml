import QtQuick 2.15

// Position Transition (T4) — persistent right-hand control + information panel.
// Occupies the competition RightPanel slot while a Position Transition session
// is active. Phase-appropriate status + primary actions. Target stays dominant.
Item {
    id: panel
    property var ctl: null                 // POSTRANS
    property bool connected: false

    readonly property bool inSetup:    ctl && ctl.phase === 1
    readonly property bool inSighters: ctl && ctl.phase === 2
    readonly property bool verifying:  ctl && ctl.phase === 3
    readonly property bool reviewOpen: ctl && ctl.phase === 4
    readonly property bool demoMode: ctl && ctl.sessionOperatingMode === "Demo"

    readonly property color _bg:     "#15171C"
    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    Rectangle { anchors.fill: parent; color: _bg
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: _line } }

    Column {
        id: header
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.margins: 16; spacing: 2
        Text { text: "TRAINING LAB"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
        Text { text: "Position Transition"; color: _txt; font.pixelSize: 17; font.bold: true }
        Text { text: ctl ? ctl.positionName : ""; color: _green; font.pixelSize: 13; font.bold: true }
        Text { text: ctl ? ctl.sequenceArrow : ""; color: _txtMut; font.pixelSize: 10 }
    }

    Flickable {
        id: body
        anchors.top: header.bottom; anchors.topMargin: 10
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: actionZone.top
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 8
        clip: true; contentHeight: bodyCol.implicitHeight
        Column {
            id: bodyCol
            width: parent.width; spacing: 12

            // SETUP
            Column {
                width: parent.width; spacing: 8; visible: panel.inSetup
                Text { text: "POSITION SETUP"; color: _txt; font.pixelSize: 18; font.bold: true }
                Text { width: parent.width; wrapMode: Text.WordWrap; color: _txtSec; font.pixelSize: 12
                       text: "Build your " + (ctl ? ctl.positionName : "") + " position. Confirm the checklist, then press Position Ready." }
                Row { spacing: 8
                    Text { text: "Setup time"; color: _txtMut; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: setupTimer; color: _txt; font.family: "Consolas"; font.pixelSize: 16; font.bold: true
                           anchors.verticalCenter: parent.verticalCenter
                        Timer { interval: 1000; running: panel.inSetup; repeat: true
                            onTriggered: { var s = panel.ctl ? panel.ctl.setupElapsedSec() : 0
                                setupTimer.text = Math.floor(s/60) + ":" + ("0"+(s%60)).slice(-2) } } } }
                // checklist (touch-friendly)
                Column { width: parent.width; spacing: 4
                    visible: ctl && ctl.checklistMode !== 2
                    Text { text: "SETUP CHECKLIST"; color: _txtMut; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1; topPadding: 4 }
                    Repeater {
                        model: panel.ctl ? panel.ctl.checklistItems() : []
                        delegate: Rectangle {
                            width: parent.width; height: 40; radius: 6
                            color: modelData.state === 1 ? "#0d2018" : (modelData.state === 2 ? "#241014" : _card)
                            border.color: _line; border.width: 1
                            Row { anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 8
                                Text { width: parent.width - 70; anchors.verticalCenter: parent.verticalCenter
                                       text: modelData.label; color: _txt; font.pixelSize: 11; wrapMode: Text.WordWrap
                                       elide: Text.ElideRight }
                                Text { anchors.verticalCenter: parent.verticalCenter; width: 20
                                       text: modelData.state === 1 ? "✓" : (modelData.state === 2 ? "–" : "○")
                                       color: modelData.state === 1 ? _green : _txtMut; font.pixelSize: 15; font.bold: true } }
                            MouseArea { anchors.fill: parent
                                onClicked: if (panel.ctl) panel.ctl.setChecklistItem(modelData.index, modelData.state === 1 ? 2 : 1) }
                        }
                    }
                }
            }

            // VERIFICATION (and sighters share the shot-count line)
            Column {
                width: parent.width; spacing: 8
                visible: panel.inSighters || panel.verifying
                Text { visible: panel.inSighters; text: "SIGHTERS — " + (ctl ? ctl.positionName.toUpperCase() : "")
                       color: _txt; font.pixelSize: 18; font.bold: true }
                Text { visible: panel.verifying
                       color: _txt; font.family: "Consolas"; font.pixelSize: 22; font.bold: true
                       text: ctl ? (ctl.positionName.toUpperCase() + " VERIFICATION") : "" }
                Text { visible: panel.verifying
                       color: _txt; font.family: "Consolas"; font.pixelSize: 20; font.bold: true
                       text: ctl ? ("Shot " + ctl.shotsCompleted + " of " + ctl.verificationShots) : "" }
                Rectangle { visible: panel.verifying
                    width: parent.width; height: 12; radius: 6; color: "#0E1014"; border.color: _line; border.width: 1
                    Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        anchors.margins: 2; radius: 5; color: _redHi
                        width: (ctl && ctl.verificationShots > 0) ? (parent.width-4) * Math.min(1, ctl.shotsCompleted/ctl.verificationShots) : 0 } }
                Row { spacing: 8; visible: panel.inSighters
                    Text { text: "Sighters fired"; color: _txtMut; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: ctl ? ("" + ctl.sighterCount) : "0"; color: _green; font.family: "Consolas"; font.pixelSize: 16; font.bold: true
                           anchors.verticalCenter: parent.verticalCenter } }
                Row { spacing: 8
                    Text { text: "Since Ready"; color: _txtMut; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: readyTimer; color: _txtSec; font.family: "Consolas"; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter
                        Timer { interval: 1000; running: panel.inSighters || panel.verifying; repeat: true
                            onTriggered: { var s = panel.ctl ? panel.ctl.readyElapsedSec() : 0
                                readyTimer.text = Math.floor(s/60) + ":" + ("0"+(s%60)).slice(-2) } } } }
            }

            // shared info
            Column {
                width: parent.width; spacing: 6
                visible: panel.inSetup || panel.inSighters || panel.verifying
                Rectangle { width: parent.width; height: 1; color: _line }
                Repeater {
                    model: {
                        if (!ctl) return []
                        return [ { k: "Focus", v: ctl.technicalFocus },
                                 { k: "Repeat", v: ctl.currentRepeat + " of " + ctl.totalRepeats },
                                 { k: "Input source", v: panel.demoMode ? "Demo" : "Live" },
                                 { k: "Target", v: panel.demoMode ? "Demo · not needed"
                                                   : (panel.connected ? "Connected" : "Not connected") } ]
                    }
                    delegate: Row { width: parent.width; spacing: 8
                        Text { text: modelData.k; color: _txtMut; font.pixelSize: 12; width: parent.width * 0.42 }
                        Text { text: modelData.v; color: _txt; font.pixelSize: 12; font.bold: true
                               width: parent.width * 0.5; elide: Text.ElideRight } }
                }
            }

            Column { width: parent.width; spacing: 6
                visible: ctl && (ctl.phase === 4 || ctl.phase === 5)
                Text { text: ctl && ctl.phase === 4 ? "Position review open" : "Session complete"
                       color: _txt; font.pixelSize: 16; font.bold: true }
                Text { width: parent.width; wrapMode: Text.WordWrap; color: _txtMut; font.pixelSize: 11
                       text: "Results are shown in the main view." } }
        }
    }

    // pinned primary action
    Item {
        id: actionZone
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 16
        height: (panel.inSetup || panel.inSighters) ? 68 : 0
        visible: panel.inSetup || panel.inSighters
        Rectangle {
            anchors.fill: parent; radius: 10
            color: primaryMouse.pressed ? panel._red : panel._redHi
            Text { anchors.centerIn: parent
                   text: panel.inSetup ? "POSITION READY"
                         : (panel.ctl ? panel.ctl.startVerifyLabel.toUpperCase() : "START VERIFICATION")
                   color: "white"; font.pixelSize: 15; font.bold: true; font.letterSpacing: 1 }
            MouseArea { id: primaryMouse; anchors.fill: parent
                onClicked: {
                    if (!panel.ctl) return
                    if (panel.inSetup) panel.ctl.positionReady()
                    else panel.ctl.startVerification()
                } }
        }
    }
}
