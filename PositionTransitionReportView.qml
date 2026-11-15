import QtQuick 2.15

// Position Transition (T4) — branded PDF report renderer. Off-screen A4 pages
// from POSTRANS.reportModel(). Reuses the enlarged Tech Aim header/footer
// branding and the shared A4 writer. Measured language only.
Item {
    id: view
    // LOC-REPORT-001: the controller supplies English position names; the
    // report translates them at display time (unknown names unchanged).
    // LOC-REPORT-001: GroupPatternAnalyzer supplies its confidence as an
    // English word; translated here, at display time.
    function trEvidence(c) {
        switch (c) {
        case "insufficient": return qsTr("insufficient")
        case "low":          return qsTr("low")
        case "moderate":     return qsTr("moderate")
        case "strong":       return qsTr("strong")
        }
        return c || ""
    }
    function trPos(n) {
        switch (n) {
        case "Kneeling": return qsTr("Kneeling")
        case "Prone":    return qsTr("Prone")
        case "Standing": return qsTr("Standing")
        }
        return n || ""
    }
    property var ctl: null                 // POSTRANS
    signal exported(string path)
    signal failed(string reason)

    readonly property int pageW: 794
    readonly property int pageH: 1123
    width: pageW; height: pageH
    x: -(pageW + 200); y: 0

    readonly property color ink:   "#14171C"
    readonly property color sub:    "#5C636E"
    readonly property color line:   "#D5D9DF"
    readonly property color red:    theme.tokens.accentHover

    property var model: ({})
    property var positions: []

    readonly property int totalPages: 3 + (positions ? positions.length : 0)

    function _pageItems() {
        var items = [overviewPage, comparisonPage]
        for (var i = 0; i < posPages.count; ++i) items.push(posPages.itemAt(i))
        items.push(notesPage)
        return items
    }

    property int _idx: 0
    property var _pages: []
    property string _path: ""

    function exportPdf(path) {
        if (!ctl) { failed(qsTr("No Position Transition session")); return }
        view.model = ctl.reportModel()
        view.positions = view.model.positions ? view.model.positions : []
        _path = path
        CUSTOMPRINT.clearImagesList()
        _pages = _pageItems()
        _idx = 0
        Qt.callLater(grabTimer.start)
    }
    Timer { id: grabTimer; interval: 60; repeat: false; onTriggered: view._grabNext() }
    function _grabNext() {
        if (_idx >= _pages.length) {
            var ok = CUSTOMPRINT.createTrainingPdf(_path)
            if (ok) exported(_path); else failed("PDF could not be written")
            return
        }
        _pages[_idx].grabToImage(function(result) { CUSTOMPRINT.addImage(result.image); _idx += 1; grabTimer.restart() },
                                 Qt.size(pageW, pageH))
    }
    function ms(v) { return v === undefined ? "—" : (Math.floor(v/1000) + "." + Math.floor((v%1000)/100) + " s") }
    function secs(v) { return (v === undefined || v <= 0) ? "—" : (Number(v).toFixed(1) + " s") }

    component ReportPage: Rectangle {
        width: view.pageW; height: view.pageH; color: "white"
        property int pageNo: 0
        default property alias content: inner.data
        Column {
            anchors.fill: parent; anchors.margins: 40; anchors.bottomMargin: 54; spacing: 12
            Item {
                width: parent.width; height: 88
                Row {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 12
                    Rectangle { width: 6; height: 74; radius: 3; color: view.red; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 4
                        Image { source: PRODUCT.brandLogoPath; height: 60
                            width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                            fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true }
                        Row { spacing: 8
                            Text { text: qsTr("TRAINING LAB"); color: view.red; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                                   anchors.verticalCenter: parent.verticalCenter }
                            Text { text: qsTr("POSITION TRANSITION REPORT"); color: view.ink; font.pixelSize: 15; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
                Column {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Repeater {
                        model: [
                            { l: qsTr("Athlete"),  v: view.model.athlete || "—" },
                            { l: qsTr("Discipline"), v: qsTr("50m Rifle 3 Positions") },
                            { l: qsTr("Date"),     v: (view.model.createdAtIso || "").substring(0, 10) },
                            { l: qsTr("Session"),  v: (view.model.sessionId || "").substring(0, 8) }
                        ]
                        delegate: Row { anchors.right: parent.right; spacing: 6
                            Text { text: modelData.l + ":"; color: view.sub; font.pixelSize: 9 }
                            Text { text: modelData.v; color: view.ink; font.pixelSize: 9; font.bold: true } }
                    }
                }
            }
            Rectangle { width: parent.width; height: 2; color: view.red; opacity: 0.85 }
            Text { text: qsTr("Not an official competition result"); color: view.sub; font.pixelSize: 9; font.italic: true }
            Item { id: inner; width: parent.width; height: parent.height - 180 }
        }
        Item {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: 40; height: 30
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: view.line }
            Row { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                Image { source: PRODUCT.brandLogoPath; height: 16
                    width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                    fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true; anchors.verticalCenter: parent.verticalCenter }
                Text { text: PRODUCT.fullProductName
                             + (PRODUCT.tagline !== "" ? "  ·  " + PRODUCT.tagline : "")
                       color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter } }
            Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                   text: qsTr("Page %1 of %2").arg(pageNo).arg(view.totalPages); color: view.sub; font.pixelSize: 9 }
        }
    }
    component KV: Row {
        property string k: ""; property string v: ""
        spacing: 8; width: 700
        Text { text: k; color: view.sub; font.pixelSize: 11; width: 220 }
        Text { text: v; color: view.ink; font.pixelSize: 11; font.bold: true; width: 460; wrapMode: Text.WordWrap }
    }

    // PAGE 1 — OVERVIEW
    ReportPage {
        id: overviewPage; pageNo: 1
        Column {
            width: parent.width; spacing: 8
            Text { text: qsTr("SESSION OVERVIEW"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Item { height: 4; width: 1 }
            KV { k: qsTr("Athlete"); v: view.model.athlete || "—" }
            KV { k: qsTr("Date / time"); v: view.model.createdAtIso || "—" }
            KV { k: qsTr("Session ID"); v: view.model.sessionId || "—" }
            KV { k: qsTr("Operating mode"); v: view.model.operatingMode || "—" }
            KV { k: qsTr("Technical focus"); v: view.model.focus || "—" }
            KV { k: qsTr("Sequence"); v: view.model.sequence || "—" }
            KV { k: qsTr("Verification shots"); v: qsTr("%1 per position").arg(view.model.verificationShots||0) }
            KV { k: qsTr("Repeats"); v: "" + (view.model.repeats||1) }
            KV { k: qsTr("Positions completed"); v: "" + (view.model.positionsCompleted||0) }
            KV { k: qsTr("Counted shots"); v: "" + (view.model.countedShots||0) }
            KV { k: qsTr("Sighters"); v: qsTr("%1 (excluded from results)").arg(view.model.totalSighters||0) }
            KV { k: qsTr("Duration"); v: { var d = view.model.durationSec||0; return qsTr("%1 min %2 s").arg(Math.floor(d/60)).arg(("0"+(d%60)).slice(-2)) } }
            Item { height: 8; width: 1 }
            Text { text: qsTr("WHAT YOU SHOULD TAKE FROM THIS SESSION"); color: view.red; font.pixelSize: 12; font.bold: true }
            Repeater { model: (view.model.insights ? (view.model.insights.observations||[]) : [])
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
            Repeater { model: (view.model.insights ? (view.model.insights.reviewItems||[]) : [])
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
        }
    }

    // PAGE 2 — POSITION COMPARISON
    ReportPage {
        id: comparisonPage; pageNo: 2
        Column {
            width: parent.width; spacing: 6
            Text { text: qsTr("POSITION COMPARISON"); color: view.ink; font.pixelSize: 15; font.bold: true }
            // ranked session highlights
            Text { text: qsTr("SESSION HIGHLIGHTS"); color: view.red; font.pixelSize: 11; font.bold: true; topPadding: 2 }
            Flow { width: 714; spacing: 8
                Repeater {
                    model: {
                        var k = view.model.rankings; if (!k) return []
                        var out = []
                        if (k.fastestSetup)    out.push(qsTr("⚡ Fastest setup: %1 (%2)").arg(view.trPos(k.fastestSetup.name)).arg(view.ms(k.fastestSetup.value)))
                        if (k.slowestSetup)    out.push(qsTr("⏳ Slowest setup: %1 (%2)").arg(view.trPos(k.slowestSetup.name)).arg(view.ms(k.slowestSetup.value)))
                        if (k.tightestGroup)   out.push(qsTr("◎ Tightest group: %1 (%2 mm)").arg(view.trPos(k.tightestGroup.name)).arg(Number(k.tightestGroup.value).toFixed(1)))
                        if (k.bestAverage)     out.push(qsTr("★ Best average: %1 (%2)").arg(view.trPos(k.bestAverage.name)).arg(Number(k.bestAverage.value).toFixed(1)))
                        if (k.steadiestRhythm) out.push(qsTr("♪ Lowest rhythm variability: %1").arg(view.trPos(k.steadiestRhythm.name)))
                        return out
                    }
                    Rectangle { width: hlTxt.implicitWidth + 16; height: 24; radius: 5; color: "#F4F5F7"; border.color: view.line; border.width: 1
                        Text { id: hlTxt; anchors.centerIn: parent; text: modelData; color: view.ink; font.pixelSize: 10; font.bold: true } }
                }
            }
            Item { height: 6; width: 1 }
            Row { spacing: 0
                Repeater { model: [qsTr("POSITION"),qsTr("SETUP"),qsTr("SIGHT"),qsTr("R→1st"),qsTr("1ST"),qsTr("DIA mm"),qsTr("AVG SCORE")]
                    Text { text: modelData; color: view.sub; font.pixelSize: 10; font.bold: true; width: 100 } } }
            Rectangle { width: parent.width; height: 1; color: view.line }
            Repeater { model: (view.model.comparison || [])
                Row { spacing: 0; property var r: modelData
                    Text { width: 100; color: view.ink; font.pixelSize: 10; font.bold: true; text: qsTr("%1 R%2").arg(view.trPos(r.positionName)).arg(r.repeat) }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: view.ms(r.setupDurationMs) }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: "" + r.sighterCount }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: view.ms(r.readyToFirstShotMs) }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—" }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: r.hasGroup?Number(r.groupDiameter).toFixed(1):"—" }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: Number(r.averageScore||0).toFixed(1) } } }
            Item { height: 8; width: 1 }
            // shot rhythm / cadence per position
            Text { text: qsTr("SHOT RHYTHM"); color: view.sub; font.pixelSize: 10; font.bold: true }
            Repeater { model: (view.model.comparison || [])
                Row { spacing: 8; property var r: modelData
                    Text { width: 140; color: view.ink; font.pixelSize: 10; font.bold: true; text: qsTr("%1 R%2").arg(view.trPos(r.positionName)).arg(r.repeat) }
                    Text { width: 190; color: view.ink; font.pixelSize: 10
                           text: (r.rhythm && r.rhythm.length>0) ? r.rhythm : "—" }
                    // PT-02: the label never appears without the measurement
                    // and the sample size it was derived from.
                    Text { color: view.sub; font.pixelSize: 10
                           text: qsTr("avg shot %1").arg(view.secs(r.avgShotTime))
                                 + ((r.rhythmBasis && r.rhythmBasis.length > 0)
                                    ? "  ·  " + r.rhythmBasis : "") } } }
            Item { height: 8; width: 1 }
            // EVID-PT-001: bound to the central model, never re-typed here.
            Text { text: POSTRANS.crossPositionCaveat
                   width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true }
        }
    }

    // PER-POSITION PAGES
    Repeater {
        id: posPages
        model: view.positions
        ReportPage {
            pageNo: 3 + index
            property var r: modelData
            Column {
                width: parent.width; spacing: 8
                Text { text: qsTr("%1 · Repeat %2").arg(view.trPos(r.positionName)).arg(r.repeat||1); color: view.ink; font.pixelSize: 15; font.bold: true }
                Text { text: qsTr("%1 counted shots · sighters excluded").arg(r.verificationShots||0); color: view.sub; font.pixelSize: 10 }
                Row {
                    width: parent.width; spacing: 16
                    // plot
                    Rectangle {
                        width: 300; height: 300; color: "#FBFBFC"; border.color: view.line; border.width: 1
                        property var shots: r.plot ? r.plot : []
                        property real rangeMm: { var mx=8; for (var i=0;i<shots.length;++i) mx=Math.max(mx,Math.abs(shots[i].xMm),Math.abs(shots[i].yMm)); return mx*1.25 }
                        readonly property real plotR: width/2 - 14
                        Repeater { model: [1.0,0.6,0.3]
                            Rectangle { width: parent.plotR*2*modelData; height: width; radius: width/2; anchors.centerIn: parent; color:"transparent"; border.color:"#E2E4E8"; border.width:1 } }
                        Rectangle { anchors.centerIn: parent; width: parent.plotR*2; height: 1; color: "#ECEEF1" }
                        Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR*2; color: "#ECEEF1" }
                        Rectangle { visible: r.hasGroup===true; width: 12; height: 12; radius: 6; color:"transparent"; border.color: view.red; border.width: 2
                            x: parent.width/2 + (r.mpiX/parent.rangeMm)*parent.plotR - 6
                            y: parent.height/2 - (r.mpiY/parent.rangeMm)*parent.plotR - 6 }
                        Repeater { model: parent.shots
                            Rectangle { width: 9; height: 9; radius: 5; color: modelData.first ? "#2E6FB0" : view.red; border.color:"white"; border.width:1
                                x: parent.width/2 + (modelData.xMm/parent.rangeMm)*parent.plotR - width/2
                                y: parent.height/2 - (modelData.yMm/parent.rangeMm)*parent.plotR - height/2 } }
                    }
                    // metrics
                    Column { spacing: 3; width: 360
                        KV { k: qsTr("Setup / transition"); v: view.ms(r.setupDurationMs); width: 360 }
                        KV { k: qsTr("Sighters"); v: "" + (r.sighterCount||0); width: 360 }
                        KV { k: qsTr("Ready → first shot"); v: view.ms(r.readyToFirstShotMs); width: 360 }
                        KV { k: qsTr("Verification block"); v: view.ms(r.verificationDurationMs); width: 360 }
                        KV { k: qsTr("First shot"); v: (r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—") + " (" + (r.firstShotDistMm!==undefined?qsTr("%1 mm from MPI").arg(Number(r.firstShotDistMm).toFixed(1)):"—") + ")"; width: 360 }
                        KV { k: qsTr("Average score"); v: Number(r.averageScore||0).toFixed(1); width: 360 }
                        KV { k: qsTr("Group diameter"); v: r.hasGroup?Number(r.groupDiameter).toFixed(1)+" mm":"—"; width: 360 }
                        KV { k: qsTr("MPI (X/Y)"); v: Number(r.mpiX||0).toFixed(1)+" / "+Number(r.mpiY||0).toFixed(1)+" mm"; width: 360 }
                        KV { k: qsTr("H/V spread"); v: r.hasGroup?Number(r.hSpread).toFixed(1)+" / "+Number(r.vSpread).toFixed(1)+" mm":"—"; width: 360 }
                        KV { k: qsTr("Shot rhythm"); v: (r.rhythm && r.rhythm.length>0) ? (r.rhythm + " · " + qsTr("avg %1").arg(view.secs(r.avgShotTime))) : "—"; width: 360 }
                    }
                }
                // checklist
                Text { text: qsTr("SETUP CHECKLIST"); color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 4 }
                Repeater { model: r.checklist ? r.checklist : []
                    Text { width: 700; color: view.ink; font.pixelSize: 10
                           text: (modelData.state===1?"✓ ":(modelData.state===2?"– ":"○ ")) + modelData.label } }
                // group pattern
                Column { width: 700; spacing: 1; visible: (r.groupPattern && r.groupPattern.hasData===true)
                    Text { text: qsTr("GROUP PATTERN INSIGHTS"); color: view.red; font.pixelSize: 10; font.bold: true; topPadding: 4 }
                    Repeater { model: (r.groupPattern ? (r.groupPattern.properties||[]) : [])
                        Column { width: 700; spacing: 0
                            Text { text: qsTr("%1  (%2 evidence)").arg(modelData.label).arg(view.trEvidence(modelData.confidence)); color: view.ink; font.pixelSize: 10; font.bold: true }
                            Text { width: 700; wrapMode: Text.WordWrap; text: modelData.evidence; color: view.sub; font.pixelSize: 10 } } } }
                Text { text: qsTr("ATHLETE NOTE"); color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 4 }
                Text { width: 700; wrapMode: Text.WordWrap; color: view.ink; font.pixelSize: 11; text: (r.note && r.note.length>0) ? r.note : "—" }
                Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 9; text: r.disclaimer || "" }
            }
        }
    }

    // FINAL
    ReportPage {
        id: notesPage; pageNo: view.totalPages
        Column {
            width: parent.width; spacing: 8
            Text { text: qsTr("NOTES & OBSERVATIONS"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Text { visible: (view.model.sessionNote||"").length>0; text: qsTr("Session note: %1").arg(view.model.sessionNote||"")
                   color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap }
            Repeater { model: view.positions
                Column { width: 700; spacing: 1; visible: modelData.note && modelData.note.length>0
                    property var r: modelData
                    Text { text: qsTr("%1 · Repeat %2").arg(view.trPos(r.positionName)).arg(r.repeat||1); color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { text: r.note || ""; color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap } } }
            Item { height: 8; width: 1 }
            Text { text: qsTr("OVERALL OBSERVED"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater { model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
            Item { height: 10; width: 1 }
            Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true
                   text: qsTr("Measured training data only. This is not an official competition result. Sighters are excluded from all results.") }
        }
    }
}
