import QtQuick 2.15

// One number and what it means. Used on Home and in the Live Range summary.
Rectangle {
    id: tile

    property string label: ""
    property string value: ""
    property string tone: "neutral"   // neutral | live | warn
    property string note: ""

    Theme { id: theme }

    width: 168
    height: 84
    radius: theme.radiusMedium
    color: theme.bgSurface
    border.width: 1
    border.color: theme.borderColor

    Column {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: theme.spacingUnit * 2
        anchors.right: parent.right
        anchors.rightMargin: theme.spacingUnit
        spacing: 2

        Text {
            text: tile.label
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            color: theme.textSecondary
            elide: Text.ElideRight
            width: parent.width
        }
        Text {
            text: tile.value
            font.family: theme.fontFamily
            font.pixelSize: 28
            font.weight: Font.DemiBold
            color: tile.tone === "live" ? theme.statusConnected
                 : tile.tone === "warn" ? theme.brandAccent
                                        : theme.textPrimary
        }
        Text {
            visible: tile.note.length > 0
            text: tile.note
            font.family: theme.fontFamily
            font.pixelSize: 10
            color: theme.textSecondary
            elide: Text.ElideRight
            width: parent.width
        }
    }
}
