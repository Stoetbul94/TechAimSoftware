import QtQuick 2.15

// Wind Map (Release 2) — session review + completion overlay.
//
// During setup/sighters/counted shots the target stays dominant and only the
// transient shot acknowledgement appears; the persistent controls live in
// WindMapRightPanel.
//
// STAGE 5 SCOPE. This shows what was RECORDED — counts, positions, and each
// shot beside the condition that was standing when it was fired. It performs
// no condition comparison, offers no coaching narrative, and produces no PDF.
// Those are the analytics stage, not this one.
Item {
    id: hud
    property var ctl: null                  // WINDMAP
    signal homeRequested()
    signal newSessionRequested()

    readonly property bool reviewOpen: ctl && ctl.phase === 5
    readonly property bool doneOpen:   ctl && ctl.phase === 6

    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _amber:  "#E0A800"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    // A shot's recorded condition, rendered as the fact it is.
    function condText(s) {
        if (!s.hasWindReading) return "No reading"
        if (s.calm) return "Calm"
        return s.directionDegrees + "° " + s.directionLabel + " · "
               + Number(s.speedMetresPerSecond).toFixed(1) + " m/s"
    }
    function condColor(s) {
        if (!s.hasWindReading) return _amber
        if (s.calm) return _txtSec
        return _txt
    }

    // ── transient shot acknowledgement ───────────────────────────────────
    Rectangle {
        id: ack
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 90
        width: ackT.implicitWidth + 40; height: 44; radius: 22
        color: "#CC1B1E24"; border.color: _line; border.width: 1
        opacity: 0; visible: opacity > 0
        Text { id: ackT; anchors.centerIn: parent; text: ""; color: hud._txt
               font.pixelSize: 13; font.bold: true }
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Timer { id: ackTimer; interval: 1600; onTriggered: ack.opacity = 0 }
        function show(msg) { ackT.text = msg; opacity = 1; ackTimer.restart() }
    }
    Connections {
        target: hud.ctl
        enabled: hud.ctl !== null
        function onShotAccepted(sighter, shotId, position) {
            // The acknowledgement names the CONDITION the shot just recorded,
            // so the athlete can see immediately what was attached to it.
            ack.show((sighter ? "Sighter · " : "Shot " + hud.ctl.countedShots + " · ")
                     + hud.ctl.conditionSummary)
        }
        function onShotRejected(reason) {
            if (reason === "SetupShotIgnored")
                ack.show("Shot ignored — the session has not started shooting yet")
            else if (reason === "DuplicateShot")
                ack.show("Repeated shot ignored")
            else if (reason === "WrongInputSource")
                ack.show("Shot refused — wrong input source for this mode")
            else if (reason !== "")
                ack.show("Shot not recorded")
        }
    }

    // ── SESSION REVIEW / COMPLETED ───────────────────────────────────────
    Rectangle {
        visible: hud.reviewOpen || hud.doneOpen
        anchors.fill: parent; color: "#EA0F1116"
        MouseArea { anchors.fill: parent }     // swallow clicks behind the panel

        Rectangle {
            width: Math.min(1080, parent.width - 40)
            height: Math.min(800, parent.height - 30)
            anchors.centerIn: parent
            color: hud._card; radius: 12; border.color: hud._line; border.width: 1

            Flickable {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: actions.top
                anchors.margins: 22; anchors.bottomMargin: 10
                clip: true; contentWidth: width; contentHeight: rvCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: rvCol
                    width: parent.width; spacing: 14
                    property var m: (hud.ctl && (hud.reviewOpen || hud.doneOpen))
                                    ? hud.ctl.reviewSummary() : ({})
                    property var shots: (hud.ctl && (hud.reviewOpen || hud.doneOpen))
                                        ? hud.ctl.reviewShots() : []

                    Text { text: hud.doneOpen ? "WIND MAP — SESSION COMPLETE"
                                              : "WIND MAP — SESSION REVIEW"
                           color: hud._txt; font.pixelSize: 24; font.bold: true }
                    Text { text: (rvCol.m.disciplineName || "") + " · "
                                 + (rvCol.m.athlete || "") + " · " + (rvCol.m.operatingMode || "")
                           color: hud._txtMut; font.pixelSize: 12 }

                    // Counts — facts, nothing derived from them.
                    Row {
                        width: parent.width; spacing: 10
                        Repeater {
                            model: [
                                { k: "COUNTED SHOTS", v: "" + (rvCol.m.countedShots || 0) },
                                { k: "SIGHTERS",      v: "" + (rvCol.m.sighterShots || 0) },
                                { k: "CONDITIONS",    v: "" + (rvCol.m.conditionChanges || 0) },
                                { k: "WITH READING",  v: "" + (rvCol.m.countedWithReading || 0) },
                                { k: "CALM",          v: "" + (rvCol.m.countedCalm || 0) },
                                { k: "NO READING",    v: "" + (rvCol.m.countedNoReading || 0) }
                            ]
                            delegate: Rectangle {
                                width: (rvCol.width - 50) / 6; height: 74; radius: 8
                                color: "#0E1014"; border.color: hud._line; border.width: 1
                                Column {
                                    anchors.centerIn: parent; spacing: 4
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.v; color: hud._txt
                                           font.family: "Consolas"; font.pixelSize: 22; font.bold: true }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.k; color: hud._txtMut
                                           font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                                }
                            }
                        }
                    }
                    Text { text: "Sighters are recorded but are never included in the counted totals."
                           color: hud._txtMut; font.pixelSize: 10 }

                    // 3P — the three positions side by side, never pooled.
                    Column {
                        width: parent.width; spacing: 6
                        visible: rvCol.m.threePositions === true
                        Rectangle { width: parent.width; height: 1; color: hud._line }
                        Text { text: "BY POSITION"; color: hud._txtMut
                               font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                        Repeater {
                            model: rvCol.m.positions ? rvCol.m.positions : []
                            delegate: Row {
                                width: parent.width; spacing: 8
                                Text { text: modelData.positionName; color: hud._txt
                                       font.pixelSize: 13; font.bold: true; width: parent.width * 0.3 }
                                Text { text: modelData.countedShots + " counted"
                                       color: hud._txtSec; font.pixelSize: 12; width: parent.width * 0.3 }
                                Text { text: modelData.sighterShots + " sighters"
                                       color: hud._txtMut; font.pixelSize: 12 }
                            }
                        }
                    }

                    // The record itself: each shot beside the condition it kept.
                    Rectangle { width: parent.width; height: 1; color: hud._line }
                    Text { text: "SHOT RECORD"; color: hud._txtMut
                           font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                    Row {
                        width: parent.width; spacing: 8
                        Text { text: "#";        color: hud._txtMut; font.pixelSize: 10; width: parent.width * 0.07 }
                        Text { text: "TYPE";     color: hud._txtMut; font.pixelSize: 10; width: parent.width * 0.13 }
                        Text { text: "POSITION"; color: hud._txtMut; font.pixelSize: 10; width: parent.width * 0.15
                               visible: rvCol.m.threePositions === true }
                        Text { text: "SCORE";    color: hud._txtMut; font.pixelSize: 10; width: parent.width * 0.12 }
                        Text { text: "CONDITION AT THE SHOT"; color: hud._txtMut; font.pixelSize: 10 }
                    }
                    Repeater {
                        model: rvCol.shots
                        delegate: Row {
                            width: parent.width; spacing: 8; height: 24
                            Text { text: modelData.shotId; color: hud._txtSec
                                   font.family: "Consolas"; font.pixelSize: 12; width: parent.width * 0.07 }
                            Text { text: modelData.sighter ? "Sighter" : "Counted"
                                   color: modelData.sighter ? hud._txtMut : hud._txt
                                   font.pixelSize: 12; width: parent.width * 0.13 }
                            Text { text: modelData.positionName; color: hud._txtSec
                                   font.pixelSize: 12; width: parent.width * 0.15
                                   visible: rvCol.m.threePositions === true }
                            Text { text: Number(modelData.score).toFixed(1); color: hud._txtSec
                                   font.family: "Consolas"; font.pixelSize: 12; width: parent.width * 0.12 }
                            Text { text: hud.condText(modelData); color: hud.condColor(modelData)
                                   font.pixelSize: 12 }
                        }
                    }

                    Rectangle { width: parent.width; height: 1; color: hud._line }
                    Text { width: parent.width; wrapMode: Text.WordWrap
                           text: rvCol.m.disclaimer || ""
                           color: hud._txtMut; font.pixelSize: 11; font.italic: true }
                }
            }

            Row {
                id: actions
                anchors.bottom: parent.bottom; anchors.right: parent.right
                anchors.margins: 22; spacing: 10; height: 52

                Rectangle {
                    visible: hud.doneOpen
                    width: 150; height: 52; radius: 8
                    color: "transparent"; border.color: hud._line; border.width: 1
                    Text { anchors.centerIn: parent; text: "New session"; color: hud._txtSec
                           font.pixelSize: 13 }
                    MouseArea { anchors.fill: parent; onClicked: hud.newSessionRequested() }
                }
                Rectangle {
                    width: 190; height: 52; radius: 8
                    color: completeMouse.pressed ? hud._red : hud._redHi
                    Text { anchors.centerIn: parent
                           text: hud.doneOpen ? "Home" : "Complete session"
                           color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { id: completeMouse; anchors.fill: parent
                        onClicked: {
                            if (hud.doneOpen) { hud.homeRequested(); return }
                            if (!hud.ctl) return
                            if (!hud.ctl.completeSession() && hud.ctl.lastError !== "")
                                dialogManager.showError(qsTr("Wind Map"), hud.ctl.lastError)
                        } }
                }
            }
        }
    }
}
