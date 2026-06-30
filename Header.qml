import QtQuick 2.2
Item {
    property int rootItemWidth:1366
    property int rootItemHeight:45
    signal close()
    signal minimize()

    Rectangle {
        id: fullRect
        color: "#202020"
        anchors.fill: parent
    }

    Image {
        id: bg
        source: "qrc:/images/header/bg.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        anchors.fill:parent
        //fillMode: Image.PreserveAspectFit
    }
    Image {
        id: minimizeButton
        source: "qrc:/images/header/minimize.png"
        x: ((parent.width/rootItemWidth)*1258)
        y: ((parent.height/rootItemHeight)*5)
        opacity: 1
        fillMode: Image.PreserveAspectFit

        MouseArea {
            anchors.fill: parent
            onClicked: minimize()
        }
    }
    Image {
        id: closeButton
        source: "qrc:/images/header/close.png"
        x: ((parent.width/rootItemWidth)*1304)
        y: ((parent.height/rootItemHeight)*5)
        opacity: 1
        fillMode: Image.PreserveAspectFit

        MouseArea {
            anchors.fill: parent
            onClicked: close()
        }
    }
    Text {
        id: heading
      //  anchors.left: parent.left
       // anchors.leftMargin: 10
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        color: "white"
        //text: isDefaultIcon ? qsTr("SHOOT ON TACHUS") : qsTr("SHOOT ON SETA")
        text: isDefaultIcon ? qsTr("Tachus Competition Software") : qsTr("SETA Targets for Winners")
        font { family: "Luxi Mono"; pixelSize: 24; capitalization: Font.AllUppercase }
    }
}
