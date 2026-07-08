import QtQuick 2.15

// Tech Aim floating-window overlay. Place once over the main content (give it a
// high z so windows float above the shooting screen). It has no MouseArea of
// its own, so empty areas stay click-through to the live target — only opened
// windows are interactive.
//
// Windows are declared as children of this manager (one instance per window
// type). Use present(win) to open-or-focus a window: if it is already open it
// is simply raised/focused, so the same action never spawns a duplicate.
Item {
    id: manager
    anchors.fill: parent

    // Highest z handed out so far; each raise() bumps the target above the rest.
    property int topZ: 1000

    // Bring a window to the front.
    function raise(w) {
        if (!w) return
        manager.topZ += 1
        w.z = manager.topZ
    }

    // Open the window if closed; otherwise focus/raise it (no duplicates).
    function present(w) {
        if (!w) return
        w.manager = manager
        if (!w.opened) w.open()
        manager.raise(w)
        w.forceActiveFocus()
    }

    // Close a window if it is open.
    function dismiss(w) {
        if (w && w.opened) w.close()
    }

    // True if any hosted window is currently open (e.g. to dim the main screen).
    function anyOpen() {
        for (var i = 0; i < manager.children.length; ++i) {
            var c = manager.children[i]
            if (c && c.opened === true) return true
        }
        return false
    }
}
