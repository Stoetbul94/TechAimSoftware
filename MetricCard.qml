import QtQuick 2.15

// Tech Aim Report System — Executive-summary metric card.
// Large value, small label, brand accent bar, thin border, subtle shadow.
// Self-contained palette so every report renders identically. Print-safe.
Item {
    id: card
    property string label: ""
    property string value: "—"
    property string unit: ""
    property color  accent: "#a80038"   // Tech Aim maroon

    implicitWidth: 172
    implicitHeight: 84

    // subtle shadow
    Rectangle {
        anchors.fill: face
        anchors.topMargin: 3
        radius: face.radius
        color: "#14000000"
    }
    // card face
    Rectangle {
        id: face
        anchors.fill: parent
        radius: 10
        color: "#ffffff"
        border.color: "#e6e8ec"
        border.width: 1

        Rectangle {
            width: 4; radius: 2; height: parent.height * 0.46
            color: card.accent
            anchors.left: parent.left; anchors.leftMargin: 11
            anchors.verticalCenter: parent.verticalCenter
        }
        Column {
            anchors.left: parent.left; anchors.leftMargin: 24
            anchors.right: parent.right; anchors.rightMargin: 12
            anchors.top: parent.top; anchors.topMargin: 13
            spacing: 5
            Text {
                width: parent.width; elide: Text.ElideRight
                text: card.label
                color: "#6b7280"; font.pixelSize: 11; font.family: "Segoe UI"; font.letterSpacing: 0.4
            }
            Row {
                spacing: 5
                Text {
                    text: card.value
                    color: "#191b1f"; font.pixelSize: 26; font.bold: true; font.family: "Segoe UI"
                }
                Text {
                    visible: card.unit.length > 0
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                    text: card.unit
                    color: "#8a8f98"; font.pixelSize: 12; font.family: "Segoe UI"
                }
            }
        }
    }
}
