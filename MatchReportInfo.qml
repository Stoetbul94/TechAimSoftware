import QtQuick 2.0

Item {
    id: matchInfo

    property bool isMatchSummary: true
    property int seriesIndex: 0 // 0 is invalid series start with 1
    property alias itemCount: repeater.count

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    //    onVisibleChanged: {
    //        update()
    //        //console.log("Match info visible ", visible)
    //    }

    Connections {
        target: MODREADER
        onShootCountChanged: {
            // ignore on sighter mode
            if (shootingPage.sligterMode)
                return;

            update()
        }
    }

    Rectangle {
        id: topRect
        width: 0.9*parent.width
        height: 150
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        color: "transparent"
        opacity: 0.5
        visible: isMatchSummary

        Column{
            id: mainCol
            spacing: 5
            Grid{
                anchors.fill: parent
                columns : 2
                rows: 11
                columnSpacing:  20
                rowSpacing: 11
                id:shooterName


//                Row {
//                    spacing: 2
//                    Text {
//                        text:"Date and Time"
//                        font.bold: true
//                    }
//                    Text {
//                        id:event_Date
//                        text:": " + new Date().toLocaleString(Qt.locale(""), "yyyy-MM-dd hh:mm:ss")
//                        font.bold: true
//                    }
//                }

//                Rectangle {
//                    width: 2
//                    height: 5
//                    color: "transparent"
//                }

                Row {
                    spacing: 2
                    Text {
                        id:shooterLabel
                        text: qsTr("Shooter Name")
                        font.bold: true
                    }
                    Text {
                        id:name
                        text: ": "+userName
                        font.bold: true
                    }
                }

                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }

                Row {
                    spacing: 2
                    Text {
                        id:eventLabel
                        text:"Event"
                        font.bold: true
                        visible: isMatchSummary
                    }
                    Text {
                        id:event_Name
                        text:": " + eventName
                        font.bold: true
                        visible: isMatchSummary
                    }
                }

                Rectangle {
                    width: 20
                    height: 5
                    color: "transparent"
                }
                Row {
                    spacing: 2
                    Text {
                        text:"Date"
                        font.bold: true
                    }
                    Text {
                        id:event_Date
                        text:": " +new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd")
                        font.bold: true
                    }
                }

                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }
                Row {
                    spacing: 2
                    Text {
                        text:"Time"
                        font.bold: true
                    }
                    Text {
                        id:event_Time
                        text:": "+ new Date().toLocaleString(Qt.locale(""), "hh:mm:ss")
                        font.bold: true
                    }
                }

                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }

                Row {
                    spacing: 2

                    Text {
                        text: "Total Shots"//14 char
                        font.bold: true
                        visible: isMatchSummary
                    }
                    Text {
                        id:number_Shots
                        text:": " + globalModelOfData.count
                        font.bold: true
                        visible: isMatchSummary
                    }
                }

                Rectangle {
                    width: 20
                    height: 5
                    color: "transparent"
                }

                Row {
                    spacing: 2
                    Text {
                        text: qsTr("Total Score")
                        font.bold: true
                    }
                    Text {
                        id:event_score
                        text: ": " + totalScore + " ("+totalScoreWithoutDecimal+")"
                        font.bold: true
                    }
                }
                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }

//                Row {
//                    spacing: 2
//                    Text {
//                        text: qsTr("Average score")
//                        font.bold: true
//                    }
//                    Text {
//                        id:event_average
//                        text: ": " + (totalScore/globalModelOfData.count).toFixed(2)
//                        font.bold: true
//                    }
//                }

                Row {
                    spacing: 2
                    Text {
                        text:"Total Time" //14 char
                        font.bold: true
                    }
                    Text {
                        id:total_time_shot
                        text: ": "+converSecondToMins(totalTime) + " Mins"
//                        text: isSaveGame ? ": NA" : ": "+converSecondToMins(totalTime) + " Mins"
//                        text: ": " + totalTime + " Sec"/*converSecondToMins(totalTime) + " Mins"*/
                        font.bold: true
                    }
                }
