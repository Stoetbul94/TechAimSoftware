import QtQuick 2.15

// FIRST RUN — describe the physical range before showing anything else.
//
// The alternative, dropping the operator straight into a list of whatever
// devices happen to be switched on, teaches them that RMS's idea of a range is
// "the tablets that are awake", which is exactly the misconception this
// milestone exists to remove.
Item {
    id: page

    signal created()

    Theme { id: theme }

    property string rangeName: ""
    property int firstLane: 1
    property int lastLane: 10
    property bool temporaryMode: false
    property string rangeType: "10 m"

    readonly property int laneCount: Math.max(0, lastLane - firstLane + 1)
    readonly property int discovered: UNASSIGNED.count

    function createRange() {
        if (temporaryMode) {
            RANGECONFIG.createTemporaryRange(
                rangeName.length > 0 ? rangeName : "Temporary range",
                rangeType, UNASSIGNED.nodeIds(), firstLane)
        } else {
            RANGECONFIG.createFixedRange(rangeName, rangeType, firstLane, lastLane)
        }
        if (RANGECONFIG.configured)
            page.created()
    }

    Rectangle {
        anchors.centerIn: parent
        width: 640
        height: Math.min(parent.height - theme.spacingUnit * 6, body.height + 64)
        color: theme.bgSurface
        border.width: 1
        border.color: theme.borderColor
        radius: theme.radiusMedium

        Column {
            id: body
            anchors { left: parent.left; right: parent.right; top: parent.top }
            anchors.margins: theme.spacingUnit * 4
            spacing: theme.spacingUnit * 2

            Text {
                text: "CREATE RANGE"
                font.family: theme.fontFamily
                font.pixelSize: 20
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                color: theme.textPrimary
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "Describe the physical shooting range. Lanes exist whether "
                      + "or not a device is switched on, so a ten-lane range stays "
                      + "a ten-lane range when only six tablets are running."
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textSecondary
            }

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            // ── name ────────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: "RANGE NAME"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Rectangle {
                    width: parent.width
                    height: 36
                    color: theme.bgBase
                    border.width: 1
                    border.color: nameInput.activeFocus ? theme.brandPrimary
                                                        : theme.borderColor
                    radius: theme.radiusSmall
                    TextInput {
                        id: nameInput
                        anchors.fill: parent
                        anchors.leftMargin: theme.spacingUnit
                        anchors.rightMargin: theme.spacingUnit
                        verticalAlignment: TextInput.AlignVCenter
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        color: theme.textPrimary
                        selectByMouse: true
                        onTextChanged: page.rangeName = text
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: nameInput.text.length === 0
                            text: "e.g. Potchefstroom 50 m"
                            font.family: theme.fontFamily
                            font.pixelSize: 14
                            color: theme.statusDisconnected
                        }
                    }
                }
            }

            // ── distance ────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: "RANGE TYPE / DISTANCE"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Row {
                    spacing: theme.spacingUnit
                    Repeater {
                        model: ["10 m", "50 m", "10 m + 50 m"]
                        delegate: Rectangle {
                            width: 110
                            height: 30
                            radius: theme.radiusSmall
                            color: page.rangeType === modelData
                                   ? Qt.rgba(0.66, 0, 0.22, 0.18) : theme.bgBase
                            border.width: 1
                            border.color: page.rangeType === modelData
                                          ? theme.brandPrimary : theme.borderColor
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.family: theme.fontFamily
                                font.pixelSize: 12
                                color: theme.textPrimary
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: page.rangeType = modelData
                            }
                        }
                    }
                }
            }

            // ── lane numbering ──────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
                visible: !page.temporaryMode
                Text {
                    text: "LANE NUMBERING"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Row {
                    spacing: theme.spacingUnit * 2
                    RmsNumberField {
                        label: "from"
                        value: page.firstLane
                        minimum: 1
                        onValueEdited: function(v) {
                            page.firstLane = v
                            if (page.lastLane < v) page.lastLane = v
                        }
                    }
                    RmsNumberField {
                        label: "to"
                        value: page.lastLane
                        minimum: page.firstLane
                        onValueEdited: function(v) { page.lastLane = v }
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        Text {
                            text: page.laneCount + " physical lanes"
                            font.family: theme.fontFamily
                            font.pixelSize: 14
                            color: theme.textPrimary
                        }
                        Text {
                            text: "created empty; assign devices in Range Setup"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // ── mode ────────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: "RANGE MODE"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Repeater {
                    model: [
                        { temp: false, title: "Fixed range",
                          note: "A saved installation. Lanes are created empty and you assign devices." },
                        { temp: true, title: "Temporary / auto-build",
                          note: "Build lanes from the devices discovered right now. For training, demos and portable setups." }
                    ]
                    // An Item, not a Row: a Row positions its children, so a
                    // child using anchors.fill (the click target) fights it and
                    // the whole option collapses onto the one above.
                    delegate: Item {
                        width: parent.width
                        height: optionText.height + 8

                        Rectangle {
                            id: dot
                            width: 14; height: 14; radius: 7
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.topMargin: 3
                            color: "transparent"
                            border.width: 1
                            border.color: page.temporaryMode === modelData.temp
                                          ? theme.brandPrimary : theme.borderColor
                            Rectangle {
                                anchors.centerIn: parent
                                width: 7; height: 7; radius: 4
                                visible: page.temporaryMode === modelData.temp
                                color: theme.brandPrimary
                            }
                        }
                        Column {
                            id: optionText
                            anchors.left: dot.right
                            anchors.leftMargin: theme.spacingUnit
                            anchors.right: parent.right
                            anchors.top: parent.top
                            spacing: 1
                            Text {
                                text: modelData.title
                                font.family: theme.fontFamily
                                font.pixelSize: 13
                                color: theme.textPrimary
                            }
                            Text {
                                width: parent.width
                                wrapMode: Text.WordWrap
                                text: modelData.temp
                                      ? modelData.note + "  ("
                                        + page.discovered + " discovered now)"
                                      : modelData.note
                                font.family: theme.fontFamily
                                font.pixelSize: 10
                                color: theme.textSecondary
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: page.temporaryMode = modelData.temp
                        }
                    }
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: RANGECONFIG.lastError.length > 0
                text: RANGECONFIG.lastError
                font.family: theme.fontFamily
                font.pixelSize: 11
                color: theme.brandAccent
            }

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            Rectangle {
                width: 190
                height: 38
                radius: theme.radiusSmall
                readonly property bool ready:
                    page.temporaryMode ? page.discovered > 0
                                       : (page.rangeName.trim().length > 0 && page.laneCount > 0)
                color: ready ? theme.brandPrimary : theme.bgSurfaceAlt
                Text {
                    anchors.centerIn: parent
                    text: "CREATE RANGE"
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    font.letterSpacing: 1.2
                    color: parent.ready ? theme.textOnBrand : theme.statusDisconnected
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: parent.ready
                    cursorShape: parent.ready ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: page.createRange()
                }
            }
        }
    }
}
