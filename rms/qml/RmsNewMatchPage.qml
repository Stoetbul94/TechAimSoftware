import QtQuick 2.15

// NEW MATCH — preparing a competition, in four steps.
//
// PREPARING A MATCH DOES NOT START OR CONTROL A TARGET STATION. Every step
// here writes RMS's own plan file. Nothing is transmitted, so nothing on any
// station changes; the banner says so on every step, because an operator who
// assumed otherwise would be waiting for a range that was never told anything.
Item {
    id: page

    Theme { id: theme }

    // The catalogue is the ONE authority for what programmes exist. RMS reads
    // it; it never keeps a second list of its own.
    CompetitionCatalogue { id: catalogue }

    property int step: 0                       // 0 programme, 1 lanes, 2 athletes, 3 review

    // A development hook so a capture can be taken of a named step without a
    // human clicking. Never reachable from the UI.
    Component.onCompleted: {
        if (typeof RMS_INITIAL_STEP !== "undefined" && RMS_INITIAL_STEP > 0)
            step = RMS_INITIAL_STEP
    }
    readonly property var stepNames: ["PROGRAMME", "SELECT LANES", "ASSIGN ATHLETES", "REVIEW"]
    property bool fifteenShotModels: false
    property string rejection: ""
    property int assigningLane: -1
    property string newAthleteName: ""

    Connections {
        target: PLANS
        function onRejected(reason) { page.rejection = reason }
        function onPlanChanged() { page.rejection = "" }
    }
    Connections {
        target: ATHLETES
        function onRejected(reason) { page.rejection = reason }
    }

    function programmeEntries() {
        var keys = page.fifteenShotModels
            ? ["game10RangeEventModel_15", "game50RangeEventModel_15"]
            : ["game10RangeEventModel", "game50RangeEventModel"]
        var out = []
        for (var k = 0; k < keys.length; ++k) {
            var e = catalogue.entriesFor(keys[k])
            for (var i = 0; i < e.length; ++i)
                out.push(e[i])
        }
        return out
    }

    function chooseProgramme(entry) {
        // IDs drive the decision. The label travels only as a snapshot for
        // historical display (QML-LANG-001).
        PLANS.setProgramme(entry.programmeId, entry.rulesetId, entry.targetStandardId,
                           entry.disciplineId, entry.distanceM, entry.shotCount,
                           entry.programmeType,
                           qsTr(entry.gameDisplay1Key) + " " + qsTr(entry.gameDisplay2Key)
                           + " · " + qsTr(entry.matchDisplayKey))
    }

    // ── header ──────────────────────────────────────────────────────────
    Column {
        id: head
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 2.5
        spacing: theme.spacingUnit

        Row {
            spacing: theme.spacingUnit * 2
            Text {
                text: PLANS.hasPlan ? PLANS.planName : "NEW MATCH"
                font.family: theme.fontFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            RmsStatusPill {
                anchors.verticalCenter: parent.verticalCenter
                visible: PLANS.hasPlan
                text: PLANS.planStatus
                tone: PLANS.planStatus === "READY" ? "live" : "neutral"
            }
        }

        // Stated on every step, not once at the start.
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Preparing this match does not start or control target stations. "
                  + "RMS saves the plan; it sends nothing."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.textSecondary
        }

        Row {
            spacing: 0
            Repeater {
                model: page.stepNames
                delegate: Row {
                    spacing: 0
                    Rectangle {
                        width: 168; height: 30
                        color: page.step === index ? theme.bgSurfaceAlt : "transparent"
                        border.width: 1
                        border.color: page.step === index ? theme.brandPrimary
                                                          : theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            text: (index + 1) + ". " + modelData
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            font.letterSpacing: 1.0
                            color: page.step === index ? theme.textPrimary
                                                       : theme.textSecondary
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: PLANS.hasPlan
                            cursorShape: Qt.PointingHandCursor
                            onClicked: page.step = index
                        }
                    }
                    Item { width: 6; height: 1 }
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: page.rejection.length > 0
            text: page.rejection
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.brandAccent
        }
    }

    // ── no plan yet ─────────────────────────────────────────────────────
    Column {
        anchors.centerIn: parent
        visible: !PLANS.hasPlan
        spacing: theme.spacingUnit * 2
        width: 460

        Text {
            text: "Name this match to begin."
            font.family: theme.fontFamily
            font.pixelSize: 15
            color: theme.textPrimary
        }
        Rectangle {
            width: parent.width; height: 38
            color: theme.bgBase
            border.width: 1
            border.color: nameInput.activeFocus ? theme.brandPrimary : theme.borderColor
            radius: theme.radiusSmall
            TextInput {
                id: nameInput
                anchors.fill: parent
                anchors.leftMargin: theme.spacingUnit
                verticalAlignment: TextInput.AlignVCenter
                font.family: theme.fontFamily
                font.pixelSize: 15
                color: theme.textPrimary
                selectByMouse: true
                onAccepted: if (text.length > 0) { PLANS.createPlan(text); page.step = 0 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: nameInput.text.length === 0
                    text: "e.g. Morning Relay"
                    font.family: theme.fontFamily
                    font.pixelSize: 15
                    color: theme.statusDisconnected
                }
            }
        }
        RmsButton {
            label: "CREATE MATCH PLAN"
            primary: true
            enabled: nameInput.text.trim().length > 0
            onActivated: { PLANS.createPlan(nameInput.text); page.step = 0 }
        }
        Text {
            visible: PLANS.planCount > 0
            text: PLANS.planCount + " saved plan(s) — open one from Home."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.textSecondary
        }
    }

    // ── step body ───────────────────────────────────────────────────────
    Item {
        id: body
        anchors { top: head.bottom; left: parent.left; right: parent.right
                  bottom: footer.top }
        anchors.margins: theme.spacingUnit * 2.5
        visible: PLANS.hasPlan

        // ── 0: programme ────────────────────────────────────────────────
        Item {
            anchors.fill: parent
            visible: page.step === 0

            Row {
                id: progFilter
                spacing: theme.spacingUnit * 2
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "CHOOSE A PROGRAMME"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 190; height: 26; radius: theme.radiusSmall
                    color: page.fifteenShotModels ? Qt.rgba(0.66, 0, 0.22, 0.18)
                                                  : theme.bgBase
                    border.width: 1
                    border.color: page.fifteenShotModels ? theme.brandPrimary
                                                         : theme.borderColor
                    Text {
                        anchors.centerIn: parent
                        text: "15-SHOT SIGHTER VARIANT"
                        font.family: theme.fontFamily
                        font.pixelSize: 9
                        font.letterSpacing: 1.0
                        color: theme.textPrimary
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.fifteenShotModels = !page.fifteenShotModels
                    }
                }
            }

            GridView {
                anchors { top: progFilter.bottom; topMargin: theme.spacingUnit
                          left: parent.left; right: parent.right; bottom: parent.bottom }
                clip: true
                cellWidth: 300
                cellHeight: 66
                model: page.programmeEntries()

                delegate: Item {
                    width: 292; height: 60
                    Rectangle {
                        anchors.fill: parent
                        radius: theme.radiusMedium
                        readonly property bool chosen:
                            PLANS.programmeId === modelData.programmeId
                        color: chosen ? theme.bgSurfaceAlt : theme.bgSurface
                        border.width: 1
                        border.color: chosen ? theme.brandPrimary : theme.borderColor

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: theme.spacingUnit * 1.5
                            anchors.right: parent.right
                            anchors.rightMargin: theme.spacingUnit
                            spacing: 3
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: qsTr(modelData.gameDisplay1Key) + " "
                                      + qsTr(modelData.gameDisplay2Key)
                                      + "  ·  " + qsTr(modelData.matchDisplayKey)
                                font.family: theme.fontFamily
                                font.pixelSize: 13
                                color: theme.textPrimary
                            }
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.programmeType === "OFFICIAL"
                                      ? "ISSF OFFICIAL COURSE" : "TECH AIM PRESET"
                                font.family: theme.fontFamily
                                font.pixelSize: 9
                                font.letterSpacing: 0.8
                                color: theme.textSecondary
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { page.chooseProgramme(modelData); page.step = 1 }
                        }
                    }
                }
            }
        }

        // ── 1: lanes ────────────────────────────────────────────────────
        Item {
            anchors.fill: parent
            visible: page.step === 1

            Row {
                id: laneHead
                spacing: theme.spacingUnit * 2
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "SELECT LANES  ·  " + PLANS.selectedLaneCount + " of "
                          + RANGECONFIG.laneCount
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                RmsButton {
                    label: "SELECT ALL ONLINE"
                    onActivated: PLANS.selectAllOnlineLanes()
                }
                RmsButton {
                    label: "CLEAR"
                    onActivated: PLANS.clearLaneSelection()
                }
            }

            ListView {
                anchors { top: laneHead.bottom; topMargin: theme.spacingUnit
                          left: parent.left; right: parent.right; bottom: parent.bottom }
                clip: true
                spacing: 5
                model: PLANLANES

                delegate: Rectangle {
                    width: Math.min(ListView.view.width, 720)
                    height: 44
                    radius: theme.radiusSmall
                    color: model.selected ? theme.bgSurfaceAlt : theme.bgSurface
                    border.width: 1
                    border.color: model.selected ? theme.brandPrimary : theme.borderColor

                    Rectangle {
                        id: box
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: theme.spacingUnit * 1.5
                        width: 16; height: 16; radius: 3
                        color: model.selected ? theme.brandPrimary : "transparent"
                        border.width: 1
                        border.color: model.selected ? theme.brandPrimary : theme.borderColor
                        Text {
                            anchors.centerIn: parent
                            visible: model.selected
                            text: "✓"
                            font.pixelSize: 11
                            color: theme.textOnBrand
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: box.right
                        anchors.leftMargin: theme.spacingUnit * 1.5
                        width: 110
                        text: model.laneLabel
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        color: theme.textPrimary
                    }
                    RmsStatusPill {
                        id: pill
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 190
                        width: 150
                        text: model.connection
                        tone: !model.hasDevice ? "neutral"
                            : model.online ? "live" : "offline"
                    }
                    // Offline lanes are selectable on purpose — a tablet may be
                    // switched on minutes before the start — but never quietly.
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: pill.right
                        anchors.leftMargin: theme.spacingUnit * 2
                        visible: model.selected && !model.online
                        text: "⚠ selected but not answering yet"
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        color: theme.brandAccent
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: PLANS.selectLane(model.laneNumber, !model.selected)
                    }
                }
            }
        }

        // ── 2: athletes ─────────────────────────────────────────────────
        Item {
            anchors.fill: parent
            visible: page.step === 2

            Text {
                id: assignHead
                text: "ASSIGN ATHLETES  ·  " + PLANS.assignedAthleteCount + " of "
                      + PLANS.selectedLaneCount + " lanes"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }

            ListView {
                id: assignList
                anchors { top: assignHead.bottom; topMargin: theme.spacingUnit
                          left: parent.left; bottom: parent.bottom }
                width: parent.width - picker.width - theme.spacingUnit * 3
                clip: true
                spacing: 5
                model: PLANLANES

                delegate: Rectangle {
                    width: ListView.view.width
                    height: model.selected ? 46 : 0
                    visible: model.selected
                    radius: theme.radiusSmall
                    color: page.assigningLane === model.laneNumber
                           ? theme.bgSurfaceAlt : theme.bgSurface
                    border.width: 1
                    border.color: page.assigningLane === model.laneNumber
                                  ? theme.brandPrimary : theme.borderColor

                    Text {
                        id: ln
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: theme.spacingUnit * 1.5
                        width: 90
                        text: model.laneLabel
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: theme.textPrimary
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: ln.right
                        width: 220
                        elide: Text.ElideRight
                        text: model.athleteName.length > 0 ? model.athleteName
                                                           : "Select athlete ▾"
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        color: model.athleteName.length > 0 ? theme.textPrimary
                                                            : theme.statusDisconnected
                    }
                    // The station's health, not its identity. nodeId belongs in
                    // diagnostics, not in front of a range officer.
                    RmsStatusPill {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: clearBtn.left
                        anchors.rightMargin: theme.spacingUnit
                        width: 150
                        text: model.readiness
                        tone: model.ready ? "live"
                            : model.readiness === "NO ATHLETE" ? "neutral" : "warn"
                    }
                    RmsButton {
                        id: clearBtn
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: theme.spacingUnit
                        label: "CLEAR"
                        visible: model.athleteName.length > 0
                        onActivated: PLANS.clearAthlete(model.laneNumber)
                    }
                    MouseArea {
                        anchors.fill: parent
                        anchors.rightMargin: 260
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.assigningLane =
                                   (page.assigningLane === model.laneNumber)
                                   ? -1 : model.laneNumber
                    }
                }
            }

            // ── the start list, with quick create ───────────────────────
            Rectangle {
                id: picker
                anchors { top: assignHead.bottom; topMargin: theme.spacingUnit
                          right: parent.right; bottom: parent.bottom }
                width: 360
                radius: theme.radiusMedium
                color: theme.bgSurface
                border.width: 1
                border.color: theme.borderColor
                clip: true

                Column {
                    anchors.fill: parent
                    anchors.margins: theme.spacingUnit * 1.5
                    spacing: theme.spacingUnit

                    Text {
                        text: page.assigningLane > 0
                              ? "CHOOSE FOR LANE " + page.assigningLane
                              : "START LIST  ·  " + ATHLETEMODEL.count
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 1.2
                        color: page.assigningLane > 0 ? theme.brandPrimary
                                                      : theme.textSecondary
                    }

                    // Quick field-test entry: a name and one press.
                    Row {
                        width: parent.width
                        spacing: 6
                        Rectangle {
                            width: parent.width - 76; height: 32
                            color: theme.bgBase
                            border.width: 1
                            border.color: quick.activeFocus ? theme.brandPrimary
                                                            : theme.borderColor
                            radius: theme.radiusSmall
                            TextInput {
                                id: quick
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                font.family: theme.fontFamily
                                font.pixelSize: 13
                                color: theme.textPrimary
                                selectByMouse: true
                                onTextChanged: page.newAthleteName = text
                                onAccepted: addQuick.activate()
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: quick.text.length === 0
                                    text: "Athlete name"
                                    font.family: theme.fontFamily
                                    font.pixelSize: 13
                                    color: theme.statusDisconnected
                                }
                            }
                        }
                        RmsButton {
                            id: addQuick
                            label: "ADD"
                            primary: true
                            enabled: quick.text.trim().length > 0
                            onActivated: {
                                var id = ATHLETES.addAthlete(quick.text, "", "", true)
                                // Usable immediately: if a lane is waiting, the
                                // new athlete goes straight onto it.
                                if (id.length > 0 && page.assigningLane > 0) {
                                    PLANS.assignAthlete(id, page.assigningLane)
                                    page.assigningLane = -1
                                }
                                quick.text = ""
                            }
                        }
                    }

                    ListView {
                        width: parent.width
                        height: parent.height - 90
                        clip: true
                        spacing: 4
                        model: ATHLETEMODEL

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 38
                            radius: theme.radiusSmall
                            color: theme.bgBase
                            border.width: 1
                            border.color: theme.borderColor
                            opacity: model.assigned && page.assigningLane > 0
                                     && model.assignedLane !== page.assigningLane ? 0.45 : 1.0

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                width: parent.width - 110
                                elide: Text.ElideRight
                                text: model.displayName
                                font.family: theme.fontFamily
                                font.pixelSize: 13
                                color: theme.textPrimary
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                text: model.assigned ? "Lane " + model.assignedLane : ""
                                font.family: theme.fontFamily
                                font.pixelSize: 10
                                color: theme.textSecondary
                            }
                            MouseArea {
                                anchors.fill: parent
                                enabled: page.assigningLane > 0
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    PLANS.assignAthlete(model.athleteId, page.assigningLane)
                                    page.assigningLane = -1
                                }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: ATHLETEMODEL.count === 0
                            text: "No athletes yet.\nType a name above."
                            horizontalAlignment: Text.AlignHCenter
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                            color: theme.textSecondary
                        }
                    }
                }
            }
        }

        // ── 3: review ───────────────────────────────────────────────────
        RmsMatchReview {
            anchors.fill: parent
            visible: page.step === 3
        }
    }

    // ── footer ──────────────────────────────────────────────────────────
    Row {
        id: footer
        anchors { left: parent.left; bottom: parent.bottom }
        anchors.margins: theme.spacingUnit * 2.5
        visible: PLANS.hasPlan
        spacing: theme.spacingUnit

        RmsButton {
            label: "◀ BACK"
            enabled: page.step > 0
            onActivated: page.step = page.step - 1
        }
        RmsButton {
            label: "NEXT ▶"
            primary: page.step < 3
            enabled: page.step < 3 && (page.step > 0 || PLANS.programmeSelected)
            onActivated: page.step = page.step + 1
        }
        RmsButton {
            label: "SAVE PLAN AS READY"
            primary: page.step === 3
            visible: page.step === 3
            onActivated: PLANS.markReady()
        }
    }
}
