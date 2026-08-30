import QtQuick 2.15

// 10m AIR RIFLE / AIR PISTOL FINAL — operator-facing report (BLOCKER G).
//
// Hosted like the other report views inside the floating Report window: pure
// content, no window chrome, no scrolling of its own; exposes implicitHeight +
// refresh() + exportPdf().
//
// Two A4 pages: (1) identity, result, course structure; (2) the sighters, kept
// visibly apart, then the official 24-shot course shot by shot.
//
// EVERY value comes from FINALS10M.buildReport(). This view formats and
// nothing else: it never reads a shot model, never adds a score, never decides
// which shot belongs to which series, and never infers a role from ordering —
// the report carries shotRole and courseSection on each row.
//
// Scoring is DECIMAL for both disciplines (6.17.2). No qualification tabs, no
// 3P structure, and no placing: one lane cannot know a ranking.
Item {
    id: finals10mReport

    signal requestClose()

    property var report: null
    property string generatedStamp: ""

    implicitHeight: pagesCol.height + 40

    // Rebuild the immutable report from the controller's stored state. Called
    // by the host window before presenting, and again by Save PDF so the
    // exported pages are never staler than the screen.
    function refresh() {
        var now = new Date()
        finals10mReport.report = FINALS10M.buildReport({
            "athlete": (typeof userName !== "undefined" && userName) ? userName : "",
            "dateText": now.toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd"),
            "timeText": now.toLocaleString(Qt.locale(""), "hh:mm")
        })
        finals10mReport.generatedStamp =
            now.toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
    }

    // ── display-only helpers ────────────────────────────────────────────
    function val(key, dflt) {
        if (!finals10mReport.report) return dflt
        var v = finals10mReport.report[key]
        return (v === undefined || v === null) ? dflt : v
    }
    function num(key, digits) {
        return Number(finals10mReport.val(key, 0)).toFixed(digits)
    }
    function sectionList() { return finals10mReport.val("courseSections", []) }
    function sighterList() { return finals10mReport.val("sighters", []) }
    // Whole seconds, as recorded. A record without a time shows a dash rather
    // than a zero an athlete could read as an instant shot.
    function shotTime(rec) {
        if (rec === undefined || rec.timeSec === undefined) return "—"
        return String(rec.timeSec) + " s"
    }
    function mm(v) {
        return (v === undefined || !isFinite(v)) ? "—" : Number(v).toFixed(2)
    }
    // Build identity, or nothing. A report that cannot name its build must say
    // nothing rather than print "undefined" - ReportHeader drops empty pairs,
    // so an absent identity leaves no orphan row behind.
    function buildIdentity() {
        if (typeof PRODUCT === "undefined") return ""
        var v = PRODUCT.version, c = PRODUCT.gitCommit
        if (v === undefined && c === undefined) return ""
        if (v === undefined) return String(c)
        if (c === undefined) return String(v)
        return String(v) + " · " + String(c)
    }

    Rectangle {
        anchors.fill: parent
        color: "#dcdad3"                       // grey backdrop; A4 pages on top

        Column {
            id: pagesCol
            anchors.horizontalCenter: parent.horizontalCenter
            y: 20
            spacing: 20

            // ══ Page 1: identity, result, course structure ═══════════════
            Rectangle {
                id: page1
                width: 794; height: 1123       // A4 @ 96 dpi
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 14

                    ReportHeader {
                        width: parent.width
                        reportTitle: "FINAL REPORT"
                        athlete: finals10mReport.val("athlete", "")
                        discipline: finals10mReport.val("displayName", "")
                        sessionType: "Final"
                        dateText: finals10mReport.val("dateText", "")
                        timeText: finals10mReport.val("timeText", "")
                        mode: finals10mReport.val("operatingMode", "")
                        extraPairs: [
                            { l: "Session", v: String(finals10mReport.val("sessionId", "")).substring(0, 18) },
                            { l: "Build", v: finals10mReport.buildIdentity() }
                        ]
                    }

                    // ── RESULT ───────────────────────────────────────────
                    SectionTitle { width: parent.width; title: "RESULT" }
                    Grid {
                        id: resultGrid
                        width: parent.width
                        columns: 4
                        columnSpacing: 12
                        rowSpacing: 12
                        readonly property real cw: (width - 3 * columnSpacing) / 4
                        MetricCard {
                            width: resultGrid.cw; label: "Final total"; valueSize: 26
                            value: finals10mReport.num("total", 1)
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Official shots"; valueSize: 22
                            value: finals10mReport.val("acceptedOfShots", "—")
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Sighters"; valueSize: 26
                            value: String(finals10mReport.val("sighterCount", 0))
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Mean shot time"; valueSize: 22
                            value: finals10mReport.num("meanShotTimeSec", 1); unit: "s"
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Series 1 (1-5)"; valueSize: 22
                            value: finals10mReport.num("series1Subtotal", 1)
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Series 2 (6-10)"; valueSize: 22
                            value: finals10mReport.num("series2Subtotal", 1)
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Single shots (11-24)"; valueSize: 22
                            value: finals10mReport.num("singlesSubtotal", 1)
                        }
                        MetricCard {
                            width: resultGrid.cw; label: "Scoring"; valueSize: 18
                            value: String(finals10mReport.val("scoringMode", "decimal")).toUpperCase()
                        }
                    }

                    // ── GROUP ────────────────────────────────────────────
                    SectionTitle { width: parent.width; title: "GROUP" }
                    Grid {
                        id: groupGrid
                        width: parent.width
                        columns: 4
                        columnSpacing: 12
                        readonly property real cw: (width - 3 * columnSpacing) / 4
                        MetricCard {
                            width: groupGrid.cw; label: "MPI X"; valueSize: 22
                            value: finals10mReport.num("mpiXmm", 2); unit: "mm"
                        }
                        MetricCard {
                            width: groupGrid.cw; label: "MPI Y"; valueSize: 22
                            value: finals10mReport.num("mpiYmm", 2); unit: "mm"
                        }
                        MetricCard {
                            width: groupGrid.cw; label: "MPI radius"; valueSize: 22
                            value: finals10mReport.num("mpiRadiusMm", 2); unit: "mm"
                        }
                        MetricCard {
                            width: groupGrid.cw; label: "Group extent"; valueSize: 22
                            value: finals10mReport.num("groupExtentMm", 2); unit: "mm"
                        }
                    }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        text: "Mean point of impact and group extent are measured over the "
                              + "official shots only; sighters are excluded."
                        color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI"
                    }

                    // ── COURSE OF FIRE ───────────────────────────────────
                    SectionTitle { width: parent.width; title: "COURSE OF FIRE" }
                    Column {
                        width: parent.width
                        spacing: 0
                        Rectangle {
                            width: parent.width; height: 24; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width * 0.34; height: parent.height; text: "Section"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.26; height: parent.height; text: "Shots"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.18; height: parent.height; text: "Count"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.22; height: parent.height; text: "Subtotal"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: PRODUCT.accentPrimary }
                        }
                        Repeater {
                            model: finals10mReport.sectionList()
                            delegate: Rectangle {
                                width: parent.width; height: 24; color: "transparent"
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width * 0.34; height: parent.height; text: modelData.label; color: "#191b1f"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.26; height: parent.height; text: modelData.range; color: "#5b6270"; font.pixelSize: 11; font.family: "Segoe UI"; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.18; height: parent.height; text: String(modelData.shotCount); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.22; height: parent.height; text: Number(modelData.subtotal).toFixed(1); color: "#191b1f"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 28; color: "#fbfbfc"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width * 0.78; height: parent.height; text: "FINAL TOTAL"; color: "#191b1f"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.22; height: parent.height; text: finals10mReport.num("total", 1); color: PRODUCT.accentPrimary; font.pixelSize: 14; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                            }
                        }
                    }

                    // ── SIGHTERS ─────────────────────────────────────────
                    // Kept apart from the course on purpose: a sighter is not
                    // part of any series, any single shot, or the total.
                    SectionTitle { width: parent.width; title: "SIGHTERS" }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        text: "Sighting shots. They do not count towards the official "
                              + "shot count, the series, the single shots or the Final total."
                        color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI"
                    }
                    Text {
                        visible: finals10mReport.sighterList().length === 0
                        text: "No sighting shots recorded for this Final."
                        color: "#5b6270"; font.pixelSize: 11; font.family: "Segoe UI"
                    }
                    Column {
                        width: parent.width
                        spacing: 0
                        visible: finals10mReport.sighterList().length > 0
                        Rectangle {
                            width: parent.width; height: 22; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width * 0.16; height: parent.height; text: "Sighter"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.18; height: parent.height; text: "Score"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.20; height: parent.height; text: "X (mm)"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.20; height: parent.height; text: "Y (mm)"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.26; height: parent.height; text: "Time"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#8a8f98" }
                        }
                        Repeater {
                            model: finals10mReport.sighterList()
                            delegate: Rectangle {
                                width: parent.width; height: 20; color: "transparent"
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width * 0.16; height: parent.height; text: "S" + (index + 1); color: "#5b6270"; font.pixelSize: 11; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.18; height: parent.height; text: Number(modelData.score).toFixed(1); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.20; height: parent.height; text: finals10mReport.mm(modelData.xmm); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.20; height: parent.height; text: finals10mReport.mm(modelData.ymm); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                    Text { width: parent.width * 0.26; height: parent.height; text: finals10mReport.shotTime(modelData); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                            }
                        }
                    }

                    // Single lane. Placing is not this application's to state.
                    Rectangle {
                        width: parent.width; height: rankNote.implicitHeight + 16
                        color: "#f7f8fa"; radius: 6
                        border.color: "#e6e8ec"; border.width: 1
                        Text {
                            id: rankNote
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            wrapMode: Text.WordWrap
                            text: finals10mReport.val("rankingNote", "")
                            color: "#5b6270"; font.pixelSize: 10; font.family: "Segoe UI"
                        }
                    }
                }

                ReportFooter {
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 22
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 34; anchors.rightMargin: 34
                    softwareVersion: (typeof PRODUCT !== "undefined"
                                      && PRODUCT.softwareVersionLabel !== undefined)
                                     ? PRODUCT.softwareVersionLabel : ""
                    generatedText: "Generated " + finals10mReport.generatedStamp
                    pageText: "Page 1 of 2"
                }
            }

            // ══ Page 2: sighters, then the official course shot by shot ══
            Rectangle {
                id: page2
                width: 794; height: 1123
                color: "white"; border.color: "#e6e8ec"; border.width: 1

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 34
                    spacing: 12

                    ReportHeader {
                        width: parent.width
                        reportTitle: "FINAL REPORT"
                        athlete: finals10mReport.val("athlete", "")
                        discipline: finals10mReport.val("displayName", "")
                        sessionType: "Final"
                        dateText: finals10mReport.val("dateText", "")
                        timeText: finals10mReport.val("timeText", "")
                        mode: finals10mReport.val("operatingMode", "")
                    }

                    // ── THE OFFICIAL COURSE ──────────────────────────────
                    SectionTitle { width: parent.width; title: "OFFICIAL COURSE" }
                    Column {
                        width: parent.width
                        spacing: 0
                        Rectangle {
                            width: parent.width; height: 22; color: "#f1f3f5"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width * 0.16; height: parent.height; text: "Shot"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.18; height: parent.height; text: "Score"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.20; height: parent.height; text: "X (mm)"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.20; height: parent.height; text: "Y (mm)"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.26; height: parent.height; text: "Time"; color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: PRODUCT.accentPrimary }
                        }
                        // One block per rule section, in course order. The
                        // grouping is the report's, not this view's.
                        Repeater {
                            model: finals10mReport.sectionList()
                            delegate: Column {
                                width: parent.width
                                property var section: modelData
                                Rectangle {
                                    width: parent.width; height: 20; color: "#f7f8fa"
                                    Row {
                                        anchors.fill: parent
                                        Text { width: parent.width * 0.60; height: parent.height; text: section.label + "  ·  " + section.range; color: PRODUCT.accentPrimary; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                        Text { width: parent.width * 0.40; height: parent.height; text: "Subtotal " + Number(section.subtotal).toFixed(1); color: "#5b6270"; font.pixelSize: 10; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                                    }
                                }
                                Repeater {
                                    model: section.shots
                                    delegate: Rectangle {
                                        width: parent.width; height: 20; color: "transparent"
                                        Row {
                                            anchors.fill: parent
                                            Text { width: parent.width * 0.16; height: parent.height; text: String(modelData.officialShotNumber); color: "#5b6270"; font.pixelSize: 11; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                            Text { width: parent.width * 0.18; height: parent.height; text: Number(modelData.score).toFixed(1); color: "#191b1f"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                            Text { width: parent.width * 0.20; height: parent.height; text: finals10mReport.mm(modelData.xmm); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                            Text { width: parent.width * 0.20; height: parent.height; text: finals10mReport.mm(modelData.ymm); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
                                            Text { width: parent.width * 0.26; height: parent.height; text: finals10mReport.shotTime(modelData); color: "#33373d"; font.pixelSize: 11; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                                        }
                                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 26; color: "#fbfbfc"
                            Row {
                                anchors.fill: parent
                                Text { width: parent.width * 0.60; height: parent.height; text: "FINAL TOTAL  ·  " + finals10mReport.val("acceptedOfShots", ""); color: "#191b1f"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI"; leftPadding: 10; verticalAlignment: Text.AlignVCenter }
                                Text { width: parent.width * 0.40; height: parent.height; text: finals10mReport.num("total", 1); color: PRODUCT.accentPrimary; font.pixelSize: 14; font.bold: true; font.family: "Segoe UI"; horizontalAlignment: Text.AlignRight; rightPadding: 10; verticalAlignment: Text.AlignVCenter }
                            }
                        }
                    }

                    // Stated, never silent: an incomplete course says so.
                    Text {
                        visible: !finals10mReport.val("complete", false)
                        width: parent.width; wrapMode: Text.WordWrap
                        text: "COURSE INCOMPLETE — "
                              + finals10mReport.val("acceptedOfShots", "")
                              + " official shots recorded. This is not a completed Final result."
                        color: PRODUCT.accentPrimary; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI"
                    }
                }

                ReportFooter {
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 22
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 34; anchors.rightMargin: 34
                    softwareVersion: (typeof PRODUCT !== "undefined"
                                      && PRODUCT.softwareVersionLabel !== undefined)
                                     ? PRODUCT.softwareVersionLabel : ""
                    generatedText: "Generated " + finals10mReport.generatedStamp
                    pageText: "Page 2 of 2"
                }
            }
        }
    }

    // PDF: the SAME report model as the screen, grabbed page by page. The grab
    // result is only valid inside its own callback (report-system rule).
    function exportPdf() {
        finals10mReport.refresh()
        CUSTOMPRINT.clearImagesList()
        page1.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        page2.grabToImage(function(result) { CUSTOMPRINT.addImage(result.image) },
                          Qt.size(8917/4, 13033/4))
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath())
        CUSTOMPRINT.createFinalsPdf(finals10mReport.val("displayName", "10m Final")
                                    + " Report", "final_report.pdf")
    }
}
