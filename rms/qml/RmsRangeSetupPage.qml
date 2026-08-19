import QtQuick 2.15

// RANGE SETUP — the range definition, the lane list, and which station stands
// on which lane.
//
// Everything on this page writes RMS's OWN configuration and nothing else. No
// control reaches a target: assigning lane 4 records where a station is, and
// tells the station nothing at all.
Item {
    id: page

    Theme { id: theme }

    // The lane currently collecting an assignment, or -1.
    property int assigningLane: -1
    property string rejection: ""

    Connections {
        target: RANGECONFIG
        function onAssignmentRejected(reason) { page.rejection = reason }
        function onRangeChanged() { page.rejection = "" }
    }

    // ── range summary ───────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 2
        height: 76
        radius: theme.radiusMedium
        color: theme.bgSurface
        border.width: 1
        border.color: theme.borderColor

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: theme.spacingUnit * 2.5
            spacing: 3
            Text {
                text: RANGECONFIG.rangeName.length > 0
                      ? RANGECONFIG.rangeName : "No range configured"
                font.family: theme.fontFamily
                font.pixelSize: 19
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            Text {
                text: RANGECONFIG.configured
                      ? RANGECONFIG.rangeMode + " range  ·  "
                        + (RANGECONFIG.rangeType.length > 0
                              ? RANGECONFIG.rangeType + "  ·  " : "")
                        + RANGECONFIG.laneCount + " physical lanes ("
                        + RANGECONFIG.firstLaneNumber + "–"
                        + RANGECONFIG.lastLaneNumber + ")  ·  "
                        + RANGECONFIG.assignedLaneCount + " assigned"
                      : "Create a range to begin."
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textSecondary
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: theme.spacingUnit * 2.5
            spacing: theme.spacingUnit

            RmsStatusPill {
                visible: RANGECONFIG.configLocked
                text: "CONFIG WRITTEN BY A NEWER RMS — READ ONLY"
                tone: "warn"
            }
            RmsStatusPill {
                text: RANGECONFIG.assignedLaneCount + " / "
                      + RANGECONFIG.laneCount + " LANES ASSIGNED"
                tone: "neutral"
            }
        }
    }

    Text {
        id: rejectionText
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        anchors.leftMargin: theme.spacingUnit * 2
        anchors.rightMargin: theme.spacingUnit * 2
        anchors.topMargin: 4
        visible: page.rejection.length > 0
        text: page.rejection
        wrapMode: Text.WordWrap
        font.family: theme.fontFamily
        font.pixelSize: 11
        color: theme.brandAccent
    }

    // ── lanes ───────────────────────────────────────────────────────────
    Item {
        id: laneColumn
        anchors {
            top: rejectionText.bottom; left: parent.left; bottom: parent.bottom
            topMargin: theme.spacingUnit
            leftMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: parent.width - unassignedPane.width - theme.spacingUnit * 6

        Text {
            id: laneTitle
            text: "LANES"
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.4
            color: theme.textSecondary
        }

        ListView {
            anchors { top: laneTitle.bottom; topMargin: theme.spacingUnit
                      left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: 6
            model: LANES

            delegate: Rectangle {
                width: ListView.view.width
                height: 56
                radius: theme.radiusMedium
                color: page.assigningLane === model.laneNumber
                       ? theme.bgSurfaceAlt : theme.bgSurface
                border.width: 1
                border.color: page.assigningLane === model.laneNumber
                              ? theme.brandPrimary : theme.borderColor

                Text {
                    id: laneName
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: theme.spacingUnit * 2
                    width: 110
                    text: model.laneLabel
                    font.family: theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: theme.textPrimary
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: laneName.right
                    width: 300
                    spacing: 2
                    Text {
                        text: model.hasDevice ? "Assigned" : "Unassigned"
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 1.1
                        color: theme.textSecondary
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: model.hasDevice ? model.assignedNodeId : "—"
                        font.family: theme.fontFamily
                        font.pixelSize: 13
                        color: model.hasDevice ? theme.textPrimary
                                               : theme.statusDisconnected
                    }
                }

                RmsStatusPill {
                    id: statePill
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: actions.left
                    anchors.rightMargin: theme.spacingUnit * 2
                    width: 150
                    text: model.connection
                    tone: !model.hasDevice ? "neutral"
                        : model.online ? "live" : "offline"
                }

                Row {
                    id: actions
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: theme.spacingUnit * 2
                    spacing: theme.spacingUnit

                    Rectangle {
                        width: 96; height: 30; radius: theme.radiusSmall
                        color: theme.bgSurfaceAlt
                        border.width: 1
                        border.color: theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            text: model.hasDevice ? "CHANGE" : "ASSIGN"
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            font.letterSpacing: 1.1
                            color: theme.textPrimary
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: page.assigningLane =
                                       (page.assigningLane === model.laneNumber)
                                       ? -1 : model.laneNumber
                        }
                    }
                    Rectangle {
                        width: 76; height: 30; radius: theme.radiusSmall
                        visible: model.hasDevice
                        color: "transparent"
                        border.width: 1
                        border.color: theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            text: "CLEAR"
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            font.letterSpacing: 1.1
                            color: theme.textSecondary
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                RANGECONFIG.clearLane(model.laneNumber)
                                page.assigningLane = -1
                            }
                        }
                    }
                }
            }
        }
    }

    // ── unassigned devices ──────────────────────────────────────────────
    Rectangle {
        id: unassignedPane
        anchors {
            top: rejectionText.bottom; right: parent.right; bottom: parent.bottom
            topMargin: theme.spacingUnit
            rightMargin: theme.spacingUnit * 2
            bottomMargin: theme.spacingUnit * 2
        }
        width: 400
        radius: theme.radiusMedium
        color: theme.bgSurface
        border.width: 1
        border.color: theme.borderColor
        clip: true

        Column {
            anchors.fill: parent
            anchors.margins: theme.spacingUnit * 2
            spacing: theme.spacingUnit

            Text {
                text: "UNASSIGNED DEVICES  ·  " + UNASSIGNED.count
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.4
                color: theme.textSecondary
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: page.assigningLane > 0
                      ? "Choose a device for Lane " + page.assigningLane + "."
                      : "Devices RMS can hear that no lane claims. A discovered "
                        + "device is not a lane — pick the lane it stands on."
                font.family: theme.fontFamily
                font.pixelSize: 11
                color: page.assigningLane > 0 ? theme.brandPrimary : theme.textSecondary
            }

            ListView {
                width: parent.width
                height: parent.height - 96
                clip: true
                spacing: 6
                model: UNASSIGNED

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 62
                    radius: theme.radiusSmall
                    color: theme.bgBase
                    border.width: 1
                    border.color: theme.borderColor

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: theme.spacingUnit * 1.5
                        width: parent.width - 120
                        spacing: 2
                        Text {
                            width: parent.width
                            elide: Text.ElideMiddle
                            text: model.nodeId
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                            color: theme.textPrimary
                        }
                        Row {
                            spacing: 6
                            Rectangle {
                                width: 7; height: 7; radius: 4
                                anchors.verticalCenter: parent.verticalCenter
                                color: model.offline ? theme.statusDisconnected
                                                     : theme.statusConnected
                            }
                            Text {
                                text: model.connection
                                font.family: theme.fontFamily
                                font.pixelSize: 10
                                color: theme.textSecondary
                            }
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: model.deviceIdentity.length > 0
                                  ? model.deviceIdentity : "—"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: theme.spacingUnit * 1.5
                        width: 100; height: 30
                        radius: theme.radiusSmall
                        readonly property bool armed: page.assigningLane > 0
                        color: armed ? theme.brandPrimary : theme.bgSurfaceAlt
                        border.width: 1
                        border.color: armed ? theme.brandPrimary : theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            text: parent.armed ? "→ LANE " + page.assigningLane
                                               : "ASSIGN TO LANE"
                            font.family: theme.fontFamily
                            font.pixelSize: parent.armed ? 11 : 9
                            font.letterSpacing: 1.0
                            color: parent.armed ? theme.textOnBrand : theme.textSecondary
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (page.assigningLane > 0) {
                                    RANGECONFIG.assignNodeToLane(model.nodeId,
                                                                 page.assigningLane)
                                    page.assigningLane = -1
                                } else {
                                    page.rejection =
                                        "Choose a lane first: press ASSIGN on the lane "
                                        + "this device stands on."
                                }
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: UNASSIGNED.count === 0
                    text: "Every discovered device has a lane."
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.textSecondary
                }
            }
        }
    }
}
