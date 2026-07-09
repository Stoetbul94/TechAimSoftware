import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

// Polished, light-themed Coach Report dashboard (print-style), matching the
// approved mockup: ISSF target shot-map row, score-distribution donut, trend
// chart, summary stats, and a per-position table. DISPLAY ONLY — every value
// comes from COACHREPORT; nothing is recomputed or fabricated here.
Rectangle {
    id: dash
    color: "#f7f4ee"

    property int gameSubMode: 0
    property var rep: COACHREPORT.report
    property var targets: []

    signal closed()
    signal detailsRequested()
    signal printRequested()

    // Hidden internal toolbar when hosted in a FloatingWindow (window supplies
    // title bar + tabs + actions). Content/analytics unchanged.
    property bool embedded: false

    // ---- light palette ----
    readonly property color cPanel:  "#ffffff"
    readonly property color cBorder: "#e6e2d8"
    readonly property color cText:   "#1c1c1e"
    readonly property color cSub:    "#6c6c72"
    readonly property color cAccent: "#a80038"
    readonly property color cGreen:  "#2e9e5b"
    readonly property color cAmber:  "#e0a020"
    readonly property color cRed:    "#d0392b"
    readonly property string fam: "Segoe UI"

    Connections {
        target: COACHREPORT
        function onReportChanged() { dash.refresh() }
    }
    Component.onCompleted: refresh()

    // ---- helpers ----
    function f(x, d) {
        if (x === undefined || x === null) return "—"
        if (typeof x !== "number") return String(x)
        return x.toFixed(d === undefined ? 2 : d)
    }
    function pct(x) { return (x === undefined || x === null) ? "—" : f(x, 1) + "%" }
    // Match the engine's analysed set exactly, so plotted shots == report counts.
    function competition(s) { return s.isValid && s.isCompetitionShot && !s.isSighter }

    // Discipline is derived from the report's own positions (single source),
    // not from any feeder debug info.
    function positionsPresent() {
        var out = []
        if (rep && rep.positionAnalysis && rep.positionAnalysis.positions)
            for (var i = 0; i < rep.positionAnalysis.positions.length; i++)
                out.push(rep.positionAnalysis.positions[i].position)
        return out
    }
    function specKey() {
        var p = positionsPresent()
        if (p.indexOf("airPistol") >= 0) return "airpistol10"
        if (p.indexOf("airRifle") >= 0)  return "airrifle10"
        return "rifle50"
    }
    function disciplineLabel() {
        var p = positionsPresent()
        if (p.indexOf("airPistol") >= 0) return "Air Pistol (10m)"
        if (p.indexOf("airRifle") >= 0)  return "Air Rifle (10m)"
        if (p.indexOf("kneeling") >= 0 && p.indexOf("standing") >= 0 && p.indexOf("prone") >= 0)
            return "3-Position Rifle (50m)"
        if (p.indexOf("prone") >= 0 && p.length === 1) return "Prone Rifle (50m)"
        return "50m Rifle"
    }
    function trendWord(slope) {
        if (slope === undefined || slope === null) return "—"
        if (slope > 0.005) return "Improving"
        if (slope < -0.005) return "Declining"
        return "Stable"
    }
    function trendColor(slope) {
        if (slope > 0.005) return cGreen
        if (slope < -0.005) return cRed
        return cSub
    }
    function up(x) { return x ? String(x).toUpperCase() : "—" }

    // ---- Key Coach Insights (compact summaries; engine values only) ----
    function limitingPosition(pa) {
        if (!pa || !pa.available) return "—"
        if (pa.comparativeAvailable && pa.hasWeakest) return up(pa.weakestPosition)
        if (pa.positions.length === 1) return up(pa.positions[0].position)
        return "—"
    }
    function fatigueLevel(fa) {
        if (!fa || !fa.available || !fa.scoreTrendAvailable) return "N/A"
        if (fa.overallPattern === "NoFatigueDetected") return "None"
        var i = fa.fatigueIndex
        return i < 0.35 ? "Mild" : (i < 0.55 ? "Moderate" : "High")
    }
    function fatigueColor(fa) {
        var l = fatigueLevel(fa)
        return l === "None" ? cGreen : (l === "Mild" ? cAmber : (l === "Moderate" ? "#e07a30" : (l === "High" ? cRed : cSub)))
    }
    function recoveryRating(ra) {
        if (!ra || !ra.overall || !ra.overall.available) return "N/A"
        switch (ra.overall.pattern) {
        case "GoodRecovery":          return "Good"
        case "SlowRecovery":          return "Slow"
        case "RepeatedErrorPattern":  return "Clustered errors"
        case "OverCorrectionPattern": return "Over-correcting"
        default:                      return "N/A"
        }
    }
    function recoveryColor(ra) {
        var r = recoveryRating(ra)
        return r === "Good" ? cGreen : (r === "N/A" ? cSub : (r === "Slow" ? cAmber : cRed))
    }
    function errorDirection(ex) {
        if (!ex) return "—"
        var x = ex.horizontalBias, y = ex.verticalBias
        var r = Math.sqrt(x * x + y * y)
        if (r < 1.0) return "Centred"
        var ang = Math.atan2(y, x) * 180 / Math.PI
        if (ang < 0) ang += 360
        var dirs = ["Right", "High-right", "High", "High-left", "Left", "Low-left", "Low", "Low-right"]
        return dirs[Math.round(ang / 45) % 8]
    }
    function trainingFocus(tp) {
        if (!tp || tp.length === 0) return "None detected"
        return tp[0].priority
    }
    function insightTiles() {
        var r = rep
        if (!r || !r.valid) return []
        return [
            { label: "Main limiting position", value: limitingPosition(r.positionAnalysis), color: cText },
            { label: "Fatigue level",          value: fatigueLevel(r.fatigueAnalysis),      color: fatigueColor(r.fatigueAnalysis) },
            { label: "Recovery rating",        value: recoveryRating(r.recoveryAnalysis),   color: recoveryColor(r.recoveryAnalysis) },
            { label: "Dominant error dir.",    value: errorDirection(r.executiveSummary),   color: cText },
            { label: "Main training focus",    value: trainingFocus(r.trainingPriorities),  color: cAccent }
        ]
    }

    function refresh() {
        // group shots by position (first-appearance order = K, P, S for 3P)
        var all = COACHREPORT.shots().filter(competition)
        var statFor = {}
        var pos = (rep && rep.positionAnalysis) ? rep.positionAnalysis.positions : []
        for (var i = 0; i < pos.length; i++) {
            var p = pos[i]
            statFor[p.position] = { avg: p.measurements.averageScore, sd: p.measurements.scoreSD,
                                    mr: p.measurements.geometry ? p.measurements.geometry.meanRadius : 0 }
        }
        var groups = {}, order = []
        for (var j = 0; j < all.length; j++) {
            var k = all[j].position
            if (!groups[k]) { groups[k] = []; order.push(k) }
            groups[k].push(all[j])
        }
        var list = []
        if (order.length > 1)
            for (var m = 0; m < order.length; m++)
                list.push({ title: order[m], shots: groups[order[m]], stat: statFor[order[m]] })
        var ex = rep ? rep.executiveSummary : null
        list.push({ title: "All Shots", shots: all,
                    stat: ex ? { avg: ex.averageScore, sd: ex.scoreStandardDeviation, mr: ex.groupRadius } : null })
        dash.targets = list
        rebuildDist(); rebuildTrend()
    }

    function rebuildDist() {
        if (typeof distPie === "undefined" || !distPie) return
        distPie.clear()
        var d = rep ? rep.shotDistribution : null
        if (!d) return
        function add(label, val, col) { if (val > 0) { var s = distPie.append(label, val); s.color = col; s.labelVisible = false } }
        add("Perfect", d.perfectCount, "#2e9e5b")
        add("Excellent", d.excellentCount, "#5bc07e")
        add("Good", d.goodCount, "#e0a020")
        add("Acceptable", d.acceptableCount, "#eec25a")
        add("Recovery", d.recoveryCount, "#e07a30")
        add("Poor", d.poorCount, "#d0392b")
    }
    function rebuildTrend() {
        if (typeof scoreSeries === "undefined" || !scoreSeries) return
        scoreSeries.clear(); rollSeries.clear()
        var t = rep ? rep.trendAnalysis : null
        if (!t || !t.scores || t.scores.length === 0) return
        var n = t.scores.length
        for (var i = 0; i < n; i++) scoreSeries.append(i + 1, t.scores[i])
        if (t.rolling5Available && t.rolling5Average)
            for (var j = 0; j < t.rolling5Average.length; j++) rollSeries.append(j + 5, t.rolling5Average[j])
        axX.min = 1; axX.max = Math.max(2, n)
    }

    // ================= header =================
    Rectangle {
        id: head
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        visible: !dash.embedded; height: dash.embedded ? 0 : 70; color: cPanel; border.color: cBorder; border.width: 1
        Column {
            anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 20; spacing: 3
            Text { text: "Coach Report"; color: cText; font.bold: true; font.pixelSize: 24; font.family: fam }
            Text {
                color: cSub; font.pixelSize: 13; font.family: fam
                text: "Discipline: " + dash.disciplineLabel()
                      + "    |    Athlete: " + (typeof userName !== "undefined" ? userName : "—")
                      + "    |    " + Qt.formatDateTime(new Date(), "MMM d, yyyy HH:mm")
            }
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 16; spacing: 12
            Button {
                text: (COACHFEED.coordinatesFlipY ? "Flip Y ✓" : "Flip Y (Invert)")
                onClicked: { COACHFEED.coordinatesFlipY = !COACHFEED.coordinatesFlipY; COACHFEED.analyzeCurrentMatch(dash.gameSubMode) }
            }
            Button { text: "Re-run"; onClicked: COACHFEED.analyzeCurrentMatch(dash.gameSubMode) }
            Button { text: "Detailed ▸"; onClicked: dash.detailsRequested() }
            Button { text: "🖶 Print"; onClicked: dash.printRequested() }
            Button { text: "Close"; onClicked: dash.closed() }
        }
    }

    Flickable {
        anchors.top: head.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.margins: 14
        clip: true
        contentWidth: width
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

        Column {
            id: col
            width: parent.width - 14
            spacing: 14

            Text {
                width: parent.width
                visible: !(dash.rep && dash.rep.valid)
                text: "No report to display yet — finish a match, then open Coach Report."
                color: cSub; font.pixelSize: 15; font.family: fam; wrapMode: Text.WordWrap
            }

            // ---------------- Key Coach Insights ----------------
            Rectangle {
                width: parent.width; visible: dash.rep && dash.rep.valid
                implicitHeight: kciCol.implicitHeight + 26; height: implicitHeight
                color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                Column {
                    id: kciCol; x: 16; y: 12; width: parent.width - 32; spacing: 10
                    Text { text: "✦  Key Coach Insights"; color: cText; font.bold: true; font.pixelSize: 16; font.family: fam }
                    Row {
                        width: parent.width
                        Repeater {
                            model: dash.insightTiles()
                            delegate: Column {
                                width: kciCol.width / 5; spacing: 4
                                Text { width: parent.width - 10; text: modelData.label; color: dash.cSub; font.pixelSize: 12; font.family: dash.fam; wrapMode: Text.WordWrap }
                                Text { width: parent.width - 10; text: modelData.value; color: modelData.color; font.bold: true; font.pixelSize: 16; font.family: dash.fam; wrapMode: Text.WordWrap }
                            }
                        }
                    }
                }
            }

            // ---------------- Shot Map ----------------
            Rectangle {
                width: parent.width; visible: dash.rep && dash.rep.valid
                implicitHeight: shotCol.implicitHeight + 28; height: implicitHeight
                color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                Column {
                    id: shotCol; x: 16; y: 14; width: parent.width - 32; spacing: 12
                    Text { text: "◎  Shot Map"; color: cText; font.bold: true; font.pixelSize: 18; font.family: fam }
                    Row {
                        width: parent.width; spacing: 16
                        // targets
                        Flow {
                            width: parent.width - 230; spacing: 16
                            Repeater {
                                model: dash.targets
                                delegate: Column {
                                    spacing: 4
                                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.title.toUpperCase() + "  (" + modelData.shots.length + ")"; color: cText; font.bold: true; font.pixelSize: 13; font.family: fam }
                                    IssfTargetCanvas { shots: modelData.shots; side: 210; discipline: dash.specKey() }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        color: cSub; font.pixelSize: 12; font.family: fam
                                        text: modelData.stat ? ("Avg " + dash.f(modelData.stat.avg, 2) + "   SD " + dash.f(modelData.stat.sd, 2) + "   MR " + dash.f(modelData.stat.mr, 1) + "mm") : ""
                                    }
                                }
                            }
                            // legend + target info
                            Column {
                                width: 200; spacing: 10
                                Text { text: "Score (legend)"; color: cText; font.bold: true; font.pixelSize: 13; font.family: fam }
                                Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: cGreen; anchors.verticalCenter: parent.verticalCenter } Text { text: "≥ 10.5  Perfect / Excellent"; color: cSub; font.pixelSize: 12; font.family: fam } }
                                Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: cAmber; anchors.verticalCenter: parent.verticalCenter } Text { text: "10.0 – 10.4  Good / Acceptable"; color: cSub; font.pixelSize: 12; font.family: fam } }
                                Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: cRed; anchors.verticalCenter: parent.verticalCenter } Text { text: "< 10.0  Recovery / Poor"; color: cSub; font.pixelSize: 12; font.family: fam } }
                                Item { width: 1; height: 6 }
                                Text { text: "Target info"; color: cText; font.bold: true; font.pixelSize: 13; font.family: fam }
                                Text { text: "Scale: real ISSF rings (mm)"; color: cSub; font.pixelSize: 12; font.family: fam }
                                Text { text: "+Y = High  (Flip Y to invert)"; color: cSub; font.pixelSize: 12; font.family: fam }
                            }
                        }
                    }
                }
            }

            // ---------------- Distribution + Trend + Summary ----------------
            Row {
                width: parent.width; visible: dash.rep && dash.rep.valid; spacing: 14

                // ---- Score Distribution ----
                Rectangle {
                    width: (parent.width - 28) * 0.33; height: 320
                    color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                    Column {
                        x: 16; y: 14; width: parent.width - 32; spacing: 8
                        Text { text: "▦  Score Distribution"; color: cText; font.bold: true; font.pixelSize: 16; font.family: fam }
                        ChartView {
                            width: parent.width; height: 250
                            antialiasing: true; legend.visible: false; backgroundColor: "transparent"; plotAreaColor: "transparent"
                            margins.top: 0; margins.bottom: 0; margins.left: 0; margins.right: 0
                            PieSeries { id: distPie; holeSize: 0.55; size: 0.9 }
                            Text {
                                anchors.centerIn: parent
                                text: (dash.rep ? dash.f(dash.rep.executiveSummary.competitionShotCount, 0) : "0") + "\nShots"
                                horizontalAlignment: Text.AlignHCenter; color: dash.cText; font.bold: true; font.pixelSize: 16; font.family: dash.fam
                            }
                            Component.onCompleted: dash.rebuildDist()
                        }
                    }
                }

                // ---- Trend ----
                Rectangle {
                    width: (parent.width - 28) * 0.40; height: 320
                    color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                    Column {
                        x: 16; y: 14; width: parent.width - 32; spacing: 6
                        Text { text: "📈  Trend"; color: cText; font.bold: true; font.pixelSize: 16; font.family: fam }
                        ChartView {
                            width: parent.width; height: 250
                            antialiasing: true; legend.visible: true; legend.alignment: Qt.AlignTop
                            backgroundColor: "transparent"; plotAreaColor: "transparent"
                            margins.top: 0; margins.bottom: 0; margins.left: 0; margins.right: 0
                            ValueAxis { id: axX; min: 1; max: 10; labelFormat: "%d" }
                            ValueAxis { id: axY; min: 8; max: 11; labelFormat: "%.1f" }
                            LineSeries { id: scoreSeries; name: "Shot Score"; axisX: axX; axisY: axY; color: "#2f6fd0"; width: 2 }
                            LineSeries { id: rollSeries; name: "Rolling Avg (5)"; axisX: axX; axisY: axY; color: dash.cAccent; width: 2 }
                            Component.onCompleted: dash.rebuildTrend()
                        }
                    }
                }

                // ---- Summary ----
                Rectangle {
                    width: (parent.width - 28) * 0.27; height: 320
                    color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                    Column {
                        x: 16; y: 14; width: parent.width - 32; spacing: 6
                        Text { text: "◎  Summary"; color: cText; font.bold: true; font.pixelSize: 16; font.family: fam }
                        Repeater {
                            model: dash.summaryRows()
                            delegate: Row {
                                width: parent.width
                                Text { width: parent.width * 0.62; text: modelData.k; color: dash.cSub; font.pixelSize: 13; font.family: dash.fam }
                                Text { text: modelData.v; color: dash.cText; font.bold: true; font.pixelSize: 13; font.family: dash.fam }
                            }
                        }
                    }
                }
            }

            // ---------------- By Position ----------------
            Rectangle {
                width: parent.width
                visible: dash.rep && dash.rep.valid && dash.rep.positionAnalysis && dash.rep.positionAnalysis.positions.length > 1
                implicitHeight: posCol.implicitHeight + 28; height: implicitHeight
                color: cPanel; border.color: cBorder; border.width: 1; radius: 10
                Column {
                    id: posCol; x: 16; y: 14; width: parent.width - 32; spacing: 8
                    Text { text: "By Position Summary"; color: cText; font.bold: true; font.pixelSize: 16; font.family: fam }
                    // header row
                    Row {
                        width: parent.width
                        Repeater {
                            model: ["Position","Shots","Total","Avg","Best","Worst","SD","MR (mm)","Diameter","1st Half","2nd Half","Slope","Trend"]
                            delegate: Text { width: (index === 0) ? posCol.width * 0.12 : posCol.width * 0.073; text: modelData; color: dash.cSub; font.bold: true; font.pixelSize: 12; font.family: dash.fam }
                        }
                    }
                    Repeater {
                        model: (dash.rep && dash.rep.positionAnalysis) ? dash.rep.positionAnalysis.positions : []
                        delegate: Row {
                            width: parent.width
                            property var m: modelData.measurements
                            Text { width: posCol.width * 0.12; text: modelData.position.toUpperCase(); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(modelData.shotCount, 0); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.totalScore, 1); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.averageScore, 2); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.maxScore, 1); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.minScore, 1); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.scoreSD, 2); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.geometry ? m.geometry.meanRadius : undefined, 1); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.geometry ? m.geometry.groupDiameter : undefined, 1); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.firstHalfAverage, 2); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.secondHalfAverage, 2); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.f(m.trendSlope, 3); color: dash.cText; font.pixelSize: 12; font.family: dash.fam }
                            Text { width: posCol.width * 0.073; text: dash.trendWord(m.trendSlope); color: dash.trendColor(m.trendSlope); font.bold: true; font.pixelSize: 12; font.family: dash.fam }
                        }
                    }
                }
            }

            Item { width: 1; height: 12 }
        }
    }

    // summary rows built from the executive summary
    function summaryRows() {
        var e = rep ? rep.executiveSummary : null
        if (!e) return []
        return [
            { k: "Total Shots",       v: f(e.competitionShotCount, 0) },
            { k: "Total Score",       v: f(e.totalScore, 1) },
            { k: "Average Score",     v: f(e.averageScore, 2) },
            { k: "Best Score",        v: f(e.bestShot, 1) },
            { k: "Worst Score",       v: f(e.worstShot, 1) },
            { k: "Score SD",          v: f(e.scoreStandardDeviation, 2) },
            { k: "Group Mean Radius", v: f(e.groupRadius, 1) + " mm" },
            { k: "Group Diameter",    v: f(e.groupDiameter, 1) + " mm" },
            { k: "Horizontal SD",     v: f(e.horizontalSD, 1) + " mm" },
            { k: "Vertical SD",       v: f(e.verticalSD, 1) + " mm" }
        ]
    }
}
