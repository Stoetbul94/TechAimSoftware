import QtQuick 2.15

// Floating Coach Report window (floating-windows phase 2).
//
// Hosts the existing Coach report views (Dashboard / Detailed / Print) inside
// the reusable FloatingWindow shell in *embedded* mode: each view's own toolbar
// is hidden, and the window supplies the chrome — title bar, the
// Dashboard/Detailed/Print tabs, and the Print actions (Export PDF / Save Diary)
// in the footer. Coach analytics, data, diary save/load and PDF export are
// untouched — only the hosting/chrome changes.
FloatingWindow {
    id: reportWin
    title: "Coach Report"
    subtitle: (typeof userName !== "undefined" && userName) ? userName : ""
    minW: 820; minH: 560

    property int gameSubMode: 0
    property int viewMode: 0            // 0 = dashboard · 1 = detailed · 2 = print

    // ── Toolbar: Dashboard / Detailed / Print tabs ──────────────────────
    toolbarItem: Rectangle {
        width: parent ? parent.width : 0
        height: 40
        color: "#1f2026"
        Row {
            anchors.left: parent.left; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Repeater {
                model: [{ l: "Dashboard", m: 0 }, { l: "Detailed", m: 1 }, { l: "Print", m: 2 }]
                delegate: Rectangle {
                    width: tabTxt.implicitWidth + 30; height: 30; radius: 6
                    color: reportWin.viewMode === modelData.m ? "#2f3138" : (tabMA.containsMouse ? "#26272c" : "transparent")
                    Text {
                        id: tabTxt; anchors.centerIn: parent; text: modelData.l
                        color: reportWin.viewMode === modelData.m ? "white" : "#9aa0a6"
                        font.family: "Segoe UI"; font.pixelSize: 13; font.bold: reportWin.viewMode === modelData.m
                    }
                    Rectangle {
                        visible: reportWin.viewMode === modelData.m
                        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width * 0.6; height: 2; radius: 1; color: "#a80038"
                    }
                    MouseArea { id: tabMA; anchors.fill: parent; hoverEnabled: true; onClicked: reportWin.viewMode = modelData.m }
                }
            }
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#33353d" }
    }

    // ── Content: the active coach view (internal toolbars hidden) ────────
    CoachDashboardPage {
        anchors.fill: parent; embedded: true
        visible: reportWin.viewMode === 0
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDetailsRequested: reportWin.viewMode = 1
        onPrintRequested: reportWin.viewMode = 2
    }
    CoachReportPage {
        anchors.fill: parent; embedded: true
        visible: reportWin.viewMode === 1
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDashboardRequested: reportWin.viewMode = 0
        onPrintRequested: reportWin.viewMode = 2
    }
    CoachPrintPage {
        id: coachPrint
        anchors.fill: parent; embedded: true
        visible: reportWin.viewMode === 2
        gameSubMode: reportWin.gameSubMode
        onClosed: reportWin.close()
        onDashboardRequested: reportWin.viewMode = 0
        onDetailsRequested: reportWin.viewMode = 1
    }

    // ── Footer: Print actions (Export PDF + Save Diary) on the Print tab ─
    footerItem: Rectangle {
        width: parent ? parent.width : 0
        height: 44
        color: "#1f2026"
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#33353d" }

        Text {
            visible: reportWin.viewMode !== 2
            anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter
            text: "Open the Print tab to export the report as PDF"
            color: "#6b6d75"; font.family: "Segoe UI"; font.pixelSize: 11
        }

        Row {
            visible: reportWin.viewMode === 2
            anchors.right: parent.right; anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Rectangle {
                width: sdTxt.implicitWidth + 26; height: 28; radius: 6
                color: sdMA.pressed ? "#2a2b30" : "#26272c"; border.color: "#3a3b42"; border.width: 1
                Text { id: sdTxt; anchors.centerIn: parent; text: "Save Diary"; color: "#d7d8dd"; font.family: "Segoe UI"; font.pixelSize: 12 }
                MouseArea { id: sdMA; anchors.fill: parent; onClicked: coachPrint.saveDiary() }
            }
            Rectangle {
                width: epTxt.implicitWidth + 26; height: 28; radius: 6
                color: epMA.pressed ? "#8a002f" : "#a80038"
                Text { id: epTxt; anchors.centerIn: parent; text: "⤓  Export PDF"; color: "white"; font.family: "Segoe UI"; font.pixelSize: 12; font.bold: true }
                MouseArea { id: epMA; anchors.fill: parent; onClicked: coachPrint.exportPdf() }
            }
        }
    }
}
