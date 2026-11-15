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
    // Safe availability (S1.4): COACHREPORT.report exists before any analysis
    // but its sections are unpopulated then — the deep recovery/fatigue chains
    // threw TypeErrors at startup. Sections resolve to null until a VALID
    // report is loaded; the cards already show a clean empty state via their
    // rep.valid visibility. No analysis is triggered here.
    readonly property bool reportAvailable: rep !== undefined && rep !== null && rep.valid === true
    readonly property var recovery: (reportAvailable && rep.recoveryAnalysis !== undefined) ? rep.recoveryAnalysis : null
    readonly property var fatigue: (reportAvailable && rep.fatigueAnalysis !== undefined) ? rep.fatigueAnalysis : null

    // Target shot-map data (refreshed on each analysis).
    property var  targets: []
    property real sharedExtent: 0
    // TRUE ISSF ring geometry for the shot maps (mm) — the same per-discipline
    // constants the scoring face uses (10-ring radius; radial step per ring).
    // loginPage.gameMode: 0 = pistol, 1 = rifle; gameRange resolves via main.
    readonly property real ringTenMm: gameRange == 10
                                      ? (loginPage.gameMode === 0 ? 5.75 : 0.25)
                                      : (loginPage.gameMode === 0 ? 25.0 : 5.2)
    readonly property real ringStepMm: gameRange == 10
                                       ? (loginPage.gameMode === 0 ? 8.0 : 2.5)
                                       : (loginPage.gameMode === 0 ? 25.0 : 8.0)

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

    // LOC-REPORT-001: every fixed label is one translatable line with %n
    // placeholders. Values the frozen coach engine supplies as text (trend
    // direction, priority wording, the assessment sentences) are shown as the
    // engine produced them - translating those would change the frozen engine.
    function fmtExec(s) {
        if (!s) return qsTr("Not available")
        return qsTr("Shots: %1   Total: %2   Average: %3").arg(f(s.competitionShotCount, 0)).arg(f(s.totalScore, 1)).arg(f(s.averageScore, 2))
            + "\n" + qsTr("Best / Worst: %1 / %2   Score SD: %3").arg(f(s.bestShot, 1)).arg(f(s.worstShot, 1)).arg(f(s.scoreStandardDeviation, 3))
            + "\n" + qsTr("Consistency: %1").arg(pct(s.consistencyPercentage))
            + "\n" + qsTr("Group radius / diameter: %1 / %2 mm").arg(f(s.groupRadius, 2)).arg(f(s.groupDiameter, 2))
            + "\n" + qsTr("MPI (x, y): %1, %2 mm   Trend: %3").arg(f(s.groupCentreX, 2)).arg(f(s.groupCentreY, 2)).arg(s.trendDirection || "—")
    }
    function fmtDist(s) {
        if (!s) return qsTr("Not available")
        return qsTr("Perfect (≥10.8): %1 (%2)   Poor (<9.5): %3 (%4)").arg(f(s.perfectCount, 0)).arg(pct(s.perfectPct)).arg(f(s.poorCount, 0)).arg(pct(s.poorPct))
            + "\n" + qsTr("≥10.5: %1   ≥10.7: %2   <10.0: %3").arg(f(s.countAtLeast10_5, 0)).arg(f(s.countAtLeast10_7, 0)).arg(f(s.countBelow10_0, 0))
            + "\n" + qsTr("Best streak ≥10.5: %1   Longest poor streak: %2").arg(f(s.bestStreak10_5, 0)).arg(f(s.longestPoorStreak, 0))
    }
    function fmtTrend(s) {
        if (!s) return qsTr("Not available")
        var t = qsTr("Direction: %1").arg(s.trendDirection || "—")
        if (s.regressionAvailable) t += "   " + qsTr("Slope: %1   Corr: %2").arg(f(s.regressionSlope, 4)).arg(f(s.regressionCorrelation, 3))
        if (s.halvesAvailable) t += "\n" + qsTr("Halves: %1 → %2").arg(f(s.firstHalfAverage, 2)).arg(f(s.secondHalfAverage, 2))
        if (s.thirdsAvailable) t += "   " + qsTr("Thirds: %1 / %2 / %3").arg(f(s.firstThirdAverage, 2)).arg(f(s.middleThirdAverage, 2)).arg(f(s.lastThirdAverage, 2))
        if (s.lateSessionDropFlag) t += "\n" + qsTr("⚠ Late-session drop: %1").arg(f(s.lateSessionDrop, 2))
        if (s.deteriorationDeterminable && s.deteriorationFlag) t += "\n" + qsTr("⚠ Deterioration (mean down, variance up)")
        return t
    }
    function fmtHeat(s) {
        if (!s || !s.available) return qsTr("No coordinate data for this session.")
        var g = s.allShots
        var t = qsTr("Group radius: %1 mm   Diameter: %2 mm").arg(f(g && g.stats ? g.stats.meanRadius : undefined, 2)).arg(f(g && g.stats ? g.stats.groupDiameter : undefined, 2))
        if (g && g.hasDominantZone) t += "   " + qsTr("Dominant zone: %1").arg(pct(g.dominantSharePct))
        t += "\n" + qsTr("Drift (halves) x/y: %1 / %2 mm").arg(f(s.horizontalDriftHalves, 2)).arg(f(s.verticalDriftHalves, 2))
        t += "   " + qsTr("(thirds): %1 / %2 mm").arg(f(s.horizontalDriftThirds, 2)).arg(f(s.verticalDriftThirds, 2))
        t += "\n" + qsTr("Dots: green ≥10.5, amber ≥10.0, red <10.0.")
        return t
    }
    function fmtTiming(s) {
        if (!s || !s.available) return qsTr("Not available (no timing data).")
        var t = qsTr("Avg interval: %1s   Median: %2s   SD: %3").arg(f(s.averageInterval, 2)).arg(f(s.medianInterval, 2)).arg(f(s.intervalSD, 2))
        t += "\n" + qsTr("Fastest / slowest: %1 / %2s").arg(f(s.fastestInterval, 2)).arg(f(s.slowestInterval, 2))
        if (s.rhythmAvailable) t += "   " + qsTr("Rhythm: %1").arg(pct(s.rhythmConsistency))
        t += "\n" + qsTr("Rushed: %1   Delayed: %2").arg(f(s.rushedShotCount, 0)).arg(f(s.delayedShotCount, 0))
        if (s.trendAvailable) t += "   " + qsTr("Pace: %1").arg(s.intervalTrend || "—")
        return t
    }
    function fmtPosition(s) {
        if (!s || !s.available) return qsTr("Not available")
        var t = ""
        for (var i = 0; i < s.positions.length; i++) {
            var p = s.positions[i], m = p.measurements
            t += (i > 0 ? "\n\n" : "") + posName(p.position)
                + "\n  " + qsTr("shots: %1   avg: %2   quality: %3").arg(f(p.shotCount, 0)).arg(f(m ? m.averageScore : undefined, 2)).arg(p.qualityAvailable ? f(p.qualityScore, 0) + "/100" : "—")
                + "\n  " + qsTr("points lost: %1   group radius: %2 mm").arg(f(p.pointsLost, 1)).arg(f(m && m.geometry ? m.geometry.groupRadius : undefined, 2))
        }
        if (s.comparativeAvailable)
            t += "\n\n" + qsTr("Strongest: %1   Weakest: %2").arg(s.hasStrongest ? posName(s.strongestPosition) : "—").arg(s.hasWeakest ? posName(s.weakestPosition) : "—")
        return t
    }
    function fmtRecovery(s) {
        if (!s || !s.overall || !s.overall.available) return qsTr("Not available (no poor shots, or too few shots).")
        var o = s.overall
        var t = qsTr("Pattern: %1").arg(o.pattern || "—")
            + "\n" + qsTr("Bad-shot recovery rate: %1   Avg recovery shots: %2").arg(pct(o.badShotRecoveryRate)).arg(o.recoveryShotsAvailable ? f(o.averageRecoveryShots, 2) : "—")
            + "\n" + qsTr("Post-error delta: %1   Repeated-error rate: %2").arg(f(o.postErrorDelta, 2)).arg(pct(o.repeatedErrorRate))
        if (o.overCorrectionAvailable) t += "\n" + qsTr("Over-correction rate: %1").arg(pct(o.overCorrectionRate))
        if (s.comparativeAvailable) t += "\n" + qsTr("Best / worst position: %1 / %2").arg(posName(s.bestRecoveryPosition)).arg(posName(s.worstRecoveryPosition))
        return t
    }
    function fmtFatigue(s) {
        if (!s || !s.available) return qsTr("Not available")
        var t = qsTr("Pattern: %1").arg(s.overallPattern || "—")
            + "\n" + qsTr("Fatigue index: %1   Confidence: %2").arg(f(s.fatigueIndex, 2)).arg(pct(s.confidence))
            + "\n" + qsTr("Early → late avg delta: %1").arg(f(s.earlyLateDelta, 2))
        if (s.coordinateTrendAvailable) t += "   " + qsTr("Group expansion: %1%").arg(f(s.groupExpansionRate * 100, 0))
        if (s.timingTrendAvailable) t += "   " + qsTr("Timing change: %1%").arg(f(s.timingChangeRate * 100, 0))
        if (s.hasFatiguedPosition) t += "\n" + qsTr("Most fatigued: %1").arg(posName(s.mostFatiguedPosition))
        return t
    }
    function fmtPriorities(list) {
        if (!list || list.length === 0) return qsTr("No major training priority detected in this session's data.")
        var t = ""
        for (var i = 0; i < list.length; i++)
            t += (i > 0 ? "\n" : "") + qsTr("%1. %2   [%3 impact, score %4]").arg(i + 1).arg(list[i].priority).arg(list[i].impact).arg(f(list[i].priorityScore, 0))
        return t
    }
    function fmtConclusion(s) {
        if (!s || !s.available) return qsTr("Not available")
        var t = qsTr("Rating: %1").arg(s.rating || "—") + "\n\n" + (s.overallAssessment || "")
        if (s.keyStrengths && s.keyStrengths.length) t += "\n\n" + qsTr("Strengths:") + "\n  • " + s.keyStrengths.join("\n  • ")
        if (s.mainLimitingFactors && s.mainLimitingFactors.length) t += "\n\n" + qsTr("Limiting factors:") + "\n  • " + s.mainLimitingFactors.join("\n  • ")
        if (s.technicalRiskFlags && s.technicalRiskFlags.length) t += "\n\n" + qsTr("Risk flags:") + "\n  • " + s.technicalRiskFlags.join("\n  • ")
        if (s.trainingFocusSummary) t += "\n\n" + s.trainingFocusSummary
        t += "\n\n" + qsTr("Confidence: %1").arg(pct(s.confidence))
        return t
    }
    function fmtDiary(s) {
        if (!s || s.isEmpty) return qsTr("No diary entered for this session.")
        var t = qsTr("Type: %1   Intensity: %2").arg(s.sessionType || "—").arg(s.sessionIntensity || "—")
        if (s.sessionGoal) t += "\n" + qsTr("Goal: %1").arg(s.sessionGoal)
        if (s.coachNotes) t += "\n" + qsTr("Coach: %1").arg(s.coachNotes)
        if (s.athleteNotes) t += "\n" + qsTr("Athlete: %1").arg(s.athleteNotes)
        if (s.tags && s.tags.length) t += "\n" + qsTr("Tags: %1").arg(s.tags.join(", "))
        return t
    }
    // Engine position ids (KNEELING / PRONE / STANDING, any case) -> label.
    function posName(x) {
        switch (String(x || "").toUpperCase()) {
        case "KNEELING": return qsTr("KNEELING")
        case "PRONE":    return qsTr("PRONE")
        case "STANDING": return qsTr("STANDING")
        }
        return up(x)
    }

    // ---------- coach-facing labels for Recovery / Fatigue ----------
    function recoveryPatternLabel(p) {
        switch (p) {
        case "GoodRecovery":          return qsTr("Recovers well after mistakes")
        case "SlowRecovery":          return qsTr("Slow to recover after mistakes")
        case "RepeatedErrorPattern":  return qsTr("Mistakes tend to cluster together")
        case "OverCorrectionPattern": return qsTr("Over-corrects after mistakes")
        default:                      return qsTr("Not enough poor shots to assess recovery")
        }
    }
    function fatiguePatternLabel(p) {
        switch (p) {
        case "NoFatigueDetected":       return qsTr("No fatigue signal — held up well")
        case "GradualDecline":          return qsTr("Gradual decline through the session")
        case "LateMatchDrop":           return qsTr("Score dropped late in the match")
        case "IncreasingDispersion":    return qsTr("Group opened up over the session")
        case "TimingSlowdown":          return qsTr("Shot timing slowed late")
        case "FatigueCompensation":     return qsTr("Working harder late to hold the score")
        case "PositionSpecificFatigue": return qsTr("Fatigue localised to one position")
        default:                        return qsTr("Not enough data to assess fatigue")
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
        var list = [{ title: qsTr("All shots"), shots: all }]
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
                text: qsTr("No report to display yet.") + "\n" + ((reportPage.rep && reportPage.rep.message) ? reportPage.rep.message : qsTr("Finish a match, then click Coach Report."))
                color: theme.textSecondary; font.pixelSize: 15; font.family: theme.fontFamily; wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: (reportPage.rep && reportPage.rep.lowSampleWarning) ? qsTr("⚠ Low sample (%1 shots) — results are indicative only.").arg(reportPage.f(reportPage.rep.analysedShotCount, 0)) : ""
                visible: text.length > 0
                color: theme.brandAccent; font.pixelSize: 13; font.family: theme.fontFamily; wrapMode: Text.WordWrap
            }

            // ----- Executive Summary -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: qsTr("Executive Summary"); body: reportPage.fmtExec(reportPage.rep ? reportPage.rep.executiveSummary : null)
            }

            // ----- Shot Map / Heat Map (density heat map + dot targets) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: qsTr("Shot Map / Heat Map"); body: reportPage.fmtHeat(reportPage.rep ? reportPage.rep.heatMapAnalysis : null)

                Text { width: parent.width; text: qsTr("Density heat map"); color: theme.textSecondary; font.pixelSize: 13; font.bold: true; font.family: theme.fontFamily }
                Flow {
                    width: parent.width; spacing: 18
                    Repeater {
                        model: reportPage.targets
                        delegate: HeatMapCanvas { title: modelData.title; shots: modelData.shots; side: 260; extentMm: reportPage.sharedExtent }
                    }
                }
                Text { width: parent.width; text: qsTr("Shot positions"); color: theme.textSecondary; font.pixelSize: 13; font.bold: true; font.family: theme.fontFamily }
                Flow {
                    width: parent.width; spacing: 18
                    Repeater {
                        model: reportPage.targets
                        delegate: ShotTargetCanvas {
                            title: modelData.title; shots: modelData.shots; side: 240
                            extentMm: reportPage.sharedExtent
                            // True ring geometry + true hole size: dot-vs-ring
                            // is the shot's real position on the card.
                            tenRingRadiusMm: reportPage.ringTenMm
                            ringStepMm: reportPage.ringStepMm
                            bulletDiameterMm: APPSETTINGS.bullet_diameter()
                        }
                    }
                }
            }

            // ----- Score Distribution (pie) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: qsTr("Score Distribution"); body: reportPage.fmtDist(reportPage.rep ? reportPage.rep.shotDistribution : null)
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
                title: qsTr("Trend"); body: reportPage.fmtTrend(reportPage.rep ? reportPage.rep.trendAnalysis : null)
                ChartView {
                    width: parent.width; height: 300
                    antialiasing: true; legend.visible: true; legend.alignment: Qt.AlignBottom
                    legend.labelColor: theme.textSecondary
                    backgroundColor: "transparent"; plotAreaColor: "transparent"
                    theme: ChartView.ChartThemeDark
                    ValueAxis { id: axX; titleText: "shot"; labelsColor: theme.textSecondary; min: 1; max: 10 }
                    ValueAxis { id: axY; titleText: "score"; labelsColor: theme.textSecondary; min: 8; max: 10.9 }
                    LineSeries { id: scoreSeries; name: qsTr("Score"); axisX: axX; axisY: axY; color: theme.brandPrimary; width: 2 }
                    LineSeries { id: rollSeries; name: qsTr("Rolling-5 avg"); axisX: axX; axisY: axY; color: "#63b8ff"; width: 2 }
                    Component.onCompleted: reportPage.rebuildTrend()
                }
            }

            // ----- remaining text sections -----
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: qsTr("Timing"); body: reportPage.fmtTiming(reportPage.rep ? reportPage.rep.timingAnalysis : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: qsTr("Position Analysis"); body: reportPage.fmtPosition(reportPage.rep ? reportPage.rep.positionAnalysis : null) }
            // ----- Recovery (structured, with evidence) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: qsTr("Recovery Analysis"); body: ""
                Row {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; anchors.verticalCenter: parent.verticalCenter
                        color: reportPage.sevColor((reportPage.recovery && reportPage.recovery.overall.available) ? reportPage.recovery.overall.recoverySeverity : "None")
                    }
                    Text {
                        text: reportPage.recoveryPatternLabel(reportPage.recovery ? reportPage.recovery.overall.pattern : "")
                        color: theme.textPrimary; font.bold: true; font.pixelSize: 15; font.family: theme.fontFamily
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.recovery && reportPage.recovery.overall.available
                    text: qsTr("Recovery rate %1   •   confidence %2")
                          .arg(reportPage.pct(reportPage.recovery ? reportPage.recovery.overall.badShotRecoveryRate : 0))
                          .arg(reportPage.pct(reportPage.recovery ? reportPage.recovery.overall.recoveryConfidence : 0))
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
                Repeater {
                    model: (reportPage.recovery && reportPage.recovery.metrics) ? reportPage.recovery.metrics : []
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
                    visible: reportPage.recovery && reportPage.recovery.comparativeAvailable
                    text: qsTr("Best position: %1   •   Worst: %2")
                          .arg(reportPage.posName(reportPage.recovery ? reportPage.recovery.bestRecoveryPosition : ""))
                          .arg(reportPage.posName(reportPage.recovery ? reportPage.recovery.worstRecoveryPosition : ""))
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
            }

            // ----- Fatigue (structured, with evidence) -----
            CoachReportCard {
                width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid
                title: qsTr("Fatigue Analysis"); body: ""
                Row {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; anchors.verticalCenter: parent.verticalCenter
                        color: reportPage.indexSevColor(reportPage.fatigue ? reportPage.fatigue.fatigueIndex : 0)
                    }
                    Text {
                        text: reportPage.fatiguePatternLabel(reportPage.fatigue ? reportPage.fatigue.overallPattern : "")
                        color: theme.textPrimary; font.bold: true; font.pixelSize: 15; font.family: theme.fontFamily
                    }
                }
                Text {
                    width: parent.width
                    visible: reportPage.fatigue && reportPage.fatigue.scoreTrendAvailable
                    text: qsTr("Fatigue index %1   •   confidence %2")
                          .arg(reportPage.f(reportPage.fatigue ? reportPage.fatigue.fatigueIndex : 0, 2))
                          .arg(reportPage.pct(reportPage.fatigue ? reportPage.fatigue.confidence : 0))
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
                Repeater {
                    model: (reportPage.fatigue && reportPage.fatigue.metrics) ? reportPage.fatigue.metrics : []
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
                    visible: reportPage.fatigue && reportPage.fatigue.hasFatiguedPosition
                    text: qsTr("Most fatigued position: %1").arg(reportPage.posName(reportPage.fatigue ? reportPage.fatigue.mostFatiguedPosition : ""))
                    color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
                }
            }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: qsTr("Training Priorities"); body: reportPage.fmtPriorities(reportPage.rep ? reportPage.rep.trainingPriorities : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: qsTr("Coach Conclusion"); body: reportPage.fmtConclusion(reportPage.rep ? reportPage.rep.coachConclusion : null) }
            CoachReportCard { width: scrollColumn.width; visible: reportPage.rep && reportPage.rep.valid; title: qsTr("Coach Diary"); body: reportPage.fmtDiary(reportPage.rep ? reportPage.rep.coachDiary : null) }

            Item { width: 1; height: 12 }
        }
    }
}
