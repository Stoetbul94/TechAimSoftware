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

    LeftPanel {
        id: leftPanel
        width: 0.15*parent.width
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
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
        height: parent.height
        anchors.right: parent.right
        anchors.top: parent.top
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
        height: parent.height
        anchors.left: leftPanel.right
        anchors.right: rightPanel.left
        anchors.top: parent.top

        property alias showMesures: leftPanel.isShowMPI


        onPointAddedToSeries: {
            rightPanel.addToSeries(xPosition,yPosition,currentCalculatedScore)
            console.log("x ", xPosition, " y ", yPosition, " score ", currentCalculatedScore, " matchShootCount ", matchShootCount)

        }

        onSighterModeTimerEnds: {
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
        centerPanel.totalSighterTime = APPSETTINGS.getPrepTimeCount()
        changedToSigherMode()
        centerPanel.startPreparationCountdown()
        MODREADER.appendToLogFile("beginPreparationPhase: done, totalSighterTime = " + centerPanel.totalSighterTime)
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
            globalModelOfData.append(globalMatchModel.get(index))
        }
        //console.log("***********globalModelOfData*****************"+globalModelOfData.count)
        sligterMode = false
        MODREADER.changeSighterMode(false)
        //console.log("***********backEndShootCount*****************"+centerPanel.backEndShootCount)
        centerPanel.backEndShootCount = 0
        //console.log("***********backEndShootCount*****************"+centerPanel.backEndShootCount)
        APPSETTINGS.setGame_is_sighter_mode(0)
        APPSETTINGS.updateStatusFeedbackFile(3)
        rightPanel.updateTotal()
        centerPanel.currentPageIndexChanged()
        centerPanel.disableMotorMovement = false
    }

    function changedToMatchFinish()
    {
        matchFinished = true
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
