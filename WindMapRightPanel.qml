import QtQuick 2.15

// Wind Map (Release 2) — persistent right-hand capture panel.
//
// Occupies the competition RightPanel slot while a Wind Map session is active.
// It is where the athlete RECORDS what they observe. It never interprets it:
// no correction, no advice, no comparison, no sight clicks.
//
// The controller owns every rule. This file constructs NO domain events, does
// NO unit conversion, and never sees hundredths of a metre per second — it
// hands WINDMAP a direction in degrees and a speed in m/s and reports whatever
// it refuses.
Item {
    id: panel
    property var ctl: null                  // WINDMAP
    property bool connected: false

    readonly property bool inSetup:    ctl && ctl.phase === 1
    readonly property bool inSighters: ctl && ctl.phase === 2
    readonly property bool counting:   ctl && ctl.phase === 3
    readonly property bool posReview:  ctl && ctl.phase === 4
    readonly property bool reviewOpen: ctl && ctl.phase === 5
    readonly property bool demoMode:   ctl && ctl.sessionOperatingMode === "Demo"

    readonly property color _bg:     "#15171C"
    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    theme.tokens.accentHover
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _amber:  "#E0A800"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    // Pending entry, applied only when the athlete records it.
    property int  pendingDeg: 0
    property string pendingSpeed: ""
    property string pendingNote: ""

    Rectangle { anchors.fill: parent; color: _bg
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: _line } }

    Column {
        id: header
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.margins: 16; spacing: 2
        Text { text: "TRAINING LAB"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
        Text { text: "Wind Map"; color: _txt; font.pixelSize: 17; font.bold: true }
        Text { visible: ctl !== null && ctl.threePositions
               text: ctl ? ctl.positionName : ""; color: _green; font.pixelSize: 13; font.bold: true }
        Text { text: ctl ? ctl.phaseName : ""; color: _txtMut; font.pixelSize: 10 }
    }

    Flickable {
        id: body
        anchors.top: header.bottom; anchors.topMargin: 10
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: actionZone.top
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 8
        clip: true; contentWidth: width; contentHeight: bodyCol.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: bodyCol
            width: body.width; spacing: 12

            // ── THE STANDING CONDITION ───────────────────────────────────
            // What the NEXT accepted shot will record. Absence is shown as
            // absence — never as calm and never as 0 degrees North.
            Rectangle {
                width: parent.width; height: standingCol.implicitHeight + 20; radius: 8
                color: _card
                border.width: 1
                border.color: (ctl && ctl.hasWindReading) ? _line : _amber
                Column {
                    id: standingCol
                    anchors.left: parent.left; anchors.leftMargin: 12
                    anchors.right: parent.right; anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter; spacing: 4
                    Text { text: "STANDING CONDITION"; color: _txtMut
                           font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                    Text { width: parent.width; wrapMode: Text.WordWrap
                           text: ctl ? ctl.conditionSummary : ""
                           color: (ctl && ctl.hasWindReading) ? _txt : _amber
                           font.pixelSize: 15; font.bold: true }
                    Text { visible: text !== ""; width: parent.width; wrapMode: Text.WordWrap
                           text: ctl ? ctl.conditionNote : ""
                           color: _txtSec; font.pixelSize: 10; font.italic: true }
                    Text { text: ctl ? ("Recorded changes: " + ctl.conditionChanges) : ""
                           color: _txtMut; font.pixelSize: 10 }
                }
            }

            // ── COMPASS RING ─────────────────────────────────────────────
            // Eight 45° sectors centred on the compass points, matching the
            // domain's sector rule. Touch-sized targets; the exact degree
            // stays editable below so the ring is a shortcut, not a limit.
            Column {
                width: parent.width; spacing: 6
                visible: !panel.reviewOpen && ctl && ctl.active && !ctl.completed
                Text { text: "DIRECTION"; color: _txtMut
                       font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                Item {
                    width: parent.width; height: 172
                    Rectangle {
                        id: ring
                        width: 168; height: 168; radius: 84
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#0E1014"; border.color: _line; border.width: 1
                        Repeater {
                            model: [ { l: "N", d: 0 },  { l: "NE", d: 45 },  { l: "E", d: 90 },  { l: "SE", d: 135 },
                                     { l: "S", d: 180 },{ l: "SW", d: 225 }, { l: "W", d: 270 }, { l: "NW", d: 315 } ]
                            delegate: Rectangle {
                                width: 44; height: 44; radius: 22
                                // 0 deg = North at the top, increasing CLOCKWISE.
                                x: ring.width / 2  - width / 2  + 62 * Math.sin(modelData.d * Math.PI / 180)
                                y: ring.height / 2 - height / 2 - 62 * Math.cos(modelData.d * Math.PI / 180)
                                color: panel.pendingDeg === modelData.d ? panel._red : "#1B1E24"
                                border.color: panel.pendingDeg === modelData.d ? panel._red : panel._line
                                border.width: 1
                                Text { anchors.centerIn: parent; text: modelData.l
                                       color: panel.pendingDeg === modelData.d ? "white" : panel._txtSec
                                       font.pixelSize: 12; font.bold: true }
                                MouseArea { anchors.fill: parent
                                    onClicked: { panel.pendingDeg = modelData.d; degField.text = "" + modelData.d } }
                            }
                        }
                        Column {
                            anchors.centerIn: parent; spacing: 0
                            Text { anchors.horizontalCenter: parent.horizontalCenter
                                   text: panel.pendingDeg + "°"; color: panel._txt
                                   font.family: "Consolas"; font.pixelSize: 20; font.bold: true }
                            Text { anchors.horizontalCenter: parent.horizontalCenter
                                   text: "pending"; color: panel._txtMut; font.pixelSize: 9 }
                        }
                    }
                }

                // Exact degrees + speed. Both are plain operator units.
                Row { spacing: 8
                    Rectangle { width: 78; height: 44; radius: 8; color: "#0E1014"
                        border.color: _line; border.width: 1
                        TextInput { id: degField
                            anchors.fill: parent; anchors.margins: 8
                            verticalAlignment: TextInput.AlignVCenter
                            color: panel._txt; font.family: "Consolas"; font.pixelSize: 15
                            text: "0"; inputMethodHints: Qt.ImhDigitsOnly
                            onTextChanged: { var v = parseInt(text); if (!isNaN(v)) panel.pendingDeg = ((v % 360) + 360) % 360 }
                        }
                        Text { visible: degField.text === ""; anchors.centerIn: parent
                               text: "deg"; color: panel._txtMut; font.pixelSize: 11 } }
                    Rectangle { width: 96; height: 44; radius: 8; color: "#0E1014"
                        border.color: _line; border.width: 1
                        TextInput { id: speedField
                            anchors.fill: parent; anchors.margins: 8
                            verticalAlignment: TextInput.AlignVCenter
                            color: panel._txt; font.family: "Consolas"; font.pixelSize: 15
                            text: ""; inputMethodHints: Qt.ImhFormattedNumbersOnly
                            onTextChanged: panel.pendingSpeed = text }
                        Text { visible: speedField.text === ""; anchors.left: parent.left
                               anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
                               text: "m/s"; color: panel._txtMut; font.pixelSize: 11 } }
                }
                Rectangle { width: parent.width; height: 40; radius: 8; color: "#0E1014"
                    border.color: _line; border.width: 1
                    TextInput { id: noteField
                        anchors.fill: parent; anchors.margins: 8
                        verticalAlignment: TextInput.AlignVCenter
                        color: panel._txtSec; font.pixelSize: 11
                        onTextChanged: panel.pendingNote = text }
                    Text { visible: noteField.text === ""; anchors.left: parent.left
                           anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
                           text: "Note (optional)"; color: panel._txtMut; font.pixelSize: 11 } }

                Text { id: condError; visible: text !== ""; width: parent.width
                       wrapMode: Text.WordWrap; text: ""
                       color: panel._redHi; font.pixelSize: 10 }

                // The three DISTINCT recorded states.
                Row { spacing: 6
                    Rectangle {
                        width: (bodyCol.width - 12) / 3; height: 46; radius: 8
                        color: recordMouse.pressed ? panel._red : panel._redHi
                        Text { anchors.centerIn: parent; text: "RECORD"; color: "white"
                               font.pixelSize: 12; font.bold: true }
                        MouseArea { id: recordMouse; anchors.fill: parent
                            onClicked: {
                                if (!panel.ctl) return
                                var sp = parseFloat(panel.pendingSpeed)
                                if (panel.pendingSpeed === "" || isNaN(sp)) {
                                    condError.text = "Enter a wind speed in metres per second, or press Calm."
                                    return
                                }
                                if (!panel.ctl.setMeasuredCondition(panel.pendingDeg, sp, panel.pendingNote)) {
                                    condError.text = panel.ctl.lastError
                                    return
                                }
                                condError.text = ""
                            } }
                    }
                    Rectangle {
                        width: (bodyCol.width - 12) / 3; height: 46; radius: 8
                        color: "transparent"; border.color: panel._line; border.width: 1
                        Text { anchors.centerIn: parent; text: "CALM"; color: panel._txt
                               font.pixelSize: 12; font.bold: true }
                        MouseArea { anchors.fill: parent
                            onClicked: {
                                if (!panel.ctl) return
                                if (!panel.ctl.setCalmCondition(panel.pendingNote)) condError.text = panel.ctl.lastError
                                else condError.text = ""
                            } }
                    }
                    Rectangle {
                        width: (bodyCol.width - 12) / 3; height: 46; radius: 8
                        color: "transparent"; border.color: panel._line; border.width: 1
                        Text { anchors.centerIn: parent; text: "NO READING"; color: panel._txtSec
                               font.pixelSize: 10; font.bold: true }
                        MouseArea { anchors.fill: parent
                            onClicked: {
                                if (!panel.ctl) return
                                if (!panel.ctl.setNoReadingCondition()) condError.text = panel.ctl.lastError
                                else condError.text = ""
                            } }
                    }
                }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: "Calm is a reading. No reading means none was taken — it is not treated as calm."
                       color: panel._txtMut; font.pixelSize: 10 }
            }

            // ── PROGRESS ─────────────────────────────────────────────────
            Column {
                width: parent.width; spacing: 6
                visible: panel.inSighters || panel.counting || panel.posReview
                Rectangle { width: parent.width; height: 1; color: panel._line }
                Text { visible: panel.inSighters; text: "SIGHTERS"; color: panel._txt
                       font.pixelSize: 15; font.bold: true }
                Text { visible: panel.counting
                       text: "Shot " + (ctl ? ctl.countedShots : 0) + " of " + (ctl ? ctl.shotPlan : 0)
                       color: panel._txt; font.family: "Consolas"; font.pixelSize: 20; font.bold: true }
                Rectangle { visible: panel.counting
                    width: parent.width; height: 12; radius: 6; color: "#0E1014"
                    border.color: panel._line; border.width: 1
                    Rectangle { anchors.left: parent.left; anchors.top: parent.top
                        anchors.bottom: parent.bottom; anchors.margins: 2; radius: 5; color: panel._redHi
                        width: (ctl && ctl.shotPlan > 0)
                               ? (parent.width - 4) * Math.min(1, ctl.countedShots / ctl.shotPlan) : 0 } }
                Repeater {
                    model: {
                        if (!ctl) return []
                        var rows = [ { k: "Sighters", v: "" + ctl.sighterCount },
                                     { k: "Counted", v: "" + ctl.countedShots } ]
                        if (ctl.threePositions)
                            rows.push({ k: "Session total", v: "" + ctl.totalCountedShots })
                        rows.push({ k: "Input source", v: panel.demoMode ? "Demo" : "Live" })
                        rows.push({ k: "Target", v: panel.demoMode ? "Demo · not needed"
                                                    : (panel.connected ? "Connected" : "Not connected") })
                        return rows
                    }
                    delegate: Row { width: parent.width; spacing: 8
                        Text { text: modelData.k; color: panel._txtMut; font.pixelSize: 12
                               width: parent.width * 0.45 }
                        Text { text: modelData.v; color: panel._txt; font.pixelSize: 12; font.bold: true
                               width: parent.width * 0.45; elide: Text.ElideRight } }
                }
            }

            // ── 3P POSITION CHANGE ───────────────────────────────────────
            Column {
                width: parent.width; spacing: 6
                visible: panel.posReview && ctl && ctl.threePositions
                Rectangle { width: parent.width; height: 1; color: panel._line }
                Text { text: "NEXT POSITION"; color: panel._txtMut
                       font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                Row { spacing: 6
                    Repeater {
                        model: [ { l: "Kneeling", p: 1 }, { l: "Prone", p: 2 }, { l: "Standing", p: 3 } ]
                        delegate: Rectangle {
                            width: (bodyCol.width - 12) / 3; height: 44; radius: 8
                            color: (ctl && ctl.currentPosition === modelData.p) ? panel._red : "#1B1E24"
                            border.color: panel._line; border.width: 1
                            opacity: (ctl && ctl.currentPosition === modelData.p) ? 1.0 : 0.85
                            Text { anchors.centerIn: parent; text: modelData.l
                                   color: (ctl && ctl.currentPosition === modelData.p) ? "white" : panel._txtSec
                                   font.pixelSize: 11; font.bold: true }
                            MouseArea { anchors.fill: parent
                                onClicked: if (panel.ctl) panel.ctl.changePosition(modelData.p) }
                        }
                    }
                }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: "Kneeling, Prone and Standing are recorded separately and are never pooled."
                       color: panel._txtMut; font.pixelSize: 10 }
                // The only route from a 3P position review to the session
                // review. Without it the athlete could change position but
                // never finish — the primary action is "start counted shots".
                Rectangle {
                    width: parent.width; height: 46; radius: 8
                    color: "transparent"; border.color: panel._line; border.width: 1
                    Text { anchors.centerIn: parent; text: "END CAPTURE"; color: panel._txtSec
                           font.pixelSize: 12; font.bold: true }
                    MouseArea { anchors.fill: parent
                        onClicked: {
                            if (!panel.ctl) return
                            if (!panel.ctl.endCapture() && panel.ctl.lastError !== "")
                                dialogManager.showError(qsTr("Wind Map"), panel.ctl.lastError)
                        } }
                }
            }
        }
    }

    // ── pinned primary action ───────────────────────────────────────────
    Item {
        id: actionZone
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.bottomMargin: 16
        height: primaryLabel() !== "" ? 68 : 0
        visible: height > 0

        function primaryLabel() {
            if (!panel.ctl) return ""
            if (panel.inSetup)    return panel.ctl.sightersEnabled ? "START SIGHTERS" : "START COUNTED SHOTS"
            if (panel.inSighters) return "START COUNTED SHOTS"
            if (panel.counting)   return (panel.ctl.threePositions ? "END POSITION" : "END CAPTURE")
            if (panel.posReview)  return "START COUNTED SHOTS"
            return ""
        }
        Rectangle {
            anchors.fill: parent; radius: 10
            color: primaryMouse.pressed ? panel._red : panel._redHi
            Text { anchors.centerIn: parent; text: actionZone.primaryLabel()
                   color: "white"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 1 }
            MouseArea { id: primaryMouse; anchors.fill: parent
                onClicked: {
                    var c = panel.ctl
                    if (!c) return
                    // Each branch asks the controller for ONE transition; if it
                    // refuses, the reason is shown and nothing is assumed.
                    var ok = true
                    if (panel.inSetup)         ok = c.sightersEnabled ? c.beginSighters() : c.beginCountedShots()
                    else if (panel.inSighters) ok = c.finishSighters()
                    else if (panel.counting)   ok = c.threePositions ? c.endPosition() : c.endCapture()
                    else if (panel.posReview)  ok = c.beginCountedShots()
                    if (!ok && c.lastError !== "")
                        dialogManager.showError(qsTr("Wind Map"), c.lastError)
                } }
        }
    }
}
