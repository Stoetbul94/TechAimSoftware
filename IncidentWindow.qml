import QtQuick 2.15
import QtQuick.Controls 2.15

// Phase E — EST incident / Jury workflow window (operator-facing).
//
// Hosts the full range-caused-incident workflow inside the FloatingWindow
// shell: raise → technical review → Jury decision (accepted duration,
// no-allowance OR time credit) → authorised recovery phases (5-min
// preparation, unlimited recovery sighting) → official resume → target
// reassignment / backup-score review → resolution — plus the per-session
// incident history (E5 summary). Every action button calls an INCIDENTS
// invokable that journals a typed event; this window renders projections
// only and is never the authoritative record. Guidance from the supplied
// EST rules (docs/issf-rules/est-malfunctions.md) is displayed, but every
// allowance requires the authorising official's explicit action — exactly
// 3:00 / 5:00 surface as boundary cases needing a manual Jury decision.
FloatingWindow {
    id: incidentWin
    title: qsTr("EST Incident — Range Control")
    subtitle: (typeof userName !== "undefined" && userName) ? userName : ""
    minW: 700; minH: 560
    scrollableContent: true
    contentNaturalHeight: contentCol.implicitHeight + 30

    // Refreshed projection of the open incident (empty when none).
    property var inc: ({})
    readonly property bool hasIncident: inc && inc.incidentId !== undefined
    property var history: []
    // Snapshotted on refresh — a bare INCIDENTS.remainingCompetitionMs() call
    // in a binding would never re-evaluate (no notifiable dependency).
    property real remainMs: 0

    function refresh() {
        inc = INCIDENTS.activeIncident()
        history = INCIDENTS.incidentHistory()
        remainMs = INCIDENTS.remainingCompetitionMs()
    }
    onAboutToOpen: refresh()
    Connections {
        target: INCIDENTS
        function onIncidentChanged() { incidentWin.refresh() }
        function onActionFailed(detail) {
            dialogManager.showError(qsTr("Incident Action Failed"), detail)
        }
    }
    // Live estimated-duration display while an incident is open.
    Timer {
        interval: 1000; running: incidentWin.opened && incidentWin.hasIncident; repeat: true
        onTriggered: incidentWin.refresh()
    }

    // ── presentation helpers (no raw enum ids in the UI) ─────────────────
    readonly property var typeNames: [
        qsTr("Target not registering"), qsTr("Continuous target fault"),
        qsTr("Scoring computer failure"), qsTr("Target network failure"),
        qsTr("Application crash"), qsTr("Computer crash"),
        qsTr("Power failure"), qsTr("Communication failure"),
        qsTr("Target move"), qsTr("Other range-caused failure")]
    readonly property var scopeNames: [
        qsTr("Individual firing point"), qsTr("Selected firing points"),
        qsTr("Entire relay / range")]
    function statusLabel(key) {
        if (key === undefined || key === null) return ""
        if (key === "awaiting-jury-decision")   return qsTr("Awaiting Jury decision")
        if (key === "awaiting-official-resume") return qsTr("Recovery authorised — awaiting official resume")
        if (key === "recovery-preparation")     return qsTr("Recovery preparation")
        if (key === "recovery-sighting")        return qsTr("Recovery sighting")
        if (key === "resumed")                  return qsTr("Resumed — awaiting resolution")
        if (key === "resolved")                 return qsTr("Resolved")
        if (key === "cancelled")                return qsTr("Cancelled")
        return key
    }
    function statusColor(key) {
        if (key === "resolved")  return "#43b581"
        if (key === "cancelled") return "#9aa0aa"
        if (key === "resumed")   return "#2e8bc0"
        return "#e8a13d"
    }
    function fmtMs(ms) {
        if (ms === undefined || ms === null || ms < 0) return "—"
        var s = Math.floor(ms / 1000); var m = Math.floor(s / 60); s -= m * 60
        return (m < 10 ? "0" + m : m) + ":" + (s < 10 ? "0" + s : s)
    }
    function guidance(key) {
        if (key === "under-3")
            return qsTr("Guidance (supplied rule 6.11.2): under 3 minutes — no additional time credit; the clock resumes from its frozen value.")
        if (key === "over-3")
            return qsTr("Guidance (supplied rule 6.11.2): over 3 minutes — the Jury may authorise time credit equal to the exact time lost.")
        if (key === "over-5")
            return qsTr("Guidance (supplied rule 6.11.2): over 5 minutes — when authorised: exact time credit, 5 minutes preparation and unlimited sighting shots before officials resume.")
        if (key === "exactly-3-boundary" || key === "exactly-5-boundary")
            return qsTr("BOUNDARY CASE — the treatment of exactly this duration has not been officially confirmed. The Jury must select the applicable action; the selected decision is journalled.")
        return ""
    }
    readonly property bool boundaryCase: hasIncident
        && (inc.boundaryKey === "exactly-3-boundary"
            || inc.boundaryKey === "exactly-5-boundary")

    // The authorising official for all authorised actions in this window.
    property alias officialName: officialField.text
    function requireOfficial() {
        if (officialField.text.trim().length > 0) return true
        dialogManager.showWarning(qsTr("Authorising Official Required"),
            qsTr("Enter the name of the authorising Range Officer or Jury member before recording an authorised action."))
        return false
    }

    Column {
        id: contentCol
        width: parent ? parent.width - 30 : 640
        x: 15; spacing: 12
        topPadding: 12; bottomPadding: 16

        // ═══ No open incident → creation form ═══
        Rectangle {
            visible: !incidentWin.hasIncident
            width: parent.width; radius: 10; color: "#1f2026"
            height: createCol.implicitHeight + 28
            Column {
                id: createCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 10
                Text { text: qsTr("Report an EST incident"); color: "white"
                       font.pixelSize: 17; font.bold: true }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap
                    color: "#9aa0aa"; font.pixelSize: 12
                    text: qsTr("For range-caused failures only (targets, network, scoring system, power). Raising an incident freezes the competition clock and blocks official shots until the Jury workflow authorises resumption. Athlete equipment malfunctions are handled separately.")
                }
                Grid {
                    columns: 2; columnSpacing: 12; rowSpacing: 8
                    width: parent.width
                    Text { text: qsTr("Incident type"); color: "#c9ced6"; font.pixelSize: 13 }
                    ComboBox { id: typeBox; width: 300; model: incidentWin.typeNames }
                    Text { text: qsTr("Scope"); color: "#c9ced6"; font.pixelSize: 13 }
                    ComboBox { id: scopeBox; width: 300; model: incidentWin.scopeNames }
                    Text { text: qsTr("Firing point(s)"); color: "#c9ced6"; font.pixelSize: 13 }
                    TextField { id: pointsField; width: 300
                                placeholderText: qsTr("e.g. 4  (or 2,5,7 for selected points)") }
                    Text { text: qsTr("Relay"); color: "#c9ced6"; font.pixelSize: 13 }
                    TextField { id: relayField; width: 300
                                placeholderText: qsTr("optional") }
                    Text { text: qsTr("Description"); color: "#c9ced6"; font.pixelSize: 13 }
                    TextField { id: descField; width: 300
                                placeholderText: qsTr("what happened") }
                }
                Button {
                    text: qsTr("Record incident and freeze clock")
                    onClicked: dialogManager.showConfirmation(
                        qsTr("Record EST Incident"),
                        qsTr("This journals the incident, freezes the competition clock and blocks official shots until the Jury workflow authorises resumption.\n\nProceed?"),
                        function (ok) {
                            if (!ok) return
                            INCIDENTS.raiseIncident(typeBox.currentIndex,
                                scopeBox.currentIndex, pointsField.text,
                                relayField.text, descField.text)
                        })
                }
            }
        }

        // ═══ Open incident → status + workflow ═══
        Rectangle {
            visible: incidentWin.hasIncident
            width: parent.width; radius: 10; color: "#1f2026"
            height: statusCol.implicitHeight + 28
            border.color: incidentWin.boundaryCase ? "#e8a13d" : "#3a3b40"
            Column {
                id: statusCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 8
                Row {
                    spacing: 10
                    Rectangle {
                        radius: 6; color: incidentWin.statusColor(inc.statusKey)
                        width: statusTxt.implicitWidth + 20; height: 26
                        Text { id: statusTxt; anchors.centerIn: parent
                               text: incidentWin.statusLabel(inc.statusKey)
                               color: "#15161a"; font.pixelSize: 13; font.bold: true }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: incidentWin.hasIncident
                              ? incidentWin.typeNames[inc.typeId] + "  ·  "
                                + incidentWin.scopeNames[inc.scopeId]
                                + (inc.firingPoint ? "  ·  " + qsTr("FP ") + inc.firingPoint : "")
                              : ""
                        color: "white"; font.pixelSize: 14; font.bold: true
                    }
                }
                Grid {
                    columns: 4; columnSpacing: 16; rowSpacing: 4
                    Text { text: qsTr("Interruption began"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text { text: inc.interruptionStartUtc || "—"; color: "white"; font.pixelSize: 12 }
                    Text { text: qsTr("Estimated duration"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text { text: incidentWin.fmtMs(inc.estimatedDurationMs); color: "white"
                           font.pixelSize: 12; font.bold: true }
                    Text { text: qsTr("Accepted duration"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text { text: incidentWin.fmtMs(inc.acceptedDurationMs); color: "white"
                           font.pixelSize: 12; font.bold: true }
                    Text { text: qsTr("Time credit"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text {
                        text: inc.creditDecision === 1
                              ? qsTr("None (ruled)")
                              : (inc.timeCreditMs > 0 ? incidentWin.fmtMs(inc.timeCreditMs)
                                                      : qsTr("Decision pending"))
                        color: inc.creditDecision === 0 ? "#e8a13d" : "white"
                        font.pixelSize: 12; font.bold: true
                    }
                    Text { text: qsTr("Match clock (frozen + credit)"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text { text: incidentWin.fmtMs(incidentWin.remainMs)
                           color: "white"; font.pixelSize: 12; font.bold: true }
                    Text { text: qsTr("Target moved"); color: "#9aa0aa"; font.pixelSize: 12 }
                    Text { text: inc.targetMoved
                                 ? (inc.originalTarget + " → " + inc.reserveTarget) : qsTr("No")
                           color: "white"; font.pixelSize: 12 }
                }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap
                    visible: text.length > 0
                    text: incidentWin.guidance(inc.boundaryKey)
                    color: incidentWin.boundaryCase ? "#e8a13d" : "#7ea6c9"
                    font.pixelSize: 12
                    font.bold: incidentWin.boundaryCase
                }
            }
        }

        // Authorising official (shared by every authorised action below).
        Rectangle {
            visible: incidentWin.hasIncident && inc.statusKey !== "resolved"
                     && inc.statusKey !== "cancelled"
            width: parent.width; radius: 10; color: "#1f2026"
            height: authRow.implicitHeight + 24
            Row {
                id: authRow
                anchors.left: parent.left; anchors.top: parent.top
                anchors.margins: 12; spacing: 10
                Text { text: qsTr("Authorising official"); color: "#c9ced6"
                       font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                TextField { id: officialField; width: 240
                            placeholderText: qsTr("Range Officer / Jury member name") }
            }
        }

        // ═══ Jury decision (E2) ═══
        Rectangle {
            visible: incidentWin.hasIncident && inc.statusKey !== "resolved"
                     && inc.statusKey !== "cancelled"
            width: parent.width; radius: 10; color: "#1f2026"
            height: juryCol.implicitHeight + 28
            Column {
                id: juryCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 10
                Text { text: qsTr("Jury decision"); color: "white"
                       font.pixelSize: 15; font.bold: true }
                Row {
                    spacing: 8
                    Text { text: qsTr("Official interruption duration (mm:ss)")
                           color: "#c9ced6"; font.pixelSize: 13
                           anchors.verticalCenter: parent.verticalCenter }
                    TextField { id: durField; width: 90; placeholderText: "04:00" }
                    Button {
                        text: qsTr("Record duration")
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            var p = durField.text.split(":")
                            var ms = (parseInt(p[0], 10) * 60 + (parseInt(p[1], 10) || 0)) * 1000
                            if (!(ms >= 0)) {
                                dialogManager.showWarning(qsTr("Invalid Duration"),
                                    qsTr("Enter the officially accepted duration as mm:ss."))
                                return
                            }
                            INCIDENTS.recordAcceptedDuration(ms, incidentWin.officialName, "")
                        }
                    }
                }
                Row {
                    spacing: 8
                    Button {
                        text: qsTr("No additional allowance")
                        enabled: inc.creditDecision === 0
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            dialogManager.showConfirmation(qsTr("No Additional Allowance"),
                                qsTr("Record the Jury's explicit decision that NO additional allowance is granted?\n\nThe competition clock will resume from its frozen value once official resumption is authorised."),
                                function (ok) { if (ok) INCIDENTS.recordNoAllowance(
                                    incidentWin.officialName, "") })
                        }
                    }
                    Text { text: qsTr("or credit (mm:ss)"); color: "#c9ced6"
                           font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                    TextField { id: creditField; width: 90; placeholderText: "04:00" }
                    Button {
                        text: qsTr("Grant time credit")
                        enabled: inc.creditDecision === 0
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            var p = creditField.text.split(":")
                            var ms = (parseInt(p[0], 10) * 60 + (parseInt(p[1], 10) || 0)) * 1000
                            if (!(ms > 0)) {
                                dialogManager.showWarning(qsTr("Invalid Credit"),
                                    qsTr("Enter the authorised time credit as mm:ss."))
                                return
                            }
                            dialogManager.showConfirmation(qsTr("Grant Time Credit"),
                                qsTr("Add %1 of Jury-authorised time credit to the frozen competition clock?\n\nThis is journalled with the authorising official.").arg(incidentWin.fmtMs(ms)),
                                function (ok) { if (ok) INCIDENTS.grantTimeCredit(
                                    ms, incidentWin.officialName, "") })
                        }
                    }
                }
            }
        }

        // ═══ Recovery phases + resume gate (E3) ═══
        Rectangle {
            visible: incidentWin.hasIncident && inc.statusKey !== "resolved"
                     && inc.statusKey !== "cancelled"
            width: parent.width; radius: 10; color: "#1f2026"
            height: phaseCol.implicitHeight + 28
            Column {
                id: phaseCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 10
                Text { text: qsTr("Recovery and resumption"); color: "white"
                       font.pixelSize: 15; font.bold: true }
                // Independent 5-minute preparation countdown (display only —
                // the match clock stays frozen; the authorisation is the
                // journalled fact).
                property int prepLeft: 300
                Timer {
                    id: prepTimer; interval: 1000; repeat: true
                    running: incidentWin.hasIncidented && inc.statusKey === "recovery-preparation"
                             && phaseCol.prepLeft > 0
                    onTriggered: phaseCol.prepLeft--
                }
                Row {
                    spacing: 8
                    Button {
                        text: qsTr("Begin 5-minute preparation")
                        enabled: !inc.preparationGranted
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            phaseCol.prepLeft = 300
                            INCIDENTS.beginRecoveryPreparation(incidentWin.officialName)
                        }
                    }
                    Text {
                        visible: inc.statusKey === "recovery-preparation"
                        text: qsTr("Preparation: %1 (match clock stays frozen)")
                              .arg(incidentWin.fmtMs(phaseCol.prepLeft * 1000))
                        color: "#e8a13d"; font.pixelSize: 13; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: 8
                    Button {
                        text: qsTr("Begin recovery sighting (unlimited)")
                        enabled: !inc.sightingGranted
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            INCIDENTS.beginRecoverySighting(incidentWin.officialName)
                        }
                    }
                    Text {
                        visible: inc.statusKey === "recovery-sighting"
                        text: qsTr("Recovery sighting active — sighters do not affect the official score")
                        color: "#e8a13d"; font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: 8
                    Button {
                        text: qsTr("Authorise official resumption")
                        enabled: !inc.officialResumeAuthorised
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            dialogManager.showConfirmation(qsTr("Authorise Official Resumption"),
                                inc.creditDecision === 0
                                ? qsTr("The Jury allowance decision is still PENDING (no credit and no explicit no-allowance ruling has been recorded).\n\nAuthorise official resumption anyway?")
                                : qsTr("Official shots will be accepted again and the competition clock resumes from the frozen value plus any authorised credit.\n\nProceed?"),
                                function (ok) { if (ok) INCIDENTS.authoriseOfficialResume(
                                    incidentWin.officialName) })
                        }
                    }
                    Text {
                        visible: inc.officialResumeAuthorised === true
                        text: qsTr("Official shots authorised")
                        color: "#43b581"; font.pixelSize: 12; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // ═══ Target reassignment + backup review ═══
        Rectangle {
            visible: incidentWin.hasIncident && inc.statusKey !== "resolved"
                     && inc.statusKey !== "cancelled"
            width: parent.width; radius: 10; color: "#1f2026"
            height: targetCol.implicitHeight + 28
            Column {
                id: targetCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 10
                Text { text: qsTr("Target reassignment and backup score"); color: "white"
                       font.pixelSize: 15; font.bold: true }
                Row {
                    spacing: 8
                    TextField { id: origTargetField; width: 130
                                placeholderText: qsTr("original target") }
                    TextField { id: reserveTargetField; width: 130
                                placeholderText: qsTr("reserve target") }
                    Button {
                        text: qsTr("Record move to reserve target")
                        onClicked: {
                            if (!incidentWin.requireOfficial()) return
                            if (reserveTargetField.text.trim().length === 0) {
                                dialogManager.showWarning(qsTr("Reserve Target Required"),
                                    qsTr("Enter the reserve target identifier."))
                                return
                            }
                            dialogManager.showConfirmation(qsTr("Confirm Target Reassignment"),
                                qsTr("Record the authorised move to the reserve target? Recovered shots, official count and numbering are unaffected."),
                                function (ok) { if (ok) INCIDENTS.reassignTarget(
                                    origTargetField.text, reserveTargetField.text,
                                    incidentWin.officialName, "") })
                        }
                    }
                }
                Row {
                    spacing: 8
                    Text { text: qsTr("Backup score review:"); color: "#c9ced6"
                           font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                    Button { text: qsTr("Accepted")
                             onClicked: if (incidentWin.requireOfficial())
                                 INCIDENTS.recordBackupReview(1, incidentWin.officialName, "") }
                    Button { text: qsTr("Rejected")
                             onClicked: if (incidentWin.requireOfficial())
                                 INCIDENTS.recordBackupReview(2, incidentWin.officialName, "") }
                    Button { text: qsTr("Inconclusive")
                             onClicked: if (incidentWin.requireOfficial())
                                 INCIDENTS.recordBackupReview(3, incidentWin.officialName, "") }
                    Text {
                        text: inc.backupReview === 1 ? qsTr("· accepted")
                            : inc.backupReview === 2 ? qsTr("· rejected")
                            : inc.backupReview === 3 ? qsTr("· inconclusive")
                            : qsTr("· not reviewed")
                        color: "#9aa0aa"; font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    width: parent.width; wrapMode: Text.WordWrap
                    color: "#9aa0aa"; font.pixelSize: 11
                    text: qsTr("A backup-score decision is recorded only — any score correction is a separate authorised action through the official correction workflow.")
                }
            }
        }

        // ═══ Resolution ═══
        Rectangle {
            visible: incidentWin.hasIncident && inc.statusKey !== "resolved"
                     && inc.statusKey !== "cancelled"
            width: parent.width; radius: 10; color: "#1f2026"
            height: closeCol.implicitHeight + 28
            Column {
                id: closeCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 10
                Text { text: qsTr("Close the incident"); color: "white"
                       font.pixelSize: 15; font.bold: true }
                Row {
                    spacing: 8
                    TextField { id: juryNoteField; width: 220; placeholderText: qsTr("Jury note") }
                    TextField { id: roNoteField; width: 220; placeholderText: qsTr("Range Officer note") }
                    TextField { id: refField; width: 130; placeholderText: qsTr("report ref.") }
                }
                Row {
                    spacing: 8
                    Button {
                        text: qsTr("Resolve incident")
                        onClicked: dialogManager.showConfirmation(qsTr("Resolve Incident"),
                            qsTr("Close this incident with the official record? This cannot be reopened."),
                            function (ok) { if (ok) INCIDENTS.resolveIncident(
                                juryNoteField.text, roNoteField.text, refField.text) })
                    }
                    Button {
                        text: qsTr("Cancel incident")
                        onClicked: dialogManager.showConfirmation(qsTr("Cancel Incident"),
                            qsTr("Cancel this incident (recorded as cancelled; the frozen clock is released without credit)?"),
                            function (ok) { if (ok) INCIDENTS.cancelIncident(
                                juryNoteField.text) })
                    }
                }
            }
        }

        // ═══ Session incident history (E5 summary) ═══
        Rectangle {
            visible: incidentWin.history.length > 0
            width: parent.width; radius: 10; color: "#1f2026"
            height: histCol.implicitHeight + 28
            Column {
                id: histCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 14; spacing: 6
                Text { text: qsTr("Incident history (this session)"); color: "white"
                       font.pixelSize: 15; font.bold: true }
                Repeater {
                    model: incidentWin.history
                    delegate: Text {
                        width: histCol.width; wrapMode: Text.WordWrap
                        color: "#c9ced6"; font.pixelSize: 12
                        text: "• " + incidentWin.typeNames[modelData.typeId]
                              + "  ·  " + incidentWin.scopeNames[modelData.scopeId]
                              + "  ·  " + incidentWin.statusLabel(modelData.statusKey)
                              + (modelData.timeCreditMs > 0
                                 ? "  ·  " + qsTr("credit ") + incidentWin.fmtMs(modelData.timeCreditMs)
                                 : (modelData.creditDecision === 1 ? "  ·  " + qsTr("no allowance") : ""))
                              + (modelData.targetMoved
                                 ? "  ·  " + modelData.originalTarget + "→" + modelData.reserveTarget : "")
                              + (modelData.authorisedBy
                                 ? "  ·  " + qsTr("by ") + modelData.authorisedBy : "")
                    }
                }
            }
        }
    }
}
