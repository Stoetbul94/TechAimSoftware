import QtQuick 2.15
// Left pane in a TRAINING context. Evidence for UI-TRAIN-001: the badge must
// read the active programme, never the inherited "MATCH 60".
Item {
    width: 460; height: 760
    Row {
        anchors.fill: parent; spacing: 20
        Loader {
            id: a; width: 210; height: parent.height
            source: "file:///" + Qt.resolvedUrl("../../LeftPanel.qml").toString().replace("file:///","")
            onLoaded: { item.width = 210; item.height = a.height
                        item.gameDisplay1 = "RIFLE"; item.gameDisplay2 = "RIFLE"
                        item.matchDisplay = "MATCH 60"          // inherited state
                        item.programmeLabel = "CALL & DIAGNOSE" // active programme
                        item.name = "Philemon" }
        }
        Loader {
            id: b; width: 210; height: parent.height
            source: "file:///" + Qt.resolvedUrl("../../LeftPanel.qml").toString().replace("file:///","")
            onLoaded: { item.width = 210; item.height = b.height
                        item.gameDisplay1 = "RIFLE"; item.gameDisplay2 = "RIFLE"
                        item.matchDisplay = "MATCH 60"
                        item.programmeLabel = ""                // genuine match
                        item.name = "Philemon" }
        }
    }
}
