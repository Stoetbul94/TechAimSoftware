import QtQuick 2.15

// 3P FINAL HUD — developer-only drawer. Visible ONLY when the app config
// enables developer mode (APPSETTINGS.getDeveloperMode(), config.ini
// developer_mode=1). Collapsed to a small "DEV" tab; expands to the testing
// controls. Production athletes never see any of this. Presentation only —
// every action is a controller call.
Item {
    id: drawer
    property var ctl
    property bool open: false

    width: open ? panel.width : tab.width
    height: open ? panel.height : tab.height

    Rectangle {
        id: tab
        visible: !drawer.open
        width: 42; height: 20; radius: 4
        color: tabMA.pressed ? "#2a2b30" : "#26272c"
        border.color: "#3a3b42"; border.width: 1
        Text { anchors.centerIn: parent; text: "DEV"; color: "#8a8a92"
               font.family: "Segoe UI"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
        MouseArea { id: tabMA; anchors.fill: parent; onClicked: drawer.open = true }
    }

    Rectangle {
        id: panel
        visible: drawer.open
        width: col.implicitWidth + 20
        height: col.implicitHeight + 20
        radius: 8
        color: "#e614151a"
        border.color: "#3a3b42"; border.width: 1

        Column {
            id: col
            anchors.centerIn: parent
            spacing: 5

            Text { text: "DEVELOPER · TESTING"; color: "#8a8a92"; font.family: "Segoe UI"
                   font.pixelSize: 9; font.letterSpacing: 1.5 }

            Repeater {
                model: [
                    { l: "SKIP CEREMONY", a: function() { drawer.ctl.skipCeremony() } },
                    { l: "PAUSE / RESUME", a: function() { drawer.ctl.paused ? drawer.ctl.resumeTrainingSimulation()
                                                                             : drawer.ctl.pauseTrainingSimulation() } },
                    { l: "ABORT", a: function() { drawer.ctl.abortFinal() } },
                    { l: "RESET + START", a: function() { drawer.ctl.resetFinal(); drawer.ctl.startFinal() } },
                    { l: "CLOSE", a: function() { drawer.open = false } }
                ]
                delegate: Rectangle {
                    width: Math.max(120, dTxt.implicitWidth + 18); height: 22; radius: 4
                    color: dMA.pressed ? "#2a2b30" : "#26272c"
                    border.color: "#3a3b42"; border.width: 1
                    Text { id: dTxt; anchors.centerIn: parent
                           text: modelData.l === "PAUSE / RESUME" && drawer.ctl && drawer.ctl.paused
                                 ? "RESUME" : modelData.l
                           color: "#d7d8dd"; font.family: "Segoe UI"; font.pixelSize: 10 }
                    MouseArea { id: dMA; anchors.fill: parent; onClicked: modelData.a() }
                }
            }
        }
    }
}
