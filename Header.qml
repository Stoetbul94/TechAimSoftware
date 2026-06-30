import QtQuick 2.2

Item {
    property int rootItemWidth: 1366
    property int rootItemHeight: 45
    signal close()
    signal minimize()

    Rectangle {
        id: fullRect
        color: theme.bgBase
        anchors.fill: parent

        Rectangle {
            // subtle separation from the content below, instead of a
            // background texture image tied to the old brand
            height: 1
            color: theme.borderColor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }

    // Brand lockup: logo + product descriptor, left-aligned with a sensible
    // margin. Height-driven sizing with PreserveAspectFit means this scales
    // correctly with the actual header height rather than relying on a
    // fixed reference resolution.
    Row {
        id: brandRow
        anchors.left: parent.left
        anchors.leftMargin: theme.spacingUnit * 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: theme.spacingUnit

        Image {
            id: logo
            source: theme.logoWhite
            anchors.verticalCenter: parent.verticalCenter
            height: parent.parent.height * 0.6
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "\u2022"
            color: theme.textSecondary
            font.family: theme.fontFamily
            font.pixelSize: 14
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("ELECTRONIC TARGET")
            color: theme.textSecondary
            font.family: theme.fontFamily
            font.pixelSize: 13
            font.letterSpacing: 1
        }
    }

    // Window controls, right-aligned. Drawn rather than sourced from the old
    // icon assets (small, boxy, styled for the previous brand - not
    // consistent with this look) so hover feedback and color are fully
    // theme-driven and don't depend on matching an external image's style.
    Row {
        anchors.right: parent.right
        anchors.rightMargin: theme.spacingUnit
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Rectangle {
            id: minimizeButton
            width: 46
            height: parent.parent.height
            color: minimizeArea.containsMouse ? theme.bgSurfaceAlt : "transparent"

            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 1
                color: theme.textPrimary
            }

            MouseArea {
                id: minimizeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: minimize()
            }
        }

        Rectangle {
            id: closeButton
            width: 46
            height: parent.parent.height
            color: closeArea.containsMouse ? theme.brandAccent : "transparent"

            Text {
                anchors.centerIn: parent
                text: "\u2715"
                color: theme.textPrimary
                font.pixelSize: 12
            }

            MouseArea {
                id: closeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: close()
            }
        }
    }
}
