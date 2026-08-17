import QtQuick 2.15
import "../.."
// Evidence scene for the SETA hierarchical selector, driven by the REAL
// CompetitionCatalogue. Each panel is one step of the same component; the
// fourth shows the state after Back, so the breadcrumb and the Back control
// are visible rather than described.
//
// Step 3 uses the practice-preset rule set on purpose: the ISSF disciplines
// have exactly one programme each, so the selector auto-commits and step 3 is
// correctly never shown for them.
Item {
    width: 760; height: 1320
    Rectangle { anchors.fill: parent; color: "#0f1013" }
    CompetitionCatalogue { id: cat }
    Column {
        x: 16; y: 12; spacing: 10
        Repeater {
            model: [0, 1, 2, 3]
            Column {
                spacing: 4
                Text {
                    text: ["STEP 1 - RULE SET",
                           "STEP 2 - DISCIPLINE (ISSF)",
                           "STEP 3 - PROGRAMME (Practice presets - 10 m Air Rifle)",
                           "BACK from step 3 - breadcrumb and Back control"][index]
                    color: "#e8003d"; font.pixelSize: 12; font.bold: true
                }
                SetaCompetitionSelector {
                    width: 720; height: 300
                    catalogue: cat
                    Component.onCompleted: {
                        if (index === 1) { pendingRuleSet = "issf"; step = 1 }
                        if (index >= 2)  { pendingRuleSet = "techaim"
                                           pendingDiscipline = "AR10"; step = 2 }
                        if (index === 3) back()
                    }
                }
            }
        }
    }
}
