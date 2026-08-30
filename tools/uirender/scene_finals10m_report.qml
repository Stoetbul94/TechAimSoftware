import QtQuick 2.15
import "../.."                  // Finals10mReportView.qml at the repository root

// 10m AR/AP FINAL report — offline render scene (BLOCKER G evidence).
//
// This renders the REAL Finals10mReportView.qml against a REAL report DTO
// emitted by the real controller after a full 24-shot Final
// (finals10m_tests --emit-report). Nothing on the page is drawn by this file.
//
// It is evidence about the report's LAYOUT and CONTENT: titles, the sighters
// section, the 24 official shots, the series subtotals, the total, and that no
// column clips. It is not evidence that the application routes to this view -
// that is what tests/qml asserts and what the running binary shows.
Item {
    id: root
    width: 834
    height: reportView.implicitHeight

    // Ancestor-scope `userName`, exactly as ShootingPage provides it.
    property string userName: "A. Bailie"

    Finals10mReportView {
        id: reportView
        anchors.fill: parent
        Component.onCompleted: reportView.refresh()
    }
}
