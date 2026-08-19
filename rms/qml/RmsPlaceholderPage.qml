import QtQuick 2.15

// A page that is navigable but not built. It states what it will do and what
// it does NOT do, rather than showing controls that quietly go nowhere.
Item {
    id: page

    property string title: ""
    property string milestone: ""
    property string summary: ""
    property var notes: []

    Theme { id: theme }

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 3
        spacing: theme.spacingUnit * 2

        Text {
            text: page.title
            font.family: theme.fontFamily
            font.pixelSize: 24
            font.weight: Font.DemiBold
            font.letterSpacing: 1.4
            color: theme.textPrimary
        }

        Rectangle {
            width: parent.width
            height: label.height + theme.spacingUnit * 3
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.width: 1
            border.color: theme.borderColor
            Column {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: theme.spacingUnit * 2
                spacing: 6
                Text {
                    text: page.milestone
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.brandPrimary
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: page.summary
                    font.family: theme.fontFamily
                    font.pixelSize: 13
                    color: theme.textPrimary
                }
            }
        }

        Column {
            width: parent.width
            spacing: 6
            Repeater {
                model: page.notes
                delegate: Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: "·  " + modelData
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.textSecondary
                }
            }
        }
    }
}
