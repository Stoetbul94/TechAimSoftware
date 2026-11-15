import QtQuick 2.15
import "../.."                  // the report views at the repository root
import "../../src/ui/theme"     // the real DesignTokens

// Training Lab report pages — offline render scene (P0 WP1, LOC-REPORT-001).
//
// Renders the REAL TrainingReportView / CallDiagnoseReportView /
// PositionTransitionReportView against a REAL report model emitted by the real
// controller (training_tests --emit-report, or tools/reporttest/cd_session.json),
// and stacks every page the PDF export would grab, in export order (_pageItems()).
// Pass a catalogue as the 5th argument to render it in that language.
//
//   UIRENDER_DATA=<report.json> uirender.exe scene_training_report.qml out.png 794 <height> [techaim_de_DE]
//
// The scene logs "pages laid out: N, height H" - render again at that height.
// It is evidence about LAYOUT in each language (clipping, overflow), not
// about the application routing to the view.
Item {
    id: window                  // the views resolve window.uiLocale() by ancestor id
    width: 794
    height: Math.max(200, laid)
    property int laid: 0

    function uiLocale() {       // identical to main.qml
        var code = (typeof LANGUAGE !== "undefined" && LANGUAGE.languageCode)
                   ? LANGUAGE.languageCode : "en"
        return Qt.locale(code === "en" ? "en_GB" : code.replace("-", "_"))
    }

    // Ancestor-scope `theme`, as main.qml provides it.
    DesignTokens { id: tk }
    QtObject {
        id: theme
        property var tokens: tk
        property string fontFamily: Qt.application.font.family
    }

    Rectangle { anchors.fill: parent; color: "#8A8F98" }      // gutter between pages

    readonly property string kind: RENDER_DATA.blocks ? "training"
                                 : RENDER_DATA.positions ? "pt"
                                 : RENDER_DATA.shots ? "cd" : ""

    Component { id: tbView; TrainingReportView { } }
    Component { id: cdView; CallDiagnoseReportView { } }
    Component { id: ptView; PositionTransitionReportView { } }

    Loader {
        id: ld
        sourceComponent: window.kind === "training" ? tbView
                       : window.kind === "pt" ? ptView
                       : window.kind === "cd" ? cdView : null
        onLoaded: Qt.callLater(window.layOut)
    }

    function layOut() {
        var v = ld.item
        v.model = RENDER_DATA
        if (kind === "training") v.blocks = RENDER_DATA.blocks || []
        if (kind === "pt") v.positions = RENDER_DATA.positions || []
        if (kind === "cd") v.shots = RENDER_DATA.shots || []
        Qt.callLater(function () {
            var pages = v._pageItems(), y = 0
            for (var i = 0; i < pages.length; ++i) {
                var p = pages[i]
                p.parent = window
                p.x = 0; p.y = y; p.visible = true
                y += p.height + 16
            }
            window.laid = y
            console.log("pages laid out: " + pages.length + ", height " + y)
        })
    }
}
