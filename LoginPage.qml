import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Dialogs
import QtQuick.Window 2.2

Item {
    id: rootItem
    property int rootItemWidth: 1366
    property int rootItemHeight: 724

    property bool demoMode: true
    property bool connectToMaster: false
    property alias username_loginPage: name_text_field.text
    property int gameMode: 0 // 0 -> pistol, 1 -> rifle
    property int gameEvent: 0
    property int gameType: 1 // 1->sighter, 0->match
    property int papermode: 0
    property bool mod_connected: false
    property bool popupMode: false
    property bool showComportConnector: true
    property bool showLaneConnector: false
    property bool hideFreePractice: isDefaultIcon

    property color licColor: theme.brandPrimary
    signal loadSavedGame()
    signal sighterStartedFromServer()
    signal matchStartedFromServer()
    signal backHomeFromServer()

    onGameModeChanged: { APPSETTINGS.setGameMode(gameMode) }
    onGameEventChanged: { APPSETTINGS.setGameEvent(gameEvent) }
    onUsername_loginPageChanged: {
        console.log("**********??????????????????????*********", username_loginPage)
        APPSETTINGS.setUsername(username_loginPage)
    }
    onGameTypeChanged: {
        console.log("***************************************** ", gameType)
        if (gameType === 0) shootingPage.loadGameInMatchMode()
    }

    Component.onCompleted: {
        if (gameRange == 10) {
            if (APPSETTINGS.getGame_distance() < 5 || APPSETTINGS.getGame_distance() > 10) {
                gameDistanceDia.visible = true
            }
        }
        MODREADER.connectedModbus()
        mod_connected = MODREADER.isModBusConnected()
        if (!MODREADER.isValidLicence()) {
            // invalidLicence.visible = true
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
        width: 300; height: 100
        visible: false
    }

    MessageDialog {
        id: invalidUserName; title: "Warning"
        text: qsTr("Please enter user name to login")
        visible: false
    }

    MessageDialog {
        id: invalidLicence; title: "Error"
        text: "Licence has expired, Please contact raosrinu2004@gmail.com or rahul.mishra@tachustechnology.com"
        visible: false
        onAccepted: { Qt.quit() }
    }

    MessageDialog {
        id: gameDistanceDia; title: "Error"
        text: "Entered distance is not in the range of 5m to 10 m."
        visible: false
        onAccepted: { Qt.quit() }
    }

    MessageDialog {
        id: masterConnection; title: "Error"
        text: "Master system is not connected, Please Click \"Connect\" button."
        visible: false
    }

    MessageDialog {
        id: validateLogin; title: "Error"
        text: "Srinu"; visible: false
    }

    MessageDialog {
        id: contactUsDia; title: "Info"
        text: isDefaultIcon ? "Please contact us contact@tachustechnology.com"
                            : " Contact us on contact@seta-online.com"
        visible: false
    }

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
        onUserNameChanged: {
            username_loginPage = name
            console.log("*******************", name)
            name_text_field.text = name
        }
        onPortNumberChanged: { port_name_text_field.text = port }
        onLaneNumberChanged: { lane_number_text = lane_number }
        onStartSighter: {
            if (visible) perfromStart()
            sighterStartedFromServer()
        }
        onStartMatch: {
            if (visible) perfromStart()
            matchStartedFromServer()
        }
        onBackHome: {
            console.log("********************************", visible)
            if (!visible) backHomeFromServer()
        }
    }

    Connections {
        target: MODREADER
        onMasterConnectionChanged: {
            console.log("Master connection changed .....,", isConnected)
            disableControls()
        }
        onMatchDetails: {
            console.log("Match Details in qml .....", gametype, matchmode, sighterTime, matchtime, sigherTime, matchpf)
            gameEvent = matchmode
            gameMode = gametype
            shootingPage.applyServerSettings(sighterTime, matchtime, sigherTime, matchpf)
        }
        onStartMatchFromServer: {
            console.log("Match Started .............")
            perfromStart()
        }
        onMatchDetailsSetaModification: {
            console.log("Match Details in qml onMatchDetailsSetaModification .....", gametype, matchmode)
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

    // Old event list/mouse kept for disableControls() compatibility
    ListView  { id: gameEventList;  width: 0; height: 0; visible: false; model: 0 }
    MouseArea { id: gameEventMouse; width: 0; height: 0 }
    MouseArea { id: resetMouse;     width: 0; height: 0; onClicked: rootItem.reset() }

    // ─── JS Functions ─────────────────────────────────────────────────────────

    function validate() {
        if (username_loginPage === "" && !isSaveGame) {
            invalidUserName.visible = true
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
        if (gameRange == 10)
            return gameMode === 0 ? "10m Air Pistol" : "10m Air Rifle"
        else
            return gameMode === 0 ? "50m Pistol" : "50m Rifle Prone"
    }

    function getEventCardTitle(index) {
        var shots = getGameEventText(index)
        if (shots === "Free Practice") return getDisciplineName() + " — Free Practice"
        return getDisciplineName() + " — " + shots + " Shots"
    }

    function getEventCardSubtitle(index) {
        var shots = getGameEventText(index)
        if (shots === "Free Practice") return "Flexible training · no time limit"
        return "ISSF 2026 · " + shots + " shots"
    }

    function getEventCardBadge(index) {
        var shots = getGameEventText(index)
        return shots === "Free Practice" ? "FP" : shots
    }

    function getMatchTime() {
        var shots = getGameEventText(gameEvent)
        if (shots === "10") return "10 min"
        if (shots === "15") return "15 min"
        if (shots === "20") return "20 min"
        if (shots === "30") return "30 min"
        if (shots === "40") return "40 min"
        if (shots === "60") return "50 min"
        return "—"
    }

    function disableControls() {
        console.log("Inside disable controls ....")
        pistolMouse.visible  = false
        rifleMouse.visible   = false
        startMouse.visible   = false
        resetMouse.visible   = false
        gameEventList.enabled = false
        gameEventMouse.visible = false
    }

    function startButtonClickedOnLoadGame() {
        console.log("app mode " + appMode)
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
                masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                masterConnection.visible = true
                return
            }
            rootItem.visible = false
        } else {
            MODREADER.appendToLogFile("Application running in Live mode")
            if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                MODREADER.appendToLogFile("Master application required")
                masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                masterConnection.visible = true
                return
            }
            if (masterConnectBtn && port_name_text_field.text != "") {
                MODREADER.appendToLogFile("Application with port text field")
                MODREADER.connectedModbus(port_name_text_field.text)
                mod_connected = MODREADER.isModBusConnected()
            }
            if (!MODREADER.isModBusConnected()) {
                MODREADER.appendToLogFile("Com port not connected")
                validateLogin.text = "Com port not connected"
                validateLogin.visible = true
            } else if (!MODREADER.isHardwareConnected()) {
                validateLogin.text = "Hardware not connected."
                validateLogin.visible = true
            } else if (!MODREADER.checkAutoFeedMode()) {
                validateLogin.text = "Auto feed mode is off"
                validateLogin.visible = false
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
    // NEW VISUAL LAYOUT
    // =========================================================================

    // Background
    Rectangle {
        id: fullRect
        anchors.fill: parent
        color: theme.bgBase
    }

    // ── Header ────────────────────────────────────────────────────────────────
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 152
        color: theme.bgSurface

        // Left crimson edge stripe
        Rectangle {
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 4; color: theme.brandPrimary
        }
        // Bottom divider
        Rectangle {
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: theme.borderColor
        }
        // Dark accent band on the right third (decorative)
        Rectangle {
            anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: parent.width * 0.34; color: theme.bgSurfaceAlt
        }
        Rectangle {
            x: parent.width * 0.66 - 3; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 3; color: theme.brandPrimary; opacity: 0.25
        }

        // App identity row
        Row {
            id: identityRow
            anchors.top: parent.top; anchors.topMargin: 18
            anchors.left: parent.left; anchors.leftMargin: 28
            spacing: 10

            Canvas {
                id: targetIcon
                width: 30; height: 30
                anchors.verticalCenter: parent.verticalCenter
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    var cx = width / 2, cy = height / 2
                    var rings  = [14,  10,  7,  4]
                    var widths = [2.0, 1.5, 1.5, 2.0]
                    for (var i = 0; i < rings.length; i++) {
                        ctx.beginPath()
                        ctx.arc(cx, cy, rings[i], 0, Math.PI * 2)
                        ctx.strokeStyle = (i === 1) ? "#ffffff50" : "#a80038"
                        ctx.lineWidth = widths[i]
                        ctx.stroke()
                    }
                    ctx.beginPath(); ctx.arc(cx, cy, 2, 0, Math.PI * 2)
                    ctx.fillStyle = "#a80038"; ctx.fill()
                }
            }
            Text {
                text: "TECH AIM"; color: theme.textPrimary
                font.family: theme.fontFamily; font.pixelSize: 15
                font.bold: true; font.letterSpacing: 3
                anchors.verticalCenter: parent.verticalCenter
            }
            Text { text: "•"; color: theme.textSecondary; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: "ELECTRONIC TARGET"; color: theme.textSecondary
                font.family: theme.fontFamily; font.pixelSize: 13; font.letterSpacing: 2
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Main heading
        Text {
            id: mainHeading
            anchors.bottom: subHeading.top; anchors.bottomMargin: 6
            anchors.left: parent.left; anchors.leftMargin: 28
            text: "Start a shooting session"
            color: theme.textPrimary; font.family: theme.fontFamily
            font.pixelSize: 28; font.bold: true
        }
        Text {
            id: subHeading
            anchors.bottom: parent.bottom; anchors.bottomMargin: 22
            anchors.left: parent.left; anchors.leftMargin: 28
            text: "Choose a discipline and shot plan. TechAim configures the target, scoring and timing."
            color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 12
        }

        // Tagline (right accent band)
        Text {
            anchors.verticalCenter: subHeading.verticalCenter
            anchors.right: parent.right; anchors.rightMargin: 28
            text: "WE AIM TO PLEASE"
            color: theme.brandAccent; font.family: theme.fontFamily
            font.pixelSize: 11; font.bold: true; font.letterSpacing: 3
        }
        // Logo
        Image {
            source: theme.logoWhite
            anchors.right: parent.right; anchors.rightMargin: 28
            anchors.top: parent.top; anchors.topMargin: 16
            height: 34; fillMode: Image.PreserveAspectFit; opacity: 0.85
        }
    }

    // ── Two-column content area ────────────────────────────────────────────────
    Item {
        id: contentArea
        anchors.top: headerBar.bottom; anchors.topMargin: 14
        anchors.left: parent.left;     anchors.leftMargin: 14
        anchors.right: parent.right;   anchors.rightMargin: 14
        anchors.bottom: statusBar.top; anchors.bottomMargin: 14

        // ── LEFT PANEL: Session details ───────────────────────────────────────
        Rectangle {
            id: leftPanel
            anchors.top: parent.top; anchors.left: parent.left; anchors.bottom: parent.bottom
            width: Math.floor(parent.width * 0.46)
            color: theme.bgSurface; radius: theme.radiusMedium
            border.color: theme.borderColor; border.width: 1
            clip: true

            // Left crimson accent stripe
            Rectangle {
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 3; color: theme.brandPrimary
            }

            // Panel title
            Text {
                id: panelTitle
                anchors.top: parent.top; anchors.topMargin: 22
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "Session details"
                color: theme.textPrimary; font.family: theme.fontFamily
                font.pixelSize: 16; font.bold: true
            }

            // ATHLETE label
            Text {
                id: athleteLabel
                anchors.top: panelTitle.bottom; anchors.topMargin: 20
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "ATHLETE"
                color: theme.textSecondary; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
            }

            // Athlete input box
            Rectangle {
                id: athleteBox
                anchors.top: athleteLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                height: 44
                color: theme.bgSurfaceAlt; radius: theme.radiusSmall
                border.color: name_text_field.activeFocus ? theme.brandPrimary : theme.borderColor
                border.width: name_text_field.activeFocus ? 2 : 1

                TextInput {
                    id: name_text_field
                    anchors.left: parent.left;  anchors.leftMargin: 14
                    anchors.right: historyBtn.left; anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: theme.fontFamily; font.pixelSize: 14
                    color: theme.textPrimary; maximumLength: 20
                    onTextChanged: { username_loginPage = text }
                }
                Rectangle {
                    id: historyBtn
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 36; color: "transparent"; radius: theme.radiusSmall
                    Text { text: "▾"; color: theme.textSecondary; font.pixelSize: 16; anchors.centerIn: parent }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (APPSETTINGS.getUserHistoryCount() > 0)
                                userHistoryList.visible = !userHistoryList.visible
                        }
                    }
                }
            }

            // History dropdown
            ListView {
                id: userHistoryList
                anchors.top: athleteBox.bottom
                anchors.left: athleteBox.left; anchors.right: athleteBox.right
                height: Math.min(count, 4) * 44
                visible: false; clip: true; z: 20
                model: APPSETTINGS.getUserHistoryCount()
                onVisibleChanged: { model = 0; model = APPSETTINGS.getUserHistoryCount() }
                delegate: Rectangle {
                    width: userHistoryList.width; height: 44
                    color: theme.bgSurfaceAlt
                    border.color: theme.borderColor; border.width: 1
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left; anchors.leftMargin: 14
                        text: APPSETTINGS.getUserHistoryData(index)
                        color: theme.textPrimary; font.family: theme.fontFamily; font.pixelSize: 14
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: parent.color = theme.bgBase
                        onExited:  parent.color = theme.bgSurfaceAlt
                        onClicked: {
                            username_loginPage = APPSETTINGS.getUserHistoryData(index)
                            userHistoryList.visible = false
                        }
                    }
                }
            }

            // TARGET CONNECTION label
            Text {
                id: connLabel
                anchors.top: athleteBox.bottom; anchors.topMargin: 18
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "TARGET CONNECTION"
                color: theme.textSecondary; font.family: theme.fontFamily
                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                visible: showComportConnector
            }

            // Port + connection status row
            Row {
                id: connRow
                anchors.top: connLabel.bottom; anchors.topMargin: 6
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                height: 44; spacing: 10
                visible: showComportConnector

                Rectangle {
                    width: parent.width - connStatusBtn.width - 10; height: 44
                    color: theme.bgSurfaceAlt; radius: theme.radiusSmall
                    border.color: port_name_text_field.activeFocus ? theme.brandPrimary : theme.borderColor
                    border.width: port_name_text_field.activeFocus ? 2 : 1
                    TextInput {
                        id: port_name_text_field
                        anchors.left: parent.left;  anchors.leftMargin: 14
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: theme.fontFamily; font.pixelSize: 14
                        color: theme.textPrimary; maximumLength: 5
                    }
                }

                Rectangle {
                    id: connStatusBtn
                    width: 130; height: 44; radius: theme.radiusSmall
                    color: mod_connected ? "#0d1f11" : theme.bgSurfaceAlt
                    border.color: mod_connected ? theme.statusConnected : theme.borderColor; border.width: 1
                    Row {
                        anchors.centerIn: parent; spacing: 7
                        Rectangle {
                            width: 7; height: 7; radius: 4
                            color: mod_connected ? theme.statusConnected : theme.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: mod_connected ? "Connected" : "Demo / Offline"
                            color: mod_connected ? theme.statusConnected : theme.textSecondary
                            font.family: theme.fontFamily; font.pixelSize: 11
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!mod_connected) {
                                if (popupMode) {
                                    MODREADER.connectedModbus()
                                    mod_connected = MODREADER.isModBusConnected()
                                    if (!mod_connected) modBusConnector.visible = true
                                }
                            } else {
                                MODREADER.disconnectModbus()
                                mod_connected = MODREADER.isModBusConnected()
                            }
                        }
                    }
                }
            }

            // Selected profile label
            Text {
                id: profileLabel
                anchors.top: showComportConnector ? connRow.bottom : athleteBox.bottom
                anchors.topMargin: 22
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "Selected profile"
                color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 11
            }
            Text {
                id: profileName
                anchors.top: profileLabel.bottom; anchors.topMargin: 4
                anchors.left: parent.left; anchors.leftMargin: 26
                text: getDisciplineName() + " — ISSF Match"
                color: theme.textPrimary; font.family: theme.fontFamily
                font.pixelSize: 17; font.bold: true
            }

            // Info tiles grid (3×2)
            Grid {
                id: infoTiles
                anchors.top: profileName.bottom; anchors.topMargin: 14
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                columns: 3; spacing: 8

                Repeater {
                    model: [
                        { lbl: "SHOT PLAN", val: getGameEventText(gameEvent) === "Free Practice" ? "Free" : getGameEventText(gameEvent) + " shots" },
                        { lbl: "SCORING",   val: "Decimal" },
                        { lbl: "PREP",      val: "15 min" },
                        { lbl: "MATCH",     val: getMatchTime() },
                        { lbl: "DISTANCE",  val: gameRange + " m" },
                        { lbl: "RULES",     val: "ISSF 2026" }
                    ]
                    delegate: Rectangle {
                        width: (infoTiles.width - 16) / 3; height: 56
                        color: theme.bgSurfaceAlt; radius: theme.radiusSmall
                        border.color: theme.borderColor; border.width: 1
                        Column {
                            anchors.centerIn: parent; spacing: 5
                            Text {
                                text: modelData.lbl; color: theme.textSecondary
                                font.family: theme.fontFamily; font.pixelSize: 9; font.letterSpacing: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: modelData.val; color: theme.textPrimary
                                font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }

            // Bottom action buttons
            Row {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 22
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                height: 50; spacing: 12

                // Load saved session
                Rectangle {
                    width: (parent.width - 12) * 0.42; height: 50
                    color: "transparent"; radius: theme.radiusMedium
                    border.color: theme.borderColor; border.width: 1
                    Text {
                        text: "Load saved session"
                        color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 12
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: parent.border.color = theme.textSecondary
                        onExited:  parent.border.color = theme.borderColor
                        onClicked: {
                            APPSETTINGS.uploadGame()
                            username_loginPage = APPSETTINGS.getUserName()
                            gameMode  = APPSETTINGS.getGameMode()
                            gameEvent = APPSETTINGS.getGameEvent()
                            gameType  = APPSETTINGS.getGameType()
                            if (gameType == 0) MODREADER.changeSighterMode(false)
                            console.log(" srinivas --- ", gameType)
                            if (userName != "") {
                                isSaveGame = true
                                startButtonClickedOnLoadGame()
                            }
                        }
                    }
                }

                // Start session (crimson)
                Rectangle {
                    id: startSessionRect
                    width: (parent.width - 12) * 0.58; height: 50
                    color: theme.brandPrimary; radius: theme.radiusMedium
                    opacity: startMouse.visible ? 1.0 : 0.4
                    Text {
                        text: "Start session"
                        color: theme.textOnBrand; font.family: theme.fontFamily
                        font.pixelSize: 14; font.bold: true
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: startMouse
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: startSessionRect.opacity = startMouse.visible ? 0.82 : 0.4
                        onExited:  startSessionRect.opacity = startMouse.visible ? 1.0  : 0.4
                        onClicked: {
                            if (!appMode) {
                                MODREADER.appendToLogFile("Application running in demo mode")
                                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                                    masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                                    masterConnection.visible = true
                                    return
                                }
                                rootItem.visible = false
                            } else {
                                MODREADER.appendToLogFile("Application running in Live mode")
                                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                                    MODREADER.appendToLogFile("Master application required")
                                    masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                                    masterConnection.visible = true
                                    return
                                }
                                if (masterConnectBtn && port_name_text_field.text != "") {
                                    MODREADER.appendToLogFile("Application with port text field")
                                    MODREADER.connectedModbus(port_name_text_field.text)
                                    mod_connected = MODREADER.isModBusConnected()
                                }
                                if (!MODREADER.isModBusConnected()) {
                                    MODREADER.appendToLogFile("Com port not connected")
                                    validateLogin.text = "Com port not connected"
                                    validateLogin.visible = true
                                } else if (!MODREADER.isHardwareConnected()) {
                                    validateLogin.text = "Hardware not connected."
                                    validateLogin.visible = true
                                } else if (!MODREADER.checkAutoFeedMode()) {
                                    validateLogin.text = "Auto feed mode is off"
                                    validateLogin.visible = false
                                } else if (validate()) {
                                    MODREADER.appendToLogFile("Validation was successful")
                                    rootItem.visible = false
                                } else {
                                    MODREADER.appendToLogFile("Com-port connected but validation failed")
                                }
                            }
                            APPSETTINGS.saveMatch(true)
                            APPSETTINGS.updateUserHistoryData(name_text_field.text)
                            MODREADER.saveNameAndPort(name_text_field.text, port_name_text_field.text)
                            if (mod_connected) {
                                MODREADER.on_pushButton_clicked()
                                MODREADER.on_pushButton_2_clicked()
                            }
                            MODREADER.resetShootinCount()
                        }
                    }
                }
            }
        } // leftPanel

        // ── RIGHT PANEL: Choose an event ──────────────────────────────────────
        Rectangle {
            id: rightPanel
            anchors.top: parent.top; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.left: leftPanel.right; anchors.leftMargin: 14
            color: theme.bgSurface; radius: theme.radiusMedium
            border.color: theme.borderColor; border.width: 1; clip: true

            // Panel title
            Text {
                id: rightTitle
                anchors.top: parent.top; anchors.topMargin: 22
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "Choose an event"
                color: theme.textPrimary; font.family: theme.fontFamily
                font.pixelSize: 16; font.bold: true
            }
            Text {
                id: rightSubtitle
                anchors.top: rightTitle.bottom; anchors.topMargin: 4
                anchors.left: parent.left; anchors.leftMargin: 26
                text: "The official match settings are applied automatically."
                color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 11
            }

            // Weapon selector (PISTOL | RIFLE)
            Row {
                id: weaponRow
                anchors.top: rightSubtitle.bottom; anchors.topMargin: 16
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                height: 44; spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2; height: 44; radius: theme.radiusSmall
                    color: gameMode === 0 ? "#1fa80038" : theme.bgSurfaceAlt
                    border.color: gameMode === 0 ? theme.brandPrimary : theme.borderColor
                    border.width: gameMode === 0 ? 2 : 1
                    Row {
                        anchors.centerIn: parent; spacing: 10
                        Image {
                            source: "qrc:/images/loginPage/iconPistol.png"
                            width: 22; height: 22; fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: gameMode === 0 ? 1.0 : 0.3
                        }
                        Text {
                            text: "PISTOL"
                            color: gameMode === 0 ? theme.brandPrimary : theme.textSecondary
                            font.family: theme.fontFamily; font.pixelSize: 13
                            font.bold: gameMode === 0; font.letterSpacing: 1
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        id: pistolMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: { papermode = 0; gameMode = 0 }
                    }
                }

                Rectangle {
                    width: (parent.width - 10) / 2; height: 44; radius: theme.radiusSmall
                    color: gameMode === 1 ? "#1fa80038" : theme.bgSurfaceAlt
                    border.color: gameMode === 1 ? theme.brandPrimary : theme.borderColor
                    border.width: gameMode === 1 ? 2 : 1
                    Row {
                        anchors.centerIn: parent; spacing: 10
                        Image {
                            source: "qrc:/images/loginPage/iconRifle.png"
                            width: 22; height: 22; fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: gameMode === 1 ? 1.0 : 0.3
                        }
                        Text {
                            text: "RIFLE"
                            color: gameMode === 1 ? theme.brandPrimary : theme.textSecondary
                            font.family: theme.fontFamily; font.pixelSize: 13
                            font.bold: gameMode === 1; font.letterSpacing: 1
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        id: rifleMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: { papermode = 0; gameMode = 1 }
                    }
                }
            }

            // Scrollable event cards
            ScrollView {
                id: eventScroll
                anchors.top: weaponRow.bottom; anchors.topMargin: 14
                anchors.left: parent.left;   anchors.leftMargin: 26
                anchors.right: parent.right; anchors.rightMargin: 26
                anchors.bottom: parent.bottom; anchors.bottomMargin: 22
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Column {
                    width: eventScroll.width
                    spacing: 10

                    Repeater {
                        model: 6
                        delegate: Rectangle {
                            width: parent.width; height: 70; radius: theme.radiusMedium
                            color: gameEvent === index ? "#1fa80038" : theme.bgSurfaceAlt
                            border.color: gameEvent === index ? theme.brandPrimary : theme.borderColor
                            border.width: gameEvent === index ? 2 : 1
                            visible: (hideFreePractice && index === 5) ? false : true

                            Row {
                                anchors.left: parent.left;  anchors.leftMargin: 14
                                anchors.right: parent.right; anchors.rightMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 14

                                // Badge circle
                                Rectangle {
                                    width: 40; height: 40; radius: 20
                                    color: gameEvent === index ? theme.brandPrimary : "#28282e"
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: getEventCardBadge(index)
                                        color: gameEvent === index ? theme.textOnBrand : theme.textSecondary
                                        font.pixelSize: index === 5 ? 9 : 12; font.bold: true
                                        anchors.centerIn: parent
                                    }
                                }

                                // Title + subtitle
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 4
                                    width: parent.width - 40 - 24 - 14 * 3
                                    Text {
                                        text: getEventCardTitle(index)
                                        color: gameEvent === index ? theme.textPrimary : theme.textSecondary
                                        font.family: theme.fontFamily; font.pixelSize: 13
                                        font.bold: gameEvent === index
                                        elide: Text.ElideRight; width: parent.width
                                    }
                                    Text {
                                        text: getEventCardSubtitle(index)
                                        color: theme.textSecondary
                                        font.family: theme.fontFamily; font.pixelSize: 11
                                    }
                                }

                                // Radio button indicator
                                Rectangle {
                                    width: 20; height: 20; radius: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: "transparent"
                                    border.color: gameEvent === index ? theme.brandPrimary : theme.borderColor
                                    border.width: 2
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 10; height: 10; radius: 5
                                        color: theme.brandPrimary
                                        visible: gameEvent === index
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true
                                onEntered: { if (gameEvent !== index) parent.color = theme.bgBase }
                                onExited:  { parent.color = gameEvent === index ? "#1fa80038" : theme.bgSurfaceAlt }
                                onClicked: { gameEvent = index }
                            }
                        }
                    }
                }
            }
        } // rightPanel
    } // contentArea

    // ── Status bar ─────────────────────────────────────────────────────────────
    Rectangle {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left; anchors.right: parent.right
        height: 34; color: theme.bgSurface

        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: theme.borderColor
        }
        Rectangle {
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 4; color: theme.brandPrimary
        }

        Text {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "TechAim • Electronic target control"
            color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 11
        }

        Row {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16

            Text {
                text: "Contact us"
                color: theme.textSecondary; font.family: theme.fontFamily; font.pixelSize: 11
                MouseArea {
                    anchors.fill: parent
                    onClicked: { contactUsDia.visible = true }
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Rectangle { width: 1; height: 14; color: theme.borderColor; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: appMode ? "LIVE MODE" : "DEMO MODE"
                color: appMode ? theme.statusConnected : theme.brandAccent
                font.family: theme.fontFamily; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
            }
        }
    }

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
