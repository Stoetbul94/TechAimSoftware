import QtQuick 2.15

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
        function onShootCountChanged(count) {
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
        // Series header (number/score/group/MPI) is now rendered by the
        // SeriesCard header strip, so this legacy block is kept only for its
        // refreshSeriesData() hook and not shown.
        visible: false

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

    // Modern shot table: light header rule, alternating rows, right-aligned
    // decimals, gentle highlight for inner-10s and low shots. Same data
    // (score + direction arrow, X/Y mm, Teiler, time) as before.
    Column {
        id: tableCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
        visible: !isMatchSummary

        // header
        Rectangle {
            width: parent.width
            height: 24
            color: "#f1f3f5"
            Row {
                anchors.fill: parent
                Text { width: parent.width*0.12; height: parent.height; text: qsTr("Sr");     color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Text { width: parent.width*0.22; height: parent.height; text: qsTr("Score");  color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                Text { width: parent.width*0.18; height: parent.height; text: qsTr("X (mm)"); color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                Text { width: parent.width*0.18; height: parent.height; text: qsTr("Y (mm)"); color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                Text { width: parent.width*0.15; height: parent.height; text: qsTr("Teiler"); color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                Text { width: parent.width*0.15; height: parent.height; text: qsTr("Time");   color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
        }

        Repeater {
            id: repeater
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
        // Guarded: ListModel.get() past count returns undefined and the
        // .timestamp access threw a TypeError for partial series. When the
        // shot (or its timing) doesn't exist yet, show an honest dash —
        // never a fabricated time.
        var listIndex = ((seriesIndex-1)*10)+shootIndex
        if (listIndex < 0 || listIndex >= globalMatchModel.count)
            return "—"
        var row = globalMatchModel.get(listIndex)
        if (!row || row.timestamp === undefined || row.timestamp === "")
            return "—"
        return row.timestamp
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

        Rectangle {
            width: tableCol.width
            height: 22
            visible: !isMatchSummary

            // shot score (numeric) drives the gentle highlighting
            property real sc: getScoreOfShoot(index) * 1
            // loginPage.gameMode, not bare gameMode: main.qml declares
            // gameRange but NOT gameMode, so the bare lookup threw a
            // ReferenceError that silently aborted this binding (see the
            // identical fix in SummaryReportView.getSeriesInnerVal).
            property bool inner: (loginPage.gameMode === 0 && sc >= rightPanel.star_limit_value_pistol)
                               || (loginPage.gameMode === 1 && sc >= rightPanel.star_limit_value_rifle)
            property bool low: sc > 0 && sc < 9.0

            color: inner ? "#fbeef2" : (index % 2 ? "#f7f8fa" : "#ffffff")

            Row {
                anchors.fill: parent

                Text {
                    width: parent.width*0.12; height: parent.height
                    text: index+1
                    color: "#8a8f98"; font.pixelSize: 11; font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }

                Item {
                    width: parent.width*0.22; height: parent.height
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4
                        Text {
                            id: scoreText
                            text: getScoreOfShoot(index)
                            font.pixelSize: 12; font.bold: true; font.family: "Segoe UI"
                            color: low ? "#bf1919" : (inner ? "#a80038" : "#191b1f")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Image {
                            id: arrowImage
                            height: 9; width: 9
                            source: "qrc:/images/rightPanel/up_arrow.png"
                            anchors.verticalCenter: parent.verticalCenter
                            Component.onCompleted: { rotation = getDirectionOfShoot(index) }
                        }
                    }
                }

                Text {
                    width: parent.width*0.18; height: parent.height
                    text: (MODREADER.getXMPIForShoot(seriesIndex, index)*1).toFixed(2)
                    color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.width*0.18; height: parent.height
                    text: (MODREADER.getYMPIForShoot(seriesIndex, index)*1).toFixed(2)
                    color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.width*0.15; height: parent.height
                    text: (MODREADER.getTeilerForShoot(seriesIndex, index)*1).toFixed(2)
                    color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.width*0.15; height: parent.height
                    text: getTimeStamp(index)
                    color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
        }
    }

    // Per-series inner-10 count, using the exact rule RightPanel applies for
    // totalStars (pistol >= 10.4, rifle >= 10.2). Display-only; no maths changed.
    function getSeriesInner(sIdx)
    {
        if (globalModelOfData.count === 0 || sIdx < 1)
            return 0
        var cnt = 0
        for (var i = (sIdx-1)*10; i < globalModelOfData.count && i < sIdx*10; ++i) {
            var s = globalModelOfData.get(i).calculatedscore * 1
            // loginPage.gameMode — see the inner-ten binding fix above.
            if ((loginPage.gameMode === 0 && s >= rightPanel.star_limit_value_pistol)
                    || (loginPage.gameMode === 1 && s >= rightPanel.star_limit_value_rifle))
                ++cnt
        }
        return cnt
    }
}
