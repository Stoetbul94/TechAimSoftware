import QtQuick 2.15
import QtQuick.Controls 2.15

// Tech Aim reusable floating-window shell.
//
// Movable (drag title bar), resizable (8 grips), maximizable/restorable,
// closable (button or ESC), non-modal by default so it never blocks the live
// target. Hosts arbitrary QML via the default `content` property; optional
// `toolbarItem` / `footerItem` slots. Remembers its geometry for the life of
// the instance (hide/show keeps position + size).
//
// The shell fills its overlay parent but only the `frame` is opaque/interactive
// — the surrounding area has no MouseArea, so clicks pass through to the target
// underneath (unless `modal`). Place instances inside a WindowManager, which
// handles focus/raise and prevents duplicates.
Item {
    id: win

    // ── Public API ──────────────────────────────────────────────────────
    property string title: "Window"
    property string subtitle: ""
    property string icon: ""              // optional qrc path, shown in the title bar
    property bool resizable: true
    property bool movable: true
    property bool maximizable: true
    property bool modal: false
    property bool escCloses: true
    property bool centerOnOpen: true
    property int  minW: 360
    property int  minH: 240
    // When true the window OWNS the vertical scrolling: content is placed in a
    // Flickable and scrolled up to `contentNaturalHeight`. Pure-content views
    // should then NOT wrap themselves in a Flickable (avoids nested scrollbars)
    // and the host sets contentNaturalHeight to the active view's implicitHeight.
    // When false (default) the content fills the area and manages its own
    // scrolling, if any.
    property bool scrollableContent: false
    property real contentNaturalHeight: 0

    property bool opened: false
    property var  manager: null           // set by WindowManager; used for raise/focus
    property var  _geo: null              // saved geometry {x,y,w,h,max}; null = never saved

    default property alias content: contentHolder.data
    property alias toolbarItem: toolbarSlot.data
    property alias footerItem: footerSlot.data

    signal closed()
    signal aboutToOpen()     // emitted just before the window becomes visible

    anchors.fill: parent
    visible: opened
    z: 1000
    focus: opened

    // ── Methods ─────────────────────────────────────────────────────────
    function open() {
        if (!opened) {
            aboutToOpen()       // let the host refresh content before it shows
            restoreGeometry()   // first open -> centre; reopen -> last geometry (clamped on-screen)
            opened = true
        }
        raise()
        forceActiveFocus()
    }
    function close() {
        if (!opened) return
        saveGeometry()          // remember size/position (incl. maximized) for next open
        if (frame.maximized) restore()
        opened = false
        win.closed()
    }
    // Open-or-focus (WindowManager also dedupes).
    function present() { open() }
    // Bring to front + give keyboard focus without changing opened state.
    function activate() { raise(); forceActiveFocus() }

    function center() {
        frame.x = Math.max(0, (width - frame.width) / 2)
        frame.y = Math.max(0, (height - frame.height) / 2)
        frame.placed = true
    }

    // ── Standard imperative setters (for windows built dynamically) ─────
    function setTitle(t)      { win.title = t }
    function setContent(item) { if (item) item.parent = contentHolder }
    function setToolbar(item) { if (item) item.parent = toolbarSlot }
    function setFooter(item)  { if (item) item.parent = footerSlot }

    // ── Geometry persistence ────────────────────────────────────────────
    // Store the *restore* geometry (not the maximized frame) plus maximized
    // state, so a window reopens exactly where/how it was left.
    function saveGeometry() {
        _geo = frame.maximized
             ? { x: frame.rx, y: frame.ry, w: frame.rw, h: frame.rh, max: true }
             : { x: frame.x,  y: frame.y,  w: frame.width, h: frame.height, max: false }
    }
    // Apply saved geometry; centre on first open, or if the saved position is
    // off the current overlay (e.g. a monitor was removed).
    function restoreGeometry() {
        frame.maximized = false
        if (!_geo) { center(); return }
        var gw = Math.max(minW, Math.min(_geo.w, win.width))
        var gh = Math.max(minH, Math.min(_geo.h, win.height))
        frame.width = gw; frame.height = gh
        var onScreen = (_geo.x + gw > 60) && (_geo.x < win.width - 60)
                    && (_geo.y >= 0) && (_geo.y < win.height - 30)
        if (onScreen) { frame.x = _geo.x; frame.y = _geo.y; frame.placed = true }
        else center()
        if (_geo.max) maximize()
    }
    function raise() {
        if (manager) { manager.raise(win); return }
        // standalone: bump above sibling floating windows
        if (parent) {
            for (var i = 0; i < parent.children.length; ++i) {
                var c = parent.children[i]
                if (c !== win && c.z >= win.z) win.z = c.z + 1
            }
        }
    }
    function toggleMaximize() { frame.maximized ? restore() : maximize() }
    function maximize() {
        if (frame.maximized) return
        frame.rx = frame.x; frame.ry = frame.y; frame.rw = frame.width; frame.rh = frame.height
        frame.maximized = true
        frame.x = 8; frame.y = 8; frame.width = win.width - 16; frame.height = win.height - 16
    }
    function restore() {
        if (!frame.maximized) return
        frame.maximized = false
        frame.x = frame.rx; frame.y = frame.ry; frame.width = frame.rw; frame.height = frame.rh
    }
    // Shared resize math (edges bitmask: 1=left 2=right 4=top 8=bottom).
    function doResize(edges, dx, dy, sfx, sfy, sfw, sfh) {
        if (edges & 2) frame.width  = Math.max(minW, sfw + dx)
        if (edges & 8) frame.height = Math.max(minH, sfh + dy)
        if (edges & 1) { var nw = Math.max(minW, sfw - dx); frame.x = sfx + (sfw - nw); frame.width = nw }
        if (edges & 4) { var nh = Math.max(minH, sfh - dy); frame.y = sfy + (sfh - nh); frame.height = nh }
    }

    Keys.onEscapePressed: if (opened && escCloses && !modal) win.close()

    // ── Modal scrim (only when modal) ───────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#99000000"
        visible: win.modal && win.opened
        MouseArea { anchors.fill: parent }   // swallow clicks behind a modal window
    }

    // ── Window frame ────────────────────────────────────────────────────
    Item {
        id: frame
        width: 760; height: 560
        x: 40; y: 40
        property bool placed: false
        property bool maximized: false
        property real rx; property real ry; property real rw; property real rh   // restore geometry

        // soft shadow
        Rectangle {
            anchors.fill: card; anchors.topMargin: 5; anchors.leftMargin: 1; anchors.rightMargin: -1
            radius: card.radius; color: "#55000000"
        }

        Rectangle {
            id: card
            anchors.fill: parent
            radius: frame.maximized ? 0 : 12
            color: "#1b1c21"
            border.color: "#33353d"; border.width: 1
            clip: true

            // ── Title bar ───────────────────────────────────────────────
            Rectangle {
                id: titleBar
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                height: 40
                color: "#232429"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#33353d" }
                Rectangle {
                    width: 3; radius: 2; height: 18; color: "#a80038"
                    anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter
                }
                Row {
                    anchors.left: parent.left; anchors.leftMargin: 22
                    anchors.right: winBtns.left; anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Image {
                        visible: win.icon.length > 0
                        source: win.icon; width: 16; height: 16; fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: win.title; color: "white"; font.family: "Segoe UI"
                        font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        visible: win.subtitle.length > 0
                        text: "·  " + win.subtitle; color: "#9aa0a6"; font.family: "Segoe UI"
                        font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // drag to move
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: winBtns.width + 8
                    enabled: win.movable && !frame.maximized
                    property real sx; property real sy; property real sfx; property real sfy
                    onPressed: { var g = mapToItem(win, mouse.x, mouse.y); sx = g.x; sy = g.y; sfx = frame.x; sfy = frame.y; win.raise() }
                    onPositionChanged: {
                        if (!pressed) return
                        var g = mapToItem(win, mouse.x, mouse.y)
                        frame.x = sfx + (g.x - sx); frame.y = sfy + (g.y - sy)
                        frame.placed = true
                    }
                    onDoubleClicked: if (win.maximizable) win.toggleMaximize()
                }

                // window buttons
                Row {
                    id: winBtns
                    anchors.right: parent.right; anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Rectangle {
                        visible: win.maximizable
                        width: 36; height: 28; radius: 6
                        color: maxMA.containsMouse ? "#2f3138" : "transparent"
                        Text { anchors.centerIn: parent; text: frame.maximized ? "❐" : "▢"; color: "#c8c9cf"; font.pixelSize: 15 }
                        MouseArea { id: maxMA; anchors.fill: parent; hoverEnabled: true; onClicked: win.toggleMaximize() }
                    }
                    Rectangle {
                        width: 36; height: 28; radius: 6
                        color: closeMA.containsMouse ? "#a80038" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: closeMA.containsMouse ? "white" : "#c8c9cf"; font.pixelSize: 13 }
                        MouseArea { id: closeMA; anchors.fill: parent; hoverEnabled: true; onClicked: win.close() }
                    }
                }
            }

            // ── Toolbar slot (0 height when empty) ──────────────────────
            Item {
                id: toolbarSlot
                anchors.top: titleBar.bottom; anchors.left: parent.left; anchors.right: parent.right
                height: childrenRect.height
            }

            // ── Content area — the window owns the Flickable ────────────
            // scrollableContent = true : scrolls the hosted view by its natural
            //   height (window owns scrolling; views expose implicitHeight).
            // scrollableContent = false: acts as a plain clipped container that
            //   the view fills (the view manages its own scrolling if needed).
            Flickable {
                id: contentFlick
                anchors.top: toolbarSlot.bottom
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: footerSlot.top
                clip: true
                interactive: win.scrollableContent
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: width
                contentHeight: win.scrollableContent
                               ? Math.max(height, win.contentNaturalHeight)
                               : height
                ScrollBar.vertical: ScrollBar {
                    policy: win.scrollableContent ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }
                Item {
                    id: contentHolder
                    width: contentFlick.width
                    height: contentFlick.contentHeight
                }
            }

            // ── Footer slot (0 height when empty) ───────────────────────
            Item {
                id: footerSlot
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                height: childrenRect.height
            }
        }

        // ── Resize grips ────────────────────────────────────────────────
        // Corners
        FloatingWindowGrip { edges: 1|4; win: win; frame: frame; cursorShape: Qt.SizeFDiagCursor
            width: 12; height: 12; anchors.left: parent.left; anchors.top: parent.top }
        FloatingWindowGrip { edges: 2|4; win: win; frame: frame; cursorShape: Qt.SizeBDiagCursor
            width: 12; height: 12; anchors.right: parent.right; anchors.top: parent.top }
        FloatingWindowGrip { edges: 1|8; win: win; frame: frame; cursorShape: Qt.SizeBDiagCursor
            width: 12; height: 12; anchors.left: parent.left; anchors.bottom: parent.bottom }
        FloatingWindowGrip { edges: 2|8; win: win; frame: frame; cursorShape: Qt.SizeFDiagCursor
            width: 16; height: 16; anchors.right: parent.right; anchors.bottom: parent.bottom }
        // Edges
        FloatingWindowGrip { edges: 4; win: win; frame: frame; cursorShape: Qt.SizeVerCursor
            height: 6; anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 12; anchors.rightMargin: 12 }
        FloatingWindowGrip { edges: 8; win: win; frame: frame; cursorShape: Qt.SizeVerCursor
            height: 6; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 12; anchors.rightMargin: 12 }
        FloatingWindowGrip { edges: 1; win: win; frame: frame; cursorShape: Qt.SizeHorCursor
            width: 6; anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 12; anchors.bottomMargin: 12 }
        FloatingWindowGrip { edges: 2; win: win; frame: frame; cursorShape: Qt.SizeHorCursor
            width: 6; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 12; anchors.bottomMargin: 12 }
    }
}
