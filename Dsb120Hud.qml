import QtQuick 2.15

// DSB 1.20 — the competition surface for the gated position sequencer.
//
// It shows the four things the rule makes an athlete and a range officer need
// at a glance: which POSITION, which PHASE, how much of THIS POSITION'S time is
// left, and how many match shots the position has. During a gate it shows that
// nothing is running - the one thing a clock display must never get wrong here.
//
// The buttons are COMPETITION CONTROL, not shooter actions, and they are styled
// apart from the athlete's controls for that reason. Every one of them asks the
// engine, which decides; a button that would be refused is simply not enabled,
// and pressing it anyway changes nothing.
Item {
    id: hud

    // Machine state, never text: the state ids come from the controller and
    // the labels are looked up from them. Translating a label can never move
    // the competition.
    readonly property int phaseId: DSB120.phaseId
    readonly property int positionIndex: DSB120.positionIndex
    readonly property int nextPositionIndex: DSB120.nextPositionIndex
    readonly property bool atGate: phaseId === 2
    readonly property bool running: phaseId === 1 || phaseId === 3 || phaseId === 4
    property int remainingSec: 0

    function positionName(index) {
        if (index === 0) return qsTr("KNEELING")
        if (index === 1) return qsTr("PRONE")
        if (index === 2) return qsTr("STANDING")
        return "—"
    }
    function phaseName(id) {
        if (id === 1) return qsTr("PREPARATION · SIGHTING")
        if (id === 2) return qsTr("POSITION CHANGE")
        if (id === 3) return qsTr("SIGHTING")
        if (id === 4) return qsTr("MATCH")
        if (id === 6) return qsTr("FINISHED")
        return "—"
    }
    function clockText(secs) {
        if (secs < 0) return "—:—"
        var m = Math.floor(secs / 60)
        var s = secs - m * 60
        return (m < 10 ? "0" + m : m) + ":" + (s < 10 ? "0" + s : s)
    }

    // The DISPLAYED clock is the ENGINE's clock, read back once a second rather
    // than counted independently in QML. A second counter would be a second
    // opinion about competition time.
    Timer {
        interval: 250; running: true; repeat: true
        onTriggered: {
            var ms = DSB120.remainingMs()
            hud.remainingSec = ms < 0 ? -1 : Math.floor(ms / 1000)
        }
    }

    implicitWidth: col.implicitWidth + 24
    implicitHeight: col.implicitHeight + 20

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "#14161c"
        border.width: 1
        border.color: hud.atGate ? theme.tokens.warningText : theme.tokens.borderSubtle
    }

    Column {
        id: col
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: "DSB 1.20  ·  " + qsTr("Air rifle 3 positions") + "  ·  "
                  + (DSB120.shotsPerPosition === 10 ? "3x10" : "3x20")
            color: theme.tokens.textSecondary
            font.family: theme.fontFamily; font.pixelSize: 11
            font.bold: true; font.letterSpacing: 1
        }

        Row {
            spacing: 18
            Column {
                spacing: 2
                Text { text: qsTr("POSITION"); color: theme.tokens.textSecondary
                       font.family: theme.fontFamily; font.pixelSize: 10; font.letterSpacing: 1 }
                Text { text: hud.positionName(hud.atGate ? hud.nextPositionIndex
                                                         : hud.positionIndex)
                       color: theme.tokens.textPrimary
                       font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true }
            }
            Column {
                spacing: 2
                Text { text: qsTr("PHASE"); color: theme.tokens.textSecondary
                       font.family: theme.fontFamily; font.pixelSize: 10; font.letterSpacing: 1 }
                Text {
                    // At the gate the phase says what is happening AND what is
                    // being waited for, because "position change" alone reads
                    // as if the next clock were already running.
                    text: hud.atGate
                          ? qsTr("WAITING FOR %1 START").arg(hud.positionName(hud.nextPositionIndex))
                          : hud.phaseName(hud.phaseId)
                    color: hud.atGate ? theme.tokens.warningText : theme.tokens.textPrimary
                    font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true
                }
            }
            Column {
                spacing: 2
                Text { text: qsTr("TIME"); color: theme.tokens.textSecondary
                       font.family: theme.fontFamily; font.pixelSize: 10; font.letterSpacing: 1 }
                Text {
                    // No clock exists during a gate, and none is shown. A dash
                    // is the honest reading; a frozen number would look running.
                    text: hud.running ? hud.clockText(hud.remainingSec) : qsTr("NOT RUNNING")
                    color: hud.running ? theme.tokens.textPrimary : theme.tokens.warningText
                    font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true
                }
            }
            Column {
                spacing: 2
                Text { text: qsTr("MATCH SHOTS"); color: theme.tokens.textSecondary
                       font.family: theme.fontFamily; font.pixelSize: 10; font.letterSpacing: 1 }
                Text { text: DSB120.matchShotsInPosition + " / " + DSB120.shotsPerPosition
                       color: theme.tokens.textPrimary
                       font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true }
            }
        }

        // ── competition control ──────────────────────────────────────────
        Row {
            spacing: 8
            Text {
                text: qsTr("COMPETITION CONTROL")
                color: theme.tokens.textSecondary
                font.family: theme.fontFamily; font.pixelSize: 10; font.letterSpacing: 1
                anchors.verticalCenter: parent.verticalCenter
            }
            Repeater {
                model: [
                    { key: "start",  label: qsTr("START %1").arg(hud.positionName(hud.nextPositionIndex)),
                      on: hud.atGate },
                    { key: "match",  label: qsTr("TO MATCH"), on: hud.phaseId === 3 },
                    { key: "end",    label: qsTr("END POSITION"),
                      on: hud.phaseId === 3 || hud.phaseId === 4 }
                ]
                Rectangle {
                    width: ctlLabel.implicitWidth + 20; height: 26; radius: 4
                    color: modelData.on ? theme.tokens.accentPrimary : "#1e2129"
                    border.width: 1
                    border.color: modelData.on ? theme.tokens.accentPrimary
                                               : theme.tokens.borderSubtle
                    opacity: modelData.on ? 1.0 : 0.45
                    Text {
                        id: ctlLabel
                        anchors.centerIn: parent; text: modelData.label
                        color: modelData.on ? theme.tokens.textOnAccent : theme.tokens.textSecondary
                        font.family: theme.fontFamily; font.pixelSize: 11; font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: modelData.on
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // The engine is asked, and the engine answers. QML
                            // never decides that a transition is legal.
                            if (modelData.key === "start")
                                DSB120.startPosition(hud.nextPositionIndex)
                            else if (modelData.key === "match")
                                DSB120.enterMatchPhase()
                            else
                                DSB120.endPosition()
                        }
                    }
                }
            }
        }
    }
}
