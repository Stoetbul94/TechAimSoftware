import QtQuick 2.15

// CONTROL-CHANNEL STATUS. Status only - there is no command control here, and
// none may be added until a control transport is wired and qualified.
//
// The bar's whole purpose is the sentence it shows when nothing is connected.
// A panel that sat quiet and empty would read as "connected, idle"; this one
// says CONTROL CHANNEL NOT ENABLED, because an operator who believes they can
// start a range they cannot reach finds out at the worst possible moment.
Rectangle {
    id: bar

    Theme { id: theme }

    implicitHeight: 40
    radius: theme.radiusMedium
    color: theme.bgSurface
    border.width: 1
    border.color: theme.borderColor

    Row {
        anchors.left: parent.left
        anchors.leftMargin: theme.spacingUnit * 2
        anchors.right: parent.right
        anchors.rightMargin: theme.spacingUnit * 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: theme.spacingUnit * 1.5

        RmsStatusPill {
            anchors.verticalCenter: parent.verticalCenter
            text: CONTROL.transportAttached ? "CONTROL" : "CONTROL OFF"
            tone: CONTROL.tone
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: bar.width - 190
            text: CONTROL.statusLine
            font.family: theme.fontFamily
            font.pixelSize: 12
            color: theme.textSecondary
            elide: Text.ElideRight
        }
    }
}
