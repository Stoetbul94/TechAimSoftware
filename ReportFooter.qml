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
        // Small brand logo (replaces the text mark; same asset as the header).
        Image {
            source: "qrc:/images/logo/techaim_color.png"
            height: 13
            width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: footer.softwareVersion.length ? ("· " + footer.softwareVersion) : ""
            color: "#9aa0aa"; font.pixelSize: 10; font.family: "Segoe UI"
            anchors.verticalCenter: parent.verticalCenter
        }
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
