import QtQuick 2.15

// HOME — what the range is, and what of it is awake.
//
// The two numbers that matter are side by side on purpose: PHYSICAL LANES is
// the range, ONLINE is today. A quiet morning shows 10 and 2, and neither
// number is wrong.
Item {
    id: page

    signal navigate(string target)

    Theme { id: theme }

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 3
        spacing: theme.spacingUnit * 3

        Column {
            spacing: 4
            Text {
                text: RANGECONFIG.rangeName.length > 0
                      ? RANGECONFIG.rangeName : "No range configured"
                font.family: theme.fontFamily
                font.pixelSize: 26
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            Text {
                text: RANGECONFIG.configured
                      ? (RANGECONFIG.rangeType.length > 0
                            ? RANGECONFIG.rangeType + "  ·  " : "")
                        + RANGECONFIG.rangeMode + " range"
                        + "  ·  lanes " + RANGECONFIG.firstLaneNumber
                        + "–" + RANGECONFIG.lastLaneNumber
                      : "Create a range in Range Setup."
                font.family: theme.fontFamily
                font.pixelSize: 13
                color: theme.textSecondary
            }
        }

        Flow {
            width: parent.width
            spacing: theme.spacingUnit * 1.5

            RmsStatTile {
                label: "PHYSICAL LANES"
                value: String(RANGECONFIG.laneCount)
                note: "configured, online or not"
            }
            RmsStatTile {
                label: "ONLINE"
                value: String(LANES.onlineCount)
                tone: "live"
                note: "lanes answering now"
            }
            RmsStatTile {
                label: "OFFLINE"
                value: String(LANES.offlineCount)
                tone: LANES.offlineCount > 0 ? "warn" : "neutral"
                note: "including unassigned lanes"
            }
            RmsStatTile {
                label: "UNASSIGNED DEVICES"
                value: String(UNASSIGNED.count)
                tone: UNASSIGNED.count > 0 ? "warn" : "neutral"
                note: UNASSIGNED.count > 0 ? "waiting for a lane" : "none waiting"
            }
            RmsStatTile {
                label: "ACTIVE SESSIONS"
                value: String(LANES.activeSessionCount)
                tone: "live"
                note: "lanes past preparation"
            }
        }

        Rectangle { width: parent.width; height: 1; color: theme.borderColor }

        Text {
            text: "QUICK ACTIONS"
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            color: theme.textSecondary
        }

        Row {
            spacing: theme.spacingUnit * 1.5

            Repeater {
                model: [
                    { page: "live",     title: "LIVE RANGE",
                      note: "Watch every lane",           enabled: true },
                    { page: "setup",    title: "RANGE SETUP",
                      note: "Lanes and device assignment", enabled: true },
                    // NEW MATCH implies control of a target. RMS has none, so
                    // it is rendered plainly unavailable rather than offered
                    // and then refused.
                    { page: "newmatch", title: "NEW MATCH",
                      note: "Central match control — a later milestone", enabled: false }
                ]
                delegate: Rectangle {
                    width: 236
                    height: 74
                    radius: theme.radiusMedium
                    color: modelData.enabled ? theme.bgSurface : theme.bgBase
                    border.width: 1
                    border.color: modelData.enabled ? theme.borderColor
                                                    : Qt.rgba(1, 1, 1, 0.06)
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: theme.spacingUnit * 2
                        anchors.right: parent.right
                        anchors.rightMargin: theme.spacingUnit
                        spacing: 4
                        Text {
                            text: modelData.title
                            font.family: theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: modelData.enabled ? theme.textPrimary
                                                     : theme.statusDisconnected
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: modelData.note
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: modelData.enabled
                        cursorShape: modelData.enabled ? Qt.PointingHandCursor
                                                       : Qt.ArrowCursor
                        onClicked: page.navigate(modelData.page)
                    }
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Range configuration: " + RANGECONFIG.configPath
            font.family: theme.fontFamily
            font.pixelSize: 10
            color: theme.textSecondary
        }
    }
}
