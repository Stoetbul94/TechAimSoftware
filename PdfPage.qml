import QtQuick 2.0

// A4 host for the Match Report cover page. The cover content (ReportHeader,
// Executive Summary, Overall Target, ReportFooter) is supplied via sourceComp
// and already carries its own branding, so this page is just a clean white
// sheet with a subtle demo watermark in non-live builds.
Item {
    id: rootItem
    width: 595 // A4 size for 72 dpi
    height: 842 // A4 sixe for 72 dpi

    property int pageIndex: 0
    property string title: qsTr("Page ") + pageIndex
    property var sourceComp: null

    Rectangle {
        anchors.fill: parent
        color: "white"

        Loader {
            anchors.fill: parent
            sourceComponent: sourceComp
        }

        // Faint diagonal watermark, live builds (appMode) hide it.
        Text {
            anchors.centerIn: parent
            text: "DEMO"
            visible: !appMode
            rotation: -30
            color: "#14bf1919"
            font.pixelSize: 160
            font.bold: true
            font.family: "Segoe UI"
        }
    }
}
