import QtQuick 2.15

// One diagnostic fact: a label on the left, its value on the right.
// `alert` colours the value when the number is one an operator should act on —
// unseen shots, offline stations, protocol rejects.
Item {
    id: kv

    property string label: ""
    property string value: ""
    property bool alert: false

    Theme { id: theme }

    width: parent ? parent.width : 400
    height: 22

    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        width: 250
        elide: Text.ElideRight
        text: kv.label
        font.family: theme.fontFamily
        font.pixelSize: 11
        color: theme.textSecondary
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 258
        anchors.right: parent.right
        elide: Text.ElideRight
        text: kv.value
        font.family: theme.fontFamily
        font.pixelSize: 11
        font.weight: kv.alert ? Font.DemiBold : Font.Normal
        color: kv.alert ? theme.brandAccent : theme.textPrimary
    }
}
