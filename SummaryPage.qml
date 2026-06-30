import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtCharts 2.2

Dialog {
    id:screenPresence
    title: "Match Summary"

    property double contentWidth: 0
    property double contentHeight: 0

    property int fontSize: 12

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
        color: "grey"
        Rectangle
        {
            id:print_region
            width:contentWidth
            height: contentHeight*0.9
            color: "white"
            border.width: 20
            border.color: "transparent"
            //        }
            Column{
                anchors.fill: parent
                Row {
                    //                anchors.fill: parent
                    width:parent.width
                    height: parent.height*0.9
//                    anchors.fill: parent
//                    anchors.leftMargin: 20

                    Rectangle {
                        width: parent.width < parent.height ? parent.width : parent.height
                        height: width

                        Image {
                            id: shootingcanvas
                            //                        source: centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                            //                                                     : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png")
                            source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                                                                        : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                                                : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                                                        : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                            //                        anchors.fill: parent
                            width: parent.width*0.8
                            height: width
                            opacity: 1
                            anchors.centerIn: parent

                            Rectangle {
                                id: shootingMianRect
                                color: "transparent"
                                anchors.fill: parent
    //                            opacity: 0.2
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
                                    //                                               : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)

                                    width: shootingcanvas.height/gameRatio

                                    height: width
                                    Rectangle
                                    {
                                        Component.onCompleted:
                                        {
                                            var xCor = MODREADER.getXCord(index+1)
                                            var yCor = MODREADER.getYCord(index+1)

                                            //                                        var pistalWidthHeight = 155.5
                                            //                                        var rifleWidthHeight = 45.5
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

                    Rectangle {
                        width:parent.width*0.5
                        height:parent.height
                        Column{
                            anchors.fill: parent
                            anchors.leftMargin: 30
                            anchors.topMargin: 30
                            spacing: 20
                            Grid{
                                columns : 2
                                rows: 12
                                columnSpacing:  40
                                rowSpacing: 20
                                id:shooterName
                                Text {
                                    text: qsTr("Date")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:event_Date
                                    text:": " + new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    text: qsTr("Time")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:event_time
                                    text:": " + new Date().toLocaleString(Qt.locale(""), "hh:mm:ss")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:shooterLabel
                                    text: qsTr("Shooter Name")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:name
                                    text:": " + userName
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:eventLabel
                                    text: qsTr("Event")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:event_Name
                                    text:": " + eventName
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    text: qsTr("Total Shots")
                                    font.pointSize: fontSize
                                    font.bold: true
                                }
                                Text {
                                    id:number_Shots
                                    text:": " + globalModelOfData.count
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    text: qsTr("Total Score")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:event_score
                                    text: ": " + totalScore + " ("+totalScoreWithoutDecimal+")"
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    text: qsTr("Avg score")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:event_average
                                    text:": " + (totalScore/globalModelOfData.count).toFixed(2)
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    text:qsTr("Avg Time/Shot (In minutes)")
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:average_time_shot
                                    //                            text:": " + averageTime
                                    text: /*isSaveGame ? ": N/A" :*/ ": " +converSecondToMins(totalTime/globalModelOfData.count)
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
//                                Text {
//                                    id:series_wise_totalLabel
//                                    text: qsTr("Series wise total")
//                                    font.bold: true
//                                }
//                                Text {
//                                    id:series_wise_total
//                                    width: 150
//                                    text:": " + getSeriesTotalText()
//                                    font.bold: true
//                                    wrapMode: Text.WordWrap
//                                }
                                Text {
                                    id:mpi_label
                                    text:"MPI (mm)"
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id:mpi
                                    width: 150
                                    text: ":  X: " + 0.0+" mm; Y: "+0.0 + " mm"
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    font.pointSize: fontSize
                                }
//                                Text {
//                                    id:teiler_lable
//                                    text:"Teiler"
//                                    font.bold: true
//                                }
//                                Text {
//                                    id:teiler
//                                    width: 150
//                                    text: ": " + 0.0
//                                    font.bold: true
//                                    wrapMode: Text.WordWrap
//                                }
                                Text {
                                    id: startTotal
                                    text:"Inner 10 Count"
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id: startTotalValue
                                    text:": " + rightPanel.totalStars //+ " mm"
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id: group
                                    text:"Group"
                                    font.bold: true
                                    font.pointSize: fontSize
                                }
                                Text {
                                    id: groupValue
                                    text:": " + MODREADER.getGroup(-1)+qsTr(" mm2")
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    font.pointSize: fontSize
                                }
                            }
                        }
                    }
                }

            }
        }
        Rectangle {
            width:parent.width
            height:parent.height*0.1
            anchors.bottom: contentRect.bottom

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
                onClicked:
                {
                    close()
                }
            }

            Button {
                text:"Save"
                anchors.right: dummyRectCenter.left
                onClicked:
                {
                    printImage()
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
        mpi.text = ": " + MODREADER.getXMPI().toFixed(1)+"; "+MODREADER.getYMPI().toFixed(1)//+" mm"
        //teiler.text = ": "+MODREADER.getTeiler(seriesIndex).toFixed(1)//+" mm"
        //var org_palletSize = gameRange == 10 ? 4.5 : 5.6
        var group_distance_text = MODREADER.getGroup(-1) //+ org_palletSize
        groupValue.text = qsTr(": ") + group_distance_text.toFixed(2) + qsTr(" mm")
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
