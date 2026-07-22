import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Controls 2.5

Item {
    id: name

    property int rootItemWidth:158
    property int rootItemHeight:61

    width:158
    height: 61

    property bool isPalletRedColor: true
    property bool isBackGroundBlack: true

    // F10: true while a session is active (a Final, qualification, a completed-
    // but-not-closed course, prep/sighting/firing, recovery or an unresolved
    // incident). Bound from ShootingPage. When true the operating mode cannot
    // be changed and the selector is disabled with an explanation.
    property bool modeChangeBlocked: false
    // OPMODE.live is CONST for the process; a pending selection differing from
    // it means "restart required".
    readonly property bool opLive: (typeof OPMODE !== "undefined") ? OPMODE.live : true

    Image {
        id: settings_popup
        source: "qrc:/images/settings/settings_popup.png"
        x: ((parent.width/rootItemWidth)*1)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_green_highlight
        source: gameRange == 10 ? "qrc:/images/settings/pellet_color_yellow_highlight.png" : "qrc:/images/settings/pellet_color_red_highlight.png"
        x: ((parent.width/rootItemWidth)*98)
        y: ((parent.height/rootItemHeight)*32)
        opacity: isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                isPalletRedColor = true
            }
        }
    }
    Image {
        id: background_color_white_no_highlight
        source: "qrc:/images/settings/background_color_blue_no_highlight.png"
        x: ((parent.width/rootItemWidth)*123)
        y: ((parent.height/rootItemHeight)*6)
        opacity: isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_white_no_highlight
        source: "qrc:/images/settings/pellet_color_green_highlight.png"
        x: ((parent.width/rootItemWidth)*123)
        y: ((parent.height/rootItemHeight)*32)
        opacity: isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: background_color_black_no_highlight
        source: "qrc:/images/settings/background_color_black_no_highlight.png"
        x: ((parent.width/rootItemWidth)*98)
        y: ((parent.height/rootItemHeight)*6)
        opacity: !isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_white_highlight
        source: "qrc:/images/settings/pellet_color_green_highlight_1.png"
        x: ((parent.width/rootItemWidth)*124)
        y: ((parent.height/rootItemHeight)*33)
        opacity: !isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                isPalletRedColor = false
                console.log("*************test")
            }
        }
    }
    Image {
        id: pellet_color_green_highlight_1
        source: gameRange == 10 ? "qrc:/images/settings/pellet_color_yellow_no_highlight.png" : "qrc:/images/settings/pellet_color_red_no_highlight.png"
        x: ((parent.width/rootItemWidth)*99)
        y: ((parent.height/rootItemHeight)*33)
        opacity: !isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: background_color_white_highlight
        source: "qrc:/images/settings/background_color_blue_highlight.png"
        x: ((parent.width/rootItemWidth)*124)
        y: ((parent.height/rootItemHeight)*6)
        opacity: !isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                isBackGroundBlack = false
            }
        }
    }
    Image {
        id: background_color_black_highlight
        source: "qrc:/images/settings/background_color_black_highlight.png"
        x: ((parent.width/rootItemWidth)*99)
        y: ((parent.height/rootItemHeight)*6)
        opacity: isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                isBackGroundBlack = true
            }
        }
    }

    // png text
    Rectangle {
        anchors.fill: settings_popup
        color: "transparent"

        Rectangle {
            width: parent.width
            height: parent.height/2 - 10
            color: "transparent"
            anchors.top: parent.top
            anchors.topMargin: 5

            Text {
                text: qsTr("Background Color")
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter            }
        }
        Rectangle {
            width: parent.width
            height: parent.height/2 - 10
            color: "transparent"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10

            Text {
                text: qsTr("Pellet Color")
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // ── Motor feed times (paper-advance duration in seconds) ─────────────
    Rectangle {
        id: motorSection
        anchors.top: settings_popup.bottom
        anchors.left: settings_popup.left
        width: settings_popup.width
        height: 96
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Text {
                text: qsTr("MOTOR FEED (SECONDS)")
                color: "#9a9ba0"; font.pixelSize: 9
                font.bold: true; font.letterSpacing: 1
            }
            Row {
                spacing: 8
                Column {
                    spacing: 2
                    Text { text: qsTr("Match"); color: "white"; font.pixelSize: 9 }
                    Rectangle {
                        width: 52; height: 20; radius: 3
                        color: "#15161a"; border.color: "#3a3b40"
                        TextInput {
                            id: motorMatchInput
                            anchors.fill: parent; anchors.margins: 3
                            color: "white"; font.pixelSize: 11
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            text: APPSETTINGS.getMotor_movement_time().toFixed(1)
                        }
                    }
                }
                Column {
                    spacing: 2
                    Text { text: qsTr("Sighter"); color: "white"; font.pixelSize: 9 }
                    Rectangle {
                        width: 52; height: 20; radius: 3
                        color: "#15161a"; border.color: "#3a3b40"
                        TextInput {
                            id: motorSighterInput
                            anchors.fill: parent; anchors.margins: 3
                            color: "white"; font.pixelSize: 11
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            text: APPSETTINGS.getMotor_movement_time_sighter().toFixed(1)
                        }
                    }
                }
            }
            Rectangle {
                width: 70; height: 20; radius: 3
                color: "#e8003d"
                Text {
                    id: motorSaveText
                    anchors.centerIn: parent
                    text: qsTr("Save")
                    color: "white"; font.pixelSize: 10; font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var m = parseFloat(motorMatchInput.text)
                        var s = parseFloat(motorSighterInput.text)
                        if (isNaN(m) || m <= 0 || m > 60) m = 2.5
                        if (isNaN(s) || s <= 0 || s > 60) s = 2.5
                        motorMatchInput.text = m.toFixed(1)
                        motorSighterInput.text = s.toFixed(1)
                        APPSETTINGS.saveMotorTimes(m, s)
                        motorSaveText.text = qsTr("Saved ✓")
                        motorSavedReset.restart()
                    }
                }
                Timer {
                    id: motorSavedReset
                    interval: 1500
                    onTriggered: motorSaveText.text = qsTr("Save")
                }
            }
        }
    }

    // ── F10: OPERATING MODE selector ─────────────────────────────────────
    // Live target (physical) vs Demo / simulation. The persisted source of the
    // mode is config.ini [App_Settings]/app_mode; changing it here writes that
    // key atomically and requires a restart. Blocked while a session is active.
    Rectangle {
        id: operatingModeSection
        anchors.top: motorSection.bottom
        anchors.left: settings_popup.left
        anchors.topMargin: 6
        width: settings_popup.width
        height: opModeCol.implicitHeight + 16
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1

        // 0 = Live, 1 = Demo. Selection previews via OPMODE; running mode is fixed.
        property int runningMode: name.opLive ? 0 : 1

        Column {
            id: opModeCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 8; spacing: 5

            Text {
                text: qsTr("OPERATING MODE")
                color: "#9a9ba0"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
            }

            // Live target option (whole row clickable via the MouseArea wrapper)
            MouseArea {
                width: opModeCol.width; height: liveRow.implicitHeight
                enabled: !name.modeChangeBlocked && !name.opLive
                onClicked: name.beginModeChange(0)
                Row {
                    id: liveRow
                    spacing: 7
                    Rectangle {
                        width: 13; height: 13; radius: 7
                        anchors.verticalCenter: parent.verticalCenter
                        color: "transparent"; border.width: 2
                        border.color: operatingModeSection.runningMode === 0 ? "#37c76a" : "#5a5b62"
                        Rectangle {
                            anchors.centerIn: parent; width: 7; height: 7; radius: 4
                            color: "#37c76a"; visible: operatingModeSection.runningMode === 0
                        }
                    }
                    Column {
                        spacing: 0
                        Text { text: qsTr("Live target"); color: "white"; font.pixelSize: 11; font.bold: true }
                        Text { text: qsTr("Receives shots from the physical TechAim target.")
                               color: "#9a9ba0"; font.pixelSize: 8 }
                    }
                }
            }

            // Demo / simulation option
            MouseArea {
                width: opModeCol.width; height: demoRow.implicitHeight
                enabled: !name.modeChangeBlocked && name.opLive
                onClicked: name.beginModeChange(1)
                Row {
                    id: demoRow
                    spacing: 7
                    Rectangle {
                        width: 13; height: 13; radius: 7
                        anchors.verticalCenter: parent.verticalCenter
                        color: "transparent"; border.width: 2
                        border.color: operatingModeSection.runningMode === 1 ? "#e8003d" : "#5a5b62"
                        Rectangle {
                            anchors.centerIn: parent; width: 7; height: 7; radius: 4
                            color: "#e8003d"; visible: operatingModeSection.runningMode === 1
                        }
                    }
                    Column {
                        spacing: 0
                        Text { text: qsTr("Demo / simulation"); color: "white"; font.pixelSize: 11; font.bold: true }
                        Text { text: qsTr("Allows simulated target shots for testing and demonstration.")
                               color: "#9a9ba0"; font.pixelSize: 8 }
                    }
                }
            }

            // Status line: current running mode.
            Text {
                text: qsTr("Current mode: ") + (name.opLive ? qsTr("LIVE TARGET") : qsTr("DEMO / SIMULATION"))
                color: name.opLive ? "#8fe0a8" : "#ff9aa8"
                font.pixelSize: 9; font.bold: true
            }
            // Blocked reason OR restart-required hint OR "tap to change" hint.
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 8
                color: name.modeChangeBlocked ? "#ffb46b"
                       : ((typeof OPMODE !== "undefined" && OPMODE.restartRequired) ? "#ffd0d7" : "#8a8b90")
                text: name.modeChangeBlocked
                        ? qsTr("Operating mode cannot be changed while a session is active. Close or complete the current session first.")
                      : ((typeof OPMODE !== "undefined" && OPMODE.restartRequired)
                            ? qsTr("Restart required — the new mode takes effect on next launch.")
                            : qsTr("Select the other option to switch (requires restart)."))
            }
        }
    }

    // Begin a mode change: preview the selection and open the confirm dialog.
    function beginModeChange(target) {
        if (modeChangeBlocked || typeof OPMODE === "undefined")
            return
        if ((target === 0 && opLive) || (target === 1 && !opLive))
            return    // already running that mode
        OPMODE.selectMode(target)
        opModeConfirm.targetMode = target
        opModeConfirm.open()
    }

    // Self-contained TechAim-styled confirm (Restart Now / Later / Cancel). Not
    // a native dialog — the framework's showConfirmation is two-button only, and
    // this needs three, mirroring main.cpp's bespoke single-instance dialog.
    Popup {
        id: opModeConfirm
        property int targetMode: 1     // the mode being switched TO
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: 360
        padding: 0
        background: Rectangle { color: "#1f2026"; radius: 13; border.color: "#3a3b42"; border.width: 1 }
        Overlay.modal: Rectangle { color: "#aa000000" }

        contentItem: Column {
            spacing: 12
            padding: 22
            width: opModeConfirm.width

            Text {
                width: parent.width - 44
                text: opModeConfirm.targetMode === 1
                      ? qsTr("Switch to Demo mode?")
                      : qsTr("Switch to Live target mode?")
                color: "#f2f3f5"; font.pixelSize: 16; font.bold: true
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width - 44
                text: opModeConfirm.targetMode === 1
                      ? qsTr("Simulated shots will be enabled. Demo sessions are intended for testing and cannot be treated as Live target results.\n\nThe application must restart before the change takes effect.")
                      : qsTr("Simulated shot input will be disabled. The application will expect the physical TechAim target connection.\n\nThe application must restart before the change takes effect.")
                color: "#b6b9c0"; font.pixelSize: 11; wrapMode: Text.WordWrap; lineHeight: 1.15
            }
            Item {
                width: parent.width - 44
                height: 34
                Row {
                anchors.right: parent.right
                spacing: 8

                // Cancel — leaves the configuration unchanged.
                Rectangle {
                    width: 74; height: 32; radius: 8
                    color: "transparent"; border.color: "#5a5b62"; border.width: 1
                    Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: "#c9ced6"; font.pixelSize: 11 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // Revert the preview so no restart is pending; nothing was written.
                            if (typeof OPMODE !== "undefined") OPMODE.selectMode(name.opLive ? 0 : 1)
                            opModeConfirm.close()
                        }
                    }
                }
                // Restart Later — write the setting now; apply on next launch.
                Rectangle {
                    width: 104; height: 32; radius: 8
                    color: "#2b2c33"; border.color: "#5a5b62"; border.width: 1
                    Text { anchors.centerIn: parent; text: qsTr("Restart Later"); color: "white"; font.pixelSize: 11 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            OPMODE.applyModeChange(name.modeChangeBlocked)
                            opModeConfirm.close()
                        }
                    }
                }
                // Restart Now — write the setting, then relaunch.
                Rectangle {
                    width: 104; height: 32; radius: 8
                    color: "#e8003d"
                    Text { anchors.centerIn: parent; text: qsTr("Restart Now"); color: "white"; font.pixelSize: 11; font.bold: true }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (OPMODE.applyModeChange(name.modeChangeBlocked))
                                OPMODE.requestRestart()
                            opModeConfirm.close()
                        }
                    }
                }
                }
            }
        }
    }

    // ── About / build identity (F9B) — embedded at compile time; lets the
    // operator confirm the release executable matches the committed source.
    Rectangle {
        id: aboutSection
        anchors.top: operatingModeSection.bottom
        anchors.left: settings_popup.left
        anchors.topMargin: 6
        width: settings_popup.width
        height: aboutCol.implicitHeight + 16
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1
        Column {
            id: aboutCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 8; spacing: 3
            Text {
                text: qsTr("ABOUT / BUILD")
                color: "#9a9ba0"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
            }
            Text {
                text: "TechAim " + (typeof BUILDINFO !== "undefined" ? BUILDINFO.version : "?")
                color: "white"; font.pixelSize: 12; font.bold: true
            }
            Text {
                text: (typeof BUILDINFO !== "undefined" ? BUILDINFO.config : "?") + qsTr(" build")
                color: "#c9ced6"; font.pixelSize: 10
            }
            Text {
                text: qsTr("Commit: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.commit : "?")
                color: "#c9ced6"; font.pixelSize: 10
            }
            Text {
                text: qsTr("Built: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.built : "?")
                color: "#c9ced6"; font.pixelSize: 10
            }
        }
    }

    function startFromServer()
    {

    }
}
