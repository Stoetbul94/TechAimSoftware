import QtQuick 2.15

// Session Reliability Layer (M2) — persistence-health banner (spec §10).
// A non-blocking strip that appears only while persistence is not Healthy:
// shots keep scoring (never-refuse-to-score, §9C) but the operator is told
// the journal is buffering in memory. View-only: it reads the controller's
// persistenceHealth and never mutates state. TechAim red; docks above the
// bottom action bar, never over the target rings.
Rectangle {
    id: banner

    // ta::rel::Health order: 0 Healthy, 1 Slow, 2 Degraded, 3 Critical,
    // 4 ReadOnly, 5 Failed.
    property int health: (typeof FINALS3P !== "undefined")
                         ? FINALS3P.persistenceHealth : 0
    readonly property bool degraded: health >= 2

    height: degraded ? 28 : 0
    visible: height > 0
    clip: true
    color: health >= 5 ? "#8a1414"      // Failed — RAM only
                       : "#d0392b"       // Degraded / Critical / ReadOnly
    Behavior on height { NumberAnimation { duration: 120 } }

    Text {
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 13
        font.bold: true
        text: {
            if (banner.health >= 5)
                return qsTr("PERSISTENCE FAILED — session held in memory only")
            if (banner.health === 4)
                return qsTr("STORAGE READ-ONLY — new sessions cannot be saved")
            if (banner.health === 3)
                return qsTr("PERSISTENCE CRITICAL — data buffered in memory")
            return qsTr("PERSISTENCE DEGRADED — data buffered in memory")
        }
    }
}
