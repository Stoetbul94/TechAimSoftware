import QtQuick 2.15

// Floating "Report" window. Hosts the report layouts (Summary + Match) inside the
// reusable FloatingWindow shell, with tabs in the toolbar and the actions (Save
// PDF / Coach Report) in the footer.
//
// Must be instantiated inside ShootingPage so the embedded views can resolve the
// ShootingPage ids they read (rightPanel, centerPanel, shootingPage, ...).
// No report calculations, values or the PDF grab paths are changed.
FloatingWindow {
    id: reportWin
    title: "Report"
    subtitle: (typeof userName !== "undefined" && userName) ? userName : ""
    minW: 900; minH: 620

    property int tab: 0                 // 0 = Summary · 1 = Match

    signal coachRequestedFromReport()

    // The window owns the scrolling; each view exposes its natural height.
    scrollableContent: true
    contentNaturalHeight: tab === 0 ? summaryView.implicitHeight : matchView.implicitHeight

    // Set the tab before the manager presents the window (works whether opening
    // fresh or re-focusing an already-open window). Called via WindowManager.
    function prepare(t) { reportWin.tab = t }

    // Kiosk auto-export: open on the Match tab and write the PDF to the network
    // path, then close on save. Driven by ShootingPage's onPrintPDF handler.
    function startMatchAutoExport() {
        reportWin.tab = 1
        matchView.startNetworkAutoExport()
    }

    onTabChanged: if (tab === 1) matchView.refresh3P()
    // Runs after prepare() has set the tab, so reopening straight onto Match
    // (no tab change) still refreshes the 3P report tables.
    onAboutToOpen: if (tab === 1) matchView.refresh3P()

    // ── Toolbar: Summary / Match tabs ───────────────────────────────────
    toolbarItem: Rectangle {
        width: parent ? parent.width : 0
        height: 40
        color: "#1f2026"
        Row {
            anchors.left: parent.left; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Repeater {
                model: [{ l: "Summary", t: 0 }, { l: "Match", t: 1 }]
                delegate: Rectangle {
                    width: tt.implicitWidth + 30; height: 30; radius: 6
                    color: reportWin.tab === modelData.t ? "#2f3138" : (ma.containsMouse ? "#26272c" : "transparent")
                    Text {
                        id: tt; anchors.centerIn: parent; text: modelData.l
                        color: reportWin.tab === modelData.t ? "white" : "#9aa0a6"
                        font.family: "Segoe UI"; font.pixelSize: 13; font.bold: reportWin.tab === modelData.t
                    }
                    Rectangle {
                        visible: reportWin.tab === modelData.t
                        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width * 0.6; height: 2; radius: 1; color: "#a80038"
                    }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: reportWin.tab = modelData.t }
                }
            }
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#33353d" }
    }

    // ── Content ─────────────────────────────────────────────────────────
    SummaryReportView {
        id: summaryView
        anchors.fill: parent
        visible: reportWin.tab === 0
    }
    MatchReportView {
        id: matchView
        anchors.fill: parent
        visible: reportWin.tab === 1
        onRequestClose: reportWin.close()
    }

    // ── Footer: Coach Report + Save PDF ─────────────────────────────────
    footerItem: Rectangle {
        width: parent ? parent.width : 0
        height: 44
        color: "#1f2026"
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#33353d" }
        Row {
            anchors.right: parent.right; anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Rectangle {
                width: crTxt.implicitWidth + 26; height: 28; radius: 6
                color: crMA.pressed ? "#2a2b30" : "#26272c"; border.color: "#3a3b42"; border.width: 1
                Text { id: crTxt; anchors.centerIn: parent; text: "Coach Report"; color: "#d7d8dd"; font.family: "Segoe UI"; font.pixelSize: 12 }
                MouseArea { id: crMA; anchors.fill: parent; onClicked: reportWin.coachRequestedFromReport() }
            }
            Rectangle {
                width: spTxt.implicitWidth + 26; height: 28; radius: 6
                color: spMA.pressed ? "#8a002f" : "#a80038"
                Text { id: spTxt; anchors.centerIn: parent; text: "⤓  Save PDF"; color: "white"; font.family: "Segoe UI"; font.pixelSize: 12; font.bold: true }
                MouseArea {
                    id: spMA; anchors.fill: parent
                    onClicked: reportWin.tab === 0 ? summaryView.exportPdf() : matchView.exportPdf()
                }
            }
        }
    }
}
