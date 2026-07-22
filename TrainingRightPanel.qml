import QtQuick 2.15

// Training Lab (T1.4) — persistent right-hand Training control + information
// panel. Occupies the SAME slot the competition RightPanel uses (which is
// hidden while a Technical Blocks session is active), so Training controls
// never sit over the target and the aiming area stays clear.
//
// It renders two shooting-phase states — SIGHTERS (phase 1) and ACTIVE BLOCK
// (phase 2). Block Review (3) and the final Summary (4) are full-workspace
// states owned by TrainingHud; here we only show a compact status while those
// are open. Every value binds to TRAINING controller projections — no score
// or geometry is computed here. The counted-shot count binds the authoritative
// `shotsCompleted` (0 before the first counted shot); never a +1 derivation.
Item {
    id: panel
    property var ctl: null            // TRAINING
    property bool connected: false    // hardware master-connection (Live only)

    readonly property bool sighters: ctl && ctl.phase === 1
    readonly property bool active:   ctl && ctl.phase === 2
    readonly property bool demoMode: ctl && ctl.sessionOperatingMode === "Demo"

    // palette (matches TrainingHud)
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

    // ── header identity (never "MATCH") ──────────────────────────────────
    Column {
        id: header
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.margins: 16; spacing: 2
        Text { text: "TRAINING LAB"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
        Text { text: "Technical Blocks"; color: _txt; font.pixelSize: 17; font.bold: true }
        Text {
            color: _green; font.pixelSize: 12; font.bold: true
            visible: ctl && ctl.positionName !== ""
            text: ctl ? ctl.positionName : ""
        }
    }

    // scrollable body so nothing is unreachable at small heights; the primary
    // action is pinned to the bottom (always visible without scrolling).
    Flickable {
        id: body
        anchors.top: header.bottom; anchors.topMargin: 10
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: actionZone.top
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 8
        clip: true
        contentHeight: bodyCol.implicitHeight
        Column {
            id: bodyCol
            width: parent.width; spacing: 12

            // ── SIGHTERS phase ────────────────────────────────────────────
            Column {
                width: parent.width; spacing: 8
                visible: panel.sighters
                Text { text: "SIGHTERS"; color: _txt; font.pixelSize: 20; font.bold: true; font.letterSpacing: 1 }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap
                    color: _txtSec; font.pixelSize: 12
                    text: "Optional shots to confirm your position and zero. "
                        + "Sighters are excluded from Training results."
                }
                Rectangle { width: parent.width; height: 40; radius: 8; color: _card; border.color: _line; border.width: 1
                    Row { anchors.fill: parent; anchors.margins: 10; spacing: 8
                        Text { text: "Sighters fired"; color: _txtMut; font.pixelSize: 12
                               anchors.verticalCenter: parent.verticalCenter }
                        Item { width: 4; height: 1 }
                        Text { text: ctl ? ("" + ctl.sighterCount) : "0"; color: _green
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }

            // ── ACTIVE BLOCK phase ────────────────────────────────────────
            Column {
                width: parent.width; spacing: 8
                visible: panel.active
                Text {
                    color: _txt; font.pixelSize: 20; font.bold: true; font.letterSpacing: 1
                    text: ctl ? ("BLOCK " + ctl.currentBlock + " OF " + ctl.blockCount) : ""
                }
                // Shot {shotsCompleted} of {shotsRequired} — 0 of N before any shot.
                Text {
                    color: _txt; font.family: "Consolas"; font.pixelSize: 26; font.bold: true
                    text: ctl ? ("Shot " + ctl.shotsCompleted + " of " + ctl.shotsPerBlock) : ""
                }
                // progress bar (counted shots only)
                Rectangle {
                    width: parent.width; height: 14; radius: 7; color: "#0E1014"
                    border.color: _line; border.width: 1
                    Rectangle {
                        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        anchors.margins: 2; radius: 6; color: _redHi
                        width: (ctl && ctl.shotsPerBlock > 0)
                               ? (parent.width - 4) * Math.min(1, ctl.shotsCompleted / ctl.shotsPerBlock)
                               : 0
                        Behavior on width { NumberAnimation { duration: 180 } }
                    }
                }
                // elapsed block time (polled; the projection has no NOTIFY)
                Row { spacing: 8
                    Text { text: "Elapsed"; color: _txtMut; font.pixelSize: 12
                           anchors.verticalCenter: parent.verticalCenter }
                    Text { id: elapsedText; color: _txtSec; font.family: "Consolas"; font.pixelSize: 13
                           anchors.verticalCenter: parent.verticalCenter
                        Timer {
                            interval: 1000; running: panel.active; repeat: true
                            onTriggered: {
                                var s = panel.ctl ? panel.ctl.blockElapsedSec() : 0
                                elapsedText.text = Math.floor(s / 60) + ":" + ("0" + (s % 60)).slice(-2)
                            }
                        }
                    }
                }
            }

            // ── shared info block (both shooting states) ──────────────────
            Column {
                width: parent.width; spacing: 6
                visible: panel.sighters || panel.active
                Rectangle { width: parent.width; height: 1; color: _line }
                Repeater {
                    model: {
                        if (!ctl) return []
                        var vis = ["Full hidden", "Group only", "Impact, no score"][ctl.visibilityMode]
                        var rows = [ { k: "Technical focus", v: ctl.technicalFocus },
                                     { k: "Visibility",      v: vis },
                                     { k: "Input source",    v: panel.demoMode ? "Demo" : "Live" },
                                     { k: "Target",          v: panel.demoMode ? "Demo · not needed"
                                                                : (panel.connected ? "Connected" : "Not connected") },
                                     { k: "Programme",       v: ctl.blockCount + " blocks × " + ctl.shotsPerBlock + " shots" } ]
                        return rows
                    }
                    delegate: Row {
                        width: parent.width; spacing: 8
                        Text { text: modelData.k; color: _txtMut; font.pixelSize: 12; width: parent.width * 0.42 }
                        Text { text: modelData.v; color: _txt; font.pixelSize: 12; font.bold: true
                               width: parent.width * 0.5; elide: Text.ElideRight; wrapMode: Text.WordWrap }
                    }
                }
            }

            // ── review / summary compact status (full view is in TrainingHud) ─
            Column {
                width: parent.width; spacing: 6
                visible: ctl && (ctl.phase === 3 || ctl.phase === 4)
                Text {
                    color: _txt; font.pixelSize: 16; font.bold: true
                    text: ctl && ctl.phase === 3 ? "Block review open" : "Session complete"
                }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap; color: _txtMut; font.pixelSize: 11
                    text: "Results are shown in the main view."
                }
            }
        }
    }

    // ── pinned primary action (Sighters → Start Block) ───────────────────
    Item {
        id: actionZone
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 16
        height: panel.sighters ? (startBtn.height + hint.implicitHeight + 8) : 0
        visible: panel.sighters

        Rectangle {
            id: startBtn
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: 60; radius: 10
            color: startMouse.pressed ? panel._red : panel._redHi
            Text {
                anchors.centerIn: parent
                text: panel.ctl ? panel.ctl.startBlockLabel.toUpperCase() : "START BLOCK"
                color: "white"; font.pixelSize: 15; font.bold: true; font.letterSpacing: 1
            }
            MouseArea {
                id: startMouse; anchors.fill: parent
                onClicked: if (panel.ctl) panel.ctl.startBlock()   // idempotent; double-tap safe
            }
        }
        Text {
            id: hint
            anchors.top: startBtn.bottom; anchors.topMargin: 8
            anchors.left: parent.left; anchors.right: parent.right
            wrapMode: Text.WordWrap; color: panel._txtMut; font.pixelSize: 10
            text: "The target will clear and your next shot will be counted as Shot 1."
        }
    }
}
