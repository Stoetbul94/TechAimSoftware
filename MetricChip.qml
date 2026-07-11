import QtQuick 2.15

// Compact "title … value" chip for the live telemetry panel.
// Presentational only — caller supplies title/value. Fixed, lightweight
// (single Rectangle + two Texts, no layouts) so many can render cheaply.
Rectangle {
    id: chip
    property string title: ""
    property string value: ""
    property color  accent: "transparent"   // optional left tick (e.g. maroon)

    radius: 6
    color: "#1e1f24"
    border.color: "#35363c"
    border.width: 1
    implicitHeight: 30
    implicitWidth: titleText.implicitWidth + valueText.implicitWidth + 26

    Rectangle {
        visible: chip.accent != "transparent"
        width: 3; radius: 2
        height: parent.height * 0.5
        color: chip.accent
        anchors.left: parent.left; anchors.leftMargin: 5
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        id: titleText
        anchors.left: parent.left; anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: chip.title
        color: "#9aa0a6"; font.family: "Segoe UI"; font.pixelSize: 11
    }
    Text {
        id: valueText
        anchors.right: parent.right; anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: chip.value
        color: "white"; font.family: "Segoe UI"; font.pixelSize: 13; font.bold: true
    }
}
