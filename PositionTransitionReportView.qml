import QtQuick 2.15

// Position Transition (T4) — branded PDF report renderer. Off-screen A4 pages
// from POSTRANS.reportModel(). Reuses the enlarged Tech Aim header/footer
// branding and the shared A4 writer. Measured language only.
Item {
    id: view
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
    readonly property color red:    "#C40046"

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
        if (!ctl) { failed("No Position Transition session"); return }
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
                        Image { source: "qrc:/images/logo/techaim_color.png"; height: 60
                            width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                            fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true }
                        Row { spacing: 8
                            Text { text: "TRAINING LAB"; color: view.red; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                                   anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "POSITION TRANSITION REPORT"; color: view.ink; font.pixelSize: 15; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
                Column {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Repeater {
                        model: [
                            { l: "Athlete",  v: view.model.athlete || "—" },
                            { l: "Discipline", v: "50m Rifle 3 Positions" },
                            { l: "Date",     v: (view.model.createdAtIso || "").substring(0, 10) },
                            { l: "Session",  v: (view.model.sessionId || "").substring(0, 8) }
                        ]
                        delegate: Row { anchors.right: parent.right; spacing: 6
                            Text { text: modelData.l + ":"; color: view.sub; font.pixelSize: 9 }
                            Text { text: modelData.v; color: view.ink; font.pixelSize: 9; font.bold: true } }
                    }
                }
            }
            Rectangle { width: parent.width; height: 2; color: view.red; opacity: 0.85 }
            Text { text: "Not an official competition result"; color: view.sub; font.pixelSize: 9; font.italic: true }
            Item { id: inner; width: parent.width; height: parent.height - 180 }
        }
        Item {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: 40; height: 30
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: view.line }
            Row { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                Image { source: "qrc:/images/logo/techaim_color.png"; height: 16
                    width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                    fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "Tech Aim Electronic Target Control  ·  WE AIM TO PLEASE"
                       color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter } }
            Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                   text: "Page " + pageNo + " of " + view.totalPages; color: view.sub; font.pixelSize: 9 }
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
            Text { text: "SESSION OVERVIEW"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Item { height: 4; width: 1 }
            KV { k: "Athlete"; v: view.model.athlete || "—" }
            KV { k: "Date / time"; v: view.model.createdAtIso || "—" }
            KV { k: "Session ID"; v: view.model.sessionId || "—" }
            KV { k: "Operating mode"; v: view.model.operatingMode || "—" }
            KV { k: "Technical focus"; v: view.model.focus || "—" }
            KV { k: "Sequence"; v: view.model.sequence || "—" }
            KV { k: "Verification shots"; v: (view.model.verificationShots||0) + " per position" }
            KV { k: "Repeats"; v: "" + (view.model.repeats||1) }
            KV { k: "Positions completed"; v: "" + (view.model.positionsCompleted||0) }
            KV { k: "Counted shots"; v: "" + (view.model.countedShots||0) }
            KV { k: "Sighters"; v: "" + (view.model.totalSighters||0) + " (excluded from results)" }
            KV { k: "Duration"; v: { var d = view.model.durationSec||0; return Math.floor(d/60)+" min "+("0"+(d%60)).slice(-2)+" s" } }
            Item { height: 8; width: 1 }
            Text { text: "WHAT YOU SHOULD TAKE FROM THIS SESSION"; color: view.red; font.pixelSize: 12; font.bold: true }
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
            Text { text: "POSITION COMPARISON"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Row { spacing: 0
                Repeater { model: ["POSITION","SETUP","SIGHT","R→1st","1ST","DIA mm","AVG SCORE"]
                    Text { text: modelData; color: view.sub; font.pixelSize: 10; font.bold: true; width: 100 } } }
            Rectangle { width: parent.width; height: 1; color: view.line }
            Repeater { model: (view.model.comparison || [])
                Row { spacing: 0; property var r: modelData
                    Text { width: 100; color: view.ink; font.pixelSize: 10; font.bold: true; text: r.positionName + " R" + r.repeat }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: view.ms(r.setupDurationMs) }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: "" + r.sighterCount }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: view.ms(r.readyToFirstShotMs) }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—" }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: r.hasGroup?Number(r.groupDiameter).toFixed(1):"—" }
                    Text { width: 100; color: view.ink; font.pixelSize: 10; text: Number(r.averageScore||0).toFixed(1) } } }
            Item { height: 8; width: 1 }
            Text { text: "Positions have different stability demands — compare each position to itself across repeats, not against another position."
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
                Text { text: (r.positionName||"") + " · Repeat " + (r.repeat||1); color: view.ink; font.pixelSize: 15; font.bold: true }
                Text { text: (r.verificationShots||0) + " counted shots · sighters excluded"; color: view.sub; font.pixelSize: 10 }
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
                        KV { k: "Setup / transition"; v: view.ms(r.setupDurationMs); width: 360 }
                        KV { k: "Sighters"; v: "" + (r.sighterCount||0); width: 360 }
                        KV { k: "Ready → first shot"; v: view.ms(r.readyToFirstShotMs); width: 360 }
                        KV { k: "Verification block"; v: view.ms(r.verificationDurationMs); width: 360 }
                        KV { k: "First shot"; v: (r.firstShotScore!==undefined?Number(r.firstShotScore).toFixed(1):"—") + " (" + (r.firstShotDistMm!==undefined?Number(r.firstShotDistMm).toFixed(1)+" mm from MPI":"—") + ")"; width: 360 }
                        KV { k: "Average score"; v: Number(r.averageScore||0).toFixed(1); width: 360 }
                        KV { k: "Group diameter"; v: r.hasGroup?Number(r.groupDiameter).toFixed(1)+" mm":"—"; width: 360 }
                        KV { k: "MPI (X/Y)"; v: Number(r.mpiX||0).toFixed(1)+" / "+Number(r.mpiY||0).toFixed(1)+" mm"; width: 360 }
                        KV { k: "H/V spread"; v: r.hasGroup?Number(r.hSpread).toFixed(1)+" / "+Number(r.vSpread).toFixed(1)+" mm":"—"; width: 360 }
                    }
                }
                // checklist
                Text { text: "SETUP CHECKLIST"; color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 4 }
                Repeater { model: r.checklist ? r.checklist : []
                    Text { width: 700; color: view.ink; font.pixelSize: 10
                           text: (modelData.state===1?"✓ ":(modelData.state===2?"– ":"○ ")) + modelData.label } }
                // group pattern
                Column { width: 700; spacing: 1; visible: (r.groupPattern && r.groupPattern.hasData===true)
                    Text { text: "GROUP PATTERN INSIGHTS"; color: view.red; font.pixelSize: 10; font.bold: true; topPadding: 4 }
                    Repeater { model: (r.groupPattern ? (r.groupPattern.properties||[]) : [])
                        Column { width: 700; spacing: 0
                            Text { text: modelData.label + "  (" + modelData.confidence + " evidence)"; color: view.ink; font.pixelSize: 10; font.bold: true }
                            Text { width: 700; wrapMode: Text.WordWrap; text: modelData.evidence; color: view.sub; font.pixelSize: 10 } } } }
                Text { text: "ATHLETE NOTE"; color: view.sub; font.pixelSize: 10; font.bold: true; topPadding: 4 }
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
            Text { text: "NOTES & OBSERVATIONS"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Text { visible: (view.model.sessionNote||"").length>0; text: "Session note: " + (view.model.sessionNote||"")
                   color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap }
            Repeater { model: view.positions
                Column { width: 700; spacing: 1; visible: modelData.note && modelData.note.length>0
                    property var r: modelData
                    Text { text: (r.positionName||"") + " · Repeat " + (r.repeat||1); color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { text: r.note || ""; color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap } } }
            Item { height: 8; width: 1 }
            Text { text: "OVERALL OBSERVED"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater { model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
            Item { height: 10; width: 1 }
            Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true
                   text: "Measured training data only. This is not an official competition result. Sighters are excluded from all results." }
        }
    }
}
