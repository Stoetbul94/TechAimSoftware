import QtQuick 2.15

// Small labelled-value pill, e.g. [X 7.4] / [Y 5.7]. Presentational only.
Rectangle {
    id: chip
    property string label: ""
    property string value: ""

    radius: 6
    color: "#1e1f24"
    border.color: "#35363c"
    border.width: 1
    implicitHeight: 26
    implicitWidth: row.implicitWidth + 20

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: chip.label
            color: "#9aa0a6"; font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: chip.value
            color: "white"; font.family: "Segoe UI"; font.pixelSize: 12
        }
    }
}
