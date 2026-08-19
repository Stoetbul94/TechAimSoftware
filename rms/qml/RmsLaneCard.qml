import QtQuick 2.15

// One PHYSICAL LANE, as a row-shaped card.
//
// This is a range officer's screen, not an engineer's. Node ids, boot ids,
// protocol versions, duplicate counts and sequence gaps have all moved to the
// lane's diagnostics panel: valuable, but not what someone running a relay
// needs in front of them. What is left is what they act on — which lane, who
// is on it, is the target answering, what are they shooting, where are they in
// it, how many, and what score.
Rectangle {
    id: card

    property bool selected: false
    property int  laneNumber: 0
    property string laneLabel: ""
    property bool hasDevice: false
    property bool online: false
    property bool laneEnabled: true
    property string athlete: ""
    property string connection: ""
    property string statusText: ""
    property string programmeLabel: ""
    property string programmeId: ""
    property bool officialProgramme: false
    property string phase: ""
    property string shotsLabel: ""
    property string scoreLabel: ""
    property int unobserved: 0
    // ── the PLAN's intention, shown BESIDE the observation above ─────────
    // Never merged into it: RMS has not told the station anything, so the two
    // may legitimately differ and the operator needs to see both.
    property bool inPlan: false
    property string plannedAthlete: ""
    property string plannedProgramme: ""
    property bool programmeMismatch: false

    signal clicked()

    Theme { id: theme }

    height: 64
    radius: theme.radiusMedium
    color: selected ? theme.bgSurfaceAlt : theme.bgSurface
    border.width: 1
    border.color: selected ? theme.brandPrimary : theme.borderColor
    // A lane with nothing on it is present but quiet; a lane out of service is
    // quieter still. Neither disappears.
    opacity: !laneEnabled ? 0.45 : (online ? 1.0 : 0.66)

    Rectangle {
        width: 4
        radius: 2
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom
                  leftMargin: 1; topMargin: 1; bottomMargin: 1 }
        color: card.programmeMismatch ? theme.brandAccent
             : !card.hasDevice ? theme.borderColor
             : !card.online ? theme.statusDisconnected
             : card.phase === "MATCH" ? theme.statusConnected
             : card.phase === "RECOVERY_REQUIRED" ? theme.brandAccent
                                                  : theme.borderColor
    }

    // A lane taking part in the plan being edited.
    Rectangle {
        visible: card.inPlan
        anchors { top: parent.top; right: parent.right; topMargin: 1; rightMargin: 1 }
        width: 44; height: 14
        radius: 2
        color: Qt.rgba(0.66, 0, 0.22, 0.22)
        Text {
            anchors.centerIn: parent
            text: "IN PLAN"
            font.family: theme.fontFamily
            font.pixelSize: 8
            font.letterSpacing: 0.6
            color: theme.brandPrimary
        }
    }

    Item {
        anchors.fill: parent
        anchors.leftMargin: theme.spacingUnit * 2.5
        anchors.rightMargin: theme.spacingUnit * 2.5

        // LANE + ATHLETE
        Column {
            id: laneCol
            width: 128
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: card.laneLabel
                font.family: theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            Text {
                // The station's own athlete text if it reports one; otherwise
                // the planned athlete, explicitly marked as a plan value.
                text: card.athlete.length > 0
                      ? card.athlete
                      : (card.plannedAthlete.length > 0
                            ? card.plannedAthlete + "  (planned)"
                            : (card.online ? "—" : ""))
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: (card.athlete.length === 0 && card.plannedAthlete.length > 0)
                       ? theme.brandPrimary : theme.textSecondary
                elide: Text.ElideRight
                width: parent.width
            }
        }

        // TARGET STATUS
        RmsStatusPill {
            id: connPill
            anchors.left: laneCol.right
            anchors.verticalCenter: parent.verticalCenter
            width: 140
            text: card.connection
            tone: !card.hasDevice ? "neutral"
                : !card.online ? "offline"
                : card.connection === "TARGET_CONNECTED" ? "live"
                : card.connection === "TARGET_DISCONNECTED" ? "warn"
                : "neutral"
        }

        // PROGRAMME, or why there is none
        Item {
            id: progCol
            anchors.left: connPill.right
            anchors.leftMargin: theme.spacingUnit * 1.75
            anchors.right: phasePill.left
            anchors.rightMargin: theme.spacingUnit * 1.75
            anchors.verticalCenter: parent.verticalCenter
            height: progName.height + progKind.height + 2

            Text {
                id: progName
                anchors { left: parent.left; right: parent.right; top: parent.top }
                text: card.programmeLabel.length > 0 ? card.programmeLabel
                                                     : card.statusText
                font.family: theme.fontFamily
                font.pixelSize: 13
                color: card.programmeLabel.length > 0 ? theme.textPrimary
                                                      : theme.textSecondary
                elide: Text.ElideRight
            }
            Text {
                id: progKind
                anchors { left: parent.left; right: parent.right
                          top: progName.bottom; topMargin: 2 }
                // A station set to something other than the plan is the one
                // thing worth saying here instead of the course type. It will
                // matter a great deal once commands exist; for now it is
                // information, and RMS changes neither side.
                text: card.programmeMismatch
                      ? "⚠ DOES NOT MATCH PLAN · " + card.plannedProgramme
                      : (card.programmeId.length === 0 ? ""
                         : (card.officialProgramme ? "ISSF OFFICIAL COURSE"
                                                   : "TECH AIM PRESET"))
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 0.8
                color: card.programmeMismatch ? theme.brandAccent : theme.textSecondary
                elide: Text.ElideRight
            }
        }

        RmsStatusPill {
            id: phasePill
            anchors.right: figures.left
            anchors.rightMargin: theme.spacingUnit * 1.75
            anchors.verticalCenter: parent.verticalCenter
            width: 100
            visible: card.phase.length > 0
            text: card.phase
            tone: !card.online ? "offline"
                : card.phase === "MATCH" ? "live"
                : card.phase === "RECOVERY_REQUIRED" ? "warn"
                : "neutral"
        }

        Row {
            id: figures
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: theme.spacingUnit * 1.25

            Column {
                width: 58
                spacing: 2
                Text {
                    text: "SHOTS"
                    font.family: theme.fontFamily
                    font.pixelSize: 9
                    font.letterSpacing: 1.1
                    color: theme.textSecondary
                }
                Text {
                    text: card.shotsLabel
                    font.family: theme.fontFamily
                    font.pixelSize: 18
                    color: theme.textPrimary
                }
            }
            Column {
                width: 70
                spacing: 2
                Text {
                    text: "SCORE"
                    font.family: theme.fontFamily
                    font.pixelSize: 9
                    font.letterSpacing: 1.1
                    color: theme.textSecondary
                }
                Text {
                    text: card.scoreLabel
                    font.family: theme.fontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: theme.textPrimary
                }
            }
            // The single piece of observation quality that survives onto the
            // operator's row: RMS is missing shots the node says it accepted.
            // That changes what the officer believes about the score, so it
            // does not belong buried in diagnostics.
            Column {
                width: 82
                spacing: 2
                visible: card.unobserved > 0
                Text {
                    text: "OBSERVATION"
                    font.family: theme.fontFamily
                    font.pixelSize: 9
                    font.letterSpacing: 1.1
                    color: theme.textSecondary
                }
                Text {
                    text: card.unobserved + " unseen"
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.brandAccent
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: card.clicked()
    }
}
