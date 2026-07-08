import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id:screenPresence
    title: "Match Report"
    header: null
    footer: null
    property bool isPrintFromBackend: false
    property bool isAutoPrintOn: false

    // Series pages now carry every series (series 1 moved off the cover page),
    // two series per A4 page. Page count = ceil(numSeries / 2).
    property int repeaterModelCount: globalModelOfData.count === 0 ? 0
                                     : Math.ceil(Math.ceil(globalModelOfData.count / 10) / 2)

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

        if (visible && is3PMatch) {
            report3p.refresh()
            for (var i = 0; i < report3pSeriesRepeater.count; ++i)
                report3pSeriesRepeater.itemAt(i).refresh()
        }

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

                // 3P matches get the position-aware result sheet as page 1.
                Report3P {
                    id: report3p
                    visible: is3PMatch
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                PdfPage {
                    id: print_region
                    visible: !is3PMatch
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

                // 3P: one shot-detail page per position, matching the new
                // result-sheet design. The legacy Seta series pages remain
                // for the other disciplines only.
                Repeater {
                    id: report3pSeriesRepeater
                    model: is3PMatch ? 3 : 0
                    delegate: Report3PSeries {
                        posIndex: index
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                Repeater {
                    id: reportRepeater
                    visible: globalModelOfData.count > 10
                    model: is3PMatch ? 0 : repeaterModelCount
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

    // Cover page (A4): Report Header → Executive Summary → Overall Target → Footer.
    // Series-wise detail lives on the following pages (see reportRepeater).
    Component {
        id: tempComp
        Rectangle {
            color: "white"

            Column {
                id: coverCol
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 28
                spacing: 14

                ReportHeader {
                    width: parent.width
                    reportTitle: qsTr("Match Report")
                    athlete: userName
                    discipline: eventName
                    dateText: new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd")
                    timeText: new Date().toLocaleString(Qt.locale(""), "hh:mm:ss")
                    extraPairs: [
                        { l: "Shots", v: "" + globalModelOfData.count },
                        { l: "Score", v: totalScore + " (" + totalScoreWithoutDecimal + ")" },
                        { l: "Total time", v: screenPresence.converSecondToMins(totalTime) + " min" }
                    ]
                }

                SectionTitle { width: parent.width; title: qsTr("Executive Summary") }

                Grid {
                    id: execGrid
                    width: parent.width
                    columns: 3
                    columnSpacing: 12
                    rowSpacing: 12
                    property real cw: (width - 2 * columnSpacing) / 3

                    MetricCard { width: execGrid.cw; label: qsTr("Score");         value: "" + totalScore }
                    MetricCard { width: execGrid.cw; label: qsTr("Integer Score"); value: "" + totalScoreWithoutDecimal }
                    MetricCard { width: execGrid.cw; label: qsTr("Total Shots");   value: "" + globalModelOfData.count }
                    MetricCard { width: execGrid.cw; label: qsTr("Inner 10");      value: "" + rightPanel.totalStars }
                    MetricCard { width: execGrid.cw; label: qsTr("MPI"); unit: "mm"; valueSize: 18
                        value: MODREADER.getXMPI(-1).toFixed(1) + " / " + MODREADER.getYMPI(-1).toFixed(1) }
                    MetricCard { width: execGrid.cw; label: qsTr("Group"); unit: "mm"; valueSize: 20
                        value: MODREADER.getGroup(-1).toFixed(1) }
                    MetricCard { width: execGrid.cw; label: qsTr("Total Time"); unit: "min"; valueSize: 20
                        value: screenPresence.converSecondToMins(totalTime) }
                    MetricCard { width: execGrid.cw; label: qsTr("Teiler"); valueSize: 20
                        value: MODREADER.getTeiler(-1).toFixed(1) }
                }

                SectionTitle { width: parent.width; title: qsTr("Overall Target") }

                // Overall target — existing visualisation and coordinate math kept
                // verbatim, reframed in a clean white card.
                Rectangle {
                    width: parent.width
                    height: 300
                    radius: 12
                    color: "white"
                    border.color: "#e6e8ec"
                    border.width: 1

                    Image {
                        id: shootingcanvas
                        source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                         : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                                : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                            : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                        width: 260
                        height: width
                        opacity: 1
                        anchors.centerIn: parent

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
                }
            }

            ReportFooter {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.bottomMargin: 20
                softwareVersion: "Seta 4.0"
                generatedText: qsTr("Generated ") + new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
                pageText: qsTr("Page 1")
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

    // Same m:ss formatting the legacy Match Report used (via MatchReportInfo),
    // so the Total Time value is presented identically.
    function converSecondToMins(seconds)
    {
        var minutes = Math.floor(seconds / 60);
        var secd = seconds % 60;
        if(secd < 10)
            return minutes+":0"+secd
        return minutes+":"+secd
    }

    function printImage()
    {
        CUSTOMPRINT.clearImagesList()
        var page1 = is3PMatch ? report3p : print_region
        var pages = is3PMatch ? report3pSeriesRepeater : reportRepeater
        var stat = page1.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        for (var i=0; i < pages.count; ++i)
        {
            pages.itemAt(i).grabToImage(function(result) {
                CUSTOMPRINT.addImage(result.image);
            }, Qt.size(8917/4, 13033/4));
        }
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath());
        CUSTOMPRINT.createPdf();
    }

    function printImageInNetworkPath()
    {
        CUSTOMPRINT.clearImagesList()
        var page1 = is3PMatch ? report3p : print_region
        var pages = is3PMatch ? report3pSeriesRepeater : reportRepeater
        var stat = page1.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        for (var i=0; i < pages.count; ++i)
        {
            pages.itemAt(i).grabToImage(function(result) {
                CUSTOMPRINT.addImage(result.image);
            }, Qt.size(8917/4, 13033/4));
        }
        CUSTOMPRINT.createPdf(APPSETTINGS.getPrintPDFFilePath());
    }
}
