import QtQuick 2.15

// 3P FINAL HUD — Layer 2: contextual performance block (bottom-right).
// Visible only while official match shots are being fired, or once the current
// stage has recorded official shots. Values come straight from controller
// properties; decimal only, no countdown duplication, no logic here.
Rectangle {
    id: perf
    property var ctl

    // Official-fire stages: KneelingMatch 3, ProneMatch 5, Series 7/8,
    // singles 9-13 (display grouping only — acceptance stays in the controller).
    readonly property int sid: ctl ? ctl.stageId : 0
    readonly property bool officialStage: sid === 3 || sid === 5 || (sid >= 7 && sid <= 13)
    readonly property bool isSingle: sid >= 9 && sid <= 13
    // Stage shot capacity for the "n / N" caption (presentation mirror of the
    // rule constants; the controller enforces the real limits).
    readonly property int stageCap: (sid === 3 || sid === 5) ? 10 : (sid === 7 || sid === 8) ? 5 : 1
    readonly property string stageCaption:
        sid === 3 ? "KNEELING" : sid === 5 ? "PRONE"
        : sid === 7 ? "SERIES 1" : sid === 8 ? "SERIES 2"
        : isSingle ? "SHOT " + (22 + sid) : ""    // 9 -> 31 … 13 -> 35

    visible: ctl && officialStage && (ctl.isFiringWindowOpen || ctl.shotsInStage > 0)
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    width: grid.implicitWidth + 28
    height: grid.implicitHeight + 20
    radius: 10
    color: "#a614151a"
    border.color: "#33353d"; border.width: 1

    Grid {
        id: grid
        anchors.centerIn: parent
        columns: 2
        columnSpacing: 14
        rowSpacing: 3
        verticalItemAlignment: Grid.AlignVCenter

        Text { text: perf.stageCaption; color: "#8a8a92"; font.family: "Segoe UI"
               font.pixelSize: 10; font.letterSpacing: 1.2 }
        Text { text: perf.isSingle ? "" : (perf.ctl ? perf.ctl.shotsInStage : 0) + " / " + perf.stageCap
               color: "white"; font.family: "Segoe UI"; font.pixelSize: 18; font.bold: true }

        Text { visible: !perf.isSingle; text: "STAGE"; color: "#8a8a92"; font.family: "Segoe UI"
               font.pixelSize: 10; font.letterSpacing: 1.2 }
        Text { visible: !perf.isSingle; text: perf.ctl ? perf.ctl.stageSubtotal.toFixed(1) : ""
               color: "white"; font.family: "Segoe UI"; font.pixelSize: 22; font.bold: true }

        Text { text: "TOTAL"; color: "#8a8a92"; font.family: "Segoe UI"
               font.pixelSize: 10; font.letterSpacing: 1.2 }
        Text { text: perf.ctl ? perf.ctl.cumulativeTotal.toFixed(1) : ""
               color: "white"; font.family: "Segoe UI"; font.pixelSize: 22; font.bold: true }
    }
}
