import QtQuick 2.2
import QtCharts 2.2
import QtQuick.Window 2.2
import QtQuick.Dialogs

Item {
    id:paneItem
    property var itemPoint: Qt.point(0,0)
    property double calculatedSccore: 0
    property double currentScoreValue: -1
    property double currentScoreDegree: -1
    property int shotCount: -1
    property int shotCount_on_app_minimise: -1
    property int windowVisibleMode: 0
    property int backEndShootCount: 0
    property double zoom_offset: 0.51

    // ── SIUS-style auto-zoom on shot ─────────────────────────────────────
    // When a shot lands, magnify the target around the shot (autoZoomOrigin)
    // for a moment, then ease back out. Driven declaratively via a Behavior +
    // a repeating hold Timer (armed by a `running:` binding) so it is safe
    // from the shot pipeline (imperative Timer.restart()/callLater there fail).
    property bool autoZoomOn: true
    property real autoZoomTarget: 2.4   // magnification while a shot is fresh
    property real autoZoomFactor: 1
    property real autoZoomOriginX: shootingPanelRect.width/2
    property real autoZoomOriginY: shootingPanelRect.height/2
    property int  autoZoomHold: 0       // ticks remaining before easing out

    function triggerAutoZoom(px, py) {
        if (!autoZoomOn)
            return
        autoZoomOriginX = px
        autoZoomOriginY = py
        autoZoomFactor = autoZoomTarget
        autoZoomHold = 4                // ~4 * 450ms hold, reset on each shot
    }

    Timer {
        interval: 450; repeat: true
        running: shootingPage.visible && paneItem.autoZoomHold > 0
        onTriggered: {
            paneItem.autoZoomHold = paneItem.autoZoomHold - 1
            if (paneItem.autoZoomHold <= 0)
                paneItem.autoZoomFactor = 1
        }
    }

    property bool lightBackGroundMode: true
    property bool gameMode: shootingPage.currentGameDisplay2 === qsTr("PISTOL") ? true : false // true for pistol
    property int shootinRectWidth: shootingMianRect.width
    property int shootinRectHeight: shootingMianRect.height



    signal pointAddedToSeries(real xPosition, real yPosition, real currentCalculatedScore)

    // Raw target-face coordinates (mm) of the most recent shot, captured at
    // scoring time so the shot models can store real geometry — the polar
    // (direction, score) pair the models held before this cannot recover mm
    // without unit archaeology. Read by ShootingPage.onPointAddedToSeries.
    property real lastShotXmm: 0
    property real lastShotYmm: 0
    //    signal pointAddedXYPoints(real xPosition, real yPosition)

    property int gameTime: 0
    property int totalGameTime: 0
    property int sighterTime: 0
    property int totalSighterTime: 0
    property bool showPolarChart: false
    property bool showShootingAnimation: true

    property int screenWidth : scWidth
    property bool disableMotorMovement: false
    property bool autoMotorMovementMode: true

    property double group_distance: 0
    property double highestShoot: 10
    property double  totalTimeSavedGame: 0
    property double  curShootTimeSavedGame: -1
    property string curShootTimeStampSavedGame: "test"

    signal sighterModeTimerEnds()
    signal positionResumeRequested()   // 3P: athlete starts record fire in the new position

    onGameModeChanged: {
        circleCordinates()
        innercircle.updateInnerCircleWidth()
        externalRect.updateExternalRectWidth()
    }

    onTotalGameTimeChanged: {
        var remainingTime = (totalGameTime - gameTime)*1
        var formatedTime = minutesToseconds(remainingTime)
        stopTimer.text = formatedTime
    }

    onWidthChanged:
    {
        circleCordinates()
        externalRect.updateExternalRectWidth()
    }

    onHeightChanged:
    {
        circleCordinates()
        externalRect.updateExternalRectWidth()
    }

    Component.onCompleted:
    {
        externalRect.updateExternalRectWidth()
        readDataFromBAckEnd()
        MODREADER.setIsAppDemoMode(appMode)
    }

    onVisibleChanged: {
        console.log(" visible ", visible, " shoot count ", APPSETTINGS.getLoadedGameShotCount())
        if (visible && APPSETTINGS.getLoadedGameShotCount()) {
            //rightPanel.startClicked() //changedToMatchMode()
            showShootingAnimation = false
            rightPanel.isGameLoaded = true
            for (var i=0; i<APPSETTINGS.getLoadedGameShotCount(); ++i)
            {
                var xCor = APPSETTINGS.getLoadedGameX(i)
                var yCor = APPSETTINGS.getLoadedGameY(i)
                curShootTimeSavedGame  = APPSETTINGS.getLoadedGameTime(i)
                curShootTimeStampSavedGame = APPSETTINGS.getLoadedGameTimeStamp(i)
                console.log("------------------time-----------------------", curShootTimeSavedGame)
                totalTimeSavedGame += curShootTimeSavedGame
                // for auto relaod after clickfF
                //shootingItem.append({"xC": xCor, "yC": yCor})
                // reload once the page is visible
                MODREADER.uxShoot(xCor, yCor)
                //                sleep(1000)
            }

            gameTime = totalTimeSavedGame
            APPSETTINGS.clearLoadedData()
            showShootingAnimation = true
            rightPanel.isGameLoaded = false
            isSaveGame = false
        }
    }

    onGroup_distanceChanged: {
        //        groupText.text = qsTr("Group: ") + group_distance.toFixed(2) + qsTr(" mm")
        //        if (group_distance == 0)
        //            groupRect.visible = false
        //        else if (leftPanel.isShowMPI && numberOverlayRepeater.count > 1 && !sighter.visible)
        //            groupRect.visible = true
    }

    ListModel {
        id: shootingItem
        //        ListElement {
        //            xC: -5.2
        //            yC: -12.8
        //        }
        //        ListElement {
        //            xC: -4.4
        //            yC: 9.8
        //        }
    }

    function refreshShowMesureStatus(showMesures) {
        if (showMesures) {
            if (numberOverlayRepeater.count > 1) {
                //mpiRect.visible = true
                if (!sighter.visible)
                    groupRect.refreshPosition()
            }
        } else {
            //            mpiRect.visible = false
            groupRect.visible = false
        }
    }

    function refreshGroupRect() {
        groupRect.refreshPosition()
    }

    function pauseGameTimer()
    {
        if (gameTime > 0)
            gameTimer.stop()
    }

    function unPauseGameTimer()
    {
        if (gameTime > 0)
            gameTimer.start()
    }

    // sleep time expects milliseconds
    function sleep (time) {
        return new Promise((resolve) >= setTimeout(resolve, time));
    }

    function readDataFromBAckEnd() {
        var newShootCount = MODREADER.getShootCount()
        if (backEndShootCount < newShootCount)
        {
            for (var i = backEndShootCount+1; i<= newShootCount; i++)
            {
                var xCor = MODREADER.getXCord(i)
                var yCor = MODREADER.getYCord(i)

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingMianRect.width/shootingWidth
                var offsetY = shootingMianRect.height/shootingHeight

                var centerX = shootingPanelRect.width/2 //* offset
                var centerY = shootingPanelRect.height/2 //* offset

                //                var bulletSize = 0//4.5/2
                var bulletSize = gameMode ? 4.5/2 : 0

                itemPoint.x = centerX+((xCor+bulletSize)*offsetX)//mouseX/shootingPanelRect.scale + (mapToItem(root,0,0).x)
                itemPoint.y = centerY-((yCor+bulletSize)*offsetY)//mouseY/shootingPanelRect.scale + (mapToItem(root,0,0).y)

                calculateShootingSocre(itemPoint.x, itemPoint.y, itemPoint.x, itemPoint.y)
                animatorCircle.x = (itemPoint.x - animatorCircle.width/2)
                animatorCircle.y = (itemPoint.y - animatorCircle.width/2)
                animatorCircle.visible = true

                sleep(1000)
            }

            backEndShootCount = newShootCount
        }

    }

    function shootCountChangedImpactAfterAppRestore(count) {
        for(var i=0; i<numberOverlayRepeater.count; ++i) {
            numberOverlayRepeater.itemAt(i).refreshBulletPostion();
        }

        refreshSelectedShootPosition();
    }

    Connections {
        target: MODREADER
        onShootCountChanged: {
            //            var logData1 = "onShootCountChanged window visibilty changed.............................."+ windowVisibleMode
            //            MODREADER.appendToLogFile(logData1)

            //            if (windowVisibleMode == 3) {
            //                shotCount_on_app_minimise = count
            //                logData1 = "onShootCountChanged window visibilty changed..............................shoot count "+ count
            //                MODREADER.appendToLogFile(logData1)

            //                return;
            //            }


            var logData1 = "inside onShootCountChanged .............................."
            MODREADER.appendToLogFile(logData1)
            if (globalModelOfData.count === shotCount) {
                var logData = "Game over "+ shotCount
                MODREADER.appendToLogFile(logData)
                return
            }

            var shooutIndex = count
            var newShootCount = MODREADER.getShootCount()
            console.log("shooutIndex ", shooutIndex, " newShootCount ", newShootCount, " backEndShootCount ", backEndShootCount)
            if (backEndShootCount < shooutIndex)
            {
                //for (var i = backEndShootCount+1; i<= newShootCount; i++)
                //{
                var xCor = MODREADER.getXCord(shooutIndex)
                var yCor = MODREADER.getYCord(shooutIndex)

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingMianRect.width/shootingWidth
                var offsetY = shootingMianRect.height/shootingHeight

                var centerX = shootingPanelRect.width/2 //* offset
                var centerY = shootingPanelRect.height/2 //* offset

                var bulletSize = 0//4.5/2

                itemPoint.x = centerX+((xCor+bulletSize)*offsetX)//mouseX/shootingPanelRect.scale + (mapToItem(root,0,0).x)
                itemPoint.y = centerY-((yCor+bulletSize)*offsetY)//mouseY/shootingPanelRect.scale + (mapToItem(root,0,0).y)

                console.log(" x pos ", itemPoint.x)
                console.log(" y Pos ", itemPoint.y)

                calculateShootingSocre(xCor, yCor, itemPoint.x, itemPoint.y)

                showShootingAnimation = false // removing the animation circle
                if (showShootingAnimation)
                {
                    animatorCircle.x = (itemPoint.x - animatorCircle.width/2)
                    animatorCircle.y = (itemPoint.y - animatorCircle.width/2)
                    animatorCircle.visible = true
                } else {
                    var temp = root.mapToValue(paneItem.itemPoint,polarSeries);
                    addIfinRange(temp)
                }

                // SIUS-style: zoom in on where the shot just landed.
                paneItem.triggerAutoZoom(paneItem.itemPoint.x, paneItem.itemPoint.y)

                backEndShootCount = newShootCount

                var curSeriesIndex = Math.floor((newShootCount-1)/shootsPerSeries)
                console.log(newShootCount,"---***********************************************************", curSeriesIndex)
                if (/*rightPanel.isValidSeries(curSeriesIndex)*/1) {
                    MODREADER.updateSeriesScore(curSeriesIndex+1, rightPanel.getSeriesTotalNonDecimal(curSeriesIndex+1))
                    MODREADER.updateSeriesScoreWD(curSeriesIndex+1, (rightPanel.getSeriesTotal(curSeriesIndex+1)))
                    //                    MODREADER.setTotalScoreWOD(seriesSubTotalED.text)
                    //                    MODREADER.setTotalScoreWD(seriesSubTotal.text)
                }
            }
            //            MODREADER.initiateMotorMovement()
            //            // for issue -> Auto feed not working #76
            //            if (!MODREADER.checkAutoFeedMode()) {
            //                MODREADER.appendToLogFile("Auto feed is false, so resetting the autofeed mode")
            //                MODREADER.intiateAutoMovementSetup()
            //            } else
            //                MODREADER.appendToLogFile("Auto feed is true.")
            //            // end
            APPSETTINGS.saveMatch()
        }

        onHardwareDisconnected: {
            gameTimer.stop()
            //            conError.visible = true
            conErrorDia.visible = true
            //            errorRect.visible = true
            var logData = "Atepting to reconnect"
            MODREADER.appendToLogFile(logData)

            MODREADER.attemptReconnection()
            //            hardwareDisconnected.visible = true
        }

        onHardwareReconnected: {
            //            hardwareDisconnected.visible = false
            //            hardwareDisconnected.text = "Reconnection successfully, Resuming the Game."
            //            hardwareDisconnected.inDisconnectedMode = false
            //            hardwareDisconnected.visible = true
            if (!sighter.visible)
                gameTimer.start()
            //            conError.visible = false
            conErrorDia.visible = false
            //            errorRect.visible = false
        }
    }

    Connections {
        target: window
        onAppVisiblityModeChanged: {
            var logData = "window visibilty changed.............................."+ mode
            MODREADER.appendToLogFile(logData)

            if (windowVisibleMode == 3 && (mode == 4 || mode == 5)) {
                shootCountChangedImpactAfterAppRestore(shotCount_on_app_minimise)
            }

            windowVisibleMode = mode
            //QWindow::Minimized            3
            //QWindow::Maximized            4
            //QWindow::FullScreen           5
        }
    }

    MessageDialog
    {
        id: hardwareDisconnected

        property bool inDisconnectedMode: true

        title: "Connection Error"
        text: qsTr("Trying to reconnect. Please wait.")
        visible: false

        onAccepted: {
            if (!inDisconnectedMode) {
                inDisconnectedMode = true
                gameTimer.start()
            }
        }
    }

    Timer {
        id: shootingTimer
        interval: 1000
        repeat: true
        property int counter: 0

        onTriggered: {
            console.log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", shootingItem.count)
            if (counter >= shootingItem.count)
            {
                shootingTimer.stop()
                return
            }
            var xCor = shootingItem.get(counter).xC
            var yCor = shootingItem.get(counter).yC
            MODREADER.uxShoot(xCor, yCor)

            counter++
        }
    }

    Timer {
        id:gameTimer
        interval: 1000
        repeat: true
        onTriggered: {
            gameTime++;
            var remainingTime = (totalGameTime - gameTime)*1
            var formatedTime = minutesToseconds(remainingTime)
            //            console.log("Formated Time is .......", formatedTime,remainingTime,
            //                        gameTime,totalGameTime)
            stopTimer.text = formatedTime
        }
    }

    Timer {
        id:sighterTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (isSaveGame)
                return

            sighterTime++;
            var remainingTime = (totalSighterTime - sighterTime)*1
            if(remainingTime <= 0 )
            {
                sighterModeTimerEnds()
                sighterTimer.stop()
                //sighter.visible = false;
                // slighterTimeUpdate hides via its !timerNotification.visible
                // binding — do not assign imperatively (it would break the
                // binding for the next session).
                //gameTimer.start()
                timerNotification.visible = true;
                MODREADER.intiateAutoMovementSetup()
            }
            var formatedTime = minutesToseconds(remainingTime)
            stStopTimer.text = formatedTime
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#ffffdd"
        //border.color: "blue"
    }

    Rectangle {
        id: shootingPanelRect
        width: parent.width*0.8
        height: parent.height*0.8
        anchors.centerIn: parent
        color: "transparent"

        // Auto-zoom on shot: scales the target (and its markers) around the
        // shot position. Compounds with the manual +/- zoom (built-in `scale`,
        // origin Center); at rest autoZoomFactor is 1 so this is identity.
        transform: Scale {
            origin.x: paneItem.autoZoomOriginX
            origin.y: paneItem.autoZoomOriginY
            xScale: paneItem.autoZoomFactor
            yScale: paneItem.autoZoomFactor
            Behavior on xScale { NumberAnimation { duration: 300; easing.type: Easing.InOutQuad } }
            Behavior on yScale { NumberAnimation { duration: 300; easing.type: Easing.InOutQuad } }
        }
        property string secondColor: greenColor //gameRange == 50 ? greenColor : "yellow"
        property string firstColor: gameRange == 10 ? "yellow" : "red" //gameRange == 10 ? greenColor : "red"
        property string highestShootColor: "red"

        property string secondImg: "qrc:/images/centerPanel/10mtr_green1.png"
        property string firstImg: gameRange == 10 ? "qrc:/images/centerPanel/10mtr_yellow.png"
                                                  : "qrc:/images/centerPanel/10mtr_red.png"
        property string secondImgWithWhiteRing: "qrc:/images/centerPanel/10mtr_green1_normal.png"
        property string firstImgWithWhiteRing: gameRange == 10 ? "qrc:/images/centerPanel/10mtr_yellow_normal.png"
                                                               : "qrc:/images/centerPanel/10mtr_red.png"
        property string secondImgRedCircle: "qrc:/images/centerPanel/10m_green_red.png"
        property string firstImgRedCircle: gameRange == 10 ? "qrc:/images/centerPanel/10m_yellow_red.png.png"
                                                           : "qrc:/images/centerPanel/Pistol bullet_50m.png"

        Component.onCompleted: {

            if (APPSETTINGS.getIsPalletTypeNormal() /*&& gameRange == 10*/) {
                firstImg = firstImgWithWhiteRing
                secondImg = secondImgWithWhiteRing
            }
        }

        Image {
            id: shootingcanvas
            anchors.margins: 10
            source: gameRange == 10 ? (gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                                : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                    : (gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
            anchors.centerIn: parent
            height: (parent.height > parent.width ? parent.width : parent.height) - anchors.margins
            width: height
            opacity: showPolarChart ? 0 : 1

            Rectangle {
                id: shootingMianRect
                color: "transparent"
                anchors.fill: parent
                //                border.color: "red"
            }
        }

        PolarChartView {
            id:root
            anchors.fill: parent
            antialiasing: true
            backgroundColor: "transparent"
            opacity: showPolarChart ? 1 : 0

            Component.onCompleted:
            {
                legend.visible = false;
            }

            ValueAxis {
                id: axisAngular
                min: 0
                max: 360
                labelsVisible: false
                color: "transparent"
                gridLineColor: "transparent"
                gridVisible: false
                lineVisible: false
                minorGridVisible: false
            }

            ValueAxis {
                id: axisRadial
                min: 0
                max: gameMode ? 10 : 9
                tickCount: gameMode ? 11 : 10
                labelsVisible: false
                lineVisible: false
                gridLineColor: lightBackGroundMode ? "black" : "white"
            }

            ScatterSeries {
                id: polarSeries
                axisAngular: axisAngular
                axisRadial: axisRadial
                markerSize: screenWidth*0.1;//screenResolution.desktopAvailableWidth *0.1//25
                color: greenColor
                borderColor: "grey"
                opacity: 0.5
                visible: false
                onPointAdded:
                {
                    var temp =  polarSeries.at(index)
                    console.log("point on main canvas "+ temp)
                    pointAddedToSeries(temp.x,temp.y, calculatedSccore)
                    paneItem.currentScoreDegree = temp.x
                    paneItem.currentScoreValue = calculatedSccore
                }
            }

            ScatterSeries {
                id: currentPolarSeriesItem
                axisAngular: axisAngular
                axisRadial: axisRadial
                markerSize: screenWidth*0.1 *0.1//25
                opacity: 0.5
                color: "red"
                visible: false

                //                onCountChanged: {
                //                    if (count === 1)
                //                        MODREADER.initiateMotorMovement()
                //                }
            }
        }

        Rectangle {
            id: externalRect
            anchors.centerIn: parent
            //width: !gameMode ? parent.height - (innercircle.width/innerCirclePartitionRepeater.count)/2 -2: parent.height -  innercircle.width/innerCirclePartitionRepeater.count + 7
            width: root.height - root.height/(axisRadial.tickCount)//gameMode ? (root.height/(axisRadial.tickCount))*10 : (root.height/(axisRadial.tickCount))*9.5 - 4

            height: width
            color: "transparent"
            border.color: gameRange == 10 ? greenColor : "red"

            opacity: 0
            Component.onCompleted: {
                console.log("-----------width---", width)
                updateExternalRectWidth()
            }

            onWidthChanged: {
                console.log("-----------width########---", width)
                console.log("co-centric circle dimension ---x---", x,"---y---", y,"-----------width########---", width)
            }

            function updateExternalRectWidth()
            {
                var startinPosition = axisRadial.max
                width = root.mapToPosition(Qt.point(90,(startinPosition)),polarSeries).x - root.mapToPosition(Qt.point(270,(startinPosition)),polarSeries).x
            }
        }

        Rectangle
        {
            id:innercircle
            z:5
            visible: true
            width: gameMode ? (root.width/(axisRadial.tickCount))*4 : (root.width/(axisRadial.tickCount))*6
            opacity: showPolarChart ? 1 : 0

            height: width
            radius:width/2
            color: "black"
            //            opacity: 0.3

            anchors.centerIn: root

            function updateInnerCircleWidth()
            {
                var temp = gameMode ? 4 : 6
                innercircle.width = root.height > root.width ?
                            (root.width/(axisRadial.tickCount))*temp:
                            (root.height/(axisRadial.tickCount))*temp
                console.log("game ", gameMode, " --- ", temp)
            }

            onWidthChanged: {
                height = width
                radius = width/2
            }

            Component.onCompleted: {
                updateInnerCircleWidth()
            }

            Repeater {
                id: innerCirclePartitionRepeater
                model: 4
                visible: gameMode

                delegate: Rectangle {
                    anchors.centerIn: innercircle
                    width: (index+1)* (innercircle.width/innerCirclePartitionRepeater.count)
                    height: width
                    radius: width/2
                    border.color: "white"
                    color: "transparent"
                    visible: index === innerCirclePartitionRepeater.count - 1 ? false : gameMode
                }
            }

            Repeater {
                id: innerCirclePartitionRepeaterRifle
                model: 6
                visible: !gameMode

                delegate: Rectangle {
                    anchors.centerIn: innercircle
                    width: (index+1)* (innercircle.width/innerCirclePartitionRepeaterRifle.count)
                    height: width
                    radius: width/2
                    border.color: "white"
                    color: "transparent"
                    visible: index === innerCirclePartitionRepeaterRifle.count - 1 ? false : !gameMode
                }
            }

            Rectangle {
                id: centerCircle
                anchors.centerIn: parent
                width: gameMode ? (innercircle.width/innerCirclePartitionRepeater.count)/2 : 5
                height: width
                radius: width/2
                color: gameMode ? "transparent" : "white"
                border.color: "white"
            }
        }

        Rectangle {
            id: animatorCircle
            width: polarSeries.markerSize*4
            height: polarSeries.markerSize*4
            color: "transparent"
            border.color: shootingPage.isPalletRed ? "red" : "black"
            border.width: 2
            radius: width/2
            visible: false
            z: 20

            ParallelAnimation {
                id: pAnimationCircle
                ScaleAnimator {
                    target: animatorCircle
                    from: 0.25
                    to: 1
                    duration: 500
                }
                running: true

                onStopped: {
                    animatorCircle.visible = false
                    var temp = root.mapToValue(paneItem.itemPoint,polarSeries);
                    //                pointAddedXYPoints(itemPoint.x, itemPoint.y)
                    addIfinRange(temp)
                }
            }

            onVisibleChanged: {
                if (visible)
                    pAnimationCircle.start()
            }
        }

        Repeater
        {
            id:scoreIndicator
            model:8
            delegate: numberDelegate
        }
        Component {
            id:numberDelegate

            Item {
                property double startinPosition: gameMode ? 9.5 : 8.5
                opacity: showPolarChart ? 1 : 0

                z: 20
                Item {
                    id:leftItem
                    width:15
                    height: 15
                    x: (root.mapToPosition(Qt.point(270,(startinPosition - index*1)),polarSeries).x) - (width/2)
                    y: (root.mapToPosition(Qt.point(270,(startinPosition - index*1)),polarSeries).y) - (width/2)
                    Rectangle
                    {
                        anchors.fill: parent
                        radius:parent.width/2
                        color:"transparent"
                        Text{
                            anchors.centerIn: parent
                            text: index+1
                            color: gameMode ? (index>5 ? "white" : "black") : (index>2 ? "white" : "black")
                        }
                    }
                }
                Item {
                    id:topItem
                    width:15
                    height: 15
                    x: (root.mapToPosition(Qt.point(0,(startinPosition - index*1)),polarSeries).x) - (width/2)
                    y: (root.mapToPosition(Qt.point(0,(startinPosition - index*1)),polarSeries).y) - (width/2)
                    Rectangle
                    {
                        anchors.fill: parent
                        radius:parent.width/2
                        color:"transparent"
                        Text{
                            anchors.centerIn: parent
                            text: index+1
                            color: gameMode ? (index>5 ? "white" : "black") : (index>2 ? "white" : "black")
                        }
                    }
                }
                Item {
                    id:rightItem
                    width:15
                    height: 15
                    z:10
                    x: (root.mapToPosition(Qt.point(90,(startinPosition - index*1)),polarSeries).x) - (width/2)
                    y: (root.mapToPosition(Qt.point(90,(startinPosition - index*1)),polarSeries).y) - (width/2)
                    Rectangle
                    {
                        anchors.fill: parent
                        radius:parent.width/2
                        color:"transparent"
                        Text{
                            anchors.centerIn: parent
                            text: index+1
                            color: gameMode ? (index>5 ? "white" : "black") : (index>2 ? "white" : "black")
                        }
                    }
                }
                Item {
                    id:bottomItem
                    width:15
                    height: 15
                    z:10
                    x: (root.mapToPosition(Qt.point(180,(startinPosition - index*1)),polarSeries).x) - (width/2)
                    y: (root.mapToPosition(Qt.point(180,(startinPosition - index*1)),polarSeries).y) - (width/2)
                    Rectangle
                    {
                        anchors.fill: parent
                        radius:parent.width/2
                        color:"transparent"
                        Text{
                            anchors.centerIn: parent
                            text: index+1
                            color: gameMode ? (index>5 ? "white" : "black") : (index>2 ? "white" : "black")
                        }
                    }
                }
            }
        }

        Repeater
        {
            id:numberOverlayRepeater
            // After a 3P match finishes, the face reviews the whole match (all six
            // series) alongside the right-panel table; during live shooting and for
            // every other discipline it stays the current per-position buffer.
            model: (is3PMatch && shootingPage.matchFinished) ? globalMatchModel : globalModelOfData
            delegate: numberOverlayDelegate

            onCountChanged: {
                mpiRect.refreshPosition()
                groupRect.refreshPosition()
            }
        }

        Component {
            id:numberOverlayDelegate //pallet circle
            Item {
                id:mainItem

                //10 Meter Pistol Ratio=155.5÷4.5=34.55
                //10 Meter Rifle Ratio=45.5÷4.5=10.11
                //50 Meter Pistol Ratio=500/5.6=89.29
                //50 Meter Rifle Ratio=154.4/5.6=27.57

                property double gameRatio: gameRange == 10 ? (gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                           : (gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())

//                width: gameRange == 10 ? (gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
//                                       : (gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
//                                                   : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)

                width: shootingcanvas.height/gameRatio
                height: width
                z: 15

                function refreshBulletPostion() {
                    innerRect.refreshBulletPOsition();
                }

                Rectangle
                {
                    id: innerRect
                    function refreshBulletPOsition() {
                        var left = Qt.point(direction*1,score*1);

                        left = root.mapToPosition(left,polarSeries);
                        mainItem.x = root.x + left.x - radius
                        mainItem.y = root.y + left.y - radius
                        //                        var distance_factor = APPSETTINGS.getMatch_meter() /*modified match distance*//10 /*10m match*/
                        //                        mainItem.x = mainItem.x/distance_factor
                        //                        mainItem.y = mainItem.y/distance_factor
                        //                        console.log("index ", index, " xcor ", mainItem.x, " ycor ", mainItem.y)
                        if((sligterMode)&&(!APPSETTINGS.getSighter_series()))
                        {

                            visible =true

                        }

                        else
                        {
                            if(((sligterMode)&&(APPSETTINGS.getSighter_series()))||(!sligterMode))
                            {
                                //Non - Slighter Mode
                                var checkIndex = currentPageIndexOfSer*10 - 1
                                if( (index > checkIndex) && (index <= (checkIndex+10)) )
                                {
                                    visible = true
                                }
                                else
                                {
                                    visible = false
                                }
                            }

                        }

                        //Check Last Item and update color
                        //                        if(index === rightPanel.currentShootIndex+1)
                        //                        {
                        //                            //Last item
                        //                            color = shootingPage.isPalletRed ? "red" : "white"
                        //                            border.color = gameMode ? (!shootingPage.isPalletRed ? "red" : "white") : ("green") //shootingPage.isPalletRed ? "red" : "white"
                        //                            opacity = 0.8
                        //                        }

                        if (!disableMotorMovement && !autoMotorMovementMode)
                            MODREADER.initiateMotorMovement()
                        else if (autoMotorMovementMode) {// to verify if automode is on or not?
                            if (globalModelOfData.count-1 === index && appMode && globalModelOfData.count == 1) // for last shot item only
                                MODREADER.checkAutoFeedMode()
                        }
                    }

                    Component.onCompleted:
                    {
                        refreshBulletPOsition();
                    }
                    anchors.fill: parent
                    radius:parent.width/2

                    //for gpu
                    color: (paneItem.currentScoreValue >= highestShoot && index === globalModelOfData.count-1) ? shootingPanelRect.highestShootColor : (index === shotCount-1 ? "red" : (index === rightPanel.currentShootIndex) ? (shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor)
                                                                                                                                                                                                                                 : (!shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor))

                    //for cpu
                    //                    color: "transparent"
                    //                    Image {
                    //                        id: selectedRectImg
                    //                        source: (index === shotCount-1 ? "qrc:/images/centerPanel/Pistol bullet_50m.png"
                    //                                                       : (index === rightPanel.currentShootIndex) ? (shootingPage.isPalletRed ? shootingPanelRect.firstImg : shootingPanelRect.secondImg)
                    //                                                                                                  : (!shootingPage.isPalletRed ? shootingPanelRect.firstImg : shootingPanelRect.secondImg))
                    //                        anchors.fill: parent
                    //                    }


                    opacity: index === rightPanel.currentShootIndex ? 0.8 : APPSETTINGS.getIsPalletTRansparent() ? 0.5 : 1
                    //                    border.width: 1
                    //                    border.color: "red"
                    Text{
                        anchors.centerIn: parent
                        visible: gameRange == 50 && gameMode ? false : true
                        // Continuous numbering through a live 3P match (K 1-20,
                        // P 21-40, S 41-60): snBase counts the completed
                        // positions' record shots straight from the data. 0 while
                        // sighting, for non-3P, and in post-match review (the
                        // model is match-absolute there already).
                        text: rightPanel.snBase + index + 1
                    }

                    //                    z: index === rightPanel.currentShootIndex ? 100 : 15

                }

                Rectangle {
                    id: hShootLine
                    anchors.centerIn: innerRect
                    color: (index === shotCount-1 ? "red" : (index === rightPanel.currentShootIndex) ? (shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor)
                                                                                                     : (!shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor))
                    width: parent.width * 7
                    height: 3
                    visible: gameMode && gameRange === 50 && innerRect.visible
                }

                Rectangle {
                    id: vShootLine
                    anchors.centerIn: innerRect
                    color: hShootLine.color
                    height: parent.height * 7
                    width: 3//shotCircle.width/2
                    visible: gameMode && gameRange === 50 && innerRect.visible
                }
            }
        }
        Rectangle {
            id: mpiRect
            width:gameRange === 50 ? hShootLine.width : 10
            height: width

            Rectangle {
                id: horizontalRect
                anchors.centerIn: parent
                height: 2
                width: parent.width
                //                y: parent.height/2 - height/2
                //                x: 0
                color: "blue"
            }

            Rectangle {
                id: verticalRect
                anchors.centerIn: parent
                height: parent.height
                width: 2

                //                x: parent.width/2 - width/2
                //                y: 0
                color: "blue"
            }

            function refreshPosition() {
                console.log("refreshPosition")
                // Mean point of impact over the shots currently DISPLAYED —
                // the backend lists span all positions in a 3P match, which
                // put this marker at a stale mixed-position average.
                var xCor = 0
                var yCor = 0
                if (globalModelOfData.count > 0) {
                    for (var i = 0; i < globalModelOfData.count; ++i) {
                        xCor += globalModelOfData.get(i).xmm*1
                        yCor += globalModelOfData.get(i).ymm*1
                    }
                    xCor /= globalModelOfData.count
                    yCor /= globalModelOfData.count
                }

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingMianRect.width/shootingWidth
                var offsetY = shootingMianRect.height/shootingHeight

                var centerX = shootingPanelRect.width/2 //* offset
                var centerY = shootingPanelRect.height/2 //* offset

                var bulletSize = 0//4.5/2

                x = centerX+((xCor+bulletSize)*offsetX) - radius
                y = centerY-((yCor+bulletSize)*offsetY) - radius

                //10 Meter Pistol Ratio=155.5÷4.5=34.55
                //10 Meter Rifle Ratio=45.5÷4.5=10.11
                //50 Meter Pistol Ratio=500/5.6=89.29
                //50 Meter Rifle Ratio=154.4/5.6=27.57

                var gameRatio = gameRange == 10 ? (gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                : (gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())

//                width: gameRange == 10 ? (gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
//                                       : (gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
//                                                   : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)

                width = shootingcanvas.height/gameRatio
                height = width

                // mpi text
                mpiText.text = qsTr("X:") + xCor.toFixed(2)+qsTr(", Y:")+yCor.toFixed(2)
            }

            Component.onCompleted:
            {
                refreshPosition()

            }
            radius: width/2
            color: "transparent" //mpiColor
            opacity: 0.5
            visible: leftPanel.isShowMPI && numberOverlayRepeater.count > 1
            z: 110
        }

        // group circle
        Rectangle {
            id: groupRect
            width:10
            height: width
            //visible: true

            function refreshPosition() {
                console.log("group refreshPosition", rightPanel.currentPageIndex)
                // Group over the shots currently DISPLAYED (see mpiRect) —
                // widest pair of shots, circle centred between them.
                var g = 0
                var xCor = 0
                var yCor = 0
                for (var i = 0; i < globalModelOfData.count; ++i) {
                    var ei = globalModelOfData.get(i)
                    for (var j = i + 1; j < globalModelOfData.count; ++j) {
                        var ej = globalModelOfData.get(j)
                        var d = Math.sqrt(Math.pow(ei.xmm*1 - ej.xmm*1, 2)
                                        + Math.pow(ei.ymm*1 - ej.ymm*1, 2))
                        if (d > g) {
                            g = d
                            xCor = (ei.xmm*1 + ej.xmm*1) / 2
                            yCor = (ei.ymm*1 + ej.ymm*1) / 2
                        }
                    }
                }
                group_distance = globalModelOfData.count > 1 ? g + APPSETTINGS.bullet_diameter() : 0
                console.log("group refreshPosition group_distance ", group_distance)

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingMianRect.width/shootingWidth
                var offsetY = shootingMianRect.height/shootingHeight

                var centerX = shootingPanelRect.width/2 //* offset
                var centerY = shootingPanelRect.height/2 //* offset

                var bulletSize = 0//4.5/2

                x = centerX+((xCor+bulletSize)*offsetX) - radius
                y = centerY-((yCor+bulletSize)*offsetY) - radius

                // for 10m
                // pistol -> target size = 155.5 and bulletSize = 4.5
                // rifle -> target size = 45.5 and bulletSize = 4.5
                // for 50m
                // pistol -> target size = 500 and bulletSize = 5.6
                // rifle -> target size = 154.4 and bulletSize = 5.6
                width = gameRange == 10 ? (centerPanel.gameMode ? shootingcanvas.height/155.5 : shootingcanvas.height/45.5 )
                                        : (centerPanel.gameMode ? shootingcanvas.height/500 /*size 500 pallet 5.6*/
                                                                : shootingcanvas.height/154.4 /*size 154.4 pallet 5.6*/)

                var gameRatio = gameRange == 10 ? (gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                : (gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())

//                var palletSize = gameRange == 10 ? (centerPanel.gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
//                                                 : (centerPanel.gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
//                                                                         : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)
                var palletSize = shootingcanvas.height/gameRatio
                width = width*distance + palletSize
                height = width

                //
                var org_palletSize = gameRange == 10 ? 4.5 : 5.6
                var group_distance_text = group_distance + org_palletSize // alread added in TachusWidget::getGroup
                groupText.text = qsTr("Group: ") + group_distance_text.toFixed(2) + qsTr(" mm")
                if (group_distance == 0)
                    groupRect.visible = false
                else if (leftPanel.isShowMPI && numberOverlayRepeater.count > 1 && !sighter.visible)
                    groupRect.visible = true

                //
                visible = leftPanel.isShowMPI && numberOverlayRepeater.count > 1 && !sighter.visible && (numberOverlayRepeater.count%shootsPerSeries != 1)
            }

            Component.onCompleted:
            {
                refreshPosition()
            }
            radius: width/2
            color: "transparent"
            //opacity: 0.5
            visible: /*false //*/leftPanel.isShowMPI && numberOverlayRepeater.count > 1 && !sighter.visible && (shotCount%shootsPerSeries != 1)
            z: 110
            border.color: "red"
        }
        // end group circle
        // selected index item
        Rectangle {
            id: selectedRect
            width:10
            height: width

            function refreshPosition() {
                // Coordinates come from the SAME record the face is paging
                // (never the backend lists — display index N doesn't map to
                // backend shot N once 3P filters per position). During live
                // shooting that's the per-position display buffer; in 3P
                // post-match review it's the full match record, because
                // currentShootIndex is match-absolute then. Reading the wrong
                // one left this marker stuck at a stale position with the
                // page's shot number — a "phantom shot" on every series page.
                var mdl = (is3PMatch && shootingPage.matchFinished) ? globalMatchModel : globalModelOfData
                var idx = rightPanel.currentShootIndex
                if (idx < 0 || idx >= mdl.count)
                    return
                var e = mdl.get(idx)
                var xCor = e.xmm*1
                var yCor = e.ymm*1

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingMianRect.width/shootingWidth
                var offsetY = shootingMianRect.height/shootingHeight

                var centerX = shootingPanelRect.width/2 //* offset
                var centerY = shootingPanelRect.height/2 //* offset

                var bulletSize = 0//4.5/2

                x = centerX+((xCor+bulletSize)*offsetX) - radius
                y = centerY-((yCor+bulletSize)*offsetY) - radius

//                width = gameRange == 10 ? (centerPanel.gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
//                                        : (centerPanel.gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
//                                                                : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)
                var gameRatio = gameRange == 10 ? (gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                : (gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())

                width = shootingcanvas.height/gameRatio
                height = width
            }

            Component.onCompleted:
            {
                refreshPosition()
            }

            // for gpu
            color: paneItem.currentScoreValue >= highestShoot ? shootingPanelRect.highestShootColor : shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor //shootingPanelRect.firstColor

            // for cpu
            //            color: "transparent" //shootingPage.isPalletRed ? shootingPanelRect.firstColor : shootingPanelRect.secondColor //shootingPanelRect.firstColor
            //            Image {
            //                id: selectedRectImg
            //                source: shootingPage.isPalletRed ? shootingPanelRect.firstImg : shootingPanelRect.secondImg
            //                anchors.fill: parent
            //            }

            Text{
                anchors.centerIn: parent
                visible: gameRange == 50 && gameMode ? false : true
                // Same continuous live-3P numbering as the shot overlay labels.
                text: rightPanel.snBase + rightPanel.currentShootIndex + 1
            }

            radius: width/2
            visible: globalModelOfData.count <= 1 ? false : true
            //            border.width: 1
            //            border.color: "red"
            z: 100
        }

        //////
    }

    MouseArea{
        anchors.fill:parent
        onClicked:
        {
            if (!appMode) // for demo
            {
                // 3P FINAL (Phase B): demo clicks flow through the REAL
                // detection + scoring pipeline (uxShoot -> calculateShootingSocre
                // -> pointAddedToSeries); the finals branch lives at the
                // registration junction in ShootingPage. No shortcut here.

                if (!shootingPanelRect.visible)
                    return

                if(matchFinished)
                {
                    matchInfoDialog.visible = true
                    return
                }

                // Synchronous cap: the auto-finish watcher polls every 500ms,
                // so matchFinished can lag the 60th shot — a click in that
                // window registered a 61st shot. Check the record directly.
                if (!sligterMode && shootingPage.matchShootCount > 0
                        && globalMatchModel.count >= shootingPage.matchShootCount)
                    return

                var logData = "canvas screen clicked position x ->"+ mouseX+ " y -> " + mouseY
                MODREADER.appendToLogFile(logData)
                console.log("--x--", mouseX, "--y--", mouseY,"mapped points --- ", mapToItem(shootingcanvas, mouseX, mouseY))

                var xPoint = mapToItem(shootingPanelRect, mouseX, mouseY).x
                var yPoint = mapToItem(shootingPanelRect, mouseX, mouseY).y
                console.log("x---- ", xPoint, " y---- ", yPoint)
                logData = "canvas mapped screen clicked position x ->"+ xPoint+ " y -> " + yPoint
                MODREADER.appendToLogFile(logData)

                var centerX = shootingPanelRect.width/2
                var centerY = shootingPanelRect.height/2

                var xCor = xPoint - centerX
                var yCor = centerY - yPoint
                logData = "Real point with respect to center as origin position x ->"+ xPoint+ " y -> " + yPoint
                MODREADER.appendToLogFile(logData)

                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                var shootingWidth = gameMode ? pistalWidthHeight : rifleWidthHeight
                var shootingHeight = gameMode ? pistalWidthHeight : rifleWidthHeight

                var offsetX = shootingWidth/shootingMianRect.width
                var offsetY = shootingHeight/shootingMianRect.height


                var bulletSize = 0//4.5/2

                var mappedX = (xCor+bulletSize)*offsetX //mouseX/shootingPanelRect.scale + (mapToItem(root,0,0).x)
                var mappedY = (yCor+bulletSize)*offsetY //mouseY/shootingPanelRect.scale + (mapToItem(root,0,0).y)

                logData = "Real point with respect to center as origin after offset been applied x ->"+ xPoint+ " y -> " + yPoint
                MODREADER.appendToLogFile(logData)

                MODREADER.uxShoot(mappedX, mappedY)
                //                shootingTimer.start()
            }
        }
    }

    Image {
        id: sighter
        source: "qrc:/images/centerPanel/corner-tri.png"
        anchors.top: parent.top
        anchors.right: parent.right
        // 3P FINAL: qualification sighter-corner indicator hidden — the HUD
        // strip shows SIGHTING/MATCH state.
        visible: !shootingPage.isFinalsMatch

        width: 0.2*parent.width
        height: width

    }

    // Phase stepper: where the athlete is in the ISSF sequence. 3P shows the
    // full position chain; other disciplines just sighting -> match.
    // Hidden on the target — the ShootingPage header strip now carries the
    // phase/position stepper, so this is redundant here.
    Row {
        id: phaseStepper
        visible: false
        anchors.top: parent.top
        anchors.topMargin: 58
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 6
        z: 30
        property var steps: is3PMatch ? [qsTr("SIGHT"), qsTr("KNEELING"), qsTr("PRONE"), qsTr("STANDING")]
                                      : [qsTr("SIGHTING"), qsTr("MATCH")]
        property int cur: {
            if (matchFinished) return 99
            if (is3PMatch)
                return (sligterMode && globalMatchModel.count === 0) ? 0 : 1 + p3Position
            return sligterMode ? 0 : 1
        }
        Repeater {
            model: phaseStepper.steps.length
            delegate: Row {
                spacing: 6
                Rectangle {
                    radius: 9; height: 18
                    width: stepText.implicitWidth + 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: index < phaseStepper.cur ? "#1d7a2f"
                         : (index === phaseStepper.cur ? "#e8003d" : "#4a4b52")
                    Text {
                        id: stepText
                        anchors.centerIn: parent
                        text: phaseStepper.steps[index]
                        color: "white"; font.pixelSize: 8
                        font.bold: true; font.letterSpacing: 1
                    }
                }
                Rectangle {
                    visible: index < phaseStepper.steps.length - 1
                    width: 10; height: 2; color: "#8a8b90"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // 3P: prominent resume control for mid-match position changes. Uses its
    // own signal into ShootingPage instead of the legacy play button, which
    // stops responding after a position transition.
    Rectangle {
        id: resumePositionBtn
        // Superseded by the fixed bottom action bar in ShootingPage (redesign);
        // the bar's start button drives the same resume path.
        visible: false
        // Top-centre below the phase/clock row, above everything else: the
        // top-right corner is occupied by the sighter corner triangle and
        // z:20 notification overlays.
        z: 30
        anchors.top: parent.top
        anchors.topMargin: 86
        anchors.horizontalCenter: parent.horizontalCenter
        width: resumeText.implicitWidth + 40
        height: 44
        radius: 8
        color: "#e8003d"
        Text {
            id: resumeText
            anchors.centerIn: parent
            text: qsTr("START ") + p3Names[p3Position] + "  →"
            color: "white"
            font.bold: true
            font.pixelSize: 16
        }
        MouseArea {
            anchors.fill: parent
            // Qualified via the root id: unqualified root-member lookup from
            // nested handlers fails silently under the compiled-QML cache.
            onClicked: paneItem.positionResumeRequested()
        }
    }

    // Phase indicator: which ISSF phase (and 3P position) the athlete is in.
    // Hidden on the target — the ShootingPage header strip shows the phase now;
    // only the countdown clock remains on the target face.
    Text {
        id: phaseIndicator
        anchors.verticalCenter: slighterTimeUpdate.verticalCenter
        // Follow whichever clock is showing — the match clock shifts left of
        // the sighter indicator when that is visible.
        anchors.right: timerNotification.visible ? timerNotification.left : slighterTimeUpdate.left
        anchors.rightMargin: 8
        text: phaseText
        color: "red"
        font.bold: true
        font.pixelSize: 20
        visible: false
    }

    Rectangle
    {
        id:timerNotification
        anchors.top: parent.top
        anchors.right: sighter.visible ? sighter.left : parent.right
        anchors.topMargin: 20
        width: 0.2*parent.width
        height: 40
        // 3P FINAL: the qualification match clock never shows — the Finals
        // HUD strip is the only time source (FINALS3P.remainingFormatted).
        visible: APPSETTINGS.timer() && !shootingPage.isFinalsMatch
        color: "transparent"

        Row {
            id:itemsRow
            anchors.fill: parent
            layoutDirection: sighter.visible ? Qt.RightToLeft : Qt.LeftToRight

            Image {
                id: clockBGImage
                source:  APPSETTINGS.timer()?"qrc:/images/centerPanel/clock.png": "qrc:/images/centerPanel/multiwhite_round.png"
                fillMode: Image.Stretch
            }
            Text {
                id: stopTimer
                //                anchors.top: parent.top
                //                anchors.right: parent.right
                //                width: 0.2*parent.width
                //                height: width
                //                visible: !sighter.visible
                text : "00:00"
                font.pixelSize: 0.65*clockBGImage.sourceSize.height
                horizontalAlignment: Text.AlignHCenter
                visible: true
                color: "red"

                Component.onCompleted:
                {
                    var remainingTime = (totalGameTime - gameTime)*1
                    var formatedTime = minutesToseconds(remainingTime)
                    stopTimer.text = formatedTime
                }

                onVisibleChanged: {
                    if (visible == false) {
                        gameTime = 0;
                        gameTimer.stop()
                    } else {
                        var remainingTime = (totalGameTime - gameTime)*1
                        var formatedTime = minutesToseconds(remainingTime)
                        stopTimer.text = formatedTime
                        gameTimer.start()
                    }
                }
            }
        }
    }

    Text {
        id: demoText
        text: qsTr("(Demo)")
        anchors.top: countText.top
        height: countText.height
        //anchors.topMargin: 10 //itemsRow.height

        anchors.left: countText.right
        //anchors.leftMargin: 10

        font.pixelSize: (0.5*finalShootNotificationRect.height)

        visible:!appMode
        color: "red"
    }

    Text {
        id: countText
        text: (globalModelOfData.count < 10) ? ("00"+globalModelOfData.count) :
                                               (globalModelOfData.count > 99 ?
                                                    globalModelOfData.count : "0"+globalModelOfData.count)

        width: 50
        height: 40

        anchors.bottom: finalShootNotificationRect.top
        anchors.left: parent.left
        anchors.leftMargin: 10
        font.pixelSize: (0.5*finalShootNotificationRect.height)
        color: "red"
    }

    Rectangle
    {
        id:slighterTimeUpdate
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 20
        width: 0.2*parent.width
        height: 40
        color: "transparent"
        // Show the sighting countdown whenever the match clock is hidden —
        // it takes the match clock's top-right spot (they are mutually
        // exclusive), keeping clear of the shot counter at top-left.
        // 3P FINAL: no qualification clocks at all (FINALS3P HUD is the only
        // time source), so this must not appear when the match clock hides.
        visible: !timerNotification.visible && !shootingPage.isFinalsMatch

        Row {
            id:stRow
            anchors.fill: parent
            Image {
                id: stBGImage
                source: APPSETTINGS.timer()?"qrc:/images/centerPanel/clock.png": "qrc:/images/centerPanel/multiwhite_round.png"
                fillMode: Image.Stretch
            }
            Text {
                id: stStopTimer
                //                anchors.top: parent.top
                //                anchors.right: parent.right
                //                width: 0.2*parent.width
                //                height: width
                //                visible: !sighter.visible
                text : "00:00"
                font.pixelSize: 0.65*clockBGImage.sourceSize.height
                horizontalAlignment: Text.AlignHCenter
                //                visible: true
                visible: false
                color: "red"

                Component.onCompleted:
                {
                    var remainingTime = (totalSighterTime - sighterTime)*1
                    var formatedTime = minutesToseconds(remainingTime)
                    stStopTimer.text = formatedTime
                }

                //                onVisibleChanged: {
                //                    if (visible == false) {
                //                        gameTime = 0;
                //                        gameTimer.stop()
                //                    } else {
                //                        var remainingTime = (totalGameTime - gameTime)*1
                //                        var formatedTime = minutesToseconds(remainingTime)
                //                        stopTimer.text = formatedTime
                //                        gameTimer.start()
                //                    }
                //                }
            }
        }
    }


    Rectangle {
        id: finalShootNotificationRect
        width: 300
        height: 50
        z: 20
        color: "red"
        opacity: 0.7

        visible: (shotCount != -1) && (globalModelOfData.count === (shotCount-1)) && APPSETTINGS.getNotificationForLastShot()

        onVisibleChanged: {
            showAnimation.start()
        }

        anchors.top: parent.top
        anchors.topMargin: 50

        Text {
            anchors.centerIn: parent
            text: qsTr("Final shot of the match")
            font.pixelSize: (0.5*parent.height)
            color: "white"
        }

        NumberAnimation on x {
            id: showAnimation
            from: -parent.width; to: 0
            duration: 500
        }
    }

    Image {
        id: currentScoreBGImage
        source: "qrc:/images/centerPanel/Circle.png"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10

        width: 0.15*parent.width
        height: width

        rotation: (paneItem.currentScoreDegree === -1 || paneItem.currentScoreValue === -1) ? -45 : paneItem.currentScoreDegree - 45
        transformOrigin: Item.Center
    }

    Text {
        id: currentScoreText
        width: implicitWidth
        height: 0.6*currentScoreBGImage.height
        font.pixelSize: (0.65*height)
        color: "red"
        anchors.centerIn: currentScoreBGImage
        text: paneItem.currentScoreValue === -1 ? "" : paneItem.currentScoreValue
    }

    Rectangle {
        id: zoomRect
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 10
        color: "transparent"

        width: 100
        height: 50

        visible:APPSETTINGS.getShowZoomButton()

        Image {
            id: zoomOut
            source: "qrc:/images/centerPanel/zoomOut.png"
            anchors.top: parent.top
            width: parent.width/2
            height: parent.height
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("*****************", shootingPanelRect.scale)
                    if (shootingPanelRect.scale > 1.1)
                        shootingPanelRect.scale -= zoom_offset
                }
            }
        }

        Image {
            id: zoomIn
            source: "qrc:/images/centerPanel/zoomIn.png"
            anchors.left: zoomOut.right
            anchors.leftMargin: 10

            width: parent.width/2
            height: parent.height

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("*****************", shootingPanelRect.scale)
                    if (shootingPanelRect.scale < 3.5)
                        shootingPanelRect.scale += zoom_offset
                }
            }
        }
    }

    Rectangle {
        id: mpiTextRect
        color: "transparent"
        width: 150
        height: 20
        anchors.left: zoomRect.right
        anchors.leftMargin: 20
        anchors.bottom: zoomRect.bottom
        anchors.bottomMargin: 10
        visible: mpiRect.visible

        Text {
            id: mpiText
            text: qsTr("X:") + MODREADER.getXMPI()+qsTr(", Y:")+MODREADER.getYMPI()
            anchors.left: parent.left
            width: implicitWidth
            height: implicitHeight
            anchors.verticalCenter: parent.verticalCenter
            color: "red"
            font.pixelSize: 14
            font.bold: true
        }
    }

    Rectangle {
        color: "transparent"
        width: 150
        height: 20
        anchors.left: zoomRect.right
        anchors.leftMargin: 20
        anchors.bottom: mpiTextRect.top
        visible: groupRect.visible

        Text {
            id: groupText
            text: qsTr("Group: ") + MODREADER.getGroup(rightPanel.currentPageIndex)+qsTr(" mm")
            anchors.left: parent.left
            width: implicitWidth
            height: implicitHeight
            anchors.verticalCenter: parent.verticalCenter
            color: "red"
            font.pixelSize: 14
            font.bold: true
        }
    }

    ConnectionError {
        id: conErrorDia
        z: 10
        height: parent.height// - header.height
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: false
        bgColor: "red"
    }

    Rectangle {
        id: errorRect
        anchors.fill: parent
        color: "green"
        visible: false
        opacity: 0.5
    }

    function addIfinRange(shootPoint)
    {
        if (!root.visible)
            return

        console.log("addIfinRange x ", shootPoint.x, " y ", shootPoint.y)
        //        if(shootPoint.x >= axisAngular.min && shootPoint.x <= axisAngular.max
        //                && shootPoint.y >= axisRadial.min && shootPoint.y <=axisRadial.max )
        //        {
        currentPolarSeriesItem.clear()
        polarSeries.append(shootPoint.x, shootPoint.y)
        currentPolarSeriesItem.append(shootPoint.x, shootPoint.y)
        //MODREADER.initiateMotorMovement()
        //        }
    }
    function circleCordinates()
    {
        if (!paneItem.visible)
            return;

        if (gameMode)
        {
            var left = Qt.point(270,4);
            left = root.mapToPosition(left,polarSeries);
            var right = Qt.point(90,4);
            right = root.mapToPosition(right,polarSeries);
            var top = Qt.point(360,4);
            top = root.mapToPosition(top,polarSeries);
            var bottom = Qt.point(180,4);
            bottom = root.mapToPosition(bottom,polarSeries);

            innercircle.x = left.x
            innercircle.y = top.y
            innercircle.width = right.x - left.x
        } else {
            var left1 = Qt.point(270,6);
            left1 = root.mapToPosition(left1,polarSeries);
            var right1 = Qt.point(90,6);
            right1 = root.mapToPosition(right1,polarSeries);
            var top1 = Qt.point(360,6);
            top1 = root.mapToPosition(top1,polarSeries);
            var bottom1 = Qt.point(180,6);
            bottom1 = root.mapToPosition(bottom1,polarSeries);

            innercircle.x = left1.x
            innercircle.y = top1.y
            innercircle.width = right1.x - left1.x
        }

        innercircle.height = innercircle.width
        innercircle.radius = innercircle.width/2

        scoreIndicator.model = null
        scoreIndicator.model = 8
    }

    function currentPageIndexChanged()
    {
        numberOverlayRepeater.model = null
        numberOverlayRepeater.model = (is3PMatch && shootingPage.matchFinished) ? globalMatchModel : globalModelOfData
    }

    function refreshCentralPanelPage()
    {
        shootingPanelRect.scale = 1
        autoZoomHold = 0
        autoZoomFactor = 1
        currentScoreValue = -1
    }

    function showSlighter(sighterVisible)
    {
        sighter.visible = sighterVisible
        if(!sighterVisible) {
            paneItem.currentScoreValue = -1
            gameTimer.restart()
        } else
            gameTimer.stop()
    }

    // Toggle only the sighter indicator, leaving the match clock untouched —
    // used for 3P position changes where ISSF counts the changeover and
    // sighting time as part of the overall match time.
    function setSighterIndicator(sighterVisible)
    {
        sighter.visible = sighterVisible
    }

    function stopMatchClock()
    {
        gameTimer.stop()
        sighterTimer.stop()
    }

    function calculateShootingSocre(xPoint, yPoint, demoXPoint, demoYPoint)
    {
        lastShotXmm = xPoint
        lastShotYmm = yPoint
        var logData = "calculateShootingSocre  -- started appmode "+ appMode+ " gamerange "+gameRange+ "game mode "+ gameMode
        MODREADER.appendToLogFile(logData)

        if (1) {
            var logData0 = ("live match new formula xpoint "+ xPoint+ " yPoint "+ yPoint)
            MODREADER.appendToLogFile(logData0)
            var distance_factor = APPSETTINGS.getMatch_meter() /*modified match distance*//10 /*10m match*/
            var dis_factor_log = ("distance factor "+ distance_factor)
            MODREADER.appendToLogFile(dis_factor_log)

            var cont_factor = 2.25 - (2.25*distance_factor)
            var const_factor_log = ("constant factor "+ cont_factor)
            MODREADER.appendToLogFile(const_factor_log)

            var modified_xPoint = xPoint/distance_factor
            var modified_yPoint = yPoint/distance_factor
            var mod_points_log = ("modified x "+ modified_xPoint +" modified y "+modified_yPoint)
            MODREADER.appendToLogFile(mod_points_log)

            var mapedRadius = Math.sqrt(Math.pow(modified_xPoint, 2)+Math.pow(modified_yPoint, 2))
            var logData1 = "sqrt(xsq+ysq) " + mapedRadius
            MODREADER.appendToLogFile(logData1)

            // config bullet radius
                if (gameRange == 10)
                {
                    if (gameMode)
                    {
                        //1-For 10 Meter Air Pistol:
                        //Ring to Ring Distance=8 mm
                        //Radius of 10th Ring=5.75 mm
                        //Radius of Pellet =2.25 mm
                        //Score=9+{(8+5.75+2.25)-sqrt(x2+y2)}/8

                        var r2rDis = 8
                        var radOf10Ring = 5.75
                        var radOfPallet = APPSETTINGS.bullet_diameter()/2
                        var totalR = r2rDis+radOf10Ring+radOfPallet


                        var logData2 = "radius of pallet "+ radOfPallet + "sqrt(x2+y2)"+mapedRadius
                        MODREADER.appendToLogFile(logData2)

                        calculatedSccore = 9+((totalR - mapedRadius)/8) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        calculatedSccore = calculatedSccore + 0.00001

                        var logData111 = "10 m game for pistol -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData111)
                        if (calculatedSccore <= 0)
                            calculatedSccore = 0.001

                        var logData100 = "after zero 10 m game for pistol -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData100)
                        //                    }
                    } else {
                        //2-For 10 Meter Air Rifle:
                        //Ring to Ring Distance=2.5 mm
                        //Radius of 10th Ring=0.25 mm
                        //Radius of Pellet =2.25 mm
                        //Score=9+{(2.5+0.25+2.25)-sqrt(x2+y2)}/2.5

                        var r2rDis = 2.5
                        var radOf10Ring = 0.25
                        var radOfPallet = APPSETTINGS.bullet_diameter()/2

                        var ratio = MODREADER.getGame_distance()*1.0/MODREADER.getGame_range();
                        /*5=2.5 (distance between rings)+ 0.25 (Inner 10 radius)+2.25 (Radius of Bullet)*/

                        var palletCaculation =  r2rDis + radOf10Ring + (radOfPallet/*/ratio*/);
                        calculatedSccore = 9+((palletCaculation - mapedRadius)/2.5) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        calculatedSccore = calculatedSccore + 0.00001
                        var logData11 = "10 m game for rifle -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData11)
                        if (calculatedSccore <= 0)
                            calculatedSccore = 0.001

                        var logData00 = "after zero 10 m game for rifle -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData00)
                        //                    }
                    }
                } else if (gameRange == 50) {
                    if (gameMode)
                    {
                        //3-For 50 Meter Pistol:
                        //Ring to Ring Distance=25 mm
                        //Radius of 10th Ring=25 mm
                        //Radius of Pellet =2.8 mm
                        //Score=9+{(25+25+2.8)-sqrt(x2+y2)}/25

                        var r2rDis = 25
                        var radOf10Ring = 25
                        var radOfPallet = APPSETTINGS.bullet_diameter()/2
                        var totalR = r2rDis+radOf10Ring+radOfPallet

                        var logData2 = "radius of pallet "+ radOfPallet + "sqrt(x2+y2)"+mapedRadius
                        MODREADER.appendToLogFile(logData2)

                        calculatedSccore = 9+((totalR - mapedRadius)/25) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        calculatedSccore = calculatedSccore + 0.00001

                        var logData111 = "50 m game for pistol -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData111)
                        if (calculatedSccore <= 0)
                            calculatedSccore = 0.001

                        var logData100 = "after zero 50 m game for pistol -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData100)
                        //                    }
                    } else {
                        //3-For 50 Meter Rifle:
                        //Ring to Ring Distance=8 mm
                        //Radius of 10th Ring=5.2 mm
                        //Radius of Pellet =2.8 mm
                        //Score = 9+{(8+5.2+2.8)-sqrt(x2+y2)}/8

                        var r2rDis = 8
                        var radOf10Ring = 5.2
                        var radOfPallet = APPSETTINGS.bullet_diameter()/2
                        var totalR = r2rDis+radOf10Ring+radOfPallet

                        var ratio = MODREADER.getGame_distance()*1.0/MODREADER.getGame_range();
                        //calculatedSccore = 9+((16.06 - mapedRadius)/8) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        calculatedSccore = 9+((totalR - mapedRadius)/8)
                        calculatedSccore = calculatedSccore + 0.00001
                        var logData11 = "50 m game for rifle -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData11)
                        if (calculatedSccore <= 0)
                            calculatedSccore = 0.001

                        var logData00 = "after zero 50 m game for rifle -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData00)
                    }
                }
        } else {
            console.log("shootingcanvas ", shootingPanelRect.width, " height ", shootingPanelRect.height, " shootingMianRect ", shootingMianRect.width, " height ", shootingMianRect.height)
            var clickedX = mapToItem(shootingcanvas, demoXPoint, demoYPoint).x
            var clcikedY = mapToItem(shootingcanvas, demoXPoint, demoYPoint).y
            var canvasMidX = shootingcanvas.width/2

            console.log(shootingPanelRect.scale, " srinivas ---clickedX ", clickedX, " clickedY ", clcikedY)
            console.log("srinivas ---XPoint ", demoXPoint, " YPOINt ", demoYPoint)
            var radius = Math.sqrt(Math.pow(canvasMidX-clickedX, 2)+Math.pow(canvasMidX-clcikedY, 2))
            radius = radius * shootingPanelRect.scale

            console.log("radius ", radius, " canvasMidX ", canvasMidX)
            var mapedRadius = gameRange == 10 ? (gameMode ? (radius*155.5) / shootingcanvas.width : (radius*45.5) / shootingcanvas.width)
                                              :(gameMode ? (radius*500)/shootingcanvas.width : (radius*154.4)/shootingcanvas.width)

            if (gameRange == 10)
            {
                if (gameMode)
                { // for pistol
                    if (mapedRadius > 80)
                        calculatedSccore = 0
                    else {
                        //calculatedSccore = ((Math.floor((11 - (((mapedRadius + 0.1)/0.8)*0.1))*10))*0.1).toFixed(1)
                        calculatedSccore = 9+((16 - mapedRadius)/8) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        var logData = "10 m game for pistol -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData)
                    }
                } else { // for rifle
                    if (mapedRadius > 25)
                        calculatedSccore = 0
                    else {
                        //calculatedSccore = ((Math.floor((11 - (((mapedRadius + 0.01)/0.25)*0.1))*10))*0.1).toFixed(1)
                        calculatedSccore = 9+((5 - mapedRadius)/2.5) //https://github.com/raosrinu2004/tachus_bug_tracker/issues/77
                        var logData1 = "10 m game for rifle -> calculate score "+ calculatedSccore
                        MODREADER.appendToLogFile(logData1)
                    }
                }
            } else if (gameRange == 50) {
                if (gameMode)
                {
                    if (mapedRadius > (500+5.6)/2)
                        calculatedSccore = 0
                    else {
                        calculatedSccore = ((Math.floor((11 - (((mapedRadius + 0.1)/2.5)*0.1))*10))*0.1).toFixed(1)
                    }

                } else {
                    if (mapedRadius > (154.4+5.6)/2)
                        calculatedSccore = 0
                    else
                        calculatedSccore = ((Math.floor((11 - (((mapedRadius + 0.1)/0.8)*0.1))*10))*0.1).toFixed(1)
                }
            }
        }

        //        calculatedSccore
        // cross-verification for 0.9 values in sccore
        if (calculatedSccore < 1)
            calculatedSccore = 0

        if (calculatedSccore >= 11)
            calculatedSccore = 10.9

        if (APPSETTINGS.getMatch_meter() != 10 && calculatedSccore <= 1) {
            calculatedSccore = 1
        }

        console.log(calculatedSccore,"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", mapedRadius)
        calculatedSccore = scoreCutoffTofirstDecimal(calculatedSccore*1)

        console.log(calculatedSccore,"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", mapedRadius)
        MODREADER.setScore(calculatedSccore)
    }

    //    function minutesToseconds(totalSecs)
    //    {
    //        var minutes = Math.floor(totalSecs / 60);
    //        var seconds = totalSecs - minutes * 60;
    //        var finalTime = str_pad_left(minutes,'0',2)+':'+str_pad_left(seconds,'0',2);
    //        return finalTime
    //    }

    //    function str_pad_left(string,pad,length) {
    //        return (new Array(length+1).join(pad)+string).slice(-length);
    //    }

    function refreshSelectedShootPosition() {
        selectedRect.refreshPosition()
    }

    function startFromServer()
    {
        //sighter.visible = false;
        sighterTimer.start()
        stStopTimer.visible = true;
        gameTimer.stop()
        countText.visible = false
        timerNotification.visible = false
        //        rightPanel.startFromServer()
    }

    // ── ISSF preparation/sighting phase (standalone, no range master) ────────
    // Mirrors startFromServer(): sighting countdown runs, match clock hidden.
    // Hiding timerNotification zeroes gameTime via stopTimer.onVisibleChanged,
    // so the match clock starts from the full discipline time later.
    function startPreparationCountdown()
    {
        MODREADER.appendToLogFile("startPreparationCountdown: totalSighterTime=" + totalSighterTime)
        sighterTime = 0
        stStopTimer.text = minutesToseconds(totalSighterTime)
        stStopTimer.visible = true
        timerNotification.visible = false
        sighterTimer.restart()
    }

    // Called when the match phase begins (play button or sighting timer expiry).
    // Idempotent: safe on later pause/resume cycles.
    function stopPreparationCountdown()
    {
        sighterTimer.stop()
        timerNotification.visible = APPSETTINGS.timer()
    }

}
