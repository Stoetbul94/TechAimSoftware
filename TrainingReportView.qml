import QtQuick 2.15

// Training Lab (T1.4) — Training-specific PDF report renderer. Off-screen A4
// pages built ENTIRELY from TRAINING.trainingReportModel() (the single
// authoritative source; nothing is recomputed here). exportPdf() grabs each
// page to an image and hands them to CUSTOMPRINT.createTrainingPdf(path).
//
// Measured language only — no diagnostic-cause claims, no official/ranking
// wording. White A4 pages so the print is readable. Not shown on screen: the
// container is sized to one page and sits behind the app (opacity 0 until a
// grab is in progress). Early-ended sessions are labelled and report only the
// completed blocks the model contains.
Item {
    id: view
    property var ctl: null                 // TRAINING
    signal exported(string path)
    signal failed(string reason)

    // A4 at ~96 dpi (portrait). One page grabbed at a time. Rendered at full
    // opacity but positioned OFF-SCREEN (grabToImage would capture a blank
    // texture for an opacity:0 item), so it is grabbable yet never seen.
    readonly property int pageW: 794
    readonly property int pageH: 1123
    width: pageW; height: pageH
    x: -(pageW + 200); y: 0

    readonly property color ink:   "#14171C"
    readonly property color sub:    "#5C636E"
    readonly property color line:   "#D5D9DF"
    readonly property color red:    "#C40046"
    readonly property color green:  "#127A5A"

    property var model: ({})
    property var blocks: []

    // Build the ordered list of page components for the current model.
    function _pageItems() {
        var items = [overviewPage, comparisonPage]
        for (var i = 0; i < blockPages.count; ++i)
            items.push(blockPages.itemAt(i))
        items.push(notesPage)
        return items
    }

    // Grab pages sequentially (grabToImage is async; its result is only valid
    // inside the callback), then write the PDF.
    property int _idx: 0
    property var _pages: []
    property string _path: ""

    function exportPdf(path) {
        if (!ctl) { failed("No training session"); return }
        view.model = ctl.trainingReportModel()
        view.blocks = view.model.blocks ? view.model.blocks : []
        _path = path
        CUSTOMPRINT.clearImagesList()
        // let bindings settle (per-block pages instantiate from the model) then grab
        _pages = _pageItems()
        _idx = 0
        Qt.callLater(grabTimer.start)
    }

    Timer {
        id: grabTimer; interval: 60; repeat: false
        onTriggered: view._grabNext()
    }

    function _grabNext() {
        if (_idx >= _pages.length) {
            var ok = CUSTOMPRINT.createTrainingPdf(_path)
            if (ok) exported(_path); else failed("PDF could not be written")
            return
        }
        var pg = _pages[_idx]
        pg.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image)
            _idx += 1
            grabTimer.restart()
        }, Qt.size(pageW, pageH))
    }

    // ── shared page chrome ────────────────────────────────────────────────
    component ReportPage: Rectangle {
        width: view.pageW; height: view.pageH; color: "white"
        default property alias content: inner.data
        Column {
            anchors.fill: parent; anchors.margins: 40; spacing: 14
            // header band
            Row {
                width: parent.width; spacing: 10
                Rectangle { width: 6; height: 40; color: view.red; radius: 3 }
                Column {
                    Text { text: "TECH AIM"; color: view.red; font.pixelSize: 12; font.bold: true; font.letterSpacing: 3 }
                    Text { text: "TRAINING SESSION · Technical Blocks"; color: view.ink; font.pixelSize: 16; font.bold: true }
                }
            }
            Rectangle { width: parent.width; height: 1; color: view.line }
            Item { id: inner; width: parent.width; height: parent.height - 90 }
        }
        // footer
        Text {
            anchors.bottom: parent.bottom; anchors.bottomMargin: 22
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Not an official competition result · Measured training data"
            color: view.sub; font.pixelSize: 9
        }
    }

    // reusable metric row
    component KV: Row {
        property string k: ""; property string v: ""
        spacing: 8; width: 700
        Text { text: k; color: view.sub; font.pixelSize: 11; width: 200 }
        Text { text: v; color: view.ink; font.pixelSize: 11; font.bold: true; width: 480; wrapMode: Text.WordWrap }
    }

    // ── PAGE 1 — SESSION OVERVIEW ─────────────────────────────────────────
    ReportPage {
        id: overviewPage
        Column {
            width: parent.width; spacing: 8
            Text { text: view.model.endedEarly ? "SESSION ENDED EARLY" : "SESSION OVERVIEW"
                   color: view.ink; font.pixelSize: 15; font.bold: true }
            Item { height: 6; width: 1 }
            KV { k: "Athlete";        v: view.model.athlete || "—" }
            KV { k: "Date / time";    v: view.model.createdAtIso || "—" }
            KV { k: "Session ID";     v: view.model.sessionId || "—" }
            KV { k: "Operating mode"; v: view.model.operatingMode || "—" }
            KV { k: "Discipline";     v: view.model.threePositions ? "50m Rifle 3 Positions" : "Single position" }
            KV { k: "Technical focus"; v: view.model.focus || "—" }
            KV { k: "Visibility mode"; v: view.model.visibilityLabel || "—" }
            KV { k: "Programme";      v: (view.model.blockCount || 0) + " blocks × " + (view.model.shotsPerBlock || 0) + " shots" }
            KV { k: "Planned shots";  v: "" + (view.model.plannedShots || 0) }
            KV { k: "Counted shots";  v: "" + (view.model.countedShots || 0) }
            KV { k: "Completed blocks"; v: (view.model.completedBlocks || 0) + " of " + (view.model.blockCount || 0) }
            KV { k: "Session duration"; v: {
                    var d = view.model.durationSec || 0
                    return Math.floor(d / 60) + " min " + ("0" + (d % 60)).slice(-2) + " s"
                } }
            KV { k: "Sighters"; v: {
                    var a = view.model.sighterAudit || {}
                    if (a.threePositions)
                        return "Kneeling " + (a.kneeling||0) + ", Prone " + (a.prone||0)
                             + ", Standing " + (a.standing||0) + "  (total " + (a.total||0) + ")"
                    return "" + (a.total || 0)
                } }
            Item { height: 8; width: 1 }
            Text { text: "Sighters are excluded from all Training results below."
                   color: view.sub; font.pixelSize: 10; font.italic: true }
        }
    }

    // ── PAGE 2 — BLOCK COMPARISON ─────────────────────────────────────────
    ReportPage {
        id: comparisonPage
        Column {
            width: parent.width; spacing: 8
            Text { text: "BLOCK COMPARISON"; color: view.ink; font.pixelSize: 15; font.bold: true }
            // table header
            Row {
                width: parent.width; spacing: 0
                Repeater { model: ["BLOCK", "AVG", "DIA mm", "MPI X/Y mm", "SCORE SD", "SHOTS"]
                    Text { text: modelData; color: view.sub; font.pixelSize: 10; font.bold: true; width: 116 } }
            }
            Rectangle { width: parent.width; height: 1; color: view.line }
            Repeater {
                model: view.blocks
                Row {
                    width: 700; spacing: 0
                    property var b: modelData
                    Text { width: 116; color: view.ink; font.pixelSize: 11; font.bold: true
                           text: "Block " + b.blockIndex + (b.positionName ? " · " + b.positionName.substring(0,1) : "") }
                    Text { width: 116; color: view.ink; font.pixelSize: 11; text: Number(b.averageScore).toFixed(1) }
                    Text { width: 116; color: view.ink; font.pixelSize: 11; text: b.hasGroup ? Number(b.groupDiameter).toFixed(1) : "—" }
                    Text { width: 116; color: view.ink; font.pixelSize: 11; text: Number(b.mpiX).toFixed(1) + " / " + Number(b.mpiY).toFixed(1) }
                    Text { width: 116; color: view.ink; font.pixelSize: 11; text: Number(b.scoreStdDev).toFixed(2) }
                    Text { width: 116; color: view.ink; font.pixelSize: 11; text: "" + b.shotCount }
                }
            }
            Item { height: 10; width: 1 }
            Text { text: "GROUP DIAMETER BY BLOCK"; color: view.sub; font.pixelSize: 10; font.bold: true }
            Repeater {
                model: view.blocks
                Row {
                    spacing: 8; property var b: modelData
                    Text { text: "B" + b.blockIndex; color: view.sub; font.pixelSize: 10; width: 26 }
                    Rectangle {
                        width: 420; height: 12; color: "#EEEFF2"; anchors.verticalCenter: parent.verticalCenter
                        Rectangle { height: parent.height; color: view.red; radius: 2
                            width: b.hasGroup ? parent.width * Math.min(1, b.groupDiameter / Math.max(1, view._maxDia())) : 0 }
                    }
                    Text { text: b.hasGroup ? Number(b.groupDiameter).toFixed(1) + " mm" : "—"
                           color: view.ink; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                }
            }
            Item { height: 10; width: 1 }
            Text { text: "OBSERVED"; color: view.sub; font.pixelSize: 10; font.bold: true }
            Repeater {
                model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 }
            }
        }
    }
    function _maxDia() {
        var mx = 1
        for (var i = 0; i < blocks.length; ++i)
            if (blocks[i].hasGroup) mx = Math.max(mx, blocks[i].groupDiameter)
        return mx
    }

    // ── PER-BLOCK PAGES ───────────────────────────────────────────────────
    Repeater {
        id: blockPages
        model: view.blocks
        ReportPage {
            property var b: modelData
            Column {
                width: parent.width; spacing: 10
                Text { text: "BLOCK " + b.blockIndex + (b.positionName ? " · " + b.positionName : "")
                       color: view.ink; font.pixelSize: 15; font.bold: true }
                Text { text: b.shotCount + " counted shots · sighters excluded"
                       color: view.sub; font.pixelSize: 10 }
                Row {
                    width: parent.width; spacing: 16
                    // target/group plot
                    Rectangle {
                        width: 320; height: 320; color: "#FBFBFC"; border.color: view.line; border.width: 1
                        property var shots: b.plot ? b.plot : []
                        property real rangeMm: {
                            var mx = 8
                            for (var i = 0; i < shots.length; ++i)
                                mx = Math.max(mx, Math.abs(shots[i].xMm), Math.abs(shots[i].yMm))
                            return mx * 1.25
                        }
                        readonly property real plotR: width / 2 - 16
                        Repeater { model: [1.0, 0.6, 0.3]
                            Rectangle { width: parent.plotR * 2 * modelData; height: width; radius: width/2
                                anchors.centerIn: parent; color: "transparent"; border.color: "#E2E4E8"; border.width: 1 } }
                        Rectangle { anchors.centerIn: parent; width: parent.plotR*2; height: 1; color: "#ECEEF1" }
                        Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR*2; color: "#ECEEF1" }
                        Repeater { model: parent.shots
                            Rectangle { width: 9; height: 9; radius: 5; color: view.red; border.color: "white"; border.width: 1
                                x: parent.width/2 + (modelData.xMm/parent.rangeMm)*parent.plotR - width/2
                                y: parent.height/2 - (modelData.yMm/parent.rangeMm)*parent.plotR - height/2 } }
                        Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 4; anchors.horizontalCenter: parent.horizontalCenter
                               text: "±" + Number(parent.rangeMm).toFixed(0) + " mm"; color: view.sub; font.pixelSize: 9 }
                    }
                    // metrics
                    Column {
                        spacing: 6
                        KV { k: "Average score"; v: Number(b.averageScore).toFixed(1); width: 360 }
                        KV { k: "Group radius";  v: b.hasGroup ? Number(b.groupRadius).toFixed(1)+" mm" : "—"; width: 360 }
                        KV { k: "Group diameter"; v: b.hasGroup ? Number(b.groupDiameter).toFixed(1)+" mm" : "—"; width: 360 }
                        KV { k: "MPI (X / Y)";   v: Number(b.mpiX).toFixed(1)+" / "+Number(b.mpiY).toFixed(1)+" mm"; width: 360 }
                        KV { k: "H / V spread";  v: b.hasGroup ? Number(b.horizontalSpread).toFixed(1)+" / "+Number(b.verticalSpread).toFixed(1)+" mm" : "—"; width: 360 }
                        KV { k: "Score SD";      v: Number(b.scoreStdDev).toFixed(2); width: 360 }
                        KV { k: "Avg shot time"; v: b.hasTiming ? Number(b.averageShotTime).toFixed(1)+" s" : "—"; width: 360 }
                    }
                }
                Text { text: "OBSERVED"; color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 6 }
                Repeater { model: b.observations ? b.observations : []
                    Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
                Text { text: "ATHLETE NOTE"; color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 6 }
                Text { width: 700; wrapMode: Text.WordWrap; color: view.ink; font.pixelSize: 11
                       text: (b.note && b.note.length > 0) ? b.note : "—" }
            }
        }
    }

    // ── FINAL PAGE — NOTES & OBSERVATIONS ─────────────────────────────────
    ReportPage {
        id: notesPage
        Column {
            width: parent.width; spacing: 10
            Text { text: "ATHLETE NOTES"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater {
                model: view.blocks
                Column {
                    width: 700; spacing: 2
                    property var b: modelData
                    Text { text: "Block " + b.blockIndex + (b.positionName ? " · " + b.positionName : "")
                           color: view.sub; font.pixelSize: 11; font.bold: true }
                    Text { width: 700; wrapMode: Text.WordWrap; color: view.ink; font.pixelSize: 11
                           text: (b.note && b.note.length > 0) ? b.note : "—" }
                }
            }
            Item { height: 10; width: 1 }
            Text { text: "OVERALL OBSERVED"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater {
                model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 }
            }
            Item { height: 12; width: 1 }
            Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true
                   text: "Measured training data only. Sighters are excluded from all results. "
                       + "This is not an official competition result and carries no ranking status." }
        }
    }
}
