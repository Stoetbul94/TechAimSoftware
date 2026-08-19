import QtQuick 2.15

// One lane in the ALL TARGETS overview.
//
// A range officer's tile: lane, athlete, the face, what the node scored and
// whether the station is answering. No node id, no boot id, no protocol
// counters — those live in Live Range → Diagnostics.
Rectangle {
    id: card

    property var lane: ({})
    property bool selected: false
    property bool compact: false
    signal activated()

    Theme { id: theme }

    function f(key, fallback) {
        return (lane && lane[key] !== undefined && lane[key] !== "")
               ? lane[key] : (fallback !== undefined ? fallback : "")
    }
    readonly property bool online: lane && lane.online === true
    readonly property bool eliminated: lane && lane.eliminated === true
    readonly property bool finished: lane && lane.finished === true
    readonly property bool terminal: lane && lane.competitionTerminal === true
    readonly property int unseen: lane && lane.unseenShotCount ? lane.unseenShotCount : 0

    radius: theme.radiusMedium
    color: selected ? theme.bgSurfaceAlt : theme.bgSurface
    border.width: selected ? 2 : 1
    border.color: selected ? theme.brandPrimary
                : eliminated ? theme.brandAccent
                             : theme.borderColor
    // A lane with no device is quiet; an offline one is dimmer still. Neither
    // disappears, and a TERMINAL competition state is not dimmed at all — it
    // is not a fault.
    opacity: !card.f("hasDevice", false) ? 0.55 : (online ? 1.0 : 0.72)

    // ── header ──────────────────────────────────────────────────────────
    Item {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit
        height: 34

        Text {
            id: laneLabel
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            text: card.f("laneLabel", "—")
            font.family: theme.fontFamily
            font.pixelSize: card.compact ? 14 : 17
            font.weight: Font.DemiBold
            color: theme.textPrimary
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            spacing: 5
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8; radius: 4
                color: !card.f("hasDevice", false) ? theme.borderColor
                     : card.online ? theme.statusConnected
                                   : theme.statusDisconnected
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: card.f("connection", "—")
                font.family: theme.fontFamily
                font.pixelSize: 9
                font.letterSpacing: 0.8
                color: card.online ? theme.statusConnected : theme.textSecondary
            }
        }
    }

    Text {
        id: athleteLabel
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        anchors.leftMargin: theme.spacingUnit
        anchors.rightMargin: theme.spacingUnit
        elide: Text.ElideRight
        text: card.f("athlete", card.f("statusText", "—"))
        font.family: theme.fontFamily
        font.pixelSize: card.compact ? 11 : 13
        color: card.f("athlete", "").length > 0 ? theme.textSecondary
                                                : theme.statusDisconnected
    }

    // ── the face ────────────────────────────────────────────────────────
    RmsTargetView {
        id: target
        anchors {
            top: athleteLabel.bottom; left: parent.left; right: parent.right
            bottom: footer.top
            margins: theme.spacingUnit * 0.5
        }
        targetStandardId: card.f("targetStandardId", "")
        shots: (card.lane && card.lane.shots) ? card.lane.shots : []
        showRingNumbers: !card.compact
        showLastShotLabel: false
        stale: card.f("hasDevice", false) && !card.online
    }

    // A terminal competition state replaces the live emphasis. The face stays
    // visible behind it — dimmed, not deleted — but the words sit on a solid
    // band so ring numbers never read through them.
    Rectangle {
        anchors.fill: target
        visible: card.terminal
        color: Qt.rgba(0, 0, 0, 0.72)
        radius: theme.radiusSmall

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - theme.spacingUnit
            height: terminalCol.implicitHeight + theme.spacingUnit * 1.5
            radius: theme.radiusSmall
            color: theme.bgSurface
            border.width: 1
            border.color: card.eliminated ? theme.brandAccent : theme.borderColor

            Column {
                id: terminalCol
                anchors.centerIn: parent
                width: parent.width - theme.spacingUnit
                spacing: 3
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: card.f("competitionStatus", "")
                    font.family: theme.fontFamily
                    font.pixelSize: card.compact ? 13 : 17
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    // FINISHED and ELIMINATED are never the same label or the
                    // same colour: completing a course is not being counted out.
                    color: card.eliminated ? theme.brandAccent : theme.textPrimary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: card.f("finalRankLabel", "").length > 0
                    text: card.f("finalRankLabel", "").toUpperCase() + " PLACE"
                    font.family: theme.fontFamily
                    font.pixelSize: card.compact ? 10 : 12
                    color: theme.textSecondary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: card.f("finalScoreLabel", "—") !== "—"
                    text: card.f("finalScoreLabel", "")
                    font.family: theme.fontFamily
                    font.pixelSize: card.compact ? 14 : 18
                    font.weight: Font.DemiBold
                    color: theme.textPrimary
                }
                Rectangle {
                    visible: card.lane && card.lane.competitionSimulated === true
                    width: parent.width
                    height: 15
                    radius: 2
                    color: theme.brandAccent
                    Text {
                        anchors.centerIn: parent
                        text: "SIMULATED"
                        font.family: theme.fontFamily
                        font.pixelSize: 9
                        font.letterSpacing: 1.0
                        color: theme.textOnBrand
                    }
                }
            }
        }
    }

    // ── footer ──────────────────────────────────────────────────────────
    Item {
        id: footer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        anchors.margins: theme.spacingUnit
        height: 30

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            text: card.f("shotsLabel", "—")
            font.family: theme.fontFamily
            font.pixelSize: card.compact ? 13 : 16
            color: theme.textPrimary
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            // The NODE's running total.
            text: card.f("nodeTotalLabel", "—")
            font.family: theme.fontFamily
            font.pixelSize: card.compact ? 14 : 18
            font.weight: Font.DemiBold
            color: theme.textPrimary
        }
        // RMS is missing impacts the node accepted: the face is not the whole
        // match, and the tile says so rather than implying it is.
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            visible: card.unseen > 0
            text: "⚠ " + card.unseen + " unseen"
            font.family: theme.fontFamily
            font.pixelSize: 10
            color: theme.brandAccent
        }
    }

    // The plan and the station disagree about the programme. An icon, not an
    // essay — the detail is on the Live Range.
    Text {
        visible: card.lane && card.lane.programmeMismatch === true
        anchors { top: parent.top; right: parent.right; margins: 3 }
        text: "⚠"
        font.pixelSize: 13
        color: theme.brandAccent
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: card.activated()
    }
}
