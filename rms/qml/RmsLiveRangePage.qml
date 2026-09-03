import QtQuick 2.15

// LIVE RANGE — every physical lane, whether or not it is answering.
Item {
    id: page

    Theme { id: theme }

    property int selectedRow: -1
    property var selectedInfo: ({})
    property var selectedShots: []

    function refreshSelection() {
        if (selectedRow < 0 || selectedRow >= LANES.laneCount) {
            selectedInfo = ({})
            selectedShots = []
            return
        }
        selectedInfo = LANES.laneDetail(selectedRow)
        selectedShots = LANES.recentShots(selectedRow, 12)
    }

    Connections {
        target: LANES
        function onSummaryChanged() { page.refreshSelection() }
    }

    Row {
        id: summary
        anchors { top: parent.top; left: parent.left }
        anchors.margins: theme.spacingUnit * 2
        spacing: theme.spacingUnit * 1.5

        RmsStatTile {
            width: 150
            label: "PHYSICAL LANES"
            value: String(RANGECONFIG.laneCount)
        }
        RmsStatTile {
            width: 130
            label: "ONLINE"
            value: String(LANES.onlineCount)
            tone: "live"
        }
        RmsStatTile {
            width: 130
            label: "OFFLINE"
            value: String(LANES.offlineCount)
            tone: LANES.offlineCount > 0 ? "warn" : "neutral"
        }
        RmsStatTile {
            width: 170
            label: "UNASSIGNED DEVICES"
            value: String(UNASSIGNED.count)
            tone: UNASSIGNED.count > 0 ? "warn" : "neutral"
        }
    }

    RmsControlStatusBar {
        id: controlBar
        anchors {
            top: summary.bottom; left: parent.left
            topMargin: theme.spacingUnit * 1.5
            leftMargin: theme.spacingUnit * 2
        }
        width: parent.width - detailPane.width - theme.spacingUnit * 6
    }

    Item {
        id: lanes
        anchors {
            top: controlBar.bottom; left: parent.left; bottom: parent.bottom
            topMargin: theme.spacingUnit * 2
            leftMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: parent.width - detailPane.width - theme.spacingUnit * 6

        Text {
            id: title
            text: (RANGECONFIG.rangeName.length > 0
                       ? RANGECONFIG.rangeName.toUpperCase() : "RANGE")
                  + "  ·  " + RANGECONFIG.laneCount + " PHYSICAL LANES"
            font.family: theme.fontFamily
            font.pixelSize: 11
            font.letterSpacing: 1.4
            color: theme.textSecondary
        }

        ListView {
            anchors { top: title.bottom; topMargin: theme.spacingUnit
                      left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: theme.spacingUnit
            model: LANES

            delegate: RmsLaneCard {
                width: ListView.view.width
                selected: page.selectedRow === index
                laneNumber: model.laneNumber
                laneLabel: model.laneLabel
                hasDevice: model.hasDevice
                online: model.online
                laneEnabled: model.laneEnabled
                athlete: model.athlete
                connection: model.connection
                statusText: model.statusText
                programmeLabel: model.programmeLabel
                programmeId: model.programmeId
                officialProgramme: model.officialProgramme
                phase: model.phase
                shotsLabel: model.shotsLabel
                scoreLabel: model.scoreLabel
                unobserved: model.unobserved
                inPlan: model.inPlan
                plannedAthlete: model.plannedAthlete
                plannedProgramme: model.plannedProgramme
                programmeMismatch: model.programmeMismatch
                competitionStatus: model.competitionStatus
                competitionTerminal: model.competitionTerminal
                eliminated: model.eliminated
                finalRankLabel: model.finalRankLabel
                finalScoreLabel: model.finalScoreLabel
                competitionSimulated: model.competitionSimulated
                onClicked: {
                    page.selectedRow = index
                    page.refreshSelection()
                }
            }

            Text {
                anchors.centerIn: parent
                visible: LANES.laneCount === 0
                text: "No lanes configured. Create a range in Range Setup."
                font.family: theme.fontFamily
                font.pixelSize: 13
                color: theme.textSecondary
            }
        }
    }

    RmsLaneDetail {
        id: detailPane
        anchors {
            top: summary.top; right: parent.right; bottom: parent.bottom
            rightMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: 380
        info: page.selectedInfo
        shots: page.selectedShots
    }

    // Select the first lane as soon as one exists. A range configured after the
    // page opened (or a first run) would otherwise leave the detail pane
    // unexplained-blank.
    Timer {
        interval: 400
        running: page.selectedRow < 0
        repeat: true
        onTriggered: {
            if (LANES.laneCount > 0) {
                // A development hook lets a capture open on a named lane; the
                // first lane is the normal behaviour.
                var want = (typeof RMS_INITIAL_LANE !== "undefined" && RMS_INITIAL_LANE > 0)
                           ? RMS_INITIAL_LANE - 1 : 0
                page.selectedRow = Math.min(want, LANES.laneCount - 1)
                page.refreshSelection()
            }
        }
    }
}
