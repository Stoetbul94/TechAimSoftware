import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Window 2.2

Item {
    id: rootItem
    property int rootItemWidth: 1366
    property int rootItemHeight: 724

    property bool demoMode: true
    // Training Lab (T1): right-panel view state — 0 events, 1 catalogue,
    // 2 Technical Blocks setup. trainingConfirmed = setup accepted, Start
    // becomes "Start training →".
    property int practiceView: 0
    property bool trainingConfirmed: false
    property bool cdConfirmed: false        // T2: Call & Diagnose setup accepted
    property bool ptConfirmed: false        // T4: Position Transition setup accepted
    property bool wmConfirmed: false        // R2: Wind Map setup accepted (50m only)
    function trainingDisciplineId() {
        if (gameMode === 0) return "AP10"
        if (gameRange === 10) return "AR10"
        return gameSubMode === 0 ? "PRONE50" : "3P50"
    }
    // Wind Map is 50m Rifle only. This deliberately returns the CURRENT
    // selection rather than forcing a supported one — WINDMAP then refuses
    // it with a reason. Never coerce an unsupported discipline into a
    // supported one just to make the programme reachable.
    function windMapDisciplineId() { return trainingDisciplineId() }
    property bool connectToMaster: false
    property alias username_loginPage: name_text_field.text
    property int gameMode: 0 // 0 -> pistol, 1 -> rifle
    property int gameEvent: 0
    property int gameType: 1 // 1->sighter, 0->match
    property int papermode: 0
    property bool mod_connected: false
    property bool popupMode: false
    property int gameSubMode: 0  // 0=Prone/Air, 1=3 Positions (50m Rifle only)
    property bool showComportConnector: true
    property bool showLaneConnector: false
    property bool hideFreePractice: isDefaultIcon

    property color licColor: theme.brandPrimary

    // ── SETA hierarchical selection (product/seta) ───────────────────────────
    // setaSelection === true replaces the weapon / distance / event controls
    // with RULE SET → DISCIPLINE → PROGRAMME. The legacy controls are NOT
    // deleted: they are gated on !setaSelection and remain the rollback and
    // reference path until this has been through an integrated approval, after
    // which a separate cleanup can remove them. Only ONE of the two is ever
    // on screen — two live selectors would be worse than either alone.
    //
    // What the hierarchy does NOT own: the 3 Positions sub-discipline and the
    // two Finals are separate domains with no catalogue entry, so they stay on
    // their existing controls. Inventing catalogue entries for them would put
    // programmes in the hierarchy that the qualification engine cannot run.
    property bool setaSelection: PRODUCT.brandKey === "SETA"
    // Resolved once here, in top-level scope: an unqualified lookup of a
    // main.qml id from inside a nested handler resolves silently to undefined.
    property var setaCatalogue: competitionCatalogue
    property string setaProgrammeId: ""
    property bool setaBrowsing: false

    // The ONE place a hierarchy choice becomes machine state. Everything it
    // sets already existed; the catalogue supplies the mapping so there is no
    // second set of match-configuration rules. Nothing here runs while the
    // operator is browsing — back and forth through the levels cannot touch
    // the live configuration, because only a commit reaches this function.
    function applySetaProgramme(programmeId) {
        var cfg = setaCatalogue ? setaCatalogue.runtimeConfig(programmeId) : null
        if (cfg === null) return
        trainingConfirmed = false; cdConfirmed = false
        ptConfirmed = false;       wmConfirmed = false
        papermode = 0
        if (gameRange !== cfg.gameRange) rangeSelected(cfg.gameRange)
        gameMode    = cfg.gameMode
        gameSubMode = 0
        gameEvent   = cfg.gameEvent
        // gameMode/gameEvent normally drive updateGameType(); calling it
        // explicitly covers the case where only the RANGE changed, in which
        // case neither property emits and ShootingPage would keep the previous
        // range's shot count.
        if (typeof rootItem.updateGameType === "function") rootItem.updateGameType()
        setaProgrammeId = programmeId
        setaBrowsing = false
    }

    // The summary reads the LIVE configuration, not the last committed id, so
    // it can never disagree with what would actually be shot — including after
    // the 3 Positions or Finals controls change the event underneath it. There
    // is no separate "displayed programme" state to fall out of step.
    function setaProgrammeLabel() {
        if (gameEvent === 6 || gameEvent === 7) return qsTr("ISSF FINAL")
        var id = setaResolveCurrentProgrammeId()
        var p = (setaCatalogue && id !== "") ? setaCatalogue.profile(id) : null
        if (p === null) return qsTr("No programme selected")
        var label = qsTr(p.gameDisplay1Key) + " " + qsTr(p.gameDisplay2Key)
                    + "  ·  " + qsTr(p.matchDisplayKey)
        // 3 Positions is a position variant of the same 60-shot course, not a
        // different programme; say so rather than hiding it.
        if (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
            label += "  ·  " + qsTr("3 POSITIONS")
        return label
    }
    function setaProgrammeIsOfficial() {
        if (gameEvent === 6 || gameEvent === 7) return true
        var id = setaResolveCurrentProgrammeId()
        var p = (setaCatalogue && id !== "") ? setaCatalogue.profile(id) : null
        return p !== null && p.programmeType === "OFFICIAL"
    }
    // Identity of the CURRENT configuration, recomputed from stable numbers
    // only — never from a display string, which may have been translated.
    // Returns "" for the two finals events: they are a separate domain with no
    // catalogue entry, and mapping them onto one would be a lie.
    function setaResolveCurrentProgrammeId() {
        if (!setaCatalogue || gameEvent === 6 || gameEvent === 7) return ""
        var fifteen = APPSETTINGS.getIs15Shoot()
        var counts  = fifteen ? [10, 15, 20, 30, 40] : [10, 20, 30, 40, 60]
        var shots   = (gameEvent >= 0 && gameEvent <= 4) ? counts[gameEvent] : -1
        return setaCatalogue.legacyProgrammeId(gameRange, gameMode === 0, shots, fifteen)
    }

    // ── Design-system bindings (UI-1) ─────────────────────────────────────────
    // This page owned a private 13-colour palette — a local copy of values that
    // belong to the product, which is exactly why three competing brand reds
    // ended up shipping at once. The short names are KEPT because ~200 call
    // sites use them, but every one now resolves to a semantic token in
    // src/ui/theme/DesignTokens.qml. There is no hard-coded palette value in
    // this file any more; changing the brand is a token edit, not a page edit.
    //
    // APPROVED 2026-07-29: the accent is #A80038, sampled from the approved
    // logo images/logo/techaim_color.png. #C40046 becomes the hover state and
    // #80032A the pressed state. See docs/design/TechAim_Design_System.md.
    readonly property color _bg:         theme.tokens.backgroundPrimary
    readonly property color _bgHeader:   theme.tokens.backgroundSecondary
    readonly property color _surface:    theme.tokens.surfacePrimary
    readonly property color _surfaceAlt: theme.tokens.surfaceSecondary
    readonly property color _input:      theme.tokens.inputBackground
    readonly property color _borderSub:  theme.tokens.borderSubtle
    readonly property color _borderStr:  theme.tokens.borderStrong
    readonly property color _red:        theme.tokens.accentPrimary
    readonly property color _redHover:   theme.tokens.accentHover
    readonly property color _redPressed: theme.tokens.accentPressed
    readonly property color _redDark:    theme.tokens.accentSubtle
    readonly property color _txt:        theme.tokens.textPrimary
    readonly property color _txtSec:     theme.tokens.textSecondary
    readonly property color _txtMut:     theme.tokens.textDisabled
    readonly property color _green:      theme.tokens.successText
    readonly property color _okBg:       theme.tokens.successBackground
    readonly property color _errBg:      theme.tokens.errorBackground
    readonly property color _warnBg:     theme.tokens.warningBackground
    readonly property color _warnTxt:    theme.tokens.warningText
    readonly property color _onAccent:   theme.tokens.textOnAccent
    signal loadSavedGame()
    signal sighterStartedFromServer()
    signal matchStartedFromServer()
    signal backHomeFromServer()
    signal rangeSelected(int range)

    onGameModeChanged: { APPSETTINGS.setGameMode(gameMode) }
    onGameEventChanged: { APPSETTINGS.setGameEvent(gameEvent) }
    onGameSubModeChanged: { APPSETTINGS.setGameSubMode(gameSubMode) }
    onUsername_loginPageChanged: {
        APPSETTINGS.setUsername(username_loginPage)
    }
    onGameTypeChanged: {
        if (gameType === 0) shootingPage.loadGameInMatchMode()
    }

    Component.onCompleted: {
        if (gameRange == 10) {
            if (APPSETTINGS.getGame_distance() < 5 || APPSETTINGS.getGame_distance() > 10) {
                dialogManager.show({ "type": "error",
                    "title": qsTr("Invalid Distance"),
                    "message": qsTr("The configured distance is outside the supported range of 5 m to 10 m.\n\nThe application will now close."),
                    "onResult": function () { Qt.quit() } })
            }
        }
        MODREADER.connectedModbus()
        mod_connected = MODREADER.isModBusConnected()
        if (!MODREADER.isValidLicence()) {
            // dialogManager.show({ "type": "error", "title": qsTr("Licence Expired"),
            //     "message": qsTr("The software licence has expired.\n\nPlease contact TechAim support to renew."),
            //     "onResult": function () { Qt.quit() } })
        } else if (!mod_connected && popupMode) {
            modBusConnector.visible = true
        }
        name_text_field.text = MODREADER.getUserName()
        port_name_text_field.text = MODREADER.getPortNumber()
        netowrk_path_text.text = MODREADER.getNetworkPath()
        APPSETTINGS.setSetaSettingsFilePathFromQML(netowrk_path_text.text)
    }

    onVisibleChanged: { MODREADER.setOnLoginPage(visible) }

    // ─── Dialogs ─────────────────────────────────────────────────────────────

    ModConnectorDialog {
        id: modBusConnector
        width: 300; height: 170
        visible: false
    }

    // Popup messages migrated to the TechAim dialog framework
    // (dialogManager in main.qml) — no QtQuick.Dialogs MessageDialog left.

    Popup {
        id: popup
        width: 300; height: 300; modal: true; focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
    }

    // ─── Signal connections ───────────────────────────────────────────────────

    Connections {
        target: APPSETTINGS
        function onUserNameChanged(name) {
            username_loginPage = name
            name_text_field.text = name
        }
        function onPortNumberChanged(port) { port_name_text_field.text = port }
        function onLaneNumberChanged(lane_number) { lane_number_text = lane_number }
        function onStartSighter() {
            if (visible) perfromStart()
            sighterStartedFromServer()
        }
        function onStartMatch() {
            if (visible) perfromStart()
            matchStartedFromServer()
        }
        function onBackHome() {
            if (!visible) backHomeFromServer()
        }
    }

    Connections {
        target: MODREADER
        function onMasterConnectionChanged(isConnected) {
            if (APPSETTINGS.getDeveloperMode()) console.log("Master connection changed .....,", isConnected)
            disableControls()
        }
        function onMatchDetails(gametype, matchmode, sighterTime, matchtime, sigherTime, matchpf) {
            if (APPSETTINGS.getDeveloperMode()) console.log("Match Details in qml .....", gametype, matchmode, sighterTime, matchtime, sigherTime, matchpf)
            gameEvent = matchmode
            gameMode = gametype
            shootingPage.applyServerSettings(sighterTime, matchtime, sigherTime, matchpf)
        }
        function onStartMatchFromServer() {
            if (APPSETTINGS.getDeveloperMode()) console.log("Match Started .............")
            perfromStart()
        }
        function onMatchDetailsSetaModification(gametype, matchmode) {
            if (APPSETTINGS.getDeveloperMode()) console.log("Match Details in qml onMatchDetailsSetaModification .....", gametype, matchmode)
            gameEvent = matchmode
            gameMode = gametype
        }
    }

    // ─── Hidden functional IDs (required by JS logic and legacy cross-refs) ──
    // These are 1×1 invisible elements whose IDs must exist for the JS
    // functions and Connections handlers to compile and run correctly.

    Rectangle { id: bgRect; width: parent.width; height: parent.height; color: "transparent"; visible: false }
    // The legacy Tachus background was removed from the resource manifest.
    Image { id: bg; source: "qrc:/images/loginPage/login_page_29Oct.png"; anchors.fill: parent; visible: false }
    Image { id: bgRectImg; source: "qrc:/images/loginPage/bgRectImg.png"; anchors.fill: parent; visible: false }
    Image { id: red;   source: "qrc:/images/loginPage/red.png";   width: 1; height: 1; visible: false }
    Image { id: green; source: "qrc:/images/loginPage/green.png"; anchors.top: red.bottom; width: 1; height: 1; visible: false }
    Image { id: name;  source: "qrc:/images/loginPage/name.png";  width: 100; height: 30; visible: false }
    Image { id: name_drop_down; source: "qrc:/images/loginPage/combo_down.png"; anchors.right: name.right; anchors.top: name.top; width: 1; height: 1; visible: false }
    Image { id: lanenamebg;  source: "qrc:/images/loginPage/name.png"; width: 100; height: 30; visible: false }
    Image { id: portnamebg;  source: "qrc:/images/loginPage/name.png"; anchors.left: lanenamebg.left; width: 100; height: 30; visible: false }
    Image { id: shots_40_match;            source: "qrc:/images/loginPage/shots_40_match.png";            width: 1; height: 1; visible: false }
    Image { id: shots_40_match_text_field; source: "qrc:/images/loginPage/shots_40_match_text_field.png"; width: 1; height: 1; visible: false }
    Image { id: start;        source: "qrc:/images/loginPage/start.png";        x: 0; y: 0; width: 1; height: 1; visible: false }
    Image { id: start_over;   source: "qrc:/images/loginPage/start_over.png";   x: 0; y: 0; width: 1; height: 1; visible: false }
    Image { id: reset;        source: "qrc:/images/loginPage/reset.png";        width: 1; height: 1; visible: false }
    Image { id: reset_over;   source: "qrc:/images/loginPage/reset_over.png";   width: 1; height: 1; visible: false }
    Image { id: device_conhnected;      source: "qrc:/images/loginPage/device_conhnected.png";      width: 1; height: 1; visible: false }
    Image { id: device_conhnected_blue; source: "qrc:/images/loginPage/device_conhnected_blue.png"; width: 1; height: 1; visible: false }
    Image { id: demo;      source: "qrc:/images/loginPage/demo.png";      width: 1; height: 1; visible: false }
    Image { id: demo_over; source: "qrc:/images/loginPage/demo_over.png"; width: 1; height: 1; visible: false }
    MouseArea { id: demo_mouse; anchors.fill: demo_over; onClicked: { demoMode = !demoMode } }
    Image { id: ctm;      source: "qrc:/images/loginPage/demo.png";      width: 1; height: 1; visible: false }
    Image { id: ctm_over; source: "qrc:/images/loginPage/demo_over.png"; width: 1; height: 1; visible: false }
    MouseArea { id: ctm_mouse; anchors.fill: ctm_over; onClicked: { connectToMaster = !connectToMaster } }
    Image { id: masterConnectBtn; source: "qrc:/images/loginPage/start.png"; anchors.horizontalCenter: start.horizontalCenter; anchors.verticalCenter: lanenamebg.verticalCenter; width: 1; height: 1; visible: false }
    Image { id: open_saved_files;      source: "qrc:/images/loginPage/open_saved_files.png";      width: 1; height: 1; visible: false }
    Image { id: open_saved_files_crop; source: "qrc:/images/loginPage/save_29Oct.png";            anchors.left: open_saved_files.left; anchors.verticalCenter: open_saved_files.verticalCenter; width: 1; height: 1; visible: false }
    Image { id: open_saved_files_over; source: "qrc:/images/loginPage/open_saved_files_over.png"; x: open_saved_files.x; width: 1; height: 1; visible: false }
    Rectangle { id: open_network_files; anchors.left: networkSwitch.right; anchors.verticalCenter: open_saved_files.verticalCenter; width: 1; height: 1; color: "transparent"; visible: networkSwitch.checked }
    Image { id: license_details;       source: "qrc:/images/loginPage/license_details.png";       width: 1; height: 1; visible: false }
    Image { id: license_details_over;  source: "qrc:/images/loginPage/license_details_over.png";  width: 1; height: 1; visible: false }
    Image { id: contact_us;       source: "qrc:/images/loginPage/contact_us.png";       width: 1; height: 1; visible: false }
    Image { id: contact_us_crop;  source: "qrc:/images/loginPage/Contact us_29Oct.png"; anchors.right: contact_us.right; anchors.top: contact_us.top; width: 1; height: 1; visible: false }
    Image { id: contact_us_over;  source: "qrc:/images/loginPage/contact_us_over.png";  width: 1; height: 1; visible: false }
    Image { id: user_guide;       source: "qrc:/images/loginPage/user_guide.png";       width: 1; height: 1; visible: false }
    Image { id: user_guide_over;  source: "qrc:/images/loginPage/user_guide_over.png";  width: 1; height: 1; visible: false }
    Image { id: pistol;      source: "qrc:/images/loginPage/pistol.png";      width: 1; height: 1; visible: false }
    Image { id: pistol_over; source: "qrc:/images/loginPage/pistol_over.png"; width: 1; height: 1; visible: false }
    Image { id: rifle;       source: "qrc:/images/loginPage/rifle.png";       width: 1; height: 1; visible: false }
    Image { id: rifle_over;  source: "qrc:/images/loginPage/rifle_over.png";  width: 1; height: 1; visible: false }

    // Dummy rects used as anchor references
    Rectangle { id: com_port_dummy_rect; color: "transparent"; width: 1; height: 1 }
    Rectangle { id: lane_dummy_rect;     color: "transparent"; width: 1; height: 1 }
    Rectangle { id: temp_dummy_rect;     color: "transparent"; width: 1; height: 1 }

    // Functional inputs (lane unused in standard mode)
    TextInput { id: lane_name_text_field; visible: false }

    // Network switch (hidden — logic preserved)
    Switch {
        id: networkSwitch
        visible: false; checked: true
        onCheckedChanged: { MODREADER.setIsServerNetworkEnabled(checked) }
    }
    Text { id: netowrk_path_text; visible: false }

    FolderDialog {
        id: networkFolderDialog
        title: "Select Network Share Folder"
        onAccepted: {
            var p = selectedFolder.toString().replace(/^file:\/\/\//, "").replace(/\//g, "\\")
            netowrk_path_text.text = p
            MODREADER.saveNameAndPort(name_text_field.text, port_name_text_field.text, p)
        }
    }

    // Old event list/mouse kept for disableControls() compatibility
    ListView  { id: gameEventList;  width: 0; height: 0; visible: false; model: 0 }
    MouseArea { id: gameEventMouse; width: 0; height: 0 }
    MouseArea { id: resetMouse;     width: 0; height: 0; onClicked: rootItem.reset() }

    // ─── JS Functions ─────────────────────────────────────────────────────────

    function validate() {
        if (username_loginPage === "" && !isSaveGame) {
            dialogManager.showWarning(qsTr("User Name Required"),
                qsTr("Please enter a user name before logging in."))
            return false
        }
        return true
    }

    function reset() {
        username_loginPage = ""
        gameMode  = 0
        gameEvent = 0
        papermode = 0
    }

    function getGameEventText(index) {
        if (index === 6) return qsTr("FINAL")   // 3P FINAL (35) — isFinalsMatch domain
        if (index === 7) return qsTr("FINAL")   // 10m FINAL (24) — isFinals10mMatch domain
        if (APPSETTINGS.getIs15Shoot()) {
            if (index === 0) return qsTr("10")
            else if (index === 1) return qsTr("15")
            else if (index === 2) return qsTr("20")
            else if (index === 3) return qsTr("30")
            else if (index === 4) return qsTr("40")
            else return qsTr("Free Practice")
        } else {
            if (index === 0) return qsTr("10")
            else if (index === 1) return qsTr("20")
            else if (index === 2) return qsTr("30")
            else if (index === 3) return qsTr("40")
            else if (index === 4) return qsTr("60")
            else return qsTr("Free Practice")
        }
    }

    function getPaperModeText(index) {
        return index === 0 ? "Standard" : "Pro Mode"
    }

    function getDisciplineName() {
        if (gameMode === 0) return "10m Air Pistol"
        if (gameRange === 10) return "10m Air Rifle"
        return gameSubMode === 0 ? "50m Rifle Prone" : "50m Rifle 3 Pos"
    }

    function getEventCardTitle(index) {
        if (index === 7) return getDisciplineName() + " — FINAL (24)"
        var shots = getGameEventText(index)
        if (shots === "FINAL") return getDisciplineName() + " — FINAL (35)"
        if (shots === "Free Practice") return getDisciplineName() + " — Free Practice"
        return getDisciplineName() + " — " + shots + " Shots"
    }

    function getEventCardSubtitle(index) {
        if (index === 7) return "ISSF 10m Final · 24 shots · decimal · on command"
        var shots = getGameEventText(index)
        if (shots === "FINAL") return "ISSF Final · 35 shots · decimal · on command"
        if (shots === "Free Practice") return "Flexible training · no time limit"
        return "ISSF 2026 · " + shots + " shots"
    }

    function getEventCardBadge(index) {
        if (index === 7) return "F24"
        var shots = getGameEventText(index)
        if (shots === "FINAL") return "F35"
        return shots === "Free Practice" ? "FP" : shots
    }

    function getMatchTime() {
        var shots = getGameEventText(gameEvent)
        if (shots === "Free Practice") return "—"
        if (shots === "10") return "10 min"
        if (shots === "15") return "15 min"
        if (shots === "20") return "20 min"
        if (shots === "30") return "30 min"
        if (shots === "40") return "40 min"
        if (shots === "60") {
            if (gameMode === 0)      return "75 min"   // 10m Air Pistol  (ISSF 2026)
            if (gameRange === 10)    return "75 min"   // 10m Air Rifle   (ISSF 2026)
            if (gameSubMode === 1)   return "90 min"   // 50m Rifle 3 Pos on EST (ISSF 2026)
            return "50 min"                            // 50m Rifle Prone (ISSF 2026)
        }
        return "—"
    }

    // ── AUTHORITATIVE SELECTED-PROGRAMME STATE ────────────────────────────
    // One source of truth for "what is selected". Previously the Selected
    // Profile label, the Start button wording and the action-bar recap each
    // derived this independently, which is how the panel could show
    // "10m Air Pistol — ISSF" while an Open Practice card was highlighted:
    // the label always said "<discipline> — ISSF" regardless of the event.
    //
    // These are PRESENTATION only. The controller dispatch in the Start
    // handler is unchanged and still keys off ptConfirmed / cdConfirmed /
    // trainingConfirmed exactly as before.

    // "POSTRANS" | "CALLDIAG" | "TRAINING" | "FINAL" | "OFFICIAL" | "PRACTICE"
    function selectedProgrammeKind() {
        if (wmConfirmed)       return "WINDMAP"
        if (ptConfirmed)       return "POSTRANS"
        if (cdConfirmed)       return "CALLDIAG"
        if (trainingConfirmed) return "TRAINING"
        if (gameEvent === 6 || gameEvent === 7) return "FINAL"
        if (gameEvent === 4)   return "OFFICIAL"
        return "PRACTICE"
    }

    // The name shown in Selected Profile AND in the action-bar recap.
    function selectedProgrammeName() {
        var k = selectedProgrammeKind()
        if (k === "WINDMAP")  return qsTr("Wind Map")
        if (k === "POSTRANS") return qsTr("Position Transition")
        if (k === "CALLDIAG") return qsTr("Call & Diagnose")
        if (k === "TRAINING") return qsTr("Technical Blocks")
        if (k === "FINAL")    return getEventCardTitle(gameEvent)
        if (k === "OFFICIAL") return getDisciplineName() + qsTr(" — ISSF")
        // Practice is NOT an ISSF programme and must not claim to be.
        return getDisciplineName() + qsTr(" — Open Practice")
    }

    // The section heading above the summary.
    function selectedProgrammeLabel() {
        var k = selectedProgrammeKind()
        if (k === "WINDMAP" || k === "POSTRANS" || k === "CALLDIAG" || k === "TRAINING")
            return qsTr("SELECTED PROGRAMME")
        if (k === "FINAL")    return qsTr("SELECTED FINAL")
        if (k === "PRACTICE") return qsTr("SELECTED PRACTICE")
        return qsTr("SELECTED MATCH")
    }

    // Start button wording — derived from the same kind, so it can never
    // disagree with the summary.
    function startButtonText() {
        var k = selectedProgrammeKind()
        if (k === "WINDMAP")  return qsTr("Start wind map  →")
        if (k === "POSTRANS") return qsTr("Start transitions  →")
        if (k === "CALLDIAG") return qsTr("Start calling  →")
        if (k === "TRAINING") return qsTr("Start training  →")
        if (k === "PRACTICE") return qsTr("Start practice  →")
        return qsTr("Start session  →")
    }

    // ── READINESS ─────────────────────────────────────────────────────────
    // Network sharing with no destination folder is not a working state: it
    // reported "Share enabled" while nothing could be written. Sharing is only
    // genuinely configured when it is switched on AND has a folder.
    readonly property bool shareRequested: networkShareCard.netEnabled
    readonly property bool shareConfigured: networkShareCard.netEnabled
                                            && netowrk_path_text.text !== ""
    readonly property bool shareIncomplete: networkShareCard.netEnabled
                                            && netowrk_path_text.text === ""

    // Advisory only — it never blocks Start. Sharing is a convenience, not a
    // precondition for shooting, so an incomplete share must not stop a match.
    readonly property bool readinessOk: !shareIncomplete

    function readinessSummary() {
        if (shareIncomplete)
            return qsTr("Network share is on but no folder is selected — results will not be shared.")
        var who = username_loginPage !== "" ? username_loginPage
                                            : qsTr("No athlete entered")
        return who + "   ·   " + selectedProgrammeName()
             + "   ·   " + (appMode ? qsTr("Live target") : qsTr("Demo / Simulation"))
    }

    function disableControls() {
        if (APPSETTINGS.getDeveloperMode()) console.log("Inside disable controls ....")
        pistolMouse.visible   = false
        rifleMouse.visible    = false
        rifle50Mouse.visible  = false
        startMouse.visible   = false
        resetMouse.visible   = false
        gameEventList.enabled = false
        gameEventMouse.visible = false
    }

    function startButtonClickedOnLoadGame() {
        if (APPSETTINGS.getDeveloperMode()) console.log("app mode " + appMode)
        if (!appMode) {
            rootItem.visible = false
        } else {
            if (!mod_connected) {
                if (popupMode) modBusConnector.visible = true
            } else if (validate()) {
                rootItem.visible = false
            }
        }
    }

    function perfromStart() {
        if (!appMode) {
            MODREADER.appendToLogFile("Application running in demo mode")
            if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                dialogManager.showError(qsTr("Master Not Connected"),
                    qsTr("The master system is not connected.\n\nPlease press \u201CConnect\u201D and try again."))
                return
            }
            rootItem.visible = false
        } else {
            MODREADER.appendToLogFile("Application running in Live mode")
            if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                MODREADER.appendToLogFile("Master application required")
                dialogManager.showError(qsTr("Master Not Connected"),
                    qsTr("The master system is not connected.\n\nPlease press \u201CConnect\u201D and try again."))
                return
            }
            if (masterConnectBtn && port_name_text_field.text != "") {
                MODREADER.appendToLogFile("Application with port text field")
                MODREADER.connectedModbus(port_name_text_field.text)
                mod_connected = MODREADER.isModBusConnected()
            }
            if (!MODREADER.isModBusConnected()) {
                MODREADER.appendToLogFile("Com port not connected")
                dialogManager.showError(qsTr("COM Port Not Connected"),
                    qsTr("No connection to the target COM port was found.\n\nPlease connect the target hardware and try again."))
            } else if (!MODREADER.isHardwareConnected()) {
                dialogManager.showError(qsTr("Hardware Not Connected"),
                    qsTr("The target hardware is not responding.\n\nPlease check the target connection and try again."))
            } else if (!MODREADER.checkAutoFeedMode()) {
                // Auto-feed notice was never shown (the legacy dialog was
                // set visible = false) — preserved as a silent branch.
            } else if (validate()) {
                MODREADER.appendToLogFile("Validation was successful")
                rootItem.visible = false
            } else {
                MODREADER.appendToLogFile("Com-port connected but validation failed")
                // This branch used to log and show NOTHING. Pressing Start
                // Practice simply did nothing, with no reason given - observed
                // during emulator bring-up on 2026-08-09. A control that
                // silently refuses is indistinguishable from a frozen
                // application. The reason comes from the SAME authoritative
                // target state the status panel binds to, not a second guess.
                dialogManager.showError(qsTr("Target Not Ready"),
                    qsTr("A valid electronic target connection is required before "
                       + "starting this Live session.\n\nTarget status: %1%2\n\n"
                       + "Check the USB connection and try again.")
                        .arg(MODREADER.targetState)
                        .arg(MODREADER.targetPort !== ""
                             ? qsTr(" (%1)").arg(MODREADER.targetPort) : ""))
            }
        }
        APPSETTINGS.saveMatch(true)
    }

    // =========================================================================
    // VISUAL LAYOUT
    // =========================================================================

    Rectangle { anchors.fill: parent; color: _bg }

    // ── Header (56 px — session title only) ──────────────────────────────────
    // UI-B: the app-identity row that used to live here (target icon,
    // "TECH AIM", "ELECTRONIC TARGET CONTROL") duplicated the shell Header,
    // which already renders the logo and wordmark directly above this bar. Two
    // stacked brand rows read as a rendering fault and cost ~20 px on a screen
    // that was already clipping its content, so this bar now carries only the
    // page title and the operating-mode badge.
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        color: _bgHeader

        Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: _red }
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: _borderSub }

        // UI-HOME-006: the subtle logo that used to sit here is gone. The shell
        // Header directly above this bar already carries the Tech Aim mark, so
        // this was the product's second logo within ~40 vertical pixels.
        // UI-DEC-005: one primary Tech Aim logo, in the application shell.

        // Session title row
        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 20
            height: 30; spacing: 12

            Text {
                text: "Start session"; color: _txt
                font.family: theme.fontFamily; font.pixelSize: 22; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            // UI-HOME-005: the LIVE/DEMO badge that used to sit beside the title
            // is gone. Operating mode is stated in Session setup (where it can
            // also be CHANGED) and once more in the footer status strip; a third
            // read-only copy in the page heading was noise.
            //
            // Deliberately kept elsewhere: Demo mode must stay unmissable,
            // because mistaking a Demo session for a Live one is a
            // result-integrity risk, not a cosmetic one. Two indicators remain.
        }
    }

    // ── Two-column content area ───────────────────────────────────────────────
    Item {
        id: contentArea
        anchors.top: headerBar.bottom; anchors.topMargin: 10
        anchors.left: parent.left;     anchors.leftMargin: 12
        anchors.right: parent.right;   anchors.rightMargin: 12
        anchors.bottom: parent.bottom; anchors.bottomMargin: 10

        // ── LEFT PANEL ────────────────────────────────────────────────────────
        Rectangle {
            id: leftPanel
            anchors.top: parent.top; anchors.left: parent.left
            anchors.bottom: actionBar.top; anchors.bottomMargin: 8
            width: Math.floor(parent.width * 0.44)
            color: _surface; radius: 10
            border.color: _borderSub; border.width: 1; clip: true

            Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: _red; radius: 2 }

            Text {
                id: panelTitle
                anchors.top: parent.top; anchors.topMargin: 18
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "Session setup"
                color: _txt; font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
            }

            // UI-B: the setup fields used to be one rigid anchor chain pinned
            // directly to the panel. Because the chain has no upper bound, on a
            // shorter window it simply overflowed and `clip: true` swallowed
            // whatever fell off the end — which is how the Start button became
            // invisible. Scrolling the fields makes overflow impossible at any
            // window height, and lets the action bar own the bottom of the page.
            Flickable {
                id: setupScroll
                anchors.top: panelTitle.bottom; anchors.topMargin: 14
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom; anchors.bottomMargin: 16
                clip: true
                contentWidth: width
                contentHeight: setupInner.height
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: setupScroll.contentHeight > setupScroll.height
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

            // The inner Item keeps the original full-panel width, so every
            // child's existing 22 px left/right margins resolve to the same
            // insets they had before.
            Item {
                id: setupInner
                width: setupScroll.width
                height: summaryCard.y + summaryCard.height + 4

            // ── ATHLETE ───────────────────────────────────────────────────────
            Text {
                id: athleteLabel
                anchors.top: parent.top; anchors.topMargin: 0
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "ATHLETE"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
            }
            Rectangle {
                id: athleteBox
                anchors.top: athleteLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 52; color: _input; radius: 6
                border.color: name_text_field.activeFocus ? _red : _borderSub
                border.width: name_text_field.activeFocus ? 2 : 1

                TextInput {
                    id: name_text_field
                    anchors.left: parent.left;  anchors.leftMargin: 14
                    anchors.right: historyBtn.left; anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: theme.fontFamily; font.pixelSize: 14
                    color: _txt; maximumLength: 20
                    onTextChanged: { username_loginPage = text }
                }
                // UI-B: TextInput has no placeholderText, so a fresh install
                // showed an unlabelled empty box with no hint that a name is
                // wanted. This is display only — it never contributes text.
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 15
                    anchors.verticalCenter: parent.verticalCenter
                    visible: name_text_field.text === "" && !name_text_field.activeFocus
                    text: qsTr("Athlete name")
                    color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 14
                }
                Rectangle {
                    id: historyBtn
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 36; color: "transparent"
                    Text { text: "▾"; color: _txtMut; font.pixelSize: 14; anchors.centerIn: parent }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { if (APPSETTINGS.getUserHistoryCount() > 0) userHistoryList.visible = !userHistoryList.visible }
                    }
                }
            }
            ListView {
                id: userHistoryList
                anchors.top: athleteBox.bottom
                anchors.left: athleteBox.left; anchors.right: athleteBox.right
                height: Math.min(count, 4) * 52
                visible: false; clip: true; z: 20
                model: APPSETTINGS.getUserHistoryCount()
                onVisibleChanged: { model = 0; model = APPSETTINGS.getUserHistoryCount() }
                delegate: Rectangle {
                    width: userHistoryList.width; height: 52
                    color: _surfaceAlt; border.color: _borderSub; border.width: 1
                    Text {
                        anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 14
                        text: APPSETTINGS.getUserHistoryData(index)
                        color: _txt; font.family: theme.fontFamily; font.pixelSize: 14
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: parent.color = _borderSub
                        onExited:  parent.color = _surfaceAlt
                        onClicked: { username_loginPage = APPSETTINGS.getUserHistoryData(index); userHistoryList.visible = false }
                    }
                }
            }

            // ── TARGET CONNECTION ─────────────────────────────────────────────
            Text {
                id: connLabel
                anchors.top: athleteBox.bottom; anchors.topMargin: 16
                anchors.left: parent.left; anchors.leftMargin: 22
                // Named as the fallback it is. The box below carries a saved
                // value and showed "COM7" on a machine with no COM7 - reading
                // like an active connection when the target was absent. The
                // authoritative state is the panel underneath.
                text: "TARGET CONNECTION — MANUAL PORT FALLBACK"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                visible: showComportConnector
            }
            Row {
                id: connRow
                anchors.top: connLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 52; spacing: 8
                visible: showComportConnector

                Rectangle {
                    width: parent.width - connStatusBtn.width - 8; height: 52
                    color: _input; radius: 6
                    border.color: port_name_text_field.activeFocus ? _red : _borderSub
                    border.width: port_name_text_field.activeFocus ? 2 : 1
                    TextInput {
                        id: port_name_text_field
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: theme.fontFamily; font.pixelSize: 14
                        color: _txt; maximumLength: 5
                    }
                }
                Rectangle {
                    id: connStatusBtn
                    width: 148; height: 52; radius: 6
                    color: mod_connected ? _okBg : _surfaceAlt
                    border.color: mod_connected ? _green : _borderSub; border.width: 1
                    Row {
                        anchors.centerIn: parent; spacing: 7
                        Rectangle {
                            width: 7; height: 7; radius: 4
                            color: mod_connected ? _green : _txtMut
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            // F11: this button is the target CONNECTION toggle, not the
                            // operating-mode switch (that is the OPERATING MODE control
                            // below). Label reflects connection state only.
                            text: mod_connected ? "Connected" : (appMode ? "Not connected" : "Demo \u00b7 not needed")
                            color: mod_connected ? _green : _txtMut
                            font.family: theme.fontFamily; font.pixelSize: 11; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!mod_connected) {
                                // Reconnect: use the typed COM port if given,
                                // otherwise auto-detect. (Was gated behind
                                // popupMode, which made re-enabling impossible.)
                                if (port_name_text_field.text !== "")
                                    MODREADER.connectedModbus(port_name_text_field.text)
                                else
                                    MODREADER.connectedModbus()
                                mod_connected = MODREADER.isModBusConnected()
                                if (!mod_connected)
                                    modBusConnector.visible = true
                            } else {
                                MODREADER.disconnectModbus()
                                mod_connected = MODREADER.isModBusConnected()
                            }
                        }
                    }
                }
            }

            // ── AUTHORITATIVE TARGET STATUS ───────────────────────────────────
            // The text box above is the MANUAL FALLBACK for typing a port. It is
            // not the truth: it showed "COM7" on a machine where no COM7 exists,
            // because it carries a remembered settings value. This panel binds
            // to the live engine state and shows the device and port ACTUALLY
            // connected, so an operator can tell at a glance what they are
            // really talking to before they start shooting.
            TargetStatusPanel {
                id: loginTargetStatus
                anchors.top: connRow.bottom; anchors.topMargin: 8
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                visible: showComportConnector
            }

            // ── OPERATING MODE (F11 fix) ──────────────────────────────────────
            // The operator-facing Live/Demo switch. Placed HERE (the idle
            // Start-session screen) because changing mode is only permitted when
            // no session is active; the in-session Settings selector is blocked.
            // Uses OPMODE + the same confirm/restart flow. Read-only display
            // falls back to appMode if OPMODE is unavailable.
            Text {
                id: opModeSectionLabel
                // Anchored below the status panel, not the manual-entry row, so
                // the authoritative target state cannot be overlapped.
                anchors.top: showComportConnector ? loginTargetStatus.bottom : athleteBox.bottom
                anchors.topMargin: 16
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "OPERATING MODE"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
            }
            Row {
                id: opModeRow
                anchors.top: opModeSectionLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 52; spacing: 8
                property bool opLive: (typeof OPMODE !== "undefined") ? OPMODE.live : appMode

                // Live target pill
                Rectangle {
                    width: (parent.width - parent.spacing) / 2; height: 52; radius: 6
                    color: opModeRow.opLive ? _okBg : _input
                    border.color: opModeRow.opLive ? _green : _borderSub
                    border.width: opModeRow.opLive ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 1
                        Text { text: "LIVE TARGET"; color: opModeRow.opLive ? _green : _txt
                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                               anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: "Physical target"; color: _txtMut
                               font.family: theme.fontFamily; font.pixelSize: theme.type.helperText.size
                               anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: !opModeRow.opLive && typeof OPMODE !== "undefined"
                        onClicked: { OPMODE.selectMode(0); opModeConfirm.targetMode = 0; opModeConfirm.open() }
                    }
                }
                // Demo pill
                Rectangle {
                    width: (parent.width - parent.spacing) / 2; height: 52; radius: 6
                    color: !opModeRow.opLive ? _errBg : _input
                    border.color: !opModeRow.opLive ? _red : _borderSub
                    border.width: !opModeRow.opLive ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 1
                        Text { text: "DEMO / SIMULATION"; color: !opModeRow.opLive ? _red : _txt
                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                               anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: "Simulated clicks"; color: _txtMut
                               font.family: theme.fontFamily; font.pixelSize: theme.type.helperText.size
                               anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: opModeRow.opLive && typeof OPMODE !== "undefined"
                        onClicked: { OPMODE.selectMode(1); opModeConfirm.targetMode = 1; opModeConfirm.open() }
                    }
                }
            }
            Text {
                id: opModeHint
                anchors.top: opModeRow.bottom; anchors.topMargin: 3
                anchors.left: parent.left; anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                wrapMode: Text.WordWrap
                font.family: theme.fontFamily
                font.pixelSize: theme.type.helperText.size
                color: (typeof OPMODE !== "undefined" && OPMODE.restartRequired) ? _warnTxt : _txtSec
                text: (typeof OPMODE !== "undefined" && OPMODE.restartRequired)
                      ? "Restart required — the selected mode takes effect on next launch."
                      : "Switch the target source. Changing mode requires an application restart."
            }

            // Confirm dialog (Restart Now / Later / Cancel) — mirrors Settings.
            Popup {
                id: opModeConfirm
                property int targetMode: 1
                parent: Overlay.overlay
                anchors.centerIn: Overlay.overlay
                modal: true; focus: true
                closePolicy: Popup.CloseOnEscape
                width: 380; padding: 0
                background: Rectangle { color: _surfaceAlt; radius: 13; border.color: _borderSub; border.width: 1 }
                Overlay.modal: Rectangle { color: theme.tokens.scrim }
                contentItem: Column {
                    spacing: 12; padding: 22; width: opModeConfirm.width
                    Text {
                        width: parent.width - 44
                        text: opModeConfirm.targetMode === 1 ? "Switch to Demo mode?" : "Switch to Live target mode?"
                        color: _txt; font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        width: parent.width - 44
                        text: opModeConfirm.targetMode === 1
                              ? "Simulated shots will be enabled. Demo sessions are intended for testing and cannot be treated as Live target results.\n\nThe application must restart before the change takes effect."
                              : "Simulated shot input will be disabled. The application will expect the physical TechAim target connection.\n\nThe application must restart before the change takes effect."
                        color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11; wrapMode: Text.WordWrap
                    }
                    Item {
                        width: parent.width - 44; height: 34
                        Row {
                            anchors.right: parent.right; spacing: 8
                            Rectangle {
                                width: 74; height: 32; radius: 8; color: "transparent"
                                border.color: _borderSub; border.width: 1
                                Text { anchors.centerIn: parent; text: "Cancel"; color: _txt
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent
                                    onClicked: { if (typeof OPMODE !== "undefined") OPMODE.selectMode(opModeRow.opLive ? 0 : 1); opModeConfirm.close() } }
                            }
                            Rectangle {
                                width: 104; height: 32; radius: 8; color: _surfaceAlt
                                border.color: _borderSub; border.width: 1
                                Text { anchors.centerIn: parent; text: "Restart Later"; color: _txt
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent
                                    onClicked: { OPMODE.applyModeChange(false); opModeConfirm.close() } }
                            }
                            Rectangle {
                                width: 104; height: 32; radius: 8; color: _red
                                Text { anchors.centerIn: parent; text: "Restart Now"; color: "white"
                                       font.family: theme.fontFamily; font.pixelSize: 11; font.bold: true }
                                MouseArea { anchors.fill: parent
                                    onClicked: { if (OPMODE.applyModeChange(false)) OPMODE.requestRestart(); opModeConfirm.close() } }
                            }
                        }
                    }
                }
            }

            // ── NETWORK SHARE ─────────────────────────────────────────────────
            Text {
                id: networkSectionLabel
                anchors.top: opModeHint.bottom
                anchors.topMargin: 14
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "NETWORK SHARE"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
            }
            Rectangle {
                id: networkShareCard
                anchors.top: networkSectionLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 56; color: _input; radius: 6
                // Incomplete is a WARNING state, not a success state. The card
                // used to show an enabled accent border while nothing could
                // actually be written anywhere.
                border.color: shareIncomplete ? _warnTxt
                                              : (shareConfigured ? _red : _borderSub)
                border.width: 1
                // Start OFF unless a destination folder already exists.
                // "Share enabled / No folder selected" was never a working
                // configuration, and it is not a sensible default either.
                property bool netEnabled: netowrk_path_text.text !== ""

                Row {
                    id: netInfoRow
                    anchors.left: parent.left; anchors.leftMargin: 12
                    anchors.right: netToggleTrack.left; anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter; spacing: 10
                    Text {
                        text: "☁"; font.pixelSize: 16
                        color: shareIncomplete ? _warnTxt
                                               : (shareConfigured ? _red : _txtMut)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text {
                            text: shareIncomplete ? qsTr("Share incomplete")
                                 : (shareConfigured ? qsTr("Share enabled")
                                                    : qsTr("Share disabled"))
                            color: shareIncomplete ? _warnTxt
                                                   : (shareConfigured ? _txt : _txtSec)
                            font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                        }
                        Text {
                            text: netowrk_path_text.text !== ""
                                  ? netowrk_path_text.text
                                  : qsTr("No folder selected — click to choose")
                            color: netowrk_path_text.text !== "" ? _txtMut : _warnTxt
                            font.family: theme.fontFamily
                            font.pixelSize: theme.type.helperText.size
                            elide: Text.ElideMiddle; width: networkShareCard.width - 100
                        }
                    }
                }
                MouseArea {
                    anchors.left: parent.left
                    anchors.right: netToggleTrack.left; anchors.rightMargin: 10
                    anchors.top: parent.top; anchors.bottom: parent.bottom
                    cursorShape: Qt.PointingHandCursor
                    onClicked: networkFolderDialog.open()
                }
                Rectangle {
                    id: netToggleTrack
                    anchors.right: parent.right; anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 46; height: 26; radius: 13
                    color: networkShareCard.netEnabled ? _red : _borderStr
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        x: networkShareCard.netEnabled ? parent.width - width - 3 : 3
                        width: 20; height: 20; radius: 10; color: "white"
                        Behavior on x { NumberAnimation { duration: 120 } }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            networkShareCard.netEnabled = !networkShareCard.netEnabled
                            MODREADER.setIsServerNetworkEnabled(networkShareCard.netEnabled)
                            // Sharing without a destination folder is a no-op
                            // (and used to freeze the app) — prompt for one.
                            if (networkShareCard.netEnabled && MODREADER.getNetworkPath() === "")
                                networkFolderDialog.open()
                        }
                    }
                }
            }

            // ── SELECTED PROGRAMME SUMMARY ────────────────────────────────────
            // UI-B: this was a heading plus six individually-bordered stat tiles,
            // all shouting at the same volume. It is now ONE card: the programme
            // name reads first, and the six values sit below a divider as quiet
            // label/value pairs. Identical data, identical expressions — only the
            // visual weight changed.
            Rectangle {
                id: summaryCard
                anchors.top: networkShareCard.bottom; anchors.topMargin: 18
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: summaryInner.implicitHeight + 28
                color: _surfaceAlt; radius: 8
                border.color: _borderSub; border.width: 1

                Column {
                    id: summaryInner
                    anchors.left: parent.left;   anchors.leftMargin: 14
                    anchors.right: parent.right; anchors.rightMargin: 14
                    anchors.top: parent.top;     anchors.topMargin: 14
                    spacing: 4

                    Text {
                        id: profileLabel
                        text: selectedProgrammeLabel()
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: theme.type.label.size
                        font.bold: true; font.letterSpacing: theme.type.label.spacing
                    }
                    Text {
                        id: profileName
                        text: selectedProgrammeName()
                        color: _txt; font.family: theme.fontFamily; font.pixelSize: 17; font.bold: true
                        width: parent.width; elide: Text.ElideRight
                    }
                    Text {
                        visible: trainingConfirmed && trainingDisciplineId() === "3P50"
                        text: "POSITION FLOW   Kneeling → Prone → Standing"
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: theme.type.helperText.size; font.letterSpacing: 1
                    }
                    Item { width: 1; height: 6 }
                    Rectangle { width: parent.width; height: 1; color: _borderSub }
                    Item { width: 1; height: 8 }

            // ── INFO GRID (T1: programme summary when Training confirmed) ─────
            Grid {
                id: infoTiles
                width: parent.width
                columns: 3; rowSpacing: 12; columnSpacing: 10
                Repeater {
                    model: wmConfirmed ? [
                        { lbl: "DISCIPLINE", val: WINDMAP.threePositions ? "50m 3 Pos" : "50m Prone" },
                        { lbl: "SHOT PLAN",  val: "" + WINDMAP.shotPlan + " shots" },
                        { lbl: "SIGHTERS",   val: WINDMAP.sightersEnabled ? "Yes" : "No" },
                        { lbl: "WIND",       val: "Manual entry" },
                        { lbl: "POSITIONS",  val: WINDMAP.threePositions ? "K · P · S separate" : "Single" },
                        { lbl: "MODE",       val: appMode ? "Live" : "Demo" }
                    ] : ptConfirmed ? [
                        { lbl: "SEQUENCE",  val: POSTRANS.sequenceString() },
                        { lbl: "VERIFY",    val: "" + POSTRANS.verificationShots + " / pos" },
                        { lbl: "REPEATS",   val: "" + POSTRANS.totalRepeats },
                        { lbl: "FOCUS",     val: POSTRANS.technicalFocus },
                        { lbl: "CHECKLIST", val: ["Self", "Coach", "Off"][POSTRANS.checklistMode] },
                        { lbl: "MODE",      val: appMode ? "Live" : "Demo" }
                    ] : cdConfirmed ? [
                        { lbl: "CALLED",    val: "" + CALLDIAG.shotCount + (trainingDisciplineId() === "3P50" ? " / pos" : "") },
                        { lbl: "TOTAL",     val: "" + (trainingDisciplineId() === "3P50" ? CALLDIAG.shotCount * 3 : CALLDIAG.shotCount) },
                        { lbl: "FOCUS",     val: CALLDIAG.technicalFocus },
                        { lbl: "REVEAL",    val: "After each call" },
                        { lbl: "EST. TIME", val: CALLDIAG.estimatedTime },
                        { lbl: "MODE",      val: appMode ? "Live" : "Demo" }
                    ] : trainingConfirmed ? [
                        { lbl: "BLOCKS",     val: "" + TRAINING.blockCount },
                        { lbl: "SHOTS/BLOCK", val: "" + TRAINING.shotsPerBlock },
                        { lbl: "TOTAL",      val: "" + (TRAINING.blockCount * TRAINING.shotsPerBlock) },
                        { lbl: "FOCUS",      val: TRAINING.technicalFocus },
                        { lbl: "VISIBILITY", val: ["Full hidden", "Group only", "Impact only"][TRAINING.visibilityMode] },
                        { lbl: "EST. TIME",  val: TRAINING.estimatedTime }
                    ] : [
                        { lbl: "SHOT PLAN", val: getGameEventText(gameEvent) === "Free Practice" ? "Free" : getGameEventText(gameEvent) + " shots" },
                        { lbl: "SCORING",   val: (gameMode === 0 || (gameMode === 1 && gameRange === 50 && gameSubMode === 1)) ? "Integer" : "Decimal" },
                        { lbl: "PREP",      val: "15 min" },
                        { lbl: "MATCH",     val: getMatchTime() },
                        { lbl: "DISTANCE",  val: gameRange + " m" },
                        { lbl: "RULES",     val: "ISSF 2026" }
                    ]
                    delegate: Column {
                        width: (infoTiles.width - infoTiles.columnSpacing * 2) / 3
                        spacing: 3
                        Text {
                            text: modelData.lbl; color: _txtMut
                            font.family: theme.fontFamily; font.pixelSize: theme.type.helperText.size
                            font.bold: true; font.letterSpacing: 1.4
                        }
                        Text {
                            text: modelData.val; color: _txt
                            font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true
                            width: parent.width; elide: Text.ElideRight
                        }
                    }
                }
            }

                }   // summaryInner
            }       // summaryCard
            }       // setupInner
            }       // setupScroll

        } // leftPanel

        // ── RIGHT PANEL ───────────────────────────────────────────────────────
        Rectangle {
            id: rightPanel
            anchors.top: parent.top; anchors.right: parent.right
            anchors.bottom: actionBar.top; anchors.bottomMargin: 8
            anchors.left: leftPanel.right; anchors.leftMargin: 10
            color: _surface; radius: 10
            border.color: _borderSub; border.width: 1; clip: true

            Text {
                id: rightTitle
                anchors.top: parent.top; anchors.topMargin: 20
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "Choose an event"
                color: _txt; font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
            }
            Text {
                id: rightSubtitle
                anchors.top: rightTitle.bottom; anchors.topMargin: 3
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "Match settings are applied automatically."
                color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11
            }

            // ── SETA: RULE SET → DISCIPLINE → PROGRAMME ──────────────────────
            // Collapses to zero height on the legacy path, so the layout below
            // is untouched when setaSelection is false.
            Item {
                id: setaBlock
                anchors.top: rightSubtitle.bottom
                anchors.topMargin: setaSelection ? 14 : 0
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: (!setaSelection || practiceView !== 0) ? 0
                        : (setaBrowsing ? Math.max(280, rightPanel.height - 210) : 68)
                visible: height > 0
                clip: true

                // Committed state: what is selected, and one way to change it.
                Rectangle {
                    anchors.fill: parent
                    visible: !setaBrowsing
                    radius: 8
                    color: _surfaceAlt
                    border.color: setaProgrammeIsOfficial() ? _red : _borderSub
                    border.width: setaProgrammeIsOfficial() ? 2 : 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3
                        Text {
                            text: setaProgrammeLabel()
                            color: _txt; font.family: theme.fontFamily
                            font.pixelSize: 13; font.bold: true
                        }
                        Text {
                            // Rule authority is stated, never implied.
                            text: setaProgrammeIsOfficial()
                                  ? qsTr("Official ISSF course")
                                  : qsTr("Practice - no rule authority")
                            color: setaProgrammeIsOfficial() ? _green : _txtMut
                            font.family: theme.fontFamily; font.pixelSize: 10
                        }
                    }
                    Rectangle {
                        id: setaChangeBtn
                        anchors.right: parent.right; anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 92; height: 32; radius: 6
                        color: setaChangeMouse.pressed ? _redPressed : _input
                        border.color: _borderSub; border.width: 1
                        Text {
                            anchors.centerIn: parent; text: qsTr("CHANGE")
                            color: _txtSec; font.family: theme.fontFamily
                            font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
                        }
                        MouseArea {
                            id: setaChangeMouse
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { setaSelector.reset(); setaBrowsing = true }
                        }
                    }
                }

                // Browsing state. Nothing here writes to the shooting
                // configuration; only programmeCommitted does.
                SetaCompetitionSelector {
                    id: setaSelector
                    anchors.fill: parent
                    visible: setaBrowsing
                    catalogue: setaCatalogue
                    fifteenShotMode: APPSETTINGS.getIs15Shoot()
                    bg: _surface; card: _surfaceAlt; cardSel: _input
                    accent: _red;  line: _borderSub
                    textPrimary: _txt; textSecondary: _txtSec
                    textMuted: _txtMut; textOfficial: _green
                    onProgrammeCommitted: function (programmeId) {
                        applySetaProgramme(programmeId)
                    }
                }
            }

            // Weapon selector (PISTOL | RIFLE) — LEGACY PATH, kept for rollback
            Row {
                id: weaponRow
                visible: !setaSelection
                anchors.top: setaBlock.bottom; anchors.topMargin: setaSelection ? 0 : 14
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: setaSelection ? 0 : 58; spacing: 8

                Rectangle {
                    width: (parent.width - 8) / 2; height: 58; radius: 8
                    color: gameMode === 0 ? _redDark : _surfaceAlt
                    border.color: gameMode === 0 ? _red : _borderSub
                    border.width: gameMode === 0 ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 2
                        Text {
                            text: "PISTOL"
                            color: gameMode === 0 ? _red : _txtSec
                            font.family: theme.fontFamily; font.pixelSize: 13
                            font.bold: gameMode === 0; font.letterSpacing: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "10 m"
                            color: gameMode === 0 ? _txtSec : _txtMut
                            font.family: theme.fontFamily; font.pixelSize: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    MouseArea {
                        id: pistolMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; papermode = 0; gameMode = 0; rangeSelected(10); gameEvent = 0 }
                    }
                }

                Rectangle {
                    width: (parent.width - 8) / 2; height: 58; radius: 8
                    color: gameMode === 1 ? _redDark : _surfaceAlt
                    border.color: gameMode === 1 ? _red : _borderSub
                    border.width: gameMode === 1 ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 2
                        Text {
                            text: "RIFLE"
                            color: gameMode === 1 ? _red : _txtSec
                            font.family: theme.fontFamily; font.pixelSize: 13
                            font.bold: gameMode === 1; font.letterSpacing: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "10 m  ·  50 m"
                            color: gameMode === 1 ? _txtSec : _txtMut
                            font.family: theme.fontFamily; font.pixelSize: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    MouseArea {
                        id: rifleMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; papermode = 0; gameMode = 1; gameEvent = 0 }
                    }
                }
            }

            // Distance selector (10m | 50m) — only when RIFLE is selected
            // Sub-discipline selector (Prone | 3 Positions) — only for RIFLE 50m
            // Both are stacked in a Column so they flow naturally
            Column {
                id: subDisciplineRow
                anchors.top: weaponRow.bottom
                anchors.topMargin: (setaSelection ? (gameMode === 1 && gameRange === 50)
                                                  : (gameMode === 1)) ? 8 : 0
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                // On the SETA path the hierarchy owns weapon + distance, so only
                // the position row (50 m Rifle) survives here: 3 Positions is a
                // position variant, not a catalogue programme.
                height: setaSelection ? ((gameMode === 1 && gameRange === 50) ? 44 : 0)
                                      : (gameMode === 1 ? (gameRange === 50 ? 96 : 44) : 0)
                spacing: 8; clip: true

                // Distance row: 10m | 50m — LEGACY PATH
                Row {
                    visible: !setaSelection
                    width: parent.width; height: 44; spacing: 8
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 44; radius: 6
                        color: gameRange === 10 ? _redDark : _input
                        border.color: gameRange === 10 ? _red : _borderSub
                        Text {
                            anchors.centerIn: parent; text: "10 m"
                            font.family: theme.fontFamily; font.pixelSize: 11; font.letterSpacing: 1
                            font.bold: gameRange === 10
                            color: gameRange === 10 ? _red : _txtSec
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; rangeSelected(10); gameSubMode = 0; gameEvent = 0 }
                        }
                    }
                    Rectangle {
                        id: rifle50Mouse
                        width: (parent.width - 8) / 2; height: 44; radius: 6
                        color: gameRange === 50 ? _redDark : _input
                        border.color: gameRange === 50 ? _red : _borderSub
                        Text {
                            anchors.centerIn: parent; text: "50 m"
                            font.family: theme.fontFamily; font.pixelSize: 11; font.letterSpacing: 1
                            font.bold: gameRange === 50
                            color: gameRange === 50 ? _red : _txtSec
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; rangeSelected(50); gameSubMode = 0; gameEvent = 4 }
                        }
                    }
                }

                // Prone | 3 Positions — only shown for 50m
                Row {
                    width: parent.width; height: 44; spacing: 8
                    visible: gameRange === 50
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 44; radius: 6
                        color: gameSubMode === 0 ? _redDark : _input
                        border.color: gameSubMode === 0 ? _red : _borderSub
                        Text {
                            anchors.centerIn: parent; text: "PRONE"
                            font.family: theme.fontFamily; font.pixelSize: 11; font.letterSpacing: 1
                            font.bold: gameSubMode === 0
                            color: gameSubMode === 0 ? _red : _txtSec
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; gameSubMode = 0; gameEvent = 4 }
                        }
                    }
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 44; radius: 6
                        color: gameSubMode === 1 ? _redDark : _input
                        border.color: gameSubMode === 1 ? _red : _borderSub
                        Text {
                            anchors.centerIn: parent; text: "3 POSITIONS"
                            font.family: theme.fontFamily; font.pixelSize: 11; font.letterSpacing: 1
                            font.bold: gameSubMode === 1
                            color: gameSubMode === 1 ? _red : _txtSec
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; gameSubMode = 1; gameEvent = 4 }
                        }
                    }
                }
            }

            // ── Scrollable event list ─────────────────────────────────────────
            // Was a ScrollView, which sizes itself from its content's implicit
            // height. With a Column of conditionally-visible cards and an
            // Open Practice card that changes height when selected, that
            // measurement was unreliable and the list simply clipped — the
            // last cards could not be reached at all.
            //
            // A Flickable with contentHeight bound explicitly to the Column is
            // deterministic: it always knows how tall the content is, so the
            // final card is always reachable. Mouse wheel and touch both drive
            // a Flickable natively.
            Flickable {
                id: eventScroll
                visible: practiceView === 0 && !(setaSelection && setaBrowsing)
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true
                // Horizontal scrolling is prohibited on this page: it is how a
                // clipped discipline would hide itself (UI-0 finding F1).
                contentWidth: width
                contentHeight: eventColumn.height
                boundsBehavior: Flickable.StopAtBounds
                flickDeceleration: 2500
                ScrollBar.vertical: ScrollBar {
                    policy: eventScroll.contentHeight > eventScroll.height
                            ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                    width: 8
                }

                Column {
                    id: eventColumn
                    width: eventScroll.width
                    spacing: 0
                    // Breathing room after the final card so it is fully
                    // reachable rather than flush against the panel edge.
                    bottomPadding: 20

                    component EventCard: Rectangle {
                        property int eventIndex: 0
                        width: eventColumn.width; height: 78; radius: 8
                        color: gameEvent === eventIndex ? _redDark : _surfaceAlt
                        border.color: gameEvent === eventIndex ? _red : _borderSub
                        border.width: gameEvent === eventIndex ? 2 : 1

                        Row {
                            anchors.left: parent.left;  anchors.leftMargin: 12
                            anchors.right: parent.right; anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 12

                            Rectangle {
                                width: 38; height: 38; radius: 19
                                color: gameEvent === eventIndex ? _red : _borderStr
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    text: getEventCardBadge(eventIndex)
                                    color: "white"
                                    font.family: "Consolas"
                                    font.pixelSize: eventIndex === 5 ? 9 : 12
                                    font.bold: true
                                    anchors.centerIn: parent
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter; spacing: 4
                                width: parent.width - 38 - 20 - 12 * 3
                                Text {
                                    text: getEventCardTitle(eventIndex)
                                    color: gameEvent === eventIndex ? _txt : _txtSec
                                    font.family: theme.fontFamily; font.pixelSize: 13
                                    font.bold: gameEvent === eventIndex
                                    elide: Text.ElideRight; width: parent.width
                                }
                                Row {
                                    spacing: 8
                                    Text {
                                        text: getEventCardSubtitle(eventIndex)
                                        color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                                    }
                                    Text {
                                        visible: getGameEventText(eventIndex) !== "Free Practice"
                                        text: {
                                            var s = getGameEventText(eventIndex)
                                            if (s === "10") return "·  10 min"
                                            if (s === "15") return "·  15 min"
                                            if (s === "20") return "·  20 min"
                                            if (s === "30") return "·  30 min"
                                            if (s === "40") return "·  40 min"
                                            if (s === "60") {
                                                if (gameMode === 0)    return "·  75 min"
                                                if (gameRange === 10)  return "·  75 min"
                                                if (gameSubMode === 1) return "·  90 min"
                                                return "·  50 min"
                                            }
                                            return ""
                                        }
                                        color: _txtMut; font.family: "Consolas"; font.pixelSize: 10
                                    }
                                }
                            }

                            Rectangle {
                                width: 18; height: 18; radius: 9
                                anchors.verticalCenter: parent.verticalCenter
                                color: "transparent"
                                border.color: gameEvent === eventIndex ? _red : _borderStr
                                border.width: 2
                                Rectangle {
                                    anchors.centerIn: parent; width: 9; height: 9; radius: 5
                                    color: _red; visible: gameEvent === eventIndex
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            onEntered: { if (gameEvent !== eventIndex) parent.color = _borderSub }
                            onExited:  { parent.color = gameEvent === eventIndex ? _redDark : _surfaceAlt }
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; gameEvent = eventIndex }
                        }
                    }

                    // ── OFFICIAL ISSF MATCH ────────────────────────────────────
                    // LEGACY PATH: on the SETA path the 60-shot course is a
                    // catalogue programme reached through the hierarchy, so
                    // offering it here as well would be a second selector.
                    Text {
                        text: "OFFICIAL ISSF MATCH"
                        visible: !setaSelection
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: theme.type.label.size; font.bold: true; font.letterSpacing: theme.type.label.spacing
                        topPadding: 4; bottomPadding: 8
                    }
                    // Official: 60 shots — Pistol, 10m Rifle, 50m Prone, 50m 3 Pos (20+20+20)
                    EventCard { eventIndex: 4; visible: !setaSelection }

                    Text {
                        text: "FINALS"
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: theme.type.label.size; font.bold: true; font.letterSpacing: theme.type.label.spacing
                        topPadding: 16; bottomPadding: 8
                        visible: gameRange === 10 || (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
                    }

                    // 3P FINAL (35) — ISSF final training mode; only offered in
                    // the 50m Rifle 3 Positions flow. Separate finals domain
                    // (isFinalsMatch) — see docs/3p-finals-discipline.md.
                    EventCard { eventIndex: 6; visible: gameMode === 1 && gameRange === 50 && gameSubMode === 1 }
                    Item { width: 1; height: 2; visible: gameMode === 1 && gameRange === 50 && gameSubMode === 1 }

                    // 10m FINAL (24) — ISSF 10m Air Rifle / Air Pistol final
                    // training mode; offered at the 10m range for both rifle and
                    // pistol. Separate single-athlete finals domain
                    // (isFinals10mMatch) — see docs/10m-finals-architecture.md.
                    EventCard { eventIndex: 7; visible: gameRange === 10 }
                    Item { width: 1; height: 2; visible: gameRange === 10 }

                    // ── PRACTICE & DEVELOPMENT (T1) ────────────────────────────
                    // Replaces the four fixed 10/20/30/40 rows. Open Practice
                    // preserves the SAME practice events (gameEvent 0-3) as
                    // compact presets; Training Lab opens the programme
                    // catalogue in this panel.
                    Text {
                        text: "TRAINING LAB"
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: theme.type.label.size; font.bold: true; font.letterSpacing: theme.type.label.spacing
                        topPadding: 16; bottomPadding: 8
                    }
                    // TRAINING LAB — gateway to the programme catalogue.
                    Rectangle {
                        width: eventColumn.width; height: 78; radius: 8
                        color: _surfaceAlt
                        border.color: trainingConfirmed ? _red : _borderSub
                        border.width: trainingConfirmed ? 2 : 1
                        Row {
                            anchors.left: parent.left; anchors.leftMargin: 12
                            anchors.right: parent.right; anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 12
                            Rectangle {
                                width: 38; height: 38; radius: 19
                                color: trainingConfirmed ? _red : _borderStr
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: "TL"; color: "white"; font.family: "Consolas"
                                       font.pixelSize: 12; font.bold: true; anchors.centerIn: parent }
                            }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter; spacing: 3
                                width: parent.width - 38 - 30 - 24
                                Text { text: "TRAINING LAB"
                                       color: _txt; font.family: theme.fontFamily
                                       font.pixelSize: 13; font.bold: true }
                                Text { text: "Structured technical practice and athlete feedback.\nTechnical Blocks · Shot calling · Group analysis"
                                       color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                            }
                            Text { text: "→"; color: _red; font.pixelSize: 20; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                        MouseArea { anchors.fill: parent; onClicked: practiceView = 1 }
                    }
                    Text {
                        text: "PRACTICE"
                        visible: !setaSelection
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: theme.type.label.size; font.bold: true; font.letterSpacing: theme.type.label.spacing
                        topPadding: 16; bottomPadding: 8
                    }
                    // OPEN PRACTICE — one card; presets select the existing
                    // practice events (identical behaviour to the old rows).
                    // LEGACY PATH: on the SETA path these same events are the
                    // practice-preset programmes in the hierarchy.
                    Rectangle {
                        id: openPracticeCard
                        visible: !setaSelection
                        readonly property bool selected: gameEvent >= 0 && gameEvent <= 3 && !trainingConfirmed
                        width: eventColumn.width
                        // UI-HOME-009: the collapsed card now matches every other
                        // event card exactly (78). Selected, it grows by just the
                        // preset row + one gap (48 + 8) instead of the old 148,
                        // so the shot-plan selector costs 56 px rather than 70.
                        height: selected ? 78 + 48 + 8 : 78
                        radius: 8
                        color: selected ? _redDark : _surfaceAlt
                        border.color: selected ? _red : _borderSub
                        border.width: selected ? 2 : 1
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 12
                            anchors.right: parent.right; anchors.rightMargin: 12
                            anchors.top: parent.top; anchors.topMargin: 12
                            spacing: 8
                            Row {
                                spacing: 12
                                Rectangle {
                                    width: 38; height: 38; radius: 19
                                    color: openPracticeCard.selected ? _red : _borderStr
                                    Text { text: "OP"; color: _onAccent; font.family: theme.type.numericMetric.family
                                           font.pixelSize: 12; font.bold: true; anchors.centerIn: parent }
                                }
                                Column {
                                    spacing: 3
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { text: "OPEN PRACTICE"
                                           color: openPracticeCard.selected ? _txt : _txtSec
                                           font.family: theme.fontFamily
                                           font.pixelSize: theme.type.cardTitle.size
                                           font.bold: openPracticeCard.selected }
                                    // UI-HOME-007 / P1-7: one line, like every
                                    // other card's subtitle. The second sentence
                                    // explained where plans come from, which the
                                    // operator does not need at selection time.
                                    Text { text: qsTr("A flexible shooting session — choose a shot plan")
                                           color: _txtMut; font.family: theme.fontFamily
                                           font.pixelSize: theme.type.helperText.size }
                                }
                            }
                            // Compact presets (existing gameEvents; 10/30 stay 10m-only)
                            Row {
                                visible: openPracticeCard.selected
                                spacing: 6
                                Repeater {
                                    model: (gameMode === 1 && gameRange === 50)
                                           ? [ {e: 1, t: "20"}, {e: 3, t: "40"}, {e: 5, t: "No limit"} ]
                                           : [ {e: 0, t: "10"}, {e: 1, t: "20"}, {e: 2, t: "30"}, {e: 3, t: "40"}, {e: 5, t: "No limit"} ]
                                    delegate: Rectangle {
                                        width: 84; height: 48; radius: 6
                                        color: gameEvent === modelData.e ? _red : _input
                                        border.color: gameEvent === modelData.e ? _red : _borderSub
                                        Text { anchors.centerIn: parent
                                               text: modelData.t
                                               color: gameEvent === modelData.e ? "white" : _txtSec
                                               font.family: "Consolas"; font.pixelSize: 12; font.bold: true }
                                        MouseArea { anchors.fill: parent; onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; gameEvent = modelData.e } }
                                    }
                                }
                            }
                        }
                        // UI-HOME-007: the same radio indicator every EventCard
                        // uses, in the same position. This card previously had
                        // no selection indicator at all, so three different
                        // patterns coexisted in one list.
                        Rectangle {
                            width: 18; height: 18; radius: 9
                            anchors.right: parent.right; anchors.rightMargin: 12
                            anchors.top: parent.top; anchors.topMargin: 22
                            color: "transparent"
                            border.color: openPracticeCard.selected ? _red : _borderStr
                            border.width: 2
                            Rectangle {
                                anchors.centerIn: parent; width: 9; height: 9; radius: 5
                                color: _red; visible: openPracticeCard.selected
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: !openPracticeCard.selected
                            onClicked: { trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false; wmConfirmed = false; gameEvent = 1 }
                        }
                    }
                    Item { width: 1; height: 8; visible: !setaSelection }
                    // T1.1: the separate FREE PRACTICE section is gone — one
                    // practice concept only. Unlimited practice (gameEvent 5)
                    // lives inside the Open Practice card as the "No limit"
                    // option; nothing else changed on the practice path.
                }
            }
            // ── TRAINING LAB catalogue (practiceView 1) ──────────────────────
            Flickable {
                id: catFlick
                visible: practiceView === 1
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true
                contentWidth: width
                contentHeight: catCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: catCol
                    width: catFlick.width
                    spacing: 8
                    bottomPadding: 12

                    Text {
                        text: "← Back to events"
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: practiceView = 0 }
                        bottomPadding: 6
                    }
                    Row {
                        spacing: 10
                        Text { text: "TRAINING LAB"; color: _txt
                               font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                        // T1.1: in-app help — no GitHub docs needed to understand it.
                        Rectangle {
                            width: 88; height: 30; radius: 15
                            color: _input; border.color: _borderSub; border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text { anchors.centerIn: parent; text: "ⓘ  Help"
                                   color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: dialogManager.showInformation(qsTr("Training Lab"),
                                    qsTr("TECHNICAL BLOCKS\nShoot several short groups while concentrating on one technical part of your process. After each block, TechAim reveals the measured group and lets you record a note before continuing.\n\nVISIBILITY MODES\nFull hidden — nothing shown until review. Group only — positions without numbers. Impact only — impacts without scores.\n\nTHREE POSITIONS\nKneeling, Prone and Standing stay separate: K1 → K2 → P1 → P2 → S1 → S2.\n\nTraining results are for development only — never an official competition result. In Demo mode no physical target is required."))
                            }
                        }
                    }
                    Text { text: "Structured technical practice and athlete feedback."
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }
                    Text { text: getDisciplineName(); color: _red
                           font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true
                           bottomPadding: 6 }

                    // AVAILABLE — Technical Blocks (clickable)
                    Rectangle {
                        width: parent.width; height: 96; radius: 8
                        color: _surfaceAlt; border.color: _red; border.width: 1
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter; spacing: 3
                            Row {
                                spacing: 8
                                Text { text: gameMode === 1 && gameRange === 50 && gameSubMode === 1
                                             ? "TECHNICAL BLOCKS · BY POSITION" : "TECHNICAL BLOCKS"
                                       color: _txt; font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true }
                                Rectangle {
                                    width: 74; height: 18; radius: 9; color: _okBg
                                    border.color: _green; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: "AVAILABLE"
                                           color: _green; font.pixelSize: theme.type.label.size; font.bold: true }
                                }
                            }
                            Text { text: "Shoot several short groups while concentrating on one technical part of your process.\nAfter each block, TechAim reveals the measured group and lets you record a note."
                                   color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                            Text { text: (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
                                         ? "Default: 36 shots · Kneeling → Prone → Standing · Configurable"
                                         : "Default: 30 shots · Configurable"
                                   color: _txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                TRAINING.configureDefaults(trainingDisciplineId())
                                practiceView = 2
                            }
                        }
                    }

                    // COMING NEXT / PLANNED — visibly disabled, non-interactive.
                    component FutureCard: Rectangle {
                        property string title: ""
                        property string status: "COMING NEXT"
                        width: parent.width; height: 54; radius: 8
                        color: _input; border.color: _borderSub; border.width: 1
                        opacity: 0.55
                        Row {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.verticalCenter: parent.verticalCenter; spacing: 10
                            Text { text: title; color: _txtSec
                                   font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                            Rectangle {
                                width: statusT.implicitWidth + 16; height: 18; radius: 9
                                color: "transparent"; border.color: _borderStr; border.width: 1
                                anchors.verticalCenter: parent.verticalCenter
                                Text { id: statusT; anchors.centerIn: parent; text: status
                                       color: _txtMut; font.pixelSize: theme.type.label.size; font.bold: true }
                            }
                        }
                        // no MouseArea: not clickable, no selector, no fake setup
                    }
                    // AVAILABLE — Call & Diagnose (T2, clickable)
                    Rectangle {
                        width: parent.width; height: 96; radius: 8
                        color: _surfaceAlt; border.color: _red; border.width: 1
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter; spacing: 3
                            Row {
                                spacing: 8
                                Text { text: (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
                                             ? "CALL & DIAGNOSE · BY POSITION" : "CALL & DIAGNOSE"
                                       color: _txt; font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true }
                                Rectangle {
                                    width: 74; height: 18; radius: 9; color: _okBg
                                    border.color: _green; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: "AVAILABLE"
                                           color: _green; font.pixelSize: theme.type.label.size; font.bold: true }
                                }
                            }
                            Text { text: "Call each shot before the actual impact is revealed.\nCompare where you believed the shot landed with where it actually landed."
                                   color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                            Text { text: (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
                                         ? "Default: 10 called shots per position · Configurable"
                                         : "Default: 20 called shots · Configurable"
                                   color: _txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                CALLDIAG.configureDefaults(trainingDisciplineId())
                                practiceView = 3
                            }
                        }
                    }
                    // AVAILABLE — Position Transition (T4, 3P only, clickable)
                    Rectangle {
                        visible: gameMode === 1 && gameRange === 50 && gameSubMode === 1
                        width: parent.width; height: 96; radius: 8
                        color: _surfaceAlt; border.color: _red; border.width: 1
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter; spacing: 3
                            Row {
                                spacing: 8
                                Text { text: "POSITION TRANSITION"; color: _txt; font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true }
                                Rectangle {
                                    width: 74; height: 18; radius: 9; color: _okBg; border.color: _green; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: "AVAILABLE"; color: _green; font.pixelSize: theme.type.label.size; font.bold: true } }
                            }
                            Text { text: "Practise changing between Kneeling, Prone and Standing.\nMeasure setup time, sighters, first-shot timing and early group repeatability after each transition."
                                   color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                            Text { text: "Default: Kneeling → Prone → Standing · 5 verification shots"
                                   color: _txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { POSTRANS.configureDefaults(); practiceView = 5 }
                        }
                    }
                    // AVAILABLE — Wind Map (Release 2). 50m Rifle Prone and 50m
                    // Rifle 3 Positions ONLY; the card is not shown anywhere
                    // else and the controller refuses anything else anyway.
                    Rectangle {
                        visible: gameMode === 1 && gameRange === 50
                        width: parent.width; height: 96; radius: 8
                        color: _surfaceAlt; border.color: _red; border.width: 1
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter; spacing: 3
                            Row {
                                spacing: 8
                                Text { text: gameSubMode === 1 ? "WIND MAP · BY POSITION" : "WIND MAP"
                                       color: _txt; font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true }
                                Rectangle {
                                    width: 74; height: 18; radius: 9; color: _okBg; border.color: _green; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: "AVAILABLE"; color: _green
                                           font.pixelSize: theme.type.label.size; font.bold: true } }
                            }
                            Text { text: "Record the wind you observe while you shoot.\nEach shot keeps the condition that was standing when it was fired, for review afterwards."
                                   color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                            Text { text: gameSubMode === 1
                                         ? "Manual entry · Kneeling, Prone and Standing kept separate"
                                         : "Manual entry · Direction in degrees, speed in m/s, or Calm"
                                   color: _txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // The controller is the single authority on
                                // where Wind Map may run — QML asks, it decides.
                                if (!WINDMAP.configureSession(windMapDisciplineId(), 40, true)) {
                                    dialogManager.showError(qsTr("Wind Map is not available here"),
                                        WINDMAP.unsupportedDisciplineMessage(windMapDisciplineId()))
                                    return
                                }
                                practiceView = 6
                            }
                        }
                    }

                    Text { text: "INCLUDED INSIGHTS"; color: _txtMut
                           font.family: theme.fontFamily; font.pixelSize: theme.type.label.size; font.bold: true
                           font.letterSpacing: 2; topPadding: 10 }
                    Text {
                        text: {
                            if (gameMode === 0) return "· Group Pattern Coach\n· Air Pistol technical checklist"
                            if (gameRange === 10) return "· Group Pattern Coach\n· Air Rifle technical checklist"
                            if (gameSubMode === 1) return "· Position-specific Group Pattern Coach\n· Kneeling checklist · Prone checklist · Standing checklist"
                            return "· Group Pattern Coach\n· Prone technical checklist"
                        }
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                    }
                }
            }

            // ── TECHNICAL BLOCKS SETUP (practiceView 2) ──────────────────────
            // Flickable with an EXPLICIT contentHeight so the whole setup —
            // including the Back / Confirm buttons at the end — is always
            // reachable by touch drag / flick / wheel, regardless of window
            // height. (Plain ScrollView mis-measured the externally-widthed
            // Column and clipped the actions off the bottom.)
            Flickable {
                id: setupFlick
                visible: practiceView === 2
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true
                contentWidth: width
                contentHeight: setupCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: setupCol
                    width: setupFlick.width
                    spacing: 10
                    bottomPadding: 12

                    Text {
                        text: "← Back"
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: practiceView = 1 }
                    }
                    Text { text: "TECHNICAL BLOCKS SETUP"; color: _txt
                           font.family: theme.fontFamily; font.pixelSize: 17; font.bold: true }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                        text: trainingDisciplineId() === "3P50"
                              ? "The programme keeps Kneeling, Prone and Standing separate. Each position gets its own blocks, measurements, notes and comparison \u2014 results are never combined into one generic 3P analysis."
                              : "You shoot each block while focusing on one selected area. The block then stops and opens a measured review; after adding a note you continue to the next block. Hidden modes reveal scores and impacts only at the review."
                    }
                    Text {
                        visible: trainingDisciplineId() === "3P50"
                        width: parent.width; wrapMode: Text.WordWrap
                        color: _green; font.family: "Consolas"; font.pixelSize: 12; font.bold: true
                        text: {
                            var bpp = Math.max(1, TRAINING.blockCount / 3)
                            var seq = [], names = ["K", "P", "S"]
                            for (var g = 0; g < 3; ++g)
                                for (var i = 1; i <= bpp; ++i) seq.push(names[g] + i)
                            return seq.join(" \u2192 ") + "   \u00b7   "
                                   + (TRAINING.blockCount * TRAINING.shotsPerBlock) + " shots total"
                        }
                    }
                    Text { text: getDisciplineName() + "  ·  " + username_loginPage
                                 + "  ·  " + TRAINING.estimatedTime
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }
                    // TB-05: the wording comes from the controller, never from
                    // here — the setup screen, the report and the manual must
                    // describe the defaults identically.
                    Text { text: TRAINING.configurationNote
                           width: 520; wrapMode: Text.WordWrap
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }

                    component Stepper: Row {
                        property string label: ""
                        property int value: 0
                        signal minus(); signal plus()
                        spacing: 10
                        Text { text: label; color: _txtSec; width: 130
                               font.family: theme.fontFamily; font.pixelSize: 12
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle {
                            width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "−"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: parent.parent.minus() }
                        }
                        Text { text: value; color: _txt; width: 40; horizontalAlignment: Text.AlignHCenter
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle {
                            width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "+"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: parent.parent.plus() }
                        }
                    }

                    Stepper {
                        label: trainingDisciplineId() === "3P50" ? "Blocks per position" : "Blocks"
                        value: trainingDisciplineId() === "3P50" ? TRAINING.blockCount / 3 : TRAINING.blockCount
                        onMinus: TRAINING.setBlockCount(TRAINING.blockCount
                                     - (trainingDisciplineId() === "3P50" ? 3 : 1))
                        onPlus:  TRAINING.setBlockCount(TRAINING.blockCount
                                     + (trainingDisciplineId() === "3P50" ? 3 : 1))
                    }
                    Stepper {
                        label: "Shots per block"
                        value: TRAINING.shotsPerBlock
                        onMinus: TRAINING.setShotsPerBlock(TRAINING.shotsPerBlock - 1)
                        onPlus:  TRAINING.setShotsPerBlock(TRAINING.shotsPerBlock + 1)
                    }
                    Text { text: "Total: " + (TRAINING.blockCount * TRAINING.shotsPerBlock) + " shots"
                                 + (trainingDisciplineId() === "3P50" ? "  ·  Kneeling → Prone → Standing" : "")
                           color: _txtSec; font.family: "Consolas"; font.pixelSize: 11 }

                    Text { text: "Technical focus"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Flow {
                        width: parent.width; spacing: 6
                        Repeater {
                            model: TRAINING.focusOptionsForDiscipline()
                            delegate: Rectangle {
                                width: focusT.implicitWidth + 30; height: 44; radius: 22
                                color: TRAINING.technicalFocus === modelData ? _red : _input
                                border.color: TRAINING.technicalFocus === modelData ? _red : _borderSub
                                Text { id: focusT; anchors.centerIn: parent; text: modelData
                                       color: TRAINING.technicalFocus === modelData ? "white" : _txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent
                                            onClicked: TRAINING.setTechnicalFocus(modelData) }
                            }
                        }
                    }

                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        visible: TRAINING.technicalFocus !== ""
                        color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                        text: {
                            var d = {
                              "Hold": "Focus on a calm, stable hold through the shot.",
                              "Aim": "Focus on a clean, consistent sight picture.",
                              "Trigger": "Focus on a smooth release without disturbing the aim.",
                              "Follow-through": "Focus on keeping the process going after the shot breaks.",
                              "Natural point of aim": "Focus on rebuilding alignment without forcing the sights onto the centre.",
                              "Head position": "Focus on a consistent, relaxed head placement.",
                              "Shoulder contact": "Focus on repeatable butt-plate contact and pressure.",
                              "Balance": "Focus on quiet, centred balance through the shot.",
                              "Rhythm": "Focus on an even shot cadence."
                            }
                            return d[TRAINING.technicalFocus] || ""
                        }
                    }
                    Text { text: "Visibility"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Column {
                        spacing: 5
                        Repeater {
                            model: [ "Full hidden block", "Group only", "Impact visible, score hidden" ]
                            delegate: MouseArea {
                                width: eventScroll.availableWidth; height: 52
                                onClicked: TRAINING.setVisibilityMode(index)
                                Rectangle {
                                    anchors.fill: parent; radius: 8
                                    color: TRAINING.visibilityMode === index ? _redDark : _input
                                    border.color: TRAINING.visibilityMode === index ? _red : _borderSub
                                    border.width: TRAINING.visibilityMode === index ? 2 : 1
                                    Column {
                                        anchors.left: parent.left; anchors.leftMargin: 14
                                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                                        Text { text: modelData; color: _txt
                                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true }
                                        Text {
                                            text: ["No score or impact is shown until block review.",
                                                   "Shot positions form a group; numerical scores stay hidden.",
                                                   "Shot positions are visible; scores stay hidden."][index]
                                            color: _txtMut; font.family: theme.fontFamily; font.pixelSize: theme.type.helperText.size
                                        }
                                    }
                                    Text { visible: TRAINING.visibilityMode === index
                                           anchors.right: parent.right; anchors.rightMargin: 14
                                           anchors.verticalCenter: parent.verticalCenter
                                           text: "\u2713"; color: _red; font.pixelSize: 16; font.bold: true }
                                }
                            }
                        }
                    }
                    Text { text: "Optional shot calling — coming with Call & Diagnose"
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }

                    // Validation (controller-owned — no duplicate rules here).
                    Text {
                        id: setupError
                        visible: text !== ""
                        text: ""
                        width: parent.width; wrapMode: Text.WordWrap
                        color: theme.tokens.errorText; font.family: theme.fontFamily; font.pixelSize: 11
                    }
                    Row {
                        spacing: 10; topPadding: 6
                        Rectangle {
                            width: 110; height: 52; radius: 8
                            color: "transparent"; border.color: _borderStr; border.width: 1
                            Text { anchors.centerIn: parent; text: "Back"; color: _txtSec
                                   font.family: theme.fontFamily; font.pixelSize: 12 }
                            MouseArea { anchors.fill: parent; onClicked: practiceView = 1 }
                        }
                        Rectangle {
                            width: 180; height: 52; radius: 8; color: _red
                            Text { anchors.centerIn: parent; text: "Confirm setup"; color: "white"
                                   font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (TRAINING.technicalFocus === "") {
                                        setupError.text = "Select a technical focus."; return
                                    }
                                    var err = TRAINING.validateConfig()
                                    if (err !== "") { setupError.text = err; return }
                                    setupError.text = ""
                                    trainingConfirmed = true
                                    cdConfirmed = false     // programmes are mutually exclusive
                                    practiceView = 0        // back to events + summary
                                }
                            }
                        }
                    }
                }
            }

            // ── CALL & DIAGNOSE SETUP (practiceView 3) ───────────────────────
            Flickable {
                id: cdSetupFlick
                visible: practiceView === 3
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true
                contentWidth: width
                contentHeight: cdSetupCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: cdSetupCol
                    width: cdSetupFlick.width
                    spacing: 10; bottomPadding: 12

                    Text {
                        text: "← Back"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: practiceView = 1 }
                    }
                    Text { text: "CALL & DIAGNOSE SETUP"; color: _txt
                           font.family: theme.fontFamily; font.pixelSize: 17; font.bold: true }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                        text: "After every shot the actual impact stays hidden. Tap the target where you believe the shot landed, confirm your call, then compare it with the measured result. This assesses how accurately you recognise your own shot."
                            + (trainingDisciplineId() === "3P50"
                               ? " Kneeling, Prone and Standing are kept separate." : "")
                    }
                    Text { text: getDisciplineName() + "  ·  " + username_loginPage + "  ·  " + CALLDIAG.estimatedTime
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }

                    Row {
                        spacing: 10
                        Text { text: trainingDisciplineId() === "3P50" ? "Called shots / position" : "Called shots"
                               color: _txtSec; width: 150; font.family: theme.fontFamily; font.pixelSize: 12
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "−"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: CALLDIAG.setShotCount(CALLDIAG.shotCount - 1) } }
                        Text { text: CALLDIAG.shotCount; color: _txt; width: 40; horizontalAlignment: Text.AlignHCenter
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "+"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: CALLDIAG.setShotCount(CALLDIAG.shotCount + 1) } }
                    }
                    Text { text: "Total: " + (trainingDisciplineId() === "3P50" ? CALLDIAG.shotCount * 3 : CALLDIAG.shotCount)
                                 + " called shots"
                                 + (trainingDisciplineId() === "3P50" ? "  ·  Kneeling → Prone → Standing" : "")
                           color: _txtSec; font.family: "Consolas"; font.pixelSize: 11 }

                    Text { text: "Technical focus"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Flow {
                        width: parent.width; spacing: 6
                        Repeater {
                            model: CALLDIAG.focusOptionsForDiscipline()
                            delegate: Rectangle {
                                width: cdFocusT.implicitWidth + 30; height: 44; radius: 22
                                color: CALLDIAG.technicalFocus === modelData ? _red : _input
                                border.color: CALLDIAG.technicalFocus === modelData ? _red : _borderSub
                                Text { id: cdFocusT; anchors.centerIn: parent; text: modelData
                                       color: CALLDIAG.technicalFocus === modelData ? "white" : _txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent
                                            onClicked: CALLDIAG.setTechnicalFocus(modelData) }
                            }
                        }
                    }
                    Text { text: "Reveal happens immediately after each call is confirmed."
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }

                    Text {
                        id: cdSetupError
                        visible: text !== ""; text: ""
                        width: parent.width; wrapMode: Text.WordWrap
                        color: theme.tokens.errorText; font.family: theme.fontFamily; font.pixelSize: 11
                    }
                    Row {
                        spacing: 10; topPadding: 6
                        Rectangle {
                            width: 110; height: 52; radius: 8
                            color: "transparent"; border.color: _borderStr; border.width: 1
                            Text { anchors.centerIn: parent; text: "Back"; color: _txtSec
                                   font.family: theme.fontFamily; font.pixelSize: 12 }
                            MouseArea { anchors.fill: parent; onClicked: practiceView = 1 }
                        }
                        Rectangle {
                            width: 180; height: 52; radius: 8; color: _red
                            Text { anchors.centerIn: parent; text: "Confirm setup"; color: "white"
                                   font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (CALLDIAG.technicalFocus === "") { cdSetupError.text = "Select a technical focus."; return }
                                    var err = CALLDIAG.validateConfig()
                                    if (err !== "") { cdSetupError.text = err; return }
                                    cdSetupError.text = ""
                                    cdConfirmed = true
                                    trainingConfirmed = false      // mutually exclusive
                                    practiceView = 0
                                }
                            }
                        }
                    }
                }
            }

            // ── POSITION TRANSITION SETUP (practiceView 5) ───────────────────
            Flickable {
                id: ptSetupFlick
                visible: practiceView === 5
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true; contentWidth: width; contentHeight: ptSetupCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: ptSetupCol
                    width: ptSetupFlick.width; spacing: 10; bottomPadding: 12

                    Text { text: "← Back"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: practiceView = 1 } }
                    Text { text: "POSITION TRANSITION SETUP"; color: _txt; font.family: theme.fontFamily; font.pixelSize: 17; font.bold: true }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                        text: "This programme measures how consistently you rebuild each Three-Position shooting position. You begin a position setup, confirm when the position is ready, fire optional sighters, then a short counted verification block. Tech Aim compares the timing and measured result of each position. This is a Training session and not an official competition result."
                    }
                    Text { text: POSTRANS.sequenceArrow; color: _green; font.family: "Consolas"; font.pixelSize: 13; font.bold: true }

                    Text { text: "Positions"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Flow { width: parent.width; spacing: 6
                        Repeater {
                            model: [ { l: "Full 3P", p: 0 }, { l: "Kneeling → Prone", p: 1 }, { l: "Prone → Standing", p: 2 },
                                     { l: "Kneeling only", p: 3 }, { l: "Prone only", p: 4 }, { l: "Standing only", p: 5 } ]
                            delegate: Rectangle {
                                width: pseqT.implicitWidth + 26; height: 40; radius: 20
                                property bool sel: POSTRANS.sequenceString() ===
                                    ({0:"K,P,S",1:"K,P",2:"P,S",3:"K",4:"P",5:"S"}[modelData.p])
                                color: sel ? _red : _input; border.color: sel ? _red : _borderSub
                                Text { id: pseqT; anchors.centerIn: parent; text: modelData.l
                                       color: sel ? "white" : _txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: POSTRANS.setSequencePreset(modelData.p) }
                            }
                        }
                    }

                    Row { spacing: 10
                        Text { text: "Verification shots"; color: _txtSec; width: 150; font.family: theme.fontFamily; font.pixelSize: 12
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "−"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: POSTRANS.setVerificationShots(POSTRANS.verificationShots - 1) } }
                        Text { text: POSTRANS.verificationShots; color: _txt; width: 40; horizontalAlignment: Text.AlignHCenter
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "+"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: POSTRANS.setVerificationShots(POSTRANS.verificationShots + 1) } }
                    }
                    Row { spacing: 10
                        Text { text: "Repeats"; color: _txtSec; width: 150; font.family: theme.fontFamily; font.pixelSize: 12
                               anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "−"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: POSTRANS.setRepeats(POSTRANS.totalRepeats - 1) } }
                        Text { text: POSTRANS.totalRepeats; color: _txt; width: 40; horizontalAlignment: Text.AlignHCenter
                               font.family: "Consolas"; font.pixelSize: 16; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 52; height: 48; radius: 8; color: _input; border.color: _borderSub
                            Text { anchors.centerIn: parent; text: "+"; color: _txt; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; onClicked: POSTRANS.setRepeats(POSTRANS.totalRepeats + 1) } }
                    }

                    Text { text: "Checklist"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Row { spacing: 6
                        Repeater {
                            model: [ "Athlete self-check", "Coach-assisted", "Disabled" ]
                            delegate: Rectangle {
                                width: pclT.implicitWidth + 24; height: 40; radius: 20
                                color: POSTRANS.checklistMode === index ? _red : _input
                                border.color: POSTRANS.checklistMode === index ? _red : _borderSub
                                Text { id: pclT; anchors.centerIn: parent; text: modelData
                                       color: POSTRANS.checklistMode === index ? "white" : _txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: POSTRANS.setChecklistMode(index) }
                            }
                        }
                    }

                    Text { text: "Technical focus"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 4 }
                    Flow { width: parent.width; spacing: 6
                        Repeater {
                            model: POSTRANS.focusOptionsForDiscipline()
                            delegate: Rectangle {
                                width: pfT.implicitWidth + 30; height: 44; radius: 22
                                color: POSTRANS.technicalFocus === modelData ? _red : _input
                                border.color: POSTRANS.technicalFocus === modelData ? _red : _borderSub
                                Text { id: pfT; anchors.centerIn: parent; text: modelData
                                       color: POSTRANS.technicalFocus === modelData ? "white" : _txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: POSTRANS.setTechnicalFocus(modelData) }
                            }
                        }
                    }

                    Text { id: ptSetupError; visible: text !== ""; text: ""; width: parent.width; wrapMode: Text.WordWrap
                           color: theme.tokens.errorText; font.family: theme.fontFamily; font.pixelSize: 11 }
                    Row { spacing: 10; topPadding: 6
                        Rectangle { width: 110; height: 52; radius: 8; color: "transparent"; border.color: _borderStr; border.width: 1
                            Text { anchors.centerIn: parent; text: "Back"; color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12 }
                            MouseArea { anchors.fill: parent; onClicked: practiceView = 1 } }
                        Rectangle { width: 180; height: 52; radius: 8; color: _red
                            Text { anchors.centerIn: parent; text: "Confirm setup"; color: "white"; font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true }
                            MouseArea { anchors.fill: parent
                                onClicked: {
                                    if (POSTRANS.technicalFocus === "") { ptSetupError.text = "Select a technical focus."; return }
                                    var err = POSTRANS.validateConfig()
                                    if (err !== "") { ptSetupError.text = err; return }
                                    ptSetupError.text = ""
                                    ptConfirmed = true; trainingConfirmed = false; cdConfirmed = false
                                    wmConfirmed = false
                                    practiceView = 0
                                } }
                        }
                    }
                }
            }

            // ── WIND MAP SETUP (practiceView 6) ──────────────────────────────
            // Release 2. Configuration only: how many shots are planned and
            // whether sighters are fired first. The wind itself is recorded
            // DURING the session, never guessed at here.
            Flickable {
                id: wmSetupFlick
                visible: practiceView === 6
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true; contentWidth: width; contentHeight: wmSetupCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Column {
                    id: wmSetupCol
                    width: wmSetupFlick.width; spacing: 10; bottomPadding: 12

                    Text { text: "← Back to Training Lab"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; bottomPadding: 4
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: practiceView = 1 } }
                    Text { text: "WIND MAP"; color: _txt
                           font.family: theme.fontFamily; font.pixelSize: 18; font.bold: true }
                    Text { text: WINDMAP.threePositions ? "50 m Rifle 3 Positions · Kneeling, Prone and Standing kept separate"
                                                        : "50 m Rifle Prone"
                           color: _red; font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true }
                    Text { width: parent.width; wrapMode: Text.WordWrap
                           text: "You record the wind you observe. Every shot keeps the condition that was standing when it was fired, so the two can be reviewed together afterwards. Nothing here is scored or corrected."
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }

                    Text { text: "Planned shots"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 6 }
                    Row { spacing: 6
                        Repeater {
                            model: [ 20, 30, 40, 60 ]
                            delegate: Rectangle {
                                width: 64; height: 44; radius: 22
                                color: wmSetupCol.plan === modelData ? _red : _input
                                border.color: wmSetupCol.plan === modelData ? _red : _borderSub
                                Text { anchors.centerIn: parent; text: modelData
                                       color: wmSetupCol.plan === modelData ? "white" : _txtSec
                                       font.family: "Consolas"; font.pixelSize: 13; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: wmSetupCol.plan = modelData }
                            }
                        }
                    }
                    // Descriptive only — the plan drives the progress readout and
                    // never caps, rejects or auto-completes a shot.
                    property int plan: 40
                    property bool sighters: true

                    Text { text: "Sighters"; color: _txtSec
                           font.family: theme.fontFamily; font.pixelSize: 12; topPadding: 6 }
                    Row { spacing: 6
                        Repeater {
                            model: [ { l: "Fire sighters first", v: true },
                                     { l: "Straight to counted shots", v: false } ]
                            delegate: Rectangle {
                                width: wmSgT.implicitWidth + 30; height: 44; radius: 22
                                color: wmSetupCol.sighters === modelData.v ? _red : _input
                                border.color: wmSetupCol.sighters === modelData.v ? _red : _borderSub
                                Text { id: wmSgT; anchors.centerIn: parent; text: modelData.l
                                       color: wmSetupCol.sighters === modelData.v ? "white" : _txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: wmSetupCol.sighters = modelData.v }
                            }
                        }
                    }
                    Text { width: parent.width; wrapMode: Text.WordWrap
                           text: "Sighters are recorded with their conditions but are never counted in the session statistics."
                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }

                    Text { id: wmSetupError; visible: text !== ""; text: ""; width: parent.width
                           wrapMode: Text.WordWrap; color: theme.tokens.errorText
                           font.family: theme.fontFamily; font.pixelSize: 11 }
                    Row { spacing: 10; topPadding: 6
                        Rectangle { width: 110; height: 52; radius: 8; color: "transparent"
                            border.color: _borderStr; border.width: 1
                            Text { anchors.centerIn: parent; text: "Back"; color: _txtSec
                                   font.family: theme.fontFamily; font.pixelSize: 12 }
                            MouseArea { anchors.fill: parent; onClicked: practiceView = 1 } }
                        Rectangle { width: 180; height: 52; radius: 8; color: _red
                            Text { anchors.centerIn: parent; text: "Confirm setup"; color: "white"
                                   font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true }
                            MouseArea { anchors.fill: parent
                                onClicked: {
                                    // The controller validates — QML only reports.
                                    if (!WINDMAP.configureSession(windMapDisciplineId(),
                                                                  wmSetupCol.plan,
                                                                  wmSetupCol.sighters)) {
                                        wmSetupError.text = WINDMAP.lastError
                                        return
                                    }
                                    var err = WINDMAP.validateConfig()
                                    if (err !== "") { wmSetupError.text = err; return }
                                    wmSetupError.text = ""
                                    wmConfirmed = true
                                    trainingConfirmed = false; cdConfirmed = false; ptConfirmed = false
                                    practiceView = 0
                                } }
                        }
                    }
                }
            }
        } // rightPanel

        // ── Bottom action area (UI-B) ────────────────────────────────
        // The primary actions used to sit at the bottom of the LEFT panel, inside
        // the same clipped anchor chain as the setup fields — so on a shorter
        // window they were pushed out of sight. They now own a full-width bar
        // that spans both columns and can never be clipped, with a one-line recap
        // of exactly what is about to start.
        Rectangle {
            id: actionBar
            anchors.bottom: contentFooter.top; anchors.bottomMargin: 8
            anchors.left: parent.left; anchors.right: parent.right
            height: 88
            color: _surface; radius: 10
            border.color: _borderSub; border.width: 1

            Rectangle {
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 3; color: _red; radius: 2
            }

            // Left region — readiness / validation state. Its width is derived
            // from the action row's FIXED width rather than from a live anchor
            // to that row, so the two regions can never intersect at any
            // window size.
            Column {
                id: readinessBlock
                anchors.left: parent.left; anchors.leftMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, actionBar.width - actionRow.width - 22 - 18 - 24)
                spacing: 5

                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: rootItem.readinessOk ? qsTr("READY TO START")
                                               : qsTr("CHECK BEFORE STARTING")
                    color: rootItem.readinessOk ? _txtMut : _warnTxt
                    font.family: theme.fontFamily
                    font.pixelSize: theme.type.label.size
                    font.bold: true; font.letterSpacing: theme.type.label.spacing
                }
                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: rootItem.readinessSummary()
                    color: _txtSec; font.family: theme.fontFamily
                    font.pixelSize: theme.type.body.size
                }
            }

            // ── ACTION BUTTONS ────────────────────────────────────────────────
            // Fixed width, right-anchored. This row previously carried BOTH a
            // left and a right anchor, so it spanned the entire bar starting at
            // x=22 and drew straight over the readiness text — the reported
            // overlap. Its height was also 52 while its children are 56.
            Row {
                id: actionRow
                anchors.right: parent.right; anchors.rightMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                width: 210 + 12 + 268
                height: 56; spacing: 12

                Rectangle {
                    width: 210; height: 56
                    color: "transparent"; radius: 8; border.color: _borderStr; border.width: 1
                    Text {
                        text: "Load saved session"
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 14; anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: parent.border.color = _txtMut
                        onExited:  parent.border.color = _borderStr
                        onClicked: {
                            APPSETTINGS.uploadGame()
                            username_loginPage = APPSETTINGS.getUserName()
                            gameMode  = APPSETTINGS.getGameMode()
                            gameEvent = APPSETTINGS.getGameEvent()
                            gameType  = APPSETTINGS.getGameType()
                            if (gameType == 0) MODREADER.changeSighterMode(false)
                            if (userName != "") { isSaveGame = true; startButtonClickedOnLoadGame() }
                        }
                    }
                }

                Rectangle {
                    id: startSessionRect
                    width: 268; height: 56
                    color: _startHov ? _redHover : _red; radius: 8
                    opacity: startMouse.visible ? 1.0 : 0.4
                    property bool _startHov: false
                    Text {
                        text: startButtonText()
                        color: _onAccent; font.family: theme.fontFamily
                        font.pixelSize: theme.type.buttonText.size + 2; font.bold: true
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: startMouse
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: startSessionRect._startHov = true
                        onExited:  startSessionRect._startHov = false
                        onClicked: {
                            // TRAINING LAB (R2): Wind Map — new Training session
                            // (kind=Training, wind_map; 50m Prone and 50m 3P only).
                            // Opens in Setup so a condition can be recorded before
                            // the first shot. NEVER a qualification/Final session.
                            if (wmConfirmed) {
                                if (!WINDMAP.startWindMap(username_loginPage)) {
                                    dialogManager.showError(qsTr("Cannot start Wind Map"),
                                        WINDMAP.lastStartError !== "" ? WINDMAP.lastStartError
                                            : qsTr("The session could not be started."))
                                    return
                                }
                                shootingPage.enterWindMapMode()
                                rootItem.visible = false
                                return
                            }
                            // TRAINING LAB (T4): Position Transition — new Training
                            // session (kind=Training, position_transition; 3P only).
                            if (ptConfirmed) {
                                if (!POSTRANS.startPositionTransition(username_loginPage)) {
                                    dialogManager.showError(qsTr("Cannot start Position Transition"),
                                        POSTRANS.lastStartError !== "" ? POSTRANS.lastStartError
                                            : qsTr("The session could not be started."))
                                    return
                                }
                                shootingPage.enterPositionTransitionMode()
                                rootItem.visible = false
                                return
                            }
                            // TRAINING LAB (T2): Call & Diagnose — new Training
                            // session (kind=Training, call_and_diagnose). Opens in
                            // Sighters; NEVER a qualification/Final session.
                            if (cdConfirmed) {
                                if (!CALLDIAG.startCallDiagnose(username_loginPage)) {
                                    dialogManager.showError(qsTr("Cannot start Call & Diagnose"),
                                        CALLDIAG.lastStartError !== ""
                                            ? CALLDIAG.lastStartError
                                            : qsTr("The session could not be started."))
                                    return
                                }
                                shootingPage.enterCallDiagnoseMode()
                                rootItem.visible = false
                                return
                            }
                            // TRAINING LAB (T1): explicit Start training — new
                            // Training session (kind=Training, technical_blocks,
                            // mode/discipline/focus/visibility persisted, Block 1
                            // started once). NEVER a qualification/Final session.
                            if (trainingConfirmed) {
                                if (!TRAINING.startTraining(username_loginPage)) {
                                    // T1.1: specific, actionable reason from the
                                    // controller (lastStartError is a real property).
                                    dialogManager.showError(qsTr("Cannot start training"),
                                        TRAINING.lastStartError !== ""
                                            ? TRAINING.lastStartError
                                            : qsTr("The training session could not be started."))
                                    return
                                }
                                shootingPage.enterTrainingMode()
                                rootItem.visible = false
                                return
                            }
                            if (!appMode) {
                                MODREADER.appendToLogFile("Application running in demo mode")
                                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                                    dialogManager.showError(qsTr("Master Not Connected"),
                                        qsTr("The master system is not connected.\n\nPlease press \u201CConnect\u201D and try again.")); return
                                }
                                rootItem.visible = false
                            } else {
                                MODREADER.appendToLogFile("Application running in Live mode")
                                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                                    MODREADER.appendToLogFile("Master application required")
                                    dialogManager.showError(qsTr("Master Not Connected"),
                                        qsTr("The master system is not connected.\n\nPlease press \u201CConnect\u201D and try again.")); return
                                }
                                if (masterConnectBtn && port_name_text_field.text != "") {
                                    MODREADER.appendToLogFile("Application with port text field")
                                    MODREADER.connectedModbus(port_name_text_field.text)
                                    mod_connected = MODREADER.isModBusConnected()
                                }
                                if (!MODREADER.isModBusConnected()) {
                                    validateLogin.text = "Com port not connected"; validateLogin.visible = true
                                } else if (!MODREADER.isHardwareConnected()) {
                                    validateLogin.text = "Hardware not connected."; validateLogin.visible = true
                                } else if (!MODREADER.checkAutoFeedMode()) {
                                    validateLogin.text = "Auto feed mode is off"; validateLogin.visible = false
                                } else if (validate()) {
                                    MODREADER.appendToLogFile("Validation was successful"); rootItem.visible = false
                                } else { MODREADER.appendToLogFile("Com-port connected but validation failed") }
                            }
                            APPSETTINGS.saveMatch(true)
                            APPSETTINGS.updateUserHistoryData(name_text_field.text)
                            MODREADER.saveNameAndPort(name_text_field.text, port_name_text_field.text, netowrk_path_text.text)
                            // Only start hardware polling in LIVE mode. In demo a
                            // COM port may be open with no target answering, and
                            // on_pushButton_clicked's blocking reads would freeze
                            // the transition to the shooting page.
                            if (appMode && mod_connected) { MODREADER.on_pushButton_clicked(); MODREADER.on_pushButton_2_clicked() }
                            MODREADER.resetShootinCount()
                        }
                    }
                }
            }
        }

        // ── Footer strip ──────────────────────────────────────────────────────
        Rectangle {
            id: contentFooter
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 34; color: _surfaceAlt; radius: 8
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                height: 1; color: _borderSub
            }
            Text {
                anchors.left: parent.left; anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: "TechAim  ·  Electronic target control"
                color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11
            }
            Row {
                anchors.right: parent.right; anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter; spacing: 12
                Text {
                    text: "Contact us"
                    color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { contactUsDia.visible = true } }
                }
                Rectangle { width: 1; height: 12; color: _borderStr; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: mod_connected ? "● Connected" : "● Offline"
                    color: mod_connected ? _green : _txtMut
                    font.family: theme.fontFamily; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle { width: 1; height: 12; color: _borderStr; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: appMode ? "LIVE" : "DEMO"
                    color: appMode ? _green : _red
                    font.family: theme.fontFamily; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle { width: 1; height: 12; color: _borderStr; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    // UI-HOME-004: the footer must not claim the share is on
                    // while it has nowhere to write.
                    text: shareIncomplete ? qsTr("☁ Share incomplete")
                         : (shareConfigured ? qsTr("☁ Share on")
                                            : qsTr("☁ Share off"))
                    color: shareIncomplete ? _warnTxt
                                           : (shareConfigured ? _green : _txtMut)
                    font.family: theme.fontFamily; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            networkShareCard.netEnabled = !networkShareCard.netEnabled
                            MODREADER.setIsServerNetworkEnabled(networkShareCard.netEnabled)
                            if (networkShareCard.netEnabled && MODREADER.getNetworkPath() === "")
                                networkFolderDialog.open()
                        }
                    }
                }
            }
        }
    } // contentArea

    // ── EULA overlay ────────────────────────────────────────────────────────────
    Rectangle {
        visible: eulaPage.visible
        color: theme.bgBase; opacity: 0.92; anchors.fill: parent; z: 50
    }

    Rectangle {
        id: eulaPage
        width: 600; height: 400
        anchors.centerIn: parent
        color: theme.bgSurface; z: 51
        visible: !APPSETTINGS.isEulaAccepted() || !MODREADER.isValidLicence()

        ScrollView {
            id: eulaScroll
            anchors.fill: parent; anchors.bottomMargin: 20
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentHeight: isDefaultIcon ? eulaFirstImage.height : eulaFirstImage.height + eulaSecondImage.height
            clip: true
            Image {
                id: eulaFirstImage
                anchors.top: parent.top; anchors.left: parent.left
                width: 500; height: 900; clip: true
                // The legacy Tachus EULA image was removed from the resource
                // manifest for RC3; only the SETA agreement ships.
                source: "qrc:/images/loginPage/End User Agreement SETA-1.png"
            }
            Image {
                id: eulaSecondImage
                anchors.top: eulaFirstImage.bottom; anchors.left: parent.left
                width: 500; height: 400; clip: true
                source: "qrc:/images/loginPage/End User Agreement SETA-2.png"
                visible: !isDefaultIcon
            }
        }
        Image {
            id: acceptBtn
            source: "qrc:/images/loginPage/reset.png"
            anchors.top: eulaScroll.bottom; anchors.right: parent.right
            width: 50; height: 20; opacity: 1
            MouseArea {
                anchors.fill: parent
                onClicked: { APPSETTINGS.eulaAccepted(); eulaPage.visible = false }
            }
        }
        Text {
            x: acceptBtn.x + (acceptBtn.width / 2) - (width / 2)
            y: acceptBtn.y + (acceptBtn.height / 2) - (height / 2) - 2
            text: qsTr("Accept"); color: "white"; font.pointSize: 12
        }
    }

    // ── Licence verification overlay ─────────────────────────────────────────
    Rectangle {
        id: licRect
        width: eulaPage.width; height: eulaPage.height
        x: eulaPage.x; y: eulaPage.y
        visible: !MODREADER.isValidLicence()
        color: theme.bgSurface; z: 52

        Rectangle {
            width: 200; height: 120; anchors.centerIn: parent
            color: "transparent"; border.color: licColor

            Rectangle {
                id: licHeaderRect; color: licColor; width: parent.width; height: 30; anchors.top: parent.top
                Text { text: qsTr("Lincence verification Process"); anchors.centerIn: parent; color: "white" }
            }
            Rectangle {
                id: emailLabelRect
                anchors.left: parent.left; anchors.leftMargin: 10
                anchors.top: licHeaderRect.bottom; anchors.topMargin: 20
                width: licEmail.width; height: 20; color: "transparent"
                Text {
                    id: licEmail; text: qsTr("e-mail id"); height: implicitHeight; width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left
                }
            }
            Rectangle {
                id: errorLabelRect
                anchors.left: parent.left; anchors.leftMargin: 10
                anchors.top: emailLabelRect.bottom; anchors.topMargin: 5
                width: licError.width; height: 20; color: "transparent"
                Text {
                    id: licError; text: qsTr("Error"); height: implicitHeight; width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left
                    visible: false; color: "red"
                }
            }
            TextField {
                id: licTextInput
                width: parent.width - emailLabelRect.width - 30; height: 20
                anchors.top: licHeaderRect.bottom
                anchors.topMargin: emailLabelRect.anchors.topMargin
                anchors.left: emailLabelRect.right; anchors.leftMargin: 10
                placeholderText: "Please enter Licenced user id"
            }
            Rectangle {
                id: cancelButton
                width: 50; height: 20
                anchors.right: parent.right;   anchors.rightMargin: 10
                anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                color: licColor
                Text { text: qsTr("Cancel"); anchors.centerIn: parent }
                MouseArea { anchors.fill: parent; onClicked: { Qt.quit() } }
            }
            Rectangle {
                id: validateButton
                width: 70; height: 20
                anchors.right: cancelButton.left; anchors.rightMargin: 10
                anchors.bottom: parent.bottom;    anchors.bottomMargin: 10
                color: licColor
                Text { text: qsTr("Validate"); anchors.centerIn: parent }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var ret = MODREADER.validateLicence(licTextInput.text)
                        if      (ret === 1) { licError.text = "No Licence file available."; licError.visible = true }
                        else if (ret === 0) { licError.text = "Invalid e-mail id.";         licError.visible = true }
                        else if (ret === 2) { licError.text = "Lincence file expired";      licError.visible = true }
                        else if (ret === 3) { licRect.visible = false }
                    }
                    onPressed:  { parent.color = "white" }
                    onReleased: { parent.color = licColor }
                }
            }
        }
    }
}
