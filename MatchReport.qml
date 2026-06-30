import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2

Dialog {
    id:screenPresence
    title: "Match Report"
    property bool isPrintFromBackend: false
    property bool isAutoPrintOn: false

    property int repeaterModelCount: globalModelOfData.count <= 10 ? 0 : (globalModelOfData.count - 10)/20 + 1

    Connections{
        target: CUSTOMPRINT
        onSaveComplete :{
            console.log("Saved done")
            close()
        }
    }

    onAccepted: {
    }

    onRejected: {
    }

    Timer {
        id: printTimer
        repeat: false
        interval: isAutoPrintOn ? 2000 : 0
        onTriggered: {
            printImageInNetworkPath()
            printTimer.stop()
            isAutoPrintOn = false
        }
    }

    onIsAutoPrintOnChanged: {
        if (!isAutoPrintOn)
            printTimer.stop()
    }

    onVisibleChanged: {
        contentRect.visible = visible

        if (visible && isAutoPrintOn)
            printTimer.start() //printImageInNetworkPath()
    }

    contentItem:Rectangle {
        id:contentRect
        anchors.fill: parent
        color: "transparent"

        onVisibleChanged: {
        }
        ScrollView {
            id: scrollView
            width: parent.width
            height: parent.height - buttonRect.height
            Column {
                spacing: 20
                width: scrollView.width

                PdfPage {
                    id: print_region
                    pageIndex: 1
//                    width: parent.width
//                    height: 800
                    width: 595 // A4 size for 72 dpi
                    height: 842 // A4 sixe for 72 dpi
                    sourceComp: tempComp
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Rectangle {
                    height: 20
                    width: scrollView.width
                    color: "transparent"
                }

                Repeater {
                    id: reportRepeater
                    visible: globalModelOfData.count > 10
                    model: repeaterModelCount
                    delegate: PdfSeriesPage {
                        pageIndex: index+2
                        //width: parent.width
                        anchors.horizontalCenter: parent.horizontalCenter
                        onVisibleChanged: {
                            console.log("test 1111--------------------------------")
                        }
                    }
                }
            }
        }

        Rectangle {
            id: buttonRect
            width:parent.width
            height:parent.height*0.1
            anchors.bottom: contentRect.bottom
            Row{
                anchors.centerIn: parent
                Button {
                    text:"Save"
                    onClicked:
                    {
                        printImage()
                    }
                }
                Button {
                    text:"Cancel"
                    onClicked:
                    {
                        close()
                    }
                }
            }
        }
    }

    Component {
        id: tempComp
        Rectangle {
            color: "transparent"

            Column{
                anchors.fill: parent
                anchors.topMargin: 20
                anchors.leftMargin: 20

                Rectangle {
                    id: matchSummartTextRect
                    width: parent.width
                    height: 40
                    color: "transparent"

                    Text {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.leftMargin: 75
                        text: qsTr("Match Summary")
                        font.pixelSize: 20
                    }
                }

                Row {
                    width: parent.width > 300 ? parent.width : 500
                    height: 300
                    spacing: 30
                    Image {
                        id: shootingcanvas
//                        source: centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
//                                                     : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png")
                        source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                         : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                                : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                            : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                        width: 300 //parent.width < parent.height ? parent.width*0.45 : parent.height*0.45
                        height: width
                        opacity: 1

                        Rectangle {
                            id: shootingMianRect
                            color: "transparent"
                            anchors.fill: parent
                        }

                        Repeater
                        {
                            id:numberRepeater
                            model:globalModelOfData
                            delegate: numberDelegate
                        }

                        Component {
                            id:numberDelegate
                            Item {
                                id:mainItem
                                // 34.55 and 10.11 was given by abins (tachus)
                                //10 Meter Pistol Ratio=155.5÷4.5=34.55
                                //10 Meter Rifle Ratio=45.5÷4.5=10.11
                                //50 Meter Pistol Ratio=500/5.6=89.29
                                //50 Meter Rifle Ratio=154.4/5.6=27.57

                                property double gameRatio: gameRange == 10 ? (centerPanel.gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                                           : (centerPanel.gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())
                                //width: gameRange == 10 ? (centerPanel.gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
                                //                       : (centerPanel.gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
                                //                                   : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)
                                width: shootingcanvas.height/gameRatio
                                height: width
                                Rectangle
                                {
                                    Component.onCompleted:
                                    {
                                        var xCor = MODREADER.getXCord(index+1)
                                        var yCor = MODREADER.getYCord(index+1)

                                        var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                                        var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                                        var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                                        var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                                        var offsetX = shootingMianRect.width/shootingWidth
                                        var offsetY = shootingMianRect.height/shootingHeight

                                        var centerX = shootingMianRect.width/2 //* offset
                                        var centerY = shootingMianRect.height/2 //* offset

                                        var bulletSize = 0//4.5/2

                                        mainItem.x = shootingMianRect.x + centerX+((xCor+bulletSize)*offsetX) - radius
                                        mainItem.y = shootingMianRect.y + centerY-((yCor+bulletSize)*offsetY) - radius
                                    }
                                    anchors.fill: parent
                                    radius:parent.width/2
                                    color: greenColor//shootingPage.isPalletRed ? "red" : "white"
                                    Text{
                                        anchors.centerIn: parent
                                        text: index+1
                                        visible: false
                                    }
//                                    border.color: "red"
                                }
                            }
                        }

                    }

                    MatchReportInfo {
                        id: matchReportInfo
                        width: parent.width - shootingcanvas.width
                        height: parent.height
                    }
                }

                Rectangle {
                    id: matchReportTextRect
                    width: parent.width
                    height: 40
                    color: "transparent"

                    Text {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("SERIES WISE RESULTS")
                        font.pixelSize: 20
                    }
                }

//                Rectangle {
//                    id: emptySpaceRect
//                    width: 20
//                    height: 40
//                    color: "transparent"
//                }

                SeriesComponent {
                    seriesIndex: 1
                    width: parent.width - 50
                    height: 500
                }
            }
        }
    }

    function circleRegionCordinates()
    {
        var left = Qt.point(270,4);
        left = preview.mapToPosition(left,scaterSeries);
        var right = Qt.point(90,4);
        right = preview.mapToPosition(right,scaterSeries);
        var top = Qt.point(360,4);
        top = preview.mapToPosition(top,scaterSeries);
        var bottom = Qt.point(180,4);
        bottom = preview.mapToPosition(bottom,scaterSeries);

        circleRegion.x = left.x
        circleRegion.y = top.y
        circleRegion.width = right.x - left.x
        circleRegion.height = circleRegion.width
        circleRegion.radius = circleRegion.width/2

        updateModel()
    }

    function updateModel()
    {
        numberRepeater.model = null
        numberRepeater.model =globalModelOfData
    }

    function printImage()
    {
        CUSTOMPRINT.clearImagesList()
        var stat = print_region.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        for (var i=0; i < reportRepeater.count; ++i)
        {
            reportRepeater.itemAt(i).grabToImage(function(result) {
                CUSTOMPRINT.addImage(result.image);
            }, Qt.size(8917/4, 13033/4));
        }
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath());
        CUSTOMPRINT.createPdf();
    }

    function printImageInNetworkPath()
    {
        CUSTOMPRINT.clearImagesList()
        var stat = print_region.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        for (var i=0; i < reportRepeater.count; ++i)
        {
            reportRepeater.itemAt(i).grabToImage(function(result) {
                CUSTOMPRINT.addImage(result.image);
            }, Qt.size(8917/4, 13033/4));
        }
        CUSTOMPRINT.createPdf(APPSETTINGS.getPrintPDFFilePath());
    }
}
