import QtQuick 2.15

// Position Transition (T4) — Position Review + final Summary overlay. During
// setup/sighters/verification only the transient ack shows (target dominant);
// the persistent controls live in PositionTransitionRightPanel. Measured
// language only — no technical-cause claims.
Item {
    id: hud
    property var ctl: null                 // POSTRANS
    signal homeRequested()
    signal newSessionRequested()
    signal exportPdfRequested()

    readonly property bool reviewOpen:  ctl && ctl.phase === 4
    readonly property bool summaryOpen: ctl && ctl.phase === 5

    readonly property color _bg:     "#15171C"
    readonly property color _card:   "#1B1E24"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    function ms(v) { return v === undefined ? "—" : (Math.floor(v/1000) + "." + Math.floor((v%1000)/100) + " s") }
    // seconds → "N.n s" for cadence values already expressed in seconds
    function secs(v) { return (v === undefined || v <= 0) ? "—" : (Number(v).toFixed(1) + " s") }
    function rhythmColor(label) {
        if (label === "Steady") return _green
        if (label === "Variable") return "#E8A13C"
        if (label === "Inconsistent") return _redHi
        return _txtMut
    }

    // ── POSITION REVIEW ──────────────────────────────────────────────────
    Rectangle {
        visible: hud.reviewOpen
        anchors.fill: parent; color: "#EA0F1116"
        MouseArea { anchors.fill: parent }
        Rectangle {
            width: Math.min(1160, parent.width - 40); height: Math.min(820, parent.height - 30)
            anchors.centerIn: parent; color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22; contentHeight: rvCol.implicitHeight; clip: true
                Column {
                    id: rvCol; width: parent.width; spacing: 12
                    property var r: (hud.reviewOpen && hud.ctl) ? hud.ctl.currentReview() : ({})
                    property var gp: (rvCol.r.groupPattern) ? rvCol.r.groupPattern : ({})

                    Text { text: (rvCol.r.positionName || "") + " REVIEW"; color: _txt; font.pixelSize: 24; font.bold: true }
                    Text { text: "Repeat " + (rvCol.r.repeat || 1) + " · " + (rvCol.r.verificationShots || 0) + " counted shots · sighters excluded"
                           color: _txtMut; font.pixelSize: 12 }

                    Row {
                        width: parent.width; spacing: 18
                        // target/group plot
                        Column {
                            width: parent.width * 0.5; spacing: 6
                            Rectangle {
                                width: parent.width; height: Math.min(width, 380); radius: 8
                                color: "#0C0E12"; border.color: _line; border.width: 1
                                property var shots: (hud.reviewOpen && hud.ctl) ? hud.ctl.verificationPlot(rvCol.r.position, rvCol.r.repeat) : []
                                property real rangeMm: {
                                    var mx = 8
                                    for (var i = 0; i < shots.length; ++i) mx = Math.max(mx, Math.abs(shots[i].xMm), Math.abs(shots[i].yMm))
                                    return mx * 1.25
                                }
                                readonly property real plotR: Math.min(width, height)/2 - 16
                                Repeater { model: [1.0,0.6,0.3]
                                    Rectangle { width: parent.plotR*2*modelData; height: width; radius: width/2
                                        anchors.centerIn: parent; color: "transparent"; border.color: "#23262d"; border.width: 1 } }
                                Rectangle { anchors.centerIn: parent; width: parent.plotR*2; height: 1; color: "#1c1f26" }
                                Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR*2; color: "#1c1f26" }
                                Rectangle { visible: rvCol.r.hasGroup === true; width: 14; height: 14; radius: 7; color: "transparent"; border.color: _green; border.width: 2
                                    x: parent.width/2 + (rvCol.r.mpiX/parent.rangeMm)*parent.plotR - 7
                                    y: parent.height/2 - (rvCol.r.mpiY/parent.rangeMm)*parent.plotR - 7 }
                                Repeater { model: parent.shots
                                    Rectangle { width: 11; height: 11; radius: 6
                                        color: modelData.first ? "#3DA9FC" : "#e8003d"; border.color: "white"; border.width: 1
                                        x: parent.width/2 + (modelData.xMm/parent.rangeMm)*parent.plotR - width/2
                                        y: parent.height/2 - (modelData.yMm/parent.rangeMm)*parent.plotR - height/2 } }
                                Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 6; anchors.horizontalCenter: parent.horizontalCenter
                                       text: "±" + Number(parent.rangeMm).toFixed(0) + " mm  ·  ● counted   ◆ first shot (blue)"
                                       color: _txtMut; font.pixelSize: 9 }
                            }
                        }
                        // timing + measured cards
                        Column {
                            width: parent.width * 0.44; spacing: 10
                            Text { text: "TIMING"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
                            Grid {
                                columns: 2; columnSpacing: 10; rowSpacing: 8; width: parent.width
                                Repeater {
                                    model: {
                                        var r = rvCol.r
                                        return [
                                            { l: "Setup / transition", v: hud.ms(r.setupDurationMs) },
                                            { l: "Sighters", v: "" + (r.sighterCount||0) + " (" + hud.ms(r.sighterDurationMs) + ")" },
                                            { l: "Ready → first shot", v: hud.ms(r.readyToFirstShotMs) },
                                            { l: "Verification block", v: hud.ms(r.verificationDurationMs) }
                                        ]
                                    }
                                    delegate: Rectangle { width: (parent.width-10)/2; height: 56; radius: 8; color: "#1D2026"; border.color: _line; border.width: 1
                                        Column { anchors.fill: parent; anchors.margins: 8; spacing: 2
                                            Text { text: modelData.l; color: _txtMut; font.pixelSize: 9; width: parent.width; wrapMode: Text.WordWrap }
                                            Text { text: modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 14; font.bold: true } } }
                                }
                            }
                            Text { text: "MEASURED RESULT"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 2 }
                            Grid {
                                columns: 3; columnSpacing: 10; rowSpacing: 6; width: parent.width
                                Repeater {
                                    model: {
                                        var r = rvCol.r
                                        return [
                                            { l: "First shot", v: (r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—") },
                                            { l: "First→MPI", v: (r.firstShotDistMm!==undefined?Number(r.firstShotDistMm).toFixed(1)+" mm":"—") },
                                            { l: "Avg score", v: Number(r.averageScore||0).toFixed(1) },
                                            { l: "Group dia", v: r.hasGroup?Number(r.groupDiameter).toFixed(1)+" mm":"—" },
                                            { l: "MPI X/Y", v: Number(r.mpiX||0).toFixed(1)+"/"+Number(r.mpiY||0).toFixed(1) },
                                            { l: "Score SD", v: Number(r.scoreSd||0).toFixed(2) }
                                        ]
                                    }
                                    delegate: Column { width: (parent.width-20)/3; spacing: 1
                                        Text { text: modelData.l; color: _txtMut; font.pixelSize: 9 }
                                        Text { text: modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 13; font.bold: true } }
                                }
                            }
                            Text { visible: rvCol.r.firstShotSeparated === true
                                   width: parent.width; wrapMode: Text.WordWrap; color: _txtSec; font.pixelSize: 11
                                   text: "The first shot was separated from the verification group." }
                        }
                    }

                    // group pattern insights
                    Column { width: parent.width; spacing: 3; visible: rvCol.gp.hasData === true
                        Text { text: "GROUP PATTERN INSIGHTS"; color: _redHi; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; topPadding: 2 }
                        Repeater { model: rvCol.gp.properties || []
                            Column { width: rvCol.width; spacing: 0
                                Text { text: modelData.label + "  (" + modelData.confidence + " evidence)"; color: _green; font.pixelSize: 11; font.bold: true }
                                Text { width: rvCol.width; wrapMode: Text.WordWrap; text: modelData.evidence; color: _txtSec; font.pixelSize: 11 } } }
                        Text { width: rvCol.width; wrapMode: Text.WordWrap; text: rvCol.r.disclaimer || ""; color: _txtMut; font.pixelSize: 9 } }

                    // note
                    Text { text: "ATHLETE NOTE"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Rectangle { width: parent.width; height: 74; radius: 6; color: "#1D2026"; border.color: ptNote.activeFocus ? _red : _line; border.width: 1
                        TextEdit { id: ptNote; anchors.fill: parent; anchors.margins: 10; color: _txt; font.pixelSize: 13; wrapMode: TextEdit.Wrap
                            onTextChanged: if (length > 300) remove(300, length) } }
                    Row { spacing: 10
                        Rectangle { width: 130; height: 40; radius: 8; color: saveM.pressed ? "#3A3C44" : "#2B2C33"; border.color: _line; border.width: 1
                            Text { id: saveLbl; anchors.centerIn: parent; text: "Save Note"; color: _txt; font.pixelSize: 13 }
                            MouseArea { id: saveM; anchors.fill: parent; onClicked: if (hud.ctl && hud.ctl.saveNote(ptNote.text)) { saveLbl.text = "Saved ✓"; savedT.restart() } }
                            Timer { id: savedT; interval: 1500; onTriggered: saveLbl.text = "Save Note" } } }

                    // actions
                    Row { width: parent.width; spacing: 12; topPadding: 8
                        Rectangle { width: parent.width * 0.62; height: 56; radius: 8; color: contM.pressed ? "#A80038" : _red
                            Text { anchors.centerIn: parent; color: "white"; font.pixelSize: 15; font.bold: true
                                   text: (hud.ctl && hud.ctl.hasNext) ? ("BEGIN TRANSITION TO " + hud.ctl.nextPositionName.toUpperCase()) : "VIEW SESSION SUMMARY" }
                            MouseArea { id: contM; anchors.fill: parent
                                onClicked: { if (!hud.ctl) return; if (ptNote.text.length>0) hud.ctl.saveNote(ptNote.text); ptNote.text=""; hud.ctl.continueToNext() } } }
                        Rectangle { width: parent.width * 0.34; height: 56; radius: 8; color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "End Training"; color: _txtSec; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("End training?"),
                                    qsTr("Completed positions are preserved."),
                                    function(ok){ if(ok && hud.ctl){ if(ptNote.text.length>0) hud.ctl.saveNote(ptNote.text); ptNote.text=""; hud.ctl.endEarly() } },
                                    qsTr("End Training"), qsTr("Keep Training")) } }
                    }
                }
            }
        }
    }

    // ── SUMMARY ──────────────────────────────────────────────────────────
    Rectangle {
        visible: hud.summaryOpen
        anchors.fill: parent; color: "#EA0F1116"
        MouseArea { anchors.fill: parent }
        Rectangle {
            width: Math.min(1080, parent.width - 40); height: Math.min(800, parent.height - 30)
            anchors.centerIn: parent; color: _card; radius: 12; border.color: _line; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 22; contentHeight: smCol.implicitHeight; clip: true
                Column {
                    id: smCol; width: parent.width; spacing: 10
                    property var rep: (hud.summaryOpen && hud.ctl) ? hud.ctl.reportModel() : ({})
                    property var comp: (smCol.rep.comparison) ? smCol.rep.comparison : []
                    property var ins: (smCol.rep.insights) ? smCol.rep.insights : ({})
                    property var rank: (smCol.rep.rankings) ? smCol.rep.rankings : ({})
                    // normalisers for the cross-position comparison bars
                    property real maxSetup: {
                        var mx = 1
                        for (var i = 0; i < comp.length; ++i) mx = Math.max(mx, comp[i].setupDurationMs || 0)
                        return mx
                    }
                    property real maxDia: {
                        var mx = 1
                        for (var i = 0; i < comp.length; ++i) if (comp[i].hasGroup) mx = Math.max(mx, comp[i].groupDiameter || 0)
                        return mx
                    }
                    property int cardCols: Math.max(1, Math.min(3, comp.length))

                    Text { text: "POSITION TRANSITION COMPLETE"; color: _txt; font.pixelSize: 22; font.bold: true }
                    Text { text: "TRAINING SESSION"; color: _red; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2 }
                    Rectangle { width: notO.implicitWidth + 22; height: 24; radius: 12; color: "#2a0b10"; border.color: _red; border.width: 1
                        Text { id: notO; anchors.centerIn: parent; text: "Not an official competition result"; color: "#ffd0d7"; font.pixelSize: 10; font.bold: true } }

                    // overview cards
                    Grid { width: parent.width; columns: 5; columnSpacing: 10; rowSpacing: 10; topPadding: 6
                        Repeater {
                            model: {
                                var r = smCol.rep
                                if (r.positionsCompleted === undefined) return []
                                var d = r.durationSec || 0
                                return [
                                    { l: "POSITIONS", v: "" + r.positionsCompleted },
                                    { l: "SEQUENCE", v: r.sequence || "" },
                                    { l: "COUNTED", v: "" + r.countedShots },
                                    { l: "SIGHTERS", v: "" + r.totalSighters },
                                    { l: "DURATION", v: Math.floor(d/60) + "m " + ("0"+(d%60)).slice(-2) + "s" }
                                ]
                            }
                            delegate: Rectangle { width: (smCol.width-40)/5; height: 58; radius: 8; color: "#1D2026"; border.color: _line; border.width: 1
                                Column { anchors.fill: parent; anchors.margins: 8; spacing: 2
                                    Text { text: modelData.l; color: _txtMut; font.pixelSize: 8; font.bold: true }
                                    Text { text: modelData.v; color: _txt; font.pixelSize: 13; font.bold: true; width: parent.width; elide: Text.ElideRight } } }
                        }
                    }

                    // ranked indicators (procedural + result highlights)
                    Text { text: "SESSION HIGHLIGHTS"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                    Flow { width: parent.width; spacing: 8
                        Repeater {
                            model: {
                                var k = smCol.rank; if (!k) return []
                                var out = []
                                if (k.fastestSetup) out.push({ i: "⚡", l: "Fastest setup", n: k.fastestSetup.name, v: hud.ms(k.fastestSetup.value), c: _green })
                                if (k.slowestSetup) out.push({ i: "⏳", l: "Slowest setup", n: k.slowestSetup.name, v: hud.ms(k.slowestSetup.value), c: "#E8A13C" })
                                if (k.tightestGroup) out.push({ i: "◎", l: "Tightest group", n: k.tightestGroup.name, v: Number(k.tightestGroup.value).toFixed(1) + " mm", c: _green })
                                if (k.bestAverage) out.push({ i: "★", l: "Best average", n: k.bestAverage.name, v: Number(k.bestAverage.value).toFixed(1), c: _green })
                                if (k.steadiestRhythm) out.push({ i: "♪", l: "Steadiest rhythm", n: k.steadiestRhythm.name, v: "", c: _green })
                                return out
                            }
                            delegate: Rectangle {
                                width: hlRow.implicitWidth + 20; height: 40; radius: 8
                                color: "#1D2026"; border.color: _line; border.width: 1
                                Row { id: hlRow; anchors.centerIn: parent; spacing: 8
                                    Text { text: modelData.i; color: modelData.c; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                                    Column { anchors.verticalCenter: parent.verticalCenter; spacing: 0
                                        Text { text: modelData.l; color: _txtMut; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1 }
                                        Text { text: modelData.n + (modelData.v ? "  ·  " + modelData.v : ""); color: _txt; font.pixelSize: 12; font.bold: true } } }
                            }
                        }
                    }

                    // per-position cards
                    Text { text: "POSITIONS"; color: _txtMut; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; topPadding: 8 }
                    Flow { width: parent.width; spacing: 10
                        Repeater { model: smCol.comp
                            delegate: Rectangle {
                                property var r: modelData
                                width: (smCol.width - (smCol.cardCols - 1) * 10) / smCol.cardCols
                                height: cardCol.implicitHeight + 24
                                radius: 10; color: "#181B21"; border.color: _line; border.width: 1
                                Column {
                                    id: cardCol; anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                                    anchors.margins: 12; spacing: 8
                                    // header: position + rhythm badge
                                    Row { width: parent.width
                                        Text { text: r.positionName + " R" + r.repeat; color: _txt; font.pixelSize: 15; font.bold: true
                                               width: parent.width - rhBadge.width; elide: Text.ElideRight }
                                        Rectangle { id: rhBadge; visible: (r.rhythm||"") !== ""
                                            width: rhTxt.implicitWidth + 14; height: 18; radius: 9
                                            color: "transparent"; border.color: hud.rhythmColor(r.rhythm); border.width: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text { id: rhTxt; anchors.centerIn: parent; text: r.rhythm||""; color: hud.rhythmColor(r.rhythm); font.pixelSize: 9; font.bold: true } }
                                    }
                                    // mini group plot
                                    Rectangle {
                                        width: parent.width; height: parent.width * 0.72; radius: 8
                                        color: "#0C0E12"; border.color: _line; border.width: 1
                                        property var shots: (hud.summaryOpen && hud.ctl) ? hud.ctl.verificationPlot(r.position, r.repeat) : []
                                        property real rangeMm: {
                                            var mx = 8
                                            for (var i = 0; i < shots.length; ++i) mx = Math.max(mx, Math.abs(shots[i].xMm), Math.abs(shots[i].yMm))
                                            return mx * 1.25
                                        }
                                        readonly property real plotR: Math.min(width, height)/2 - 10
                                        Repeater { model: [1.0,0.6,0.3]
                                            Rectangle { width: parent.plotR*2*modelData; height: width; radius: width/2
                                                anchors.centerIn: parent; color: "transparent"; border.color: "#23262d"; border.width: 1 } }
                                        Rectangle { anchors.centerIn: parent; width: parent.plotR*2; height: 1; color: "#1c1f26" }
                                        Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR*2; color: "#1c1f26" }
                                        Rectangle { visible: r.hasGroup === true; width: 10; height: 10; radius: 5; color: "transparent"; border.color: _green; border.width: 2
                                            x: parent.width/2 + (r.mpiX/parent.rangeMm)*parent.plotR - 5
                                            y: parent.height/2 - (r.mpiY/parent.rangeMm)*parent.plotR - 5 }
                                        Repeater { model: parent.shots
                                            Rectangle { width: 8; height: 8; radius: 4
                                                color: modelData.first ? "#3DA9FC" : "#e8003d"; border.color: "white"; border.width: 1
                                                x: parent.width/2 + (modelData.xMm/parent.rangeMm)*parent.plotR - width/2
                                                y: parent.height/2 - (modelData.yMm/parent.rangeMm)*parent.plotR - height/2 } }
                                    }
                                    // metric grid
                                    Grid { width: parent.width; columns: 2; columnSpacing: 8; rowSpacing: 5
                                        Repeater {
                                            model: [
                                                { l: "Setup", v: hud.ms(r.setupDurationMs) },
                                                { l: "Sighters", v: "" + (r.sighterCount||0) },
                                                { l: "Ready→1st", v: hud.ms(r.readyToFirstShotMs) },
                                                { l: "First shot", v: (r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—") },
                                                { l: "Average", v: Number(r.averageScore||0).toFixed(1) },
                                                { l: "Group dia", v: (r.hasGroup?Number(r.groupDiameter).toFixed(1)+" mm":"—") },
                                                { l: "MPI X/Y", v: Number(r.mpiX||0).toFixed(1)+"/"+Number(r.mpiY||0).toFixed(1) },
                                                { l: "Avg shot", v: hud.secs(r.avgShotTime) }
                                            ]
                                            delegate: Column { width: (parent.width-8)/2; spacing: 1
                                                Text { text: modelData.l; color: _txtMut; font.pixelSize: 9 }
                                                Text { text: modelData.v; color: _txt; font.family: "Consolas"; font.pixelSize: 12; font.bold: true } }
                                        }
                                    }
                                    // comparison bars (relative to session max)
                                    Column { width: parent.width; spacing: 4; topPadding: 2
                                        Column { width: parent.width; spacing: 2
                                            Text { text: "Setup vs session"; color: _txtMut; font.pixelSize: 8 }
                                            Rectangle { width: parent.width; height: 6; radius: 3; color: "#0C0E12"
                                                Rectangle { height: parent.height; radius: 3; color: "#E8A13C"
                                                    width: parent.width * Math.min(1, (r.setupDurationMs||0)/smCol.maxSetup) } } }
                                        Column { width: parent.width; spacing: 2; visible: r.hasGroup === true
                                            Text { text: "Group vs session"; color: _txtMut; font.pixelSize: 8 }
                                            Rectangle { width: parent.width; height: 6; radius: 3; color: "#0C0E12"
                                                Rectangle { height: parent.height; radius: 3; color: _green
                                                    width: parent.width * Math.min(1, (r.groupDiameter||0)/smCol.maxDia) } } }
                                    }
                                    // interpretation
                                    Text { visible: (r.groupPattern && r.groupPattern.primaryLabel) ? true : false
                                           width: parent.width; wrapMode: Text.WordWrap
                                           text: "◆ " + ((r.groupPattern && r.groupPattern.primaryLabel) ? r.groupPattern.primaryLabel : "")
                                           color: _green; font.pixelSize: 10 }
                                }
                            }
                        }
                    }
                    Text { text: "● counted   ◆ first shot (blue)   ◎ MPI (green)   ·   rings are relative, not ISSF scoring zones";
                           color: _txtMut; font.pixelSize: 9; topPadding: 2 }

                    // What You Should Take
                    Text { text: "WHAT YOU SHOULD TAKE FROM THIS SESSION"; color: _redHi; font.pixelSize: 12; font.bold: true; topPadding: 6 }
                    Column { width: parent.width; spacing: 3
                        Repeater { model: smCol.ins.observations || []
                            Text { width: smCol.width; wrapMode: Text.WordWrap; text: "· " + modelData; color: _txtSec; font.pixelSize: 12 } }
                        Repeater { model: smCol.ins.reviewItems || []
                            Text { width: smCol.width; wrapMode: Text.WordWrap; text: "· " + modelData; color: _txtSec; font.pixelSize: 12 } } }

                    Row { width: parent.width; spacing: 10; topPadding: 12
                        Rectangle { width: 180; height: 56; radius: 8; color: pdfM.pressed ? "#2A2C34" : "#23252C"; border.color: _line; border.width: 1
                            Row { anchors.centerIn: parent; spacing: 8
                                Text { text: "⭳"; color: _green; font.pixelSize: 18; anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "EXPORT PDF"; color: _txt; font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter } }
                            MouseArea { id: pdfM; anchors.fill: parent; onClicked: hud.exportPdfRequested() } }
                        Rectangle { width: 160; height: 56; radius: 8; color: nsM.pressed ? "#A80038" : _red
                            Text { anchors.centerIn: parent; text: "NEW SESSION"; color: "white"; font.pixelSize: 14; font.bold: true }
                            MouseArea { id: nsM; anchors.fill: parent
                                onClicked: dialogManager.showConfirmation(qsTr("Start a new session?"),
                                    qsTr("This session is closed and preserved. A new session ID is created."),
                                    function(ok){ if(ok) hud.newSessionRequested() }, qsTr("New Session"), qsTr("Cancel")) } }
                        Rectangle { width: 120; height: 56; radius: 8; color: "transparent"; border.color: _line; border.width: 1
                            Text { anchors.centerIn: parent; text: "Home"; color: _txtSec; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent; onClicked: hud.homeRequested() } }
                    }
                }
            }
        }
    }
}
