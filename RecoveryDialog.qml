import QtQuick 2.15
import QtQuick.Controls 2.15

// Session Reliability Layer (M3) — recovery dialog. Shown BEFORE LoginPage
// when the RecoveryCoordinator finds an unfinished match in Sessions/Current.
// View-only over the recovery metadata; all rebuild/replay happens in C++
// (Journal → Validator → Reducer). Resume is offered only for a resumable
// (Clean/Recoverable) session; otherwise the operator gets Discard + details.
Rectangle {
    id: root
    anchors.fill: parent
    color: "#e6000000"          // dim the app behind
    visible: false
    z: 2000

    // The list of candidate maps from FINALS3P.scanForRecovery().
    property var candidates: []
    readonly property var current: (candidates && candidates.length > 0)
                                   ? candidates[0] : null
    readonly property bool resumable: current ? current.resumable === true : false

    // disciplineId is the stable machine id (e.g. FINAL3P / AR10 / AP10 /
    // PRONE50); main.qml's recovery dispatcher selects the restorer from it.
    signal resumeRequested(string sessionId, string disciplineId)
    signal discardRequested(string sessionId)
    signal dismissed()

    function open(list) {
        candidates = list
        visible = (list && list.length > 0)
    }

    // Presentation helpers over the candidate's generic reducer metadata.
    function phaseName(id) {
        if (id === 1) return qsTr("Preparation")
        if (id === 2) return qsTr("Sighting")
        if (id === 3) return qsTr("Official match")
        return "—"
    }
    function remainingText(ms) {
        if (ms === undefined || ms < 0) return ""
        var totalSecs = Math.floor(ms / 1000)
        var m = Math.floor(totalSecs / 60)
        var s = totalSecs - m * 60
        return (m < 10 ? "0" + m : m) + ":" + (s < 10 ? "0" + s : s)
    }
    // Score format follows the discipline: AP10 is integer/full-ring, the
    // decimal disciplines show one decimal.
    function totalText(c) {
        if (!c) return ""
        return c.disciplineId === "AP10" ? String(Math.round(c.totalTenths / 10))
                                         : (c.totalTenths / 10.0).toFixed(1)
    }

    // block clicks to the app behind
    MouseArea { anchors.fill: parent; onClicked: {} }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 520
        height: contentCol.implicitHeight + 48
        radius: 12
        color: "#1b1c22"
        border.color: "#d0392b"
        border.width: 2

        Column {
            id: contentCol
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12

            Text {
                // T1.4: a Training session is never a "Match".
                text: (root.current && root.current.sessionKind === "Training")
                      ? qsTr("Unfinished Training Session")
                      : qsTr("Unfinished Match Found")
                color: "white"; font.pixelSize: 22; font.bold: true
            }
            Rectangle { width: parent.width; height: 1; color: "#333" }

            Grid {
                columns: 2; columnSpacing: 18; rowSpacing: 8
                visible: root.current !== null
                property var c: root.current
                // T1: a Training session is labelled TRAINING SESSION — never
                // "Match recovery" / "Final recovery" / an official event.
                Text { visible: parent.c && parent.c.sessionKind === "Training"
                       text: qsTr("Type"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { visible: parent.c && parent.c.sessionKind === "Training"
                       text: qsTr("TRAINING SESSION · Technical Blocks")
                             + (parent.c && parent.c.trainingBlock > 0
                                ? qsTr("  ·  Block %1").arg(parent.c.trainingBlock) : "")
                       color: "#c9a06a"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Discipline"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: parent.c ? (parent.c.disciplineLabel || "") : ""
                       color: "white"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Athlete"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: parent.c ? (parent.c.athlete || "—") : ""
                       color: "white"; font.pixelSize: 13; font.bold: true }
                // F10: the mode the session was RECORDED with — a Demo session
                // stays labelled Demo, a pre-F10 session shows Legacy; never
                // re-inferred from the current running mode.
                Text { text: qsTr("Mode"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: (typeof OPMODE !== "undefined" && parent.c)
                             ? OPMODE.sessionBadge(parent.c.operatingMode || "")
                             : (parent.c ? (parent.c.operatingMode || "LEGACY") : "")
                       color: (parent.c && parent.c.operatingMode === "Demo") ? "#ff9aa8"
                              : (parent.c && parent.c.operatingMode === "Live") ? "#8fe0a8" : "#c9a06a"
                       font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Shots"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: parent.c ? (parent.c.officialShots + " / " + parent.c.expectedShots) : ""
                       color: "white"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Total"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: root.totalText(parent.c)
                       color: "white"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Phase"); color: "#9aa0aa"; font.pixelSize: 13
                       visible: parent.c ? parent.c.phaseId > 0 : false }
                Text { text: parent.c ? root.phaseName(parent.c.phaseId) : ""
                       visible: parent.c ? parent.c.phaseId > 0 : false
                       color: "white"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Time left"); color: "#9aa0aa"; font.pixelSize: 13
                       visible: parent.c ? parent.c.remainingMs >= 0 : false }
                Text { text: parent.c ? root.remainingText(parent.c.remainingMs) : ""
                       visible: parent.c ? parent.c.remainingMs >= 0 : false
                       color: "white"; font.pixelSize: 13; font.bold: true }
                Text { text: qsTr("Saved"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: parent.c ? (parent.c.lastEventWallIso || parent.c.lastModifiedIso || "") : ""
                       color: "white"; font.pixelSize: 13 }
                Text { text: qsTr("Journal"); color: "#9aa0aa"; font.pixelSize: 13 }
                Text { text: parent.c ? (parent.c.recoveryClass || "") : ""
                       color: root.resumable ? "#43b581" : "#d0392b"
                       font.pixelSize: 13; font.bold: true }
            }

            Text {
                width: parent.width
                visible: !root.resumable && root.current !== null
                text: root.current
                      ? qsTr("This session cannot be resumed cleanly: ")
                        + (root.current.validationDetail || "")
                      : ""
                color: "#d0392b"; font.pixelSize: 12; wrapMode: Text.WordWrap
            }

            // Phase E: unresolved EST incident warning. Resume restores the
            // session, but official shots stay blocked at the controller until
            // the Jury workflow authorises resumption (no bypass).
            Rectangle {
                width: parent.width; radius: 8
                visible: root.current ? root.current.openIncident === true : false
                color: "#33261024"
                border.color: "#e8a13d"
                height: incidentWarnCol.implicitHeight + 16
                Column {
                    id: incidentWarnCol
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top; anchors.margins: 8; spacing: 3
                    Text {
                        text: qsTr("⚠ Unresolved EST incident")
                        color: "#e8a13d"; font.pixelSize: 13; font.bold: true
                    }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        color: "#e8ce9d"; font.pixelSize: 12
                        text: (root.current && root.current.incidentCreditDecision === 0
                               ? qsTr("Jury decision pending. ")
                               : (root.current && root.current.incidentCreditMs > 0
                                  ? qsTr("Time credit authorised. ") : qsTr("No allowance ruled. ")))
                              + qsTr("Official shots remain blocked after resume until the Jury workflow authorises official resumption.")
                    }
                }
            }

            Row {
                spacing: 12
                anchors.right: parent.right

                Rectangle {
                    width: 120; height: 40; radius: 8; color: "#2a2b33"
                    Text { anchors.centerIn: parent; text: qsTr("Discard")
                           color: "white"; font.pixelSize: 14 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (root.current)
                                root.discardRequested(root.current.sessionId)
                            root.visible = false
                            root.dismissed()
                        }
                    }
                }
                Rectangle {
                    width: 160; height: 40; radius: 8
                    color: root.resumable ? "#e8003d" : "#3a3b44"
                    Text { anchors.centerIn: parent
                           text: !root.resumable ? qsTr("Recovery Wizard")
                                 : ((root.current && root.current.sessionKind === "Training")
                                    ? qsTr("Resume Training") : qsTr("Resume Match"))
                           color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (root.resumable && root.current) {
                                // T1: a Training session must never enter a
                                // competition restorer — route it explicitly.
                                root.resumeRequested(root.current.sessionId,
                                    root.current.sessionKind === "Training"
                                        ? (root.current.trainingProgramId === "call_and_diagnose"
                                            ? "CALLDIAG" : "TRAINING")
                                        : (root.current.disciplineId || ""))
                                root.visible = false
                            }
                            // Recovery Wizard (non-resumable) is deferred; the
                            // detail text already explains why Resume is off.
                        }
                    }
                }
            }
        }
    }
}
