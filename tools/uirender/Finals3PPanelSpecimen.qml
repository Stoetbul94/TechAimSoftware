import QtQuick 2.15
import "../.."                 // Finals3PRightPanel.qml at the repository root
import "../../src/ui/theme"    // the real DesignTokens

// 50 m 3P Final right panel — offline render specimen.
//
// This renders the REAL Finals3PRightPanel.qml against the REAL DesignTokens,
// so the PNG shows the actual component and the actual theme values, not a
// redrawing of them. The controller is a mock holding one representative
// mid-Final state, because a render is a still: the moving parts are qualified
// by tests/finals, which drives the real controller through the whole course.
//
// This is a COMPONENT RENDER, not the application. It is evidence about
// layout, hierarchy and contrast. It is not evidence that the application
// mounts the panel correctly — that is what the running binary shows.
Item {
    id: root
    width: 420
    height: 800

    property string appearanceMode: "dark"

    DesignTokens { id: tk; appearance: root.appearanceMode }

    // Ancestor-scope `theme`, exactly as main.qml provides it.
    property QtObject theme: QtObject {
        property var tokens: tk
        property string fontFamily: "Segoe UI"
    }

    Rectangle { anchors.fill: parent; color: tk.backgroundPrimary }

    QtObject {
        id: mock
        // A representative moment: standing, series 2, 27 official shots down.
        property int stageId: 8                       // Stage::StandingSeries2
        property string positionLabel: "STANDING"
        property string stageLabel: "STANDING · SERIES 2"
        property string remainingFormatted: "03:12"
        property string commandText: "START"
        property bool paused: false
        property bool isFiringWindowOpen: true
        property bool primaryActionVisible: true
        property bool primaryActionEnabled: true
        property string primaryActionLabel: "SERIES 2 — 2 / 5"
        property int officialShotCount: 27
        property real cumulativeTotal: 281.4
        function stageSubtotals() {
            return { "KneelingMatch": 104.2, "ProneMatch": 105.6,
                     "StandingSeries1": 52.1, "StandingSeries2": 19.5 }
        }
        signal shotAccepted(var shot)
        signal totalsChanged()
        signal phaseChanged()
    }

    Finals3PRightPanel {
        id: panel
        anchors.fill: parent
        ctl: mock
        Component.onCompleted: {
            panel.refreshSubtotals()
            // A short, representative history including a sighter, so the
            // sighter treatment is visible in the render.
            var hist = [
                { n: 0,  s: "10.2", sig: true  },
                { n: 21, s: "10.4", sig: false },
                { n: 22, s: "9.8",  sig: false },
                { n: 23, s: "10.7", sig: false },
                { n: 24, s: "10.1", sig: false },
                { n: 25, s: "9.6",  sig: false },
                { n: 26, s: "10.9", sig: false },
                { n: 27, s: "8.6",  sig: false }
            ]
            for (var i = 0; i < hist.length; ++i)
                mock.shotAccepted({
                    isSighter: hist[i].sig,
                    finalsShotNumber: hist[i].n,
                    calculatedscore: hist[i].s,
                    timeComsumed: 14 + i,
                    finalsPosition: 2
                })
        }
    }
}
