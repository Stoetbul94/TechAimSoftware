import QtQuick 2.15

// Tech Aim Report System — unified footer.
Item {
    id: footer
    property string softwareVersion: ""
    property string generatedText: ""
    property string pageText: ""
    property color  accent: "#a80038"
    implicitHeight: 28

    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#e6e8ec" }

    Row {
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 6
        Text { text: "TECH AIM"; color: footer.accent; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; font.letterSpacing: 0.5 }
        Text { text: footer.softwareVersion.length ? ("· " + footer.softwareVersion) : ""; color: "#9aa0aa"; font.pixelSize: 10; font.family: "Segoe UI" }
    }
    Text {
        anchors.centerIn: parent
        text: footer.generatedText
        color: "#9aa0aa"; font.pixelSize: 10; font.family: "Segoe UI"
    }
    Text {
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
        text: footer.pageText
        color: "#9aa0aa"; font.pixelSize: 10; font.family: "Segoe UI"
    }
}
