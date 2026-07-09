import QtQuick 2.2
Item {
    property int rootItemWidth:1674
    property int rootItemHeight:3092
    property int currentPageIndex : 0
    // Series navigation state (drives the new chevron buttons; auto-updates).
    readonly property int maxSeriesPage: globalModelOfData.count > 0 ? Math.floor((globalModelOfData.count - 1) / 10) : 0
    readonly property bool canPrevSeries: currentPageIndex > 0
    readonly property bool canNextSeries: currentPageIndex < maxSeriesPage
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

    // Redesign: the LAST SHOT card sits at the top of the panel; the series
    // header and score table are shifted down by this amount to make room.
    property real tableShift: 68
    // Extra height reclaimed from the score table for the redesigned TOTAL +
    // distribution card that sits between the table and the series cards.
    property real totalCardRoom: 48

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
    // Redesign: uniform dark panel background covering the legacy PNG chrome
    // (the old "total score block" shape peeked out beside the new cards).
    // All content below draws on top of this.
    Rectangle {
        anchors.fill: parent
        color: "#15161a"
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
        opacity: 0   // redesign: value shown in the TOTAL card; kept as data source
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
        opacity: 0   // redesign: value shown in the TOTAL card; kept as data source
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
        opacity: 0   // redesign: value shown in the TOTAL card; kept as data source
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
        opacity: 0   // redesign: value shown in the TOTAL card; kept as data source
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
        opacity: 0   // redesign: legacy white table bg hidden; geometry kept for matchScore
        width: ((parent.width/rootItemWidth)*sourceSize.width)*1.2
        // Shifted down by tableShift for the LAST SHOT card; height reduced by
        // that plus totalCardRoom so the redesigned TOTAL card fits below.
        height: ((parent.height/rootItemHeight)*sourceSize.height) - tableShift - totalCardRoom
        x: ((parent.width/rootItemWidth)*400) - width*0.1
        y: ((parent.height/rootItemHeight)*347) + tableShift
    }
    // Dark score-table background (redesign). Drawn before matchScore so the
    // shot rows render on top.
    Rectangle {
        anchors.fill: num
        color: "#1a1a1f"
        radius: 8
        border.color: "#2a2b30"; border.width: 1
    }

    // ── LAST SHOT card (redesign) ────────────────────────────────────────
    // Reads the most recent entry in globalModelOfData (the same shot shown
    // selected in the table) plus its raw target-face x/y in mm.
    Rectangle {
        id: lastShotCard
        anchors.top: parent.top; anchors.topMargin: 8
        anchors.left: num.left
        anchors.right: num.right
        height: tableShift - 12
        radius: 10
        color: "#1a1a1f"; border.color: "#2a2b30"; border.width: 1

        property int shotN: globalModelOfData.count
        property bool hasShot: shotN > 0
        property real lastScore: hasShot ? globalModelOfData.get(shotN-1).calculatedscore*1 : 0
        property real lastDir: hasShot ? globalModelOfData.get(shotN-1).direction*1 : 0
        property real lastXmm: hasShot ? globalModelOfData.get(shotN-1).xmm*1 : 0
        property real lastYmm: hasShot ? globalModelOfData.get(shotN-1).ymm*1 : 0

        // Left group: label above the big score + direction arrow, centered
        // vertically so it lines up with the X/Y block on the right.
        Column {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: qsTr("LAST SHOT")
                color: "#8a8a92"; font.family: theme.fontFamily
                font.pixelSize: 10; font.letterSpacing: 1.5
            }
            Row {
                spacing: 8
                Text {
                    id: lastShotScore
                    anchors.verticalCenter: parent.verticalCenter
                    text: lastShotCard.hasShot ? scoreCutoffTofirstDecimal(lastShotCard.lastScore)*1 : "—"
                    color: "white"; font.family: theme.fontFamily
                    font.pixelSize: 28; font.bold: true
                }
                Image {
                    visible: lastShotCard.hasShot
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18; height: 18
                    source: "qrc:/images/rightPanel/up_arrow.png"
                    rotation: lastShotCard.lastDir
                }
            }
        }
        // Right group: X/Y in mm, right-aligned, vertically centered.
        Column {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Text {
                anchors.right: parent.right
                text: lastShotCard.hasShot ? ("X   " + lastShotCard.lastXmm.toFixed(1) + " mm") : ""
                color: "#c8c9cf"; font.family: theme.fontFamily; font.pixelSize: 13
            }
            Text {
                anchors.right: parent.right
                text: lastShotCard.hasShot ? ("Y   " + lastShotCard.lastYmm.toFixed(1) + " mm") : ""
                color: "#c8c9cf"; font.family: theme.fontFamily; font.pixelSize: 13
            }
        }
    }
    Image {
        id: series_6
        source: "qrc:/images/rightPanel/series_6.png"
        anchors.left: left_arrow.right
        anchors.right: right.left
        anchors.top: left_arrow.top
        anchors.bottom: left_arrow.bottom
//        x: ((parent.width/rootItemWidth)*400)
//        y: ((parent.height/rootItemHeight)*131) + tableShift
        opacity: 1
//        width: ((parent.width/rootItemWidth)*sourceSize.width)
//        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: true
    }

    Image {
        id: series_text_field
        source: "qrc:/images/rightPanel/series_text_field.png"
        x: ((parent.width/rootItemWidth)*970)
        y: ((parent.height/rootItemHeight)*170) + tableShift
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false   // legacy arrow replaced by nextSeriesBtn
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: false   // legacy arrow replaced by prevSeriesBtn
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
        y: ((parent.height/rootItemHeight)*131) + tableShift
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

    // ── Series navigation — clean chevron buttons over the legacy arrow slots.
    //    Enabled state binds to canPrevSeries/canNextSeries; large hit area.
    Rectangle {
        id: prevSeriesBtn
        anchors.centerIn: left_arrow
        width: Math.max(40, left_arrow.width * 1.15)
        height: Math.max(32, series_6.height * 0.82)
        radius: height / 2
        color: prevMA.pressed && canPrevSeries ? "#1d5fa8" : (canPrevSeries ? "#2f6fd0" : "#23252c")
        border.color: canPrevSeries ? "#4b8ae0" : "#34363e"
        border.width: 1
        opacity: canPrevSeries ? 1 : 0.55
        Text {
            anchors.centerIn: parent
            text: "◀"
            font.family: theme.fontFamily
            color: canPrevSeries ? "white" : "#5a5c64"
            font.pixelSize: parent.height * 0.42; font.bold: true
        }
        MouseArea {
            id: prevMA
            anchors.fill: parent
            enabled: canPrevSeries
            cursorShape: Qt.PointingHandCursor
            onClicked: leftClicked()
        }
    }
    Rectangle {
        id: nextSeriesBtn
        anchors.centerIn: right
        width: Math.max(40, right.width * 1.15)
        height: Math.max(32, series_6.height * 0.82)
        radius: height / 2
        color: nextMA.pressed && canNextSeries ? "#1d5fa8" : (canNextSeries ? "#2f6fd0" : "#23252c")
        border.color: canNextSeries ? "#4b8ae0" : "#34363e"
        border.width: 1
        opacity: canNextSeries ? 1 : 0.55
        Text {
            anchors.centerIn: parent
            text: "▶"
            font.family: theme.fontFamily
            color: canNextSeries ? "white" : "#5a5c64"
            font.pixelSize: parent.height * 0.42; font.bold: true
        }
        MouseArea {
            id: nextMA
            anchors.fill: parent
            enabled: canNextSeries
            cursorShape: Qt.PointingHandCursor
            onClicked: rightClicked()
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
                color: "#3a0d16"   // redesign: themed selection highlight
                visible: matchScore.currentIndex == index //(right_end.visible) && (index === (globalModelOfData.count-1)%10)
            }

            Rectangle {
                id: indexRect
                height: parent.height
                width: parent.width*0.1
                border.color: "#2a2b30"
                color: "transparent"

                Text {
                    text: currentPageIndex*10 + index + 1
                    anchors.centerIn: parent
                    color: "#9a9ba0"
                    font.pixelSize: 0.65*currentItem.height
                }
            }

            Rectangle {
                anchors.right: parent.right
                width: parent.width - indexRect.width
                height: parent.height

                border.color: "#2a2b30"
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
                        color: "white"

                        text:APPSETTINGS.getScoringSystem()? (scoreCutoffTofirstDecimal(calculatedscore)*1): parseInt(scoreCutoffTofirstDecimal(calculatedscore)*1)
                        font.pixelSize: 0.8*currentItem.height
                        font.bold: true
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
                        color: "#c8c9cf"
                        // Per-shot split time in seconds (was the German "Teiler"
                        // precision metric). timeComsumed is a model role in
                        // seconds; no C++ getter, so no negative-index risk.
                        text: (typeof timeComsumed === "undefined" || timeComsumed === "")
                              ? "" : (Number(timeComsumed).toFixed(0) + " s")
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

    function addToSeries(angle,radius,calScore,xmm,ymm)
    {
        // Raw target-face mm (used by the 3P report for per-position group,
        // MPI and plots). Default 0 for callers that don't supply them.
        var shotXmm = xmm === undefined ? 0 : xmm*1
        var shotYmm = ymm === undefined ? 0 : ymm*1
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
                                           ,"position": shotPosition
                                           ,"xmm": shotXmm, "ymm": shotYmm})
            console.log("Shreeraksha-----",APPSETTINGS.getScoringSystem())
        }
        else
        {
            globalMatchModel.append({"direction":angle.toFixed(2)
                                        ,"score":radius.toFixed(2)/*radius.toFixed(2)*/
                                        ,"timeComsumed":text
                                        ,"calculatedscore":scoreCutoffTofirstDecimal(calScore)
                                        ,"timestamp":timeStampString
                                        ,"position": shotPosition
                                        ,"xmm": shotXmm, "ymm": shotYmm})
        }
        globalModelOfData.append({"direction":angle.toFixed(2)
                                     ,"score":radius.toFixed(2)/*radius.toFixed(2)*/,  "timeComsumed":text
                                     ,"calculatedscore":scoreCutoffTofirstDecimal(calScore)
                                     ,"position": shotPosition
                                     ,"xmm": shotXmm, "ymm": shotYmm})

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
        if (listNavigationON)
            matchScore.model = 0 //used in Qt 5.13
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
        // Clamp: entering match mode with 0 match shots gives endIndex 0 →
        // floor(-1/10) = -1, and a negative page index feeds negative shot
        // numbers into delegate bindings (see getTeilerForShootOfMatch).
        currentPageIndex = Math.max(0, Math.floor( (endIndex-1)/10))

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

    // Legacy PNG arrows replaced by prevSeriesBtn/nextSeriesBtn, which bind
    // directly to canPrevSeries/canNextSeries. Kept as no-ops so existing
    // callers stay valid.
    function enableLeftNavigation(showFlag) {}
    function enableRightNavigation(showFlag) {}

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
        text: qsTr("Time (s)")
        font.pixelSize: dafaultFontSize
    }
    Rectangle {
        id: midRect
        visible: false   // redesign: replaced by the TOTAL + distribution card
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
        // Distribution strip: live ring-band tallies (10s / 9s / 8s / <=7).
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            spacing: 12
            Repeater {
                model: [{ lbl: qsTr("10s"), b: 10 }, { lbl: qsTr("9s"), b: 9 },
                        { lbl: qsTr("8s"), b: 8 }, { lbl: qsTr("≤7"), b: 7 }]
                delegate: Row {
                    spacing: 4
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.lbl
                        color: "#8a8a92"; font.pixelSize: 12
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: bandCount(modelData.b, globalModelOfData.count)
                        color: "white"; font.pixelSize: 13; font.bold: true
                    }
                }
            }
        }
    }

    Rectangle {
        visible: false   // redesign: "Time (m)" label folded into the TOTAL card
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

    // (removed leftover "MatchOld Performance" label — redesign)

    function startFromServer()
    {
        leftPanel.playVisible = false;
    }


    // ── TOTAL + distribution card (redesign) ─────────────────────────────
    // Sits between the score table and the series cards. Reads grandTotal /
    // grandTotalExculdeDec / totalStars / totalTimeConsume directly; the
    // legacy value texts are kept (opacity 0) only for their backend pushes.
    Rectangle {
        id: totalCard
        anchors.left: num.left
        anchors.right: num.right
        anchors.top: num.bottom
        anchors.topMargin: 8
        anchors.bottom: series_sum.top
        anchors.bottomMargin: 8
        radius: 10
        color: "#1a1a1f"; border.color: "#2a2b30"; border.width: 1

        property int shots: globalModelOfData.count

        // Top row: TOTAL label + big score (int) · inner-10s · time
        Item {
            id: totalTopRow
            anchors.top: parent.top; anchors.topMargin: 8
            anchors.left: parent.left; anchors.leftMargin: 14
            anchors.right: parent.right; anchors.rightMargin: 14
            height: 40
            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5
                Text {
                    text: qsTr("TOTAL SCORE")
                    color: "#8a8a92"; font.family: theme.fontFamily
                    font.pixelSize: 10; font.letterSpacing: 1.5
                }
                Row {
                    spacing: 5
                    Text {
                        anchors.baseline: totalInt.baseline
                        text: scoreCutoffTofirstDecimal(grandTotal)*1
                        color: "white"; font.family: theme.fontFamily
                        font.pixelSize: 24; font.bold: true
                    }
                    Text {
                        id: totalInt
                        text: "(" + grandTotalExculdeDec + ")"
                        color: "#8a8a92"; font.family: theme.fontFamily
                        font.pixelSize: 13
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 18
                Column {
                    Text {
                        anchors.right: parent.right
                        text: qsTr("INNER 10s")
                        color: "#8a8a92"; font.family: theme.fontFamily; font.pixelSize: 10
                    }
                    Text {
                        anchors.right: parent.right
                        text: "★ " + totalStars
                        color: "#ffb020"; font.family: theme.fontFamily
                        font.pixelSize: 15; font.bold: true
                    }
                }
                Column {
                    Text {
                        anchors.right: parent.right
                        text: qsTr("TIME")
                        color: "#8a8a92"; font.family: theme.fontFamily; font.pixelSize: 10
                    }
                    Text {
                        anchors.right: parent.right
                        text: minutesToseconds(totalTimeConsume)
                        color: "white"; font.family: theme.fontFamily
                        font.pixelSize: 15; font.bold: true
                    }
                }
            }
        }

        // Distribution: horizontal bars, one per ring band, width proportional
        // to the tally. Colour-coded so quality reads at a glance.
        Column {
            anchors.top: totalTopRow.bottom; anchors.topMargin: 6
            anchors.left: parent.left; anchors.leftMargin: 14
            anchors.right: parent.right; anchors.rightMargin: 14
            anchors.bottom: parent.bottom; anchors.bottomMargin: 8
            spacing: 3
            Repeater {
                model: [{ lbl: "10", b: 10, c: "#2ecc71" },
                        { lbl: "9",  b: 9,  c: "#3aa0ff" },
                        { lbl: "8",  b: 8,  c: "#ffb020" },
                        { lbl: "≤7", b: 7,  c: "#e8003d" }]
                delegate: Item {
                    width: parent.width
                    height: (parent.height - 9) / 4
                    property int cnt: bandCount(modelData.b, totalCard.shots)
                    Text {
                        id: bandLbl
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        text: modelData.lbl
                        color: "#9a9ba0"; font.family: theme.fontFamily; font.pixelSize: 11
                        horizontalAlignment: Text.AlignRight
                    }
                    Rectangle {   // track
                        id: bandTrack
                        anchors.left: bandLbl.right; anchors.leftMargin: 8
                        anchors.right: bandCnt.left; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        height: 9; radius: 4
                        color: "#141519"
                        Rectangle {   // fill
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            height: parent.height; radius: 4
                            width: parent.width * (parent.parent.cnt / maxBandCount(totalCard.shots))
                            color: modelData.c
                            visible: parent.parent.cnt > 0
                        }
                    }
                    Text {
                        id: bandCnt
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 16
                        text: parent.cnt
                        color: "white"; font.family: theme.fontFamily
                        font.pixelSize: 11; font.bold: true
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }

    // for series Sum
    Rectangle {
        id: series_sum
//        source: "qrc:/images/rightPanel/series_6.png"
        // Aligned to the card column (num) so it lines up with the LAST SHOT,
        // score-table and TOTAL cards above it.
        anchors.left: num.left
        anchors.right: num.right
        anchors.top: text_field2.bottom
        anchors.bottom: total_score_block.bottom
        anchors.bottomMargin: 30
        opacity: 1
        //height: series_6.height*1.5
        visible: true
        color: "transparent"   // redesign: themed cells below draw the look

        Row {
            id: firstRow
            anchors.top: parent.top
            Repeater {
                model: 6
                Rectangle {
                    width: series_sum.width/6; height: series_sum.height/5*1
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "S"+(index+1)
                        color: "#8a8a92"
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
                        border.color: "#2a2b30"
                        radius: 6
                        color: "#1a1a1f"

                        Text {
                            anchors.centerIn: parent
                            text: isValidSeries(index) ? getSeriesTotal(index+1) : "—"
                            color: isValidSeries(index) ? "white" : "#55555e"
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
                        color: "#141519"
                        border.color: "#2a2b30"
                        radius: 6

                        Text {
                            anchors.centerIn: parent
                            text: isValidSeries(index) ? /*"("+*/getSeriesTotalNonDecimal(index+1)/*+")"*/ : ""
                            color: "#9a9ba0"
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
        // 3P: the six series map to the whole match (K=S1,2 · P=S3,4 · S=S5,6),
        // so validity follows the continuous match record — not the per-position
        // display buffer, which resets to 20 shots on each position change and
        // made S1,S2 re-appear for prone/standing. Non-3P behaviour is unchanged.
        if (is3PMatch)
            return globalMatchModel.count > index*10
        if (currentShootIndex >= index*10)
            return true
        else
            return false
    }

    function getSeriesTotal(seriesIndex)
    {
        // 3P sums the continuous match record so S3-S6 carry prone/standing;
        // other disciplines keep the per-position display buffer. Per-shot
        // calculatedscore values are untouched — only which shots are summed.
        var seriesData = is3PMatch ? globalMatchModel : globalModelOfData
        if(seriesData.count === 0)
            return 0
        var seriesScore = 0
        for(var i=(seriesIndex-1)*10; i<seriesData.count; i++)
        {
            if (i >=seriesIndex*10)
                break;

            var scoreatIndex = seriesData.get(i).calculatedscore*1
            seriesScore = seriesScore*1  +  (scoreatIndex.toFixed(1))*1
            //            console.log("Total score and score at current Index is",seriesScore,scoreatIndex)

        }
        //return isSingleDecimal ? seriesScore.toFixed(1) : scoreCutoffTofirstDecimal(seriesScore)
        return seriesScore.toFixed(1)
    }

    // Score-distribution count: how many match shots fall in a ring band.
    // `trigger` is passed globalModelOfData.count so the binding re-evaluates
    // as shots land (functions aren't reactive on their own).
    function bandCount(band, trigger)
    {
        var n = 0
        for (var i = 0; i < globalModelOfData.count; i++) {
            var s = Math.floor(globalModelOfData.get(i).calculatedscore*1)
            if (band === 10) { if (s >= 10) n++ }
            else if (band === 7) { if (s <= 7) n++ }
            else if (s === band) n++
        }
        return n
    }

    // Largest band tally (>=1) so the distribution bars scale to full width.
    function maxBandCount(trigger)
    {
        return Math.max(bandCount(10, trigger), bandCount(9, trigger),
                        bandCount(8, trigger), bandCount(7, trigger), 1)
    }

    function getSeriesTotalNonDecimal(seriesIndex)
    {
        // Integer series total. 3P reads the continuous match record (so the
        // bracketed integer series match K/P/S), non-3P the per-position buffer.
        var seriesData = is3PMatch ? globalMatchModel : globalModelOfData
        if(seriesData.count === 0)
            return 0
        var seriesScore = 0
        for(var i=(seriesIndex-1)*10; i<seriesData.count; i++)
        {
            if (i >=seriesIndex*10)
                break;

            var scoreatIndex = Math.floor(seriesData.get(i).calculatedscore*1)
            seriesScore = seriesScore*1  +  (scoreatIndex)*1
        }
        return seriesScore
    }
}
