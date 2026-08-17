import QtQuick 2.15
import "../.."
// Evidence scene for the SETA hierarchical selector: the three steps side by
// side, each driven by the real CompetitionCatalogue.
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
                    text: ["STEP 1 - RULE SET", "STEP 2 - DISCIPLINE (ISSF)",
                           "STEP 3 - PROGRAMME (ISSF - 10 m Air Rifle)"][index]
                    color: "#e8003d"; font.pixelSize: 12; font.bold: true
                }
                SetaCompetitionSelector {
                    width: 720; height: 300
                    catalogue: cat
                    Component.onCompleted: {
                        if (index >= 1) { pendingRuleSet = "issf"; step = 1 }
                        if (index >= 2) { pendingDiscipline = "AR10"; step = 2 }
                    }
                }
            }
        }
    }
}
