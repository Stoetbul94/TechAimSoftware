import QtQuick 2.15
import QtQuick.Window 2.15

// DISPLAYS — the target display.
//
// ALL TARGETS by default, one lane large on demand, previous/next, full screen
// and auto-rotation. Everything here is RMS-LOCAL presentation: choosing what
// to look at sends nothing, and no station can tell the difference.
Item {
    id: page

    Theme { id: theme }

    // Leaving the page must not leave a rotation timer running behind it.
    Component.onDestruction: DISPLAY.leaveDisplays()

    readonly property var selectedLane: DISPLAYLANES.laneByNumber(DISPLAY.selectedLane)
    property var laneRefresh: 0
    Connections {
        target: DISPLAYLANES
        function onChanged() { page.laneRefresh = page.laneRefresh + 1 }
    }
    // Re-read whenever the model or the selection moves.
    function currentLane() {
        laneRefresh    // dependency
        return DISPLAYLANES.laneByNumber(DISPLAY.selectedLane)
    }

    // ── toolbar ─────────────────────────────────────────────────────────
    Rectangle {
        id: toolbar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 1.5
        height: 46
        radius: theme.radiusMedium
        color: theme.bgSurface
        border.width: 1
        border.color: theme.borderColor

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: theme.spacingUnit * 1.5
            spacing: theme.spacingUnit

            RmsButton {
                label: "ALL TARGETS"
                primary: DISPLAY.showingAll
                onActivated: DISPLAY.showAllTargets()
            }
            RmsButton {
                label: "◀ PREVIOUS"
                enabled: DISPLAY.laneCount > 0
                onActivated: DISPLAY.previous()
            }
            RmsButton {
                label: "NEXT ▶"
                enabled: DISPLAY.laneCount > 0
                onActivated: DISPLAY.next()
            }
            RmsButton {
                label: "FULL SCREEN"
                enabled: DISPLAY.laneCount > 0
                onActivated: DISPLAY.setFullScreen(true)
            }
            RmsButton {
                label: DISPLAY.rotating ? "■ STOP ROTATE" : "▶ AUTO ROTATE"
                primary: DISPLAY.rotating
                enabled: DISPLAY.laneCount > 0
                onActivated: DISPLAY.setRotating(!DISPLAY.rotating)
            }

            // Rotation interval.
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Repeater {
                    model: [5, 10, 15, 30]
                    delegate: Rectangle {
                        width: 34; height: 24; radius: theme.radiusSmall
                        readonly property bool chosen:
                            DISPLAY.rotationIntervalMs === modelData * 1000
                        color: chosen ? Qt.rgba(0.66, 0, 0.22, 0.20) : theme.bgBase
                        border.width: 1
                        border.color: chosen ? theme.brandPrimary : theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            text: modelData + "s"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textPrimary
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: DISPLAY.setRotationIntervalMs(modelData * 1000)
                        }
                    }
                }
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: theme.spacingUnit * 1.5
            spacing: theme.spacingUnit

            // Which lanes the display walks. A filter changes what is on
            // screen; it never removes a lane from the range.
            Repeater {
                model: [
                    { id: "PARTICIPATING", label: "MATCH LANES" },
                    { id: "ALL_PHYSICAL",  label: "ALL LANES" }
                ]
                delegate: Rectangle {
                    width: 108; height: 26; radius: theme.radiusSmall
                    readonly property bool chosen: DISPLAY.laneFilter === modelData.id
                    color: chosen ? Qt.rgba(0.66, 0, 0.22, 0.20) : theme.bgBase
                    border.width: 1
                    border.color: chosen ? theme.brandPrimary : theme.borderColor
                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 0.8
                        color: theme.textPrimary
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: DISPLAY.setLaneFilterLabel(modelData.id)
                    }
                }
            }
        }
    }

    // ── lane strip ──────────────────────────────────────────────────────
    // A scrolling strip rather than a wall of buttons: thirty lanes must not
    // become thirty things to hunt through, and previous/next stay in the
    // toolbar where they are always one click away.
    Rectangle {
        id: strip
        anchors { top: toolbar.bottom; left: parent.left; right: parent.right }
        anchors.leftMargin: theme.spacingUnit * 1.5
        anchors.rightMargin: theme.spacingUnit * 1.5
        anchors.topMargin: theme.spacingUnit
        height: DISPLAY.laneCount > 0 ? 32 : 0
        color: "transparent"

        ListView {
            anchors.fill: parent
            orientation: ListView.Horizontal
            clip: true
            spacing: 4
            model: DISPLAY.laneOrderList

            delegate: Rectangle {
                width: 62; height: 26; radius: theme.radiusSmall
                readonly property bool chosen: DISPLAY.selectedLane === modelData
                                               && !DISPLAY.showingAll
                color: chosen ? theme.brandPrimary : theme.bgSurface
                border.width: 1
                border.color: chosen ? theme.brandPrimary : theme.borderColor
                Text {
                    anchors.centerIn: parent
                    text: "LANE " + modelData
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 0.8
                    color: chosen ? theme.textOnBrand : theme.textSecondary
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: DISPLAY.selectLane(modelData)
                }
            }
        }
    }

    // ── body ────────────────────────────────────────────────────────────
    Item {
        id: body
        anchors {
            top: strip.bottom; left: parent.left; right: parent.right; bottom: parent.bottom
            margins: theme.spacingUnit * 1.5
        }

        // Empty state — never a blank black surface.
        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width - 40, 460)
            spacing: theme.spacingUnit
            visible: DISPLAY.laneCount === 0
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: DISPLAY.emptyReason
                font.family: theme.fontFamily
                font.pixelSize: 14
                color: theme.textSecondary
            }
            RmsButton {
                anchors.horizontalCenter: parent.horizontalCenter
                label: "SHOW ALL PHYSICAL LANES"
                visible: DISPLAY.laneFilter === "PARTICIPATING"
                onActivated: DISPLAY.setLaneFilterLabel("ALL_PHYSICAL")
            }
        }

        RmsTargetGrid {
            anchors.fill: parent
            visible: DISPLAY.showingAll && DISPLAY.laneCount > 0
        }

        RmsSingleTarget {
            anchors.fill: parent
            visible: !DISPLAY.showingAll && DISPLAY.laneCount > 0
            lane: page.currentLane()
            onPreviousLane: DISPLAY.previous()
            onNextLane: DISPLAY.next()
            onAllTargets: DISPLAY.showAllTargets()
        }
    }

    // ── full screen ─────────────────────────────────────────────────────
    RmsFullScreenDisplay {
        id: fullScreen
        visible: DISPLAY.fullScreen
    }
}
