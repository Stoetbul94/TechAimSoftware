import QtQuick 2.15

// 3P FINAL HUD — Layer 4: compact grouped progress. Milestones
// PREP | K | P | S | S1 | S2 | SINGLES | DONE; the SINGLES group expands to
// 31–35 dots only once the final enters the single-shot phase. Colours:
// completed = neutral green · current = TechAim maroon · future = muted grey ·
// stage with missing shots = amber marker · aborted = red on the current group.
// Maps controller state to visuals only; no logic.
Row {
    id: progress
    property var ctl

    // stepIndex (0..11) -> milestone group (0..7); singles collapse to group 6.
    readonly property int curGroup: !ctl ? 0
        : ctl.stepIndex >= 11 ? 7
        : ctl.stepIndex >= 6 ? 6
        : ctl.stepIndex
    readonly property bool aborted: ctl && ctl.stageId === 15
    readonly property bool inSingles: ctl && ctl.stepIndex >= 6 && ctl.stepIndex <= 10

    // Amber marker per group when the controller records missing shots there.
    property var missingGroups: ({})
    Connections {
        target: progress.ctl
        function onMissingShotRecorded(rec) {
            // Finals stage id -> milestone group (K=3->1, P=5->2, S1=7->4,
            // S2=8->5, singles 9..13 -> 6).
            var s = rec.stageId
            var g = s === 3 ? 1 : s === 5 ? 2 : s === 7 ? 4 : s === 8 ? 5
                  : (s >= 9 && s <= 13) ? 6 : -1
            if (g >= 0) {
                var m = progress.missingGroups
                m[g] = true
                progress.missingGroups = m
            }
        }
    }

    spacing: 4

    Repeater {
        model: [
            { l: "PREP" }, { l: "K" }, { l: "P" }, { l: "S" },
            { l: "S1" }, { l: "S2" }, { l: "SINGLES" }, { l: "DONE" }
        ]
        delegate: Rectangle {
            readonly property bool isCurrent: index === progress.curGroup
            readonly property bool isDone: index < progress.curGroup
            readonly property bool hasMissing: progress.missingGroups[index] === true

            height: 16; radius: 8
            width: (index === 6 && progress.inSingles) ? singlesRow.width + 14
                                                       : lbl.implicitWidth + 14
            color: progress.aborted && isCurrent ? "#d0392b"
                 : isCurrent ? "#a80038"
                 : isDone ? (hasMissing ? "#8a6d1a" : "#1f4d33")
                 : "transparent"
            border.color: isCurrent || isDone ? "transparent" : "#3a3b42"
            border.width: 1
            Behavior on color { ColorAnimation { duration: 200 } }

            Text {
                id: lbl
                anchors.centerIn: parent
                visible: !(index === 6 && progress.inSingles)
                text: modelData.l + (hasMissing ? " !" : "")
                color: isCurrent ? "white" : isDone ? "#9fd3b4" : "#8a8a92"
                font.family: "Segoe UI"; font.pixelSize: 9
                font.bold: isCurrent; font.letterSpacing: 0.5
            }

            // Expanded 31–35 dots, only during the single-shot phase.
            Row {
                id: singlesRow
                visible: index === 6 && progress.inSingles
                anchors.centerIn: parent
                spacing: 4
                Repeater {
                    model: 5
                    delegate: Text {
                        text: 31 + index
                        color: (progress.ctl && progress.ctl.stepIndex - 6 === index) ? "white"
                             : (progress.ctl && progress.ctl.stepIndex - 6 > index) ? "#9fd3b4" : "#c99aa8"
                        font.family: "Segoe UI"; font.pixelSize: 9
                        font.bold: progress.ctl && progress.ctl.stepIndex - 6 === index
                    }
                }
            }
        }
    }
}
