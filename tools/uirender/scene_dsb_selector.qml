import QtQuick 2.15
import "../.."
// Evidence scene: the DSB 2026 rule set in the hierarchical selector, driven by
// the real CompetitionCatalogue. Step 1 shows the three rule sets; steps 2 and 3
// walk into DSB rifle and its Luftgewehr 3-Stellung variants.
Item {
    width: 760; height: 1000
    Rectangle { anchors.fill: parent; color: "#0f1013" }
    CompetitionCatalogue { id: cat }
    Column {
        x: 16; y: 12; spacing: 10
        Repeater {
            model: [0, 1, 2]
            Column {
                spacing: 4
                Text {
                    text: ["STEP 1 - RULE SET (ISSF - DSB 2026 - practice)",
                           "STEP 2 - DSB 2026 DISCIPLINES",
                           "STEP 3 - DSB 1.20 LUFTGEWEHR 3-STELLUNG"][index]
                    color: "#25B0E6"; font.pixelSize: 12; font.bold: true
                }
                SetaCompetitionSelector {
                    width: 720; height: 300
                    catalogue: cat
                    Component.onCompleted: {
                        if (index >= 1) { pendingRuleSet = "dsb"; step = 1 }
                        if (index >= 2) { pendingDiscipline = "AR10_3P"; step = 2 }
                    }
                }
            }
        }
    }
}
