import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

// Detailed Coach Report view. DISPLAY ONLY — no analytics, no recomputation,
// no fabricated values. Charts/targets are drawn from what COACHREPORT already
// computed (+ COACHREPORT.shots() for the target map). `theme` and COACHREPORT
// resolve via main.qml's context.
Rectangle {
    id: reportPage
    color: theme.bgBase

    property int gameSubMode: 0
    property var rep: COACHREPORT.report

    // Target shot-map data (refreshed on each analysis).
    property var  targets: []
    property real sharedExtent: 0

    // Pure content view — no window chrome. The hosting CoachReportWindow owns
    // the title bar, tabs and footer actions; these intent signals let it react.
    signal closed()
    signal dashboardRequested()
    signal printRequested()

    Connections {
        target: COACHREPORT
        function onReportChanged() {
            reportPage.refreshTargets()
            reportPage.rebuildCharts()
        }
    }
    Component.onCompleted: {
        refreshTargets()
        rebuildCharts()
    }

    // ---------- formatting helpers (display only) ----------
    function f(x, d) {
        if (x === undefined || x === null) return "—"
        if (typeof x !== "number") return String(x)
        return x.toFixed(d === undefined ? 2 : d)
    }
    function pct(x) { return (x === undefined || x === null) ? "—" : f(x, 1) + "%" }
    function up(x) { return x ? String(x).toUpperCase() : "—" }

    function fmtExec(s) {
        if (!s) return "Not available"
        return "Shots: " + f(s.competitionShotCount, 0) + "   Total: " + f(s.totalScore, 1)
            + "   Average: " + f(s.averageScore, 2)
            + "\nBest / Worst: " + f(s.bestShot, 1) + " / " + f(s.worstShot, 1)
            + "   Score SD: " + f(s.scoreStandardDeviation, 3)
            + "\nConsistency: " + pct(s.consistencyPercentage)
            + "\nGroup radius / diameter: " + f(s.groupRadius, 2) + " / " + f(s.groupDiameter, 2) + " mm"
            + "\nMPI (x, y): " + f(s.groupCentreX, 2) + ", " + f(s.groupCentreY, 2) + " mm"
            + "   Trend: " + (s.trendDirection || "—")
    }
    function fmtDist(s) {
        if (!s) return "Not available"
        return "Perfect (≥10.8): " + f(s.perfectCount, 0) + " (" + pct(s.perfectPct) + ")"
            + "   Poor (<9.5): " + f(s.poorCount, 0) + " (" + pct(s.poorPct) + ")"
            + "\n≥10.5: " + f(s.countAtLeast10_5, 0) + "   ≥10.7: " + f(s.countAtLeast10_7, 0)
            + "   <10.0: " + f(s.countBelow10_0, 0)
            + "\nBest streak ≥10.5: " + f(s.bestStreak10_5, 0)
            + "   Longest poor streak: " + f(s.longestPoorStreak, 0)
    }
    function fmtTrend(s) {
        if (!s) return "Not available"
        var t = "Direction: " + (s.trendDirection || "—")
        if (s.regressionAvailable) t += "   Slope: " + f(s.regressionSlope, 4) + "   Corr: " + f(s.regressionCorrelation, 3)
        if (s.halvesAvailable) t += "\nHalves: " + f(s.firstHalfAverage, 2) + " → " + f(s.secondHalfAverage, 2)
        if (s.thirdsAvailable) t += "   Thirds: " + f(s.firstThirdAverage, 2) + " / " + f(s.middleThirdAverage, 2) + " / " + f(s.lastThirdAverage, 2)
        if (s.lateSessionDropFlag) t += "\n⚠ Late-session drop: " + f(s.lateSessionDrop, 2)
        if (s.deteriorationDeterminable && s.deteriorationFlag) t += "\n⚠ Deterioration (mean down, variance up)"
        return t
    }
    function fmtHeat(s) {
        if (!s || !s.available) return "No coordinate data for this session."
        var g = s.allShots
        var t = "Group radius: " + f(g && g.stats ? g.stats.meanRadius : undefined, 2) + " mm"
            + "   Diameter: " + f(g && g.stats ? g.stats.groupDiameter : undefined, 2) + " mm"
        if (g && g.hasDominantZone) t += "   Dominant zone: " + pct(g.dominantSharePct)
        t += "\nDrift (halves) x/y: " + f(s.horizontalDriftHalves, 2) + " / " + f(s.verticalDriftHalves, 2) + " mm"
        t += "   (thirds): " + f(s.horizontalDriftThirds, 2) + " / " + f(s.verticalDriftThirds, 2) + " mm"
        t += "\nDots: green ≥10.5, amber ≥10.0, red <10.0."
        return t
    }
    function fmtTiming(s) {
        if (!s || !s.available) return "Not available (no timing data)."
        var t = "Avg interval: " + f(s.averageInterval, 2) + "s   Median: " + f(s.medianInterval, 2) + "s   SD: " + f(s.intervalSD, 2)
        t += "\nFastest / slowest: " + f(s.fastestInterval, 2) + " / " + f(s.slowestInterval, 2) + "s"
        if (s.rhythmAvailable) t += "   Rhythm: " + pct(s.rhythmConsistency)
        t += "\nRushed: " + f(s.rushedShotCount, 0) + "   Delayed: " + f(s.delayedShotCount, 0)
        if (s.trendAvailable) t += "   Pace: " + (s.intervalTrend || "—")
        return t
    }
    function fmtPosition(s) {
        if (!s || !s.available) return "Not available"
        var t = ""
        for (var i = 0; i < s.positions.length; i++) {
            var p = s.positions[i], m = p.measurements
            t += (i > 0 ? "\n\n" : "") + up(p.position)
                + "\n  shots: " + f(p.shotCount, 0) + "   avg: " + f(m ? m.averageScore : undefined, 2)
                + "   quality: " + (p.qualityAvailable ? f(p.qualityScore, 0) + "/100" : "—")
                + "\n  points lost: " + f(p.pointsLost, 1)
                + "   group radius: " + f(m && m.geometry ? m.geometry.groupRadius : undefined, 2) + " mm"
        }
        if (s.comparativeAvailable)
            t += "\n\nStrongest: " + (s.hasStrongest ? up(s.strongestPosition) : "—")
                + "   Weakest: " + (s.hasWeakest ? up(s.weakestPosition) : "—")
        return t
    }
    function fmtRecovery(s) {
        if (!s || !s.overall || !s.overall.available) return "Not available (no poor shots, or too few shots)."
        var o = s.overall
        var t = "Pattern: " + (o.pattern || "—")
            + "\nBad-shot recovery rate: " + pct(o.badShotRecoveryRate)
            + "   Avg recovery shots: " + (o.recoveryShotsAvailable ? f(o.averageRecoveryShots, 2) : "—")
            + "\nPost-error delta: " + f(o.postErrorDelta, 2)
            + "   Repeated-error rate: " + pct(o.repeatedErrorRate)
        if (o.overCorrectionAvailable) t += "\nOver-correction rate: " + pct(o.overCorrectionRate)
        if (s.comparativeAvailable) t += "\nBest / worst position: " + up(s.bestRecoveryPosition) + " / " + up(s.worstRecoveryPosition)
        return t
    }
    function fmtFatigue(s) {
        if (!s || !s.available) return "Not available"
        var t = "Pattern: " + (s.overallPattern || "—")
            + "\nFatigue index: " + f(s.fatigueIndex, 2) + "   Confidence: " + pct(s.confidence)
            + "\nEarly → late avg delta: " + f(s.earlyLateDelta, 2)
        if (s.coordinateTrendAvailable) t += "   Group expansion: " + f(s.groupExpansionRate * 100, 0) + "%"
        if (s.timingTrendAvailable) t += "   Timing change: " + f(s.timingChangeRate * 100, 0) + "%"
        if (s.hasFatiguedPosition) t += "\nMost fatigued: " + up(s.mostFatiguedPosition)
        return t
    }
    function fmtPriorities(list) {
        if (!list || list.length === 0) return "No major training priority detected in this session's data."
        var t = ""
        for (var i = 0; i < list.length; i++)
            t += (i > 0 ? "\n" : "") + (i + 1) + ". " + list[i].priority + "   [" + list[i].impact + " impact, score " + f(list[i].priorityScore, 0) + "]"
        return t
    }
    function fmtConclusion(s) {
        if (!s || !s.available) return "Not available"
        var t = "Rating: " + (s.rating || "—") + "\n\n" + (s.overallAssessment || "")
        if (s.keyStrengths && s.keyStrengths.length) t += "\n\nStrengths:\n  • " + s.keyStrengths.join("\n  • ")
        if (s.mainLimitingFactors && s.mainLimitingFactors.length) t += "\n\nLimiting factors:\n  • " + s.mainLimitingFactors.join("\n  • ")
        if (s.technicalRiskFlags && s.technicalRiskFlags.length) t += "\n\nRisk flags:\n  • " + s.technicalRiskFlags.join("\n  • ")
        if (s.trainingFocusSummary) t += "\n\n" + s.trainingFocusSummary
        t += "\n\nConfidence: " + pct(s.confidence)
        return t
    }
    function fmtDiary(s) {
        if (!s || s.isEmpty) return "No diary entered for this session."
        var t = "Type: " + (s.sessionType || "—") + "   Intensity: " + (s.sessionIntensity || "—")
        if (s.sessionGoal) t += "\nGoal: " + s.sessionGoal
        if (s.coachNotes) t += "\nCoach: " + s.coachNotes
        if (s.athleteNotes) t += "\nAthlete: " + s.athleteNotes
        if (s.tags && s.tags.length) t += "\nTags: " + s.tags.join(", ")
        return t
    }

    // ---------- coach-facing labels for Recovery / Fatigue ----------
    function recoveryPatternLabel(p) {
        switch (p) {
        case "GoodRecovery":          return "Recovers well after mistakes"
        case "SlowRecovery":          return "Slow to recover after mistakes"
        case "RepeatedErrorPattern":  return "Mistakes tend to cluster together"
        case "OverCorrectionPattern": return "Over-corrects after mistakes"
        default:                      return "Not enough poor shots to assess recovery"
        }
    }
    function fatiguePatternLabel(p) {
        switch (p) {
        case "NoFatigueDetected":       return "No fatigue signal — held up well"
        case "GradualDecline":          return "Gradual decline through the session"
        case "LateMatchDrop":           return "Score dropped late in the match"
        case "IncreasingDispersion":    return "Group opened up over the session"
        case "TimingSlowdown":          return "Shot timing slowed late"
        case "FatigueCompensation":     return "Working harder late to hold the score"
        case "PositionSpecificFatigue": return "Fatigue localised to one position"
        default:                        return "Not enough data to assess fatigue"
        }
    }
    function sevColor(s) {
        switch (s) {
        case "Low":      return "#8bbf5a"
        case "Moderate": return "#e0b020"
        case "High":     return "#e07a30"
        case "Critical": return "#d0392b"
        default:         return theme.textSecondary   // None
        }
    }
    function indexSevColor(idx) {
        if (idx === undefined || idx === null) return theme.textSecondary
        if (idx < 0.20) return theme.textSecondary
        if (idx < 0.35) return "#8bbf5a"
        if (idx < 0.50) return "#e0b020"
        if (idx < 0.70) return "#e07a30"
        return "#d0392b"
    }

    // ---------- target shot-map data ----------
    function refreshTargets() {
        // Match the engine's analysed set exactly (same filter as the dashboard).
        var all = COACHREPORT.shots()
        all = all.filter(function (s) { return s.isValid && s.isCompetitionShot && !s.isSighter })
        var ext = 8
        for (var i = 0; i < all.length; i++)
            ext = Math.max(ext, Math.sqrt(all[i].x * all[i].x + all[i].y * all[i].y))
        reportPage.sharedExtent = ext * 1.15
        var list = [{ title: "All shots", shots: all }]
        var groups = {}, order = []
        for (var j = 0; j < all.length; j++) {
            var p = all[j].position
            if (!groups[p]) { groups[p] = []; order.push(p) }
            groups[p].push(all[j])
        }
        if (order.length > 1)
            for (var k = 0; k < order.length; k++) list.push({ title: order[k], shots: groups[order[k]] })
        reportPage.targets = list
    }

    // ---------- charts ----------
    function rebuildCharts() { rebuildDist(); rebuildTrend() }
    function rebuildDist() {
        if (typeof distPie === "undefined" || !distPie) return
        distPie.clear()
        var d = reportPage.rep ? reportPage.rep.shotDistribution : null
        if (!d) return
        function add(label, val, col) {
            if (val > 0) { var s = distPie.append(label + " (" + val + ")", val); s.color = col; s.labelVisible = false }
        }
        add("Perfect", d.perfectCount, "#37c871")
        add("Excellent", d.excellentCount, "#63b8ff")
        add("Good", d.goodCount, "#9a7bff")
        add("Acceptable", d.acceptableCount, "#e0b020")
        add("Recovery", d.recoveryCount, "#e07f30")
        add("Poor", d.poorCount, "#e0503a")
    }
    function rebuildTrend() {
        if (typeof scoreSeries === "undefined" || !scoreSeries) return
        scoreSeries.clear(); rollSeries.clear()
        var t = reportPage.rep ? reportPage.rep.trendAnalysis : null
        if (!t || !t.scores || t.scores.length === 0) return
        var n = t.scores.length, lo = 11, hi = 0
        for (var i = 0; i < n; i++) {
            var v = t.scores[i]; scoreSeries.append(i + 1, v)
            lo = Math.min(lo, v); hi = Math.max(hi, v)
        }
        if (t.rolling5Available && t.rolling5Average)
            for (var j = 0; j < t.rolling5Average.length; j++) rollSeries.append(j + 5, t.rolling5Average[j])
        axX.min = 1; axX.max = Math.max(2, n)
        axY.min = Math.max(0, Math.floor(lo) - 0.2); axY.max = Math.min(10.9, Math.ceil(hi) + 0.1)
    }

    // ================= body =================
    // (The old screen-only top bar — title, rating chip, Flip Y / Re-run / nav
    // buttons — is gone: the window owns the chrome, and the analysis is fed by
    // ShootingPage.feedCoachReport() before every open.)
    Flickable {
        id: scroller
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        contentWidth: width
        contentHeight: scrollColumn.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

        Column {
            id: scrollColumn
            width: scroller.width - 14
            spacing: 12

            Text {
                width: parent.width
                visible: !(reportPage.rep && reportPage.rep.valid)
                text: "No report to display yet.\n" + ((reportPage.rep && reportPage.rep.message) ? reportPage.rep.message : "Finish a match, then click Coach Report.")
                color: theme.textSecondary; font.pixelSize: 15; font.family: theme.fontFamily; wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: (reportPage.rep && reportPage.rep.lowSampleWarning) ? "⚠ Low sample (" + reportPage.f(reportPage.rep.analysedShotCount, 0) + " shots) — results are indicative only." : ""
                visible: text.length > 0
                color: theme.brandAccent; font.pixelSize: 13; font.family: theme.fontFamily; wrapMode: Text.WordWrap
            }

            // ----- Executive Summary -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Executive Summary"; body: reportPage.fmtExec(reportPage.rep ? reportPage.rep.executiveSummary : null)
            }

            // ----- Shot Map / Heat Map (density heat map + dot targets) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Shot Map / Heat Map"; body: reportPage.fmtHeat(reportPage.rep ? reportPage.rep.heatMapAnalysis : null)

                Text { width: parent.width; text: "Density heat map"; color: theme.textSecondary; font.pixelSize: 13; font.bold: true; font.family: theme.fontFamily }
                Flow {
                    width: parent.width; spacing: 18
                    Repeater {
                        model: reportPage.targets
                        delegate: HeatMapCanvas { title: modelData.title; shots: modelData.shots; side: 260; extentMm: reportPage.sharedExtent }
                    }
                }
                Text { width: parent.width; text: "Shot positions"; color: theme.textSecondary; font.pixelSize: 13; font.bold: true; font.family: theme.fontFamily }
                Flow {
                    width: parent.width; spacing: 18
                    Repeater {
                        model: reportPage.targets
                        delegate: ShotTargetCanvas { title: modelData.title; shots: modelData.shots; side: 240; extentMm: reportPage.sharedExtent }
                    }
                }
            }

            // ----- Score Distribution (pie) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Score Distribution"; body: reportPage.fmtDist(reportPage.rep ? reportPage.rep.shotDistribution : null)
                ChartView {
                    width: parent.width; height: 280
                    antialiasing: true; legend.visible: true; legend.alignment: Qt.AlignRight
                    legend.labelColor: theme.textSecondary
                    backgroundColor: "transparent"; plotAreaColor: "transparent"
                    theme: ChartView.ChartThemeDark
                    PieSeries { id: distPie; holeSize: 0.35; size: 0.85 }
                    Component.onCompleted: reportPage.rebuildDist()
                }
            }

            // ----- Trend (line) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Trend"; body: reportPage.fmtTrend(reportPage.rep ? reportPage.rep.trendAnalysis : null)
                ChartView {
                    width: parent.width; height: 300
                    antialiasing: true; legend.visible: true; legend.alignment: Qt.AlignBottom
                    legend.labelColor: theme.textSecondary
                    backgroundColor: "transparent"; plotAreaColor: "transparent"
                    theme: ChartView.ChartThemeDark
                    ValueAxis { id: axX; titleText: "shot"; labelsColor: theme.textSecondary; min: 1; max: 10 }
                    ValueAxis { id: axY; titleText: "score"; labelsColor: theme.textSecondary; min: 8; max: 10.9 }
                    LineSeries { id: scoreSeries; name: "Score"; axisX: axX; axisY: axY; color: theme.brandPrimary; width: 2 }
                    LineSeries { id: rollSeries; name: "Rolling-5 avg"; axisX: axX; axisY: axY; color: "#63b8ff"; width: 2 }
                    Component.onCompleted: reportPage.rebuildTrend()
                }
            }

            // ----- remaining text sections -----
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: "Timing"; body: reportPage.fmtTiming(reportPage.rep ? reportPage.rep.timingAnalysis : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: "Position Analysis"; body: reportPage.fmtPosition(reportPage.rep ? reportPage.rep.positionAnalysis : null) }
            // ----- Recovery (structured, with evidence) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Recovery Analysis"; body: ""
                Row {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; anchors.verticalCenter: parent.verticalCenter
                        color: reportPage.sevColor((reportPage.rep && reportPage.rep.recoveryAnalysis.overall.available) ? reportPage.rep.recoveryAnalysis.overall.recoverySeverity : "None")
                    }
                    Text {
                        text: reportPage.recoveryPatternLabel(reportPage.rep ? reportPage.rep.recoveryAnalysis.overall.pattern : "")
                        color: theme.textPrimary; font.bold: true; font.pixelSize: 15; font.family: theme.fontFamily
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.rep && reportPage.rep.recoveryAnalysis.overall.available
                    text: "Recovery rate " + reportPage.pct(reportPage.rep ? reportPage.rep.recoveryAnalysis.overall.badShotRecoveryRate : 0)
                          + "   •   confidence " + reportPage.pct(reportPage.rep ? reportPage.rep.recoveryAnalysis.overall.recoveryConfidence : 0)
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
                Repeater {
                    model: (reportPage.rep && reportPage.rep.recoveryAnalysis.metrics) ? reportPage.rep.recoveryAnalysis.metrics : []
                    delegate: Column {
                        width: parent.width; spacing: 2
                        Row {
                            spacing: 6
                            Rectangle { width: 9; height: 9; radius: 4.5; anchors.verticalCenter: parent.verticalCenter; color: reportPage.sevColor(modelData.severity) }
                            Text { text: modelData.name; color: theme.textPrimary; font.bold: true; font.pixelSize: 13; font.family: theme.fontFamily }
                        }
                        Repeater {
                            model: modelData.evidence
                            delegate: Text { width: parent.width; text: "    " + modelData.statement; color: theme.textSecondary; font.pixelSize: 12; font.family: theme.fontFamily; wrapMode: Text.WordWrap }
                        }
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.rep && reportPage.rep.recoveryAnalysis.comparativeAvailable
                    text: "Best position: " + reportPage.up(reportPage.rep ? reportPage.rep.recoveryAnalysis.bestRecoveryPosition : "")
                          + "   •   Worst: " + reportPage.up(reportPage.rep ? reportPage.rep.recoveryAnalysis.worstRecoveryPosition : "")
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
            }

            // ----- Fatigue (structured, with evidence) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: "Fatigue Analysis"; body: ""
                Row {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; anchors.verticalCenter: parent.verticalCenter
                        color: reportPage.indexSevColor(reportPage.rep ? reportPage.rep.fatigueAnalysis.fatigueIndex : 0)
                    }
                    Text {
                        text: reportPage.fatiguePatternLabel(reportPage.rep ? reportPage.rep.fatigueAnalysis.overallPattern : "")
                        color: theme.textPrimary; font.bold: true; font.pixelSize: 15; font.family: theme.fontFamily
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.rep && reportPage.rep.fatigueAnalysis.scoreTrendAvailable
                    text: "Fatigue index " + reportPage.f(reportPage.rep ? reportPage.rep.fatigueAnalysis.fatigueIndex : 0, 2)
                          + "   •   confidence " + reportPage.pct(reportPage.rep ? reportPage.rep.fatigueAnalysis.confidence : 0)
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
                Repeater {
                    model: (reportPage.rep && reportPage.rep.fatigueAnalysis.metrics) ? reportPage.rep.fatigueAnalysis.metrics : []
                    delegate: Column {
                        width: parent.width; spacing: 2
                        Row {
                            spacing: 6
                            Rectangle { width: 9; height: 9; radius: 4.5; anchors.verticalCenter: parent.verticalCenter; color: reportPage.sevColor(modelData.severity) }
                            Text { text: modelData.name; color: theme.textPrimary; font.bold: true; font.pixelSize: 13; font.family: theme.fontFamily }
                        }
                        Repeater {
                            model: modelData.evidence
                            delegate: Text { width: parent.width; text: "    " + modelData.statement; color: theme.textSecondary; font.pixelSize: 12; font.family: theme.fontFamily; wrapMode: Text.WordWrap }
                        }
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.rep && reportPage.rep.fatigueAnalysis.hasFatiguedPosition
                    text: "Most fatigued position: " + reportPage.up(reportPage.rep ? reportPage.rep.fatigueAnalysis.mostFatiguedPosition : "")
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
            }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: "Training Priorities"; body: reportPage.fmtPriorities(reportPage.rep ? reportPage.rep.trainingPriorities : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: "Coach Conclusion"; body: reportPage.fmtConclusion(reportPage.rep ? reportPage.rep.coachConclusion : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: "Coach Diary"; body: reportPage.fmtDiary(reportPage.rep ? reportPage.rep.coachDiary : null) }

            Item { width: 1; height: 12 }
        }
    }
}
