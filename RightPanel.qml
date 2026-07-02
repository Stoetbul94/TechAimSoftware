import QtQuick 2.2
Item {
    property int rootItemWidth:1674
    property int rootItemHeight:3092
    property int currentPageIndex : 0
    property int totalStars : 0
    property int seriesStars : 0
    property int totalTimeConsume: 0
    property int seriesTimeConsume: 0
    property int currentShootIndex: -1
    property bool listNavigationON: false
    property bool isGameLoaded: false // as main.qml isSAveGame not working

    property real star_limit_value_pistol: 10.4
    property real star_limit_value_rifle: 10.2

//    property alias isPlayVisible: leftPanel.playVisible

    property int fontForSeries: 14
    property int fontForMath: 18
    property int dafaultFontSize: 19

    signal switchToSighter(bool sighterEnable)

    signal matchFinished()


    property alias firstRowX: firstRow.x
    property alias firstRowY: firstRow.y
    property alias firstRowWidth: firstRow.width
    property alias firstRowHeight: firstRow.height

//    function getFormatedScore(calculatedscore) {
//        return calculatedscore.substring(0, calculatedscore.length)
//    }

    Connections {
        target: loginPage

        onSighterStartedFromServer : {
            pauseClicked()
        }

        onMatchStartedFromServer : {
            startClicked()
        }

    }

    onCurrentPageIndexChanged:
    {
        var maxPageIndex = Math.floor((globalModelOfData.count-1)/10)
        console.log("Current Page Index and Max page Index " , currentPageIndex,maxPageIndex)
        if(currentPageIndex < maxPageIndex)
        {
            enableRightNavigation(true)
        }
        else
        {
            enableRightNavigation(false)
        }

        if(currentPageIndex > 0)
        {
            enableLeftNavigation(true)
        }
        else
        {
            enableLeftNavigation(false)
        }

        //Update centerPanel
        centerPanel.disableMotorMovement = true
        centerPanel.currentPageIndexChanged()
        centerPanel.disableMotorMovement = false

        centerPanel.refreshGroupRect()
    }

    onCurrentShootIndexChanged: {
        centerPanel.currentScoreValue = scoreCutoffTofirstDecimal(globalModelOfData.get(currentShootIndex).calculatedscore)*1
//        if (centerPanel.currentScoreValue == "nan" || centerPanel.currentScoreValue == "NaN")
//            centerPanel.currentScoreValue = "0"
        centerPanel.currentScoreDegree = globalModelOfData.get(currentShootIndex).direction*1
        if (matchScore.count != 0)
            centerPanel.refreshSelectedShootPosition()

        console.log(globalModelOfData.count , globalModelOfData.get(currentShootIndex).calculatedscore, " ***srinu ---",matchScore.count, "current shoot index changed", currentShootIndex)
        console.log(scoreCutoffTofirstDecimal(globalModelOfData.get(currentShootIndex).calculatedscore)*1, " srinu ---",matchScore.count, "current shoot index changed", currentShootIndex)
    }

    Image {
        id: layer_0
        source: "qrc:/images/rightPanel/layer_0.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: total_score_block
        source: /*isDefaultIcon ? "qrc:/images/rightPanel/total_score_block_tachus.png" :*/ "qrc:/images/rightPanel/total_score_block.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: text_field
        source: "qrc:/images/rightPanel/text_field.png"
        x: ((parent.width/rootItemWidth)*1188)
        y: ((parent.height/rootItemHeight)*2480)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "lightblue"
        }
    }

    Text {
        id: totalTime
        width: ((parent.width/rootItemWidth)*text_field.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field_593_2.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.horizontalCenter: text_field.horizontalCenter
        anchors.verticalCenter: text_field_593_2.verticalCenter
        opacity: 0
    }

    Image {
        id: text_field1
        source: "qrc:/images/rightPanel/text_field.png"
        x: ((parent.width/rootItemWidth)*823)
        y: ((parent.height/rootItemHeight)*2480)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)

        Rectangle {
            anchors.fill: parent
            color: "red"
        }
    }

    Text {
        id: grandTotalED
        width: ((parent.width/rootItemWidth)*text_field1.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field_593_2.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.horizontalCenter: text_field1.horizontalCenter
        anchors.verticalCenter: text_field_593_2.verticalCenter
        opacity: 0
    }

    Image {
        id: text_field_593_2
        source: "qrc:/images/rightPanel/text_field_593_2.png"
        x: ((parent.width/rootItemWidth)*400)
        y: ((parent.height/rootItemHeight)*2480)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "yellow"
        }
    }

    Text {
        id: grandTotalText
        width: ((parent.width/rootItemWidth)*text_field_593_2.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field_593_2.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: text_field_593_2
        opacity: 0
    }

    Image {
        id: text_field_8
        source: "qrc:/images/rightPanel/text_field_8.png"
        x: ((parent.width/rootItemWidth)*228)
        y: ((parent.height/rootItemHeight)*2480)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "green"
        }
    }

    Text {
        id: grandStarText
        width: ((parent.width/rootItemWidth)*text_field_8.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field_8.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: text_field_8
        opacity: 0
    }

    Image {
        id: text_field_1_55
        source: "qrc:/images/rightPanel/text_field_1_55.png"
        x: ((parent.width/rootItemWidth)*1188)
        y: ((parent.height/rootItemHeight)*2186)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "blue"
        }
    }

    Text {
        id: seriesTime
        width: ((parent.width/rootItemWidth)*text_field_1_55.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field_1_55.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: text_field_1_55
    }

    Image {
        id: text_field2
        source: "qrc:/images/rightPanel/text_field.png"
        x: ((parent.width/rootItemWidth)*823)
        y: ((parent.height/rootItemHeight)*2186)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "orange"
        }
    }
    Text {
        id: seriesSubTotalED
        width: ((parent.width/rootItemWidth)*text_field2.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_field2.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: text_field2

        onTextChanged: {
            MODREADER.setTotalScoreWOD(seriesSubTotalED.text)
            MODREADER.updateSetaShootSummaryData()
        }
    }
    Image {
        id: field_88_7
        source: "qrc:/images/rightPanel/field_88_7.png"
        x: ((parent.width/rootItemWidth)*400)
        y: ((parent.height/rootItemHeight)*2186)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "white"
        }
    }

    Text {
        id: seriesSubTotal
        width: ((parent.width/rootItemWidth)*field_88_7.sourceSize.width)
        height: ((parent.height/rootItemHeight)*field_88_7.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: field_88_7

        onTextChanged: {
            MODREADER.setTotalScoreWD(seriesSubTotal.text)
            MODREADER.updateSetaShootSummaryData()
        }
    }

    Image {
        id: text_filed_3
        source: "qrc:/images/rightPanel/text_filed_3.png"
        x: ((parent.width/rootItemWidth)*228)
        y: ((parent.height/rootItemHeight)*2186)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        Rectangle {
            anchors.fill: parent
            color: "pink"
        }
    }

    Text {
        id: seriesStarText
        width: ((parent.width/rootItemWidth)*text_filed_3.sourceSize.width)
        height: ((parent.height/rootItemHeight)*text_filed_3.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (0.55*height)

        text: ""
        color: "white"
        anchors.centerIn: text_filed_3
    }

    Image {
        id: restart
        source: "qrc:/images/rightPanel/stop.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*1550)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        Rectangle
        {
            anchors.fill: parent
            color:"green"
        }
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {

            }
        }
    }
    Image {
        id: stop_over
        source: "qrc:/images/rightPanel/stop_over.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*1550)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                //                stopClicked()
            }
        }
    }
