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
    function trainingDisciplineId() {
        if (gameMode === 0) return "AP10"
        if (gameRange === 10) return "AR10"
        return gameSubMode === 0 ? "PRONE50" : "3P50"
    }
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

    // ── Refined color palette ─────────────────────────────────────────────────
    readonly property color _bg:         "#0B0D10"
    readonly property color _surface:    "#15171C"
    readonly property color _surfaceAlt: "#1B1E24"
    readonly property color _input:      "#1D2026"
    readonly property color _borderSub:  "#2A2E36"
    readonly property color _borderStr:  "#3A404A"
    readonly property color _red:        "#C40046"
    readonly property color _redHover:   "#E00052"
    readonly property color _redDark:    "#2D0A18"
    readonly property color _txt:        "#F3F6FA"
    readonly property color _txtSec:     "#AAB4C0"
    readonly property color _txtMut:     "#6F7A86"
    readonly property color _green:      "#20C997"
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
    Image { id: bg; source: isDefaultIcon ? "qrc:/images/loginPage/bg_tachus.png" : "qrc:/images/loginPage/login_page_29Oct.png"; anchors.fill: parent; visible: false }
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
            }
        }
        APPSETTINGS.saveMatch(true)
    }

    // =========================================================================
    // VISUAL LAYOUT
    // =========================================================================

    Rectangle { anchors.fill: parent; color: _bg }

    // ── Header (74 px — compact app bar + session title) ─────────────────────
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 74
        color: "#0C0E12"

        Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: _red }
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: _borderSub }

        // Logo — right side, subtle
        Image {
            source: theme.logoWhite
            anchors.right: parent.right; anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            height: 26; fillMode: Image.PreserveAspectFit; opacity: 0.55
        }

        // App identity (top row)
        Row {
            id: identityRow
            anchors.top: parent.top; anchors.topMargin: 11
            anchors.left: parent.left; anchors.leftMargin: 20
            height: 18; spacing: 10

            Canvas {
                id: targetIcon
                width: 18; height: 18
                anchors.verticalCenter: parent.verticalCenter
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    var cx = 9, cy = 9
                    var rings = [8, 5.5, 3.5, 1.5]
                    for (var i = 0; i < rings.length; i++) {
                        ctx.beginPath()
                        ctx.arc(cx, cy, rings[i], 0, Math.PI * 2)
                        ctx.strokeStyle = i % 2 === 0 ? "#C40046" : "#ffffff22"
                        ctx.lineWidth = 1.2
                        ctx.stroke()
                    }
                    ctx.beginPath(); ctx.arc(cx, cy, 1.2, 0, Math.PI * 2)
                    ctx.fillStyle = "#C40046"; ctx.fill()
                }
            }
            Text {
                text: "TECH AIM"; color: _txt
                font.family: theme.fontFamily; font.pixelSize: 12
                font.bold: true; font.letterSpacing: 2.5
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle { width: 1; height: 12; color: _borderStr; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: "ELECTRONIC TARGET CONTROL"; color: _txtMut
                font.family: theme.fontFamily; font.pixelSize: 11; font.letterSpacing: 1.5
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Session title row (bottom)
        Row {
            anchors.bottom: parent.bottom; anchors.bottomMargin: 11
            anchors.left: parent.left; anchors.leftMargin: 20
            height: 28; spacing: 12

            Text {
                text: "Start session"; color: _txt
                font.family: theme.fontFamily; font.pixelSize: 23; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                height: 20; radius: 4
                width: _modeBadge.implicitWidth + 14
                color: appMode ? "#0d2018" : "#2a0b10"
                border.color: appMode ? _green : _red; border.width: 1
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: _modeBadge
                    anchors.centerIn: parent
                    text: appMode ? "LIVE" : "DEMO"
                    color: appMode ? _green : _red
                    font.family: theme.fontFamily; font.pixelSize: 9; font.bold: true; font.letterSpacing: 2
                }
            }
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
            anchors.bottom: contentFooter.top; anchors.bottomMargin: 8
            width: Math.floor(parent.width * 0.44)
            color: _surface; radius: 10
            border.color: _borderSub; border.width: 1; clip: true

            Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: _red; radius: 2 }

            Text {
                id: panelTitle
                anchors.top: parent.top; anchors.topMargin: 20
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "Session details"
                color: _txt; font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
            }

            // ── ATHLETE ───────────────────────────────────────────────────────
            Text {
                id: athleteLabel
                anchors.top: panelTitle.bottom; anchors.topMargin: 16
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
                height: 46; color: _input; radius: 6
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
                height: Math.min(count, 4) * 46
                visible: false; clip: true; z: 20
                model: APPSETTINGS.getUserHistoryCount()
                onVisibleChanged: { model = 0; model = APPSETTINGS.getUserHistoryCount() }
                delegate: Rectangle {
                    width: userHistoryList.width; height: 46
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
                text: "TARGET CONNECTION"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                visible: showComportConnector
            }
            Row {
                id: connRow
                anchors.top: connLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 46; spacing: 8
                visible: showComportConnector

                Rectangle {
                    width: parent.width - connStatusBtn.width - 8; height: 46
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
                    width: 130; height: 46; radius: 6
                    color: mod_connected ? "#0d2018" : _surfaceAlt
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

            // ── OPERATING MODE (F11 fix) ──────────────────────────────────────
            // The operator-facing Live/Demo switch. Placed HERE (the idle
            // Start-session screen) because changing mode is only permitted when
            // no session is active; the in-session Settings selector is blocked.
            // Uses OPMODE + the same confirm/restart flow. Read-only display
            // falls back to appMode if OPMODE is unavailable.
            Text {
                id: opModeSectionLabel
                anchors.top: showComportConnector ? connRow.bottom : athleteBox.bottom
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
                height: 46; spacing: 8
                property bool opLive: (typeof OPMODE !== "undefined") ? OPMODE.live : appMode

                // Live target pill
                Rectangle {
                    width: (parent.width - parent.spacing) / 2; height: 46; radius: 6
                    color: opModeRow.opLive ? "#0d2018" : _input
                    border.color: opModeRow.opLive ? _green : _borderSub
                    border.width: opModeRow.opLive ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 1
                        Text { text: "LIVE TARGET"; color: opModeRow.opLive ? _green : _txt
                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                               anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: "Physical target"; color: _txtMut
                               font.family: theme.fontFamily; font.pixelSize: 8
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
                    width: (parent.width - parent.spacing) / 2; height: 46; radius: 6
                    color: !opModeRow.opLive ? "#2a0b10" : _input
                    border.color: !opModeRow.opLive ? _red : _borderSub
                    border.width: !opModeRow.opLive ? 2 : 1
                    Column {
                        anchors.centerIn: parent; spacing: 1
                        Text { text: "DEMO / SIMULATION"; color: !opModeRow.opLive ? _red : _txt
                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                               anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: "Simulated clicks"; color: _txtMut
                               font.family: theme.fontFamily; font.pixelSize: 8
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
                font.family: theme.fontFamily; font.pixelSize: 8
                color: (typeof OPMODE !== "undefined" && OPMODE.restartRequired) ? _red : _txtMut
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
                background: Rectangle { color: "#1B1E24"; radius: 13; border.color: _borderSub; border.width: 1 }
                Overlay.modal: Rectangle { color: "#AA000000" }
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
                height: 50; color: _input; radius: 6
                border.color: netEnabled ? _red : _borderSub; border.width: 1
                property bool netEnabled: true

                Row {
                    id: netInfoRow
                    anchors.left: parent.left; anchors.leftMargin: 12
                    anchors.right: netToggleTrack.left; anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter; spacing: 10
                    Text {
                        text: "☁"; font.pixelSize: 16
                        color: networkShareCard.netEnabled ? _red : _txtMut
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 2
                        Text {
                            text: networkShareCard.netEnabled ? "Share enabled" : "Share disabled"
                            color: networkShareCard.netEnabled ? _txt : _txtSec
                            font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true
                        }
                        Text {
                            text: netowrk_path_text.text !== "" ? netowrk_path_text.text : "Tap to set folder…"
                            color: netowrk_path_text.text !== "" ? _txtMut : _red
                            font.family: "Consolas"; font.pixelSize: 10
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
                    width: 38; height: 20; radius: 10
                    color: networkShareCard.netEnabled ? _red : _borderStr
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        x: networkShareCard.netEnabled ? parent.width - width - 2 : 2
                        width: 16; height: 16; radius: 8; color: "white"
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

            // ── SELECTED PROFILE ──────────────────────────────────────────────
            Text {
                id: profileLabel
                anchors.top: networkShareCard.bottom; anchors.topMargin: 16
                anchors.left: parent.left; anchors.leftMargin: 22
                text: trainingConfirmed ? "SELECTED PROGRAMME" : "SELECTED PROFILE"
                color: _txtMut; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
            }
            Text {
                id: profileName
                anchors.top: profileLabel.bottom; anchors.topMargin: 4
                anchors.left: parent.left; anchors.leftMargin: 22
                text: trainingConfirmed ? "Technical Blocks" : getDisciplineName() + " — ISSF"
                color: _txt; font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
            }
            Text {
                visible: trainingConfirmed && trainingDisciplineId() === "3P50"
                anchors.top: profileName.bottom; anchors.topMargin: 2
                anchors.left: parent.left; anchors.leftMargin: 22
                text: "POSITION FLOW   Kneeling → Prone → Standing"
                color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 9; font.letterSpacing: 1
            }

            // ── INFO TILES (T1: programme summary when Training confirmed) ────
            Grid {
                id: infoTiles
                anchors.top: profileName.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                columns: 3; spacing: 8
                Repeater {
                    model: trainingConfirmed ? [
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
                    delegate: Rectangle {
                        width: (infoTiles.width - 16) / 3; height: 52
                        color: _surfaceAlt; radius: 6; border.color: _borderSub; border.width: 1
                        Column {
                            anchors.centerIn: parent; spacing: 4
                            Text {
                                text: modelData.lbl; color: _txtMut
                                font.family: theme.fontFamily; font.pixelSize: 9; font.letterSpacing: 1.5
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: modelData.val; color: _txt
                                font.family: "Consolas"; font.pixelSize: 13; font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }

            // ── ACTION BUTTONS ────────────────────────────────────────────────
            Row {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 52; spacing: 10

                Rectangle {
                    width: (parent.width - 10) * 0.40; height: 52
                    color: "transparent"; radius: 8; border.color: _borderStr; border.width: 1
                    Text {
                        text: "Load saved session"
                        color: _txtSec; font.family: theme.fontFamily; font.pixelSize: 12; anchors.centerIn: parent
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
                    width: (parent.width - 10) * 0.60; height: 52
                    color: _startHov ? _redHover : _red; radius: 8
                    opacity: startMouse.visible ? 1.0 : 0.4
                    property bool _startHov: false
                    Text {
                        text: trainingConfirmed ? "Start training  →" : "Start session  →"
                        color: "white"; font.family: theme.fontFamily; font.pixelSize: 15; font.bold: true
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: startMouse
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: startSessionRect._startHov = true
                        onExited:  startSessionRect._startHov = false
                        onClicked: {
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
        } // leftPanel

        // ── RIGHT PANEL ───────────────────────────────────────────────────────
        Rectangle {
            id: rightPanel
            anchors.top: parent.top; anchors.right: parent.right
            anchors.bottom: contentFooter.top; anchors.bottomMargin: 8
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

            // Weapon selector (PISTOL | RIFLE)
            Row {
                id: weaponRow
                anchors.top: rightSubtitle.bottom; anchors.topMargin: 14
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: 52; spacing: 8

                Rectangle {
                    width: (parent.width - 8) / 2; height: 52; radius: 8
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
                        onClicked: { papermode = 0; gameMode = 0; rangeSelected(10); gameEvent = 0 }
                    }
                }

                Rectangle {
                    width: (parent.width - 8) / 2; height: 52; radius: 8
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
                        onClicked: { papermode = 0; gameMode = 1; gameEvent = 0 }
                    }
                }
            }

            // Distance selector (10m | 50m) — only when RIFLE is selected
            // Sub-discipline selector (Prone | 3 Positions) — only for RIFLE 50m
            // Both are stacked in a Column so they flow naturally
            Column {
                id: subDisciplineRow
                anchors.top: weaponRow.bottom
                anchors.topMargin: gameMode === 1 ? 8 : 0
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                height: gameMode === 1 ? (gameRange === 50 ? 80 : 36) : 0
                spacing: 8; clip: true

                // Distance row: 10m | 50m
                Row {
                    width: parent.width; height: 36; spacing: 8
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 36; radius: 6
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
                            onClicked: { rangeSelected(10); gameSubMode = 0; gameEvent = 0 }
                        }
                    }
                    Rectangle {
                        id: rifle50Mouse
                        width: (parent.width - 8) / 2; height: 36; radius: 6
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
                            onClicked: { rangeSelected(50); gameSubMode = 0; gameEvent = 4 }
                        }
                    }
                }

                // Prone | 3 Positions — only shown for 50m
                Row {
                    width: parent.width; height: 36; spacing: 8
                    visible: gameRange === 50
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 36; radius: 6
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
                            onClicked: { gameSubMode = 0; gameEvent = 4 }
                        }
                    }
                    Rectangle {
                        width: (parent.width - 8) / 2; height: 36; radius: 6
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
                            onClicked: { gameSubMode = 1; gameEvent = 4 }
                        }
                    }
                }
            }

            // Scrollable event cards
            ScrollView {
                id: eventScroll
                visible: practiceView === 0
                anchors.top: subDisciplineRow.bottom; anchors.topMargin: 12
                anchors.left: parent.left;   anchors.leftMargin: 22
                anchors.right: parent.right; anchors.rightMargin: 22
                anchors.bottom: parent.bottom; anchors.bottomMargin: 18
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Column {
                    id: eventColumn
                    width: eventScroll.availableWidth
                    spacing: 0

                    component EventCard: Rectangle {
                        property int eventIndex: 0
                        width: eventColumn.width; height: 72; radius: 8
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
                            onClicked: { gameEvent = eventIndex }
                        }
                    }

                    // ── OFFICIAL ISSF MATCH ────────────────────────────────────
                    Text {
                        text: "OFFICIAL ISSF MATCH"
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: 9; font.bold: true; font.letterSpacing: 2
                        topPadding: 4; bottomPadding: 8
                    }
                    // Official: 60 shots — Pistol, 10m Rifle, 50m Prone, 50m 3 Pos (20+20+20)
                    EventCard { eventIndex: 4 }

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
                        text: "PRACTICE & DEVELOPMENT"
                        color: _txtMut; font.family: theme.fontFamily
                        font.pixelSize: 9; font.bold: true; font.letterSpacing: 2
                        topPadding: 14; bottomPadding: 8
                    }
                    // OPEN PRACTICE — one card; presets select the existing
                    // practice events (identical behaviour to the old rows).
                    Rectangle {
                        readonly property bool selected: gameEvent >= 0 && gameEvent <= 3 && !trainingConfirmed
                        width: eventColumn.width
                        height: selected ? 132 : 72
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
                                    color: parent.parent.parent.selected ? _red : _borderStr
                                    Text { text: "OP"; color: "white"; font.family: "Consolas"
                                           font.pixelSize: 12; font.bold: true; anchors.centerIn: parent }
                                }
                                Column {
                                    spacing: 3
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { text: "OPEN PRACTICE"
                                           color: _txt; font.family: theme.fontFamily
                                           font.pixelSize: 13; font.bold: true }
                                    Text { text: "A flexible shooting session. Choose a shot plan below.\nPlans come from the discipline's event definitions."
                                           color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                                }
                            }
                            // Compact presets (existing gameEvents; 10/30 stay 10m-only)
                            Row {
                                visible: parent.parent.selected
                                spacing: 6
                                Repeater {
                                    model: (gameMode === 1 && gameRange === 50)
                                           ? [ {e: 1, t: "20"}, {e: 3, t: "40"}, {e: 5, t: "No limit"} ]
                                           : [ {e: 0, t: "10"}, {e: 1, t: "20"}, {e: 2, t: "30"}, {e: 3, t: "40"}, {e: 5, t: "No limit"} ]
                                    delegate: Rectangle {
                                        width: 72; height: 44; radius: 6
                                        color: gameEvent === modelData.e ? _red : _input
                                        border.color: gameEvent === modelData.e ? _red : _borderSub
                                        Text { anchors.centerIn: parent
                                               text: modelData.t
                                               color: gameEvent === modelData.e ? "white" : _txtSec
                                               font.family: "Consolas"; font.pixelSize: 12; font.bold: true }
                                        MouseArea { anchors.fill: parent; onClicked: gameEvent = modelData.e }
                                    }
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: !parent.selected
                            onClicked: { trainingConfirmed = false; gameEvent = 1 }
                        }
                    }
                    Item { width: 1; height: 8 }
                    // TRAINING LAB — gateway to the programme catalogue.
                    Rectangle {
                        width: eventColumn.width; height: 72; radius: 8
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
                    Item { width: 1; height: 8 }
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
                                    width: 74; height: 18; radius: 9; color: "#0d2018"
                                    border.color: _green; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: "AVAILABLE"
                                           color: _green; font.pixelSize: 9; font.bold: true }
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
                                       color: _txtMut; font.pixelSize: 9; font.bold: true }
                            }
                        }
                        // no MouseArea: not clickable, no selector, no fake setup
                    }
                    FutureCard { title: (gameMode === 1 && gameRange === 50 && gameSubMode === 1)
                                        ? "CALL & DIAGNOSE · BY POSITION" : "CALL & DIAGNOSE" }
                    FutureCard { title: "POSITION TRANSITION"
                                 visible: gameMode === 1 && gameRange === 50 && gameSubMode === 1 }
                    FutureCard { title: "WIND MAP"; status: "PLANNED"
                                 visible: gameMode === 1 && gameRange === 50 }

                    Text { text: "INCLUDED INSIGHTS"; color: _txtMut
                           font.family: theme.fontFamily; font.pixelSize: 9; font.bold: true
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
                                            color: _txtMut; font.family: theme.fontFamily; font.pixelSize: 9
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
                        color: "#ff9aa8"; font.family: theme.fontFamily; font.pixelSize: 11
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
                                    practiceView = 0        // back to events + summary
                                }
                            }
                        }
                    }
                }
            }
        } // rightPanel

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
                    text: networkShareCard.netEnabled ? "☁ Share on" : "☁ Share off"
                    color: networkShareCard.netEnabled ? _green : _txtMut
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
                source: isDefaultIcon ? "qrc:/images/loginPage/End User Agreement Tachus-1.png"
                                      : "qrc:/images/loginPage/End User Agreement SETA-1.png"
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
