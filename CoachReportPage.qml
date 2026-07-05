import QtQuick 2.15
import QtQuick.Controls 2.15

// First visible Coach Report screen.
//
// STRICT RULE: this page only DISPLAYS what COACHREPORT already computed.
// No analytics, no recalculation, no fabricated values. Missing/low-sample
// data is shown as "Not available", never faked.
//
// `theme`, `gameSubMode`, COACHREPORT and COACHFEED all resolve via main.qml's
// context (the same ancestor-scope pattern the other pages use).
Rectangle {
    id: reportPage
    color: theme.bgBase

    // gameSubMode is passed in from main.qml (0 = prone/air, 1 = 3P). Used only
    // to re-run the feeder when the coordinate orientation toggle changes.
    property int gameSubMode: 0

    // Live view of the computed report. Re-evaluates on COACHREPORT.reportChanged.
    property var rep: COACHREPORT.report

    signal closed()

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
        return "Shots: " + f(s.competitionShotCount, 0)
            + "   Total: " + f(s.totalScore, 1)
            + "   Average: " + f(s.averageScore, 2)
            + "\nBest / Worst: " + f(s.bestShot, 1) + " / " + f(s.worstShot, 1)
            + "   Score SD: " + f(s.scoreStandardDeviation, 3)
            + "\nConsistency: " + pct(s.consistencyPercentage)
            + "\nGroup radius / diameter: " + f(s.groupRadius, 2) + " / " + f(s.groupDiameter, 2) + " mm"
            + "\nMPI (x, y): " + f(s.groupCentreX, 2) + ", " + f(s.groupCentreY, 2) + " mm"
            + "\nTrend: " + (s.trendDirection || "—")
    }
    function fmtDist(s) {
        if (!s) return "Not available"
        return "Perfect (≥10.8): " + f(s.perfectCount, 0) + " (" + pct(s.perfectPct) + ")"
            + "   Excellent: " + f(s.excellentCount, 0) + "   Good: " + f(s.goodCount, 0)
            + "\nAcceptable: " + f(s.acceptableCount, 0) + "   Recovery: " + f(s.recoveryCount, 0)
            + "   Poor (<9.5): " + f(s.poorCount, 0) + " (" + pct(s.poorPct) + ")"
            + "\n≥10.5: " + f(s.countAtLeast10_5, 0) + "   ≥10.7: " + f(s.countAtLeast10_7, 0)
            + "   <10.0: " + f(s.countBelow10_0, 0)
            + "\nBest streak ≥10.5: " + f(s.bestStreak10_5, 0)
            + "   Longest poor streak: " + f(s.longestPoorStreak, 0)
    }
    function fmtTrend(s) {
        if (!s) return "Not available"
        var t = "Direction: " + (s.trendDirection || "—")
        if (s.regressionAvailable) t += "\nSlope: " + f(s.regressionSlope, 4) + "   Correlation: " + f(s.regressionCorrelation, 3)
        if (s.halvesAvailable) t += "\nHalves: " + f(s.firstHalfAverage, 2) + " → " + f(s.secondHalfAverage, 2)
        if (s.thirdsAvailable) t += "\nThirds: " + f(s.firstThirdAverage, 2) + " / " + f(s.middleThirdAverage, 2) + " / " + f(s.lastThirdAverage, 2)
        if (s.lateSessionDropFlag) t += "\n⚠ Late-session drop: " + f(s.lateSessionDrop, 2)
        if (s.deteriorationDeterminable && s.deteriorationFlag) t += "\n⚠ Deterioration (mean down, variance up)"
        return t
    }
    function fmtHeat(s) {
        if (!s || !s.available) return "No coordinate data for this session."
        var g = s.allShots
        var t = "Group radius: " + f(g && g.stats ? g.stats.meanRadius : undefined, 2) + " mm"
            + "   Diameter: " + f(g && g.stats ? g.stats.groupDiameter : undefined, 2) + " mm"
        if (g && g.hasDominantZone) t += "\nDominant zone share: " + pct(g.dominantSharePct)
        t += "\nDrift (halves) x/y: " + f(s.horizontalDriftHalves, 2) + " / " + f(s.verticalDriftHalves, 2) + " mm"
        t += "\nDrift (thirds) x/y: " + f(s.horizontalDriftThirds, 2) + " / " + f(s.verticalDriftThirds, 2) + " mm"
        return t
    }
    function fmtTiming(s) {
        if (!s || !s.available) return "Not available (no timing data)."
        var t = "Avg interval: " + f(s.averageInterval, 2) + "s   Median: " + f(s.medianInterval, 2) + "s   SD: " + f(s.intervalSD, 2)
        t += "\nFastest / slowest: " + f(s.fastestInterval, 2) + " / " + f(s.slowestInterval, 2) + "s"
        if (s.rhythmAvailable) t += "\nRhythm consistency: " + pct(s.rhythmConsistency)
        t += "\nRushed: " + f(s.rushedShotCount, 0) + "   Delayed: " + f(s.delayedShotCount, 0)
        if (s.trendAvailable) t += "\nPace: " + (s.intervalTrend || "—")
        return t
    }
    function fmtPosition(s) {
        if (!s || !s.available) return "Not available"
        var t = ""
        for (var i = 0; i < s.positions.length; i++) {
            var p = s.positions[i]
            var m = p.measurements
            t += (i > 0 ? "\n\n" : "") + up(p.position)
                + "\n  shots: " + f(p.shotCount, 0) + "   avg: " + f(m ? m.averageScore : undefined, 2)
                + "   quality: " + (p.qualityAvailable ? f(p.qualityScore, 0) + "/100" : "—")
                + "\n  points lost: " + f(p.pointsLost, 1)
                + "   group radius: " + f(m && m.geometry ? m.geometry.groupRadius : undefined, 2) + " mm"
        }
        if (s.comparativeAvailable) {
            t += "\n\nStrongest: " + (s.hasStrongest ? up(s.strongestPosition) : "—")
                + "   Weakest: " + (s.hasWeakest ? up(s.weakestPosition) : "—")
        }
        return t
    }
    function fmtRecovery(s) {
        if (!s || !s.overall || !s.overall.available) return "Not available (no poor shots, or too few shots)."
        var o = s.overall
        var t = "Pattern: " + (o.pattern || "—")
            + "\nBad-shot recovery rate: " + pct(o.badShotRecoveryRate)
            + "\nAvg recovery shots: " + (o.recoveryShotsAvailable ? f(o.averageRecoveryShots, 2) : "—")
            + "\nPost-error delta: " + f(o.postErrorDelta, 2)
            + "\nRepeated-error rate: " + pct(o.repeatedErrorRate)
        if (o.overCorrectionAvailable) t += "\nOver-correction rate: " + pct(o.overCorrectionRate)
        if (s.comparativeAvailable) t += "\nBest / worst position: " + up(s.bestRecoveryPosition) + " / " + up(s.worstRecoveryPosition)
        return t
    }
    function fmtFatigue(s) {
        if (!s || !s.available) return "Not available"
        var t = "Pattern: " + (s.overallPattern || "—")
            + "\nFatigue index: " + f(s.fatigueIndex, 2) + "   Confidence: " + pct(s.confidence)
            + "\nEarly → late avg delta: " + f(s.earlyLateDelta, 2)
        if (s.coordinateTrendAvailable) t += "\nGroup expansion: " + f(s.groupExpansionRate * 100, 0) + "%"
        if (s.timingTrendAvailable) t += "\nTiming change: " + f(s.timingChangeRate * 100, 0) + "%"
        if (s.hasFatiguedPosition) t += "\nMost fatigued: " + up(s.mostFatiguedPosition)
        return t
    }
    function fmtPriorities(list) {
        if (!list || list.length === 0) return "No major training priority detected in this session's data."
        var t = ""
        for (var i = 0; i < list.length; i++) {
            var p = list[i]
            t += (i > 0 ? "\n" : "") + (i + 1) + ". " + p.priority + "   [" + p.impact + " impact, score " + f(p.priorityScore, 0) + "]"
        }
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

    // Section model, rebuilt whenever the report changes.
    property var cards: buildCards(rep)
    function buildCards(r) {
        return [
            { title: "Executive Summary", body: fmtExec(r.executiveSummary) },
            { title: "Score Distribution", body: fmtDist(r.shotDistribution) },
            { title: "Trend", body: fmtTrend(r.trendAnalysis) },
            { title: "Shot Map / Heat Map", body: fmtHeat(r.heatMapAnalysis), map: true },
            { title: "Timing", body: fmtTiming(r.timingAnalysis) },
            { title: "Position Analysis", body: fmtPosition(r.positionAnalysis) },
            { title: "Recovery Analysis", body: fmtRecovery(r.recoveryAnalysis) },
            { title: "Fatigue Analysis", body: fmtFatigue(r.fatigueAnalysis) },
            { title: "Training Priorities", body: fmtPriorities(r.trainingPriorities) },
            { title: "Coach Conclusion", body: fmtConclusion(r.coachConclusion) },
            { title: "Coach Diary", body: fmtDiary(r.coachDiary) }
        ]
    }

    // ---------- shot-map canvas (visual orientation check) ----------
    Component {
        id: shotMapComponent
        Canvas {
            id: mapCanvas
            width: parent ? parent.width : 260
            height: 260
            Connections { target: COACHREPORT; function onReportChanged() { mapCanvas.requestPaint() } }
            Component.onCompleted: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var w = width, h = height, cx = w / 2, cy = h / 2
                // frame + crosshair
                ctx.strokeStyle = theme.borderColor; ctx.lineWidth = 1
                ctx.strokeRect(0.5, 0.5, w - 1, h - 1)
                ctx.beginPath(); ctx.moveTo(cx, 8); ctx.lineTo(cx, h - 8)
                ctx.moveTo(8, cy); ctx.lineTo(w - 8, cy); ctx.stroke()
                // high/low labels so orientation is visible during flip testing
                ctx.fillStyle = theme.textSecondary; ctx.font = "10px sans-serif"
                ctx.fillText("HIGH", cx + 4, 14); ctx.fillText("LOW", cx + 4, h - 6)
                ctx.fillText("R", w - 12, cy - 4); ctx.fillText("L", 4, cy - 4)

                var heat = reportPage.rep ? reportPage.rep.heatMapAnalysis : null
                var g = heat && heat.available ? heat.allShots : null
                if (!g || !g.cells || g.cells.length === 0) {
                    ctx.fillStyle = theme.textSecondary
                    ctx.fillText("No coordinate data", cx - 40, cy + 20)
                    return
                }
                // fit scale from cell extent (mm), leave a margin
                var ext = 1
                for (var i = 0; i < g.cells.length; i++) {
                    ext = Math.max(ext, Math.abs(g.cells[i].centreX), Math.abs(g.cells[i].centreY))
                }
                ext += (g.binSize || 5)
                var scale = (Math.min(w, h) / 2 - 16) / ext
                for (var j = 0; j < g.cells.length; j++) {
                    var c = g.cells[j]
                    var px = cx + scale * c.centreX
                    var py = cy - scale * c.centreY   // +y = high on screen
                    var rad = 3 + Math.min(8, c.count)
                    ctx.beginPath(); ctx.arc(px, py, rad, 0, 2 * Math.PI)
                    ctx.fillStyle = theme.brandPrimary; ctx.globalAlpha = 0.75
                    ctx.fill(); ctx.globalAlpha = 1.0
                }
            }
        }
    }

    // ---------- top bar ----------
    Rectangle {
        id: topBar
        width: parent.width
        height: 52
        color: theme.bgSurface
        anchors.top: parent.top

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 16
            text: "COACH REPORT"
            color: theme.textPrimary; font.bold: true; font.pixelSize: 20; font.family: theme.fontFamily
        }
        Rectangle {
            id: ratingChip
            visible: reportPage.rep && reportPage.rep.coachConclusion && reportPage.rep.coachConclusion.available
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 190
            width: ratingText.width + 20; height: 26; radius: 13
            color: theme.brandPrimary
            Text {
                id: ratingText; anchors.centerIn: parent
                text: (reportPage.rep && reportPage.rep.coachConclusion) ? (reportPage.rep.coachConclusion.rating || "") : ""
                color: theme.textOnBrand; font.pixelSize: 13; font.bold: true; font.family: theme.fontFamily
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right; anchors.rightMargin: 12
            spacing: 12

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Flip Y"
                color: theme.textSecondary; font.pixelSize: 13; font.family: theme.fontFamily
            }
            Switch {
                id: flipSwitch
                anchors.verticalCenter: parent.verticalCenter
                checked: COACHFEED.coordinatesFlipY
                onToggled: {
                    // Orientation is a builder-level input, so re-run the feeder
                    // to recompute the report with the new y convention.
                    COACHFEED.coordinatesFlipY = checked
                    COACHFEED.analyzeCurrentMatch(reportPage.gameSubMode)
                }
            }
            Button {
                anchors.verticalCenter: parent.verticalCenter
                text: "Re-run"
                onClicked: COACHFEED.analyzeCurrentMatch(reportPage.gameSubMode)
            }
            Button {
                anchors.verticalCenter: parent.verticalCenter
                text: "Close"
                onClicked: reportPage.closed()
            }
        }
    }

    // ---------- body ----------
    Text {
        anchors.centerIn: parent
        visible: !(reportPage.rep && reportPage.rep.valid)
        width: parent.width * 0.7
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "No report yet.\n\n" + ((reportPage.rep && reportPage.rep.message) ? reportPage.rep.message
              : "Finish a match, then open the Coach Report.")
        color: theme.textSecondary; font.pixelSize: 16; font.family: theme.fontFamily
    }

    ScrollView {
        id: scroller
        visible: reportPage.rep && reportPage.rep.valid
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: scrollColumn
            width: scroller.availableWidth
            spacing: 12

            Text {
                width: parent.width
                text: (reportPage.rep && reportPage.rep.lowSampleWarning)
                      ? "⚠ Low sample (" + reportPage.f(reportPage.rep.analysedShotCount, 0)
                        + " shots) — results are indicative only." : ""
                visible: text.length > 0
                color: theme.brandAccent; font.pixelSize: 13; font.family: theme.fontFamily
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: reportPage.cards
                delegate: CoachReportCard {
                    width: scrollColumn.width
                    title: modelData.title
                    body: modelData.body
                    Loader {
                        active: modelData.map === true
                        width: parent ? parent.width : 260
                        sourceComponent: shotMapComponent
                    }
                }
            }

            Item { width: 1; height: 12 }  // bottom padding
        }
    }
}
