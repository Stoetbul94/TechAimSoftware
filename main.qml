import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    property bool isOpenGLEnabled: true
    property string userName: "Tachus"
    property string lane_number_text: "Lane1"
    property string eventName: shootingPage.currentGameDisplay1 + " " + shootingPage.currentGameDisplay2
    property string eventDate: "9/3/2017 4:34 PM"
    property string numberOfShots: "10"
    property string averageScore : "9.68"
    property string averageTime : "00:12"
    property int totalScore : shootingPage.totalScore
    property int totalTime: shootingPage.totalTime
    property int scWidth: Screen.width
    property int scHeight: Screen.height
    property bool isDefaultIcon: false //APPSETTINGS.getBrandName() == "tachus" ? true : false // true for tachus and false for seta
    property bool appMode: APPSETTINGS.getAppMode() // false for demo and true for live
    property bool isSaveGame: false
    property int gameRange: APPSETTINGS.get10or50mRange()   // 10 for 10m 50 for 50m
    property int shootsPerSeries: 10
    property string greenColor: "#00ff00" //"lightgreen"
    property string mpiColor: /*"transparent"//*/"blue"
    property bool isSingleDecimal: APPSETTINGS.getIsSingleDecimal()

    //property string valueString: ""

    // Shared TechAim theme - see Theme.qml. Instantiated once here so every
    // descendant page can reference `theme.xxx` without an import statement,
    // the same way isDefaultIcon/gameRange/gameMode above already work.
    Theme {
        id: theme
    }

    signal appVisiblityModeChanged(int mode)

    flags: Qt.FramelessWindowHint | Qt.Window

    function scoreCutoffTofirstDecimal(value) {
        return MODREADER.getFormatedSCore(value).toFixed(2)
    }

    ListModel
    {
        id:globalModelOfData
        onCountChanged: {
            console.log("******globalModelOfData****"+count)
        }
    }

    ListModel {
        id: game10RangeEventModel

            ListElement {
                name: qsTr("10M AIR RIFLE FREE")
                count: -1
                gameDisplay1: qsTr("10M AIR")
                gameDisplay2: qsTr("RIFLE")
                matchDisplay: qsTr("UN-LIMITED")
            }
            ListElement {
            name: qsTr("10M AIR RIFLE 10")
            count: 10
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 20")
            count: 20
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-20")
        }

        ListElement {
            name: qsTr("10M AIR RIFLE 30")
            count: 30
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 40")
            count: 40
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 60")
            count: 60
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-60")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL FREE")
            count: -1
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 10")
            count: 10
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 20")
            count: 20
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-20")
        
	        }
        ListElement {
            name: qsTr("10M AIR PISTOL 30")
            count: 30
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 40")
            count: 40
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 60")
            count: 60
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-60")

        }
    }

    ListModel {
        id: game10RangeEventModel_15

        ListElement {
            name: qsTr("10M AIR RIFLE FREE")
            count: -1
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 10")
            count: 10
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 15")
            count: 15
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-15")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 20")
            count: 20
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-20")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 30")
            count: 30
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("10M AIR RIFLE 40")
            count: 40
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL FREE")
            count: -1
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 10")
            count: 10
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 15")
            count: 15
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-15")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 20")
            count: 20
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-20")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 30")
            count: 30
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("10M AIR PISTOL 40")
            count: 40
            gameDisplay1: qsTr("10M AIR")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-40")
        }
    }


//    ListModel {
//        id: gameEventModel
//    }

//    ListView {
//        id: gemeEventListView
//        visible: false
//        model: gameRange === 10 ? game10RangeEventModel : game50RangeEventModel
//    }

    ListModel {
        id: game50RangeEventModel

        ListElement {
            name: qsTr("50 Meter RIFLE FREE")
            count: -1
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 10")
            count: 10
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 20")
            count: 20
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-20")
        } 
	        ListElement {
            name: qsTr("50 Meter RIFLE 30")
            count: 30
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 40")
            count: 40
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 60")
            count: 60
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-60")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL FREE")
            count: -1
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 10")
            count: 10
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 20")
            count: 20
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-20")
        }

        ListElement {
            name: qsTr("50 Meter Free PISTOL 30")
            count: 30
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 40")
            count: 40
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 60")
            count: 60
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-60")
        }
    }

    ListModel {
        id: game50RangeEventModel_15

        ListElement {
            name: qsTr("50 Meter RIFLE FREE")
            count: -1
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 10")
            count: 10
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 15")
            count: 15
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-15")
        }
            ListElement {
            name: qsTr("50 Meter RIFLE 20")
            count: 20
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-20")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 30")
            count: 30
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("50 Meter RIFLE 40")
            count: 40
            gameDisplay1: qsTr("50 Meter")
            gameDisplay2: qsTr("RIFLE")
            matchDisplay: qsTr("MATCH-40")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL FREE")
            count: -1
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("UN-LIMITED")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 10")
            count: 10
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-10")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 15")
            count: 15
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-15")
        }

        ListElement {
            name: qsTr("50 Meter Free PISTOL 20")
            count: 20
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-20")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 30")
            count: 30
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-30")
        }
        ListElement {
            name: qsTr("50 Meter Free PISTOL 40")
            count: 40
            gameDisplay1: qsTr("50 Meter Free")
            gameDisplay2: qsTr("PISTOL")
            matchDisplay: qsTr("MATCH-40")
        }
    }


    Component.onCompleted: {
        MODREADER.setIsSingleDecimal(isSingleDecimal)
        shootingPage.setCurrentGameType(1)
        title = isDefaultIcon ? "TACHUS" : "SETA"
        MODREADER.setGame_range(gameRange)
        MODREADER.setShotPerSeries(shootsPerSeries)
        //MODREADER.on_pushButton_clicked();
    }

