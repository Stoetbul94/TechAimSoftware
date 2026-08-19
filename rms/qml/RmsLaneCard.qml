import QtQuick 2.15

// One lane, as a row-shaped card: the table/card hybrid the milestone asks
// for. It reads like a table at 20 lanes and like a card at 6.
//
// Every value shown is the NODE's value. There is no computed score here and
// no button — selecting a lane opens a detail pane and nothing else.
Rectangle {
    id: card

    property bool selected: false
    property bool offline: false
    property string laneLabel: ""
    property string athlete: ""
    property string connection: ""
    property string programmeLabel: ""
    property string programmeId: ""
    property bool officialProgramme: false
    property string phase: ""
    property string shotsLabel: ""
    property string scoreLabel: ""
    property int unobserved: 0
    property int gapCount: 0

    signal clicked()

    Theme { id: theme }

    height: 64
    radius: theme.radiusMedium
    color: selected ? theme.bgSurfaceAlt : theme.bgSurface
    border.width: 1
    border.color: selected ? theme.brandPrimary : theme.borderColor
    opacity: offline ? 0.62 : 1.0

    // Left edge: the fastest read on the whole screen.
    Rectangle {
        width: 4
        radius: 2
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom
                  leftMargin: 1; topMargin: 1; bottomMargin: 1 }
        color: card.offline ? theme.statusDisconnected
             : card.phase === "MATCH" ? theme.statusConnected
             : card.phase === "RECOVERY_REQUIRED" ? theme.brandAccent
                                                  : theme.borderColor
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
                text: card.athlete.length > 0 ? card.athlete : "—"
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textSecondary
                elide: Text.ElideRight
                width: parent.width
            }
        }

        // TARGET / CONNECTION
        RmsStatusPill {
            id: connPill
            anchors.left: laneCol.right
            anchors.verticalCenter: parent.verticalCenter
            width: 152
            text: card.connection
            tone: card.offline ? "offline"
                : card.connection === "TARGET_CONNECTED" ? "live"
                : card.connection === "TARGET_DISCONNECTED" ? "warn"
                : "neutral"
        }

        // PROGRAMME — label derived from the stable programmeId, never the
        // other way round. Elastic: it absorbs the width the fixed columns
        // on either side do not need, so the row never collides at any
        // window size.
        // An Item, not a Column: a Column sized by anchors whose children are
        // sized from the Column is a binding loop, and QML resolves it by
        // collapsing the width — which crushed the programme name to an
        // ellipsis. Anchored children of a sized Item have no such cycle.
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
                text: card.programmeLabel.length > 0 ? card.programmeLabel : "—"
                font.family: theme.fontFamily
                font.pixelSize: 13
                color: theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                id: progKind
                anchors { left: parent.left; right: parent.right
                          top: progName.bottom; topMargin: 2 }
                text: card.programmeId.length === 0 ? ""
                      : (card.officialProgramme ? "ISSF OFFICIAL COURSE"
                                                : "TECH AIM PRESET")
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 0.8
                color: theme.textSecondary
                elide: Text.ElideRight
            }
        }

        // PHASE — fixed width, pinned to the left of the numeric block.
        RmsStatusPill {
            id: phasePill
            anchors.right: figures.left
            anchors.rightMargin: theme.spacingUnit * 1.75
            anchors.verticalCenter: parent.verticalCenter
            width: 128
            text: card.phase
            tone: card.offline ? "offline"
                : card.phase === "MATCH" ? "live"
                : card.phase === "RECOVERY_REQUIRED" ? "warn"
                : "neutral"
        }

        // SHOTS + SCORE, right aligned so the numbers form a column.
        Row {
            id: figures
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: theme.spacingUnit * 1.75

            Column {
                width: 66
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
                width: 78
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
            // Observation quality. Shown rather than hidden: a gap in what
            // RMS received is a fact the range officer is entitled to see.
            Column {
                width: 82
                spacing: 2
                Text {
                    text: "OBSERVATION"
                    font.family: theme.fontFamily
                    font.pixelSize: 9
                    font.letterSpacing: 1.1
                    color: theme.textSecondary
                }
                Text {
                    text: (card.unobserved === 0 && card.gapCount === 0)
                          ? "complete"
                          : (card.unobserved > 0 ? card.unobserved + " unseen"
                                                 : card.gapCount + " gap")
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: (card.unobserved === 0 && card.gapCount === 0)
                           ? theme.textSecondary : theme.brandAccent
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: card.clicked()
    }
}
