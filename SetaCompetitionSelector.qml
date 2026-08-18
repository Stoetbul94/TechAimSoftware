import QtQuick 2.15
import QtQuick.Controls 2.15

// ─────────────────────────────────────────────────────────────────────────────
// SETA hierarchical competition selector — RULE SET → DISCIPLINE → PROGRAMME.
//
// SETA-ONLY. This lives on product/seta and IS the SETA production selection
// path: LoginPage shows it in place of the weapon/distance/event controls,
// which are kept intact behind setaSelection as the rollback reference until
// this has been through an integrated approval.
//
// Why it exists: one card per programme does not survive federation
// programmes. Four ISSF courses plus Tech Aim presets already make 48 entries;
// adding DSB would make the current screen unusable.
//
// AUTHORITY. Every level is derived from CompetitionCatalogue - the same
// entries the engine uses - so this cannot offer a programme the engine does
// not know, and cannot lose one. Selection is committed as a programmeId, the
// stable machine identity. Labels are display only: per QML-LANG-001 no
// comparison here touches a translated string, and switching language cannot
// change what was selected.
//
// Step 3 is SKIPPED when a discipline has exactly one programme, so a
// single-programme path stays as short as it is today.
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: selector

    property var catalogue: null

    // The installation's paper mode (APPSETTINGS.getIs15Shoot()). Every level
    // is filtered by it, so the hierarchy only ever offers programmes this
    // installation can actually run - and never shows the same preset twice.
    property bool fifteenShotMode: false

    // Committed ONLY when the final programme is chosen. Nothing upstream sees
    // a half-made choice, so changing level cannot partially mutate a session.
    property string selectedProgrammeId: ""
    signal programmeCommitted(string programmeId)

    // Work-in-progress state, discarded freely by Back.
    property string pendingRuleSet: ""
    property string pendingDiscipline: ""
    property int    step: 0                 // 0 rule set · 1 discipline · 2 programme

    // Overridable so the host page hands in the design tokens rather than this
    // component owning a private palette - the mistake UI-1 unpicked. The
    // literals are only the standalone-render defaults.
    property color bg:      "#15161a"
    property color card:    "#26272c"
    property color cardSel: "#2f3037"
    property color accent:  PRODUCT.accentBright
    property color line:    "#3a3b40"
    property color textPrimary:   "#f2f3f5"
    property color textSecondary: "#c9ced6"
    property color textMuted:     "#9a9ba0"
    property color textOfficial:  "#8fe0a8"

    implicitWidth: 720
    implicitHeight: 460

    function reset() {
        pendingRuleSet = ""; pendingDiscipline = ""; step = 0
    }
    function back() {
        if (step > 0) --step
        if (step < 2) pendingDiscipline = ""
        if (step < 1) pendingRuleSet = ""
    }
    function chooseRuleSet(id) {
        pendingRuleSet = id
        var d = catalogue ? catalogue.disciplines(id, fifteenShotMode) : []
        step = 1
        if (d.length === 1) chooseDiscipline(d[0].disciplineId)
    }
    function chooseDiscipline(id) {
        pendingDiscipline = id
        var p = catalogue ? catalogue.programmes(pendingRuleSet, id, fifteenShotMode) : []
        // Step 3 only earns its place when there is a real choice to make.
        if (p.length === 1) commit(p[0].programmeId)
        else step = 2
    }
    function commit(programmeId) {
        selectedProgrammeId = programmeId
        programmeCommitted(programmeId)
    }

    // Responsive card width. Two per row whenever the panel is wide enough,
    // one when it is not - a fixed 216 px card silently pushed the second row
    // of ISSF disciplines below a clipped, unscrollable edge at 1366x724, so
    // two of the four disciplines could not be reached at all.
    // Catalogue labels arrive as DATA (modelData.labelKey), and qsTr() with a
    // variable argument cannot be seen by lupdate - so those strings would
    // never enter the catalogue and a German UI would show them in English.
    // Listing the English SOURCE text here registers them in this file's
    // translation context, which is the same context the runtime qsTr() call
    // uses. This is a translation-extraction aid ONLY: nothing reads it, and
    // no logic anywhere compares these strings (QML-LANG-001).
    readonly property var translatableLabels: [
        qsTr("ISSF"), qsTr("Practice presets"), qsTr("DSB / German"),
        qsTr("10M AIR RIFLE"), qsTr("10M AIR PISTOL"),
        qsTr("50 Meter RIFLE"), qsTr("50 Meter Free PISTOL"),
        qsTr("UN-LIMITED")
    ]

    readonly property real gutter: 14
    readonly property real cardGap: 12
    // 10 px is left for the scrollbar so it never sits on top of a card.
    readonly property real avail: width - 2 * gutter - 10
    readonly property real cardW: Math.max(
        180, Math.min(260, Math.floor((avail - cardGap) / 2)))
    readonly property real progW: Math.max(
        146, Math.min(190, Math.floor((avail - 2 * cardGap) / 3)))

    Rectangle { anchors.fill: parent; color: selector.bg }

    // -- breadcrumb + back ------------------------------------------------
    Item {
        id: crumbRow
        anchors.top: parent.top; anchors.topMargin: selector.gutter
        anchors.left: parent.left; anchors.leftMargin: selector.gutter
        anchors.right: parent.right; anchors.rightMargin: selector.gutter
        height: 34

        Rectangle {
            id: backBtn
            width: 84; height: 32; radius: 6
            anchors.verticalCenter: parent.verticalCenter
            color: backMouse.pressed ? selector.accent
                 : backMouse.containsMouse ? selector.cardSel : selector.card
            border.color: backMouse.containsMouse ? selector.accent : selector.line
            border.width: 1
            visible: selector.step > 0
            Text {
                anchors.centerIn: parent
                text: qsTr("< Back")
                color: backMouse.pressed ? "white" : selector.textSecondary
                font.pixelSize: 12; font.bold: true
            }
            MouseArea {
                id: backMouse; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selector.back()
            }
        }
        // Breadcrumb. The level being chosen NOW carries the accent; the levels
        // behind you are muted, so "where am I" needs no reading.
        Row {
            anchors.left: selector.step > 0 ? backBtn.right : parent.left
            anchors.leftMargin: selector.step > 0 ? 14 : 0
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            Repeater {
                model: [qsTr("Rule set"), qsTr("Discipline"), qsTr("Programme")]
                Row {
                    spacing: 6
                    visible: index <= selector.step
                    Text {
                        text: "\u203A"
                        visible: index > 0
                        color: selector.textMuted; font.pixelSize: 12
                    }
                    Text {
                        text: modelData
                        color: index === selector.step ? selector.accent : selector.textMuted
                        font.pixelSize: 12
                        font.bold: index === selector.step
                    }
                }
            }
        }
    }

    Text {
        id: stepTitle
        anchors.top: crumbRow.bottom; anchors.topMargin: 12
        anchors.left: parent.left; anchors.leftMargin: selector.gutter
        anchors.right: parent.right; anchors.rightMargin: selector.gutter
        text: selector.step === 0 ? qsTr("SELECT RULE SET")
            : selector.step === 1 ? qsTr("SELECT DISCIPLINE")
                                  : qsTr("SELECT PROGRAMME")
        color: selector.textPrimary
        font.pixelSize: 15; font.bold: true; font.letterSpacing: 1.2
        elide: Text.ElideRight
    }

    // Every step scrolls. A step that cannot scroll can hide an option, and an
    // option the operator cannot reach is the same as an option that does not
    // exist - the one thing a catalogue-driven selector must never do.
    Flickable {
        id: stepScroll
        anchors.top: stepTitle.bottom; anchors.topMargin: 12
        anchors.left: parent.left; anchors.leftMargin: selector.gutter
        anchors.right: parent.right; anchors.rightMargin: selector.gutter
        anchors.bottom: parent.bottom; anchors.bottomMargin: selector.gutter
        clip: true
        contentWidth: width
        contentHeight: ruleSetFlow.visible ? ruleSetFlow.height
                     : disciplineFlow.visible ? disciplineFlow.height
                                              : programmeFlow.height
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            policy: stepScroll.contentHeight > stepScroll.height
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            width: 8
        }

        // -- step 1 - rule set --------------------------------------------
        Flow {
            id: ruleSetFlow
            width: stepScroll.width; spacing: selector.cardGap
            visible: selector.step === 0
            Repeater {
                model: selector.catalogue ? selector.catalogue.ruleSets(selector.fifteenShotMode) : []
                Rectangle {
                    width: selector.cardW; height: 76; radius: 8
                    color: rsMouse.pressed ? selector.cardSel : selector.card
                    border.color: rsMouse.containsMouse ? selector.accent : selector.line
                    border.width: rsMouse.containsMouse ? 2 : 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5
                        Text {
                            text: qsTr(modelData.labelKey)
                            color: selector.textPrimary; font.pixelSize: 16; font.bold: true
                            width: parent.width; elide: Text.ElideRight
                        }
                        Text {
                            // Official authority is stated, never implied.
                            text: modelData.federation !== ""
                                  ? qsTr("Official competition rules")
                                  : qsTr("Practice - no rule authority")
                            color: modelData.federation !== "" ? selector.textOfficial : selector.textMuted
                            font.pixelSize: 10
                            width: parent.width; elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        id: rsMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: selector.chooseRuleSet(modelData.rulesetId)
                    }
                }
            }
        }

        // -- step 2 - discipline ------------------------------------------
        Flow {
            id: disciplineFlow
            width: stepScroll.width; spacing: selector.cardGap
            visible: selector.step === 1
            Repeater {
                model: (selector.catalogue && selector.pendingRuleSet !== "")
                       ? selector.catalogue.disciplines(selector.pendingRuleSet,
                                                        selector.fifteenShotMode) : []
                Rectangle {
                    width: selector.cardW; height: 70; radius: 8
                    color: dMouse.pressed ? selector.cardSel : selector.card
                    border.color: dMouse.containsMouse ? selector.accent : selector.line
                    border.width: dMouse.containsMouse ? 2 : 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4
                        Text {
                            text: qsTr(modelData.labelKey)
                            color: selector.textPrimary; font.pixelSize: 15; font.bold: true
                            width: parent.width; elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.distanceM + " m"
                            color: selector.textMuted; font.pixelSize: 11
                        }
                    }
                    MouseArea {
                        id: dMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: selector.chooseDiscipline(modelData.disciplineId)
                    }
                }
            }
        }

        // -- step 3 - programme -------------------------------------------
        Flow {
            id: programmeFlow
            width: stepScroll.width; spacing: 10
            visible: selector.step === 2
            Repeater {
                model: (selector.catalogue && selector.pendingDiscipline !== "")
                       ? selector.catalogue.programmes(selector.pendingRuleSet,
                                                       selector.pendingDiscipline,
                                                       selector.fifteenShotMode) : []
                Rectangle {
                    width: selector.progW; height: 62; radius: 7
                    color: pMouse.pressed ? selector.cardSel : selector.card
                    border.color: pMouse.containsMouse ? selector.accent
                                : modelData.programmeType === "OFFICIAL"
                                  ? selector.accent : selector.line
                    border.width: (pMouse.containsMouse
                                   || modelData.programmeType === "OFFICIAL") ? 2 : 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 13
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3
                        Text {
                            text: qsTr(modelData.matchDisplayKey)
                            color: selector.textPrimary; font.pixelSize: 14; font.bold: true
                            width: parent.width; elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.programmeType === "OFFICIAL"
                                  ? qsTr("Official course") : qsTr("Preset")
                            color: modelData.programmeType === "OFFICIAL"
                                   ? selector.textOfficial : selector.textMuted
                            font.pixelSize: 10
                            width: parent.width; elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        id: pMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: selector.commit(modelData.programmeId)
                    }
                }
            }
        }
    }
}
