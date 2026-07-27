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
    signal exportPdfRequested()               // T1.4: Training PDF export

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

    // ── BLOCK REVIEW (full-workspace) ────────────────────────────────────
    Rectangle {
        visible: hud.reviewOpen
        anchors.fill: parent
        color: "#EA0F1116"
        MouseArea { anchors.fill: parent }    // modal: no click-through, no shots UI
        Rectangle {
            width: Math.min(1180, parent.width - 40); height: Math.min(820, parent.height - 30)
            anchors.centerIn: parent
            color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22
                contentHeight: reviewCol.implicitHeight; clip: true
                Column {
                    id: reviewCol
                    width: parent.width; spacing: 12
                    property var m: (hud.reviewOpen && hud.ctl)
                                    ? hud.ctl.blockReviewMetrics(hud.ctl.currentBlock) : ({})
                    property var dlt: (hud.reviewOpen && hud.ctl)
                                    ? hud.ctl.blockDelta(hud.ctl.currentBlock) : ({})
                    property var obs: (hud.reviewOpen && hud.ctl)
                                    ? hud.ctl.blockObservations(hud.ctl.currentBlock) : []

                    Text { text: "BLOCK " + (hud.ctl ? hud.ctl.currentBlock : "") + " COMPLETE"
                           color: _txt; font.pixelSize: 24; font.bold: true }
                    Text { text: "MEASURED BLOCK RESULT"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
                    Row {
                        spacing: 10
                        Text { visible: hud.ctl && hud.ctl.positionName !== ""
                               text: hud.ctl ? hud.ctl.positionName : ""; color: _green; font.pixelSize: 13; font.bold: true }
                        Text { text: (reviewCol.m.shotCount !== undefined ? reviewCol.m.shotCount : "0")
                                     + " counted shots · sighters excluded"
                               color: _txtMut; font.pixelSize: 12 }
                        Text { text: hud.ctl ? ("Focus · " + hud.ctl.technicalFocus) : ""
                               color: _txtMut; font.pixelSize: 12 }
                    }

                    // LEFT target/group view + RIGHT metric cards
                    Row {
                        width: parent.width; spacing: 18

                        // — target/group view (≈58%) —
                        Column {
                            width: parent.width * 0.56; spacing: 6
                            Rectangle {
                                width: parent.width; height: Math.min(width, 420); radius: 8
                                color: "#0C0E12"; border.color: _line; border.width: 1
                                property var shots: (hud.reviewOpen && hud.ctl)
                                                    ? hud.ctl.blockShotPlot(hud.ctl.currentBlock) : []
                                property real rangeMm: {
                                    var mx = 8
                                    for (var i = 0; i < shots.length; ++i)
                                        mx = Math.max(mx, Math.abs(shots[i].xMm), Math.abs(shots[i].yMm))
                                    return mx * 1.25
                                }
                                readonly property real plotR: Math.min(width, height) / 2 - 16
                                Repeater {
                                    model: [1.0, 0.6, 0.3]
                                    Rectangle {
                                        width: parent.plotR * 2 * modelData; height: width; radius: width / 2
                                        anchors.centerIn: parent
                                        color: "transparent"; border.color: "#23262d"; border.width: 1
                                    }
                                }
                                Rectangle { anchors.centerIn: parent; width: parent.plotR * 2; height: 1; color: "#1c1f26" }
                                Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR * 2; color: "#1c1f26" }
                                // group-centre (MPI) marker
                                Rectangle {
                                    visible: reviewCol.m.hasGroup === true
                                    width: 14; height: 14; radius: 7; color: "transparent"
                                    border.color: _green; border.width: 2
                                    x: parent.width / 2 + (reviewCol.m.mpiX / parent.rangeMm) * parent.plotR - width / 2
                                    y: parent.height / 2 - (reviewCol.m.mpiY / parent.rangeMm) * parent.plotR - height / 2
                                }
                                Repeater {
                                    model: parent.shots
                                    Rectangle {
                                        width: 11; height: 11; radius: 6
                                        color: index === parent.parent.shots.length - 1 ? "#ff2d55" : "#e8003d"
                                        x: parent.width / 2 + (modelData.xMm / parent.rangeMm) * parent.plotR - width / 2
                                        y: parent.height / 2 - (modelData.yMm / parent.rangeMm) * parent.plotR - height / 2
                                        border.color: "#ffffff"; border.width: 1
                                    }
                                }
                                Text {
                                    anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "±" + Number(parent.rangeMm).toFixed(0) + " mm  ·  ● counted shot   ○ group centre"
                                    color: _txtMut; font.pixelSize: 10
                                }
                            }
                        }

                        // — primary metric cards (≈42%) —
                        Grid {
                            width: parent.width * 0.40; columns: 2
                            columnSpacing: 10; rowSpacing: 10
                            property var m: reviewCol.m
                            property var d: reviewCol.dlt
                            Repeater {
                                model: {
                                    var m = reviewCol.m, d = reviewCol.dlt
                                    if (!m || m.shotCount === undefined) return []
                                    function cmp(v, unit, better) {
                                        if (v === undefined || !d || !d.hasPrev) return ""
                                        if (Math.abs(v) < 0.05) return "Same as previous block"
                                        return "Block " + d.prevBlock + ": " + Math.abs(v).toFixed(1) + " " + unit
                                               + (v > 0 ? " more" : " less")
                                    }
                                    return [
                                        { l: "AVERAGE SCORE", v: Number(m.averageScore).toFixed(1), u: "",
                                          e: "Mean of the counted-shot scores.",
                                          c: cmp(d ? d.averageScoreDelta : undefined, "", true) },
                                        { l: "GROUP DIAMETER", v: m.hasGroup ? Number(m.groupDiameter).toFixed(1) : "—", u: "mm",
                                          e: "The largest distance between two shots.",
                                          c: cmp(d ? d.diameterDeltaMm : undefined, "mm", false) },
                                        { l: "GROUP CENTRE (MPI)",
                                          v: Math.abs(Number(m.mpiX)).toFixed(1) + (m.mpiX >= 0 ? " R" : " L")
                                             + " · " + Math.abs(Number(m.mpiY)).toFixed(1) + (m.mpiY >= 0 ? " Hi" : " Lo"),
                                          u: "mm",
                                          e: "Average centre of this block's group.", c: "" },
                                        { l: "SCORE VARIATION", v: Number(m.scoreStdDev).toFixed(2), u: "",
                                          e: "Lower means more consistent scores.",
                                          c: cmp(d ? d.scoreStdDevDelta : undefined, "", false) }
                                    ]
                                }
                                delegate: Rectangle {
                                    width: (parent.width - 10) / 2; height: 128; radius: 8
                                    color: "#1D2026"; border.color: _line; border.width: 1
                                    Column {
                                        anchors.fill: parent; anchors.margins: 10; spacing: 3
                                        Text { text: modelData.l; color: _txtMut; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
                                               width: parent.width; wrapMode: Text.WordWrap }
                                        Row { spacing: 3
                                            Text { text: modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 20; font.bold: true }
                                            Text { text: modelData.u; color: _txtSec; font.pixelSize: 11; anchors.bottom: parent.bottom; anchors.bottomMargin: 3 }
                                        }
                                        Text { text: modelData.e; color: _txtSec; font.pixelSize: 9; width: parent.width; wrapMode: Text.WordWrap }
                                        Text { visible: modelData.c !== ""; text: modelData.c; color: _green; font.pixelSize: 9; width: parent.width; wrapMode: Text.WordWrap }
                                    }
                                }
                            }
                        }
                    }

                    // secondary details
                    Grid {
                        columns: 4; columnSpacing: 14; rowSpacing: 8; width: parent.width
                        Repeater {
                            model: {
                                var m = reviewCol.m
                                if (!m || m.shotCount === undefined) return []
                                return [
                                    { l: "Shots",          v: m.shotCount },
                                    { l: "Group radius",   v: m.hasGroup ? Number(m.groupRadius).toFixed(1) + " mm" : "—" },
                                    { l: "H spread",       v: m.hasGroup ? Number(m.horizontalSpread).toFixed(1) + " mm" : "—" },
                                    { l: "V spread",       v: m.hasGroup ? Number(m.verticalSpread).toFixed(1) + " mm" : "—" },
                                    { l: "Avg shot time",  v: m.hasTiming ? Number(m.averageShotTime).toFixed(1) + " s" : "—" },
                                    { l: "Timing variation", v: m.hasTiming ? Number(m.shotTimeStdDev).toFixed(2) + " s" : "—" }
                                ]
                            }
                            delegate: Column {
                                width: (reviewCol.width - 42) / 4; spacing: 2
                                Text { text: modelData.l; color: _txtMut; font.pixelSize: 10 }
                                Text { text: "" + modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 14; font.bold: true }
                            }
                        }
                    }

                    // OBSERVED (measured statements only)
                    Column {
                        width: parent.width; spacing: 3
                        visible: reviewCol.obs.length > 0
                        Text { text: "OBSERVED"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                        Repeater {
                            model: reviewCol.obs
                            Text { width: reviewCol.width; wrapMode: Text.WordWrap
                                   text: "· " + modelData; color: _txtSec; font.pixelSize: 12 }
                        }
                    }

                    // GROUP PATTERN INSIGHTS (T3) — measured shape/movement + evidence
                    Rectangle {
                        width: parent.width; radius: 8; color: "#12161C"; border.color: _line; border.width: 1
                        property var gp: (hud.reviewOpen && hud.ctl) ? hud.ctl.groupPattern(hud.ctl.currentBlock) : ({})
                        visible: gp.hasData === true
                        height: gpCol.implicitHeight + 22
                        Column {
                            id: gpCol
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 11
                            spacing: 6
                            Text { text: "GROUP PATTERN INSIGHTS"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1 }
                            Repeater {
                                model: parent.parent.gp.properties || []
                                delegate: Column { width: gpCol.width; spacing: 1
                                    Row { spacing: 8
                                        Text { text: modelData.label; color: _green; font.pixelSize: 12; font.bold: true }
                                        Rectangle { width: cf.implicitWidth + 12; height: 16; radius: 8; color: "#0d2018"; border.color: _line; border.width: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text { id: cf; anchors.centerIn: parent; text: modelData.confidence + " evidence"; color: _txtSec; font.pixelSize: 8 } } }
                                    Text { width: gpCol.width; wrapMode: Text.WordWrap; text: modelData.evidence; color: _txtSec; font.pixelSize: 11 } }
                            }
                            Text { visible: (parent.parent.gp.prompt || "") !== ""
                                   width: gpCol.width; wrapMode: Text.WordWrap
                                   text: "Coach discussion: " + parent.parent.gp.prompt; color: _txt; font.pixelSize: 11; font.italic: true; topPadding: 2 }
                            Text { width: gpCol.width; wrapMode: Text.WordWrap
                                   text: parent.parent.gp.disclaimer || ""; color: _txtMut; font.pixelSize: 9 }
                        }
                    }

                    Text { text: "ATHLETE NOTE"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Text { text: "What did you notice during this block?"; color: _txtMut; font.pixelSize: 11 }
                    Rectangle {
                        width: parent.width; height: 96; radius: 6
                        color: "#1D2026"; border.color: noteInput.activeFocus ? _red : _line; border.width: 1
                        TextEdit {
                            id: noteInput
                            anchors.fill: parent; anchors.margins: 10
                            color: _txt; font.pixelSize: 13; wrapMode: TextEdit.Wrap
                            // reasonable length limit
                            onTextChanged: if (length > 300) remove(300, length)
                            // restore any durably-saved note when the review opens
                            Connections {
                                target: hud
                                function onReviewOpenChanged() {
                                    if (hud.reviewOpen && hud.ctl) {
                                        var mm = hud.ctl.blockReviewMetrics(hud.ctl.currentBlock)
                                        noteInput.text = (mm && mm.note) ? mm.note : ""
                                    }
                                }
                            }
                        }
                    }
                    Row {
                        spacing: 10
                        Rectangle {
                            width: 130; height: 40; radius: 8
                            color: saveNoteMouse.pressed ? "#3A3C44" : "#2B2C33"; border.color: _line; border.width: 1
                            Text { id: saveNoteLabel; anchors.centerIn: parent; text: "Save Note"; color: _txt; font.pixelSize: 13 }
                            MouseArea {
                                id: saveNoteMouse; anchors.fill: parent
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
                            text: (300 - noteInput.length) + " characters left · one note per block"
                            color: _txtMut; font.pixelSize: 10
                        }
                    }

                    // primary + secondary actions (touch: primary ≥ 56 px)
                    Row {
                        width: parent.width; spacing: 12; topPadding: 8
                        Rectangle {
                            width: parent.width * 0.62; height: 56; radius: 8
                            color: continueMouse.pressed ? "#A80038" : _red
                            Text {
                                anchors.centerIn: parent; color: "white"; font.pixelSize: 15; font.bold: true
                                text: hud.ctl && hud.ctl.currentBlock >= hud.ctl.blockCount
                                      ? "VIEW SESSION SUMMARY" : "CONTINUE TO BLOCK " + (hud.ctl ? hud.ctl.currentBlock + 1 : "")
                            }
                            MouseArea {
                                id: continueMouse; anchors.fill: parent
                                onClicked: {
                                    if (!hud.ctl) return
                                    if (noteInput.text.length > 0) hud.ctl.saveNote(noteInput.text)
                                    noteInput.text = ""
                                    hud.ctl.continueToNextBlock()   // double-clicks rejected by phase guard
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width * 0.34; height: 56; radius: 8
                            color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "End Training"; color: _txtSec; font.pixelSize: 13 }
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
            width: Math.min(1120, parent.width - 40); height: Math.min(840, parent.height - 30)
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
                    property var au:     (hud.summaryOpen && hud.ctl) ? hud.ctl.sighterAudit() : ({})
                    property var sobs:   (hud.summaryOpen && hud.ctl) ? hud.ctl.sessionObservations() : []
                    // max group diameter across blocks (for the comparison bar scale)
                    readonly property real maxDia: {
                        var mx = 1
                        for (var i = 0; i < blocks.length; ++i)
                            if (blocks[i].hasGroup) mx = Math.max(mx, blocks[i].groupDiameter)
                        return mx
                    }

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

                    // ── overview cards ────────────────────────────────────
                    Grid {
                        width: parent.width; columns: 6; columnSpacing: 10; rowSpacing: 10
                        Repeater {
                            model: {
                                if (!hud.ctl) return []
                                var d = hud.ctl.sessionDurationSec()
                                var dur = Math.floor(d / 60) + "m " + ("0" + (d % 60)).slice(-2) + "s"
                                var au = sumCol.au
                                var sight = (au && au.total !== undefined) ? au.total : 0
                                return [
                                    { l: "BLOCKS",     v: "" + hud.ctl.completedBlockCount() + "/" + hud.ctl.blockCount },
                                    { l: "COUNTED",    v: "" + hud.ctl.countedShotsTotal() },
                                    { l: "SIGHTERS",   v: "" + sight },
                                    { l: "DURATION",   v: dur },
                                    { l: "FOCUS",      v: hud.ctl.technicalFocus },
                                    { l: "VISIBILITY", v: ["Full hidden", "Group only", "Impact only"][hud.ctl.visibilityMode] }
                                ]
                            }
                            delegate: Rectangle {
                                width: (sumCol.width - 50) / 6; height: 62; radius: 8
                                color: "#1D2026"; border.color: _line; border.width: 1
                                Column {
                                    anchors.fill: parent; anchors.margins: 8; spacing: 3
                                    Text { text: modelData.l; color: _txtMut; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1 }
                                    Text { text: modelData.v; color: _txt; font.pixelSize: 14; font.bold: true
                                           width: parent.width; elide: Text.ElideRight }
                                }
                            }
                        }
                    }

                    // ── comparison bars (average score + group diameter) ──
                    Text { text: "GROUP SIZE BY BLOCK"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column {
                        width: parent.width; spacing: 4
                        Repeater {
                            model: sumCol.blocks
                            delegate: Row {
                                width: sumCol.width; spacing: 8
                                property var b: modelData
                                Text { text: "B" + b.blockIndex; color: _txtSec; font.pixelSize: 11; width: 30
                                       anchors.verticalCenter: parent.verticalCenter }
                                Rectangle {
                                    width: sumCol.width * 0.6; height: 14; radius: 4; color: "#0E1014"
                                    anchors.verticalCenter: parent.verticalCenter
                                    Rectangle {
                                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                        height: parent.height; radius: 4; color: "#e8003d"
                                        width: b.hasGroup ? parent.width * Math.min(1, b.groupDiameter / sumCol.maxDia) : 0
                                    }
                                }
                                Text { text: b.hasGroup ? Number(b.groupDiameter).toFixed(1) + " mm" : "—"
                                       color: _txt; font.family: "Consolas"; font.pixelSize: 11
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
                    }
                    Text { text: "AVERAGE SCORE BY BLOCK"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Column {
                        width: parent.width; spacing: 4
                        Repeater {
                            model: sumCol.blocks
                            delegate: Row {
                                width: sumCol.width; spacing: 8
                                property var b: modelData
                                Text { text: "B" + b.blockIndex; color: _txtSec; font.pixelSize: 11; width: 30
                                       anchors.verticalCenter: parent.verticalCenter }
                                Rectangle {
                                    width: sumCol.width * 0.6; height: 14; radius: 4; color: "#0E1014"
                                    anchors.verticalCenter: parent.verticalCenter
                                    Rectangle {
                                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                        height: parent.height; radius: 4; color: "#20C997"
                                        width: parent.width * Math.min(1, Number(b.averageScore) / 11.0)
                                    }
                                }
                                Text { text: Number(b.averageScore).toFixed(1)
                                       color: _txt; font.family: "Consolas"; font.pixelSize: 11
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
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
                        width: parent.width; spacing: 3
                        Repeater {
                            model: sumCol.sobs
                            Text { width: sumCol.width; wrapMode: Text.WordWrap
                                   text: "· " + modelData; color: _txtSec; font.pixelSize: 12 }
                        }
                        Text { visible: sumCol.sobs.length === 0
                               text: sumCol.blocks.length === 0 ? "No completed blocks."
                                                                : "Not enough blocks to compare."
                               color: _txtMut; font.pixelSize: 12 }
                    }

                    Row {
                        width: parent.width; spacing: 10; topPadding: 12
                        // Export PDF (Training-specific report)
                        Rectangle {
                            width: 190; height: 56; radius: 8
                            color: pdfMouse.pressed ? "#2A2C34" : "#23252C"; border.color: _line; border.width: 1
                            Row { anchors.centerIn: parent; spacing: 8
                                Text { text: "⭳"; color: _green; font.pixelSize: 18; anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "EXPORT PDF"; color: _txt; font.pixelSize: 14; font.bold: true
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                            MouseArea { id: pdfMouse; anchors.fill: parent; onClicked: hud.exportPdfRequested() }
                        }
                        Rectangle {
                            width: 170; height: 56; radius: 8; color: newSessMouse.pressed ? "#A80038" : _red
                            Text { anchors.centerIn: parent; text: "NEW SESSION"; color: "white"; font.pixelSize: 14; font.bold: true }
                            MouseArea {
                                id: newSessMouse; anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("Start a new training session?"),
                                    qsTr("This session is closed and preserved. A new session ID is created."),
                                    function(ok) { if (ok) hud.newSessionRequested() },
                                    qsTr("New Session"), qsTr("Cancel"))
                            }
                        }
                        Rectangle {
                            width: 120; height: 56; radius: 8
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
