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
    // M3 (reliability): set while resuming a crashed match so ShootingPage
    // does NOT run a fresh preparation phase — the state is already restored.
    property bool isRecoveredGame: false
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
            } else {
                dialogManager.show({
                    "type": "question",
                    "title": qsTr("Save Match?"),
                    "message": qsTr("The match is finished.\n\nDo you want to save this match before closing the application?"),
                    "buttons": [
                        { "label": qsTr("Cancel"),  "result": "cancel",  "accent": false },
                        { "label": qsTr("Discard"), "result": "discard", "accent": false },
                        { "label": qsTr("Save"),    "result": "save",    "accent": true }
                    ],
                    "defaultResult": "save",
                    "cancelResult": "cancel",
                    "onResult": function (r) {
                        // Same actions the legacy ClosePopupDialog performed.
                        if (r === "discard") {
                            window.close()
                            Qt.quit()
                        } else if (r === "save") {
                            APPSETTINGS.saveMatch()
                            window.close()
                            Qt.quit()
                        }
                    }
                })
            }
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
                if (!isSaveGame && !isRecoveredGame)
                    shootingPage.beginPreparationPhase()
                APPSETTINGS.updateStatusFeedbackFile(2)
            } else {
                APPSETTINGS.updateStatusFeedbackFile(1)
            }
        }
    }

    // Single floating-window overlay + registry for the whole app. Windows
    // M3 (reliability): recovery dialog + startup scan. Before the operator
    // reaches LoginPage, the RecoveryCoordinator (via FINALS3P) scans
    // Sessions/Current for an unfinished match. If one exists, this dialog
    // offers Resume (Clean/Recoverable) or Discard; all rebuild/replay is done
    // in C++ exclusively through the reducer.
    // ── Discipline recovery dispatcher (M3 Phase A) ──────────────────────
    // Selects the discipline restorer from the recovered session's stable
    // discipline id. The core recovery engine (RecoveryCoordinator/ReplayEngine/
    // reducer) is discipline-agnostic; discipline knowledge lives ONLY here and
    // in each restorer. Finals3P is wired; the qualification disciplines return
    // a clear "not yet implemented" result (their restorers land in Phase D).
    // Rules: never silently fall back to Finals; an unknown discipline fails
    // safe. See docs/issf-rules/README.md.
    function dispatchRecovery(sessionId, disciplineId) {
        // T1 closure: Training sessions are classified separately and resume
        // through the TRAINING owner only — never a competition restorer.
        if (disciplineId === "TRAINING") {
            var ok = shootingPage.restoreTrainingSession(sessionId)
            if (!ok)
                dialogManager.showError(qsTr("Training Recovery Failed"),
                    qsTr("The training session could not be resumed. "
                         + "Its journal has been left intact."))
            return ok
        }
        if (disciplineId === "FINAL3P")
            return window.restoreFinals3P(sessionId)
        // F3/F5: 10m Air Rifle / Air Pistol FINAL recovery via the dedicated
        // single-athlete finals restorer (FINALS10M owns its own clock/state).
        if (disciplineId === "FINAL_AR10" || disciplineId === "FINAL_AP10")
            return window.restoreFinals10m(sessionId, disciplineId)
        // Phase D1-D3: qualification recovery via the shared restorer — AR10
        // (decimal), AP10 (integer/full-ring), PRONE50 (decimal, 50-min clock,
        // no Final / no 3P transition). 3P qualification stays blocked on rules.
        if (disciplineId === "AR10" || disciplineId === "AP10"
                || disciplineId === "PRONE50")
            return window.restoreQualification(sessionId, disciplineId)
        if (disciplineId === "3P50") {
            dialogManager.showError(qsTr("Recovery Not Yet Available"),
                qsTr("Crash recovery for this discipline is not implemented in "
                     + "this build yet.\n\nThe unfinished session has been left "
                     + "intact and can be resumed by a later TechAim version."))
            return false
        }
        // Unknown / unsupported discipline: fail safe — never enter the Finals
        // restorer for a non-Finals session.
        dialogManager.showError(qsTr("Recovery Not Supported"),
            qsTr("This session's discipline (%1) cannot be recovered by this "
                 + "build.").arg(disciplineId && disciplineId.length
                                 ? disciplineId : qsTr("unknown")))
        return false
    }

    // The existing, verified Finals3P restorer — behaviour unchanged from M3;
    // now reached ONLY via dispatchRecovery() for FINAL3P sessions. Recovery
    // enters the SAME finals flow as a fresh start (shared enterFinalsMode()),
    // not a parallel path.
    function restoreFinals3P(sessionId) {
        // isRecoveredGame suppresses the duplicate beginPreparationPhase() in
        // onVisibleChanged so no fresh session is started.
        window.isRecoveredGame = true
        // Configure the existing finals selectors/settings (labels + the
        // discipline mapping used by onVisibleChanged.updateGameType()).
        loginPage.gameMode = 1     // rifle
        gameRange = 50             // window-scope (per scope gotcha)
        loginPage.gameSubMode = 1  // 3 positions
        loginPage.gameEvent = 6    // 3P FINAL (35)
        shootingPage.setFinalsGameType()
        loginPage.visible = false          // reveal the ShootingPage
        // Engage finals mode (isFinalsMatch = true) BEFORE resume: the C++
        // loadRecoveredState() re-emits the recovered shot signals, which are
        // only routed while the `enabled: isFinalsMatch` connection is active.
        // startFresh = false → no new session is started.
        shootingPage.enterFinalsMode(false)
        // Guard the target face: restoreStageFiringState() re-opens the current
        // firing window, whose handler would otherwise defer-clear the face and
        // erase the shots we are about to replay.
        shootingPage.suppressFaceClearOnce = true
        // While replaying, keep past-window sighters off the current face (they
        // still populate the shot list), so shot numbering on the face matches a
        // never-crashed match: recovered officials 1,2,3 then 4 live.
        shootingPage.recoveryReplayInProgress = true
        var resumed = FINALS3P.resumeFromRecovery(sessionId)
        shootingPage.recoveryReplayInProgress = false
        if (!resumed) {
            loginPage.visible = true
            window.isRecoveredGame = false
            dialogManager.showError(qsTr("Recovery Failed"),
                qsTr("The unfinished match could not be resumed."))
        }
        return resumed
    }

    // F3/F5: 10m Air Rifle / Air Pistol FINAL restorer. Mirrors restoreFinals3P
    // but for the dedicated FINALS10M controller: configure the SAME selectors a
    // fresh start uses, engage isFinals10mMatch (so the shot router re-routes the
    // replayed shots to the face) WITHOUT starting a new session, then inject the
    // reducer-rebuilt state. FINALS10M has its own RecoveryCoordinator, so it
    // must scan before resume (the startup scan populated FINALS3P's coordinator,
    // which is discipline-agnostic only for surfacing candidates).
    function restoreFinals10m(sessionId, disciplineId) {
        window.isRecoveredGame = true
        loginPage.gameMode = (disciplineId === "FINAL_AR10") ? 1 : 0   // rifle / pistol
        gameRange = 10
        loginPage.gameSubMode = 0
        loginPage.gameEvent = 7            // 10m FINAL (24)
        shootingPage.setFinals10mGameType()
        loginPage.visible = false          // reveal ShootingPage; isRecoveredGame
                                           // suppresses beginPreparationPhase()
        shootingPage.enterFinals10mMode(disciplineId, false)   // engage mode, no fresh start
        shootingPage.suppressFaceClearOnce = true
        shootingPage.recoveryReplayInProgress = true
        FINALS10M.scanForRecovery()        // populate FINALS10M's own coordinator
        var resumed = FINALS10M.resumeFromRecovery(sessionId)
        shootingPage.recoveryReplayInProgress = false
        if (!resumed) {
            loginPage.visible = true
            window.isRecoveredGame = false
            dialogManager.showError(qsTr("Recovery Failed"),
                qsTr("The unfinished 10m final could not be resumed."))
        }
        return resumed
    }

    // Phase D: shared qualification restorer (AR10/AP10/PRONE50). Configures
    // the SAME selectors/labels a fresh start uses, then enters qualification
    // mode via the canonical enterQualificationMode(config, false) seam and
    // injects the recovered session — never a parallel recovery-only UI.
    function restoreQualification(sessionId, disciplineId) {
        window.isRecoveredGame = true
        // Discipline-specific configuration: mode / distance / sub-mode.
        // (gameMode 0 = pistol, 1 = rifle; gameRange is window-scope.)
        if (disciplineId === "AR10") {
            loginPage.gameMode = 1; gameRange = 10; loginPage.gameSubMode = 0
        } else if (disciplineId === "AP10") {
            loginPage.gameMode = 0; gameRange = 10; loginPage.gameSubMode = 0
        } else {                                        // PRONE50
            loginPage.gameMode = 1; gameRange = 50; loginPage.gameSubMode = 0
        }
        // Event selection from the recovered course size, in LOGIN-space
        // (gameEvent 0..4 = 10/20/30/40/60 for both modes; the canonical
        // loginPage.updateGameType() translates to the per-mode event-model
        // index — rifle 1..5, pistol 7..11 — and runs both on assignment and
        // on the visibility flip). Defaults to the 60-shot official event.
        var expected = recoveryDialog.current ? recoveryDialog.current.expectedShots * 1 : 60
        var evIndex = {10: 0, 20: 1, 30: 2, 40: 3, 60: 4}[expected]
        if (evIndex === undefined) evIndex = 4
        loginPage.gameEvent = evIndex
        // Fresh-start parity: LoginPage.perfromStart() creates the legacy
        // .tch save-match file; without it setGame_is_sighter_mode()
        // dereferences a null QFile when the match phase engages.
        APPSETTINGS.saveMatch(true)
        loginPage.visible = false   // reveal ShootingPage; isRecoveredGame
                                    // suppresses beginPreparationPhase()
        var resumed = shootingPage.restoreQualificationSession(sessionId, disciplineId)
        if (!resumed) {
            loginPage.visible = true
            window.isRecoveredGame = false
            dialogManager.showError(qsTr("Recovery Failed"),
                qsTr("The unfinished match could not be resumed."))
        }
        return resumed
    }

    RecoveryDialog {
        id: recoveryDialog
        onResumeRequested: function(sessionId, disciplineId) {
            window.dispatchRecovery(sessionId, disciplineId)
        }
        onDiscardRequested: function(sessionId) { FINALS3P.discardRecovery(sessionId) }
    }
    Timer {
        // Run once after load so FINALS3P + QML are fully ready.
        interval: 200; running: true; repeat: false
        onTriggered: {
            var list = FINALS3P.scanForRecovery()
            if (list && list.length > 0)
                recoveryDialog.open(list)
        }
    }

    // register by key; pages open them via windowManager.openReport()/openCoach()
    // etc. (they never touch instances). Click-through except the window frames,
    // so the live target is never blocked.
    WindowManager {
        id: windowManager
        z: 50
        // Coach report window (Dashboard/Detailed/Print). The match is re-analysed
        // by the opener (ShootingPage.feedCoachReport, from the authoritative match
        // record) right before this opens, so here we only reset to the Dashboard
        // tab — analysing again would overwrite that correct data with the old
        // sighter-polluted C++ feeder path.
        CoachReportWindow {
            id: coachReportWindow
            gameSubMode: loginPage.gameSubMode
            Component.onCompleted: windowManager.register("coach", coachReportWindow)
            onAboutToOpen: viewMode = 0
        }
        // Phase E — EST incident / Jury workflow (operator range control).
        IncidentWindow {
            id: incidentWindow
            Component.onCompleted: windowManager.register("incidents", incidentWindow)
        }
    }

    // TechAim dialog framework: the ONE modal message/confirmation surface for
    // the whole app (id resolves everywhere via ancestor scope, like
    // windowManager). Above every window/HUD so modality is real.
    TechAimDialogManager {
        id: dialogManager
        anchors.fill: parent
        z: 4000
    }

    // F10: a failed operating-mode config write is surfaced visibly (never
    // silent) through the same TechAim dialog framework.
    Connections {
        target: (typeof OPMODE !== "undefined") ? OPMODE : null
        ignoreUnknownSignals: true
        function onConfigWriteFailed(message) {
            dialogManager.showError(qsTr("Could not change operating mode"), message)
        }
    }

    // C++ -> dialog-framework bridges: backend messages render in the same
    // TechAim dialogs as QML ones (no native message boxes anywhere).
    Connections {
        target: MODREADER
        function onUiDialogRequested(type, title, message) {
            if (type === "warning") dialogManager.showWarning(title, message)
            else if (type === "info") dialogManager.showInformation(title, message)
            else dialogManager.showError(title, message)
        }
    }
    Connections {
        target: CUSTOMPRINT
        function onPrintingNotice(message, timeoutMs) {
            dialogManager.show({ "type": "info",
                                 "title": qsTr("Exporting Report"),
                                 "message": message,
                                 "autoDismissMs": timeoutMs })
        }
    }
    // Session Reliability Layer (M0): a finals journal open/write/archive
    // failure is never silent — the controller emits once per session.
    Connections {
        target: FINALS3P
        function onJournalWriteFailed(path, detail) {
            dialogManager.showError(qsTr("Session Journal Error"),
                qsTr("The session journal could not be written. Shots continue "
                     + "to be scored, but this session may not be recoverable "
                     + "after a crash."),
                detail + "\n" + path)
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
                else if (gameEvent === 7)
                    shootingPage.setFinals10mGameType()   // 10m Air Pistol FINAL (24)
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
                else if (gameEvent === 6)
                    shootingPage.setFinalsGameType()   // 3P FINAL (35)
                else if (gameEvent === 7)
                    shootingPage.setFinals10mGameType()   // 10m Air Rifle FINAL (24)
                else
                    shootingPage.setCurrentGameType(0)
            }
    }

    // ── F10: global operating-mode badge ─────────────────────────────────
    // Unmistakable in BOTH modes and present on every screen (event selection,
    // qualification, 10m Finals, 3P Final, recovery, completed-result) because
    // it overlays the whole window at the top z-order. Demo uses the warning
    // (amber/red) treatment; Live uses a restrained neutral-green pill so it
    // never distracts the athlete. Reads the running mode from OPMODE.
    Rectangle {
        id: opModeBadge
        readonly property bool opLive: (typeof OPMODE !== "undefined") ? OPMODE.live : window.appMode
        z: 100000
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 6
        width: opModeBadgeText.implicitWidth + 26
        height: 22
        radius: 11
        color: opLive ? "#14361f" : "#3a0d12"
        border.width: 1
        border.color: opLive ? "#2f7d4f" : "#e8003d"
        opacity: opLive ? 0.82 : 0.96
        Row {
            anchors.centerIn: parent
            spacing: 6
            Rectangle {
                width: 7; height: 7; radius: 3.5
                anchors.verticalCenter: parent.verticalCenter
                color: opModeBadge.opLive ? "#37c76a" : "#ff2d55"
            }
            Text {
                id: opModeBadgeText
                anchors.verticalCenter: parent.verticalCenter
                text: (typeof OPMODE !== "undefined")
                      ? OPMODE.badgeText
                      : (opModeBadge.opLive ? "LIVE TARGET" : "DEMO / SIMULATION")
                color: opModeBadge.opLive ? "#bfe8cd" : "#ffd0d7"
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 1
            }
        }
    }



    // The exit/save confirmation now runs through dialogManager (TechAim
    // dialog framework); the legacy grey ClosePopupDialog is gone.

}
}
