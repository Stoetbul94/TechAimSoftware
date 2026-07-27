import QtQuick 2.15

// Call & Diagnose (T2) — branded PDF report renderer. Off-screen A4 pages
// built ENTIRELY from CALLDIAG.reportModel(). Reuses the Tech Aim report
// branding (colour wordmark header, WE AIM TO PLEASE footer, page numbers).
// Measured language only — no diagnostic-cause claims, no ranking wording.
Item {
    id: view
    property var ctl: null                 // CALLDIAG
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
    readonly property color blue:   "#2E6FB0"

    property var model: ({})
    property var shots: []

    readonly property int totalPages: 3 + Math.max(1, Math.ceil((shots ? shots.length : 0) / 6))

    function _pageItems() {
        var items = [overviewPage, analyticsPage]
        for (var i = 0; i < shotPages.count; ++i) items.push(shotPages.itemAt(i))
        items.push(notesPage)
        return items
    }

    property int _idx: 0
    property var _pages: []
    property string _path: ""

    function exportPdf(path) {
        if (!ctl) { failed("No Call & Diagnose session"); return }
        view.model = ctl.reportModel()
        view.shots = view.model.shots ? view.model.shots : []
        _path = path
        CUSTOMPRINT.clearImagesList()
        _pages = _pageItems()
        _idx = 0
        Qt.callLater(grabTimer.start)
    }

    Timer { id: grabTimer; interval: 60; repeat: false; onTriggered: view._grabNext() }

    function _grabNext() {
        if (_idx >= _pages.length) {
            var ok = CUSTOMPRINT.createTrainingPdf(_path)   // shared branded A4 writer
            if (ok) exported(_path); else failed("PDF could not be written")
            return
        }
        var pg = _pages[_idx]
        pg.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image); _idx += 1; grabTimer.restart() },
                       Qt.size(pageW, pageH))
    }

    // shots grouped into pages of 6
    function _shotPage(i) {
        var out = []
        for (var k = i * 6; k < Math.min((i + 1) * 6, shots.length); ++k) out.push(shots[k])
        return out
    }

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
                            Text { text: "CALL & DIAGNOSE REPORT"; color: view.ink; font.pixelSize: 15; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
                Column {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Repeater {
                        model: [
                            { l: "Athlete",  v: view.model.athlete || "—" },
                            { l: "Discipline", v: view.model.threePositions ? "50m Rifle 3P" : "Single position" },
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
                Image { source: "qrc:/images/logo/techaim_color.png"; height: 12
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
        Text { text: k; color: view.sub; font.pixelSize: 11; width: 200 }
        Text { text: v; color: view.ink; font.pixelSize: 11; font.bold: true; width: 480; wrapMode: Text.WordWrap }
    }

    // mini called-vs-actual target
    component MiniTarget: Rectangle {
        property var s: ({})
        width: 96; height: 96; color: "#FBFBFC"; border.color: view.line; border.width: 1
        readonly property real cx: width / 2
        readonly property real cy: height / 2
        readonly property real plotR: width / 2 - 8
        readonly property real rangeMm: {
            var mx = 6
            if (s && s.errorMm !== undefined) mx = Math.max(mx, Math.abs(s.errorXMm), Math.abs(s.errorYMm)) * 1.4
            return mx
        }
        Rectangle { anchors.centerIn: parent; width: parent.plotR * 2; height: 1; color: "#ECEEF1" }
        Rectangle { anchors.centerIn: parent; width: 1; height: parent.plotR * 2; color: "#ECEEF1" }
        // actual at centre (reference); call offset by (−errorX, −errorY) relative
        Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 2; rotation: 45; color: view.red; border.color: "white"; border.width: 1 }
        Rectangle {
            visible: s && s.errorMm !== undefined
            width: 10; height: 10; radius: 6; color: "transparent"; border.color: view.blue; border.width: 2
            x: parent.cx + ((s.errorXMm || 0) / parent.rangeMm) * parent.plotR - width / 2
            y: parent.cy - ((s.errorYMm || 0) / parent.rangeMm) * parent.plotR - height / 2
        }
    }

    // ── PAGE 1 — OVERVIEW ─────────────────────────────────────────────────
    ReportPage {
        id: overviewPage; pageNo: 1
        Column {
            width: parent.width; spacing: 8
            Text { text: "SESSION OVERVIEW"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Item { height: 4; width: 1 }
            KV { k: "Athlete";        v: view.model.athlete || "—" }
            KV { k: "Date / time";    v: view.model.createdAtIso || "—" }
            KV { k: "Session ID";     v: view.model.sessionId || "—" }
            KV { k: "Operating mode"; v: view.model.operatingMode || "—" }
            KV { k: "Technical focus"; v: view.model.focus || "—" }
            KV { k: "Programme";      v: (view.model.shotCount || 0) + " called shots"
                                         + (view.model.threePositions ? " per position" : "") }
            KV { k: "Called shots";   v: "" + (view.model.calledShots || 0) }
            KV { k: "Sighters";       v: "" + (view.model.sighters || 0) + " (excluded from results)" }
            KV { k: "Duration";       v: {
                    var d = view.model.durationSec || 0
                    return Math.floor(d / 60) + " min " + ("0" + (d % 60)).slice(-2) + " s"
                } }
            Item { height: 8; width: 1 }
            Text { text: "CALL ACCURACY"; color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
            Grid {
                columns: 3; columnSpacing: 40; rowSpacing: 6
                property var s: view.model.stats || {}
                Repeater {
                    model: {
                        var s = view.model.stats || {}
                        if (s.count === undefined || s.count === 0) return [{ l: "Called shots", v: "0" }]
                        return [
                            { l: "Average error", v: Number(s.averageError).toFixed(1) + " mm" },
                            { l: "Median error",  v: Number(s.medianError).toFixed(1) + " mm" },
                            { l: "Best call",     v: Number(s.smallestError).toFixed(1) + " mm" },
                            { l: "Largest",       v: Number(s.largestError).toFixed(1) + " mm" },
                            { l: "Consistency SD", v: Number(s.errorStdDev).toFixed(2) },
                            { l: "Direction bias", v: Math.abs(s.biasX).toFixed(1) + (s.biasX >= 0 ? " R" : " L")
                                                     + " · " + Math.abs(s.biasY).toFixed(1) + (s.biasY >= 0 ? " Hi" : " Lo") }
                        ]
                    }
                    delegate: Column { width: 210; spacing: 1
                        Text { text: modelData.l; color: view.sub; font.pixelSize: 10 }
                        Text { text: modelData.v; color: view.ink; font.family: "Consolas"; font.pixelSize: 14; font.bold: true } }
                }
            }
            Item { height: 8; width: 1 }
            Text { text: "WHAT YOU SHOULD TAKE FROM THIS SESSION"; color: view.red; font.pixelSize: 12; font.bold: true }
            Repeater {
                model: {
                    var i = view.model.insights || {}
                    if (!i.hasData) return []
                    var m = []
                    m.push({ t: "Typical call accuracy", b: (i.typicalText || "") + "  " + (i.withinText || "") })
                    m.push({ t: "Average vs typical", b: i.averageVsMedian || "" })
                    m.push({ t: "Directional bias", b: (i.biasSentences || []).join("  ") + "  " + (i.biasCaveat || "") })
                    m.push({ t: "Call consistency", b: i.consistencyText || "" })
                    m.push({ t: "Trend", b: i.trendText || "" })
                    m.push({ t: "Awareness vs result", b: i.awarenessText || "" })
                    return m
                }
                delegate: Column { width: 700; spacing: 1
                    Text { text: modelData.t; color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { width: 700; wrapMode: Text.WordWrap; text: modelData.b; color: view.ink; font.pixelSize: 11 } }
            }
            Item { height: 6; width: 1 }
            Text { visible: view.model.insights !== undefined && (view.model.insights.reviewShots || []).length > 0
                   text: "SHOTS TO REVIEW"; color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
            Repeater { model: (view.model.insights ? (view.model.insights.reviewShots || []) : [])
                Text { width: 700; wrapMode: Text.WordWrap; text: "· Shot " + modelData.shotNumber + ": " + modelData.text
                       color: view.ink; font.pixelSize: 11 } }
        }
    }

    // ── PAGE 2 — ANALYTICS ────────────────────────────────────────────────
    ReportPage {
        id: analyticsPage; pageNo: 2
        Column {
            width: parent.width; spacing: 8
            Text { text: "CALL ERROR BY SHOT"; color: view.ink; font.pixelSize: 15; font.bold: true }
            // simple bar chart of radial error per shot
            Column {
                width: parent.width; spacing: 3
                readonly property real maxErr: {
                    var mx = 1
                    for (var i = 0; i < view.shots.length; ++i) mx = Math.max(mx, view.shots[i].errorMm)
                    return mx
                }
                Repeater {
                    model: view.shots
                    Row { spacing: 8; property var s: modelData
                        Text { text: (s.positionName ? s.positionName.substring(0,1) + " " : "") + "#" + s.shotNumber
                               color: view.sub; font.pixelSize: 9; width: 46 }
                        Rectangle { width: 480; height: 11; color: "#EEEFF2"; anchors.verticalCenter: parent.verticalCenter
                            Rectangle { height: parent.height; color: view.red; radius: 2
                                width: parent.width * Math.min(1, s.errorMm / parent.parent.parent.maxErr) } }
                        Text { text: Number(s.errorMm).toFixed(1) + " mm"; color: view.ink; font.family: "Consolas"; font.pixelSize: 9
                               anchors.verticalCenter: parent.verticalCenter } }
                }
            }
            Item { height: 10; width: 1 }
            Text { text: "DIRECTIONAL BIAS (call − actual)"; color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            Text {
                width: 700; wrapMode: Text.WordWrap; color: view.ink; font.pixelSize: 11
                text: {
                    var s = view.model.stats || {}
                    if (s.count === undefined || s.count === 0) return "No called shots."
                    return "Mean call offset: " + Math.abs(s.biasX).toFixed(1) + " mm " + (s.biasX >= 0 ? "right" : "left")
                         + ", " + Math.abs(s.biasY).toFixed(1) + " mm " + (s.biasY >= 0 ? "high" : "low")
                         + ".  Average |X| " + Number(s.avgAbsX).toFixed(1) + " mm, average |Y| " + Number(s.avgAbsY).toFixed(1) + " mm."
                }
            }
            // 3P position breakdown
            Column {
                width: parent.width; spacing: 3; visible: view.model.threePositions === true
                Text { text: "BY POSITION"; color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; topPadding: 6 }
                Repeater {
                    model: view.model.byPosition ? view.model.byPosition : []
                    Text { width: 700; color: view.ink; font.pixelSize: 11
                           text: (modelData.positionName || "") + ": " + (modelData.count || 0) + " calls · avg "
                                 + (modelData.count > 0 ? Number(modelData.averageError).toFixed(1) + " mm" : "—") }
                }
            }
        }
    }

    // ── SHOT PAGES (6 per page) ───────────────────────────────────────────
    Repeater {
        id: shotPages
        model: Math.max(1, Math.ceil((view.shots ? view.shots.length : 0) / 6))
        ReportPage {
            pageNo: 3 + index
            property var pageShots: view._shotPage(index)
            Column {
                width: parent.width; spacing: 8
                Text { text: "CALLED vs ACTUAL — shots " + (index * 6 + 1) + "–" + (index * 6 + Math.max(1, pageShots.length))
                       color: view.ink; font.pixelSize: 15; font.bold: true }
                Grid {
                    columns: 3; columnSpacing: 16; rowSpacing: 14; width: parent.width
                    Repeater {
                        model: parent.parent.pageShots
                        Row { spacing: 10; property var s: modelData
                            MiniTarget { s: parent.s }
                            Column { spacing: 1; width: 120
                                Text { text: (s.positionName ? s.positionName + " · " : "") + "Shot " + s.shotNumber
                                       color: view.ink; font.pixelSize: 11; font.bold: true }
                                Text { text: "Score " + Number(s.actualScore).toFixed(1); color: view.sub; font.pixelSize: 10 }
                                Text { text: "Call error " + Number(s.errorMm).toFixed(1) + " mm"; color: view.ink; font.pixelSize: 10 }
                                Text { text: Math.abs(s.errorXMm).toFixed(1) + (s.errorXMm >= 0 ? " R" : " L")
                                             + " · " + Math.abs(s.errorYMm).toFixed(1) + (s.errorYMm >= 0 ? " Hi" : " Lo")
                                       color: view.sub; font.pixelSize: 10; font.family: "Consolas" }
                                Text { visible: s.note && s.note.length > 0; text: s.note || ""; color: view.sub; font.pixelSize: 9
                                       width: 120; wrapMode: Text.WordWrap } }
                        }
                    }
                }
                Row { spacing: 8
                    Rectangle { width: 11; height: 11; radius: 6; color: "transparent"; border.color: view.blue; border.width: 2; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "CALL"; color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter }
                    Item { width: 12; height: 1 }
                    Rectangle { width: 9; height: 9; radius: 2; rotation: 45; color: view.red; border.color: "white"; border.width: 1; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "ACTUAL (centre)"; color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter } }
            }
        }
    }

    // ── FINAL — NOTES & OBSERVATIONS ──────────────────────────────────────
    ReportPage {
        id: notesPage; pageNo: view.totalPages
        Column {
            width: parent.width; spacing: 8
            Text { text: "NOTES & OBSERVATIONS"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Text { visible: (view.model.sessionNote || "").length > 0; text: "Session note: " + (view.model.sessionNote || "")
                   color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap }
            Repeater {
                model: view.shots
                Column { width: 700; spacing: 1; visible: modelData.note && modelData.note.length > 0
                    property var s: modelData
                    Text { text: (s.positionName ? s.positionName + " · " : "") + "Shot " + s.shotNumber
                           color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { text: s.note || ""; color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap } }
            }
            Item { height: 8; width: 1 }
            Text { text: "OVERALL OBSERVED"; color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater { model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
            Item { height: 10; width: 1 }
            Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true
                   text: "Measured shot-awareness data only. This is not an official competition result and carries no ranking status. Sighters are excluded from all results." }
        }
    }
}