//                Row {
//                    spacing: 2
//                    Text {
//                        text:"Average Time/shot"
//                        font.bold: true
//                    }
//                    Text {
//                        id:average_time_shot
//                        //                            text:": " + averageTime
//                        text: ": " + (totalTime/globalModelOfData.count).toFixed(2) + " min"
//                        font.bold: true
//                    }
//                }
                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }

                Row {
                    visible: isMatchSummary
                    spacing: 2
                    Text {
                        id: mpiLabel
                        text:"MPI"
                        font.bold: true
                    }
                    Text {
                        id: mpi
                        text:":  X: " + 0.0+" mm; Y: "+0.0 + " mm"
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }

                Row {
                    visible: isMatchSummary
                    spacing: 2
                    Text {
                        id: teilerLabel
                        text:"Teiler"
                        font.bold: true
                    }
                    Text {
                        id: teiler
                        text:": " + 0.0 //+ " mm"
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }
                Row {
                    visible: isMatchSummary
                    spacing: 2
                    Text {
                        id: startTotal
                        text:"Inner 10 Count"
                        font.bold: true
                    }
                    Text {
                        id: startTotalValue
                        text:": " + rightPanel.totalStars //+ " mm"
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }
                Row {
                    visible: isMatchSummary
                    spacing: 2
                    Text {
                        id: group
                        text:"Group"
                        font.bold: true
                    }
                    Text {
                        id: groupValue
                        text:": " + MODREADER.getGroup(-1)+qsTr(" mm1")
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                }

                Rectangle {
                    width: 2
                    height: 5
                    color: "transparent"
                }
            }
        }
    }

    Rectangle {
        id: topRect1
        width: 0.9*parent.width
        height: 60
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: "transparent"
        opacity: 0.5
        visible: !isMatchSummary

        Column{
            spacing: 5
            Grid{
                anchors.fill: parent
                columns : 2
                rows: isMatchSummary ? 3 : 3
                columnSpacing:  20
                rowSpacing: 10
                id:seriesName
                //                Text {
                //                    text:"Date and Time"
                //                    font.bold: true
                //                }
                //                Text {
                //                    id:event_Date
                //                    text:": " + new Date().toLocaleString()
                //                    font.bold: true
                //                }

                Row {
                    spacing: 2
                    Text {
                        id:seriesLabel
                        text: qsTr("SERIES")
                        font.bold: true
                    }
                    Text {
                        id:srName
                        text: ": "+seriesIndex+ " Group: "+MODREADER.getGroup(seriesIndex-1) + "MPI: "+MODREADER.getXMPI(seriesIndex)+","+MODREADER.getYMPI(seriesIndex)
                        font.bold: true
                    }
                }

                Rectangle {
                    width: 20
                    height: 5
                    color: "transparent"
                }

                Row {
                    spacing: 2
                    Text {
                        text: qsTr("Total Score")
                        font.bold: true
                    }
                    Text {
                        id: score
                        property string textInReload: ": " + getSeriesTotal(seriesIndex) + " ("+getSeriesTotalNonDecimal(seriesIndex)+")"
                        property string defaulTExt: ": " + getSeriesTotal(seriesIndex) + " ("+getSeriesTotalNonDecimal(seriesIndex)+")"//   \t  Total Time: "+converSecondToMins(getTotalTimeOfSeries(seriesIndex))+" Mins"
                        text: /*isSaveGame ? textInReload : */defaulTExt
                        font.bold: true
                    }
                }
//                Row {
//                    spacing: 2
//                    Text {
//                        text: qsTr("Avg score")
//                        font.bold: true
//                        visible: false
//                    }
//                    Text {
//                        id:eventaverage
//                        text: ": "+ getAverageScore(seriesIndex)
//                        font.bold: true
//                        visible: false
//                    }
//                }
                Rectangle {
                    width: 20
                    height: 5
                    color: "transparent"
                }
//                Row {
//                    spacing: 2
//                    Text {
//                        text:qsTr("Total Time") //14 char
//                        font.bold: true
//                    }
//                    Text {
//                        id:totaltimeshot
//                        text: !isMatchSummary ? ": " + converSecondToMins(getTotalTimeOfSeries(seriesIndex)) + " Mins" : 0
////                        text: !isMatchSummary ? ": " + /*converSecondToMins(*/getTotalTimeOfSeries(seriesIndex)/*)*/ + " Sec" : 0
//                        font.bold: true
//                    }
//                }
//                Row {
//                    spacing: 2
//                    Text {
//                        text: qsTr("Avg Time/shot")
//                        font.bold: true
//                        visible: false
//                    }
//                    Text {
//                        id:averagetimeshot
//                        text: ": " + /*converSecondToMins(*/getAverageTime(seriesIndex)/*)*/ + " Sec"
//                        font.bold: true
//                        visible: false
//                    }
//                }
//                Rectangle {
//                    width: 20
//                    height: 5
//                    color: "transparent"
//                }
            }

        }

        function refreshSeriesData() {
            srName.text = ": "+seriesIndex+ "    Group: "+MODREADER.getGroup(seriesIndex-1) + " mm    MPI: "+MODREADER.getXMPI(seriesIndex).toFixed(2)+", "+MODREADER.getYMPI(seriesIndex).toFixed(2)
        }
    }

    Column {
        anchors.top: topRect1.bottom
        anchors.topMargin: 5
        anchors.left: topRect1.left

        spacing: 5

        Row {
            visible: !isMatchSummary
            spacing: 20
            Text {
                id: serialNumber
                text: qsTr("Sr No.")
                font.bold: true
                width: 25
            }
            Text {
                id: scoreText
                text: qsTr("Score")
                font.bold: true
                width:30
            }
            Text {
                id: mpiX
                text: qsTr("    X\n(mm)")
                 anchors.leftMargin: text.horizontalCenter
                font.bold: true
                width: 30
            }
            Text {
                id: mpiY
                text: qsTr("    Y\n(mm)")
                anchors.leftMargin: text.horizontalCenter
                font.bold: true
                width:30
            }
            Text {
                id: teilerText
                text: qsTr("Teiler")
                font.bold: true
               // font.pointSize:5
                width: 30
            }
            Text {
                id: timeStamp
                text: qsTr("      Time")

                font.bold: true
                width:40
//                visible: !isSaveGame
            }
        }


        Repeater {
            id: repeater
            height: 300
            visible: !isMatchSummary
            model: !isMatchSummary ? (seriesIndex*10 < globalModelOfData.count ? 10 : globalModelOfData.count - ((seriesIndex-1)*10)) : 0
            delegate: seriesItem
        }
    }


    function update()
    {
        mpi.text = ":  X: " + MODREADER.getXMPI(seriesIndex).toFixed(1)+" mm; Y: "+MODREADER.getYMPI(seriesIndex).toFixed(1)+" mm"
        teiler.text = ": "+MODREADER.getTeiler(seriesIndex).toFixed(1)//+" mm"

        //var org_palletSize = gameRange == 10 ? 4.5 : 5.6
        var group_distance_text = MODREADER.getGroup(-1) //+ org_palletSize
        groupValue.text = qsTr(": ") + group_distance_text.toFixed(2) + qsTr(" mm")

        topRect1.refreshSeriesData()
    }

    function getSeriesTotalText()
    {
        var formatText = ""
        if(globalModelOfData.count === 0)
            return formatText
        var seriesScore = 0;
        for(var i=0; i<globalModelOfData.count; i++)
        {
            var scoreatIndex = globalModelOfData.get(i).calculatedscore*1
            seriesScore = seriesScore*1  +  (scoreatIndex.toFixed(1))*1
            //            console.log("Total score and score at current Index is",seriesScore,scoreatIndex)
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
                seriesScoreText += "(" + (seriesScore*1).toFixed(1) +"), "
                formatText = formatText + seriesScoreText
                seriesScore = 0
            }
        }
        return formatText
    }

    function getSeriesTotal(seriesIndex)
    {
        if(globalModelOfData.count === 0)
            return 0
        var seriesScore = 0
        for(var i=(seriesIndex-1)*10; i<globalModelOfData.count; i++)
        {
            if (i >=seriesIndex*10)
                break;

            var scoreatIndex = globalModelOfData.get(i).calculatedscore*1
            seriesScore = seriesScore*1  +  (scoreatIndex.toFixed(1))*1
            //            console.log("Total score and score at current Index is",seriesScore,scoreatIndex)

        }
        return seriesScore.toFixed(2)
    }

    function getSeriesTotalNonDecimal(seriesIndex)
    {
        if(globalModelOfData.count === 0)
            return 0
        var seriesScore = 0
        for(var i=(seriesIndex-1)*10; i<globalModelOfData.count; i++)
        {
            if (i >=seriesIndex*10)
                break;

            var scoreatIndex = Math.floor(globalModelOfData.get(i).calculatedscore*1)
            seriesScore = seriesScore*1  +  (scoreatIndex)*1
        }
        return seriesScore
    }

    function getAverageScore(seriesIndex)
    {
        var shootCount  = globalModelOfData.count
        var shootsInCurrentSeries = shootCount - (seriesIndex-1)*10

        if (shootsInCurrentSeries >= 10)
            return ((getSeriesTotal(seriesIndex))/10).toFixed(2)

        else if (shootsInCurrentSeries > 0 )
            return ((getSeriesTotal(seriesIndex))/shootsInCurrentSeries).toFixed(2)

        return ((getSeriesTotal(seriesIndex))/1).toFixed(2)
    }

    function getTotalTimeOfSeries(seriesIndex)
    {
        if(globalMatchModel.count === 0 || seriesIndex < 1)
            return 0
        var seriesTime = 0
        for(var i=(seriesIndex-1)*10; i<globalMatchModel.count; i++)
        {
            if (i >=seriesIndex*10)
                break;

            console.log("getTotalTimeOfSeries ------------", seriesIndex)
            var timeAtIndex = globalMatchModel.get(i).timeComsumed*1
            seriesTime = seriesTime*1  +  (timeAtIndex.toFixed(1))*1
        }

        return seriesTime.toFixed(2)
    }

    function getAverageTime(seriesIndex)
    {
        var shootCount  = globalMatchModel.count
        var shootsInCurrentSeries = shootCount - (seriesIndex-1)*10

        if (shootsInCurrentSeries >= 10)
            return ((getTotalTimeOfSeries(seriesIndex))/10).toFixed(2)

        else if (shootsInCurrentSeries > 0 )
            return ((getTotalTimeOfSeries(seriesIndex))/shootsInCurrentSeries).toFixed(2)

        return ((getTotalTimeOfSeries(seriesIndex))).toFixed(2)
    }

    function getTimeStamp(shootIndex)
    {
        var listIndex = ((seriesIndex-1)*10)+shootIndex
        return globalMatchModel.get(listIndex).timestamp
    }

    function getScoreOfShoot(shootIndex)
    {
        var realShootIndex = ((seriesIndex-1)*10) + shootIndex
        if (globalMatchModel.count > realShootIndex && seriesIndex >= 1)
            return (globalModelOfData.get(realShootIndex).calculatedscore*1).toFixed(1)

        return 0
    }

    function getDirectionOfShoot(shootIndex)
    {
        var realShootIndex = ((seriesIndex-1)*10) + shootIndex
        if (globalMatchModel.count > realShootIndex && seriesIndex >= 1)
            return (globalMatchModel.get(realShootIndex).direction*1)

        return 0
    }

    function converSecondToMins(seconds)
    {
        var minutes = Math.floor(seconds / 60);
        var secd = seconds % 60;
        if(secd < 10)
            return minutes+":0"+secd
        //var result_seconds = Math.ceil(secd);

        return minutes+":"+secd

//        return (seconds/60).toFixed(2)
    }

    Component {
        id: seriesItem

        Row {
            visible: !isMatchSummary
            height: 16
            spacing: 20
            Text {
                id: serialNumber
                text: index+1
                width: 25
            }

            Rectangle {
                width: 30
                height: parent.height
                color: "transparent"
                opacity: 0.5
                Text {
                    id: scoreText
                    text: getScoreOfShoot(index)
             
                    width: implicitWidth + 5
                    anchors.left: parent.left
                }
                Image {
                    id: arrowImage
                    height: 10 //parent.height - 4
                    width: 10
                    source: "qrc:/images/rightPanel/up_arrow.png"
                    anchors.top: parent.top
                    anchors.left: scoreText.right
                    Component.onCompleted:
                    {
                        rotation = getDirectionOfShoot(index)
                    }
                }
            }

            Text {
                id: mpiX
                text: (MODREADER.getXMPIForShoot(seriesIndex, index)*1).toFixed(2) //+ " mm"

                width:30
            }
            Text {
                id: mpiY
                text: (MODREADER.getYMPIForShoot(seriesIndex, index)*1).toFixed(2) //+ " mm"

                width:30

            }
            Text {
                id: teilerText
                text: " "+(MODREADER.getTeilerForShoot(seriesIndex, index)*1).toFixed(2) //+ " mm"

                width:35
            }
            Text {
                id: timeStamp
                text: "  "+getTimeStamp(index)
                anchors.leftMargin: text.horizontalCenter

                width: 40
//                visible: !isSaveGame
            }
        }
    }
}
