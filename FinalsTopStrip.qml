import QtQuick 2.15

// 3P FINAL HUD — Layer 1: the only permanent element. Phase identity (left),
// compact progress (centre), countdown + firing-window state (right).
// Collapses gracefully at narrow widths (spec §11). Presentation only.
Rectangle {
    id: strip
    property var ctl
    readonly property bool narrow: width < 640
    readonly property bool ended: ctl && (ctl.stageId === 14 || ctl.stageId === 15)

    color: "#d915161a"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#33353d" }

    // Left: concise stage/position identity (controller-provided label).
    Text {
        anchors.left: parent.left; anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width * (strip.narrow ? 0.5 : 0.32)
        text: ctl ? ctl.stageLabel : ""
        color: "white"; font.family: "Segoe UI"; font.bold: true
        font.pixelSize: Math.max(12, Math.min(15, strip.height * 0.34))
        font.letterSpacing: 1.2
        elide: Text.ElideRight
    }

    // Centre: compact grouped progress (Layer 4).
    FinalsProgressIndicator {
        ctl: strip.ctl
        visible: !strip.narrow
        anchors.centerIn: parent
    }

    // Right: large remaining time + window LED (+ label when wide).
    Row {
        anchors.right: parent.right; anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: strip.ended || !ctl ? "" : ctl.remainingFormatted
            color: "white"; font.family: "Segoe UI"; font.bold: true
            font.pixelSize: Math.max(16, Math.min(22, strip.height * 0.5))
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 10; height: 10; radius: 5
            color: !ctl ? "#5a5c64"
                 : ctl.windowState === 2 ? "#3ddc84"
                 : ctl.windowState === 1 ? "#e8b93d" : "#5a5c64"
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        Text {
            visible: !strip.narrow
            anchors.verticalCenter: parent.verticalCenter
            text: !ctl ? "" : ctl.windowState === 2 ? "MATCH OPEN"
                 : ctl.windowState === 1 ? "SIGHTING OPEN" : "CLOSED"
            color: ctl && ctl.isFiringWindowOpen ? "#d7d8dd" : "#8a8a92"
            font.family: "Segoe UI"; font.pixelSize: 11; font.letterSpacing: 1
        }
    }
}
