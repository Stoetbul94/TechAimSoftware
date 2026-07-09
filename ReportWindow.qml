import QtQuick 2.15

// Floating Coach Report window (floating-windows phase 2).
//
// Hosts the existing Coach report views (Dashboard / Detailed / Print) inside
// the reusable FloatingWindow shell, toggled by viewMode. Only the *hosting*
// changes — full-screen overlay becomes a movable, resizable, non-blocking
// window; the coach analytics, data and PDF export are untouched. Each view
// keeps its own toolbar (view switching / export / diary); this window adds the
// move/resize/maximize/close chrome and keeps the report off the live target.
FloatingWindow {
    id: reportWin
    title: "Coach Report"
    subtitle: (typeof userName !== "undefined" && userName) ? userName : ""
    minW: 820; minH: 560

    property int gameSubMode: 0
    property int viewMode: 0            // 0 = dashboard · 1 = detailed · 2 = print

    CoachDashboardPage {
        anchors.fill: parent
        visible: reportWin.viewMode === 0
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDetailsRequested: reportWin.viewMode = 1
        onPrintRequested: reportWin.viewMode = 2
    }
    CoachReportPage {
        anchors.fill: parent
        visible: reportWin.viewMode === 1
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDashboardRequested: reportWin.viewMode = 0
        onPrintRequested: reportWin.viewMode = 2
    }
    CoachPrintPage {
        anchors.fill: parent
        visible: reportWin.viewMode === 2
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDashboardRequested: reportWin.viewMode = 0
        onDetailsRequested: reportWin.viewMode = 1
    }
}