//    Image {
//        id: play
//        source: "qrc:/images/rightPanel/play.png"
//        x: ((parent.width/rootItemWidth)*0)
//        y: ((parent.height/rootItemHeight)*1223)
//        opacity: 1
//        width: ((parent.width/rootItemWidth)*sourceSize.width)
//        height: ((parent.height/rootItemHeight)*sourceSize.height)
//        MouseArea
//        {
//            anchors.fill: parent
//            onClicked:
//            {
//                startClicked()
//            }
//        }
//    }
    Image {
        id: pause_over
        source: "qrc:/images/rightPanel/pause_over.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*1223)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked: {
                //                pauseClicked()
            }
        }
    }
    Image {
        id: num
        source: "qrc:/images/rightPanel/num.png"
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)*1.2
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        x: ((parent.width/rootItemWidth)*400) - width*0.1
        y: ((parent.height/rootItemHeight)*347)
    }
    Image {
        id: series_6
        source: "qrc:/images/rightPanel/series_6.png"
        anchors.left: left_arrow.right
        anchors.right: right.left
        anchors.top: left_arrow.top
        anchors.bottom: left_arrow.bottom
//        x: ((parent.width/rootItemWidth)*400)
//        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
//        width: ((parent.width/rootItemWidth)*sourceSize.width)
//        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: true
    }

    Image {
        id: series_text_field
        source: "qrc:/images/rightPanel/series_text_field.png"
        x: ((parent.width/rootItemWidth)*970)
        y: ((parent.height/rootItemHeight)*170)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: seriesText
        anchors.bottom: series_text_field.bottom
        anchors.bottomMargin: -5
        anchors.left: series_text_field.left
        //anchors.topMargin: -3
        //        anchors.horizontalCenter: series_text_field.horizontalCenter
        text : (currentPageIndex+1)
        color: "white"
        font.pixelSize: dafaultFontSize
    }
    Image {
        id: right
        source: "qrc:/images/rightPanel/right.png"
        x: ((parent.width/rootItemWidth)*1408) + num.width*0.07
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                rightClicked()
            }
        }
    }
    Image {
        id: right_end
        source: "qrc:/images/rightPanel/right_end.png"
        x: ((parent.width/rootItemWidth)*1408) + num.width*0.07
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: true
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {

            }
        }
    }
    Image {
        id: right_over
        source: "qrc:/images/rightPanel/right_over.png"
        x: ((parent.width/rootItemWidth)*1408) + num.width*0.07
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {

            }
        }
    }
    Image {
        id: left_arrow
        source: "qrc:/images/rightPanel/left_arrow.png"
        x: ((parent.width/rootItemWidth)*297) - num.width*0.1
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                leftClicked()
            }
        }
    }
    Image {
        id: left_arrow_end
        source: "qrc:/images/rightPanel/left_arrow_end.png"
        x: ((parent.width/rootItemWidth)*297) - num.width*0.1
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: true
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {

            }
        }
    }
    Image {
        id: left_arrow_over
        source: "qrc:/images/rightPanel/left_arrow_over.png"
        x: ((parent.width/rootItemWidth)*297) - num.width*0.1
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {

            }
        }
    }

    /////////////////////////////////////
    property var subTotal: 0
    property var subTotalExculdeDec: 0
    property var grandTotal: 0
    property var grandTotalExculdeDec: 0
    property bool minPreviewMode: true
    property int timeInSec: 0
    property int lastUsedTime: 0


    function resetTimer() {
        timeInSec = 0;
        lastUsedTime = 0;
        console.log("------------------------------------------timer reset------------------------------------------")
    }

    ListModel {
        id:listModel
    }
    ListModel {
        id:seriesModelScore
    }
    //    ListModel
    //    {
    //        id:globalModelOfData
    //    }

    Rectangle
    {
        id:scoresummary
        anchors.margins: 10
        height: parent.height
        width: parent.width*0.4
        border.color: "black"
        radius: 3
        visible: false
        Column
        {
            anchors.fill: parent
            Text{
                text : "Sub Total     " + subTotal +   "  " + subTotalExculdeDec
            }
            Text{
                text : "Grand Total   " + grandTotal + "  " + grandTotalExculdeDec
            }
        }
    }

    ListView{
        id:matchScore
        anchors.fill: num
        model: listModel
        delegate: matchSeriesDelegate

        onCountChanged: {
            if (count != 0)
            {
                var direction = listModel.get(count-1).direction;
                var score = listModel.get(count-1).calculatedscore
                console.log(direction, "-------------------", score)
                currentIndex = count - 1
            }
        }

        onCurrentIndexChanged: {
            console.log(count, "right list view current index", currentIndex)
            currentShootIndex = currentIndex + (currentPageIndex*10)
        }
    }

    Timer {
        id: timer
        interval: 1000
        repeat: true
        onTriggered: {
            timeInSec++;
        }
    }

    Component {
        id: matchSeriesDelegate
        Item {
            width:matchScore.width
            height: matchScore.height/10

            Rectangle {
                id: currentItem
                anchors.fill: parent
                color: "#A6CE72"
                visible: matchScore.currentIndex == index //(right_end.visible) && (index === (globalModelOfData.count-1)%10)
            }

            Rectangle {
                id: indexRect
                height: parent.height
                width: parent.width*0.1
                border.color: "grey"
                color: "transparent"

                Text {
                    text: currentPageIndex*10 + index + 1
                    anchors.centerIn: parent
                    font.pixelSize: 0.65*currentItem.height
                }
            }

            Rectangle {
                anchors.right: parent.right
                width: parent.width - indexRect.width
                height: parent.height

                border.color: "grey"
                color: "transparent"

                Image {
                    id: arrowImage
                    anchors.left: scoreTextRect.right
                    transformOrigin: Item.Center

                    height: 16 //parent.height - 4
                    width: 16
                    source: "qrc:/images/rightPanel/up_arrow.png"
                    anchors.verticalCenter: parent.verticalCenter

                    Component.onCompleted:
                    {
                        rotation = direction
                    }
                }

                Rectangle {
                    id: scoreTextRect
                    width: parent.width * 0.3
                    height: parent.height

                    anchors.left: parent.left
                    anchors.leftMargin: 10

                    color: "transparent"
                    Text {
                        id: scoreText
                        anchors.centerIn: parent

                        text:APPSETTINGS.getScoringSystem()? (scoreCutoffTofirstDecimal(calculatedscore)*1): parseInt(scoreCutoffTofirstDecimal(calculatedscore)*1)
                        font.pixelSize: 0.65*currentItem.height
                    }
                }

                Image {
                    id: starImage
                    height: 16 //parent.height - 4
                    width: 16
                    anchors.left: arrowImage.right
                    anchors.leftMargin: 10

                    visible: loginPage.gameMode == 0 ? (score >= star_limit_value_pistol) : (score >= star_limit_value_rifle)
                    source: "qrc:/images/rightPanel/star.png"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    id: timeTextRect
                    width: parent.width * 0.3
                    height: parent.height

                    anchors.right: parent.right
                    color: "transparent"

                    Text {
                        id: timeText
                        anchors.centerIn: parent
                        text: MODREADER.getTeilerForShootOfMatch((currentPageIndex*10)+index).toFixed(1) //isSaveGame ? "NA" : timeComsumed
                        font.pixelSize: 0.65*currentItem.height
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    matchScore.currentIndex = index
                }
            }
        }
    }

    Component {
        id: seriesDelegate
        Item {
            width:seriesListView.width
            height: seriesListView.height/12
            Row {
                spacing: 2
                Text { text: index+1 }
                Text { text: score }
            }
        }
    }

    Component.onCompleted: {
        timer.start()
    }

    function addToSeries(angle,radius,calScore)
    {
        var relativeVal = (10 - radius) > 0 ? 10 - radius : 0
        grandTotal = scoreCutoffTofirstDecimal( grandTotal*1 + (calScore)*1)
//        subTotal = /*Math.round*/scoreCutoffTofirstDecimal( subTotal + (calculatedScore)*1)
        grandTotalExculdeDec = ( grandTotalExculdeDec*1 + Math.floor(calScore))
        totalTimeConsume = totalTimeConsume*1 + (timeInSec - lastUsedTime)


        var text = isGameLoaded ? centerPanel.curShootTimeSavedGame : timeInSec - lastUsedTime
        console.log(timeInSec+"---"+lastUsedTime+"---"+text+" matchinfo------------------------------------------1-----------------"+centerPanel.curShootTimeSavedGame+ "***********"+isGameLoaded)
        lastUsedTime = timeInSec
        MODREADER.appendTimeConsumed(text)
        MODREADER.appendShotDirection(angle.toFixed(2))

        var timeStampString = isGameLoaded ? centerPanel.curShootTimeStampSavedGame :new Date().toLocaleTimeString(Qt.locale("en-US"),"HH:mm:ss")
        MODREADER.appendTimeStamp(timeStampString)

        //        globalModelOfData.append({"direction":angle.toFixed(2), "score":radius.toFixed(2),  "timeComsumed":text})
        // 3P: 0=kneeling 1=prone 2=standing, -1 otherwise. The role must be
        // present on ALL models that copy entries between each other —
        // ListModel role sets are locked at first append, so a copy from a
        // model with this role into one without it throws.
        var shotPosition = is3PMatch ? p3Position : -1
        if(sligterMode)
        {
            globalSlighterModel.append({"direction":angle.toFixed(2)
                                           ,"score":radius.toFixed(2)/*radius.toFixed(2)*/
                                           ,"timeComsumed":text
                                           ,"calculatedscore":scoreCutoffTofirstDecimal(calScore)
                                           ,"position": shotPosition})
            console.log("Shreeraksha-----",APPSETTINGS.getScoringSystem())
        }
        else
        {
            globalMatchModel.append({"direction":angle.toFixed(2)
                                        ,"score":radius.toFixed(2)/*radius.toFixed(2)*/
                                        ,"timeComsumed":text
                                        ,"calculatedscore":scoreCutoffTofirstDecimal(calScore)
                                        ,"timestamp":timeStampString
                                        ,"position": shotPosition})
        }
        globalModelOfData.append({"direction":angle.toFixed(2)
                                     ,"score":radius.toFixed(2)/*radius.toFixed(2)*/,  "timeComsumed":text
                                     ,"calculatedscore":scoreCutoffTofirstDecimal(calScore)
                                     ,"position": shotPosition})

        console.log("$$$$$$$$$$$$ calscore ", calScore)
        console.log("index ",globalModelOfData.count, " timestamp ", globalModelOfData)
        matchScore.model =listModel
        if(minPreviewMode && radius > 4)
        {
            minPreviewMode = false
        }
        if( (loginPage.gameMode === 0 && calScore >= star_limit_value_pistol)
                || (loginPage.gameMode === 1 && calScore >= star_limit_value_rifle) )
            ++totalStars
        var startIndex = Math.floor((globalModelOfData.count-1)/10)
        var endIndex = globalModelOfData.count;
        updateListModel(startIndex*10,endIndex)
    }


    function updateListModel(startIndex,endIndex)
    {
        if (!visible)   // see updateTotal — no view churn while hidden
            return
        listModel.clear()
        console.log("inside updateListModel")
        if (listNavigationON)
            matchScore.model = 0 //used in Qt 5.13
        console.log("inside updateListModel model assign to empty")
        subTotal = subTotal*0
        subTotalExculdeDec = subTotalExculdeDec*0
        seriesStars = 0
        seriesTimeConsume = 0
        for(var i = startIndex; i < endIndex; i++)
        {
            var relativeVal = globalModelOfData.get(i).calculatedscore*1
            if (relativeVal < 0)
                relativeVal = 0
            var direction = globalModelOfData.get(i).direction*1
            var timeConsumed = globalModelOfData.get(i).timeComsumed
            var calculatedScore = globalModelOfData.get(i).calculatedscore

            listModel.append({"direction":direction.toFixed(2),
                                 "score":relativeVal.toFixed(2),
                                 "timeComsumed":timeConsumed,
                                 "calculatedscore":scoreCutoffTofirstDecimal(calculatedScore),})
            console.log("index ",i," before addition ----------------------", subTotal, " timestamp ", timeConsumed)
            subTotal = /*Math.round*//*scoreCutoffTofirstDecimal*/( subTotal + (calculatedScore)*1)
            console.log("index ",i," score test----------------------", subTotal, " curScore ", calculatedScore)
            //            subTotalExculdeDec = ( subTotalExculdeDec*1 + relativeVal.toFixed(0)*1)
            subTotalExculdeDec = ( subTotalExculdeDec*1 + Math.floor(calculatedScore))
            seriesTimeConsume = (seriesTimeConsume + timeConsumed)
            if((loginPage.gameMode === 0 && relativeVal >= star_limit_value_pistol)
                    || (loginPage.gameMode === 1 && relativeVal >= star_limit_value_rifle) )
                ++seriesStars
        }
        matchScore.model = listModel
        console.log("inside updateListModel reassigning the model")
        currentPageIndex = Math.floor( (endIndex-1)/10)
        console.log("inside updateListModel pageindex changed")

        //Log messages
        grandStarText.text = totalStars
        seriesStarText.text =  totalStars //seriesStars
        seriesSubTotal.text = scoreCutoffTofirstDecimal(grandTotal)*1 //scoreCutoffTofirstDecimal(subTotal)*1
        seriesSubTotalED.text = grandTotalExculdeDec.toFixed(0)*1 //subTotalExculdeDec.toFixed(0)*1
        grandTotalText.text = scoreCutoffTofirstDecimal(grandTotal)*1
        grandTotalED.text = grandTotalExculdeDec.toFixed(0)*1
        var formatedTime = minutesToseconds(seriesTimeConsume)
        seriesTime.text = formatedTime//(seriesTimeConsume*1/60).toFixed(1)
        totalTimeConsume = isGameLoaded ? centerPanel.totalTimeSavedGame : totalTimeConsume
        totalTime.text =  minutesToseconds(totalTimeConsume)//(totalTimeConsume*1/60).toFixed(1)

        console.log("last line updateListModel")
    }

    function leftClicked()
    {
        listNavigationON = true
        --currentPageIndex
        var maxPageIndex = Math.floor(globalModelOfData.count/10)
        var startIndex = currentPageIndex*10
        var endIndex = startIndex+10;//maxPageIndex*10
        if(endIndex >= globalModelOfData.count)
        {
            endIndex = globalModelOfData.count
        }
        updateListModel(startIndex,endIndex)
        listNavigationON = false
    }

    function rightClicked()
    {
        listNavigationON = true
        ++currentPageIndex
        var maxPageIndex = Math.floor(globalModelOfData.count/10)
        var startIndex = currentPageIndex*10
        var endIndex = startIndex+10;//(maxPageIndex)*10
        if(endIndex >= globalModelOfData.count)
            endIndex = globalModelOfData.count
        updateListModel(startIndex,endIndex)
        listNavigationON = true
    }

    function updateTotal()
    {
        // Refreshing the score views while the shooting page is hidden (Home
        // teardown) reassigns models under delegates that are mid-destruction
        // and crashes the app natively. The next visible session start runs
        // the full refresh chain anyway.
        if (!visible)
            return
        updateGrandTotal()
        // Clamp to 0: with an empty model, floor((0-1)/10) = -1 and the
        // resulting negative indices make ListModel.get() return undefined,
        // throwing a TypeError that silently aborts the calling chain.
        var startIndex = Math.max(0, Math.floor((globalModelOfData.count-1)/10))
        var endIndex = globalModelOfData.count;
        updateListModel(startIndex*10,endIndex)
    }

    function updateGrandTotal()
    {
        grandTotal = 0;//( grandTotal*1 + relativeVal.toFixed(1)*1)
        grandTotalExculdeDec = 0;// ( grandTotalExculdeDec*1 + Math.floor(relativeVal))
        totalTimeConsume = 0;//totalTimeConsume*1 + (timeInSec - lastUsedTime)
        seriesStars = 0
        totalStars = 0
        subTotal = 0
        subTotalExculdeDec = 0
        seriesTimeConsume = 0

        for(var i = 0; i < globalModelOfData.count; i++)
        {
            var relativeVal = globalModelOfData.get(i).calculatedscore*1
            var direction = globalModelOfData.get(i).direction*1
            var timeConsumed = globalModelOfData.get(i).timeComsumed

            grandTotal = ( grandTotal*1 + scoreCutoffTofirstDecimal(relativeVal)*1)
            grandTotalExculdeDec = ( grandTotalExculdeDec*1 + Math.floor(relativeVal))
            totalTimeConsume = totalTimeConsume*1 + (timeInSec - lastUsedTime)

            if(relativeVal > 10)
                ++totalStars
        }
        grandStarText.text = totalStars
        seriesStarText.text =  seriesStars
        seriesSubTotal.text = scoreCutoffTofirstDecimal(subTotal)*1
        seriesSubTotalED.text = scoreCutoffTofirstDecimal(subTotalExculdeDec)*1
        grandTotalText.text = scoreCutoffTofirstDecimal(grandTotal)*1
        grandTotalED.text = scoreCutoffTofirstDecimal(grandTotalExculdeDec)*1
        seriesTime.text = minutesToseconds(seriesTimeConsume)//(seriesTimeConsume*1 / 60).toFixed(1)
        totalTime.text = minutesToseconds(totalTimeConsume)//(totalTimeConsume*1 / 60).toFixed(1)

        console.log("-----------------------------------------------------------"+scoreCutoffTofirstDecimal(subTotal))
    }

    function enableLeftNavigation(showFlag)
    {
        left_arrow.visible = showFlag
        left_arrow_end.visible = !showFlag
    }

    function enableRightNavigation(showFlag)
    {
        right.visible = showFlag
        right_end.visible = !showFlag
    }

    function resetRightPanelModels()
    {
        listModel.clear()
        globalModelOfData.clear()
        seriesModelScore.clear()
        currentPageIndex = 0
        totalStars = 0
        seriesStars = 0
        totalTimeConsume = 0
        seriesTimeConsume = 0
        subTotal = 0
        subTotalExculdeDec = 0
        grandTotal =0
        grandTotalExculdeDec = 0
        minPreviewMode = true
        timeInSec = 0
        lastUsedTime = 0

        grandStarText.text = ""
        seriesStarText.text =  ""
        seriesSubTotal.text = ""
        seriesSubTotalED.text = ""
        grandTotalText.text = ""
        grandTotalED.text = ""
        seriesTime.text = ""
        totalTime.text = ""

        pauseClicked()
    }

    function pauseClicked()
    {
        switchToSighter(true)
        leftPanel.playVisible = true
        pause_over.visible = false
    }

    function stopClicked()
    {
        if(sligterMode)
            matchNotStarted.visible = true
        else
            matchFinishConfirmation.visible = true
    }

    function startClicked()
    {
        resetTimer()
        switchToSighter(false)
        pause_over.visible = true
        leftPanel.playVisible = false
    }

    function startClickedThroughLoad()
    {
        pause_over.visible = true
        leftPanel.playVisible = false
    }


    function restartClicked()
    {
        //        stop_over.
    }

    // png text for translation
    Text {
        anchors.right: seriesText.left
        anchors.rightMargin: 5
        anchors.top: seriesText.top
        //anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("SERIES")
        color: "white"
        width: implicitWidth
        height: implicitHeight
        font.pixelSize: dafaultFontSize
    }
    Text {
        anchors.left: series_6.left
        anchors.leftMargin: 10
        anchors.bottom: series_6.bottom
        //anchors.bottomMargin: 4
        color: "white"
        width: implicitWidth
        height: implicitHeight
        text: qsTr("SN")
        font.pixelSize: dafaultFontSize
    }
    Text {
        anchors.left: series_6.left
        anchors.leftMargin: 70
        anchors.bottom: series_6.bottom
        //anchors.bottomMargin: 4
        color: "white"
        width: implicitWidth
        height: implicitHeight
        text: qsTr("Score")
        font.pixelSize: dafaultFontSize
    }
    Text {
        anchors.right: series_6.right
        anchors.rightMargin: 25
        anchors.bottom: series_6.bottom
        //anchors.bottomMargin: 4
        color: "white"
        width: implicitWidth
        height: implicitHeight
        text: qsTr("Teiler")//qsTr("Time (s)")
        font.pixelSize: dafaultFontSize
    }
    Rectangle {
        id: midRect
        anchors.left: field_88_7.left
        anchors.right: text_field2.right
        anchors.bottom: text_field2.top
        height: text_field2.height + 8
        color: "transparent"

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: parent.height/2
            color: "transparent"
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                color: "white"
                width: implicitWidth
                height: implicitHeight
                text: qsTr("TOTAL SCORE")
                font.pixelSize: dafaultFontSize
            }
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.height/2
            color: "transparent"
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                color: "white"
                width: implicitWidth
                height: implicitHeight
                text: qsTr("Match Performance")
                font.pixelSize: dafaultFontSize
            }
        }
    }

    Rectangle {
        width: text_field_1_55.width
        anchors.left: midRect.right
        anchors.top: midRect.top
        height: midRect.height/2
        color: "transparent"

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            width: implicitWidth
            height: implicitHeight
            text: qsTr("Time (m)")
            font.pixelSize: dafaultFontSize
        }
    }

    Rectangle {
        anchors.left: field_88_7.left
        anchors.right: text_field2.right
        anchors.top: text_field2.bottom
        height: midRect.height/2
        color: "transparent"
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            width: implicitWidth
            height: implicitHeight
            text: qsTr("MatchOLd Performance")
        }
    }

    function startFromServer()
    {
        leftPanel.playVisible = false;
    }


    // for series Sum
    Rectangle {
        id: series_sum
//        source: "qrc:/images/rightPanel/series_6.png"
        anchors.left: text_field_8.left
        anchors.right: text_field.right
        anchors.rightMargin: text_field.width*0.07
        anchors.top: text_field2.bottom
        anchors.bottom: total_score_block.bottom
        anchors.bottomMargin: 30
        opacity: 1
        //height: series_6.height*1.5
        visible: true
        color: "red"

        Row {
            id: firstRow
            anchors.top: parent.top
            Repeater {
                model: 6
                Rectangle {
                    width: series_sum.width/6; height: series_sum.height/5*1
                    //border.width: 1
                    color: "#373536"

                    Text {
                        anchors.centerIn: parent
                        text: "S"+(index+1)
                        color: "white"
                        font.pixelSize: dafaultFontSize
                    }
                }
            }
        }

        Column {
            anchors.top: firstRow.bottom
            anchors.bottom: parent.bottom
            Row {
                id: secondRow
                Repeater {
                    model: 6
                    Rectangle {
                        width: series_sum.width/6; height: series_sum.height/5*2
                        border.width: 1
                        border.color: "lightgrey"
                        color:/*"blue" //*/"#312E2F"

                        Text {
                            anchors.centerIn: parent
                            text: isValidSeries(index) ? getSeriesTotal(index+1) : ""
                            color: "white"
                            font.pointSize: parent.height*0.25 //parent.height*0.3
//                            font.pointSize: isSingleDecimal ? parent.height*0.37 : parent.height*0.32
//                            font.bold: true

                            onTextChanged: {
                                var textLength = text.length
                                if (textLength == 5)
                                    font.pointSize = parent.height*0.2
                                console.log("sssssssssssssssssssssssssssssssssssss-------------------", textLength)
                            }
                        }
                    }
                }
            }

            Row {
                id: thirdRow
                Repeater {
                    model: 6
                    Rectangle {
                        width: series_sum.width/6; height: series_sum.height/5*2
                        border.width: 1
                        color: "#312E2F"
                        border.color: "white"

                        Text {
                            anchors.centerIn: parent
                            text: isValidSeries(index) ? /*"("+*/getSeriesTotalNonDecimal(index+1)/*+")"*/ : ""
                            color: "white"
                            font.pointSize: parent.height*0.25//parent.height*0.3
//                            font.bold: true

                            onTextChanged: {
                                //update the backend variables and file
                                if (isValidSeries(index)) {
                                    MODREADER.updateSeriesScore(index+1, getSeriesTotalNonDecimal(index+1))
                                    MODREADER.updateSeriesScoreWD(index+1, (getSeriesTotal(index+1)))
                                    MODREADER.setTotalScoreWOD(seriesSubTotalED.text)
                                    MODREADER.setTotalScoreWD(seriesSubTotal.text)
                                    MODREADER.updateSetaShootSummaryData()

//                                    seriesSubTotal.text = scoreCutoffTofirstDecimal(grandTotal)*1 //scoreCutoffTofirstDecimal(subTotal)*1
//                                    seriesSubTotalED.text = grandTotalExculdeDec.toFixed(0)*1 //subTotalExculdeDec.toFixed(0)*1

                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            x: firstRowX
            y: firstRowY
            width: firstRowWidth
            height: firstRowHeight
            color: "transparent"
            border.width: 1
            border.color: "grey"
        }
    }

    function isValidSeries(index) {
        if (currentShootIndex >= index*10)
            return true
        else
            return false
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
        //return isSingleDecimal ? seriesScore.toFixed(1) : scoreCutoffTofirstDecimal(seriesScore)
        return seriesScore.toFixed(1)
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
}
