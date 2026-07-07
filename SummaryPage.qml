import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

Dialog {
    id:screenPresence
    title: "Match Summary"
    header: null
    footer: null

    property double screenContentWidth: 0
    property double screenContentHeight: 0

    property int fontSize: 12
    property int tick: 0   // bumped in update() to refresh MPI/Group metric cards

    Connections {
        target: MODREADER
        onShootCountChanged: {
            // ignore on sighter mode
            if (shootingPage.sligterMode)
                return;

            update()
        }
    }

    contentItem:Rectangle {
        id:contentRect
        anchors.fill: parent
        color: "#dcdad3"                     // grey backdrop; the A4 page sits on top

        // Scrollable print-preview area (same pattern as the Coach Print view).
        Flickable {
            id: flick
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: buttonBar.top
            clip: true
            contentWidth: width
            contentHeight: pageWrap.height
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

            Item {
                id: pageWrap
                width: flick.width
                height: print_region.height + 40

                // Fixed portrait A4-ratio page so the grab (and the PDF) are
                // portrait and undistorted.
                Rectangle {
                    id: print_region
                    width: 794           // A4 width @ 96 dpi
                    height: 1123         // A4 height @ 96 dpi
                    x: (pageWrap.width - width) / 2
                    y: 20
                    color: "white"
                    border.color: "#e6e8ec"
                    border.width: 1

                    Column {
                        id: coverCol
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 34
                        spacing: 16

                        ReportHeader {
                            width: parent.width
                            reportTitle: "Match Summary"
                            athlete: userName
                            discipline: eventName
                            dateText: new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd")
                            timeText: new Date().toLocaleString(Qt.locale(""), "hh:mm:ss")
                        }

                        SectionTitle { width: parent.width; title: "Executive Summary" }

                        Grid {
                            id: metricGrid
                            width: parent.width
                            columns: 3
                            columnSpacing: 12
                            rowSpacing: 12
                            property real cw: (width - 2 * columnSpacing) / 3

                            MetricCard { width: metricGrid.cw; label: "Total Score"; value: "" + totalScore; unit: "(" + totalScoreWithoutDecimal + ")" }
                            MetricCard { width: metricGrid.cw; label: "Average Shot"; value: globalModelOfData.count > 0 ? (totalScore / globalModelOfData.count).toFixed(2) : "—" }
                            MetricCard { width: metricGrid.cw; label: "Total Shots"; value: "" + globalModelOfData.count }
                            MetricCard { width: metricGrid.cw; label: "Inner 10"; value: "" + rightPanel.totalStars }
                            MetricCard { width: metricGrid.cw; label: "MPI"; unit: "mm"; valueSize: 18
                                value: (screenPresence.tick, MODREADER.getXMPI().toFixed(1) + " / " + MODREADER.getYMPI().toFixed(1)) }
                            MetricCard { width: metricGrid.cw; label: "Group"; unit: "mm²"; valueSize: 20
                                value: (screenPresence.tick, "" + MODREADER.getGroup(-1).toFixed(1)) }
                            MetricCard { width: metricGrid.cw; label: "Avg Time / Shot"; unit: "min"; valueSize: 20
                                value: globalModelOfData.count > 0 ? converSecondToMins(totalTime / globalModelOfData.count) : "—" }
                        }

                        SectionTitle { width: parent.width; title: "Overall Target" }

                        // Overall target — existing visualisation, coordinate math kept
                        // verbatim, reframed in a clean white card.
                        Rectangle {
                            width: parent.width
                            height: 540
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
                                width: Math.min(parent.width * 0.82, parent.height * 0.9)
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
                        anchors.leftMargin: 34
                        anchors.rightMargin: 34
                        anchors.bottomMargin: 20
                        softwareVersion: "Seta 4.0"
                        generatedText: "Generated " + new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
                    }
                }
            }
        }

        Rectangle {
            id: buttonBar
            width:parent.width
            height:parent.height*0.1
            anchors.bottom: contentRect.bottom
            color: "#2a2a2e"

            Rectangle {
                id: dummyRectCenter
                width: 5
                height: parent.height
                color: "transparent"
                anchors.centerIn: parent
            }

            Button {
                text:"Close"
                anchors.left: dummyRectCenter.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked:
                {
                    close()
                }
            }

            Button {
                text:"Save PDF"
                anchors.right: dummyRectCenter.left
                anchors.verticalCenter: parent.verticalCenter
                onClicked:
                {
                    printImage()
                }
            }

            // Run the offline Coach Report on the just-finished match and open
            // the report overlay. gameSubMode/coachReportVisible resolve via the
            // main.qml context (same pattern as gameRange/theme).
            Button {
                text: "Coach Report"
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                onClicked:
                {
                    COACHFEED.analyzeCurrentMatch(loginPage.gameSubMode)
                    coachReportVisible = true
                    close()
                }
            }
        }
    }

    function updateModel()
    {
        numberRepeater.model = null
        numberRepeater.model =globalModelOfData
    }

//    function printImage()
//    {
//        var stat = print_region.grabToImage(function(result) {
//            CUSTOMPRINT.print(result.image); //result.image holds the QVariant
//        });
//    }

    function getSeriesTotalText()
    {
        var formatText = ""
        if(globalModelOfData.count === 0)
            return formatText
        var seriesScore = 0;
        for(var i=0; i<globalModelOfData.count; i++)
        {
            var scoreatIndex =globalModelOfData.get(i).calculatedscore*1
            seriesScore = seriesScore*1  +  (scoreatIndex.toFixed(1))*1
            if( ( (i+1) % 10 == 0) && (i>0) )
            {
                var seriesId = Math.floor((i+1)/10)
                var seriesText = "Series " + seriesId*1
                seriesText += "(" + (seriesScore*1).toFixed(1) +") , "
                formatText = formatText + seriesText
                seriesScore = 0
            }
            if( (i === (globalModelOfData.count-1)) && ( (i+1)%10 != 0) )
            {
                var seriesIdNum = Math.floor((i+1)/10)
                var seriesScoreText = "Series " + seriesIdNum*1
                seriesScoreText += "(" + (seriesScore*1).toFixed(1) +")"
                formatText = formatText + seriesScoreText
                seriesScore = 0
            }
        }
        return formatText
    }

    function getMPI()
    {
        return (":  X: " + getXMPI()+"; Y: "+getYMPI())
    }

    function getXMPI()
    {
        console.log("-----------------*******************__________________")
        return MODREADER.getXMPI()
    }
    function getYMPI()
    {
        console.log("-----------------*******************__________________1")
        return MODREADER.getYMPI()
    }
    function converSecondToMins(seconds)
    {
        //        var minutes = Math.floor(seconds / 60);
        //        var secd = seconds % 60;
        //        var result_seconds = Math.ceil(secd);

        //        return minutes+":"+result_seconds

        return (seconds/60).toFixed(2)
    }

    function update()
    {
        // MPI / Group are read via functions (not notifying properties), so the
        // metric cards can't auto-bind to them; bump tick to re-evaluate.
        screenPresence.tick++
    }

    function printImage()
    {
        CUSTOMPRINT.clearImagesList()
        var stat = print_region.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath());
        CUSTOMPRINT.createSummryPdf();
    }
}
