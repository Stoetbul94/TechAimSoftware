import QtQuick 2.15

// Range-wide facts, above the lane list. Everything here is observation
// bookkeeping — there is nothing to press.
Rectangle {
    id: bar

    property int nodeCount: 0
    property int onlineCount: 0
    property int offlineCount: 0
    property int rejectedDatagrams: 0
    property int protocolVersion: 0
    property int observationPort: 0
    property bool simulated: false

    Theme { id: theme }

    height: 62
    color: theme.bgSurface
    border.width: 1
    border.color: theme.borderColor
    radius: theme.radiusMedium

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: theme.spacingUnit * 3
        spacing: theme.spacingUnit * 5

        Repeater {
            model: [
                { k: "LANES OBSERVED", v: bar.nodeCount.toString(),    t: "neutral" },
                { k: "ONLINE",         v: bar.onlineCount.toString(),  t: "live"    },
                { k: "OFFLINE",        v: bar.offlineCount.toString(),
                  t: bar.offlineCount > 0 ? "warn" : "neutral" },
                { k: "REJECTED",       v: bar.rejectedDatagrams.toString(),
                  t: bar.rejectedDatagrams > 0 ? "warn" : "neutral" },
                { k: "PROTOCOL",       v: "v" + bar.protocolVersion,   t: "neutral" },
                { k: "OBSERVING UDP",  v: bar.observationPort.toString(), t: "neutral" }
            ]

            delegate: Column {
                spacing: 3
                Text {
                    text: modelData.k
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Text {
                    text: modelData.v
                    font.family: theme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    color: modelData.t === "live" ? theme.statusConnected
                         : modelData.t === "warn" ? theme.brandAccent
                                                  : theme.textPrimary
                }
            }
        }
    }

    // The read-only boundary is stated on the screen itself, not only in the
    // architecture document. Milestone 1 has no control capability at all.
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: theme.spacingUnit * 3
        spacing: theme.spacingUnit

        RmsStatusPill {
            visible: bar.simulated
            text: "SIMULATED RANGE"
            tone: "warn"
        }
        RmsStatusPill {
            text: "READ-ONLY OBSERVER  ·  NO CONTROL"
            tone: "neutral"
        }
    }
}
