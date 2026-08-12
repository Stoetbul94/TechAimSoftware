import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Controls 2.5

// ─────────────────────────────────────────────────────────────────────────────
// Settings drawer, opened from the left pane's Settings button.
//
// This was a 158x61 raster popup: a background PNG with every control placed
// by fraction-of-a-PNG arithmetic, and the later sections (motor feed,
// operating mode, language, about) anchored BELOW it in a chain. Nothing
// bounded that chain, so the panel's own height stayed 61 px while its content
// ran off the bottom of the application - everything under Operating Mode was
// unreachable, descriptions overflowed a width that came from an image's
// sourceSize, and the colour swatches floated free of the layout.
//
// It is now a real panel: a fixed sensible drawer width, a height clamped to
// whatever the parent leaves below the anchor, and one scrolling content
// region holding every section. Per UI-DEC-007 the scroller is a Flickable
// with contentHeight bound EXPLICITLY to its column - a ScrollView measures
// implicit height, which is exactly what is unreliable with wrapped text and
// a Repeater whose delegates change size.
//
// Public API is unchanged: isBackGroundBlack, isPalletRedColor,
// modeChangeBlocked, startFromServer().
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: name

    // Colour selection. Semantics unchanged - only how they are presented.
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

    // ── panel geometry ───────────────────────────────────────────────────
    // Wide enough for a heading, a label, a value and a wrapped description
    // without turning into a second content pane. Clamped so it can never be
    // wider than the space to the right of the button that opened it.
    readonly property real pad: 10
    readonly property real preferredWidth: 320
    width: parent ? Math.max(260, Math.min(preferredWidth, parent.width - x - 16))
                  : preferredWidth
    // Never taller than what is actually below the anchor point. If the
    // content is shorter than that, the panel shrinks to fit it.
    readonly property real availableHeight:
        parent ? Math.max(180, parent.height - y - 16) : 600
    height: Math.min(availableHeight, header.height + contentCol.implicitHeight + 2 * pad + 6)

    Rectangle {
        anchors.fill: parent
        color: "#1f2026"
        radius: 12
        border.color: "#3a3b42"
        border.width: 1

        // ── header ───────────────────────────────────────────────────────
        Item {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 38

            Text {
                anchors.left: parent.left
                anchors.leftMargin: name.pad + 2
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("SETTINGS")
                color: "#f2f3f5"
                font.pixelSize: 12; font.bold: true; font.letterSpacing: 1.2
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: name.pad; anchors.rightMargin: name.pad
                height: 1
                color: "#33343b"
            }
        }

        // ── scrolling content region ─────────────────────────────────────
        Flickable {
            id: settingsFlick
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: name.pad
            anchors.topMargin: 8

            clip: true
            // Explicit, per UI-DEC-007. Bottom padding so the last control is
            // never flush against the panel edge when scrolled to the end.
            contentHeight: contentCol.implicitHeight + 8
            contentWidth: width               // no horizontal scrolling, ever
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {
                id: settingsScrollBar
                policy: settingsFlick.contentHeight > settingsFlick.height
                        ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                width: 6
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: settingsScrollBar.pressed ? "#8a8b93" : "#55565e"
                    opacity: settingsScrollBar.policy === ScrollBar.AlwaysOn ? 1 : 0
                }
                background: Rectangle { color: "transparent" }
            }

            Column {
                id: contentCol
                width: settingsFlick.width - (settingsScrollBar.visible ? 10 : 0)
                spacing: 8

                // ── DISPLAY COLOURS ──────────────────────────────────────
                // Was a floating PNG block that sat outside the layout. The
                // swatches are drawn now, so they scale with the panel and
                // carry the same two flags they always did.
                Rectangle {
                    id: colourSection
                    width: contentCol.width
                    height: colourCol.implicitHeight + 16
                    radius: 6
                    color: "#26272c"
                    border.color: "#3a3b40"; border.width: 1

                    Column {
                        id: colourCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.margins: 8
                        spacing: 8

                        Text {
                            text: qsTr("TARGET DISPLAY")
                            color: "#9a9ba0"; font.pixelSize: 9
                            font.bold: true; font.letterSpacing: 1
                        }

                        // Background colour
                        Row {
                            spacing: 8
                            Text {
                                width: 108
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Background")
                                color: "white"; font.pixelSize: 11
                                wrapMode: Text.WordWrap
                            }
                            Repeater {
                                model: [ { c: "#101114", black: true },
                                         { c: "#2f78d0", black: false } ]
                                Rectangle {
                                    width: 34; height: 24; radius: 5
                                    color: modelData.c
                                    border.width: name.isBackGroundBlack === modelData.black ? 2 : 1
                                    border.color: name.isBackGroundBlack === modelData.black
                                                  ? "#e8003d" : "#5a5b62"
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: name.isBackGroundBlack = modelData.black
                                    }
                                }
                            }
                        }

                        // Pellet colour. The warm option is yellow at 10 m and
                        // red at 50 m, exactly as the old artwork switched it.
                        Row {
                            spacing: 8
                            Text {
                                width: 108
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Pellet")
                                color: "white"; font.pixelSize: 11
                                wrapMode: Text.WordWrap
                            }
                            Repeater {
                                model: [ { warm: true }, { warm: false } ]
                                Rectangle {
                                    width: 34; height: 24; radius: 5
                                    color: modelData.warm
                                           ? (gameRange === 10 ? "#f2c200" : "#e8003d")
                                           : "#2fa84f"
                                    border.width: name.isPalletRedColor === modelData.warm ? 2 : 1
                                    border.color: name.isPalletRedColor === modelData.warm
                                                  ? "#ffffff" : "#5a5b62"
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: name.isPalletRedColor = modelData.warm
                                    }
                                }
                            }
                        }
                    }
                }

                // ── MOTOR FEED ───────────────────────────────────────────
                Rectangle {
                    id: motorSection
                    width: contentCol.width
                    height: motorCol.implicitHeight + 16
                    radius: 6
                    color: "#26272c"
                    border.color: "#3a3b40"; border.width: 1

                    Column {
                        id: motorCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.margins: 8
                        spacing: 8

                        Text {
                            text: qsTr("MOTOR FEED (SECONDS)")
                            color: "#9a9ba0"; font.pixelSize: 9
                            font.bold: true; font.letterSpacing: 1
                        }
                        // Labels above equal-sized fields, both columns the
                        // same width so the two inputs line up. Written out
                        // rather than repeated: an unqualified `modelData`
                        // read inside a nested Component.onCompleted fails
                        // SILENTLY in QML, which left both fields blank.
                        Row {
                            spacing: 10
                            Column {
                                spacing: 3
                                Text {
                                    text: qsTr("Match")
                                    color: "#c9ced6"; font.pixelSize: 10
                                }
                                Rectangle {
                                    width: 72; height: 26; radius: 4
                                    color: "#15161a"
                                    border.color: "#3a3b40"; border.width: 1
                                    TextInput {
                                        id: motorMatchInput
                                        anchors.fill: parent
                                        anchors.leftMargin: 7; anchors.rightMargin: 7
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: "white"; font.pixelSize: 12
                                        selectByMouse: true
                                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                                        text: APPSETTINGS.getMotor_movement_time().toFixed(1)
                                    }
                                }
                            }
                            Column {
                                spacing: 3
                                Text {
                                    text: qsTr("Sighter")
                                    color: "#c9ced6"; font.pixelSize: 10
                                }
                                Rectangle {
                                    width: 72; height: 26; radius: 4
                                    color: "#15161a"
                                    border.color: "#3a3b40"; border.width: 1
                                    TextInput {
                                        id: motorSighterInput
                                        anchors.fill: parent
                                        anchors.leftMargin: 7; anchors.rightMargin: 7
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: "white"; font.pixelSize: 12
                                        selectByMouse: true
                                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                                        text: APPSETTINGS.getMotor_movement_time_sighter().toFixed(1)
                                    }
                                }
                            }
                        }
                        Rectangle {
                            width: 84; height: 28; radius: 5
                            color: motorSaveMouse.pressed ? "#c40046" : "#e8003d"
                            Text {
                                id: motorSaveText
                                anchors.centerIn: parent
                                text: qsTr("Save")
                                color: "white"; font.pixelSize: 11; font.bold: true
                            }
                            MouseArea {
                                id: motorSaveMouse
                                anchors.fill: parent
                                onClicked: motorSection.saveMotorTimes()
                            }
                        }
                    }

                    // Behaviour and clamping unchanged from the original.
                    function saveMotorTimes() {
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
                    Timer {
                        id: motorSavedReset
                        interval: 1500
                        onTriggered: motorSaveText.text = qsTr("Save")
                    }
                }

                // ── OPERATING MODE ───────────────────────────────────────
                // The persisted source of the mode is config.ini
                // [App_Settings]/app_mode; changing it here writes that key
                // atomically and requires a restart. Blocked while a session
                // is active.
                Rectangle {
                    id: operatingModeSection
                    width: contentCol.width
                    height: opModeCol.implicitHeight + 16
                    radius: 6
                    color: "#26272c"
                    border.color: "#3a3b40"; border.width: 1

                    // 0 = Live, 1 = Demo. Selection previews via OPMODE; the
                    // running mode is fixed for the process.
                    property int runningMode: name.opLive ? 0 : 1

                    Column {
                        id: opModeCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.margins: 8
                        spacing: 9

                        Text {
                            text: qsTr("OPERATING MODE")
                            color: "#9a9ba0"; font.pixelSize: 9
                            font.bold: true; font.letterSpacing: 1
                        }

                        // Both options share one delegate so the radio, the
                        // heading and the wrapped description cannot drift
                        // apart between them.
                        Repeater {
                            model: [
                                { mode: 0, dot: "#37c76a",
                                  title: qsTr("Live target"),
                                  desc: qsTr("Receives shots from the physical Tech Aim target.") },
                                { mode: 1, dot: "#e8003d",
                                  title: qsTr("Demo / simulation"),
                                  desc: qsTr("Allows simulated target shots for testing and demonstration.") }
                            ]
                            Item {
                                width: opModeCol.width
                                height: Math.max(optRadio.height, optText.implicitHeight)

                                Rectangle {
                                    id: optRadio
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.topMargin: 1
                                    width: 14; height: 14; radius: 7
                                    color: "transparent"; border.width: 2
                                    border.color: operatingModeSection.runningMode === modelData.mode
                                                  ? modelData.dot : "#5a5b62"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 7; height: 7; radius: 4
                                        color: modelData.dot
                                        visible: operatingModeSection.runningMode === modelData.mode
                                    }
                                }
                                Column {
                                    id: optText
                                    anchors.left: optRadio.right
                                    anchors.leftMargin: 9
                                    anchors.right: parent.right
                                    spacing: 1
                                    Text {
                                        width: parent.width
                                        text: modelData.title
                                        color: "white"; font.pixelSize: 11; font.bold: true
                                        wrapMode: Text.WordWrap
                                    }
                                    Text {
                                        width: parent.width
                                        text: modelData.desc
                                        color: "#9a9ba0"; font.pixelSize: 9
                                        wrapMode: Text.WordWrap
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !name.modeChangeBlocked
                                             && operatingModeSection.runningMode !== modelData.mode
                                    onClicked: name.beginModeChange(modelData.mode)
                                }
                            }
                        }

                        // Current running mode.
                        Text {
                            width: opModeCol.width
                            wrapMode: Text.WordWrap
                            text: qsTr("Current mode: ")
                                  + (name.opLive ? qsTr("LIVE TARGET") : qsTr("DEMO / SIMULATION"))
                            color: name.opLive ? "#8fe0a8" : "#ff9aa8"
                            font.pixelSize: 10; font.bold: true
                        }
                        // Blocked reason OR restart-required hint OR how to change.
                        Text {
                            width: opModeCol.width
                            wrapMode: Text.WordWrap
                            font.pixelSize: 9
                            color: name.modeChangeBlocked ? "#ffb46b"
                                   : ((typeof OPMODE !== "undefined" && OPMODE.restartRequired)
                                      ? "#ffd0d7" : "#8a8b90")
                            text: name.modeChangeBlocked
                                    ? qsTr("Operating mode cannot be changed while a session is active. Close or complete the current session first.")
                                  : ((typeof OPMODE !== "undefined" && OPMODE.restartRequired)
                                        ? qsTr("Restart required — the new mode takes effect on next launch.")
                                        : qsTr("Select the other option to switch (requires restart)."))
                        }
                    }
                }

                // ── LANGUAGE ─────────────────────────────────────────────
                // Full-width touch targets. Changing language affects
                // TRANSLATIONS ONLY: never the brand, theme, executable
                // identity or the operating mode.
                Rectangle {
                    id: languageSection
                    width: contentCol.width
                    height: langCol.implicitHeight + 16
                    radius: 6
                    color: "#26272c"
                    border.color: "#3a3b40"; border.width: 1
                    Column {
                        id: langCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.margins: 8
                        spacing: 6
                        Text {
                            text: qsTr("LANGUAGE")
                            color: "#9a9ba0"; font.pixelSize: 9
                            font.bold: true; font.letterSpacing: 1
                        }
                        Repeater {
                            model: LANGUAGE.availableLanguages
                            delegate: Rectangle {
                                property bool selected: modelData.code === LANGUAGE.languageCode
                                width: langCol.width; height: 40; radius: 6
                                color: selected ? "#a80038" : (langMouse.pressed ? "#34353c" : "#2c2d33")
                                border.color: selected ? "#c40046" : "#3a3b40"; border.width: 1
                                Text {
                                    anchors.left: parent.left; anchors.leftMargin: 12
                                    anchors.right: langTick.left; anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    color: parent.selected ? "white" : "#c9ced6"
                                    font.pixelSize: 12; font.bold: parent.selected
                                    elide: Text.ElideRight
                                }
                                Text {
                                    id: langTick
                                    anchors.right: parent.right; anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: parent.selected ? "✓" : ""
                                    color: "white"; font.pixelSize: 14; font.bold: true
                                }
                                MouseArea {
                                    id: langMouse
                                    anchors.fill: parent
                                    onClicked: LANGUAGE.selectLanguage(modelData.code)
                                }
                            }
                        }
                        // The German catalogue is an untested beta: say so
                        // rather than implying a certified translation.
                        Text {
                            visible: LANGUAGE.isBetaTranslation
                            width: langCol.width; wrapMode: Text.WordWrap
                            text: qsTr("German is a beta translation awaiting native review. "
                                       + "Untranslated text stays in English.")
                            color: "#9a9ba0"; font.pixelSize: 9
                        }
                        // Shown only if a live switch could not retranslate everything.
                        Text {
                            visible: LANGUAGE.restartRequired
                            width: langCol.width; wrapMode: Text.WordWrap
                            text: qsTr("Restart required to finish applying the language.")
                            color: "#e8a13c"; font.pixelSize: 9; font.bold: true
                        }
                    }
                }

                // ── ABOUT / BUILD ────────────────────────────────────────
                // F9B: embedded at compile time; lets the operator confirm the
                // release executable matches the committed source.
                Rectangle {
                    id: aboutSection
                    width: contentCol.width
                    height: aboutCol.implicitHeight + 16
                    radius: 6
                    color: "#26272c"
                    border.color: "#3a3b40"; border.width: 1
                    Column {
                        id: aboutCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top; anchors.margins: 8
                        spacing: 3
                        Text {
                            text: qsTr("ABOUT / BUILD")
                            color: "#9a9ba0"; font.pixelSize: 9
                            font.bold: true; font.letterSpacing: 1
                        }
                        // Prose form is "Tech Aim" (spaced); the unspaced
                        // "TechAim" is reserved for the executable and file
                        // prefixes.
                        Text {
                            text: PRODUCT.fullProductName
                            color: "white"; font.pixelSize: 12; font.bold: true
                            width: aboutCol.width; wrapMode: Text.WordWrap
                        }
                        Repeater {
                            model: [
                                PRODUCT.displayName + " " + PRODUCT.version + " · " + PRODUCT.releaseChannel,
                                PRODUCT.legalPublisher,
                                (typeof BUILDINFO !== "undefined" ? BUILDINFO.config : "?") + qsTr(" build"),
                                qsTr("Commit: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.commit : "?"),
                                qsTr("Built: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.built : "?"),
                                qsTr("Qt: ") + PRODUCT.qtVersion + "   ·   " + PRODUCT.architecture,
                                qsTr("Windows: ") + PRODUCT.windowsVersion,
                                qsTr("Operating mode: ")
                                    + (APPSETTINGS.getAppMode() ? qsTr("Live") : qsTr("Demo")),
                                qsTr("Analytics: ") + PRODUCT.analyticsVersion
                            ]
                            Text {
                                width: aboutCol.width
                                text: modelData
                                color: "#c9ced6"; font.pixelSize: 10
                                wrapMode: Text.WordWrap
                            }
                        }
                        // Restrained, and only in a build that IS a field test.
                        Text {
                            visible: PRODUCT.isFieldTest
                            text: PRODUCT.fieldTestNotice
                            color: "#E8B4B8"; font.pixelSize: 10; font.bold: true
                            width: aboutCol.width; wrapMode: Text.WordWrap
                        }
                    }
                }
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

    function startFromServer()
    {

    }
}
