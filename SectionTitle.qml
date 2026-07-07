import QtQuick 2.15

// Tech Aim Report System — section heading with a thin rule.
Item {
    id: st
    property string title: ""
    property color  accent: "#a80038"
    implicitHeight: 26
    width: parent ? parent.width : 200

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Rectangle { width: 3; height: 15; radius: 2; color: st.accent; anchors.verticalCenter: parent.verticalCenter }
        Text { text: st.title; color: "#191b1f"; font.pixelSize: 15; font.bold: true; font.family: "Segoe UI" }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
}
