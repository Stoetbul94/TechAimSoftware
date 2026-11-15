import QtQuick 2.15

// Call & Diagnose (T2) — branded PDF report renderer. Off-screen A4 pages
// built ENTIRELY from CALLDIAG.reportModel(). Reuses the Tech Aim report
// branding (colour wordmark header, WE AIM TO PLEASE footer, page numbers).
// Measured language only — no diagnostic-cause claims, no ranking wording.
Item {
    id: view
    // LOC-REPORT-001: the controller supplies English position names; the
    // report translates them at display time (unknown names unchanged).
    function trPos(n) {
        switch (n) {
        case "Kneeling": return qsTr("Kneeling")
        case "Prone":    return qsTr("Prone")
        case "Standing": return qsTr("Standing")
        }
        return n || ""
    }
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
    readonly property color red:    theme.tokens.accentHover
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
        if (!ctl) { failed(qsTr("No Call & Diagnose session")); return }
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
                        Image { source: PRODUCT.brandLogoPath; height: 60
                            width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                            fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true }
                        Row { spacing: 8
                            Text { text: qsTr("TRAINING LAB"); color: view.red; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                                   anchors.verticalCenter: parent.verticalCenter }
                            Text { text: qsTr("CALL & DIAGNOSE REPORT"); color: view.ink; font.pixelSize: 15; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
                Column {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Repeater {
                        model: [
                            { l: qsTr("Athlete"),  v: view.model.athlete || "—" },
                            // CD-REPORT-002: name the discipline (from the
                            // controller), then the layout only where it adds
                            // something. "Single position" alone told the
                            // reader nothing about what was shot.
                            { l: qsTr("Discipline"), v: (view.model.discipline || "—")
                                                  + (view.model.threePositions ? qsTr(" (K/P/S)") : "") },
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
                Image { source: PRODUCT.brandLogoPath; height: 12
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
            Text { text: qsTr("SESSION OVERVIEW"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Item { height: 4; width: 1 }
            KV { k: qsTr("Athlete");        v: view.model.athlete || "—" }
            KV { k: qsTr("Date / time");    v: view.model.createdAtIso || "—" }
            KV { k: qsTr("Session ID");     v: view.model.sessionId || "—" }
            KV { k: qsTr("Operating mode"); v: view.model.operatingMode || "—" }
            KV { k: qsTr("Technical focus"); v: view.model.focus || "—" }
            KV { k: qsTr("Programme");      v: (view.model.threePositions ? qsTr("%1 called shots per position") : qsTr("%1 called shots"))
                                         .arg(view.model.shotCount || 0) }
            KV { k: qsTr("Called shots");   v: "" + (view.model.calledShots || 0) }
            KV { k: qsTr("Sighters");       v: qsTr("%1 (excluded from results)").arg(view.model.sighters || 0) }
            KV { k: qsTr("Duration");       v: {
                    var d = view.model.durationSec || 0
                    return Math.floor(d / 60) + " min " + ("0" + (d % 60)).slice(-2) + " s"
                } }
            Item { height: 8; width: 1 }
            Text { text: qsTr("CALL ACCURACY"); color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
            Grid {
                columns: 3; columnSpacing: 40; rowSpacing: 6
                property var s: view.model.stats || {}
                Repeater {
                    model: {
                        var s = view.model.stats || {}
                        if (s.count === undefined || s.count === 0) return [{ l: qsTr("Called shots"), v: "0" }]
                        return [
                            { l: qsTr("Average error"), v: Number(s.averageError).toFixed(1) + " mm" },
                            { l: qsTr("Median error"),  v: Number(s.medianError).toFixed(1) + " mm" },
                            { l: qsTr("Best call"),     v: Number(s.smallestError).toFixed(1) + " mm" },
                            { l: qsTr("Largest"),       v: Number(s.largestError).toFixed(1) + " mm" },
                            { l: qsTr("Consistency SD"), v: Number(s.errorStdDev).toFixed(2) },
                            { l: qsTr("Direction bias"), v: Math.abs(s.biasX).toFixed(1) + (s.biasX >= 0 ? qsTr(" R") : qsTr(" L"))
                                                     + " · " + Math.abs(s.biasY).toFixed(1) + (s.biasY >= 0 ? qsTr(" Hi") : qsTr(" Lo")) }
                        ]
                    }
                    delegate: Column { width: 210; spacing: 1
                        Text { text: modelData.l; color: view.sub; font.pixelSize: 10 }
                        Text { text: modelData.v; color: view.ink; font.family: "Consolas"; font.pixelSize: 14; font.bold: true } }
                }
            }
            Item { height: 8; width: 1 }
            Text { text: qsTr("WHAT YOU SHOULD TAKE FROM THIS SESSION"); color: view.red; font.pixelSize: 12; font.bold: true }
            Repeater {
                model: {
                    var i = view.model.insights || {}
                    if (!i.hasData) return []
                    var m = []
                    m.push({ t: qsTr("Typical call accuracy"), b: (i.typicalText || "") + "  " + (i.withinText || "") })
                    m.push({ t: qsTr("Average vs typical"), b: i.averageVsMedian || "" })
                    m.push({ t: qsTr("Directional bias"), b: (i.biasSentences || []).join("  ") + "  " + (i.biasCaveat || "") })
                    m.push({ t: qsTr("Call consistency"), b: i.consistencyText || "" })
                    m.push({ t: qsTr("Trend"), b: i.trendText || "" })
                    m.push({ t: qsTr("Awareness vs result"), b: i.awarenessText || "" })
                    return m
                }
                delegate: Column { width: 700; spacing: 1
                    Text { text: modelData.t; color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { width: 700; wrapMode: Text.WordWrap; text: modelData.b; color: view.ink; font.pixelSize: 11 } }
            }
            Item { height: 6; width: 1 }
            Text { visible: view.model.insights !== undefined && (view.model.insights.reviewShots || []).length > 0
                   text: qsTr("SHOTS TO REVIEW"); color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2 }
            Repeater { model: (view.model.insights ? (view.model.insights.reviewShots || []) : [])
                Text { width: 700; wrapMode: Text.WordWrap; text: qsTr("· Shot %1: %2").arg(modelData.shotNumber).arg(modelData.text)
                       color: view.ink; font.pixelSize: 11 } }
        }
    }

    // ── PAGE 2 — ANALYTICS ────────────────────────────────────────────────
    ReportPage {
        id: analyticsPage; pageNo: 2
        Column {
            width: parent.width; spacing: 8
            Text { text: qsTr("CALL ERROR BY SHOT"); color: view.ink; font.pixelSize: 15; font.bold: true }
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
                        Text { text: (s.positionName ? view.trPos(s.positionName).substring(0,1) + " " : "") + "#" + s.shotNumber
                               color: view.sub; font.pixelSize: 9; width: 46 }
                        Rectangle { width: 480; height: 11; color: "#EEEFF2"; anchors.verticalCenter: parent.verticalCenter
                            Rectangle { height: parent.height; color: view.red; radius: 2
                                width: parent.width * Math.min(1, s.errorMm / parent.parent.parent.maxErr) } }
                        Text { text: Number(s.errorMm).toFixed(1) + " mm"; color: view.ink; font.family: "Consolas"; font.pixelSize: 9
                               anchors.verticalCenter: parent.verticalCenter } }
                }
            }
            Item { height: 10; width: 1 }
            Text { text: qsTr("DIRECTIONAL BIAS (call − actual)"); color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            Text {
                width: 700; wrapMode: Text.WordWrap; color: view.ink; font.pixelSize: 11
                text: {
                    var s = view.model.stats || {}
                    if (s.count === undefined || s.count === 0) return qsTr("No called shots.")
                    return qsTr("Mean call offset: %1 mm %2, %3 mm %4.  Average |X| %5 mm, average |Y| %6 mm.")
                         .arg(Math.abs(s.biasX).toFixed(1)).arg(s.biasX >= 0 ? qsTr("right") : qsTr("left"))
                         .arg(Math.abs(s.biasY).toFixed(1)).arg(s.biasY >= 0 ? qsTr("high") : qsTr("low"))
                         .arg(Number(s.avgAbsX).toFixed(1)).arg(Number(s.avgAbsY).toFixed(1))
                }
            }
            // 3P position breakdown
            Column {
                width: parent.width; spacing: 3; visible: view.model.threePositions === true
                Text { text: qsTr("BY POSITION"); color: view.sub; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; topPadding: 6 }
                Repeater {
                    model: view.model.byPosition ? view.model.byPosition : []
                    Text { width: 700; color: view.ink; font.pixelSize: 11
                           text: qsTr("%1: %2 calls · avg %3").arg(view.trPos(modelData.positionName))
                                 .arg(modelData.count || 0)
                                 .arg(modelData.count > 0 ? Number(modelData.averageError).toFixed(1) + " mm" : "—") }
                }
            }
        }
    }

    // ── SHOT PAGES (6 per page) ───────────────────────────────────────────
    Repeater {
        id: shotPages
        model: Math.max(1, Math.ceil((view.shots ? view.shots.length : 0) / 6))
        ReportPage {
            id: shotPage
            pageNo: 3 + index
            property var pageShots: view._shotPage(index)
            Column {
                width: parent.width; spacing: 8
                Text { text: qsTr("CALLED vs ACTUAL — shots %1–%2").arg(index * 6 + 1).arg(index * 6 + Math.max(1, pageShots.length))
                       color: view.ink; font.pixelSize: 15; font.bold: true }
                Grid {
                    columns: 3; columnSpacing: 16; rowSpacing: 14; width: parent.width
                    Repeater {
                        // CD-REPORT-001. This was `parent.parent.pageShots`,
                        // which walks Grid -> Column and stops there - NOT the
                        // page that declares pageShots. The model resolved to
                        // undefined, the Repeater built nothing, and the report
                        // printed a heading and a legend over an empty page
                        // while every per-shot target diagram silently vanished
                        // (observed in the 50 m field test, 2026-08-10).
                        // Address the page by id so no parent count can drift.
                        model: shotPage.pageShots
                        Row { spacing: 10; property var s: modelData
                            MiniTarget { s: parent.s }
                            Column { spacing: 1; width: 120
                                Text { text: (s.positionName ? view.trPos(s.positionName) + " · " : "") + qsTr("Shot %1").arg(s.shotNumber)
                                       color: view.ink; font.pixelSize: 11; font.bold: true }
                                Text { text: qsTr("Score %1").arg(Number(s.actualScore).toFixed(1)); color: view.sub; font.pixelSize: 10 }
                                Text { text: qsTr("Call error %1 mm").arg(Number(s.errorMm).toFixed(1)); color: view.ink; font.pixelSize: 10 }
                                Text { text: Math.abs(s.errorXMm).toFixed(1) + (s.errorXMm >= 0 ? qsTr(" R") : qsTr(" L"))
                                             + " · " + Math.abs(s.errorYMm).toFixed(1) + (s.errorYMm >= 0 ? qsTr(" Hi") : qsTr(" Lo"))
                                       color: view.sub; font.pixelSize: 10; font.family: "Consolas" }
                                Text { visible: s.note && s.note.length > 0; text: s.note || ""; color: view.sub; font.pixelSize: 9
                                       width: 120; wrapMode: Text.WordWrap } }
                        }
                    }
                }
                Row { spacing: 8
                    Rectangle { width: 11; height: 11; radius: 6; color: "transparent"; border.color: view.blue; border.width: 2; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("CALL"); color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter }
                    Item { width: 12; height: 1 }
                    Rectangle { width: 9; height: 9; radius: 2; rotation: 45; color: view.red; border.color: "white"; border.width: 1; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("ACTUAL (centre)"); color: view.sub; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter } }
            }
        }
    }

    // ── FINAL — NOTES & OBSERVATIONS ──────────────────────────────────────
    ReportPage {
        id: notesPage; pageNo: view.totalPages
        Column {
            width: parent.width; spacing: 8
            Text { text: qsTr("NOTES & OBSERVATIONS"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Text { visible: (view.model.sessionNote || "").length > 0; text: qsTr("Session note: %1").arg(view.model.sessionNote || "")
                   color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap }
            Repeater {
                model: view.shots
                Column { width: 700; spacing: 1; visible: modelData.note && modelData.note.length > 0
                    property var s: modelData
                    Text { text: (s.positionName ? view.trPos(s.positionName) + " · " : "") + qsTr("Shot %1").arg(s.shotNumber)
                           color: view.sub; font.pixelSize: 10; font.bold: true }
                    Text { text: s.note || ""; color: view.ink; font.pixelSize: 11; width: 700; wrapMode: Text.WordWrap } }
            }
            Item { height: 8; width: 1 }
            Text { text: qsTr("OVERALL OBSERVED"); color: view.ink; font.pixelSize: 15; font.bold: true }
            Repeater { model: view.model.observations ? view.model.observations : []
                Text { width: 700; wrapMode: Text.WordWrap; text: "· " + modelData; color: view.ink; font.pixelSize: 11 } }
            Item { height: 10; width: 1 }
            Text { width: 700; wrapMode: Text.WordWrap; color: view.sub; font.pixelSize: 10; font.italic: true
                   text: qsTr("Measured shot-awareness data only. This is not an official competition result and carries no ranking status. Sighters are excluded from all results.") }
        }
    }
}
