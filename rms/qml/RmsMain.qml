import QtQuick 2.15
import QtQuick.Window 2.15

// TECH AIM RANGE MANAGEMENT SYSTEM — read-only range view.
//
// Tech Aim branding only: the shared Theme, the Tech Aim wordmark, no SETA
// asset and no customer-specific product identity.
//
// There is no control anywhere on this screen by design. The only interaction
// is selecting a lane to inspect it.
Window {
    id: root
    visible: true
    width: 1440
    height: 880
    minimumWidth: 1100
    minimumHeight: 700
    title: "Tech Aim — Range Management System"
    color: theme.bgBase

    Theme { id: theme }

    property int selectedRow: -1
    // Re-read on every model change; the detail pane is a projection, so
    // recomputing it is correct and keeps one source of truth.
    property var selectedInfo: ({})
    property var selectedShots: []

    function refreshSelection() {
        if (selectedRow < 0 || selectedRow >= RANGE.nodeCount) {
            selectedInfo = ({})
            selectedShots = []
            return
        }
        selectedInfo = RANGE.nodeDetail(selectedRow)
        selectedShots = RANGE.recentShots(selectedRow, 12)
    }

    Connections {
        target: RANGE
        function onSummaryChanged() { root.refreshSelection() }
    }

    // ── header ──────────────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 76
        color: theme.bgSurface

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color: theme.borderColor
        }

        Image {
            id: logo
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: theme.spacingUnit * 3
            source: "qrc:/images/logo/techaim_white.png"
            fillMode: Image.PreserveAspectFit
            height: 34
            mipmap: true
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: logo.right
            anchors.leftMargin: theme.spacingUnit * 3
            spacing: 2
            Text {
                text: "RANGE MANAGEMENT SYSTEM"
                font.family: theme.fontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                color: theme.textPrimary
            }
            Text {
                text: "The target node remains authoritative. RMS observes; it does not score and does not command."
                font.family: theme.fontFamily
                font.pixelSize: 11
                color: theme.textSecondary
            }
        }
    }

    // ── summary ─────────────────────────────────────────────────────────
    RmsSummaryBar {
        id: summary
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 2
        nodeCount: RANGE.nodeCount
        onlineCount: RANGE.onlineCount
        offlineCount: RANGE.offlineCount
        rejectedDatagrams: RANGE.rejectedDatagrams
        protocolVersion: RMS_PROTOCOL_VERSION
        observationPort: RMS_OBSERVATION_PORT
        simulated: RMS_SIMULATED
    }

    // ── lanes ───────────────────────────────────────────────────────────
    Item {
        id: lanes
        anchors {
            top: summary.bottom; left: parent.left; bottom: parent.bottom
            topMargin: theme.spacingUnit * 2
            leftMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: parent.width - detailPane.width - theme.spacingUnit * 6

        Text {
            id: lanesTitle
            text: "RANGE  ·  " + RANGE.nodeCount + " LANES"
            font.family: theme.fontFamily
            font.pixelSize: 11
            font.letterSpacing: 1.4
            color: theme.textSecondary
        }

        ListView {
            anchors { top: lanesTitle.bottom; topMargin: theme.spacingUnit
                      left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: theme.spacingUnit
            model: RANGE

            delegate: RmsLaneCard {
                width: ListView.view.width
                selected: root.selectedRow === index
                offline: model.offline
                laneLabel: model.laneLabel
                athlete: model.athlete
                connection: model.connection
                programmeLabel: model.programmeLabel
                programmeId: model.programmeId
                officialProgramme: model.officialProgramme
                phase: model.phase
                shotsLabel: model.shotsLabel
                scoreLabel: model.scoreLabel
                unobserved: model.unobserved
                gapCount: model.gapCount
                onClicked: {
                    root.selectedRow = index
                    root.refreshSelection()
                }
            }

            // Nothing observed yet is a state worth naming, not an empty box.
            Text {
                anchors.centerIn: parent
                visible: RANGE.nodeCount === 0
                text: RMS_SIMULATED
                      ? "Starting the simulated range…"
                      : "Listening on UDP " + RMS_OBSERVATION_PORT
                        + " — no target node has announced itself yet."
                font.family: theme.fontFamily
                font.pixelSize: 13
                color: theme.textSecondary
            }
        }
    }

    // ── detail ──────────────────────────────────────────────────────────
    RmsLaneDetail {
        id: detailPane
        anchors {
            top: summary.bottom; right: parent.right; bottom: parent.bottom
            topMargin: theme.spacingUnit * 2
            rightMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: 430
        info: root.selectedInfo
        shots: root.selectedShots
    }

    // Select the first lane as soon as one appears, so the detail pane is
    // never an unexplained blank on first run.
    Timer {
        interval: 400
        running: root.selectedRow < 0
        repeat: true
        onTriggered: {
            if (RANGE.nodeCount > 0) {
                root.selectedRow = 0
                root.refreshSelection()
            }
        }
    }
}
