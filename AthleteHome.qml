import QtQuick 2.15

// UI-DEC-016 — SETA athlete home. Spec: docs/seta/SETA-ATHLETE-MODE-SPEC.md
//
// PRESENTATION ONLY. This screen decides nothing about a match: it hands a
// programmeId and a name to LoginPage.athleteStart(), which commits the tile
// through applySetaProgramme() and then runs the operator page's own start
// handler. No start gate, connection check or rule data lives here.
Item {
    id: home

    // ── Inputs ────────────────────────────────────────────────────────────
    property var tok: theme.homeTokens
    property var catalogue: null
    property var programmeIds: []          // a single ungrouped list (evidence scenes)
    // UI-DEC-018: [{key, label, ids}] already filtered by LoginPage. When set,
    // it replaces programmeIds and a group switcher is shown above the tiles.
    property var groups: []
    // ISSF / practice tile times, from LoginPage.legacyMatchTimeText().
    property var tileTimes: ({})
    property int groupIndex: 0
    readonly property int groupIndexClamped:
        groups.length > 0 ? Math.max(0, Math.min(groupIndex, groups.length - 1)) : 0
    readonly property var shownIds:
        groups.length > 0 ? groups[groupIndexClamped].ids : programmeIds
    property bool liveMode: true            // config.ini app_mode=Live
    property bool targetReady: false        // MODREADER.targetReady
    property string operatorPin: ""         // empty = confirm only
    property string athleteName: ""

    // ── Outputs ───────────────────────────────────────────────────────────
    signal startRequested(string programmeId, string name)
    signal operatorUnlocked()

    property string selectedId: ""
    // Overlay state, exposed so evidence scenes can show each overlay open.
    property alias displayOpen: displayOverlay.visible
    property alias pinOpen: pinOverlay.visible
    readonly property bool canStart: nameField.text.trim() !== "" && selectedId !== ""

    // ISSF / practice tiles are named like the DSB ones ("10m Air Rifle ·
    // Practice 20"), not by the operator page's upper-case display codes, and
    // every word is a literal qsTr() so lupdate can extract it. Keyed on the
    // machine disciplineId, never on display text (QML-LANG-001).
    function disciplineName(p) {
        switch (p.disciplineId) {
        case "AR10":         return qsTr("10m Air Rifle")
        case "AP10":         return qsTr("10m Air Pistol")
        case "RIFLE50":      return qsTr("50m Rifle")
        case "FREEPISTOL50": return qsTr("50m Pistol")
        }
        return qsTr(p.gameDisplay1Key) + " " + qsTr(p.gameDisplay2Key)
    }
    function programmeTitle(p) {
        if (p.rulesetId !== "dsb")
            return disciplineName(p) + "  ·  "
                 + (p.rulesetId === "issf" ? qsTr("Qualification %1").arg(p.shotCount)
                    : p.unlimited ? qsTr("Free practice")
                    : qsTr("Practice %1").arg(p.shotCount))
        return p.ruleNumber + "  " + qsTr(p.disciplineLabelKey)
               + "  " + p.programmeVariant.replace("x", "×")
    }
    function programmeDetail(p) {
        if (p.rulesetId !== "dsb") {
            var t = home.tileTimes[p.programmeId]
            return (p.unlimited ? qsTr("Unlimited shots") : qsTr("%1 shots").arg(p.shotCount))
                   + ((t && t !== "—") ? "  ·  " + t : "")
        }
        var time = p.matchMinutes > 0
                ? qsTr("%1 min").arg(p.matchMinutes)
                : qsTr("%1 min per position").arg(p.positionMinutes.join(" / "))
        return qsTr("%1 shots").arg(p.shotCount) + "  ·  " + time
    }

    Rectangle { anchors.fill: parent; color: tok.backgroundPrimary }
    // Block clicks from reaching the operator homepage underneath.
    MouseArea { anchors.fill: parent }

    // ── Top: title + status ──────────────────────────────────────────────
    Item {
        id: topRow
        anchors.top: parent.top; anchors.topMargin: 24
        anchors.left: parent.left; anchors.leftMargin: 32
        anchors.right: parent.right; anchors.rightMargin: 32
        height: 52

        Text {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            anchors.right: statusPill.left; anchors.rightMargin: 16
            text: qsTr("Ready to shoot"); elide: Text.ElideRight
            color: tok.textPrimary; font.family: theme.fontFamily
            font.pixelSize: 32; font.bold: true
        }

        // One status, plain words. Demo stays unmissable: mistaking a Demo
        // session for a Live one is a result-integrity risk.
        Rectangle {
            id: statusPill
            objectName: "athleteStatusPill"
            readonly property int state_: !home.liveMode ? 2 : (home.targetReady ? 0 : 1)
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            height: 48; radius: 24
            width: statusRow.implicitWidth + 36
            color: state_ === 0 ? tok.successBackground
                 : state_ === 1 ? tok.errorBackground : tok.warningBackground
            border.width: 1
            border.color: state_ === 0 ? tok.successText
                        : state_ === 1 ? tok.errorText : tok.warningText
            Row {
                id: statusRow
                anchors.centerIn: parent; spacing: 10
                Rectangle {
                    width: 14; height: 14; radius: 7
                    anchors.verticalCenter: parent.verticalCenter
                    color: statusPill.border.color
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: statusPill.state_ === 0 ? qsTr("Target ready")
                        : statusPill.state_ === 1 ? qsTr("Target not connected — check the cable")
                                                  : qsTr("DEMO — no live target")
                    color: statusPill.border.color; font.family: theme.fontFamily
                    font.pixelSize: 17; font.bold: true
                }
            }
        }
    }

    // ── Name ─────────────────────────────────────────────────────────────
    Text {
        id: nameLabel
        anchors.top: topRow.bottom; anchors.topMargin: 28
        anchors.left: topRow.left
        text: qsTr("YOUR NAME"); color: tok.textSecondary
        font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1
    }
    Rectangle {
        id: nameBox
        anchors.top: nameLabel.bottom; anchors.topMargin: 8
        anchors.left: topRow.left
        width: Math.min(560, topRow.width); height: 60; radius: 10
        color: tok.inputBackground
        border.color: nameField.activeFocus ? tok.accentPrimary : tok.borderStrong
        border.width: nameField.activeFocus ? 2 : 1
        TextInput {
            id: nameField
            objectName: "athleteNameField"
            anchors.fill: parent; anchors.leftMargin: 18; anchors.rightMargin: 18
            verticalAlignment: TextInput.AlignVCenter
            text: home.athleteName; maximumLength: 20
            color: tok.textPrimary; selectionColor: tok.accentPrimary
            font.family: theme.fontFamily; font.pixelSize: 22
            clip: true
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: parent.text === "" && !parent.activeFocus
                text: qsTr("Type your name"); color: tok.textDisabled
                font.family: theme.fontFamily; font.pixelSize: 22
            }
        }
    }

    // ── Events ───────────────────────────────────────────────────────────
    Text {
        id: eventLabel
        anchors.top: nameBox.bottom; anchors.topMargin: 28
        anchors.left: topRow.left
        text: qsTr("CHOOSE YOUR EVENT"); color: tok.textSecondary
        font.family: theme.fontFamily; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1
    }
    Text {
        anchors.top: eventLabel.bottom; anchors.topMargin: 12
        anchors.left: topRow.left; anchors.right: topRow.right
        visible: home.shownIds.length === 0
        wrapMode: Text.WordWrap
        text: qsTr("No events are set up for this lane. Ask the range operator.")
        color: tok.warningText; font.family: theme.fontFamily; font.pixelSize: 18
    }
    // Scrolls on its own; the action bar below never scrolls (Screen Layout
    // Rules §1: the primary action is always reachable).
    // UI-DEC-018: DSB / ISSF / Practice. Shown only when there is a choice.
    // Switching clears the selection so START never runs an unseen tile.
    Row {
        id: groupRow
        objectName: "athleteGroupRow"
        anchors.top: eventLabel.bottom; anchors.topMargin: 12
        anchors.left: topRow.left
        spacing: 8
        visible: home.groups.length > 1
        height: visible ? 48 : 0
        Repeater {
            model: home.groups
            Rectangle {
                objectName: "athleteGroup_" + modelData.key
                readonly property bool current: index === home.groupIndexClamped
                width: Math.max(120, grpText.implicitWidth + 40); height: 48; radius: 24
                color: current ? tok.accentPrimary : tok.surfacePrimary
                border.color: current ? tok.accentPrimary : tok.borderStrong; border.width: 1
                Text {
                    id: grpText; anchors.centerIn: parent; text: modelData.label
                    color: parent.current ? tok.textOnAccent : tok.textPrimary
                    font.family: theme.fontFamily; font.pixelSize: 17; font.bold: true
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: if (home.groupIndex !== index) {
                                   home.groupIndex = index; home.selectedId = "" }
                }
            }
        }
    }

    GridView {
        id: tiles
        objectName: "athleteProgrammeGrid"
        anchors.top: groupRow.bottom; anchors.topMargin: groupRow.visible ? 16 : 0
        anchors.left: topRow.left; anchors.right: topRow.right
        anchors.bottom: actionBar.top; anchors.bottomMargin: 16
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        readonly property int cols: width >= 1000 ? 3 : 2
        cellWidth: width / cols; cellHeight: 104
        model: home.shownIds
        delegate: Item {
            width: tiles.cellWidth; height: tiles.cellHeight
            readonly property var p: home.catalogue ? home.catalogue.profile(modelData) : null
            Rectangle {
                objectName: "athleteTile_" + modelData
                anchors.fill: parent; anchors.rightMargin: 12; anchors.bottomMargin: 12
                readonly property bool selected: home.selectedId === modelData
                radius: 10
                color: selected ? tok.accentSubtle : tok.surfacePrimary
                border.color: selected ? tok.accentPrimary : tok.borderSubtle
                border.width: selected ? 3 : 1
                Column {
                    anchors.left: parent.left; anchors.leftMargin: 18
                    anchors.right: tick.left; anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6
                    Text {
                        width: parent.width; elide: Text.ElideRight
                        text: p ? home.programmeTitle(p) : modelData
                        color: tok.textPrimary; font.family: theme.fontFamily
                        font.pixelSize: 20; font.bold: true
                    }
                    Text {
                        width: parent.width; elide: Text.ElideRight
                        text: p ? home.programmeDetail(p) : ""
                        color: tok.textSecondary; font.family: theme.fontFamily; font.pixelSize: 15
                    }
                }
                // Selection indicator, same position on every tile (UI-DEC-009).
                Rectangle {
                    id: tick
                    anchors.right: parent.right; anchors.rightMargin: 18
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28; height: 28; radius: 14
                    color: parent.selected ? tok.accentPrimary : "transparent"
                    border.color: parent.selected ? tok.accentPrimary : tok.borderStrong; border.width: 2
                    Rectangle { anchors.centerIn: parent; visible: parent.parent.selected
                                width: 10; height: 10; radius: 5; color: tok.textOnAccent }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: home.selectedId = modelData }
            }
        }
    }

    // ── Action bar: fixed, never scrolls ─────────────────────────────────
    Rectangle {
        id: actionBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 104
        color: tok.backgroundSecondary
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: tok.borderSubtle }

        Row {
            anchors.left: parent.left; anchors.leftMargin: 32
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            Repeater {
                model: [ { key: "display", label: qsTr("Display") },
                         { key: "operator", label: qsTr("Operator") } ]
                Rectangle {
                    objectName: modelData.key === "display" ? "athleteDisplayButton" : "athleteOperatorButton"
                    width: Math.max(120, btnText.implicitWidth + 40); height: 56; radius: 8
                    color: btnMouse.pressed ? tok.surfaceElevated : tok.surfacePrimary
                    border.color: tok.borderStrong; border.width: 1
                    Text { id: btnText; anchors.centerIn: parent; text: modelData.label
                           color: tok.textSecondary; font.family: theme.fontFamily; font.pixelSize: 16 }
                    MouseArea {
                        id: btnMouse; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.key === "display") displayOverlay.visible = true
                            else { pinOverlay.entered = ""; pinOverlay.wrong = false; pinOverlay.visible = true }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: startBtn
            objectName: "athleteStartButton"
            anchors.right: parent.right; anchors.rightMargin: 32
            anchors.verticalCenter: parent.verticalCenter
            width: 300; height: 68; radius: 10
            color: !home.canStart ? tok.surfaceSecondary
                 : startMouse.pressed ? tok.accentPressed : tok.accentPrimary
            border.color: home.canStart ? tok.accentPrimary : tok.borderSubtle; border.width: 1
            Text {
                anchors.centerIn: parent
                text: qsTr("START")
                color: home.canStart ? tok.textOnAccent : tok.textDisabled
                font.family: theme.fontFamily; font.pixelSize: 26; font.bold: true; font.letterSpacing: 2
            }
            MouseArea {
                id: startMouse; anchors.fill: parent
                cursorShape: home.canStart ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: if (home.canStart) home.startRequested(home.selectedId, nameField.text.trim())
            }
        }
        // Say WHY start is not available instead of a silent dead button.
        Text {
            anchors.right: startBtn.left; anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            visible: !home.canStart
            text: nameField.text.trim() === "" ? qsTr("Type your name first")
                                               : qsTr("Choose an event")
            color: tok.textSecondary; font.family: theme.fontFamily; font.pixelSize: 16
        }
    }

    // ── Display settings overlay ─────────────────────────────────────────
    Rectangle {
        id: displayOverlay
        anchors.fill: parent; visible: false
        color: tok.scrim
        MouseArea { anchors.fill: parent; onClicked: displayOverlay.visible = false }
        HomeDisplaySettings {
            objectName: "athleteDisplayPanel"
            anchors.centerIn: parent
            tok: home.tok
            onCloseRequested: displayOverlay.visible = false
        }
    }

    // ── Operator unlock: PIN pad, or a confirm when no PIN is set ────────
    Rectangle {
        id: pinOverlay
        objectName: "athletePinOverlay"
        property string entered: ""
        property bool wrong: false
        readonly property bool needsPin: home.operatorPin !== ""
        anchors.fill: parent; visible: false
        color: tok.scrim
        MouseArea { anchors.fill: parent; onClicked: pinOverlay.visible = false }

        // UI-DEC-018: the keyboard works too - digits (main row and numpad),
        // Backspace, Enter to confirm, Esc to cancel. Same rules as the pad.
        onVisibleChanged: if (visible) forceActiveFocus()
        Keys.onPressed: function (event) {
            if (event.key === Qt.Key_Escape) {
                visible = false
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                submit()
            } else if (!needsPin) {
                return
            } else if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
                wrong = false
                if (entered.length < 8) entered += String(event.key - Qt.Key_0)
            } else if (event.key === Qt.Key_Backspace) {
                wrong = false
                entered = entered.slice(0, -1)
            } else {
                return
            }
            event.accepted = true
        }

        function submit() {
            if (!needsPin || entered === home.operatorPin) {
                visible = false; entered = ""; wrong = false
                home.operatorUnlocked()
            } else {
                wrong = true; entered = ""
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: 340; height: pinCol.implicitHeight + 40; radius: 12
            color: tok.surfacePrimary; border.color: tok.borderStrong; border.width: 1
            MouseArea { anchors.fill: parent }
            Column {
                id: pinCol
                x: 20; y: 20; width: parent.width - 40; spacing: 14
                Text {
                    text: qsTr("Operator"); color: tok.textPrimary
                    font.family: theme.fontFamily; font.pixelSize: 20; font.bold: true
                }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap
                    text: pinOverlay.wrong ? qsTr("Wrong PIN. Try again.")
                        : pinOverlay.needsPin ? qsTr("Enter the operator PIN.")
                        : qsTr("Open the full operator screen?")
                    color: pinOverlay.wrong ? tok.errorText : tok.textSecondary
                    font.family: theme.fontFamily; font.pixelSize: 15
                }
                Rectangle {
                    visible: pinOverlay.needsPin
                    width: parent.width; height: 52; radius: 8
                    color: tok.inputBackground; border.color: tok.borderStrong; border.width: 1
                    Text { anchors.centerIn: parent
                           text: pinOverlay.entered.replace(/./g, "•")
                           color: tok.textPrimary; font.pixelSize: 22; font.letterSpacing: 6 }
                }
                Grid {
                    visible: pinOverlay.needsPin
                    columns: 3; spacing: 8
                    Repeater {
                        model: ["1","2","3","4","5","6","7","8","9","DEL","0","OK"]
                        Rectangle {
                            width: (pinCol.width - 16) / 3; height: 56; radius: 8
                            color: keyMouse.pressed ? tok.surfaceElevated
                                 : modelData === "OK" ? tok.accentPrimary : tok.surfaceSecondary
                            border.color: tok.borderSubtle; border.width: 1
                            Text { anchors.centerIn: parent; text: modelData
                                   color: modelData === "OK" ? tok.textOnAccent : tok.textPrimary
                                   font.family: theme.fontFamily; font.pixelSize: 20; font.bold: true }
                            MouseArea {
                                id: keyMouse; anchors.fill: parent
                                onClicked: {
                                    pinOverlay.wrong = false
                                    if (modelData === "OK") pinOverlay.submit()
                                    else if (modelData === "DEL") pinOverlay.entered = pinOverlay.entered.slice(0, -1)
                                    else if (pinOverlay.entered.length < 8) pinOverlay.entered += modelData
                                }
                            }
                        }
                    }
                }
                Row {
                    spacing: 8
                    Rectangle {
                        width: pinOverlay.needsPin ? pinCol.width : (pinCol.width - 8) / 2
                        height: 52; radius: 8
                        color: tok.surfaceSecondary; border.color: tok.borderStrong; border.width: 1
                        Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: tok.textPrimary
                               font.family: theme.fontFamily; font.pixelSize: 16 }
                        MouseArea { anchors.fill: parent; onClicked: pinOverlay.visible = false }
                    }
                    Rectangle {
                        visible: !pinOverlay.needsPin
                        width: (pinCol.width - 8) / 2; height: 52; radius: 8
                        color: tok.accentPrimary
                        Text { anchors.centerIn: parent; text: qsTr("Open"); color: tok.textOnAccent
                               font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: pinOverlay.submit() }
                    }
                }
            }
        }
    }
}
