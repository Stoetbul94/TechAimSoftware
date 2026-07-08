import QtQuick 2.7

// One titled section card for the Coach Report page. Plain by design.
// `theme` resolves via main.qml's context (same pattern as the other pages).
Rectangle {
    id: card
    property string title: ""
    property string body: ""
    default property alias extra: extraArea.data

    color: theme.bgSurface
    radius: 8
    border.color: theme.borderColor
    border.width: 1
    implicitHeight: col.implicitHeight + 24

    Column {
        id: col
        x: 14
        y: 12
        width: parent.width - 28
        spacing: 8

        Text {
            width: parent.width
            text: card.title
            color: theme.brandPrimary
            font.bold: true
            font.pixelSize: 18
            font.family: theme.fontFamily
        }

        // Slot for extra visual content (e.g. the shot-map canvas).
        Column { id: extraArea; width: parent.width; spacing: 8 }

        Text {
            width: parent.width
            visible: card.body.length > 0
            text: card.body
            color: theme.textPrimary
            font.pixelSize: 14
            font.family: theme.fontFamily
            wrapMode: Text.WordWrap
            lineHeight: 1.25
        }
    }
}
