import QtQuick 2.15
import "../.."
// Same placement, but scrolled to the END so the last control can be seen to
// be reachable. The Flickable is found by walking the panel's children - the
// scene reaches in, the component exposes nothing extra for evidence.
Item {
    id: appArea
    property int appH: 960
    width: 720; height: appH
    Rectangle { anchors.fill: parent; color: "#15161a" }
    SettingsPage { id: sp; x: 226; y: 430 }

    function findFlick(item) {
        for (var i = 0; i < item.children.length; ++i) {
            var c = item.children[i]
            if (c.contentHeight !== undefined && c.contentY !== undefined) return c
            var f = findFlick(c)
            if (f) return f
        }
        return null
    }
    Timer {
        interval: 400; running: true
        onTriggered: {
            var f = appArea.findFlick(sp)
            if (f) f.contentY = Math.max(0, f.contentHeight - f.height)
        }
    }
    Text {
        anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
        text: "scrolled to end · panel " + Math.round(sp.width) + " x " + Math.round(sp.height)
        color: "#7d8794"; font.pixelSize: 11
    }
}
