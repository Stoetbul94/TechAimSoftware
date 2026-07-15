import QtQuick 2.15
import QtCharts 2.15

// 3P FINAL — professional finals report (redesign D1-D6). Pure content,
// hosted like the other report views inside the floating Report window: no
// window chrome, no scrolling of its own; exposes implicitHeight +
// refresh() + exportPdf().
//
// Four A4 pages: (1) identity header · match validation · executive summary
// · stage breakdown · position comparison; (2) K/P/S target plots ·
// momentum · cumulative score; (3) shot-by-shot with hold/split/running
// total; (4) incident log · CRO command timeline · performance summary ·
// coach notes. Layout leaves room for RMS-era additions (lane/target in the
// header, rankings/eliminations/penalties as future page sections).
//
// Every value comes from FINALS3P.buildReport() — the immutable report
// assembled from the controller's stored session state (D1). The view only
// formats; it never reads the shot models, never computes scores, and shows
// DECIMAL scoring only (no qualification integer-primary rules).
//
// Display-only naming: the persisted finalsSeriesIndex schema (0=K, 1=P,
// 2=S1, 3=S2, 4-8=singles) is LOCKED; this view maps it to human-facing
// "Standing Series 1/2" / "Standing Single 31-35" purely for presentation.
Item {
    id: finalsReport

    signal requestClose()

    property var report: null
    property string generatedStamp: ""
    // Shared mm extent for the three target plots so K/P/S render at the SAME
    // zoom (comparable groups). Presentation scaling only — the max radius of
    // the report's stored coordinates, like the coach maps' shared extent.
    property real plotExtentMm: 0

    implicitHeight: pagesCol.height + 40

    // Rebuild the immutable report from the controller's stored state. Called
    // by the host window before presenting (and by Save PDF for freshness).
    function refresh() {
        var now = new Date()
        var meta = {
            "athlete": (typeof userName !== "undefined" && userName) ? userName : "",
            "eventName": "50m Rifle 3 Positions — FINAL (35)",
            "dateTime": now.toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
        }
        finalsReport.report = FINALS3P.buildReport(meta)
        finalsReport.generatedStamp = now.toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
        // Shared plot zoom (presentation only): worst-shot radius across all
        // three position groups.
        var ext = 0
        var plots = finalsReport.report ? finalsReport.report.positionPlots : []
        for (var p = 0; p < plots.length; ++p)
            for (var q = 0; q < plots[p].shots.length; ++q) {
                var sh = plots[p].shots[q]
                ext = Math.max(ext, Math.sqrt(sh.x * sh.x + sh.y * sh.y))
            }
        finalsReport.plotExtentMm = ext
        finalsReport.rebuildCharts()
    }

    // Chart population is imperative in QtCharts; the values plotted are the
    // report's shot rows verbatim (score / runningTotal). Axis bounds only
    // frame the data — no report value is derived here.
    function rebuildCharts() {
        if (typeof momSeries === "undefined" || !momSeries)
            return
        momSeries.clear()
        cumSeries.clear()
        var shots = finalsReport.report ? finalsReport.report.shots : []
        var minScore = 10.9
        var maxTotal = 0
        for (var i = 0; i < shots.length; ++i) {
            var s = shots[i]
            if (!s.provisional) {
                momSeries.append(s.number, s.score)
                if (s.score < minScore) minScore = s.score
            }
            cumSeries.append(s.number, s.runningTotal)
            if (s.runningTotal > maxTotal) maxTotal = s.runningTotal
        }
        momY.min = Math.max(0, Math.floor(minScore) - 1)
        momY.max = 11
        cumY.max = Math.max(50, Math.ceil(maxTotal / 50) * 50)
    }

    // ── display-only mappings ────────────────────────────────────────────
    function stageDisplay(stageId) {
        if (stageId === 3) return "Kneeling"
        if (stageId === 5) return "Prone"
        if (stageId === 7) return "Standing Series 1"
        if (stageId === 8) return "Standing Series 2"
        if (stageId >= 9 && stageId <= 13) return "Standing Single " + (22 + stageId)
        return ""
    }
    function seriesDisplay(si) {
        if (si === 0) return "Kneeling"
        if (si === 1) return "Prone"
        if (si === 2) return "Standing Series 1"
        if (si === 3) return "Standing Series 2"
        if (si >= 4 && si <= 8) return "Standing Single " + (27 + si)
        return ""
    }
    function statusDisplay(name) {
        if (name === "Complete")   return "COMPLETE"
        if (name === "Incomplete") return "INCOMPLETE"
        if (name === "Aborted")    return "ABORTED"
        if (name === "InProgress") return "IN PROGRESS"
        return "—"
    }
    function statusColor(name) {
        if (name === "Complete")   return "#1f8a4c"
        if (name === "Incomplete") return "#c77700"
        if (name === "Aborted")    return "#a80038"
        return "#8a8f98"
    }
    function completionColor(s) {
        if (s === "COMPLETE")   return "#1f8a4c"
        if (s === "INCOMPLETE") return "#c77700"
        if (s === "ABORTED")    return "#a80038"
        return "#8a8f98"
    }
    // Incident severities are derived in the builder; this only colours them.
    function severityColor(s) {
        if (s === "Information") return "#1f8a4c"
        if (s === "Warning")     return "#c77700"
        if (s === "Critical")    return "#a80038"
        return "#8a8f98"
    }
    // Timeline stamps are scaled ms since the final started (controller clock).
    function fmtClock(ms) {
        var s = Math.floor(ms / 1000)
        var m = Math.floor(s / 60)
        return (m < 10 ? "0" : "") + m + ":" + ((s % 60) < 10 ? "0" : "") + (s % 60)
    }
    function summaryVal(key, dflt) {
        return (finalsReport.report && finalsReport.report.summary)
                ? finalsReport.report.summary[key] : dflt
    }
    // Builder-derived validation panel (display only; verdicts never
    // recomputed here).
    function validationChecks() {
        return (finalsReport.report && finalsReport.report.validation)
                ? finalsReport.report.validation.checks : []
    }
    function validationValid() {
        return (finalsReport.report && finalsReport.report.validation)
                ? finalsReport.report.validation.valid === true : false
    }

    // How many rows page 3 can show before pointing at the session journal —
    // the cap is stated on the page, never silent.
    readonly property int maxIncidentRows: 8
    readonly property int maxTimelineRows: 16

    Rectangle {
        anchors.fill: parent
        color: "#dcdad3"                 // grey backdrop; A4 pages sit on top

        Column {
            id: pagesCol
            anchors.horizontalCenter: parent.horizontalCenter
            y: 20
            spacing: 20

            // ── Page 1: identity · validation · summary · stages · bars ──
            Rectangle {
                id: page1
                width: 794; height: 1123          // A4 @ 96 dpi
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 12

                    // ── Report header (finals-local; the shared ReportHeader
                    //    stays untouched for qualification reports) ─────────
                    Item {
                        width: parent.width; height: 88
                        Column {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6
                            Image {
                                source: "qrc:/images/logo/techaim_color.png"
                                height: 30
                                width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                                fillMode: Image.PreserveAspectFit
                                smooth: true; mipmap: true
                            }
                            Column {
                                spacing: 1
                                Text {
                                    text: "3P FINAL REPORT"
                                    color: "#191b1f"; font.family: "Segoe UI"
                                    font.pixelSize: 20; font.bold: true; font.letterSpacing: 0.5
                                }
                                Text {
                                    text: "ISSF 50m Rifle 3 Positions — Final · 35 shots · decimal scoring"
                                    color: "#6b7280"; font.family: "Segoe UI"; font.pixelSize: 11
                                }
                            }
                        }
                        // Protocol meta block (Lane / Target ID reserved for RMS).
                        Grid {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            columns: 2; columnSpacing: 26; rowSpacing: 3
                            Repeater {
                                model: [
                                    { l: "Athlete",    v: finalsReport.summaryVal("athlete", "") },
                                    { l: "Discipline", v: "50m Rifle 3 Positions" },
                                    { l: "Date",       v: finalsReport.summaryVal("dateTime", "") },
                                    { l: "Session",    v: finalsReport.summaryVal("sessionType", "") },
                                    { l: "Software",   v: finalsReport.summaryVal("softwareVersion", "") },
                                    { l: "Mode",       v: "Training" },
                                    { l: "Lane",       v: finalsReport.summaryVal("lane", "") || "—" },
                                    { l: "Target ID",  v: finalsReport.summaryVal("targetId", "") || "—" }
                                ]
                                delegate: Row {
                                    spacing: 6
                                    Text { text: modelData.l + ":"; color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { text: String(modelData.v); color: "#33373d"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                }
                            }
                        }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
                    }

                    // ── MATCH VALIDATION ─────────────────────────────────
                    Rectangle {
                        width: parent.width
                        height: valCol.height + 24
                        radius: 8
                        color: "#f7f8fa"; border.color: "#e6e8ec"; border.width: 1
                        Column {
                            id: valCol
                            x: 16; y: 12; width: parent.width - 32
                            spacing: 8
                            Item {
                                width: parent.width; height: 18
                                Text {
                                    text: "MATCH VALIDATION"
                                    color: "#5b6270"; font.family: "Segoe UI"
                                    font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
                                }
                                Text {
                                    anchors.right: parent.right
                                    text: finalsReport.validationValid()
                                          ? "✓  REPORT VERIFIED" : "⚠  MATCH INCOMPLETE"
                                    color: finalsReport.validationValid() ? "#1f8a4c" : "#c77700"
                                    font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true
                                    font.letterSpacing: 0.5
                                }
                            }
                            Grid {
                                width: parent.width
                                columns: 3; columnSpacing: 12; rowSpacing: 6
                                Repeater {
                                    model: finalsReport.validationChecks()
                                    delegate: Row {
                                        spacing: 6
                                        Text {
                                            text: modelData.ok ? "✓" : "✗"
                                            color: modelData.ok ? "#1f8a4c" : "#a80038"
                                            font.family: "Segoe UI"; font.pixelSize: 12; font.bold: true
                                        }
                                        Text {
                                            text: modelData.label
                                            color: "#191b1f"; font.family: "Segoe UI"; font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                            // Failure reasons — only for failed checks with detail.
                            Column {
                                width: parent.width
                                spacing: 2
                                Repeater {
                                    model: finalsReport.validationChecks().filter(
                                               function (c) { return !c.ok && c.detail.length > 0 })
                                    delegate: Text {
                                        width: parent.width
                                        text: "Reason — " + modelData.label + ": " + modelData.detail
                                        color: "#c77700"; font.family: "Segoe UI"; font.pixelSize: 10
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }

                    SectionTitle { width: parent.width; title: "Executive Summary" }

                    Grid {
                        id: finalsMetricGrid
                        width: parent.width
                        columns: 3; columnSpacing: 10; rowSpacing: 10
                        property real cw: (width - 2 * columnSpacing) / 3

                        MetricCard { width: finalsMetricGrid.cw; label: "Total Score"
                            value: Number(finalsReport.summaryVal("cumulativeTotal", 0)).toFixed(1) }
                        MetricCard { width: finalsMetricGrid.cw; label: "Official Shots"
                            value: finalsReport.summaryVal("officialShotCount", 0) + " / 35" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Average Shot"
                            value: finalsReport.summaryVal("officialShotCount", 0) > 0
                                   ? Number(finalsReport.summaryVal("averageShot", 0)).toFixed(2) : "—" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Highest Shot"; valueSize: 22
                            value: finalsReport.summaryVal("officialShotCount", 0) > 0
                                   ? Number(finalsReport.summaryVal("highestShot", 0)).toFixed(1) : "—"
                            unit: finalsReport.summaryVal("highestShotNumber", 0) > 0
                                  ? "shot " + finalsReport.summaryVal("highestShotNumber", 0) : "" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Lowest Shot"; valueSize: 22
                            value: finalsReport.summaryVal("officialShotCount", 0) > 0
                                   ? Number(finalsReport.summaryVal("lowestShot", 0)).toFixed(1) : "—"
                            unit: finalsReport.summaryVal("lowestShotNumber", 0) > 0
                                  ? "shot " + finalsReport.summaryVal("lowestShotNumber", 0) : "" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Inner 10s"
                            value: "" + finalsReport.summaryVal("innerTens", 0)
                            unit: "≥ 10.2" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Sighting Shots"
                            value: "" + finalsReport.summaryVal("sighterCount", 0) }
                        MetricCard { width: finalsMetricGrid.cw; label: "Missing Shots"
                            value: "" + finalsReport.summaryVal("missingCount", 0)
                            unit: finalsReport.summaryVal("missingCount", 0) > 0 ? "DNS" : "" }
                        MetricCard { width: finalsMetricGrid.cw; label: "Incidents"
                            value: "" + finalsReport.summaryVal("incidentCount", 0)
                            unit: finalsReport.summaryVal("incidentCount", 0) > 0 ? "rejected triggers" : "" }
                    }

                    SectionTitle { width: parent.width; title: "Stage Breakdown" }

                    Column {
                        width: parent.width
                        spacing: 0
                        Rectangle {
                            width: parent.width; height: 24; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.17; height: parent.height; text: "Stage";   leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.09; height: parent.height; text: "Shots";   horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.12; height: parent.height; text: "Score";   horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: "Average"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: "Best";    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: "Worst";   horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.08; height: parent.height; text: "★ 10";    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: "Time";    horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.14; height: parent.height; text: "Status";  horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
                        }
                        Repeater {
                            model: finalsReport.report ? finalsReport.report.stages : []
                            delegate: Rectangle {
                                width: parent.width; height: 24
                                color: index % 2 ? "#f7f8fa" : "#ffffff"
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width*0.17; height: parent.height; text: finalsReport.stageDisplay(modelData.stageId); leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI"; elide: Text.ElideRight }
                                    Text { width: parent.width*0.09; height: parent.height; text: modelData.fired + "/" + modelData.expected; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.12; height: parent.height; text: Number(modelData.subtotal).toFixed(1); horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.10; height: parent.height; text: modelData.fired > 0 ? Number(modelData.average).toFixed(2) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.10; height: parent.height; text: modelData.fired > 0 ? Number(modelData.bestShot).toFixed(1) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.10; height: parent.height; text: modelData.fired > 0 ? Number(modelData.worstShot).toFixed(1) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.08; height: parent.height; text: modelData.fired > 0 ? "" + modelData.innerTens : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.10; height: parent.height; text: modelData.fired > 0 ? modelData.timeUsedSec + " s" : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.14; height: parent.height; text: finalsReport.statusDisplay(modelData.statusName); horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: finalsReport.statusColor(modelData.statusName); font.pixelSize: 9; font.bold: true; font.family: "Segoe UI" }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                            }
                        }
                        // TOTAL row — every figure straight from the report summary.
                        Rectangle {
                            width: parent.width; height: 28; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.17; height: parent.height; text: "TOTAL"; leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.09; height: parent.height; text: finalsReport.summaryVal("officialShotCount", 0) + "/35"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.12; height: parent.height; text: Number(finalsReport.summaryVal("cumulativeTotal", 0)).toFixed(1); horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#a80038"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: finalsReport.summaryVal("officialShotCount", 0) > 0 ? Number(finalsReport.summaryVal("averageShot", 0)).toFixed(2) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: finalsReport.summaryVal("officialShotCount", 0) > 0 ? Number(finalsReport.summaryVal("highestShot", 0)).toFixed(1) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                Text { width: parent.width*0.10; height: parent.height; text: finalsReport.summaryVal("officialShotCount", 0) > 0 ? Number(finalsReport.summaryVal("lowestShot", 0)).toFixed(1) : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                Text { width: parent.width*0.08; height: parent.height; text: "" + finalsReport.summaryVal("innerTens", 0); horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Item { width: parent.width*0.10; height: 1 }
                                Text { width: parent.width*0.14; height: parent.height; text: finalsReport.report ? finalsReport.report.completionStatus : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter; color: finalsReport.completionColor(finalsReport.report ? finalsReport.report.completionStatus : ""); font.pixelSize: 9; font.bold: true; font.family: "Segoe UI" }
                            }
                            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#a80038" }
                        }
                    }

                    SectionTitle { width: parent.width; title: "Position Comparison" }

                    Column {
                        width: parent.width
                        spacing: 8
                        Repeater {
                            model: finalsReport.report ? finalsReport.report.positionComparison : []
                            delegate: Item {
                                width: parent.width; height: 20
                                Text {
                                    width: parent.width * 0.15
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    color: "#5b6270"; font.family: "Segoe UI"
                                    font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.5
                                }
                                Rectangle {
                                    x: parent.width * 0.16
                                    width: parent.width * 0.62; height: 12; radius: 3
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: "#eceef1"
                                    Rectangle {
                                        width: parent.width * modelData.barPct / 100
                                        height: parent.height; radius: 3
                                        color: "#a80038"
                                    }
                                }
                                Text {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: Number(modelData.score).toFixed(1)
                                          + "   (" + modelData.firedShots + "/" + modelData.expectedShots + ")"
                                    color: "#191b1f"; font.family: "Segoe UI"
                                    font.pixelSize: 11; font.bold: true
                                }
                            }
                        }
                    }
                }

                ReportFooter {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 34; anchors.rightMargin: 34; anchors.bottomMargin: 20
                    softwareVersion: "Seta 4.0"
                    generatedText: "Generated " + finalsReport.generatedStamp
                    pageText: "Page 1 / 4"
                }
            }

            // ── Page 2: target plots · momentum · cumulative ─────────────
            Rectangle {
                id: page2
                width: 794; height: 1123
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    id: page2Col
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 12

                    SectionTitle { width: parent.width; title: "Target Plots" }

                    Item {
                        width: parent.width
                        height: 258
                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 14
                            Repeater {
                                model: finalsReport.report ? finalsReport.report.positionPlots : []
                                delegate: FinalsReportTarget {
                                    title: modelData.label
                                    shots: modelData.shots
                                    side: 226
                                    extentMm: finalsReport.plotExtentMm
                                }
                            }
                        }
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: "True ISSF ring geometry · holes at bullet scale · ● ≥ 10.5   ● 10.0 – 10.4   ● < 10.0"
                        color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9
                    }

                    SectionTitle { width: parent.width; title: "Momentum — Shot Score" }

                    ChartView {
                        width: parent.width; height: 280
                        antialiasing: true; legend.visible: false
                        backgroundColor: "transparent"; plotAreaColor: "transparent"
                        theme: ChartView.ChartThemeLight
                        margins.top: 2; margins.bottom: 2; margins.left: 2; margins.right: 2
                        ValueAxis {
                            id: momX
                            min: 1; max: 35; tickCount: 18; labelFormat: "%d"
                            titleText: "Shot"; labelsColor: "#6b7280"
                            gridLineColor: "#eceef1"; color: "#e6e8ec"
                        }
                        ValueAxis {
                            id: momY
                            min: 8; max: 11; labelFormat: "%.1f"
                            titleText: "Decimal score"; labelsColor: "#6b7280"
                            gridLineColor: "#eceef1"; color: "#e6e8ec"
                        }
                        LineSeries {
                            id: momSeries
                            axisX: momX; axisY: momY
                            color: "#a80038"; width: 2
                            pointsVisible: true
                        }
                    }

                    SectionTitle { width: parent.width; title: "Cumulative Score" }

                    ChartView {
                        width: parent.width; height: 280
                        antialiasing: true; legend.visible: false
                        backgroundColor: "transparent"; plotAreaColor: "transparent"
                        theme: ChartView.ChartThemeLight
                        margins.top: 2; margins.bottom: 2; margins.left: 2; margins.right: 2
                        ValueAxis {
                            id: cumX
                            min: 1; max: 35; tickCount: 18; labelFormat: "%d"
                            titleText: "Shot"; labelsColor: "#6b7280"
                            gridLineColor: "#eceef1"; color: "#e6e8ec"
                        }
                        ValueAxis {
                            id: cumY
                            min: 0; max: 400; labelFormat: "%d"
                            titleText: "Running total"; labelsColor: "#6b7280"
                            gridLineColor: "#eceef1"; color: "#e6e8ec"
                        }
                        LineSeries {
                            id: cumSeries
                            axisX: cumX; axisY: cumY
                            color: "#2f6fd0"; width: 2
                        }
                    }
                }

                ReportFooter {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 34; anchors.rightMargin: 34; anchors.bottomMargin: 20
                    softwareVersion: "Seta 4.0"
                    generatedText: "Generated " + finalsReport.generatedStamp
                    pageText: "Page 2 / 4"
                }
            }

            // ── Page 3: shot by shot (35 rows incl. provisional DNS) ─────
            Rectangle {
                id: page3
                width: 794; height: 1123
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 10

                    SectionTitle { width: parent.width; title: "Shot by Shot" }

                    Column {
                        width: parent.width
                        spacing: 0
                        Rectangle {
                            width: parent.width; height: 26; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.07; height: parent.height; text: "#";             leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.21; height: parent.height; text: "Stage";         verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.11; height: parent.height; text: "Score";         horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.11; height: parent.height; text: "Hold";          horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.11; height: parent.height; text: "Split";         horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.15; height: parent.height; text: "Running Total"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.24; height: parent.height; text: "Notes";         horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
                        }
                        Repeater {
                            model: finalsReport.report ? finalsReport.report.shots : []
                            delegate: Rectangle {
                                width: parent.width; height: 24
                                color: modelData.provisional ? "#fdf6ec"
                                                             : (index % 2 ? "#f7f8fa" : "#ffffff")
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width*0.07; height: parent.height; text: modelData.number; leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.21; height: parent.height; text: finalsReport.seriesDisplay(modelData.seriesIndex); verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI"; elide: Text.ElideRight }
                                    Text { width: parent.width*0.11; height: parent.height; text: Number(modelData.score).toFixed(1) + (modelData.innerTen ? " ★" : ""); horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: modelData.provisional ? "#c77700" : "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                    // Hold time: future hardware feature — the builder ships -1 until
                                    // it exists; the column keeps the layout future-proof.
                                    Text { width: parent.width*0.11; height: parent.height; text: modelData.holdSec >= 0 ? modelData.holdSec + " s" : "—"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.11; height: parent.height; text: modelData.provisional ? "—" : modelData.timeUsedSec + " s"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.15; height: parent.height; text: Number(modelData.runningTotal).toFixed(1); horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: modelData.number % 5 === 0; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.24; height: parent.height; text: modelData.note; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#c77700"; font.pixelSize: 9; font.family: "Segoe UI"; elide: Text.ElideRight }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        text: "★ inner ten (≥ 10.2) · Split = time since the previous shot in the firing window · Hold pending hardware support"
                        color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9
                    }
                }

                ReportFooter {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 34; anchors.rightMargin: 34; anchors.bottomMargin: 20
                    softwareVersion: "Seta 4.0"
                    generatedText: "Generated " + finalsReport.generatedStamp
                    pageText: "Page 3 / 4"
                }
            }

            // ── Page 4: incidents · command timeline ─────────────────────
            Rectangle {
                id: page4
                width: 794; height: 1123
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 10

                    SectionTitle { width: parent.width; title: "Incident Log" }

                    Text {
                        visible: !finalsReport.report
                                 || finalsReport.report.incidents.length === 0
                        text: "No incidents — no shots were rejected during this final."
                        color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 11
                    }
                    Column {
                        width: parent.width
                        spacing: 0
                        visible: finalsReport.report && finalsReport.report.incidents.length > 0
                        Rectangle {
                            width: parent.width; height: 22; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width*0.13; height: parent.height; text: "Time";     leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.22; height: parent.height; text: "Stage";    verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.17; height: parent.height; text: "Severity"; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                                Text { width: parent.width*0.48; height: parent.height; text: "Event";    verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI" }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
                        }
                        Repeater {
                            model: finalsReport.report
                                   ? finalsReport.report.incidents.slice(0, finalsReport.maxIncidentRows)
                                   : []
                            delegate: Rectangle {
                                width: parent.width; height: 22
                                color: index % 2 ? "#f7f8fa" : "#ffffff"
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width*0.13; height: parent.height; text: modelData.timestamp.length >= 19 ? modelData.timestamp.substr(11, 8) : modelData.timestamp; leftPadding: 8; verticalAlignment: Text.AlignVCenter; color: "#6b7280"; font.pixelSize: 10; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.22; height: parent.height; text: modelData.stageLabel; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI"; elide: Text.ElideRight }
                                    Row {
                                        width: parent.width*0.17; height: parent.height
                                        spacing: 5
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: finalsReport.severityColor(modelData.severity)
                                        }
                                        Text {
                                            height: parent.height
                                            text: modelData.severity
                                            verticalAlignment: Text.AlignVCenter
                                            color: finalsReport.severityColor(modelData.severity)
                                            font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"
                                        }
                                    }
                                    Text { width: parent.width*0.48; height: parent.height; text: modelData.displayText.length ? modelData.displayText : modelData.reason; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.family: "Segoe UI"; elide: Text.ElideRight; rightPadding: 8 }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                            }
                        }
                        Text {
                            visible: finalsReport.report
                                     && finalsReport.report.incidents.length > finalsReport.maxIncidentRows
                            text: "+ " + (finalsReport.report
                                          ? finalsReport.report.incidents.length - finalsReport.maxIncidentRows : 0)
                                  + " further incidents — see the session journal (finals_session.jsonl)"
                            color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9
                            topPadding: 4
                        }
                    }

                    SectionTitle { width: parent.width; title: "CRO Command Timeline" }

                    Column {
                        width: parent.width
                        spacing: 0
                        Repeater {
                            model: finalsReport.report
                                   ? finalsReport.report.timeline.slice(0, finalsReport.maxTimelineRows)
                                   : []
                            delegate: Item {
                                width: parent.width; height: 19
                                Text { x: 8; width: parent.width*0.11; height: parent.height; text: finalsReport.fmtClock(modelData.issuedAtMs); verticalAlignment: Text.AlignVCenter; color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI" }
                                Text { x: parent.width*0.13; width: parent.width*0.22; height: parent.height; text: modelData.type; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; font.letterSpacing: 0.3 }
                                Text { x: parent.width*0.36; width: parent.width*0.62; height: parent.height; text: modelData.text; verticalAlignment: Text.AlignVCenter; color: "#6b7280"; font.pixelSize: 10; font.family: "Segoe UI"; elide: Text.ElideRight }
                                Rectangle { anchors.bottom: parent.bottom; x: 8; width: parent.width - 16; height: 1; color: "#f0f1f4" }
                            }
                        }
                        Text {
                            visible: finalsReport.report
                                     && finalsReport.report.timeline.length > finalsReport.maxTimelineRows
                            text: "+ " + (finalsReport.report
                                          ? finalsReport.report.timeline.length - finalsReport.maxTimelineRows : 0)
                                  + " further events — see the session journal (finals_session.jsonl)"
                            color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9
                            topPadding: 4
                        }
                    }

                    SectionTitle { width: parent.width; title: "Performance Summary" }

                    Grid {
                        width: parent.width
                        columns: 4; columnSpacing: 12; rowSpacing: 8
                        property real cw: (width - 3 * columnSpacing) / 4
                        property var perf: (finalsReport.report && finalsReport.report.performance
                                            && finalsReport.report.performance.available)
                                           ? finalsReport.report.performance : null
                        Repeater {
                            id: perfRepeater
                            model: {
                                var p = parent.perf
                                if (!p) return []
                                return [
                                    { l: "Highest Shot",   v: Number(p.highestShot).toFixed(1),  d: "shot " + p.highestShotNumber },
                                    { l: "Lowest Shot",    v: Number(p.lowestShot).toFixed(1),   d: "shot " + p.lowestShotNumber },
                                    { l: "Longest Split",  v: p.longestSplitSec + " s",          d: "shot " + p.longestSplitShot },
                                    { l: "Shortest Split", v: p.shortestSplitSec + " s",         d: "shot " + p.shortestSplitShot },
                                    { l: "Fastest Stage",  v: p.fastestStage,                    d: p.fastestStageSec + " s" },
                                    { l: "Slowest Stage",  v: p.slowestStage,                    d: p.slowestStageSec + " s" },
                                    { l: "Standing Avg",   v: Number(p.standingAverage).toFixed(2), d: "shots 21–35" },
                                    { l: "Overall Avg",    v: Number(p.overallAverage).toFixed(2),  d: "all official shots" }
                                ]
                            }
                            delegate: Column {
                                width: perfRepeater.parent.cw
                                spacing: 1
                                Text { text: modelData.l; color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9; font.letterSpacing: 0.4 }
                                Text { text: modelData.v; color: "#191b1f"; font.family: "Segoe UI"; font.pixelSize: 14; font.bold: true }
                                Text { text: modelData.d; color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 9 }
                            }
                        }
                        Text {
                            visible: parent.perf === null
                            text: "Not available — no official shots recorded."
                            color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 11
                        }
                    }

                    SectionTitle { width: parent.width; title: "Coach Notes" }

                    // Ruled lines for handwritten notes — many coaches print
                    // this report (~20% of the page reserved).
                    Column {
                        width: parent.width
                        spacing: 30
                        Repeater {
                            model: 6
                            delegate: Rectangle { width: parent.width; height: 1; color: "#d9dce1" }
                        }
                    }
                }

                ReportFooter {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 34; anchors.rightMargin: 34; anchors.bottomMargin: 20
                    softwareVersion: "Seta 4.0"
                    generatedText: "Generated " + finalsReport.generatedStamp
                    pageText: "Page 4 / 4"
                }
            }
        }
    }

    // Owns the grab: same grabToImage -> CUSTOMPRINT path as the other report
    // views (one image per A4 page, four pages -> finals_report.pdf).
    function exportPdf() {
        finalsReport.refresh()
        CUSTOMPRINT.clearImagesList()
        page1.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        page2.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        page3.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        page4.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath())
        CUSTOMPRINT.createFinalsPdf()
    }
}
