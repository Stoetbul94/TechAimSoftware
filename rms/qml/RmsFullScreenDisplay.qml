import QtQuick 2.15
import QtQuick.Window 2.15

// FULL SCREEN presentation, for a range-side monitor, projector or TV.
//
// Both modes are available full screen: one target large, or the whole range.
// The operator is never trapped — ESC exits, and a visible EXIT control is
// always on screen.
//
// SECOND DISPLAY: when Windows reports more than one screen, the window can be
// sent to another before it goes full screen. With one screen the picker is
// hidden and the window simply fills it.
Window {
    id: fullWindow
    objectName: "rmsFullScreenWindow"

    Theme { id: theme }

    width: 1280
    height: 800
    color: theme.bgBase
    title: "Tech Aim RMS — Target Display"
    flags: Qt.Window

    property int screenIndex: 0
    property var laneRefresh: 0
    Connections {
        target: DISPLAYLANES
        function onChanged() { fullWindow.laneRefresh = fullWindow.laneRefresh + 1 }
    }
    function currentLane() {
        laneRefresh
        return DISPLAYLANES.laneByNumber(DISPLAY.selectedLane)
    }

    // `visibility` is set here rather than bound, because binding it and
    // `visible` at the same time makes them fight each other.
    onVisibleChanged: {
        if (visible) {
            visibility = Window.FullScreen
            contentRoot.forceActiveFocus()
        }
    }

    Item {
        id: contentRoot
        anchors.fill: parent
        focus: true

        // ESC always leaves. A display an operator cannot get out of is worse
        // than no display.
        Keys.onEscapePressed: DISPLAY.setFullScreen(false)
        Keys.onLeftPressed: DISPLAY.previous()
        Keys.onRightPressed: DISPLAY.next()
        Keys.onSpacePressed: DISPLAY.setRotating(!DISPLAY.rotating)

        RmsTargetGrid {
            anchors.fill: parent
            anchors.margins: theme.spacingUnit * 2
            anchors.topMargin: bar.height + theme.spacingUnit
            visible: DISPLAY.showingAll
        }

        RmsSingleTarget {
            anchors.fill: parent
            anchors.topMargin: bar.height
            visible: !DISPLAY.showingAll
            // The bar below already carries navigation; two rows of the same
            // buttons would be clutter on a projector.
            showNavigation: false
            lane: fullWindow.currentLane()
        }

        // ── control bar ─────────────────────────────────────────────────
        Rectangle {
            id: bar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 52
            color: Qt.rgba(0, 0, 0, 0.55)

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: theme.spacingUnit * 2
                spacing: theme.spacingUnit

                RmsButton {
                    label: "ALL TARGETS"
                    primary: DISPLAY.showingAll
                    onActivated: DISPLAY.showAllTargets()
                }
                RmsButton { label: "◀ PREVIOUS"; onActivated: DISPLAY.previous() }
                RmsButton { label: "NEXT ▶"; onActivated: DISPLAY.next() }
                RmsButton {
                    label: DISPLAY.rotating ? "■ STOP ROTATE" : "▶ AUTO ROTATE"
                    primary: DISPLAY.rotating
                    onActivated: DISPLAY.setRotating(!DISPLAY.rotating)
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: DISPLAY.rotating
                          ? "rotating every " + Math.round(DISPLAY.rotationIntervalMs / 1000) + "s"
                          : ""
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.textSecondary
                }
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: theme.spacingUnit * 2
                spacing: theme.spacingUnit

                // Only offered when there is somewhere else to send it.
                Row {
                    visible: Qt.application.screens.length > 1
                    spacing: 4
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "DISPLAY OUTPUT"
                        font.family: theme.fontFamily
                        font.pixelSize: 9
                        font.letterSpacing: 1.0
                        color: theme.textSecondary
                    }
                    Repeater {
                        model: Qt.application.screens
                        delegate: Rectangle {
                            width: 92; height: 26; radius: theme.radiusSmall
                            readonly property bool chosen: fullWindow.screenIndex === index
                            color: chosen ? Qt.rgba(0.66, 0, 0.22, 0.25) : "transparent"
                            border.width: 1
                            border.color: chosen ? theme.brandPrimary : theme.borderColor
                            Text {
                                anchors.centerIn: parent
                                elide: Text.ElideRight
                                width: parent.width - 8
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData.name.length > 0
                                      ? modelData.name : ("Screen " + (index + 1))
                                font.family: theme.fontFamily
                                font.pixelSize: 9
                                color: theme.textPrimary
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    fullWindow.screenIndex = index
                                    // Leave full screen to move, then restore
                                    // it: a window cannot change screen while
                                    // it is filling one.
                                    fullWindow.visibility = Window.Windowed
                                    fullWindow.screen = Qt.application.screens[index]
                                    fullWindow.visibility = Window.FullScreen
                                    contentRoot.forceActiveFocus()
                                }
                            }
                        }
                    }
                }

                RmsButton {
                    label: "EXIT FULL SCREEN  (ESC)"
                    onActivated: DISPLAY.setFullScreen(false)
                }
            }
        }
    }
}
