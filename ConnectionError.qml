import QtQuick 2.0

Item {
    id: errorDia

    property alias bgColor: bgRect.color

    Rectangle {
        id: bgRect
        anchors.fill: parent
        color: "transparent"
        opacity: 0.2

        MouseArea {
            anchors.fill: parent
        }
    }

    Rectangle {
        width: parent.width * 0.7
        height: parent.height * 0.3
        anchors.centerIn: parent
        color: "lightgrey"

        Text {
            id: error
            text: qsTr("Connection failure, Trying to reconnect. Please wait.")
            font.pointSize: 18
            anchors.centerIn: parent
            height: implicitHeight
            width: implicitWidth
        }
    }
}
