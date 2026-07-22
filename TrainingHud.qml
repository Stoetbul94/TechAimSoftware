import QtQuick 2.15

// Training Lab (T1) — Technical Blocks session overlay.
// Every visible value binds to TRAINING controller projections; no score or
// impact is computed or cached here beyond what the projections expose.
// Competition components are gated off by ShootingPage while this HUD is
// active, so hidden data cannot leak through legacy panels.
Item {
    id: hud
    property var ctl: null                    // TRAINING
    signal homeRequested()
    signal newSessionRequested()

    readonly property bool sightersOpen: ctl && ctl.phase === 1
    readonly property bool blockActive:  ctl && ctl.phase === 2
    readonly property bool reviewOpen:   ctl && ctl.phase === 3
    readonly property bool summaryOpen:  ctl && ctl.phase === 4
    // Live/Demo + hardware connection (for the Sighters readiness panel).
    property bool connected: false
    readonly property bool demoMode: ctl && ctl.sessionOperatingMode === "Demo"

    readonly property color _bg:     "#15171C"
    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _green:  "#20C997"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    // T1.4: the persistent block/shot/focus status + the SIGHTERS control moved
    // to TrainingRightPanel (target-first — nothing persistent over the target).
    // TrainingHud now owns only the transient acknowledgement, the full-workspace
    // Block Review and the final Summary.

    // Mode A acknowledgement: confirms a shot WITHOUT score or impact. Transient,
    // non-blocking — persistent status lives in TrainingRightPanel.
    Rectangle {
        id: ackToast
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 18
        width: ackText.implicitWidth + 40; height: 40; radius: 20
        color: "#DD1B1E24"; border.color: _line; border.width: 1
        Text { id: ackText; anchors.centerIn: parent; color: _txt; font.pixelSize: 14; font.bold: true }
        Timer { id: ackHide; interval: 1600; onTriggered: ackToast.visible = false }
        function show(msg) { ackText.text = msg; ackToast.visible = true; ackHide.restart() }
    }
    Connections {
        target: hud.ctl
        function onShotAccepted(rec) {
            if (hud.ctl && hud.ctl.visibilityMode === 0 && hud.blockActive)
                ackToast.show(rec.withinBlock + " / " + hud.ctl.shotsPerBlock + " RECORDED")
        }
    }

    // ── BLOCK REVIEW ─────────────────────────────────────────────────────
    Rectangle {
        visible: hud.reviewOpen
        anchors.fill: parent
        color: "#B8000000"
        MouseArea { anchors.fill: parent }    // modal: no click-through, no shots UI
        Rectangle {
            width: Math.min(640, parent.width - 60); height: Math.min(560, parent.height - 40)
            anchors.centerIn: parent
            color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22
                contentHeight: reviewCol.implicitHeight; clip: true
                Column {
                    id: reviewCol
                    width: parent.width; spacing: 10
                    property var m: (hud.reviewOpen && hud.ctl)
                                    ? hud.ctl.blockReviewMetrics(hud.ctl.currentBlock) : ({})

                    Text { text: "BLOCK " + (hud.ctl ? hud.ctl.currentBlock : "") + " COMPLETE"
                           color: _txt; font.pixelSize: 20; font.bold: true }
                    Text { visible: hud.ctl && hud.ctl.positionName !== ""
                           text: hud.ctl ? hud.ctl.positionName : ""; color: _green; font.pixelSize: 13; font.bold: true }
                    Text { text: "MEASURED RESULTS"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
                    Text { text: "Counted block shots only · sighters excluded"; color: _txtMut; font.pixelSize: 9 }

                    // Target plot: the block's shots revealed on a target-like
                    // view (auto-scaled to the group so shape/spread are clear;
                    // absolute radius/diameter are in the metrics below).
                    Rectangle {
                        width: 220; height: 220; radius: 6
                        color: "#0C0E12"; border.color: _line; border.width: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        property var shots: (hud.reviewOpen && hud.ctl)
                                            ? hud.ctl.blockShotPlot(hud.ctl.currentBlock) : []
                        // half-range in mm (min 8mm so a tight group isn't over-magnified)
                        property real rangeMm: {
                            var mx = 8
                            for (var i = 0; i < shots.length; ++i)
                                mx = Math.max(mx, Math.abs(shots[i].xMm), Math.abs(shots[i].yMm))
                            return mx * 1.25
                        }
                        readonly property real plotR: width / 2 - 10
                        // reference rings
                        Repeater {
                            model: [1.0, 0.6, 0.3]
                            Rectangle {
                                width: parent.plotR * 2 * modelData; height: width; radius: width / 2
                                anchors.centerIn: parent
                                color: "transparent"; border.color: "#23262d"; border.width: 1
                            }
                        }
                        // crosshair
                        Rectangle { anchors.centerIn: parent; width: parent.plotR * 2; height: 1; color: "#1c1f26" }
                        Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR * 2; color: "#1c1f26" }
                        // shot dots (index shown, newest brightest)
                        Repeater {
                            model: parent.shots
                            Rectangle {
                                width: 9; height: 9; radius: 5
                                color: index === parent.parent.shots.length - 1 ? "#ff2d55" : "#e8003d"
                                x: parent.width / 2 + (modelData.xMm / parent.rangeMm) * parent.plotR - width / 2
                                y: parent.height / 2 - (modelData.yMm / parent.rangeMm) * parent.plotR - height / 2
                                border.color: "#ffffff"; border.width: 1
                            }
                        }
                        Text {
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "±" + Number(parent.rangeMm).toFixed(0) + " mm"
                            color: _txtMut; font.pixelSize: 9
                        }
                    }

                    Grid {
                        columns: 3; columnSpacing: 14; rowSpacing: 8; width: parent.width
                        Repeater {
                            model: {
                                var m = reviewCol.m
                                if (!m || m.shotCount === undefined) return []
                                return [
                                    { l: "Shots",            v: m.shotCount },
                                    { l: "Average score",    v: Number(m.averageScore).toFixed(1) },
                                    { l: "Score std dev",    v: Number(m.scoreStdDev).toFixed(2) },
                                    { l: "Group radius",     v: m.hasGroup ? Number(m.groupRadius).toFixed(1) + " mm" : "—" },
                                    { l: "Group diameter",   v: m.hasGroup ? Number(m.groupDiameter).toFixed(1) + " mm" : "—" },
                                    { l: "MPI X / Y",        v: Number(m.mpiX).toFixed(1) + " / " + Number(m.mpiY).toFixed(1) + " mm" },
                                    { l: "H spread",         v: m.hasGroup ? Number(m.horizontalSpread).toFixed(1) + " mm" : "—" },
                                    { l: "V spread",         v: m.hasGroup ? Number(m.verticalSpread).toFixed(1) + " mm" : "—" },
                                    { l: "Avg shot time",    v: m.hasTiming ? Number(m.averageShotTime).toFixed(1) + " s" : "—" }
                                ]
                            }
                            delegate: Column {
                                width: (reviewCol.width - 28) / 3; spacing: 2
                                Text { text: modelData.l; color: _txtMut; font.pixelSize: 10 }
                                Text { text: "" + modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 15; font.bold: true }
                            }
                        }
                    }

                    Text { text: "SESSION FOCUS"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Text { text: hud.ctl ? hud.ctl.technicalFocus : ""; color: _txt; font.pixelSize: 13; font.bold: true }

                    Text { text: "ATHLETE NOTE"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Rectangle {
                        width: parent.width; height: 66; radius: 6
                        color: "#1D2026"; border.color: noteInput.activeFocus ? _red : _line; border.width: 1
                        TextEdit {
                            id: noteInput
                            anchors.fill: parent; anchors.margins: 8
                            color: _txt; font.pixelSize: 12; wrapMode: TextEdit.Wrap
                            // reasonable length limit
                            onTextChanged: if (length > 300) remove(300, length)
                        }
                    }
                    Row {
                        spacing: 8
                        Rectangle {
                            width: 110; height: 34; radius: 6
                            color: "#2B2C33"; border.color: _line; border.width: 1
                            Text { id: saveNoteLabel; anchors.centerIn: parent; text: "Save Note"; color: _txt; font.pixelSize: 12 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (hud.ctl && hud.ctl.saveNote(noteInput.text)) {
                                        saveNoteLabel.text = "Saved ✓"
                                        savedReset.restart()
                                    }
                                }
                            }
                            Timer { id: savedReset; interval: 1600; onTriggered: saveNoteLabel.text = "Save Note" }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "One note per block · saved durably"
                            color: _txtMut; font.pixelSize: 10
                        }
                    }

                    Row {
                        spacing: 10; topPadding: 8
                        Rectangle {
                            width: 190; height: 44; radius: 8; color: _red
                            Text {
                                anchors.centerIn: parent; color: "white"; font.pixelSize: 13; font.bold: true
                                text: hud.ctl && hud.ctl.currentBlock >= hud.ctl.blockCount
                                      ? "View Session Summary" : "Continue to Block " + (hud.ctl ? hud.ctl.currentBlock + 1 : "")
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (!hud.ctl) return
                                    if (noteInput.text.length > 0) hud.ctl.saveNote(noteInput.text)
                                    noteInput.text = ""
                                    hud.ctl.continueToNextBlock()   // double-clicks rejected by phase guard
                                }
                            }
                        }
                        Rectangle {
                            width: 130; height: 44; radius: 8
                            color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "End Training"; color: _txtSec; font.pixelSize: 12 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("End training?"),
                                    qsTr("Completed blocks are preserved. Uncompleted blocks are not counted."),
                                    function(ok) { if (ok && hud.ctl) { if (noteInput.text.length > 0) hud.ctl.saveNote(noteInput.text); noteInput.text = ""; hud.ctl.endTrainingEarly() } },
                                    qsTr("End Training"), qsTr("Keep Training"))
                            }
                        }
                    }
                }
            }
        }
    }

    // ── FINAL SUMMARY / on-screen TRAINING report ────────────────────────
    Rectangle {
        visible: hud.summaryOpen
        anchors.fill: parent
        color: "#B8000000"
        MouseArea { anchors.fill: parent }
        Rectangle {
            width: Math.min(720, parent.width - 50); height: Math.min(600, parent.height - 30)
            anchors.centerIn: parent
            color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22
                contentHeight: sumCol.implicitHeight; clip: true
                Column {
                    id: sumCol
                    width: parent.width; spacing: 10
                    property var blocks: (hud.summaryOpen && hud.ctl) ? hud.ctl.completedBlockSummaries() : []
                    property var cmp:    (hud.summaryOpen && hud.ctl) ? hud.ctl.finalComparison() : ({})

                    Text { text: (hud.ctl && hud.ctl.endedEarly()) ? "TRAINING ENDED" : "TECHNICAL BLOCKS COMPLETE"
                           color: _txt; font.pixelSize: 21; font.bold: true }
                    Text { text: "TRAINING SESSION"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
                    Rectangle {
                        width: notOfficial.implicitWidth + 22; height: 24; radius: 12
                        color: "#2a0b10"; border.color: _red; border.width: 1
                        Text { id: notOfficial; anchors.centerIn: parent
                               text: "Not an official competition result"
                               color: "#ffd0d7"; font.pixelSize: 10; font.bold: true }
                    }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        color: _txtSec; font.pixelSize: 11
                        text: (hud.ctl ? ("Session " + hud.ctl.sessionId.substring(0, 8)
                              + " · " + (hud.ctl.sessionOperatingMode || "Legacy") + " mode"
                              + " · Focus " + hud.ctl.technicalFocus
                              + " · " + ["Full hidden", "Group only", "Impact, no score"][hud.ctl.visibilityMode]) : "")
                    }

                    Text { text: "BLOCK COMPARISON"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    // header row + per-block rows
                    Column {
                        width: parent.width; spacing: 3
                        Row {
                            spacing: 8
                            Text { text: "BLOCK"; color: _txtMut; font.pixelSize: 9; width: 84 }
                            Text { text: "AVG"; color: _txtMut; font.pixelSize: 9; width: 46 }
                            Text { text: "DIA mm"; color: _txtMut; font.pixelSize: 9; width: 56 }
                            Text { text: "MPI mm"; color: _txtMut; font.pixelSize: 9; width: 84 }
                            Text { text: "SD"; color: _txtMut; font.pixelSize: 9; width: 44 }
                            Text { text: "NOTE"; color: _txtMut; font.pixelSize: 9 }
                        }
                        Repeater {
                            model: sumCol.blocks
                            delegate: Row {
                                spacing: 8
                                property var b: modelData
                                Text {
                                    width: 84; font.pixelSize: 11; color: _txt; font.bold: true
                                    text: "B" + b.blockIndex + (b.position !== "" && hud.ctl && hud.ctl.positionName !== undefined && b.position !== undefined && b.position !== "Kneeling" || (b.position && hud.ctl.blockCount === 6) ? " · " + b.position.substring(0, 1) : "")
                                }
                                Text { width: 46; font.family: "Consolas"; font.pixelSize: 11; color: _txt
                                       text: Number(b.averageScore).toFixed(1) }
                                Text { width: 56; font.family: "Consolas"; font.pixelSize: 11; color: _txt
                                       text: b.hasGroup ? Number(b.groupDiameter).toFixed(1) : "—" }
                                Text { width: 84; font.family: "Consolas"; font.pixelSize: 11; color: _txtSec
                                       text: Number(b.mpiX).toFixed(1) + "/" + Number(b.mpiY).toFixed(1) }
                                Text { width: 44; font.family: "Consolas"; font.pixelSize: 11; color: _txtSec
                                       text: Number(b.scoreStdDev).toFixed(2) }
                                Text { font.pixelSize: 10; color: _txtMut; elide: Text.ElideRight; width: 150
                                       text: b.note || "" }
                            }
                        }
                    }

                    Text { text: "SIGHTERS"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column {
                        spacing: 2
                        property var au: (hud.summaryOpen && hud.ctl) ? hud.ctl.sighterAudit() : ({})
                        Text {
                            color: _txtSec; font.pixelSize: 11
                            text: {
                                var a = parent.au
                                if (!a || a.total === undefined) return "Sighters: 0"
                                if (a.threePositions)
                                    return "Sighters — Kneeling " + a.kneeling
                                         + ", Prone " + a.prone + ", Standing " + a.standing
                                         + "  (total " + a.total + ")"
                                return "Sighters: " + a.total
                            }
                        }
                        Text { text: "Sighters excluded from Training results."
                               color: _txtMut; font.pixelSize: 10 }
                    }

                    Text { text: "OBSERVED"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column {
                        spacing: 3
                        Text { visible: sumCol.cmp.bestScoreBlock > 0
                               text: "· Highest-score block: Block " + sumCol.cmp.bestScoreBlock
                               color: _txtSec; font.pixelSize: 11 }
                        Text { visible: sumCol.cmp.tightestGroupBlock > 0
                               text: "· Tightest-group block: Block " + sumCol.cmp.tightestGroupBlock
                               color: _txtSec; font.pixelSize: 11 }
                        Text { visible: sumCol.cmp.mostRepeatableBlock > 0
                               text: "· Most repeatable block: Block " + sumCol.cmp.mostRepeatableBlock
                               color: _txtSec; font.pixelSize: 11 }
                        Text { visible: sumCol.cmp.hasSizeChange === true
                               text: "· Group " + (sumCol.cmp.groupSizeChangePct < 0 ? "became " + Math.abs(sumCol.cmp.groupSizeChangePct).toFixed(0) + "% smaller"
                                                                                      : "grew " + Number(sumCol.cmp.groupSizeChangePct).toFixed(0) + "%")
                                     + " from the first to the last block"
                               color: _txtSec; font.pixelSize: 11 }
                        Text { visible: sumCol.cmp.hasDrift === true && sumCol.cmp.centreDriftMm >= 0.5
                               text: "· Group centre moved " + Number(sumCol.cmp.centreDriftMm).toFixed(1) + " mm ("
                                     + (sumCol.cmp.centreDriftX >= 0 ? "right" : "left") + "/"
                                     + (sumCol.cmp.centreDriftY >= 0 ? "up" : "down") + ")"
                               color: _txtSec; font.pixelSize: 11 }
                        Text { visible: sumCol.blocks.length === 0
                               text: "No completed blocks."
                               color: _txtMut; font.pixelSize: 11 }
                    }

                    Row {
                        spacing: 10; topPadding: 10
                        Rectangle {
                            width: 150; height: 44; radius: 8; color: _red
                            Text { anchors.centerIn: parent; text: "New Session"; color: "white"; font.pixelSize: 13; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("Start a new training session?"),
                                    qsTr("This session is closed and preserved. A new session ID is created."),
                                    function(ok) { if (ok) hud.newSessionRequested() },
                                    qsTr("New Session"), qsTr("Cancel"))
                            }
                        }
                        Rectangle {
                            width: 120; height: 44; radius: 8
                            color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "Home"; color: _txtSec; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent; onClicked: hud.homeRequested() }
                        }
                    }
                }
            }
        }
    }
}
