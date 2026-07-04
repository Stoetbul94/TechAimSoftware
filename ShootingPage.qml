import QtQuick 2.15
import QtQuick.Dialogs
import QtQuick.Controls 2.15

//import Qt.labs.platform 1.0


Item {
    id: shootingPage

    visible: false

    property int matchShootCount: -1
    property alias currentGameDisplay1: leftPanel.gameDisplay1
    property alias currentGameDisplay2: leftPanel.gameDisplay2
    property alias currentmatchDisplay: leftPanel.matchDisplay
    property alias totalScore : rightPanel.grandTotal
    property alias totalScoreWithoutDecimal: rightPanel.grandTotalExculdeDec
    property alias totalTime: rightPanel.totalTimeConsume

    property alias isBackgroudBlack: settingsPage.isBackGroundBlack
    property alias isPalletRed: settingsPage.isPalletRedColor
    property alias currentPageIndexOfSer: rightPanel.currentPageIndex

    property bool sligterMode: true

    property bool matchFinished :false

    // ── 50m Rifle 3 Positions (ISSF 3x20 qualification) ──────────────────────
    // Position is derived from the match-shot count: 0-19 kneeling, 20-39
    // prone, 40-59 standing. The overall 105-min match clock runs through
    // position changes; only shot tagging switches to sighter during them.
    property bool is3PMatch: false
    // Highest shot-count boundary (20/40) already handled — without it the
    // watcher re-fires after the athlete resumes, because the count is still
    // exactly at the boundary, bouncing them straight back into sighting.
    property int p3BreaksDone: 0
    readonly property var p3Names: [qsTr("KNEELING"), qsTr("PRONE"), qsTr("STANDING")]
    readonly property int p3Position: globalMatchModel.count < 20 ? 0
                                    : (globalMatchModel.count < 40 ? 1 : 2)

    property string phaseDebug: ""

    // Shown next to the countdown clock (CenterPane phaseIndicator).
    property string phaseText: {
        if (matchFinished)
            return qsTr("MATCH COMPLETE")
        var phase = sligterMode ? qsTr("SIGHTING") : qsTr("MATCH")
        return (is3PMatch ? phase + " · " + p3Names[p3Position] : phase) + phaseDebug
    }

    property string messageText: "Match is completed, restart to stimulate"
    property string sighterSummaryText: "You are in Sighter. You can't generate Match Summary"
    property string sighterMatchText: "You are in Sighter. You can't generate Match Report"
    property string minimumShotsSummary: "Minimum 10 shots required to generate Summary"
    property string minimumShotsMatchReport: "Minimum 10 shots required to generate Match Report"

    onTotalScoreChanged: {
        console.log("Total score changed ............................"+ totalScore)

    }

    MessageDialog
    {
        id: matchInfoDialog
        text: messageText
        visible: false
    }


    Rectangle {
        id:settingsMask
        anchors.fill:parent
        color: "transparent"
        visible: false
        z:100
        MouseArea
        {
            id:parentMouseArea
            anchors.fill: parent
            onClicked: {
                settingsMask.visible = false
            }
        }

        SettingsPage
        {
            id:settingsPage
            x:leftPanel.settingsX + leftPanel.settingsWidth
            y:leftPanel.settingsY

            onIsBackGroundBlackChanged: {
                settingsMask.visible = false
            }
            onIsPalletRedColorChanged: {
                settingsMask.visible = false
            }
        }
    }


    Dialog
    {
        id:matchFinishConfirmation
        width: 200//parent.width*0.2
        height: 75//parent.height*0.2
        Label{
            text: "Are you sure you want to finish the match ?"
            anchors.centerIn: parent
        }

        standardButtons: StandardButton.Ok | StandardButton.Cancel

        onAccepted: {
            changedToMatchFinish()
        }
    }

    MessageDialog
    {
        id: matchNotStarted
        title: "Warning"
        text: "Match Not Started"
        visible: false
    }



    MessageDialog
    {
        id: cannotGenerate
        text: sighterSummaryText
        title: "Warning"
        visible: false
    }

    ListModel
    {
        id:globalModelOfData
        onCountChanged: {
            centerPanel.disableMotorMovement = false
            centerPanel.currentPageIndexChanged()
            console.log("globalModelOfData count changes ", count)
        }
    }

    ListModel
    {
        id:globalSlighterModel
        onCountChanged: {
            console.log("******globalSlighterModel****"+count)
        }
    }

    ListModel
    {
        id:globalMatchModel
        onCountChanged: {
            console.log("******globalMatchModel****"+count)
        }
    }

    function loadGameInMatchMode() {
        is3PMatch = false   // restored sessions bypass beginPreparationPhase
        rightPanel.startClickedThroughLoad()
        sligterMode = false
        centerPanel.showSlighter(false)
    }

    function setCurrentGameType(index)
    {
        console.log("setCurrentGameType  ", index)
        if (gameRange === 10)
        {
            // if 15 shoots
            if (APPSETTINGS.getIs15Shoot()) {
                if (index >= game10RangeEventModel_15.count)
                    return

                matchShootCount = game10RangeEventModel_15.get(index).count
                currentGameDisplay1 = game10RangeEventModel_15.get(index).gameDisplay1
                currentGameDisplay2 = game10RangeEventModel_15.get(index).gameDisplay2
                currentmatchDisplay = game10RangeEventModel_15.get(index).matchDisplay
            } else {
                if (index >= game10RangeEventModel.count)
                    return

                matchShootCount = game10RangeEventModel.get(index).count
                currentGameDisplay1 = game10RangeEventModel.get(index).gameDisplay1
                currentGameDisplay2 = game10RangeEventModel.get(index).gameDisplay2
                currentmatchDisplay = game10RangeEventModel.get(index).matchDisplay
            }
        } else if (gameRange === 50) {
            // if 15 shoots
            if (APPSETTINGS.getIs15Shoot()) {
                if (index >= game50RangeEventModel_15.count)
                    return

                matchShootCount = game50RangeEventModel_15.get(index).count
                currentGameDisplay1 = game50RangeEventModel_15.get(index).gameDisplay1
                currentGameDisplay2 = game50RangeEventModel_15.get(index).gameDisplay2
                currentmatchDisplay = game50RangeEventModel_15.get(index).matchDisplay
            } else {
                if (index >= game50RangeEventModel.count)
                    return

                matchShootCount = game50RangeEventModel.get(index).count
                currentGameDisplay1 = game50RangeEventModel.get(index).gameDisplay1
                currentGameDisplay2 = game50RangeEventModel.get(index).gameDisplay2
                currentmatchDisplay = game50RangeEventModel.get(index).matchDisplay
            }

    }
    }

    onVisibleChanged: {
        if (visible) {
            centerPanel.circleCordinates()
            rightPanel.resetTimer()
        }
        if (!isSaveGame)
            MODREADER.removeSetaLaneShootDataFile()
    }

    onMatchShootCountChanged: {
        centerPanel.shotCount = matchShootCount
//        console.log(APPSETTINGS.getTimeCount(matchShootCount)," Match Shoot count is ",matchShootCount)
        centerPanel.totalGameTime = APPSETTINGS.getTimeCount(matchShootCount)
        MODREADER.setCurrentMatchTotalShotsCount(matchShootCount)
    }

    // Force the match timer to re-read getTimeCount even when the shot count is
    // unchanged (e.g. Prone <-> 3 Positions both = 60 shots) so the countdown
    // reflects the current discipline's official time.
    function refreshMatchTime() {
        centerPanel.totalGameTime = APPSETTINGS.getTimeCount(matchShootCount)
        centerPanel.shotCount = matchShootCount
        MODREADER.setCurrentMatchTotalShotsCount(matchShootCount)
    }

    // ── Top status strip (rebrand phase 1) ───────────────────────────────
    Rectangle {
        id: statusStrip
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 42
        color: "#15161a"
        z: 40

        Row {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            Text {
                text: currentGameDisplay1 + " " + currentGameDisplay2
                color: "white"; font.family: theme.fontFamily
                font.pixelSize: 14; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle { width: 1; height: 18; color: "#3a3b40"; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: currentmatchDisplay
                color: "#9a9ba0"; font.family: theme.fontFamily; font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle { width: 1; height: 18; color: "#3a3b40"; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: window.userName
                color: "#9a9ba0"; font.family: theme.fontFamily; font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            Text {
                text: globalMatchModel.count + " / " + (matchShootCount > 0 ? matchShootCount : "—")
                color: "white"; font.family: theme.fontFamily
                font.pixelSize: 14; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: qsTr("SHOTS")
                color: "#9a9ba0"; font.family: theme.fontFamily
                font.pixelSize: 9; font.letterSpacing: 1.5
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                radius: 4; height: 24; width: phaseChipText.implicitWidth + 20
                anchors.verticalCenter: parent.verticalCenter
                color: matchFinished ? "#1d7a2f" : (sligterMode ? "#8a6d00" : "#e8003d")
                Text {
                    id: phaseChipText
                    anchors.centerIn: parent
                    text: phaseText
                    color: "white"; font.family: theme.fontFamily
                    font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
                }
            }
            Rectangle {
                visible: !appMode
                radius: 4; height: 24; width: demoChipText.implicitWidth + 16
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"; border.color: "#e8003d"; border.width: 1
                Text {
                    id: demoChipText
                    anchors.centerIn: parent
                    text: qsTr("DEMO")
                    color: "#e8003d"; font.family: theme.fontFamily
                    font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
                }
            }
        }
    }

    LeftPanel {
        id: leftPanel
        width: 0.15*parent.width
        height: parent.height - statusStrip.height
        anchors.left: parent.left
        anchors.top: statusStrip.bottom
        name: window.userName
        z: 10

        onHomeButtonClicked: {
            loginPage.visible = true
            resetDataModels()
        }
        onSettingsClicked:
        {
            settingsMask.visible = true
        }
    }

    RightPanel {
        id: rightPanel
        width: 0.31*parent.width
        height: parent.height - statusStrip.height
        anchors.right: parent.right
        anchors.top: statusStrip.bottom
        z: 10
        onSwitchToSighter:
        {
            if(sighterEnable)
            {
                changedToSigherMode()
            }
            else
            {
                changedToMatchMode()
            }
        }

        onMatchFinished: {
            changedToMatchFinish()
        }
    }

    CenterPane {
        id: centerPanel
        width: parent.width - leftPanel.width - rightPanel.width
        height: parent.height - statusStrip.height
        anchors.left: leftPanel.right
        anchors.right: rightPanel.left
        anchors.top: statusStrip.bottom

        property alias showMesures: leftPanel.isShowMPI


        onPointAddedToSeries: {
            rightPanel.addToSeries(xPosition,yPosition,currentCalculatedScore,
                                   centerPanel.lastShotXmm, centerPanel.lastShotYmm)
            console.log("x ", xPosition, " y ", yPosition, " score ", currentCalculatedScore, " matchShootCount ", matchShootCount)

            // 3P: after the 20th and 40th match shots, switch to the next
            // position (kneeling -> prone -> standing) via a sighting break.
            // Deferred with callLater: running it inside this signal handler
            // resets models whose delegates are still on the emitting call
            // stack (shot animation / overlay repeater), which crashes.
            // 3P position rollover at 20/40 match shots is handled by
            // positionWatch — see its comment for why it must not be
            // triggered imperatively from this handler.
        }

        onSighterModeTimerEnds: {
            changedToMatchMode()

        }

        onPositionResumeRequested: {
            changedToMatchMode()
        }

        onShowMesuresChanged: {
            refreshShowMesureStatus(showMesures)
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: window.userName
            font.pixelSize: 32
            font.capitalization: Font.AllUppercase
            font.bold: true
            color: "red"
        }
    }


    SummaryPage {
        id:showSummaryPage
        visible: false
        width:parent.width*3/4
        height:parent.height*3/4

        contentWidth: parent.width*3/4
        contentHeight: parent.height*3/4

        onVisibleChanged: {
            if (visible)
                centerPanel.pauseGameTimer()
            else
                centerPanel.unPauseGameTimer()
        }
        //z: 20
    }

    MatchReport
    {
        id:matchReportPage
        visible: false
        width:parent.width*3/4
        height:parent.height*3/4

        onVisibleChanged: {
            if (visible)
                centerPanel.pauseGameTimer()
            else
                centerPanel.unPauseGameTimer()
        }
    }

    Connections {
        target: APPSETTINGS
        onPrintPDF: {
            if (leftPanel.playVisible)
                return;

            matchReportPage.isAutoPrintOn = true
            matchReportPage.visible = true
            console.log("-APPSETTINGS-----------------------------onPrintPDF--------------------------")
//            matchReportPage.printImageInNetworkPath()
        }
    }
    ConnectionError {
        id: conError
        z: 10
        height: parent.height// - header.height
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: false
    }

    function resetDataModels()
    {
        globalModelOfData.clear()
        globalSlighterModel.clear()
        globalMatchModel.clear()
        matchFinished = false
        rightPanel.resetRightPanelModels()
        centerPanel.refreshCentralPanelPage()
        centerPanel.backEndShootCount = 0
    }

    function changedToSigherMode()
    {
        centerPanel.disableMotorMovement = true
        centerPanel.showSlighter(true)
        leftPanel.enableSighterMode(true)
        globalModelOfData.clear()
        for(var index = 0; index <globalSlighterModel.count; ++index )
        {
            globalModelOfData.append(globalSlighterModel.get(index))
        }
        sligterMode = true
        MODREADER.changeSighterMode(true)
        rightPanel.updateTotal()
        centerPanel.currentPageIndexChanged()
        centerPanel.disableMotorMovement = false
        APPSETTINGS.updateStatusFeedbackFile(2)
    }

    // ISSF flow: enter the preparation/sighting phase at session start.
    // The 15-min sighting countdown runs; the match clock stays hidden and
    // zeroed until the match starts (play button or countdown expiry).
    function beginPreparationPhase()
    {
        MODREADER.appendToLogFile("beginPreparationPhase: prep seconds = " + APPSETTINGS.getPrepTimeCount())
        is3PMatch = APPSETTINGS.getGameMode() === 1
                 && APPSETTINGS.get10or50mRange() === 50
                 && APPSETTINGS.getGameSubMode() === 1
                 && matchShootCount === 60
        p3BreaksDone = 0
        centerPanel.totalSighterTime = APPSETTINGS.getPrepTimeCount()
        changedToSigherMode()
        centerPanel.startPreparationCountdown()
        MODREADER.appendToLogFile("beginPreparationPhase: done, totalSighterTime = "
                                  + centerPanel.totalSighterTime + " is3P = " + is3PMatch)
    }

    // 3P mid-match position change: shots become sighters for the new position
    // but the overall 105-min match clock keeps running (ISSF: changeover and
    // sighting time are part of the match time). The athlete presses play to
    // resume record fire in the new position.
    // Runs slightly delayed (positionChangeTimer) so the 20th/40th shot's
    // processing pipeline (animation, overlay delegates, demo feed timer) has
    // fully settled before models are swapped.
    // Declarative watcher instead of an imperatively-armed one-shot: the shot
    // pipeline delivers shots from a worker-thread emission context where
    // QTimer.restart() silently fails to arm (running reads true but the
    // timer never fires) and direct QML mutation crashes. A repeating timer
    // whose `running` is a plain binding is armed by the engine on the GUI
    // thread and is immune to all of that.
    Timer {
        id: positionWatch
        interval: 500
        repeat: true
        running: is3PMatch && !sligterMode && shootingPage.visible
        onTriggered: {
            var count = globalMatchModel.count
            if ((count === 20 || count === 40) && count > p3BreaksDone) {
                shootingPage.p3BreaksDone = count
                shootingPage.enterPositionTransition()
            }
        }
    }

    // Auto match-finish: when the final match shot of any discipline lands,
    // declare the match complete and stop the clock. matchShootCount is -1
    // for free practice, which keeps this watcher off.
    Timer {
        id: matchCompleteWatch
        interval: 500
        repeat: true
        running: !sligterMode && !matchFinished && matchShootCount > 0
                 && shootingPage.visible
        onTriggered: {
            if (globalMatchModel.count >= matchShootCount)
                shootingPage.changedToMatchFinish()
        }
    }

    function enterPositionTransition()
    {
        if (sligterMode)   // already transitioned (watcher may fire twice)
            return
        try {
            centerPanel.disableMotorMovement = true
            centerPanel.setSighterIndicator(true)
            leftPanel.enableSighterMode(true)
            globalModelOfData.clear()
            for(var index = 0; index <globalSlighterModel.count; ++index )
            {
                // Only sighters fired for the position being entered.
                if (globalSlighterModel.get(index).position !== p3Position)
                    continue
                globalModelOfData.append(globalSlighterModel.get(index))
            }
            sligterMode = true
            // Deliberately NOT calling MODREADER.changeSighterMode() here: its
            // QList swaps race against the 100ms polling worker that reads the
            // same lists and crash the app once shots exist (heap corruption).
            // Shot routing to sighter/match models is handled QML-side by
            // sligterMode in addToSeries, which is all the 3P transition needs.
            rightPanel.updateTotal()
            centerPanel.currentPageIndexChanged()
            centerPanel.disableMotorMovement = false
            leftPanel.playVisible = true
            MODREADER.appendToLogFile("3P: position change -> " + p3Names[p3Position]
                                      + " (sighting, match clock keeps running)")
        } catch (e) {
            phaseDebug = " [E:" + e + "]"
        }
    }

    function changedToMatchMode()
    {
        centerPanel.stopPreparationCountdown()
        centerPanel.disableMotorMovement = true
        centerPanel.showSlighter(false)
        leftPanel.enableSighterMode(false)
        globalModelOfData.clear()
        //console.log("**************globalMatchModel.count**************"+globalMatchModel.count)
        for(var index = 0; index <globalMatchModel.count; ++index )
        {
            // 3P: each position starts on a clean target face — only the
            // current position's shots are displayed. The full record stays
            // in globalMatchModel for the report.
            if (is3PMatch && globalMatchModel.get(index).position !== p3Position)
                continue
            globalModelOfData.append(globalMatchModel.get(index))
        }
        //console.log("***********globalModelOfData*****************"+globalModelOfData.count)
        sligterMode = false
        MODREADER.changeSighterMode(false)
        MODREADER.appendToLogFile("play: changeSighterMode returned")
        centerPanel.backEndShootCount = 0
        MODREADER.appendToLogFile("play: backEndShootCount cleared")
        APPSETTINGS.setGame_is_sighter_mode(0)
        MODREADER.appendToLogFile("play: sighter-mode flag saved")
        APPSETTINGS.updateStatusFeedbackFile(3)
        MODREADER.appendToLogFile("play: status feedback updated")
        rightPanel.updateTotal()
        MODREADER.appendToLogFile("play: updateTotal done")
        centerPanel.currentPageIndexChanged()
        MODREADER.appendToLogFile("play: page index refreshed")
        centerPanel.disableMotorMovement = false
    }

    function changedToMatchFinish()
    {
        matchFinished = true
        centerPanel.stopMatchClock()
        MODREADER.appendToLogFile("Match finished: " + globalMatchModel.count
                                  + "/" + matchShootCount + " match shots")
    }

    function minutesToseconds(totalSecs)
    {
        var minutes = Math.floor(totalSecs / 60);
        var seconds = totalSecs - minutes * 60;
        var finalTime = str_pad_left(minutes,'0',2)+':'+str_pad_left(seconds,'0',2);
        return finalTime
    }

    function str_pad_left(string,pad,length) {
        // Pad to `length`, but never truncate longer values (e.g. 104 minutes
        // for the 105-min 50m Rifle 3 Positions match must not become "04").
        var s = String(string)
        if (s.length >= length)
            return s
        return (new Array(length+1).join(pad)+s).slice(-length);
    }

    function startFromServer()
    {
//        rightPanel.startFromServer()
//        settingsPage.startFromServer()
//        leftPanel.startFromServer()
//        centerPanel.startFromServer()
    }

    function applyServerSettings(st, mt, spf,mpf)
    {
        centerPanel.totalGameTime = mt*60
        centerPanel.totalSighterTime = st*60
//        centerPanel.s
    }
}
