import QtQuick 2.15
import "../.."
// The real SettingsPage, placed exactly as ShootingPage places it: to the
// right of the nav column, at the Settings button's y, inside a parent that
// stands in for the application area.
Item {
    id: appArea
    property int appH: 960
    width: 720; height: appH
    Rectangle { anchors.fill: parent; color: "#15161a" }
    Rectangle {
        x: 8; y: 8; width: 210; height: parent.height - 16
        color: "#1b1c20"; radius: 8
        Text { anchors.centerIn: parent; text: "(left pane)"; color: "#4a4b52"; font.pixelSize: 12 }
    }
    SettingsPage {
        id: sp
        x: 226
        y: 430
        modeChangeBlocked: false
    }
    Text {
        anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
        text: "panel " + Math.round(sp.width) + " x " + Math.round(sp.height)
              + "  ·  app height " + appArea.appH
        color: "#7d8794"; font.pixelSize: 11
    }
}
