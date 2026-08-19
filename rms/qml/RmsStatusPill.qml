import QtQuick 2.15

// A single status token — connection state or match phase — rendered as a
// pill. Colour comes from the shared Tech Aim theme; nothing here invents a
// palette.
Rectangle {
    id: pill

    property string text: ""
    property string tone: "neutral"   // neutral | live | warn | offline

    Theme { id: theme }

    implicitWidth: label.implicitWidth + 20
    implicitHeight: 22
    radius: 11
    color: tone === "live"    ? Qt.rgba(0.18, 0.62, 0.36, 0.18)
         : tone === "warn"    ? Qt.rgba(0.75, 0.10, 0.10, 0.18)
         : tone === "offline" ? Qt.rgba(0.60, 0.60, 0.63, 0.14)
                              : Qt.rgba(1, 1, 1, 0.06)
    border.width: 1
    border.color: tone === "live"    ? theme.statusConnected
                : tone === "warn"    ? theme.brandAccent
                : tone === "offline" ? theme.statusDisconnected
                                     : theme.borderColor

    Text {
        id: label
        anchors.centerIn: parent
        text: pill.text
        font.family: theme.fontFamily
        font.pixelSize: 11
        font.letterSpacing: 0.6
        color: tone === "live"    ? theme.statusConnected
             : tone === "warn"    ? theme.brandAccent
             : tone === "offline" ? theme.statusDisconnected
                                  : theme.textSecondary
    }
}
