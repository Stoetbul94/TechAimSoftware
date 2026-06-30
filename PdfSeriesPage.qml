import QtQuick 2.0

Item {
    id: rootItem
    width: 595 // A4 size for 72 dpi
    height: 842 // A4 sixe for 72 dpi

    property int pageIndex: 0
    property string title: qsTr("Page ") + pageIndex
    property var sourceComp: null
    property int numberOfSeriesInAPagee: 2

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



        Rectangle {
            color: "transparent"
            anchors.top: parent.top
            anchors.topMargin: 40
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            Column{
                anchors.fill: parent
                anchors.topMargin: 20
                anchors.leftMargin: 20

                SeriesComponent {
                    seriesIndex: (rootItem.pageIndex-2) * numberOfSeriesInAPagee + 2 // as series 1 is already in first page
                    width: parent.width
                    height: 800/numberOfSeriesInAPagee
                    visible: (seriesIndex-1)*10 <= globalModelOfData.count

                    Component.onCompleted: {
                        update()
                    }
                }
                SeriesComponent {
                    seriesIndex: (rootItem.pageIndex-2) * numberOfSeriesInAPagee + 3 // as series 1 is already in first page
                    width: parent.width
                    height: 800/numberOfSeriesInAPagee
                    visible: (seriesIndex-1)*10 <= globalModelOfData.count
                    Component.onCompleted: {
                        update()
                    }
                }
            }
        }

        radius: 8
    }
}
