import QtQuick 2.15
import QtQuick.Window 2.15
import "../.."   // resolve the Finals* HUD components from the repo root

// 3P FINAL HUD — presentation-state harness (HUD4). Drives FinalsHud with a
// MOCK controller through the validation states and saves screenshots to
// docs/img/. Run:  qml.exe hud_harness.qml   (from tests/finals/).
// No app, no controller, no models — pure component verification.
Window {
    id: win
    width: 1100; height: 760
    visible: true
    color: "#0f1013"

    // ── controller mock: same property/signal surface the HUD binds to ──
    QtObject {
        id: mock
        property string stageLabel: "IDLE"
        property int stageId: 0
        property int stepIndex: 0
        property int windowState: 0
        property bool isFiringWindowOpen: windowState !== 0
        property string remainingFormatted: "00:00"
        property int shotsInStage: 0
        property real stageSubtotal: 0
        property real cumulativeTotal: 0
        property string advanceLabel: ""
        property bool paused: false
        signal commandIssued(var ev)
        signal shotRejected(var rej)
        signal advanceConfirmationRequired(int fired, int limit)
        signal phaseChanged()
        signal missingShotRecorded(var rec)
        function advanceStage1() {}
        function confirmStage1Advance() {}
        function cancelStage1Advance() {}
        function skipCeremony() {}
        function pauseTrainingSimulation() {}
        function resumeTrainingSimulation() {}
        function abortFinal() {}
        function resetFinal() {}
        function startFinal() {}
    }

    Item {
        id: sceneRoot
        anchors.fill: parent

    // Simulated target area (real 50m rifle face artwork).
    Rectangle {
        id: targetArea
        anchors.fill: parent
        color: "#f4efe4"
        Image {
            source: "../../images/centerPanel/black_50_Rifle.png"
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height) * 0.92
            height: width
        }
    }

    FinalsHud {
        id: hud
        anchors.fill: targetArea
        ctl: mock
        developerMode: false
    }
    }

    function snap(name) {
        sceneRoot.grabToImage(function(result) {
            result.saveToFile("../../docs/img/" + name)
            console.log("SAVED", name)
        })
    }

    property int step: 0
    Timer {
        interval: 900; running: true; repeat: true
        onTriggered: {
            step++
            if (step === 1) {          // 1. Preparation / Sighting
                mock.stageLabel = "PREPARATION · SIGHTING"
                mock.stageId = 2; mock.stepIndex = 0
                mock.windowState = 1
                mock.remainingFormatted = "04:39"
                mock.commandIssued({ typeName: "PreparationSightingStart",
                                     text: "FIVE MINUTES PREPARATION AND SIGHTING TIME…START" })
            } else if (step === 2) { snap("finals-hud-1-prep.png")
            } else if (step === 3) {   // 2. Kneeling Match (perf block + advance)
                mock.stageLabel = "KNEELING · MATCH"
                mock.stageId = 3; mock.stepIndex = 1
                mock.windowState = 2
                mock.remainingFormatted = "18:43"
                mock.shotsInStage = 3
                mock.stageSubtotal = 31.4; mock.cumulativeTotal = 31.4
                mock.advanceLabel = "CHANGE TO PRONE — SIGHTERS"
            } else if (step === 4) { snap("finals-hud-2-kneeling.png")
            } else if (step === 5) {   // 3. Standing Series 1, START overlay live
                mock.stageLabel = "STANDING · SERIES 1"
                mock.stageId = 7; mock.stepIndex = 4
                mock.windowState = 2
                mock.remainingFormatted = "03:18"
                mock.shotsInStage = 4
                mock.stageSubtotal = 42.8; mock.cumulativeTotal = 245.7
                mock.advanceLabel = ""
                mock.commandIssued({ typeName: "StartSeries", text: "START" })
            } else if (step === 6) { snap("finals-hud-3-series.png")
            } else if (step === 7) {   // 4. Standing Single 33 + incident toast
                mock.stageLabel = "SINGLE SHOT 33"
                mock.stageId = 11; mock.stepIndex = 8
                mock.windowState = 2
                mock.remainingFormatted = "00:46"
                mock.shotsInStage = 0
                mock.stageSubtotal = 0; mock.cumulativeTotal = 312.6
                mock.shotRejected({ reason: "StageShotLimitReached" })
            } else if (step === 8) { snap("finals-hud-4-single.png")
            } else if (step === 10) { Qt.quit() }
        }
    }
}
