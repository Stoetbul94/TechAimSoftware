import QtQuick 2.15

// CD-REPORT-001 evidence harness. Loads the REAL CallDiagnoseReportView.qml
// (not a copy) with the REAL archived 50 m session, and shows one shot page so
// it can be grabbed. If the Repeater model regresses to an undefined lookup
// again, the rendered page comes back empty and the checker below says so.
Item {
    id: root
    width: 794; height: 1123

    property int shotPageIndex: 0
    property int miniTargetsFound: -1
    property int shotRowsFound: -1
    signal ready()

    Loader {
        id: ld
        source: VIEW_URL          // absolute file: URL, injected from C++
        onLoaded: Qt.callLater(root._populate)
    }

    function _populate() {
        var v = ld.item
        v.model = SESSION
        v.shots = SESSION.shots

        // Pages are stacked at (0,0) - the real exporter grabs them one at a
        // time. Show only the shot page we are inspecting.
        var target = null, seen = 0
        for (var i = 0; i < v.children.length; ++i) {
            var c = v.children[i]
            var isShotPage = (c.pageShots !== undefined)
            if (isShotPage && seen++ === root.shotPageIndex) target = c
            c.visible = false
        }
        if (target) { target.visible = true; target.parent = root }
        Qt.callLater(function () { root._count(target); root.ready() })
    }

    // Count what actually got instantiated. A MiniTarget is 96x96 by contract;
    // the per-shot text column is 120 wide. Counting real scene-graph items is
    // the point - asserting on the binding expression would prove nothing.
    function _count(page) {
        var minis = 0, rows = 0
        function walk(it) {
            if (!it) return
            if (it.width === 96 && it.height === 96) ++minis
            if (it.width === 120) ++rows
            for (var i = 0; i < it.children.length; ++i) walk(it.children[i])
        }
        walk(page)
        root.miniTargetsFound = minis
        root.shotRowsFound = rows
    }
}
