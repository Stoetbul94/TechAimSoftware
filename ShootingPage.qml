import QtQuick 2.15
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

    // ── Phase B1: 10m Air Rifle qualification durable journaling ─────────────
    // Non-empty when the current qualification session is journalled through the
    // QUAL seam (B1: "AR10" only; AP10/Prone follow in B2/B3). Empty => the
    // legacy direct-to-model path. Gates the QUAL routing so no second parallel
    // scoring path exists for the un-migrated disciplines.
    property string qualDisciplineId: ""
    // Per-session strictly-increasing shot identity for the durable seam. One
    // per processed detection event (the C++ backEndShootCount guard already
    // dedupes repeated hardware deliveries upstream, so a value is never
    // consumed twice); carried as the event externalId for reducer-level dedup
    // and future resume seeding. NOT the visible model count.
    property int qualShotSeq: 0
    // The polar display (angle, radius) of the shot in flight, stashed so the
    // QUAL.onShotAccepted projection reuses the EXACT existing addToSeries args
    // (the signal fires synchronously inside submit, so these stay valid).
    property real qualPendingAngle: 0
    property real qualPendingRadius: 0
    // Phase D: true while restoreQualificationSession() replays a recovered
    // session into the UI. Guards the canonical transitions it reuses
    // (changedToMatchMode) from RE-journalling phase events — the recovered
    // journal already contains OfficialMatchStarted + its TimerStarted anchor,
    // and re-anchoring would restart the frozen match clock.
    property bool qualRecoveryInProgress: false

    // ── 50m Rifle 3 Positions (ISSF 3x20 qualification) ──────────────────────
    // Position is derived from the match-shot count: 0-19 kneeling, 20-39
    // prone, 40-59 standing. The overall 105-min match clock runs through
    // position changes; only shot tagging switches to sighter during them.
    property bool is3PMatch: false
    // 3P FINAL (35) — separate finals domain; FINALS3P owns all finals timing.
    property bool isFinalsMatch: false
    // M3 recovery: one-shot guard so the window-open re-established during a
    // recovery resume does not clear the recovered shots off the target face.
    property bool suppressFaceClearOnce: false
    // M3 recovery: true while loadRecoveredState re-emits the recovered shots.
    // Sighters belong to a PAST (sighting) window and would normally have been
    // cleared off the current firing-window face at window-open — so during the
    // replay the router keeps them out of the face buffer (they still populate
    // the shot list). Officials of the current window populate the face as usual.
    property bool recoveryReplayInProgress: false
    // Shot direction (angle) and polar display radius of the shot currently
    // being registered with the finals controller; injected into the record by
    // the router. `score`/`direction` are the qualification polar-display
    // convention the target overlay plots with (mapToPosition on polarSeries);
    // xmm/ymm remain the canonical millimetre record.
    property real finalsLastDirection: 0
    property real finalsLastRadius: 0
    // Deterministic detection-event identity for finals (FIX1): one strictly
    // increasing sequence per session, incremented once per processed
    // detection event (demo click or live shot — same pipeline). A repeated
    // backend delivery never reaches pointAddedToSeries twice (the C++ layer
    // dedupes register reads), so each emission IS a new physical detection.
    property int finalsShotSeq: 0
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

    property string minimumShotsSummary: "Minimum 10 shots required to generate Summary"
    property string minimumShotsMatchReport: "Minimum 10 shots required to generate Match Report"

    onTotalScoreChanged: {
        if (APPSETTINGS.getDeveloperMode()) console.log("Total score changed ............................"+ totalScore)

    }

    // Popup messages migrated to the TechAim dialog framework
    // (dialogManager in main.qml) — no QtQuick.Dialogs MessageDialog left.

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


    // The finish-match confirmation now runs through dialogManager (see
    // confirmMatchFinish below). The legacy Dialog's StandardButton enum was
    // never in scope under the QtQuick.Dialogs import (ReferenceError at
    // runtime), so its OK/Cancel buttons never rendered — this migration also
    // fixes that.
    function confirmMatchFinish() {
        dialogManager.showConfirmation(qsTr("Finish Match"),
            qsTr("Are you sure you want to finish this match?\n\nOnce finished, no further shots can be recorded."),
            function (ok) { if (ok) changedToMatchFinish() },
            qsTr("Finish Match"), qsTr("Cancel"))
    }

    ListModel
    {
        id:globalModelOfData
        onCountChanged: {
            centerPanel.disableMotorMovement = false
            centerPanel.currentPageIndexChanged()
            if (APPSETTINGS.getDeveloperMode()) console.log("globalModelOfData count changes ", count)
        }
    }

    ListModel
    {
        id:globalSlighterModel
        onCountChanged: {
            if (APPSETTINGS.getDeveloperMode()) console.log("******globalSlighterModel****"+count)
        }
    }

    // Pre-lock the shared shot models to the ROLE UNION (qualification +
    // finals schema) before any real append: QML ListModel locks its role set
    // at the first append, so without this a finals session would lock the
    // models to whichever mode appended first and silently drop the other
    // mode's roles. The template comes from the C++ schema builder, so it can
    // never drift from the real records. Appended once, then cleared — role
    // locks survive clear(). Qualification rows simply leave the finals-*
    // roles unset; qualification behaviour is unchanged.
    Component.onCompleted: {
        var tpl = FINALS3P.templateShotRecord()
        globalMatchModel.append(tpl);    globalMatchModel.clear()
        globalSlighterModel.append(tpl); globalSlighterModel.clear()
        globalModelOfData.append(tpl);   globalModelOfData.clear()
    }

    // Rejected finals shots (never in the official models) — plan §8.
    ListModel { id: finalsIncidentModel }
    // Compact accepted-shot feed for the finals panel list (display only).
    ListModel { id: finalsShotListModel }

    ListModel
    {
        id:globalMatchModel
        onCountChanged: {
            if (APPSETTINGS.getDeveloperMode()) console.log("******globalMatchModel****"+count)
        }
    }

    function loadGameInMatchMode() {
        is3PMatch = false   // restored sessions bypass beginPreparationPhase
        isFinalsMatch = false
        rightPanel.startClickedThroughLoad()
        sligterMode = false
        centerPanel.showSlighter(false)
    }

    // 3P FINAL (35) — the finals event bypasses the qualification event models.
    function setFinalsGameType()
    {
        matchShootCount = 35
        currentGameDisplay1 = "FINAL"
        currentGameDisplay2 = "35"
        currentmatchDisplay = "FINAL 35"
    }

    function setCurrentGameType(index)
    {
        if (APPSETTINGS.getDeveloperMode()) console.log("setCurrentGameType  ", index)
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

        // ── Phase stepper: SIGHT → MATCH, or SIGHT → KNEEL → PRONE → STAND ──
        Row {
            anchors.centerIn: parent
            spacing: 6
            Repeater {
                model: is3PMatch ? [qsTr("SIGHT"), qsTr("KNEEL"), qsTr("PRONE"), qsTr("STAND")]
                                 : [qsTr("SIGHTING"), qsTr("MATCH")]
                delegate: Rectangle {
                    // Index of the active step: initial sighting is step 0;
                    // once the match starts, 3P tracks the current position.
                    readonly property int activeStep: {
                        if (is3PMatch)
                            return (sligterMode && p3BreaksDone === 0) ? 0 : 1 + p3Position
                        return sligterMode ? 0 : 1
                    }
                    readonly property bool isCurrent: !matchFinished && index === activeStep
                    readonly property bool isDone: matchFinished || index < activeStep
                    radius: 10; height: 20
                    width: stepText.implicitWidth + 18
                    color: isCurrent ? "#3a0d16" : "#26262c"
                    border.color: isCurrent ? "#e8003d" : "transparent"
                    border.width: 1
                    Text {
                        id: stepText
                        anchors.centerIn: parent
                        text: modelData
                        color: isCurrent ? "#ff5c7a" : (isDone ? "#9a9ba0" : "#55555e")
                        font.family: theme.fontFamily
                        font.pixelSize: 9; font.bold: isCurrent; font.letterSpacing: 1
                    }
                }
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
                // 3P FINAL: the HUD strip owns phase display; the qualification
                // phase chip (SIGHTING/MATCH from sligterMode) would conflict.
                visible: !isFinalsMatch
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

    // ── Bottom action bar: ONE context-aware primary button, always in the
    // same place (range feedback: the floating resume strip and the small
    // play icon were not ergonomic). Feed-paper stays as a jam-recovery
    // fallback — the motor is otherwise automatic after every shot.
    // M2 (reliability): persistence-health banner, docked directly above the
    // action bar. Only visible while persistence is degraded; shots keep
    // scoring regardless (§9C). Finals only — qualification persistence is M4.
    PersistenceBanner {
        id: persistenceBanner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionBar.top
        visible: shootingPage.isFinalsMatch && height > 0
        z: 41
    }

    Rectangle {
        id: actionBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 62
        color: "#15161a"
        z: 40

        // matchFinished → view report · sighting/position break → start ·
        // record fire → finish match (with confirmation)
        readonly property int barMode: matchFinished ? 2 : (sligterMode ? 0 : 1)

        Rectangle {
            id: primaryActionBtn
            anchors.left: parent.left; anchors.leftMargin: leftPanel.width
            anchors.right: feedPaperBtn.left; anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            height: 44; radius: 8
            // 3P FINAL (FIX2): the bar is the Finals contextual primary action.
            // The controller owns visibility/enabled/label/legality; this QML
            // only renders and invokes. Qualification behaviour is untouched.
            readonly property bool finalsMode: shootingPage.isFinalsMatch
            readonly property bool finalsEnabled: finalsMode && FINALS3P.primaryActionEnabled
            visible: finalsMode ? FINALS3P.primaryActionVisible : true
            color: finalsMode
                   ? (finalsEnabled ? (primaryMouse.containsMouse ? "#c40034" : "#e8003d")
                                    : "transparent")
                   : (actionBar.barMode === 1
                      ? "transparent"
                      : (primaryMouse.containsMouse ? "#c40034" : "#e8003d"))
            border.color: (finalsMode ? !finalsEnabled : actionBar.barMode === 1)
                          ? "#3a3b40" : "transparent"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: {
                    if (primaryActionBtn.finalsMode)
                        return FINALS3P.primaryActionLabel
                    if (actionBar.barMode === 2) return qsTr("VIEW REPORT  →")
                    if (actionBar.barMode === 1)
                        return qsTr("MATCH IN PROGRESS  ·  ") + qsTr("FINISH MATCH")
                    if (is3PMatch && p3BreaksDone > 0)
                        return qsTr("START ") + p3Names[p3Position] + "  →"
                    return qsTr("START MATCH  →")
                }
                color: (primaryActionBtn.finalsMode ? !primaryActionBtn.finalsEnabled
                                                    : actionBar.barMode === 1)
                       ? "#9a9ba0" : "white"
                font.family: theme.fontFamily
                font.pixelSize: 14
                font.bold: primaryActionBtn.finalsMode ? primaryActionBtn.finalsEnabled
                                                       : actionBar.barMode !== 1
                font.letterSpacing: 1.5
            }
            MouseArea {
                id: primaryMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (primaryActionBtn.finalsMode) {
                        FINALS3P.executePrimaryAction()   // controller owns legality
                        return
                    }
                    if (actionBar.barMode === 2)
                        windowManager.openMatchReport()
                    else if (actionBar.barMode === 1)
                        confirmMatchFinish()
                    else
                        rightPanel.startClicked()
                }
            }
        }

        Rectangle {
            id: feedPaperBtn
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: 130; height: 44; radius: 8
            color: feedMouse.containsMouse ? "#26272c" : "#1a1a1f"
            border.color: "#3a3b40"; border.width: 1
            Text {
                anchors.centerIn: parent
                text: qsTr("Feed paper")
                color: "#b9b9c0"; font.family: theme.fontFamily; font.pixelSize: 12
            }
            MouseArea {
                id: feedMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: MODREADER.initiateMotorMovement()
            }
        }
    }

    LeftPanel {
        id: leftPanel
        width: 0.15*parent.width
        height: parent.height - statusStrip.height - actionBar.height
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
        width: 0.34*parent.width
        height: parent.height - statusStrip.height - actionBar.height
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
        height: parent.height - statusStrip.height - actionBar.height
        anchors.left: leftPanel.right
        anchors.right: rightPanel.left
        anchors.top: statusStrip.bottom

        property alias showMesures: leftPanel.isShowMPI


        onPointAddedToSeries: {
            // 3P FINAL: shots reach here through the SAME detection + scoring
            // pipeline as qualification; only the registration differs. The
            // controller validates (window, limits, duplicates) and emits the
            // complete record; the router below appends to the models.
            // Duplicate key: the backend's cumulative detection counter
            // (MODREADER.getShootCount() at receipt) — strictly increasing
            // per firing window; a repeated value is a retransmission.
            if (isFinalsMatch) {
                shootingPage.finalsLastDirection = xPosition
                shootingPage.finalsLastRadius = yPosition
                FINALS3P.registerShot(centerPanel.lastShotXmm, centerPanel.lastShotYmm,
                                      currentCalculatedScore, ++shootingPage.finalsShotSeq,
                                      shootingPage.finalsLastDirection)
                return
            }
            // B1/B2: 10m Air Rifle (AR10, decimal) and 10m Air Pistol (AP10,
            // integer) — the DURABLE acceptance route. No UI append happens
            // here; the shot is submitted to QUAL first and the QUAL.onShotAccepted
            // signal drives the (existing) projection, so a shot becomes visible
            // only after it is journalled. If QUAL refuses it (cap / duplicate /
            // wrong phase) nothing is projected. sligterMode classifies sighter
            // vs official (the same source addToSeries would use).
            if (qualDisciplineId !== "") {
                shootingPage.qualPendingAngle = xPosition
                shootingPage.qualPendingRadius = yPosition
                var extId = ++shootingPage.qualShotSeq
                // AP10 is full-ring INTEGER scoring: floor the decimal ring
                // position to the integer ring value BEFORE it is journalled,
                // so the reducer total is integer-based. AR10 stays decimal.
                var seamScore = (qualDisciplineId === "AP10")
                    ? Math.floor(currentCalculatedScore)
                    : currentCalculatedScore
                if (sligterMode)
                    QUAL.submitSighter(centerPanel.lastShotXmm, centerPanel.lastShotYmm,
                                       seamScore, extId, xPosition, false)
                else
                    QUAL.submitOfficial(centerPanel.lastShotXmm, centerPanel.lastShotYmm,
                                        seamScore, extId, xPosition, false)
                return
            }
            // Hard cap at the match shot count. The auto-finish watcher polls
            // every 500ms, so a shot arriving in that window (an extra demo
            // click, or a late hardware report) used to register as shot 61
            // and pollute the totals. Free practice (matchShootCount -1) is
            // unaffected; sighters are unaffected (they don't fill the record).
            if (!sligterMode && matchShootCount > 0
                    && globalMatchModel.count >= matchShootCount) {
                MODREADER.appendToLogFile("Ignoring shot beyond match limit ("
                                          + globalMatchModel.count + "/" + matchShootCount + ")")
                return
            }
            rightPanel.addToSeries(xPosition,yPosition,currentCalculatedScore,
                                   centerPanel.lastShotXmm, centerPanel.lastShotYmm)
            if (APPSETTINGS.getDeveloperMode()) console.log("x ", xPosition, " y ", yPosition, " score ", currentCalculatedScore, " matchShootCount ", matchShootCount)

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


    // Map a globalMatchModel position role (0=K 1=P 2=S for 3P, -1 otherwise) to
    // the coach engine's position token. Non-3P disciplines carry a single body
    // position derived from the discipline, matching the old feeder's mapping.
    function coachPositionName(pos) {
        if (is3PMatch) {
            if (pos === 0) return "Kneeling"
            if (pos === 1) return "Prone"
            if (pos === 2) return "Standing"
            return "Unknown"
        }
        if (gameRange === 10) return gameMode ? "AirPistol" : "AirRifle"
        if (gameRange === 50 && gameMode === 0) return "Prone"   // 50m rifle prone
        return "Unknown"                                         // 50m pistol etc.
    }

    // Feed the Coach analytics from the authoritative match record. globalMatchModel
    // holds, per MATCH shot in order, the correct display coordinate (xmm/ymm), the
    // decimal score and the REAL per-shot position — the same data the Match report
    // plots. This replaces COACHFEED.analyzeCurrentMatch, whose C++ path read a
    // coordinate list polluted by each position's sighter shots (so getXCord and the
    // match-only getScore drifted out of alignment after position 1) and guessed the
    // 3P positions by an equal-thirds split. No scores/coords are recomputed here —
    // the model values are passed straight through to the (unchanged) analytics engine.
    function feedCoachReport() {
        var n = globalMatchModel.count
        var shotsPerSeries = 10   // ISSF series size, as used throughout the app

        // Cumulative per-shot timestamps, all-or-nothing like the old feeder: any
        // missing/invalid interval disables timing rather than fabricating it.
        var ts = []
        var timingOk = n >= 1
        var t = 0
        for (var k = 0; k < n; ++k) {
            if (k > 0) {
                var iv = globalMatchModel.get(k).timeComsumed * 1
                if (iv > 0) t += iv
                else timingOk = false
            }
            ts.push(t)
        }

        var list = []
        for (var i = 0; i < n; ++i) {
            var e = globalMatchModel.get(i)
            list.push({
                "shotNumber": i + 1,
                "seriesNumber": Math.floor(i / shotsPerSeries) + 1,
                "decimalScore": e.calculatedscore * 1,
                "x": e.xmm * 1,
                "y": e.ymm * 1,
                "position": coachPositionName(e.position * 1),
                "timestamp": ts[i],
                "hasTimestamp": timingOk,
                "isValid": true,
                "isSighter": false,
                "isCompetitionShot": true
            })
        }
        COACHREPORT.analyzeShots(list)

        // Transfer guarantee: every shot in the match record must be held by
        // the engine — count AND total must survive the bridge round-trip.
        var held = COACHREPORT.shots()
        if (held.length !== n) {
            console.warn("COACH ASSERT: fed", n, "shots but engine holds", held.length)
        } else if (n > 0) {
            var fedSum = 0, heldSum = 0
            for (var c = 0; c < n; ++c) {
                fedSum += globalMatchModel.get(c).calculatedscore * 1
                heldSum += held[c].decimalScore
            }
            if (Math.abs(fedSum - heldSum) > 0.05)
                console.warn("COACH ASSERT: fed total", fedSum.toFixed(1),
                             "!= engine total", heldSum.toFixed(1))
        }
    }

    // Floating Report window (Summary now, Match next). Declared here so the
    // embedded SummaryReportView can resolve rightPanel/centerPanel/shootingPage,
    // but registered with the single app WindowManager. Non-blocking.
    ReportWindow {
        id: reportWindow
        Component.onCompleted: windowManager.register("report", reportWindow)
        // Tab is chosen by the opener via prepare(): Stats -> Summary (0),
        // Report -> Match (1). Do NOT reset it in onAboutToOpen — that fires
        // after prepare() on first open and would force both buttons to Summary.
        onCoachRequestedFromReport: {
            windowManager.dismiss(reportWindow)
            feedCoachReport()
            windowManager.openCoach()
        }
    }

    // ── 3P FINAL shot router (Phase B) ───────────────────────────────────
    // Thin, bind-only: the controller has already validated and built the
    // complete record. Sighters -> globalSlighterModel; accepted match shots
    // -> globalMatchModel; rejections -> finalsIncidentModel only. The
    // official models NEVER receive synthetic or rejected shots.
    Connections {
        target: FINALS3P
        enabled: isFinalsMatch

        function onShotAccepted(shot) {
            var rec = shot
            rec.direction = shootingPage.finalsLastDirection.toFixed(2)
            // Polar display radius (FIX1): the overlay positions shots via
            // mapToPosition(direction, score) — `score` must be the scoring
            // engine's polar radius, NOT the decimal score.
            rec.score = shootingPage.finalsLastRadius.toFixed(2)
            if (rec.isSighter)
                globalSlighterModel.append(rec)
            else
                globalMatchModel.append(rec)
            // Display buffer drives the target face (cleared per window below).
            // M3 recovery: a replayed SIGHTER belongs to a past sighting window
            // and, in a never-crashed match, would already have been cleared off
            // the current firing-window face at window-open — so keep it out of
            // the face buffer here. Officials of the current window populate the
            // face exactly as in live firing, preserving shot numbering.
            if (!(shootingPage.recoveryReplayInProgress && rec.isSighter))
                globalModelOfData.append(rec)
            finalsShotListModel.append(rec)
            // Right panel consumes the SAME accepted record (FIX4).
            rightPanel.finalsOnShotAccepted(rec)

            // Developer-mode single-source assertions (plan §7): controller
            // totals must equal the official rows in globalMatchModel.
            if (APPSETTINGS.getDeveloperMode() && !rec.isSighter) {
                var n = 0, sum = 0
                for (var i = 0; i < globalMatchModel.count; ++i) {
                    var e = globalMatchModel.get(i)
                    if (e.isFinalsShot === true && e.isSighter !== true) {
                        ++n
                        sum += e.calculatedscore * 1
                    }
                }
                if (n !== FINALS3P.officialShotCount
                        || Math.abs(sum - FINALS3P.cumulativeTotal) > 0.05)
                    console.warn("FINALS ASSERT: model", n, sum.toFixed(1),
                                 "!= controller", FINALS3P.officialShotCount,
                                 FINALS3P.cumulativeTotal.toFixed(1))
            }
        }

        function onShotRejected(rej) {
            finalsIncidentModel.append(rej)
            // Toast presentation listens to FINALS3P.shotRejected itself (HUD3).
        }

        // D3: RESULTS ARE FINAL -> bottom-bar VIEW REPORT -> finals report tab.
        function onReportRequested() {
            windowManager.openFinalsReport()
        }

        function onWindowStateChanged() {
            // Clean face per firing window (mirrors qualification's
            // clean-face-per-position). Deferred: model resets from inside
            // signal handlers with live delegates crash (see 3P doc gotchas).
            if (FINALS3P.isFiringWindowOpen) {
                // M3 recovery: the window re-established by restoreStageFiringState
                // must NOT erase the shots just replayed onto the face — consume
                // the one-shot guard and skip this clear (normal firing windows
                // still clear the face as before).
                if (shootingPage.suppressFaceClearOnce) {
                    shootingPage.suppressFaceClearOnce = false
                    return
                }
                Qt.callLater(function() { globalModelOfData.clear() })
            }
        }
    }

    // ── B1/B2: qualification durable-shot projection router ──────────────
    // The ONLY route that appends an AR10/AP10 shot to the visible models. It
    // fires (synchronously) after QUAL has durably submitted the SighterAccepted
    // / ShotAccepted event, so the journal — not the UI — is authoritative. It
    // reuses the EXACT existing qualification projection (rightPanel.addToSeries)
    // with the shot's polar display args, stashed just before submit. The
    // reducer-authoritative score (decimal for AR10, integer for AP10) and
    // coords come from the accepted record.
    Connections {
        target: QUAL
        enabled: shootingPage.qualDisciplineId !== ""
        function onShotAccepted(record) {
            rightPanel.addToSeries(shootingPage.qualPendingAngle,
                                   shootingPage.qualPendingRadius,
                                   record.calculatedscore * 1,
                                   record.xmm * 1, record.ymm * 1)
        }
    }

    // ── 3P FINAL HUD (Option A: top strip + contextual layers) ───────────
    // Pure presentation over FINALS3P — see docs/3p-finals-discipline.md.
    FinalsHud {
        visible: isFinalsMatch
        z: 30
        anchors.fill: centerPanel
        ctl: FINALS3P
        developerMode: APPSETTINGS.getDeveloperMode()
    }

    // Match report now lives in the floating Report window (Match tab); see the
    // ReportWindow instance below. The old standalone MatchReport dialog is gone.

    Connections {
        target: APPSETTINGS
        function onPrintPDF() {
            if (leftPanel.playVisible)
                return;

            // 3P FINAL: the qualification Match auto-export must never be fed
            // finals data — open the finals report instead (manual Save PDF).
            if (isFinalsMatch) {
                windowManager.openFinalsReport()
                return;
            }

            // Kiosk auto-export: open the Report window on the Match tab and let it
            // write the PDF to the configured network path, then close on save.
            windowManager.openMatchReport()
            reportWindow.startMatchAutoExport()
            if (APPSETTINGS.getDeveloperMode()) console.log("-APPSETTINGS-----------------------------onPrintPDF--------------------------")
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
    // 3P FINAL mode-engagement — the SINGLE way the finals view is entered
    // (canonical fresh start AND recovery resume). It sets isFinalsMatch (which
    // drives the finals RightPanel/HUD and the `enabled: isFinalsMatch` shot
    // router) and prepares the finals models. Only when startFresh is true does
    // it also start a brand-new session — recovery passes false and injects the
    // recovered state instead (M3). Steps (a)–(c) are byte-identical to the old
    // inline finals branch; (d)–(e) are gated on startFresh.
    function enterFinalsMode(startFresh)
    {
        isFinalsMatch = true
        is3PMatch = false
        // Fresh, start-from-zero session: the finals path returns before the
        // qualification clearing flows run, so clear the shot models here
        // (role locks survive clear()).
        globalMatchModel.clear()
        globalSlighterModel.clear()
        globalModelOfData.clear()
        finalsIncidentModel.clear()
        finalsShotListModel.clear()
        // FIX1: deterministic session state. Qualification resets these in
        // changedToSigherMode/changedToMatchMode, which finals never runs:
        // stale backEndShootCount made the shot processor read wrong backend
        // indices after a prior session; stale sligterMode/matchFinished leak
        // previous-session behaviour into the shared display pipeline.
        centerPanel.backEndShootCount = 0
        sligterMode = true            // face shows all current-window shots
        matchFinished = false
        finalsShotSeq = 0
        rightPanel.resetRightPanelModels()   // clears the shot table + totals
        if (startFresh) {
            MODREADER.appendToLogFile("3P FINAL: session start (FINALS3P owns timing)")
            // D4 ceremony polish: the Full ceremony announces the athlete.
            FINALS3P.athleteName = (typeof userName !== "undefined" && userName) ? userName : ""
            FINALS3P.resetFinal()
            FINALS3P.startFinal()
        }
    }

    function beginPreparationPhase()
    {
        // 3P FINAL: a separate domain — the FINALS3P controller owns ALL finals
        // timing and phases; none of the qualification prep/sighter machinery
        // (or its timers) runs. Phase A: skeleton + dry-run only, no scoring.
        isFinalsMatch = APPSETTINGS.getGameMode() === 1
                     && APPSETTINGS.get10or50mRange() === 50
                     && APPSETTINGS.getGameSubMode() === 1
                     && matchShootCount === 35
        if (isFinalsMatch) {
            enterFinalsMode(true)            // canonical fresh finals start
            return
        }
        MODREADER.appendToLogFile("beginPreparationPhase: prep seconds = " + APPSETTINGS.getPrepTimeCount())
        is3PMatch = APPSETTINGS.getGameMode() === 1
                 && APPSETTINGS.get10or50mRange() === 50
                 && APPSETTINGS.getGameSubMode() === 1
                 && matchShootCount === 60
        p3BreaksDone = 0
        centerPanel.totalSighterTime = APPSETTINGS.getPrepTimeCount()
        changedToSigherMode()
        centerPanel.startPreparationCountdown()

        // Phase B1: 10m Air Rifle qualification is journalled through the QUAL
        // seam so the journal + reducer become authoritative. Only air rifle
        // (gameMode 1 = rifle, 10m range) migrates in B1; AP10/Prone keep the
        // legacy path until B2/B3. is3PMatch/isFinalsMatch are already excluded
        // above. See docs/issf-rules/10m-air-rifle.md.
        // Which migrated qualification discipline is this? (B1: AR10 decimal;
        // B2: AP10 integer; B3: PRONE50 decimal). Pure detection — the actual
        // mode engagement is the shared enterQualificationMode() seam below.
        var freshQualId = ""
        if (APPSETTINGS.get10or50mRange() === 10) {
            if (APPSETTINGS.getGameMode() === 1)
                freshQualId = "AR10"             // 10m Air Rifle  (decimal)
            else if (APPSETTINGS.getGameMode() === 0)
                freshQualId = "AP10"             // 10m Air Pistol (integer)
        } else if (APPSETTINGS.get10or50mRange() === 50) {
            // 50m Rifle Prone: rifle, sub-mode 0 (Prone), decimal. 50m 3P
            // qualification (sub-mode 1 / is3PMatch) is NOT migrated — its rules
            // are pending; 50m pistol is out of scope. Prone must never enter
            // Finals or a 3P position transition (both already excluded above).
            if (APPSETTINGS.getGameMode() === 1
                    && APPSETTINGS.getGameSubMode() === 0 && !is3PMatch)
                freshQualId = "PRONE50"          // 50m Rifle Prone (decimal)
        }
        if (freshQualId !== "")
            enterQualificationMode(freshQualId, true)   // canonical fresh start
        else
            qualDisciplineId = ""                       // unmigrated: legacy path
        MODREADER.appendToLogFile("beginPreparationPhase: done, totalSighterTime = "
                                  + centerPanel.totalSighterTime + " is3P = " + is3PMatch)
    }

    // Phase D: single qualification mode-engagement for the MIGRATED
    // disciplines (AR10/AP10/PRONE50) — the canonical fresh start AND the
    // recovery resume enter here; there is no parallel recovery-only setup.
    // Part A (always): discipline routing + shot-identity reset. Part B
    // (startFresh only): begin a brand-new journalled QUAL session and its
    // original preparation+sighting phase. Recovery passes false and injects
    // the recovered session instead — it must never create a new session,
    // archive the interrupted journal, or restart the phase clock.
    function enterQualificationMode(disciplineId, startFresh)
    {
        if (!startFresh) {
            // Recovery arrives without beginPreparationPhase(): establish the
            // qualification mode it would have, in the same order the fresh
            // path runs (mode flags → models → sighter routing). Fresh-path
            // callers arrive with all of this already done.
            isFinalsMatch = false
            is3PMatch = false
            resetDataModels()                    // clear stale fresh models
            centerPanel.totalSighterTime = APPSETTINGS.getPrepTimeCount()
            changedToSigherMode()                // sighter routing + clean face
        }
        qualDisciplineId = disciplineId
        qualShotSeq = 0
        if (!startFresh)
            return
        var athlete = (typeof userName !== "undefined" && userName) ? userName : ""
        // ISSF course parameters are CONFIG the caller supplies (not rules in
        // the engine): official count + match/prep clocks as timer anchors.
        // matchShootCount = the selected count (60 for the full event);
        // getTimeCount/getPrepTimeCount return seconds.
        var matchMs = APPSETTINGS.getTimeCount(matchShootCount) * 1000
        var prepMs = APPSETTINGS.getPrepTimeCount() * 1000
        var started = QUAL.startSession(disciplineId, String(matchShootCount),
                                        athlete, matchShootCount, matchMs,
                                        prepMs, -1, "", "")
        if (started) {
            QUAL.beginPreparation()
            QUAL.beginSighting()   // combined ISSF prep+sighting phase
            MODREADER.appendToLogFile("QUAL: " + disciplineId
                                      + " session journalling started ("
                                      + QUAL.journalPath() + ")")
        } else {
            qualDisciplineId = ""
            MODREADER.appendToLogFile("QUAL: startSession failed — "
                                      + "falling back to the legacy path")
        }
    }

    // Phase D: restore a crashed qualification session (AR10/AP10/PRONE50).
    // Deterministic order (docs/session-reliability-phaseB-qualification.md):
    // the journal was already validated + reducer-replayed in C++; here we
    // (1) configure the qualification mode WITHOUT starting a session,
    // (2) adopt the recovered journal/state, (3) project the recovered shots
    // through the SAME rightPanel.addToSeries used after a durable live
    // acceptance, (4) re-establish the recovered phase via the canonical
    // sighting→match transition, (5) rebase the frozen phase clock, and
    // (6) seed the live shot identity so a re-reported pre-crash measurement
    // is refused by the reducer's externalId guard.
    function restoreQualificationSession(sessionId, disciplineId)
    {
        enterQualificationMode(disciplineId, false)
        if (!QUAL.resumeFromRecovery(sessionId)) {
            qualDisciplineId = ""
            return false
        }
        qualRecoveryInProgress = true
        var shots = QUAL.recoveredShots()
        var i, s, p
        // Sighters first, while sligterMode is true — the exact classification
        // addToSeries applies to a live sighting-phase shot.
        for (i = 0; i < shots.length; ++i) {
            s = shots[i]
            if (s.isSighter !== true)
                continue
            p = centerPanel.polarForMm(s.xmm * 1, s.ymm * 1)
            rightPanel.addToSeries(p.x, p.y, s.calculatedscore * 1,
                                   s.xmm * 1, s.ymm * 1)
        }
        var remainSecs = Math.floor(QUAL.recoveredRemainingMs() / 1000)
        if (QUAL.recoveredPhaseId() === 3) {   // MatchPhase::OfficialMatch
            // Canonical sighting→match transition (clean face, officials-only
            // totals, match clock running). qualRecoveryInProgress suppresses
            // its QUAL.beginOfficialMatch() so the recovered journal is not
            // re-anchored — the frozen clock is rebased below instead.
            changedToMatchMode()
            for (i = 0; i < shots.length; ++i) {
                s = shots[i]
                if (s.isSighter === true)
                    continue
                p = centerPanel.polarForMm(s.xmm * 1, s.ymm * 1)
                rightPanel.addToSeries(p.x, p.y, s.calculatedscore * 1,
                                       s.xmm * 1, s.ymm * 1)
            }
            centerPanel.restoreMatchClockRemaining(remainSecs)
        } else {
            // Preparation+Sighting: resume the countdown from the frozen
            // remaining prep time (never the full 15 minutes).
            centerPanel.startPreparationCountdown()
            centerPanel.restorePrepCountdownRemaining(remainSecs)
        }
        // Live shot identity continues past every recovered externalId, so the
        // next live measurement becomes recovered-max+1 (and a duplicate
        // retransmission of the final pre-crash shot is refused by identity).
        qualShotSeq = Math.max(qualShotSeq, QUAL.recoveredMaxExternalId())
        qualRecoveryInProgress = false
        MODREADER.appendToLogFile("QUAL: recovered " + disciplineId + " session "
                                  + sessionId + " — officials "
                                  + QUAL.officialShotCount() + ", sighters "
                                  + QUAL.sighterCount() + ", remaining "
                                  + remainSecs + "s")
        return true
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

        // B1/B2: the official match clock has begun — journal the transition so
        // a recovered qualification session knows it is past sighting. Phase D:
        // suppressed during recovery replay — the recovered journal already
        // holds OfficialMatchStarted + its TimerStarted anchor, and re-emitting
        // them would restart the frozen match clock.
        if (qualDisciplineId !== "" && !qualRecoveryInProgress)
            QUAL.beginOfficialMatch()
    }

    function changedToMatchFinish()
    {
        matchFinished = true
        centerPanel.stopMatchClock()

        // B1/B2: record the qualification total and close the session cleanly
        // so it validates as a finished (non-recoverable) journal.
        if (qualDisciplineId !== "") {
            QUAL.completeMatch()
            QUAL.closeSession()
        }
        MODREADER.appendToLogFile("Match finished: " + globalMatchModel.count
                                  + "/" + matchShootCount + " match shots")
        // 3P: open the right-panel table + face on the final series for review of
        // all six series. Deferred so the matchFinished-driven paging bindings
        // have settled before we re-page.
        if (is3PMatch)
            Qt.callLater(rightPanel.showLastSeriesForReview)
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
