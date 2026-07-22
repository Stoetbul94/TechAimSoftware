import QtQuick 2.15

// Tech Aim floating-window overlay + registry.
//
// Single owner of the app's floating windows. Windows register themselves by
// key (register("report", win)); pages then request them by intent
// (openReport(), openCoach(), …) rather than touching window instances. It has
// no MouseArea of its own, so empty areas stay click-through to whatever is
// underneath — only opened windows are interactive.
Item {
    id: manager
    anchors.fill: parent

    // Highest z handed out so far; each raise() bumps the target above the rest.
    property int topZ: 1000
    // key -> FloatingWindow instance. Mutated in place (no bindings depend on it).
    property var registry: ({})

    // ── Registration ────────────────────────────────────────────────────
    function register(key, w) {
        if (key && w) { manager.registry[key] = w; w.manager = manager }
    }
    function windowFor(key) { return manager.registry[key] }

    // ── Generic lifetime ────────────────────────────────────────────────
    function raise(w) {
        if (!w) return
        manager.topZ += 1
        w.z = manager.topZ
    }
    // Open-or-focus a window instance (no duplicates: an open window is raised).
    function present(w) {
        if (!w) return
        w.manager = manager
        if (!w.opened) w.open()
        manager.raise(w)
        w.forceActiveFocus()
    }
    function dismiss(w) { if (w && w.opened) w.close() }

    function anyOpen() {
        for (var k in manager.registry) {
            var w = manager.registry[k]
            if (w && w.opened === true) return true
        }
        return false
    }

    // ── App-facing facade (pages call these, not window instances) ──────
    // Optional `arg` is handed to the window's prepare() (if it defines one)
    // before it is presented — e.g. the Report window uses it to pick a tab.
    function open(key, arg) {
        var w = manager.registry[key]
        if (!w) { console.log("WindowManager: no window registered for key '" + key + "'"); return }
        if (arg !== undefined && typeof w.prepare === "function") w.prepare(arg)
        manager.present(w)
    }
    function close(key) { manager.dismiss(manager.registry[key]) }

    function openReport()      { manager.open("report", 0) }   // Summary tab
    function openMatchReport() { manager.open("report", 1) }   // Match tab
    function openFinalsReport(){ manager.open("report", 2) }   // Finals tab (3P FINAL)
    function openCoach()       { manager.open("coach") }
    function openIncidents()   { manager.open("incidents") }   // EST incident workflow (Phase E)
    // Reserved for upcoming tools — register the window, then these just work:
    function openStatistics() { manager.open("statistics") }
    function openSettings()   { manager.open("settings") }
    function openGroup()      { manager.open("group") }
}
