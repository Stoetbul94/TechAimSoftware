import QtQuick 2.15
import "../.."                  // Finals10mReportView.qml at the repository root

// 10m AR/AP FINAL report — offline render scene (BLOCKER G evidence).
//
// This renders the REAL Finals10mReportView.qml against a REAL report DTO
// emitted by the real controller after a full 24-shot Final
// (finals10m_tests --emit-report). Nothing on the page is drawn by this file.
//
//   UIRENDER_DATA=<report_ar.json> uirender.exe scene_finals10m_report.qml out.png 834 <h> [techaim_de_DE]
//
// It is evidence about the report's LAYOUT and CONTENT: titles, the sighters
// section, the 24 official shots, the series subtotals, the total, and that no
// column clips - in English and, with the catalogue argument, in German
// (LOC-REPORT-001). It is not evidence that the application routes to this
// view - that is what tests/qml asserts and what the running binary shows.
Item {
    id: window                  // the view resolves window.uiLocale() by ancestor id
    width: 834
    height: reportView.implicitHeight

    function uiLocale() {       // identical to main.qml
        var code = (typeof LANGUAGE !== "undefined" && LANGUAGE.languageCode)
                   ? LANGUAGE.languageCode : "en"
        return Qt.locale(code === "en" ? "en_GB" : code.replace("-", "_"))
    }

    // Ancestor-scope `userName`, exactly as ShootingPage provides it.
    property string userName: "A. Bailie"

    Finals10mReportView {
        id: reportView
        anchors.fill: parent
        Component.onCompleted: {
            if (RENDER_DATA && RENDER_DATA.displayName !== undefined) {
                // the emitted DTO is exactly what FINALS10M.buildReport() returns
                reportView.report = RENDER_DATA
                reportView.generatedStamp = new Date(2026, 6, 17, 10, 0)
                        .toLocaleString(window.uiLocale(), "ddd yyyy-MM-dd hh:mm")
            } else {
                reportView.refresh()
            }
        }
    }
}
