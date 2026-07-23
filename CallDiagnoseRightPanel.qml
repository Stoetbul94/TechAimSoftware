import QtQuick 2.15

// Call & Diagnose (T2) — persistent right-hand control + information panel.
// Occupies the SAME slot as the competition RightPanel (hidden while a C&D
// session is active). Shows the phase-appropriate status and the primary
// action (Start in Sighters). The actual impact is NEVER shown here — the
// panel only reports "shot received / awaiting call / call difference".
Item {
    id: panel
    property var ctl: null                 // CALLDIAG
    property bool connected: false

    readonly property bool sighters:     ctl && ctl.phase === 1
    readonly property bool awaitingShot:  ctl && ctl.phase === 2
    readonly property bool awaitingCall:  ctl && ctl.phase === 3
    readonly property bool revealOpen:    ctl && ctl.phase === 4
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
        Text { text: "Call & Diagnose"; color: _txt; font.pixelSize: 17; font.bold: true }
        Text { visible: ctl && ctl.positionName !== ""; text: ctl ? ctl.positionName : ""
               color: _green; font.pixelSize: 12; font.bold: true }
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

            // SIGHTERS
            Column {
                width: parent.width; spacing: 8; visible: panel.sighters
                Text { text: "SIGHTERS"; color: _txt; font.pixelSize: 20; font.bold: true }
                Text { width: parent.width; wrapMode: Text.WordWrap; color: _txtSec; font.pixelSize: 12
                       text: "Optional shots to confirm your position and zero. Sighters are excluded from Call & Diagnose results." }
                Rectangle { width: parent.width; height: 40; radius: 8; color: _card; border.color: _line; border.width: 1
                    Row { anchors.fill: parent; anchors.margins: 10; spacing: 8
                        Text { text: "Sighters fired"; color: _txtMut; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: ctl ? ("" + ctl.sighterCount) : "0"; color: _green
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true; anchors.verticalCenter: parent.verticalCenter } } }
            }

            // CALLING STATUS (AwaitingShot / AwaitingCall / Reveal)
            Column {
                width: parent.width; spacing: 8
                visible: panel.awaitingShot || panel.awaitingCall || panel.revealOpen
                Text {
                    color: _txt; font.family: "Consolas"; font.pixelSize: 22; font.bold: true
                    text: ctl ? ("Shot " + ctl.pendingShotNumber + " of " + ctl.shotCount) : ""
                }
                // progress bar (completed calls)
                Rectangle {
                    width: parent.width; height: 12; radius: 6; color: "#0E1014"; border.color: _line; border.width: 1
                    Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        anchors.margins: 2; radius: 5; color: _green
                        width: (ctl && ctl.shotCount > 0) ? (parent.width - 4) * Math.min(1, ctl.shotsCompleted / ctl.shotCount) : 0 } }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap; font.pixelSize: 13
                    color: panel.awaitingShot ? _txtSec : (panel.awaitingCall ? _redHi : _green)
                    text: panel.awaitingShot ? "Fire your next shot on the target."
                          : panel.awaitingCall ? "SHOT RECEIVED — mark where you believe it landed, then confirm."
                          : "Call revealed. Review the difference and continue."
                }
                // reveal readout (call difference) — only in Reveal
                Rectangle {
                    visible: panel.revealOpen; width: parent.width; radius: 8
                    color: _card; border.color: _line; border.width: 1
                    height: revealCol.implicitHeight + 20
                    Column {
                        id: revealCol
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10
                        spacing: 3
                        property var rv: panel.revealOpen && panel.ctl ? panel.ctl.revealCurrent() : ({})
                        Text { text: "CALL DIFFERENCE"; color: _txtMut; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                        Text { text: (revealCol.rv.errorMm !== undefined ? Number(revealCol.rv.errorMm).toFixed(1) : "—") + " mm"
                               color: _txt; font.family: "Consolas"; font.pixelSize: 22; font.bold: true }
                        Text { text: (revealCol.rv.xPhrase || "") + " · " + (revealCol.rv.yPhrase || "")
                               color: _txtSec; font.pixelSize: 12 }
                        Text { text: "Actual score: " + (revealCol.rv.actualScore !== undefined ? Number(revealCol.rv.actualScore).toFixed(1) : "—")
                               color: _txtMut; font.pixelSize: 11 }
                    }
                }
            }

            // shared info
            Column {
                width: parent.width; spacing: 6
                visible: panel.sighters || panel.awaitingShot || panel.awaitingCall || panel.revealOpen
                Rectangle { width: parent.width; height: 1; color: _line }
                Repeater {
                    model: {
                        if (!ctl) return []
                        return [ { k: "Technical focus", v: ctl.technicalFocus },
                                 { k: "Input source",    v: panel.demoMode ? "Demo" : "Live" },
                                 { k: "Target",          v: panel.demoMode ? "Demo · not needed"
                                                            : (panel.connected ? "Connected" : "Not connected") },
                                 { k: "Programme",        v: ctl.shotCount + " called shots" } ]
                    }
                    delegate: Row { width: parent.width; spacing: 8
                        Text { text: modelData.k; color: _txtMut; font.pixelSize: 12; width: parent.width * 0.42 }
                        Text { text: modelData.v; color: _txt; font.pixelSize: 12; font.bold: true
                               width: parent.width * 0.5; elide: Text.ElideRight } }
                }
            }

            Column {
                width: parent.width; spacing: 6
                visible: ctl && ctl.phase === 5
                Text { text: "Session complete"; color: _txt; font.pixelSize: 16; font.bold: true }
                Text { width: parent.width; wrapMode: Text.WordWrap; color: _txtMut; font.pixelSize: 11
                       text: "Results are shown in the main view." }
            }
        }
    }

    // pinned Start (Sighters)
    Item {
        id: actionZone
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 16
        height: panel.sighters ? (startBtn.height + hint.implicitHeight + 8) : 0
        visible: panel.sighters
        Rectangle {
            id: startBtn
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: 60; radius: 10; color: startMouse.pressed ? panel._red : panel._redHi
            Text { anchors.centerIn: parent; text: panel.ctl ? panel.ctl.startLabel.toUpperCase() : "START"
                   color: "white"; font.pixelSize: 15; font.bold: true; font.letterSpacing: 1 }
            MouseArea { id: startMouse; anchors.fill: parent; onClicked: if (panel.ctl) panel.ctl.startProgramme() }
        }
        Text { id: hint
            anchors.top: startBtn.bottom; anchors.topMargin: 8; anchors.left: parent.left; anchors.right: parent.right
            wrapMode: Text.WordWrap; color: panel._txtMut; font.pixelSize: 10
            text: "The target clears and your next shot is the first called shot." }
    }
}
