import QtQuick 2.15

// The production navigation structure, established now so a later milestone
// adds pages rather than redesigns the shell.
//
// A future page is NOT hidden and NOT faked. It is either navigable to a page
// that states plainly what is and is not built, or — where the name implies
// control RMS deliberately does not have — visibly disabled. NEW MATCH is the
// second kind: an operator must never be able to click something that looks
// like it starts a match.
Rectangle {
    id: rail

    property string currentPage: "home"
    signal navigate(string page)

    Theme { id: theme }

    width: 208
    color: theme.bgSurface

    Rectangle {
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        width: 1
        color: theme.borderColor
    }

    readonly property var items: [
        { page: "home",     label: "HOME",        enabled: true  },
        { page: "live",     label: "LIVE RANGE",  enabled: true  },
        { page: "newmatch", label: "NEW MATCH",   enabled: false },
        { page: "athletes", label: "ATHLETES",    enabled: true  },
        { page: "results",  label: "RESULTS",     enabled: true  },
        { page: "displays", label: "DISPLAYS",    enabled: true  },
        { page: "setup",    label: "RANGE SETUP", enabled: true  },
        { page: "settings", label: "SETTINGS",    enabled: true  }
    ]

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.topMargin: theme.spacingUnit * 2
        spacing: 2

        Repeater {
            model: rail.items
            delegate: Rectangle {
                width: rail.width
                height: 42
                color: rail.currentPage === modelData.page
                       ? theme.bgSurfaceAlt : "transparent"

                // The selected page is marked on the edge, not by a fill
                // alone — at a glance across a range office that reads faster.
                Rectangle {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                    width: 3
                    visible: rail.currentPage === modelData.page
                    color: theme.brandPrimary
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: theme.spacingUnit * 3
                    text: modelData.label
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    font.letterSpacing: 1.1
                    color: !modelData.enabled ? theme.statusDisconnected
                         : rail.currentPage === modelData.page ? theme.textPrimary
                                                               : theme.textSecondary
                }

                Text {
                    visible: !modelData.enabled
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: theme.spacingUnit * 2
                    text: "—"
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.statusDisconnected
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: modelData.enabled
                    cursorShape: modelData.enabled ? Qt.PointingHandCursor
                                                   : Qt.ArrowCursor
                    onClicked: rail.navigate(modelData.page)
                }
            }
        }
    }

    // The boundary, stated where the operator looks for actions.
    Text {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        anchors.margins: theme.spacingUnit * 2
        wrapMode: Text.WordWrap
        text: "Observation and configuration only. RMS cannot start, stop or "
              + "otherwise control a target."
        font.family: theme.fontFamily
        font.pixelSize: 10
        color: theme.textSecondary
    }
}
