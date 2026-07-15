// QtQuick 2.15 (was 2.0): Image.mipmap — used by the brand logo — needs a
// newer import; matches the other report components (ReportHeader/Footer).
import QtQuick 2.15

// A4 series-detail page. Carries two series cards. Because series 1 now lives
// here (not on the cover), the first slot starts at series 1:
//   first  = (pageIndex-2)*2 + 1
//   second = (pageIndex-2)*2 + 2
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
        color: "white"

        Column {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 24
            spacing: 14

            // Slim brand strip
            Item {
                width: parent.width
                height: 30
                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    // Brand logo (replaces the text wordmark; see ReportHeader).
                    Image {
                        source: "qrc:/images/logo/techaim_color.png"
                        height: 20
                        width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "· Match Report — Series Detail"; color: "#9aa0aa"; font.pixelSize: 12; font.family: "Segoe UI"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: userName
                    color: "#33373d"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI"
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#e6e8ec" }
            }

            SeriesComponent {
                seriesIndex: (rootItem.pageIndex - 2) * numberOfSeriesInAPagee + 1
                width: parent.width
                height: 340
                Component.onCompleted: { update() }
            }
            SeriesComponent {
                seriesIndex: (rootItem.pageIndex - 2) * numberOfSeriesInAPagee + 2
                width: parent.width
                height: 340
                Component.onCompleted: { update() }
            }
        }

        ReportFooter {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.bottomMargin: 16
            softwareVersion: "Seta 4.0"
            pageText: qsTr("Page ") + rootItem.pageIndex
        }
    }
}
