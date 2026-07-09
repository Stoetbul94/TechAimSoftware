import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

// Printable A4-style Coach Report (light, clean). Display of the single
// COACHREPORT data plus editable Coach Diary fields (saved via
// COACHREPORT.setCoachDiary). No analytics here. Basis for future PDF export.
Rectangle {
    id: pg
    color: "#dcdad3"                      // grey backdrop; the white "page" sits on top

    property int gameSubMode: 0
    property var rep: COACHREPORT.report
    property var targets: []

    signal closed()
    signal dashboardRequested()
    signal detailsRequested()

    // When hosted inside a FloatingWindow, the window supplies the title bar,
    // Dashboard/Detailed/Print tabs and Export/Close actions — so the internal
    // toolbar is hidden to avoid duplicate chrome. Content, diary and PDF export
    // are unchanged.
    property bool embedded: false

    // ---- print palette ----
    readonly property color cPage:   "#ffffff"
    readonly property color cInk:    "#161616"
    readonly property color cSub:    "#555555"
    readonly property color cRule:   "#c9c5bb"
    readonly property color cAccent: "#a80038"
    readonly property color cGreen:  "#2e7d46"
    readonly property color cAmber:  "#b9891b"
    readonly property color cRed:    "#c0392b"
    readonly property string fam: "Segoe UI"

    Connections {
        target: COACHREPORT
        function onReportChanged() { pg.refresh() }
    }
    onVisibleChanged: if (visible) { refresh(); loadDiary() }
    Component.onCompleted: refresh()

    // ---- helpers ----
    function f(x,d){ if(x===undefined||x===null) return "—"; if(typeof x!=="number") return String(x); return x.toFixed(d===undefined?2:d) }
    function pct(x){ return (x===undefined||x===null)?"—":f(x,1)+"%" }
    function up(x){ return x?String(x).toUpperCase():"—" }
    function competition(s){ return s.isValid && s.isCompetitionShot && !s.isSighter }
    // Discipline derived from the report's own positions (single source).
    function positionsPresent(){ var out=[]; if(rep&&rep.positionAnalysis&&rep.positionAnalysis.positions) for(var i=0;i<rep.positionAnalysis.positions.length;i++) out.push(rep.positionAnalysis.positions[i].position); return out }
    function specKey(){ var p=positionsPresent(); if(p.indexOf("airPistol")>=0)return "airpistol10"; if(p.indexOf("airRifle")>=0)return "airrifle10"; return "rifle50" }
    function disciplineLabel(){ var p=positionsPresent(); if(p.indexOf("airPistol")>=0)return "Air Pistol (10m)"; if(p.indexOf("airRifle")>=0)return "Air Rifle (10m)"; if(p.indexOf("kneeling")>=0&&p.indexOf("standing")>=0&&p.indexOf("prone")>=0)return "3-Position Rifle (50m)"; if(p.indexOf("prone")>=0&&p.length===1)return "Prone Rifle (50m)"; return "50m Rifle" }
    function recoveryLabel(p){ switch(p){case "GoodRecovery":return "Recovers well after mistakes";case "SlowRecovery":return "Slow to recover after mistakes";case "RepeatedErrorPattern":return "Mistakes tend to cluster together";case "OverCorrectionPattern":return "Over-corrects after mistakes";default:return "Not enough poor shots to assess"} }
    function fatigueLabel(p){ switch(p){case "NoFatigueDetected":return "No fatigue signal — held up well";case "GradualDecline":return "Gradual decline through the session";case "LateMatchDrop":return "Score dropped late in the match";case "IncreasingDispersion":return "Group opened up over the session";case "TimingSlowdown":return "Shot timing slowed late";case "FatigueCompensation":return "Working harder late to hold the score";case "PositionSpecificFatigue":return "Fatigue localised to one position";default:return "Not enough data to assess fatigue"} }

    function refresh(){
        var all = COACHREPORT.shots().filter(competition)
        var groups={}, order=[]
        for(var i=0;i<all.length;i++){ var k=all[i].position; if(!groups[k]){groups[k]=[];order.push(k)} groups[k].push(all[i]) }
        var list=[]
        if(order.length>1) for(var m=0;m<order.length;m++) list.push({title:order[m], shots:groups[order[m]]})
        list.push({title:"All", shots:all})
        pg.targets=list
        rebuildTrend()
    }
    function rebuildTrend(){
        if(typeof pScore==="undefined"||!pScore) return
        pScore.clear(); pRoll.clear()
        var t=rep?rep.trendAnalysis:null
        if(!t||!t.scores||t.scores.length===0) return
        var n=t.scores.length
        for(var i=0;i<n;i++) pScore.append(i+1,t.scores[i])
        if(t.rolling5Available&&t.rolling5Average) for(var j=0;j<t.rolling5Average.length;j++) pRoll.append(j+5,t.rolling5Average[j])
        pAxX.min=1; pAxX.max=Math.max(2,n)
    }
    function evLines(metrics){ var o=[]; if(!metrics) return o; for(var i=0;i<metrics.length;i++){ var e=metrics[i].evidence; for(var j=0;j<e.length;j++) o.push(e[j].statement) } return o }

    // ---- diary edit state ----
    property var focusOptions: ["Trigger control","Follow-through","Position stability","Breathing","Rhythm & timing","Recovery routine","Endurance","Sight picture"]
    property var focusChecked: ({})
    property int diaryVer: 0   // bumped on reload to force checkbox delegates to rebuild
    function loadDiary(){
        var d = COACHREPORT.coachDiary()
        dfEquip.text=d.equipmentNotes||""; dfSight.text=d.sightAdjustmentNotes||""; dfAmmo.text=d.ammunitionNotes||""
        dfWeather.text=d.rangeConditionNotes||""; dfWind.text=d.windNotes||""; dfLight.text=d.lightingNotes||""
        dfPhys.text=d.physicalConditionNotes||""; dfMental.text=d.mentalStateNotes||""
        dfCoach.text=d.coachNotes||""; dfAthlete.text=d.athleteNotes||""; dfNext.text=d.nextSessionFocus||""
        var fc={}; var tags=d.tags||[]; for(var i=0;i<tags.length;i++) fc[tags[i]]=true; pg.focusChecked=fc
        pg.diaryVer++   // force the focus checkboxes to rebuild from the reloaded state
    }
    function saveDiary(){
        var cur = COACHREPORT.coachDiary()
        var tags=[]; for(var k in pg.focusChecked) if(pg.focusChecked[k]) tags.push(k)
        COACHREPORT.setCoachDiary({
            sessionType: cur.sessionType, sessionIntensity: cur.sessionIntensity, createdAt: cur.createdAt,
            equipmentNotes: dfEquip.text, sightAdjustmentNotes: dfSight.text, ammunitionNotes: dfAmmo.text,
            rangeConditionNotes: dfWeather.text, windNotes: dfWind.text, lightingNotes: dfLight.text,
            physicalConditionNotes: dfPhys.text, mentalStateNotes: dfMental.text,
            coachNotes: dfCoach.text, athleteNotes: dfAthlete.text, nextSessionFocus: dfNext.text, tags: tags
        })
        savedFlash.text = "✓ diary saved"; savedFlash.visible = true; flashTimer.restart()
    }
    Timer { id: flashTimer; interval: 2000; onTriggered: savedFlash.visible=false }

    // ---- PDF export (grabs Print-view sections; no analysis) ----
    function athleteName(){ return (typeof userName!=="undefined"&&userName)?userName:"Athlete" }
    function exportPdf(){
        if(!(rep && rep.valid)) return
        var secs=[]
        for(var i=0;i<content.children.length;i++){
            var c=content.children[i]
            if(c && c.visible && c.width>4 && c.height>8) secs.push(c)
        }
        if(secs.length===0) return
        PDFEXPORT.clear()
        var results=new Array(secs.length); var remaining=secs.length
        for(var j=0;j<secs.length;j++){
            (function(idx){
                secs[idx].grabToImage(function(res){
                    results[idx]=res; remaining--
                    if(remaining===0){
                        for(var k=0;k<results.length;k++) if(results[k]) PDFEXPORT.addSection(results[k].image)
                        PDFEXPORT.exportToFile(PDFEXPORT.suggestFileName(pg.athleteName(), pg.disciplineLabel(), Qt.formatDateTime(new Date(),"yyyy-MM-dd")))
                    }
                }, Qt.size(secs[idx].width*2, secs[idx].height*2))
            })(j)
        }
    }
    Connections {
        target: PDFEXPORT
        function onExportFinished(path){ if(path && path.length){ savedFlash.text="✓ PDF exported"; savedFlash.visible=true; flashTimer.restart() } }
    }

    // ================= toolbar (screen only) =================
    Rectangle {
        id: bar; anchors.top: parent.top; width: parent.width; color: "#2a2a2e"
        visible: !pg.embedded; height: pg.embedded ? 0 : 44
        Row {
            anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 10
            Text { text: "PRINT PREVIEW"; color: "#f0f0f0"; font.bold: true; font.pixelSize: 14; font.family: pg.fam; anchors.verticalCenter: parent.verticalCenter }
            Text { id: savedFlash; visible: false; text: "✓ diary saved"; color: "#7fd18f"; font.pixelSize: 13; font.family: pg.fam; anchors.verticalCenter: parent.verticalCenter }
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 12; spacing: 10
            Button { text: "⤓ Export PDF"; onClicked: pg.exportPdf() }
            Button { text: "Save Diary"; onClicked: pg.saveDiary() }
            Button { text: "Dashboard"; onClicked: pg.dashboardRequested() }
            Button { text: "Detailed"; onClicked: pg.detailsRequested() }
            Button { text: "Close"; onClicked: pg.closed() }
        }
    }

    Flickable {
        anchors.top: bar.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        clip: true; contentWidth: width; contentHeight: pageWrap.height + 40
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

        Item {
            id: pageWrap; width: parent.width; height: page.height + 40
            // The A4-ish white page, centred.
            Rectangle {
                id: page
                width: Math.min(794, pageWrap.width - 40)   // A4 width at 96 dpi (210 mm)
                x: (pageWrap.width - width) / 2; y: 20
                height: content.implicitHeight + 56
                color: pg.cPage; border.color: pg.cRule; border.width: 1

                Column {
                    id: content; x: 28; y: 26; width: parent.width - 56; spacing: 16
                    visible: pg.rep && pg.rep.valid

                    // ---- header ----
                    ReportHeader {
                        width: parent.width
                        reportTitle: "Coach Report"
                        athlete: pg.athleteName()
                        discipline: pg.disciplineLabel()
                        sessionType: "Match"
                        dateText: Qt.formatDateTime(new Date(),"d MMM yyyy")
                        timeText: Qt.formatDateTime(new Date(),"HH:mm")
                    }

                    // Executive Summary
                    Column {
                        width: parent.width; spacing: 10
                        property var e: pg.rep ? pg.rep.executiveSummary : null
                        SectionTitle { width: parent.width; title: "Executive Summary" }
                        Grid {
                            id: execGrid
                            width: parent.width; columns: 3; columnSpacing: 12; rowSpacing: 12
                            property real cw: (width - 2*columnSpacing) / 3
                            property var e: parent.e
                            MetricCard { width: execGrid.cw; label: "Shots";          value: execGrid.e ? pg.f(execGrid.e.competitionShotCount,0) : "—" }
                            MetricCard { width: execGrid.cw; label: "Total Score";    value: execGrid.e ? pg.f(execGrid.e.totalScore,1) : "—" }
                            MetricCard { width: execGrid.cw; label: "Average";        value: execGrid.e ? pg.f(execGrid.e.averageScore,2) : "—" }
                            MetricCard { width: execGrid.cw; label: "Consistency";    valueSize: 22; value: execGrid.e ? pg.pct(execGrid.e.consistencyPercentage) : "—" }
                            MetricCard { width: execGrid.cw; label: "Best / Worst";   valueSize: 18; value: execGrid.e ? pg.f(execGrid.e.bestShot,1)+" / "+pg.f(execGrid.e.worstShot,1) : "—" }
                            MetricCard { width: execGrid.cw; label: "Score SD";       valueSize: 20; value: execGrid.e ? pg.f(execGrid.e.scoreStandardDeviation,3) : "—" }
                            MetricCard { width: execGrid.cw; label: "Group Radius";   unit: "mm"; valueSize: 20; value: execGrid.e ? pg.f(execGrid.e.groupRadius,1) : "—" }
                            MetricCard { width: execGrid.cw; label: "Group Diameter"; unit: "mm"; valueSize: 20; value: execGrid.e ? pg.f(execGrid.e.groupDiameter,1) : "—" }
                            MetricCard { width: execGrid.cw; label: "MPI"; unit: "mm"; valueSize: 18; value: execGrid.e ? pg.f(execGrid.e.horizontalBias,1)+", "+pg.f(execGrid.e.verticalBias,1) : "—" }
                            MetricCard { width: execGrid.cw; label: "Trend";          valueSize: 18; value: execGrid.e ? (execGrid.e.trendDirection||"—") : "—" }
                        }
                    }

                    // Shot Map (ISSF targets)
                    Column {
                        width: parent.width; spacing: 6
                        SectionTitle { width: parent.width; title: "Shot Map" }
                        Flow {
                            width: parent.width; spacing: 12
                            Repeater {
                                model: pg.targets
                                delegate: Column {
                                    spacing: 2
                                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: pg.up(modelData.title)+" ("+modelData.shots.length+")"; color: pg.cInk; font.bold: true; font.pixelSize: 11; font.family: pg.fam }
                                    IssfTargetCanvas { shots: modelData.shots; side: 168; discipline: pg.specKey() }
                                }
                            }
                        }
                    }

                    // Score Distribution (print-friendly bars)
                    Column {
                        width: parent.width; spacing: 6
                        SectionTitle { width: parent.width; title: "Score Distribution" }
                        Repeater {
                            model: {
                                var d = pg.rep ? pg.rep.shotDistribution : null; if(!d) return []
                                return [
                                    {l:"Perfect ≥10.8", c:d.perfectCount, p:d.perfectPct, col:"#2e7d46"},
                                    {l:"Excellent 10.5–10.7", c:d.excellentCount, p:d.excellentPct, col:"#5bc07e"},
                                    {l:"Good 10.2–10.4", c:d.goodCount, p:d.goodPct, col:"#b9891b"},
                                    {l:"Acceptable 10.0–10.1", c:d.acceptableCount, p:d.acceptablePct, col:"#d0a83a"},
                                    {l:"Recovery 9.5–9.9", c:d.recoveryCount, p:d.recoveryPct, col:"#d1762e"},
                                    {l:"Poor <9.5", c:d.poorCount, p:d.poorPct, col:"#c0392b"}
                                ]
                            }
                            delegate: Row {
                                width: parent.width; spacing: 8; height: 18
                                Text { width: parent.width*0.26; text: modelData.l; color: pg.cInk; font.pixelSize: 11; font.family: pg.fam; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                Rectangle {
                                    width: parent.width*0.52; height: 12; radius: 2; color: "#eee"; anchors.verticalCenter: parent.verticalCenter
                                    Rectangle { width: parent.width*Math.min(1,(modelData.p||0)/100); height: parent.height; radius: 2; color: modelData.col }
                                }
                                Text { width: parent.width*0.18; text: modelData.c+"  ("+pg.f(modelData.p,1)+"%)"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
                    }

                    // Trend
                    Column {
                        width: parent.width; spacing: 6
                        SectionTitle { width: parent.width; title: "Trend" }
                        ChartView {
                            width: parent.width; height: 220; antialiasing: true; legend.visible: true; legend.alignment: Qt.AlignTop
                            backgroundColor: "transparent"; plotAreaColor: "transparent"; theme: ChartView.ChartThemeLight
                            margins.top: 0; margins.bottom: 0; margins.left: 0; margins.right: 0
                            ValueAxis { id: pAxX; min: 1; max: 10; labelFormat: "%d" }
                            ValueAxis { id: pAxY; min: 8; max: 11; labelFormat: "%.1f" }
                            LineSeries { id: pScore; name: "Shot Score"; axisX: pAxX; axisY: pAxY; color: "#2f6fd0"; width: 2 }
                            LineSeries { id: pRoll; name: "Rolling Avg (5)"; axisX: pAxX; axisY: pAxY; color: pg.cAccent; width: 2 }
                            Component.onCompleted: pg.rebuildTrend()
                        }
                    }

                    // Position Summary
                    Column {
                        width: parent.width; spacing: 6
                        visible: pg.rep && pg.rep.positionAnalysis && pg.rep.positionAnalysis.positions.length > 1
                        SectionTitle { width: parent.width; title: "Position Summary" }
                        Row {
                            width: parent.width
                            Repeater {
                                model: ["Position","Shots","Total","Avg","Best","Worst","SD","MR mm","Ø mm"]
                                delegate: Text { width: index===0?parent.width*0.2:parent.width*0.1; text: modelData; color: pg.cSub; font.bold: true; font.pixelSize: 11; font.family: pg.fam }
                            }
                        }
                        Repeater {
                            model: (pg.rep&&pg.rep.positionAnalysis)?pg.rep.positionAnalysis.positions:[]
                            delegate: Row {
                                width: parent.width; property var m: modelData.measurements
                                Text { width: parent.width*0.2; text: pg.up(modelData.position); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(modelData.shotCount,0); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.totalScore,1); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.averageScore,2); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.maxScore,1); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.minScore,1); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.scoreSD,2); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.geometry?m.geometry.meanRadius:undefined,1); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                                Text { width: parent.width*0.1; text: pg.f(m.geometry?m.geometry.groupDiameter:undefined,1); color: pg.cInk; font.pixelSize: 11; font.family: pg.fam }
                            }
                        }
                    }

                    // Recovery
                    Column {
                        width: parent.width; spacing: 4
                        property var ra: pg.rep ? pg.rep.recoveryAnalysis : null
                        SectionTitle { width: parent.width; title: "Recovery Analysis" }
                        Text { text: pg.recoveryLabel(parent.ra?parent.ra.overall.pattern:""); color: pg.cInk; font.bold: true; font.pixelSize: 13; font.family: pg.fam }
                        Text { visible: parent.ra&&parent.ra.overall.available; text: "Recovery rate "+pg.pct(parent.ra?parent.ra.overall.badShotRecoveryRate:0)+"   ·   confidence "+pg.pct(parent.ra?parent.ra.overall.recoveryConfidence:0); color: pg.cSub; font.pixelSize: 12; font.family: pg.fam }
                        Repeater { model: pg.evLines(parent.ra?parent.ra.metrics:[]); delegate: Text { width: parent.width; text: "• "+modelData; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam; wrapMode: Text.WordWrap } }
                    }

                    // Fatigue
                    Column {
                        width: parent.width; spacing: 4
                        property var fa: pg.rep ? pg.rep.fatigueAnalysis : null
                        SectionTitle { width: parent.width; title: "Fatigue Analysis" }
                        Text { text: pg.fatigueLabel(parent.fa?parent.fa.overallPattern:""); color: pg.cInk; font.bold: true; font.pixelSize: 13; font.family: pg.fam }
                        Text { visible: parent.fa&&parent.fa.scoreTrendAvailable; text: "Fatigue index "+pg.f(parent.fa?parent.fa.fatigueIndex:0,2)+"   ·   confidence "+pg.pct(parent.fa?parent.fa.confidence:0); color: pg.cSub; font.pixelSize: 12; font.family: pg.fam }
                        Repeater { model: pg.evLines(parent.fa?parent.fa.metrics:[]); delegate: Text { width: parent.width; text: "• "+modelData; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam; wrapMode: Text.WordWrap } }
                    }

                    // Training Priorities
                    Column {
                        width: parent.width; spacing: 4
                        SectionTitle { width: parent.width; title: "Training Priorities" }
                        Repeater {
                            model: (pg.rep&&pg.rep.trainingPriorities)?pg.rep.trainingPriorities:[]
                            delegate: Text { width: parent.width; text: (index+1)+".  "+modelData.priority+"   ["+modelData.impact+" impact]"; color: pg.cInk; font.pixelSize: 12; font.family: pg.fam; wrapMode: Text.WordWrap }
                        }
                        Text { visible: !(pg.rep&&pg.rep.trainingPriorities&&pg.rep.trainingPriorities.length); text: "No major training priority detected."; color: pg.cSub; font.pixelSize: 12; font.family: pg.fam }
                    }

                    // Coach Conclusion
                    Column {
                        width: parent.width; spacing: 4
                        property var cc: pg.rep ? pg.rep.coachConclusion : null
                        SectionTitle { width: parent.width; title: "Coach Conclusion" }
                        Text { text: "Rating: "+(parent.cc?parent.cc.rating:"—"); color: pg.cAccent; font.bold: true; font.pixelSize: 13; font.family: pg.fam }
                        Text { width: parent.width; text: parent.cc?parent.cc.overallAssessment:""; color: pg.cInk; font.pixelSize: 12; font.family: pg.fam; wrapMode: Text.WordWrap }
                        Text { width: parent.width; visible: parent.cc&&parent.cc.trainingFocusSummary; text: parent.cc?parent.cc.trainingFocusSummary:""; color: pg.cSub; font.pixelSize: 12; font.family: pg.fam; wrapMode: Text.WordWrap }
                    }

                    // Coach Diary (editable)
                    Column {
                        width: parent.width; spacing: 8
                        SectionTitle { width: parent.width; title: "Coach Diary" }
                        Grid {
                            id: diaryGrid
                            width: parent.width; columns: 2; columnSpacing: 16; rowSpacing: 8
                            property real fw: (width - 16) / 2
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Equipment changes"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfEquip; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Sight changes"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfSight; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Ammunition / pellets"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfAmmo; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Weather"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfWeather; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Wind direction"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfWind; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Lighting"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfLight; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Physical state"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfPhys; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                            Column { width: diaryGrid.fw; spacing: 2
                                Text { text: "Mental state"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                                TextField { id: dfMental; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                    background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } } }
                        }
                        Column { width: parent.width; spacing: 2
                            Text { text: "Coach notes"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                            TextArea { id: dfCoach; width: parent.width; height: 46; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } }
                        }
                        Column { width: parent.width; spacing: 2
                            Text { text: "Athlete notes"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                            TextArea { id: dfAthlete; width: parent.width; height: 46; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } }
                        }
                        Text { text: "Next training focus"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                        Flow {
                            width: parent.width; spacing: 6
                            Repeater {
                                model: (pg.diaryVer, pg.focusOptions.slice())   // rebuilds on reload
                                delegate: Row {
                                    spacing: 4
                                    CheckBox {
                                        checked: pg.focusChecked[modelData] === true
                                        onToggled: { var fc=pg.focusChecked; fc[modelData]=checked; pg.focusChecked=fc }
                                    }
                                    Text { text: modelData; color: pg.cInk; font.pixelSize: 11; font.family: pg.fam; anchors.verticalCenter: parent.verticalCenter }
                                }
                            }
                        }
                        Column { width: parent.width; spacing: 2
                            Text { text: "Free-text next-session focus"; color: pg.cSub; font.pixelSize: 11; font.family: pg.fam }
                            TextField { id: dfNext; width: parent.width; font.pixelSize: 12; font.family: pg.fam; color: pg.cInk
                                background: Rectangle { color: "#fbfbf9"; border.color: pg.cRule; border.width: 1; radius: 3 } }
                        }
                    }

                    ReportFooter {
                        width: parent.width
                        softwareVersion: "Seta 4.0"
                        generatedText: "Generated " + Qt.formatDateTime(new Date(),"ddd yyyy-MM-dd HH:mm")
                    }

                    Item { width: 1; height: 6 }
                }

                Text {
                    anchors.centerIn: parent
                    visible: !(pg.rep && pg.rep.valid)
                    text: "No report to print yet."
                    color: pg.cSub; font.pixelSize: 15; font.family: pg.fam
                }
            }
        }
    }
}
