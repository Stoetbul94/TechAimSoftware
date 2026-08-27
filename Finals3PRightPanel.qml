import QtQuick 2.15

// 50 m RIFLE 3 POSITIONS FINAL — right-hand information column.
//
// The 3P equivalent of Finals10mRightPanel: the same information architecture
// (command/countdown/last shot on top, shot history in the middle, score
// summary at the bottom) because the operator asked for that structure to be
// kept — and a completely different competition model underneath it, because
// the two disciplines share nothing but a shape.
//
// EVERY value comes from FINALS3P. This file must never read FINALS10M, and
// must never state a 10 m course fact (24 shots, Series 1 / Series 2 /
// 14 singles, "10m Air Rifle Final"). FINALS-3P-MIX-001 was exactly that
// contamination arriving through a stale flag; `tests/qml` now asserts the
// separation from both directions.
//
// Colours come from the semantic token layer (UI-DEC-016), not from literals,
// so the panel follows Light/Dark without a second code path. The 10 m panel
// still carries its own hex literals; migrating it is separate work and is not
// done here to keep this change reviewable.
//
// Course model (docs/3p-finals-discipline.md), from Finals3PTypes.h::Stage:
//   Ceremony -> KneelingPrepSight -> KneelingMatch (1-10)
//   -> ProneSighting -> ProneMatch (11-20)
//   -> StandingSighting -> StandingSeries1 (21-25) -> StandingSeries2 (26-30)
//   -> StandingSingle1..5 (31-35) -> Complete
Item {
    id: rp

    property var ctl                    // FINALS3P — the sole authority
    // ── tokens ───────────────────────────────────────────────────────────
    readonly property color _bg:     theme.tokens.surfacePrimary
    readonly property color _bgAlt:  theme.tokens.surfaceSecondary
    readonly property color _raised: theme.tokens.surfaceElevated
    readonly property color _line:   theme.tokens.borderSubtle
    readonly property color _accent: theme.tokens.accentPrimary
    readonly property color _txt:    theme.tokens.textPrimary
    readonly property color _txtSec: theme.tokens.textSecondary
    readonly property color _txtMut: theme.tokens.textDisabled
    readonly property color _ok:     theme.tokens.successText
    readonly property color _warn:   theme.tokens.warningText

    readonly property int _COMPLETE: 14      // Stage::Complete
    readonly property int _ABORTED:  15      // Stage::Aborted

    function fmt1(v) { return (v === undefined || v === null) ? "—" : Number(v).toFixed(1) }

    // The shot record carries `finalsPosition` as a ROLE (Finals3PTypes.h
    // positionRoleFor: 0 kneeling, 1 prone, 2 standing) chosen for
    // compatibility with the qualification models - not as a display string.
    // Rendering it raw prints "0". Found by driving the real controller
    // through the panel, not by reading the header.
    function positionName(role) {
        switch (Number(role)) {
        case 0:  return qsTr("KNEELING")
        case 1:  return qsTr("PRONE")
        case 2:  return qsTr("STANDING")
        default: return ""
        }
    }

    // ── shot projection ──────────────────────────────────────────────────
    // Appended from the durable shotAccepted signal, never inferred from
    // target dots. Sighters are kept and marked, never silently dropped and
    // never renumbered into the official sequence: the record carries explicit
    // phase ownership (isSighter, finalsPosition, finalsShotNumber) and this
    // panel reads that rather than guessing from order.
    ListModel { id: shotModel }

    property int    lastNumber:   0
    property string lastScore:    ""
    property int    lastTimeSec:  -1
    property string lastPosition: ""
    property bool   lastWasSighter: false

    readonly property string disciplineName: qsTr("50 m Rifle 3 Positions Final")

    readonly property string historyHeading: {
        if (!ctl) return qsTr("SHOTS")
        switch (ctl.stageId) {
        case 3:  return qsTr("KNEELING · SHOTS 1–10")
        case 5:  return qsTr("PRONE · SHOTS 11–20")
        case 7:  return qsTr("STANDING · SERIES 1 (21–25)")
        case 8:  return qsTr("STANDING · SERIES 2 (26–30)")
        case 9:  case 10: case 11: case 12: case 13:
                 return qsTr("STANDING · SINGLE SHOTS (31–35)")
        case rp._COMPLETE: return qsTr("FINAL — ALL 35 SHOTS")
        default: return qsTr("SIGHTERS")
        }
    }

    // Position subtotals are held as STATE refreshed on totalsChanged, not as
    // a binding that calls stageSubtotals(). stageSubtotals() is a Q_INVOKABLE,
    // not a NOTIFYing property, so a binding on it evaluates once when the
    // delegate is created - with the map still empty - and never re-evaluates.
    // The panel then shows 0.0 for the whole Final. Caught by driving the real
    // controller through the real panel; reading the source would not show it.
    property var subtotals: ({ "KNEELING": "0.0", "PRONE": "0.0", "STANDING": "0.0" })

    function refreshSubtotals() {
        rp.subtotals = { "KNEELING": rp.positionSubtotal("KNEELING"),
                         "PRONE":    rp.positionSubtotal("PRONE"),
                         "STANDING": rp.positionSubtotal("STANDING") }
    }

    Connections {
        target: rp.ctl
        function onTotalsChanged() { rp.refreshSubtotals() }
        function onPhaseChanged()  { rp.refreshSubtotals() }
        function onShotAccepted(shot) {
            var sighter = shot.isSighter === true
            shotModel.append({
                num:      sighter ? 0 : (shot.finalsShotNumber === undefined ? 0 : shot.finalsShotNumber),
                score:    shot.calculatedscore === undefined ? "" : String(shot.calculatedscore),
                sighter:  sighter,
                timeSec:  shot.timeComsumed === undefined ? -1 : shot.timeComsumed,
                position: rp.positionName(shot.finalsPosition)
            })
            // "Last OFFICIAL shot" means official. A sighter updates the list
            // but must not overwrite the last official shot readout.
            if (!sighter) {
                rp.lastNumber     = shot.finalsShotNumber === undefined ? 0 : shot.finalsShotNumber
                rp.lastScore      = shot.calculatedscore === undefined ? "" : String(shot.calculatedscore)
                rp.lastTimeSec    = shot.timeComsumed === undefined ? -1 : shot.timeComsumed
                rp.lastPosition   = rp.positionName(shot.finalsPosition)
                rp.lastWasSighter = false
            }
            shotList.positionViewAtEnd()
        }
    }

    function reset() {
        shotModel.clear()
        rp.lastNumber = 0; rp.lastScore = ""; rp.lastTimeSec = -1
        rp.lastPosition = ""; rp.lastWasSighter = false
        rp.refreshSubtotals()
    }

    Rectangle { anchors.fill: parent; color: rp._bgAlt }

    // ── 1. discipline + position + phase + timer + command ───────────────
    Rectangle {
        id: head
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.margins: 8
        height: headCol.implicitHeight + 20
        color: rp._bg
        radius: 6
        border.width: 1
        border.color: rp._line

        Column {
            id: headCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 10
            spacing: 6

            Text {
                text: rp.disciplineName
                color: rp._accent
                font.pixelSize: 12; font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }

            // POSITION — the single most important fact on a 3P screen.
            Text {
                objectName: "positionLabel"
                text: rp.ctl ? rp.ctl.positionLabel : ""
                color: rp._txt
                font.pixelSize: 26; font.bold: true; font.letterSpacing: 2
            }

            Text {
                objectName: "stageLabel"
                text: rp.ctl ? rp.ctl.stageLabel : ""
                color: rp._txtSec
                font.pixelSize: 12
                elide: Text.ElideRight
                width: parent.width
            }

            // THE authoritative clock. There is exactly one, and it is this
            // controller's. FINALS-TIMER-001 / FINALS-DISPLAY-TIMER-002.
            Text {
                objectName: "finalsClock"
                text: rp.ctl ? rp.ctl.remainingFormatted : "--:--"
                color: (rp.ctl && rp.ctl.paused) ? rp._warn
                     : (rp.ctl && rp.ctl.isFiringWindowOpen) ? rp._ok : rp._txt
                font.pixelSize: 40; font.bold: true
            }

            Row {
                spacing: 6
                Rectangle {
                    width: 8; height: 8; radius: 4
                    anchors.verticalCenter: parent.verticalCenter
                    color: (rp.ctl && rp.ctl.paused) ? rp._warn
                         : (rp.ctl && rp.ctl.isFiringWindowOpen) ? rp._ok : rp._txtMut
                }
                Text {
                    objectName: "commandText"
                    text: (rp.ctl && rp.ctl.paused) ? qsTr("PAUSED")
                          : (rp.ctl ? rp.ctl.commandText : "")
                    color: rp._txtSec
                    font.pixelSize: 12; font.bold: true
                }
            }

            // Position-change / primary action state, when the controller
            // offers one. Presentation of the controller's own label - this
            // panel never decides when a transition is allowed.
            Text {
                objectName: "advanceLabel"
                visible: rp.ctl && rp.ctl.primaryActionVisible
                         && rp.ctl.primaryActionLabel !== ""
                text: rp.ctl ? ("▸ " + rp.ctl.primaryActionLabel) : ""
                color: rp.ctl && rp.ctl.primaryActionEnabled ? rp._accent : rp._txtMut
                font.pixelSize: 11; font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }
        }
    }

    // ── 2. last official shot ────────────────────────────────────────────
    Rectangle {
        id: lastShot
        anchors.top: head.bottom; anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 8; anchors.rightMargin: 8; anchors.topMargin: 6
        height: 52
        color: rp._bg
        radius: 6
        border.width: 1
        border.color: rp._line

        Text {
            anchors.left: parent.left; anchors.leftMargin: 10
            anchors.top: parent.top; anchors.topMargin: 7
            text: qsTr("LAST OFFICIAL SHOT")
            color: rp._txtMut; font.pixelSize: 9; font.letterSpacing: 1.5
        }
        Text {
            objectName: "lastShotValue"
            anchors.left: parent.left; anchors.leftMargin: 10
            anchors.bottom: parent.bottom; anchors.bottomMargin: 7
            text: rp.lastNumber > 0
                  ? (qsTr("Shot ") + rp.lastNumber + " · " + rp.lastScore
                     + (rp.lastPosition !== "" ? " · " + rp.lastPosition : ""))
                  : "—"
            color: rp._txt; font.pixelSize: 14; font.bold: true
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 10
            anchors.bottom: parent.bottom; anchors.bottomMargin: 7
            text: rp.lastTimeSec >= 0 ? (rp.lastTimeSec + qsTr(" s")) : "—"
            color: rp._txtSec; font.pixelSize: 12
        }
    }

    // ── 4. score summary (bottom, fixed) ─────────────────────────────────
    Rectangle {
        id: summary
        anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
        anchors.margins: 8
        height: sumCol.implicitHeight + 20
        color: rp._bg
        radius: 6
        border.width: 1
        border.color: rp._line

        Column {
            id: sumCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 10
            spacing: 8

            Row {
                width: parent.width
                Text {
                    text: qsTr("SCORE SUMMARY"); color: rp._txtMut
                    font.pixelSize: 9; font.letterSpacing: 2
                }
                Item { width: parent.width - 150; height: 1 }
                // Course progress. 35, never 24.
                Text {
                    objectName: "courseProgress"
                    text: (rp.ctl ? rp.ctl.officialShotCount : 0) + qsTr(" / 35 shots")
                    color: rp._txtMut; font.pixelSize: 10
                }
            }

            Row {
                width: parent.width
                spacing: 0
                Repeater {
                    model: [ { k: qsTr("KNEELING"), s: "KNEELING" },
                             { k: qsTr("PRONE"),    s: "PRONE"    },
                             { k: qsTr("STANDING"), s: "STANDING" } ]
                    Column {
                        width: parent.width / 3
                        spacing: 2
                        Text {
                            text: modelData.k
                            color: (rp.ctl && rp.ctl.positionLabel === modelData.s)
                                   ? rp._accent : rp._txtMut
                            font.pixelSize: 9; font.letterSpacing: 1
                        }
                        Text {
                            objectName: "subtotal_" + modelData.s
                            text: rp.subtotals[modelData.s]
                            color: rp._txt; font.pixelSize: 15; font.bold: true
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: rp._line }

            Row {
                width: parent.width
                Text {
                    text: qsTr("TOTAL"); color: rp._txtMut
                    font.pixelSize: 10; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { width: parent.width - 120; height: 1 }
                Text {
                    objectName: "overallTotal"
                    text: rp.fmt1(rp.ctl ? rp.ctl.cumulativeTotal : 0)
                    color: rp._accent; font.pixelSize: 20; font.bold: true
                }
            }
        }
    }

    // Position subtotals come from the controller's own per-stage map, summed
    // across the stages that belong to a position (Standing has four scoring
    // stages plus five singles). The panel does no scoring: it adds up numbers
    // the controller already decided.
    function positionSubtotal(pos) {
        if (!ctl) return "0.0"
        var m = ctl.stageSubtotals()
        var sum = 0
        for (var k in m) {
            var key = String(k).toUpperCase()
            if (pos === "KNEELING" && key.indexOf("KNEELING") >= 0) sum += Number(m[k])
            else if (pos === "PRONE" && key.indexOf("PRONE") >= 0) sum += Number(m[k])
            else if (pos === "STANDING" && key.indexOf("STANDING") >= 0) sum += Number(m[k])
        }
        return sum.toFixed(1)
    }

    // ── 3. shot history (middle, flexible) ───────────────────────────────
    Rectangle {
        anchors.top: lastShot.bottom; anchors.bottom: summary.top
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 8; anchors.rightMargin: 8
        anchors.topMargin: 6; anchors.bottomMargin: 6
        color: rp._bg
        radius: 6
        border.width: 1
        border.color: rp._line

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Row {
                width: parent.width
                Text {
                    objectName: "historyHeading"
                    text: rp.historyHeading; color: rp._txtMut
                    font.pixelSize: 9; font.letterSpacing: 1.5
                }
                Item { width: parent.width - 170; height: 1 }
                Text { text: qsTr("Score"); color: rp._txtMut; font.pixelSize: 10 }
            }

            ListView {
                id: shotList
                width: parent.width
                height: parent.height - 24
                clip: true
                model: shotModel
                delegate: Rectangle {
                    width: shotList.width
                    height: 22
                    color: model.sighter ? rp._raised : "transparent"
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.sighter ? qsTr("SIGHTER") : String(model.num)
                        color: model.sighter ? rp._txtMut : rp._txtSec
                        font.pixelSize: model.sighter ? 9 : 12
                        font.letterSpacing: model.sighter ? 1 : 0
                    }
                    Text {
                        anchors.right: parent.right; anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.score
                        color: model.sighter ? rp._txtMut : rp._txt
                        font.pixelSize: 12
                        font.bold: !model.sighter
                    }
                }
            }
        }
    }

}
