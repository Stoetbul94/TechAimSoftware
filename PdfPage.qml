import QtQuick 2.0

Item {
    id: rootItem
    width: 595 // A4 size for 72 dpi
    height: 842 // A4 sixe for 72 dpi

    property int pageIndex: 0
    property string title: appMode ? qsTr("Page ") + pageIndex : qsTr("Page ") + pageIndex + qsTr(" DEMO")
    property var sourceComp: null

    onVisibleChanged: {
        console.log("Pdf page visible ", visible)
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Text {
            id: headerTitle
            width: implicitWidth
            height: implicitHeight
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.horizontalCenter: parent.horizontalCenter
            text: title
            color: "black"
        }

        Image {
            id: bg
            source: isDefaultIcon ? "qrc:/images/logo/tachus_logo.png" : "qrc:/images/logo/seta.png"
            opacity: 1
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            //        fillMode: Image.PreserveAspectFit
            width: (0.3*sourceSize.width)
            height: (0.3*sourceSize.height)
        }
        Text {
            id: demoText
            width: implicitWidth
            height: bg.height/2
            anchors.top: bg.bottom
            anchors.topMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 10
            text: "DEMO"
            color: "red"
            font.pixelSize: 0.3*bg.height
            visible: !appMode
        }

        Loader {
            sourceComponent: sourceComp
            anchors.top: parent.top
            anchors.topMargin: 40
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        radius: 8
    }
}
