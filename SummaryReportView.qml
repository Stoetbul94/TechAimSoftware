import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

// Embeddable Match Summary report view (hosted inside a FloatingWindow).
// Was a Dialog; now a plain Item so the FloatingWindow supplies the chrome
// (title bar, footer actions). The A4 print page + print_region grab and every
// value are unchanged.
Item {
    id:screenPresence

    // Pure content view — no window logic. The host FloatingWindow owns the
    // chrome (title bar, tabs, scrolling) and the actions. This view only
    // exposes its natural height and intent signals, plus exportPdf() which
    // owns the print_region grab. Every report value is unchanged.
    signal requestClose()
    signal requestExportPdf()
    signal requestPrint()

    implicitHeight: 1163      // A4 page (1123) + 40 top/bottom margin; window scrolls it

    property int fontSize: 12
    property int tick: 0   // bumped in update() to refresh MPI/Group metric cards

    Connections {
        target: MODREADER
        onShootCountChanged: {
            // ignore on sighter mode
            if (shootingPage.sligterMode)
                return;

            update()
        }
    }

    Rectangle {
        id:contentRect
        anchors.fill: parent
        color: "#dcdad3"                     // grey backdrop; the A4 page sits on top

        // A4 print page, centred. The host FloatingWindow owns the scrolling
        // (scrollableContent), so this view has NO Flickable of its own.
        Rectangle {
                    id: print_region
                    width: 794           // A4 width @ 96 dpi
                    height: 1123         // A4 height @ 96 dpi
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 20
                    color: "white"
                    border.color: "#e6e8ec"
                    border.width: 1

                    Column {
                        id: coverCol
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 34
                        spacing: 12

                        ReportHeader {
                            width: parent.width
                            reportTitle: "Match Summary"
                            athlete: userName
                            discipline: eventName
                            dateText: new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd")
                            timeText: new Date().toLocaleString(Qt.locale(""), "hh:mm:ss")
                        }

                        SectionTitle { width: parent.width; title: "Executive Summary" }

                        Grid {
                            id: metricGrid
                            width: parent.width
                            columns: 3
                            columnSpacing: 12
                            rowSpacing: 12
                            property real cw: (width - 2 * columnSpacing) / 3

                            MetricCard { width: metricGrid.cw; label: "Total Score"; value: "" + totalScore; unit: "(" + totalScoreWithoutDecimal + ")" }
                            MetricCard { width: metricGrid.cw; label: "Average Shot"; value: globalMatchModel.count > 0 ? (totalScore / globalMatchModel.count).toFixed(2) : "—" }
                            MetricCard { width: metricGrid.cw; label: "Total Shots"; value: "" + globalMatchModel.count }
                            MetricCard { width: metricGrid.cw; label: "Inner 10"; value: "" + rightPanel.totalStars }
                            MetricCard { width: metricGrid.cw; label: "MPI"; unit: "mm"; valueSize: 18
                                value: (screenPresence.tick, MODREADER.getXMPI().toFixed(1) + " / " + MODREADER.getYMPI().toFixed(1)) }
                            MetricCard { width: metricGrid.cw; label: "Group"; unit: "mm²"; valueSize: 20
                                value: (screenPresence.tick, "" + MODREADER.getGroup(-1).toFixed(1)) }
                            MetricCard { width: metricGrid.cw; label: "Teiler"; valueSize: 20
                                value: (screenPresence.tick, MODREADER.getTeiler(-1).toFixed(1)) }
                            MetricCard { width: metricGrid.cw; label: "Total Time"; unit: "min"; valueSize: 20
                                value: converSecondToMins(totalTime) }
                            MetricCard { width: metricGrid.cw; label: "Avg Time / Shot"; unit: "min"; valueSize: 20
                                value: globalMatchModel.count > 0 ? converSecondToMins(totalTime / globalMatchModel.count) : "—" }
                        }

                        SectionTitle { width: parent.width; title: "Overall Target" }

                        // Overall target — existing visualisation, coordinate math kept
                        // verbatim, reframed in a clean white card.
                        Rectangle {
                            width: parent.width
                            height: shootingPage.is3PMatch ? 200 : 300   // shorter for 3P to fit the position breakdown
                            radius: 12
                            color: "white"
                            border.color: "#e6e8ec"
                            border.width: 1

                            Image {
                                id: shootingcanvas
                                source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                                                                            : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                                                    : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                                                            : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                                width: Math.min(parent.width * 0.82, parent.height * 0.9)
                                height: width
                                opacity: 1
                                anchors.centerIn: parent

                                Rectangle {
                                    id: shootingMianRect
                                    color: "transparent"
                                    anchors.fill: parent
                                }

                                Repeater
                                {
                                    id:numberRepeater
                                    model:globalMatchModel
                                    delegate: numberDelegate
                                }

                                Component {
                                    id:numberDelegate
                                    Item {
                                        id:mainItem
                                        // 34.55 and 10.11 was given by abins (tachus)
                                        //10 Meter Pistol Ratio=155.5÷4.5=34.55
                                        //10 Meter Rifle Ratio=45.5÷4.5=10.11
                                        //50 Meter Pistol Ratio=500/5.6=89.29
                                        //50 Meter Rifle Ratio=154.4/5.6=27.57

                                        property double gameRatio: gameRange == 10 ? (centerPanel.gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                                                   : (centerPanel.gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())

                                        width: shootingcanvas.height/gameRatio

                                        height: width
                                        Rectangle
                                        {
                                            Component.onCompleted:
                                            {
                                                // Use each shot's own stored target-face mm (aligned to the
                                                // full match record), so 3P shots from every position plot
                                                // correctly - not just the current position. Same source and
                                                // formula the 3P report uses.
                                                var xCor = globalMatchModel.get(index).xmm * 1
                                                var yCor = globalMatchModel.get(index).ymm * 1

                                                var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                                                var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                                                var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                                                var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                                                var offsetX = shootingMianRect.width/shootingWidth
                                                var offsetY = shootingMianRect.height/shootingHeight

                                                var centerX = shootingMianRect.width/2 //* offset
                                                var centerY = shootingMianRect.height/2 //* offset

                                                var bulletSize = 0//4.5/2

                                                mainItem.x = shootingMianRect.x + centerX+((xCor+bulletSize)*offsetX) - radius
                                                mainItem.y = shootingMianRect.y + centerY-((yCor+bulletSize)*offsetY) - radius
                                            }
                                            anchors.fill: parent
                                            radius:parent.width/2
                                            color: greenColor//shootingPage.isPalletRed ? "red" : "white"
                                            Text{
                                                anchors.centerIn: parent
                                                text: index+1
                                                visible: false
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ── Position breakdown (3P only; auto-hidden otherwise) ──
                        SectionTitle {
                            width: parent.width; title: "Position Breakdown"
                            visible: shootingPage.is3PMatch
                        }
                        Row {
                            width: parent.width; spacing: 12
                            visible: shootingPage.is3PMatch
                            Repeater {
                                model: shootingPage.is3PMatch
                                       ? [{ n: "Kneeling", p: 0 }, { n: "Prone", p: 1 }, { n: "Standing", p: 2 }]
                                       : []
                                delegate: Rectangle {
                                    width: (parent.width - 24) / 3
                                    height: 72
                                    radius: 12
                                    color: "white"; border.color: "#e6e8ec"; border.width: 1
                                    Rectangle { width: 4; radius: 2; height: parent.height * 0.5; color: "#a80038"
                                        anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter }
                                    Column {
                                        anchors.left: parent.left; anchors.leftMargin: 26
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 3
                                        Text { text: modelData.n; color: "#6b7280"; font.family: "Segoe UI"; font.pixelSize: 12; font.letterSpacing: 0.4 }
                                        Text { text: (screenPresence.tick, getPositionTotal(modelData.p)); color: "#191b1f"; font.family: "Segoe UI"; font.pixelSize: 24; font.bold: true }
                                        Text {
                                            // Depend on `tick` (like the score above) so the count/stars
                                            // re-evaluate as shots land — the delegate is built before any
                                            // shots exist, so without this it stays stuck at "0 shots".
                                            text: (screenPresence.tick, getPositionCount(modelData.p) + " shots · ★ " + getPositionInner(modelData.p))
                                            color: "#8a8f98"; font.family: "Segoe UI"; font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                        }

                        SectionTitle { width: parent.width; title: "Series Breakdown" }

                        // Per-series scores across the full match (10 shots per
                        // series; every series/position included).
                        Column {
                            width: parent.width
                            spacing: 0

                            Rectangle {
                                width: parent.width; height: 26; color: "#f1f3f5"
                                Row {
                                    anchors.fill: parent
                                    Text { width: parent.width*0.34; height: parent.height; text: qsTr("Series");   leftPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.33; height: parent.height; text: qsTr("Score");    horizontalAlignment: Text.AlignRight; rightPadding: 12; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI" }
                                    Text { width: parent.width*0.33; height: parent.height; text: qsTr("Inner 10"); horizontalAlignment: Text.AlignRight; rightPadding: 12; verticalAlignment: Text.AlignVCenter; color: "#5b6270"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI" }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: "#a80038" }
                            }
                            Repeater {
                                model: Math.ceil(globalMatchModel.count / 10)
                                delegate: Rectangle {
                                    width: parent.width; height: 26
                                    color: index % 2 ? "#f7f8fa" : "#ffffff"
                                    Row {
                                        anchors.fill: parent
                                        Text { width: parent.width*0.34; height: parent.height; text: qsTr("Series ") + (index+1); leftPadding: 10; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 12; font.family: "Segoe UI" }
                                        Text { width: parent.width*0.33; height: parent.height; text: getSeriesScoreVal(index+1); horizontalAlignment: Text.AlignRight; rightPadding: 12; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 12; font.bold: true; font.family: "Segoe UI" }
                                        Text { width: parent.width*0.33; height: parent.height; text: "" + getSeriesInnerVal(index+1); horizontalAlignment: Text.AlignRight; rightPadding: 12; verticalAlignment: Text.AlignVCenter; color: "#191b1f"; font.pixelSize: 12; font.family: "Segoe UI" }
                                    }
                                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
                                }
                            }
                        }
                    }

                    ReportFooter {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 34
                        anchors.rightMargin: 34
                        anchors.bottomMargin: 20
                        softwareVersion: "Seta 4.0"
                        generatedText: "Generated " + new Date().toLocaleString(Qt.locale(""), "ddd yyyy-MM-dd hh:mm")
                    }
                }

    }

    function updateModel()
    {
        numberRepeater.model = null
        numberRepeater.model =globalMatchModel
    }

//    function printImage()
//    {
//        var stat = print_region.grabToImage(function(result) {
//            CUSTOMPRINT.print(result.image); //result.image holds the QVariant
//        });
//    }

    function getSeriesTotalText()
    {
        var formatText = ""
        if(globalMatchModel.count === 0)
            return formatText
        var seriesScore = 0;
        for(var i=0; i<globalMatchModel.count; i++)
        {
            var scoreatIndex =globalMatchModel.get(i).calculatedscore*1
            seriesScore = seriesScore*1  +  (scoreatIndex.toFixed(1))*1
            if( ( (i+1) % 10 == 0) && (i>0) )
            {
                var seriesId = Math.floor((i+1)/10)
                var seriesText = "Series " + seriesId*1
                seriesText += "(" + (seriesScore*1).toFixed(1) +") , "
                formatText = formatText + seriesText
                seriesScore = 0
            }
            if( (i === (globalMatchModel.count-1)) && ( (i+1)%10 != 0) )
            {
                var seriesIdNum = Math.floor((i+1)/10)
                var seriesScoreText = "Series " + seriesIdNum*1
                seriesScoreText += "(" + (seriesScore*1).toFixed(1) +")"
                formatText = formatText + seriesScoreText
                seriesScore = 0
            }
        }
        return formatText
    }

    function getMPI()
    {
        return (":  X: " + getXMPI()+"; Y: "+getYMPI())
    }

    function getXMPI()
    {
        console.log("-----------------*******************__________________")
        return MODREADER.getXMPI()
    }
    function getYMPI()
    {
        console.log("-----------------*******************__________________1")
        return MODREADER.getYMPI()
    }
    function converSecondToMins(seconds)
    {
        //        var minutes = Math.floor(seconds / 60);
        //        var secd = seconds % 60;
        //        var result_seconds = Math.ceil(secd);

        //        return minutes+":"+result_seconds

        return (seconds/60).toFixed(2)
    }

    // Per-series (10-shot) score across the full match record.
    function getSeriesScoreVal(s)
    {
        if (globalMatchModel.count === 0) return "0.0"
        var sum = 0
        for (var i = (s-1)*10; i < globalMatchModel.count && i < s*10; ++i)
            sum += globalMatchModel.get(i).calculatedscore * 1
        return sum.toFixed(1)
    }

    // Per-series inner-10 count, same rule the app uses for totalStars
    // (pistol >= 10.4, rifle >= 10.2).
    function getSeriesInnerVal(s)
    {
        if (globalMatchModel.count === 0) return 0
        var cnt = 0
        for (var i = (s-1)*10; i < globalMatchModel.count && i < s*10; ++i) {
            var v = globalMatchModel.get(i).calculatedscore * 1
            if ((gameMode === 0 && v >= rightPanel.star_limit_value_pistol)
                    || (gameMode === 1 && v >= rightPanel.star_limit_value_rifle))
                ++cnt
        }
        return cnt
    }

    // 3P position subtotals from the full match record (position role:
    // 0 = kneeling, 1 = prone, 2 = standing).
    function getPositionTotal(p)
    {
        var s = 0
        for (var i = 0; i < globalMatchModel.count; ++i)
            if (globalMatchModel.get(i).position === p)
                s = s * 1 + (globalMatchModel.get(i).calculatedscore * 1).toFixed(1) * 1
        return s.toFixed(1)
    }
    function getPositionCount(p)
    {
        var c = 0
        for (var i = 0; i < globalMatchModel.count; ++i)
            if (globalMatchModel.get(i).position === p) ++c
        return c
    }
    function getPositionInner(p)
    {
        var c = 0
        for (var i = 0; i < globalMatchModel.count; ++i) {
            if (globalMatchModel.get(i).position !== p) continue
            var v = globalMatchModel.get(i).calculatedscore * 1
            if ((gameMode === 0 && v >= rightPanel.star_limit_value_pistol)
                    || (gameMode === 1 && v >= rightPanel.star_limit_value_rifle))
                ++c
        }
        return c
    }

    function update()
    {
        // MPI / Group are read via functions (not notifying properties), so the
        // metric cards can't auto-bind to them; bump tick to re-evaluate.
        screenPresence.tick++
    }

    // Owns the grab: same print_region.grabToImage -> createSummryPdf path as
    // before. The host window's Save PDF action calls this.
    function exportPdf()
    {
        CUSTOMPRINT.clearImagesList()
        var stat = print_region.grabToImage(function(result) {
            CUSTOMPRINT.addImage(result.image);
        }, Qt.size(8917/4, 13033/4)); //2229, 3258
        CUSTOMPRINT.setServerPath(APPSETTINGS.getPrintPDFFilePath());
        CUSTOMPRINT.createSummryPdf();
    }
}