//    visibility: "FullScreen"
    visibility: "Maximized"

    onVisibilityChanged: {
        console.log("wiiiiiin visibility changed .... ", visibility)
        appVisiblityModeChanged(visibility)
    }

    Rectangle {
        id: fullRect
        color: "#202020"
        anchors.fill: parent
    }

    Header {
        id: header
        width: parent.width
        height: 40
        anchors.top: parent.top
        z: 5

        onMinimize: {
            console.log("windows visibility ", window.visibility)

            if (window.visibility == 2)
                window.visibility = 5
            else if (window.visibility == 5)
                window.visibility = 2
            else
                window.visibility = 5

            window.visibility = "Minimized"
        }

        onClose: {
            if (loginPage.visible)
            {
                window.close()
                Qt.quit()
            } else
                closeDia.visible = true
        }
    }

    ShootingPage {
        id: shootingPage
        z: 0
        height: parent.height - header.height
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        visible: !loginPage.visible

        Component.onCompleted: {
            console.log("srinivas", MODREADER.getNetworkPath(), "ssssssssssssss")
            if (MODREADER.getNetworkPath() == "") {
                console.log("testing")
                
                setCurrentGameType(7)
            }
        }

        onVisibleChanged: {
            if (visible) {
                // Re-map the discipline for the current distance/sub-mode (10m vs
                // 50m, Prone vs 3P share gameEvent=4, so onGameEventChanged alone
                // won't refresh it), then force the match timer to recompute.
                loginPage.updateGameType()
                shootingPage.refreshMatchTime()
                // ISSF sequence: every fresh session opens in the preparation/
                // sighting phase. Loaded sessions restore straight into match.
                if (!isSaveGame)
                    shootingPage.beginPreparationPhase()
                APPSETTINGS.updateStatusFeedbackFile(2)
            } else {
                APPSETTINGS.updateStatusFeedbackFile(1)
            }
        }
    }

    // Single floating-window overlay + registry for the whole app. Windows
    // register by key; pages open them via windowManager.openReport()/openCoach()
    // etc. (they never touch instances). Click-through except the window frames,
    // so the live target is never blocked.
    WindowManager {
        id: windowManager
        z: 50
        // Coach report window (Dashboard/Detailed/Print). Re-analyses the current
        // match on every open (aboutToOpen) so it never shows stale/demo data.
        CoachReportWindow {
            id: coachReportWindow
            gameSubMode: loginPage.gameSubMode
            Component.onCompleted: windowManager.register("coach", coachReportWindow)
            onAboutToOpen: {
                COACHFEED.analyzeCurrentMatch(loginPage.gameSubMode)
                viewMode = 0
            }
        }
    }

    LoginPage {
        id: loginPage

//        visible: false
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom

        onUsername_loginPageChanged: {
            window.userName = loginPage.username_loginPage
        }

        onGameModeChanged: updateGameType()
        onGameEventChanged: updateGameType()
        onRangeSelected: { gameRange = range; APPSETTINGS.set10or50mRange(range) }

        function updateGameType()
        {
            if (gameMode === 0)
            {
                if (gameEvent === 0)
                    shootingPage.setCurrentGameType(7)
                else if (gameEvent === 1)
                    shootingPage.setCurrentGameType(8)
                else if (gameEvent === 2)
                    shootingPage.setCurrentGameType(9)
                else if (gameEvent === 3)
                    shootingPage.setCurrentGameType(10)
                else if (gameEvent === 4)
                    shootingPage.setCurrentGameType(11)
                else
                    shootingPage.setCurrentGameType(6)           
		     } else {
                if (gameEvent === 0)
                    shootingPage.setCurrentGameType(1)
                else if (gameEvent === 1)
                    shootingPage.setCurrentGameType(2)
                else if (gameEvent === 2)
                    shootingPage.setCurrentGameType(3)
                else if (gameEvent === 3)
                    shootingPage.setCurrentGameType(4)
                else if (gameEvent === 4)
                    shootingPage.setCurrentGameType(5)

                else
                    shootingPage.setCurrentGameType(0)
            }
    }
           




    ClosePopupDialog {
        id: closeDia
        visible: false

        width: 300
        height: 100

        onCancel: {
            closeDia.visible = false
        }

        onDiscard: {
            window.close()
            Qt.quit()
        }

        onSave: {
            APPSETTINGS.saveMatch()
            closeDia.visible = false
            window.close()
            Qt.quit()
        }
    }

}
}
