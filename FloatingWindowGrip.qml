import QtQuick 2.15

// Resize grip for FloatingWindow. `edges` is a bitmask: 1=left 2=right 4=top
// 8=bottom (corners combine two). Reports drags to the owning window's
// doResize(); all cross-object access is qualified via the passed ids.
MouseArea {
    property int edges: 0
    property var win: null
    property var frame: null

    property real sx; property real sy
    property real sfx; property real sfy; property real sfw; property real sfh

    enabled: win && win.resizable && frame && !frame.maximized
    visible: enabled

    onPressed: {
        var g = mapToItem(win, mouse.x, mouse.y)
        sx = g.x; sy = g.y
        sfx = frame.x; sfy = frame.y; sfw = frame.width; sfh = frame.height
        win.raise()
    }
    onPositionChanged: {
        if (!pressed) return
        var g = mapToItem(win, mouse.x, mouse.y)
        win.doResize(edges, g.x - sx, g.y - sy, sfx, sfy, sfw, sfh)
    }
}
