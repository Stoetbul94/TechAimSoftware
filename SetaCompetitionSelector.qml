import QtQuick 2.15

// ─────────────────────────────────────────────────────────────────────────────
// SETA hierarchical competition selector — RULE SET → DISCIPLINE → PROGRAMME.
//
// SETA-ONLY. This lives on product/seta and replaces nothing yet: the existing
// LoginPage card grid still drives production selection. Swapping it in is a
// separate, approved migration step with its own integrated check.
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

    // Committed ONLY when the final programme is chosen. Nothing upstream sees
    // a half-made choice, so changing level cannot partially mutate a session.
    property string selectedProgrammeId: ""
    signal programmeCommitted(string programmeId)

    // Work-in-progress state, discarded freely by Back.
    property string pendingRuleSet: ""
    property string pendingDiscipline: ""
    property int    step: 0                 // 0 rule set · 1 discipline · 2 programme

    readonly property color bg:      "#15161a"
    readonly property color card:    "#26272c"
    readonly property color cardSel: "#2f3037"
    readonly property color accent:  "#e8003d"
    readonly property color line:    "#3a3b40"

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
        var d = catalogue ? catalogue.disciplines(id) : []
        step = 1
        if (d.length === 1) chooseDiscipline(d[0].disciplineId)
    }
    function chooseDiscipline(id) {
        pendingDiscipline = id
        var p = catalogue ? catalogue.programmes(pendingRuleSet, id) : []
        // Step 3 only earns its place when there is a real choice to make.
        if (p.length === 1) commit(p[0].programmeId)
        else step = 2
    }
    function commit(programmeId) {
        selectedProgrammeId = programmeId
        programmeCommitted(programmeId)
    }

    Rectangle { anchors.fill: parent; color: selector.bg }

    Column {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        // ── breadcrumb + back ────────────────────────────────────────────
        Item {
            width: parent.width; height: 34

            Rectangle {
                id: backBtn
                width: 78; height: 30; radius: 6
                color: backMouse.pressed ? selector.cardSel : selector.card
                border.color: selector.line; border.width: 1
                visible: selector.step > 0
                Text {
                    anchors.centerIn: parent
                    text: qsTr("< Back"); color: "#c9ced6"
                    font.pixelSize: 12; font.bold: true
                }
                MouseArea { id: backMouse; anchors.fill: parent; onClicked: selector.back() }
            }
            Text {
                anchors.left: selector.step > 0 ? backBtn.right : parent.left
                anchors.leftMargin: selector.step > 0 ? 14 : 0
                anchors.verticalCenter: parent.verticalCenter
                color: "#8a8b90"; font.pixelSize: 12
                text: {
                    var t = qsTr("Rule set")
                    if (selector.step > 0) t += "  ›  " + qsTr("Discipline")
                    if (selector.step > 1) t += "  ›  " + qsTr("Programme")
                    return t
                }
            }
        }

        Text {
            text: selector.step === 0 ? qsTr("SELECT RULE SET")
                : selector.step === 1 ? qsTr("SELECT DISCIPLINE")
                                      : qsTr("SELECT PROGRAMME")
            color: "#f2f3f5"
            font.pixelSize: 15; font.bold: true; font.letterSpacing: 1.2
        }

        // ── step 1 — rule set ────────────────────────────────────────────
        Flow {
            width: parent.width; spacing: 12
            visible: selector.step === 0
            Repeater {
                model: selector.catalogue ? selector.catalogue.ruleSets() : []
                Rectangle {
                    width: 216; height: 84; radius: 8
                    color: rsMouse.pressed ? selector.cardSel : selector.card
                    border.color: selector.line; border.width: 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5
                        Text {
                            text: qsTr(modelData.labelKey)
                            color: "white"; font.pixelSize: 16; font.bold: true
                        }
                        Text {
                            // Official authority is stated, never implied.
                            text: modelData.federation !== ""
                                  ? qsTr("Official competition rules")
                                  : qsTr("Practice - no rule authority")
                            color: modelData.federation !== "" ? "#8fe0a8" : "#9a9ba0"
                            font.pixelSize: 10
                        }
                    }
                    MouseArea {
                        id: rsMouse; anchors.fill: parent
                        onClicked: selector.chooseRuleSet(modelData.rulesetId)
                    }
                }
            }
        }

        // ── step 2 — discipline ──────────────────────────────────────────
        Flow {
            width: parent.width; spacing: 12
            visible: selector.step === 1
            Repeater {
                model: (selector.catalogue && selector.pendingRuleSet !== "")
                       ? selector.catalogue.disciplines(selector.pendingRuleSet) : []
                Rectangle {
                    width: 216; height: 84; radius: 8
                    color: dMouse.pressed ? selector.cardSel : selector.card
                    border.color: selector.line; border.width: 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5
                        Text {
                            text: qsTr(modelData.labelKey)
                            color: "white"; font.pixelSize: 15; font.bold: true
                        }
                        Text {
                            text: modelData.distanceM + " m"
                            color: "#9a9ba0"; font.pixelSize: 11
                        }
                    }
                    MouseArea {
                        id: dMouse; anchors.fill: parent
                        onClicked: selector.chooseDiscipline(modelData.disciplineId)
                    }
                }
            }
        }

        // ── step 3 — programme ───────────────────────────────────────────
        Flow {
            width: parent.width; spacing: 10
            visible: selector.step === 2
            Repeater {
                model: (selector.catalogue && selector.pendingDiscipline !== "")
                       ? selector.catalogue.programmes(selector.pendingRuleSet,
                                                       selector.pendingDiscipline) : []
                Rectangle {
                    width: 168; height: 66; radius: 7
                    color: pMouse.pressed ? selector.cardSel : selector.card
                    border.color: modelData.programmeType === "OFFICIAL"
                                  ? selector.accent : selector.line
                    border.width: modelData.programmeType === "OFFICIAL" ? 2 : 1
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 13
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3
                        Text {
                            text: qsTr(modelData.matchDisplayKey)
                            color: "white"; font.pixelSize: 14; font.bold: true
                        }
                        Text {
                            text: modelData.programmeType === "OFFICIAL"
                                  ? qsTr("Official course") : qsTr("Preset")
                            color: modelData.programmeType === "OFFICIAL"
                                   ? "#8fe0a8" : "#9a9ba0"
                            font.pixelSize: 10
                        }
                    }
                    MouseArea {
                        id: pMouse; anchors.fill: parent
                        onClicked: selector.commit(modelData.programmeId)
                    }
                }
            }
        }
    }
}
