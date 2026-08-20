import QtQuick 2.15
// Header at the real test width (1536) with the connection panel's slot
// reserved. Evidence for the overlap fix: the honesty line must sit entirely
// clear of the panel rectangle drawn on the right.
Item {
    width: 1536; height: 42
    property real reserve: 380 + 32          // compact panel width + margins
    // PLATFORM is not registered in this tool's standalone QQuickView, so the
    // guard is required (same idiom as src/ui/theme/Typography.qml). The tool
    // renders the DESKTOP look, so the fallback is the Windows face.
    QtObject { id: theme; property string fontFamily:
        (typeof PLATFORM !== "undefined") ? PLATFORM.uiFont : "Segoe UI" }
    Rectangle { anchors.fill: parent; color: "#15161a" }
    Loader {
        id: ld
        anchors.fill: parent
        source: "file:///" + Qt.resolvedUrl("../../TrainingTopBar.qml").toString().replace("file:///","")
        onLoaded: {
            item.programmeName = "Call & Diagnose"
            item.athlete = "Philemon"
            item.discipline = "50m Rifle Prone"
            item.progressValue = "7 / 10"
            item.progressLabel = "CALLED"
            item.phaseLabel = "SHOOTING"
            item.phaseActive = true
            item.rightReserve = reserve
        }
    }
    // The reserved slot, drawn so the gap is visible in the PNG.
    Rectangle {
        anchors.right: parent.right; anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: 380; height: 30; radius: 6
        color: "#1b2733"; border.color: "#e8003d"; border.width: 1
        Text { anchors.centerIn: parent; text: "USB Serial Port · COM7  [panel slot]"
               color: "white"; font.pixelSize: 11; font.bold: true }
    }
}
