import QtQuick 2.15

// Wind Map — completed-session ANALYSIS REVIEW (Stage 6.1).
//
// WindMapAnalyticsEngine is the only calculation authority. This file reads
// WINDMAP.analysisModel() and FORMATS it. It computes no metric: no mean, no
// spread, no shift, no threshold decision. If a value is not in the model it
// is not shown — a withheld metric arrives with its key ABSENT, so nothing
// here can print a zero that looks like a measurement.
//
// It also never instructs. There is no aim arrow, no correction, no hold-off:
// the MPI shift is drawn and labelled as an OBSERVED GROUP-CENTRE SHIFT.
Item {
    id: view
    property var ctl: null                   // WINDMAP
    signal exportPdfRequested()
    signal newSessionRequested()
    signal homeRequested()

    // The model, re-read whenever the view is shown.
    property var model: ({})
    property int positionIndex: 0
    property string conditionFilter: ""       // "" = all
    property bool showSighters: false

    function refresh() { view.model = ctl ? ctl.analysisModel() : ({}) }
    onVisibleChanged: if (visible) refresh()

    readonly property var session:  model.session  ? model.session  : ({})
    readonly property var summary:  model.summary  ? model.summary  : ({})
    readonly property var positions: model.positions ? model.positions : []
    readonly property var pos: positionIndex < positions.length ? positions[positionIndex] : ({})
    readonly property bool threeP: session.threePositions === true

    readonly property color _card:   "#1B1E24"
    readonly property color _panel:  "#0E1014"
    readonly property color _line:   "#2A2E36"
    readonly property color _red:    "#C40046"
    readonly property color _redHi:  "#E8004F"
    readonly property color _green:  "#20C997"
    readonly property color _amber:  "#E0A800"
    readonly property color _txt:    "#F3F6FA"
    readonly property color _txtSec: "#B6BCC6"
    readonly property color _txtMut: "#6F7A86"

    // A stable colour per condition label, so the plot, the legend and the
    // tables agree. Calm and No reading get fixed, meaningful colours.
    function conditionColour(label) {
        if (label === "Calm") return "#4EA8DE"
        if (label === "No reading") return view._amber
        var h = 0
        for (var i = 0; i < label.length; ++i) h = (h * 31 + label.charCodeAt(i)) % 360
        return Qt.hsla(h / 360, 0.62, 0.58, 1.0)
    }
    function mm(v, d) { return v === undefined ? "—" : Number(v).toFixed(d === undefined ? 1 : d) }
    // A metric is shown ONLY when the model says its sample supports it.
    function metric(g, key, flag, d) {
        return g[flag] === true ? mm(g[key], d) : "—"
    }

    Rectangle { anchors.fill: parent; color: "#EA0F1116" }
    MouseArea { anchors.fill: parent }        // swallow clicks behind the panel

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(1180, parent.width - 40)
        height: Math.min(860, parent.height - 30)
        color: view._card; radius: 12; border.color: view._line; border.width: 1

        // ── header + 3P tabs ────────────────────────────────────────────
        Column {
            id: head
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 22; spacing: 8

            Text { text: qsTr("WIND MAP — SESSION ANALYSIS")
                   color: view._txt; font.family: theme.fontFamily
                   font.pixelSize: 24; font.bold: true }
            Text { text: (view.session.disciplineName || "") + " · " + (view.session.athlete || "")
                         + " · " + (view.session.operatingMode || "")
                   color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 12 }
            Text { text: qsTr("NOT AN OFFICIAL COMPETITION RESULT")
                   color: view._amber; font.family: theme.fontFamily
                   font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.5 }

            // 8. 3P TABS — Kneeling · Prone · Standing · Overview.
            Row {
                spacing: 6; visible: view.threeP
                Repeater {
                    model: {
                        var t = []
                        for (var i = 0; i < view.positions.length; ++i)
                            t.push({ label: view.positions[i].positionName, idx: i })
                        t.push({ label: qsTr("Overview"), idx: -1 })
                        return t
                    }
                    delegate: Rectangle {
                        width: tabT.implicitWidth + 26; height: 32; radius: 16
                        color: view.positionIndex === modelData.idx ? view._red : view._panel
                        border.color: view.positionIndex === modelData.idx ? view._red : view._line
                        border.width: 1
                        Text { id: tabT; anchors.centerIn: parent; text: modelData.label
                               color: view.positionIndex === modelData.idx ? "white" : view._txtSec
                               font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: view.positionIndex = modelData.idx }
                    }
                }
            }
        }

        Flickable {
            id: body
            anchors.top: head.bottom; anchors.topMargin: 12
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: actions.top
            anchors.leftMargin: 22; anchors.rightMargin: 22; anchors.bottomMargin: 10
            clip: true; contentWidth: width; contentHeight: col.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

            Column {
                id: col
                width: body.width; spacing: 18; bottomPadding: 16

                // ── 1. SESSION OVERVIEW ─────────────────────────────────
                SectionHead { text: qsTr("1 · SESSION OVERVIEW") }
                Grid {
                    width: parent.width; columns: 4; rowSpacing: 10; columnSpacing: 10
                    Repeater {
                        model: [
                            { k: qsTr("COUNTED SHOTS"),     v: "" + (view.summary.countedShots || 0) },
                            { k: qsTr("SIGHTERS"),          v: "" + (view.summary.sighterShots || 0) },
                            { k: qsTr("UNIQUE CONDITIONS"), v: "" + (view.summary.uniqueConditions || 0) },
                            { k: qsTr("CONDITION ENTRIES"), v: "" + (view.summary.conditionEntries || 0) },
                            { k: qsTr("COUNTED · READING"), v: "" + (view.summary.countedWithReading || 0) },
                            { k: qsTr("COUNTED · CALM"),    v: "" + (view.summary.countedCalm || 0) },
                            { k: qsTr("COUNTED · NO READ"), v: "" + (view.summary.countedNoReading || 0) },
                            { k: qsTr("POSITIONS"),         v: (view.session.positionsRepresented || []).join(" · ") || "—" }
                        ]
                        delegate: Rectangle {
                            width: (col.width - 30) / 4; height: 70; radius: 8
                            color: view._panel; border.color: view._line; border.width: 1
                            Column {
                                anchors.centerIn: parent; spacing: 3; width: parent.width - 16
                                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                                       text: modelData.v; color: view._txt; elide: Text.ElideRight
                                       font.family: "Consolas"; font.pixelSize: 18; font.bold: true }
                                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                                       text: modelData.k; color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 9
                                       font.bold: true; font.letterSpacing: 1 }
                            }
                        }
                    }
                }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: qsTr("Data quality: ") + (view.summary.dataQuality || "—")
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }

                // ── 2. CONDITION-COLOURED TARGET PLOT ───────────────────
                SectionHead { text: qsTr("2 · CONDITION-COLOURED TARGET PLOT"); visible: view.positionIndex >= 0 }
                Row {
                    width: parent.width; spacing: 16; visible: view.positionIndex >= 0
                    Rectangle {
                        id: plotBox
                        width: parent.width * 0.52; height: width
                        color: view._panel; radius: 8; border.color: view._line; border.width: 1

                        readonly property var rows: view.model.shotRows ? view.model.shotRows : []
                        readonly property real span: {
                            var mx = 8
                            for (var i = 0; i < rows.length; ++i) {
                                if (rows[i].position !== view.pos.position) continue
                                if (rows[i].sighter && !view.showSighters) continue
                                mx = Math.max(mx, Math.abs(rows[i].xMm), Math.abs(rows[i].yMm))
                            }
                            return mx * 1.3
                        }
                        readonly property real rad: Math.min(width, height) / 2 - 14
                        function px(v) { return width / 2 + (v / span) * rad }
                        function py(v) { return height / 2 - (v / span) * rad }

                        // target centre
                        Rectangle { width: 11; height: 1; color: view._txtMut
                                    x: plotBox.width/2 - 5; y: plotBox.height/2 }
                        Rectangle { width: 1; height: 11; color: view._txtMut
                                    x: plotBox.width/2; y: plotBox.height/2 - 5 }

                        // mean-radius circle for the selected condition, when valid
                        Repeater {
                            model: view.pos.byExactCondition ? view.pos.byExactCondition : []
                            delegate: Rectangle {
                                visible: modelData.hasDispersion === true && modelData.hasMpi === true
                                         && (view.conditionFilter === "" || view.conditionFilter === modelData.label)
                                width: 2 * (modelData.meanRadiusMm / plotBox.span) * plotBox.rad
                                height: width; radius: width / 2
                                color: "transparent"; border.width: 1
                                border.color: view.conditionColour(modelData.label)
                                opacity: 0.45
                                x: plotBox.px(modelData.mpiXMm) - width / 2
                                y: plotBox.py(modelData.mpiYMm) - height / 2
                            }
                        }
                        // shots
                        Repeater {
                            model: plotBox.rows
                            delegate: Rectangle {
                                visible: modelData.position === view.pos.position
                                         && (!modelData.sighter || view.showSighters)
                                         && (view.conditionFilter === ""
                                             || view.conditionFilter === modelData.conditionLabel)
                                width: modelData.sighter ? 6 : 8; height: width; radius: width / 2
                                color: modelData.sighter ? "transparent"
                                                         : view.conditionColour(modelData.conditionLabel)
                                border.color: view.conditionColour(modelData.conditionLabel)
                                border.width: modelData.sighter ? 1 : 0
                                x: plotBox.px(modelData.xMm) - width / 2
                                y: plotBox.py(modelData.yMm) - height / 2
                            }
                        }
                        // group MPI markers
                        Repeater {
                            model: view.pos.byExactCondition ? view.pos.byExactCondition : []
                            delegate: Rectangle {
                                visible: modelData.hasMpi === true
                                         && (view.conditionFilter === "" || view.conditionFilter === modelData.label)
                                width: 12; height: 12; radius: 2; rotation: 45
                                color: "transparent"; border.width: 2
                                border.color: view.conditionColour(modelData.label)
                                x: plotBox.px(modelData.mpiXMm) - 6
                                y: plotBox.py(modelData.mpiYMm) - 6
                            }
                        }
                        // reference centre
                        Rectangle {
                            visible: view.pos.reference && view.pos.reference.valid === true
                            width: 16; height: 16; radius: 8
                            color: "transparent"; border.color: "white"; border.width: 2
                            x: plotBox.px(view.pos.reference ? view.pos.reference.xMm : 0) - 8
                            y: plotBox.py(view.pos.reference ? view.pos.reference.yMm : 0) - 8
                        }
                    }

                    Column {
                        width: parent.width * 0.44; spacing: 8
                        Text { text: qsTr("LEGEND"); color: view._txtMut
                               font.family: theme.fontFamily; font.pixelSize: 9
                               font.bold: true; font.letterSpacing: 1 }
                        Repeater {
                            model: view.pos.byExactCondition ? view.pos.byExactCondition : []
                            delegate: Row {
                                spacing: 8; width: parent.width
                                Rectangle { width: 12; height: 12; radius: 6
                                            color: view.conditionColour(modelData.label)
                                            anchors.verticalCenter: parent.verticalCenter }
                                Text { text: modelData.label + "   n=" + modelData.n
                                       color: view.conditionFilter === modelData.label ? view._txt : view._txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 12
                                       font.bold: view.conditionFilter === modelData.label
                                       anchors.verticalCenter: parent.verticalCenter }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: view.conditionFilter =
                                        (view.conditionFilter === modelData.label) ? "" : modelData.label
                                }
                            }
                        }
                        Row {
                            spacing: 8
                            Rectangle { width: 14; height: 14; radius: 7; color: "transparent"
                                        border.color: "white"; border.width: 2
                                        anchors.verticalCenter: parent.verticalCenter }
                            Text { text: qsTr("Reference centre: ") + (view.pos.reference ? view.pos.reference.label : "—")
                                         + (view.pos.reference && view.pos.reference.valid ? "  n=" + view.pos.reference.n : "")
                                   color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                        // filters
                        Row {
                            spacing: 6
                            Rectangle {
                                width: sgT.implicitWidth + 24; height: 32; radius: 16
                                color: view.showSighters ? view._red : view._panel
                                border.color: view._line; border.width: 1
                                Text { id: sgT; anchors.centerIn: parent; text: qsTr("Show sighters")
                                       color: view.showSighters ? "white" : view._txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: view.showSighters = !view.showSighters }
                            }
                            Rectangle {
                                visible: view.conditionFilter !== ""
                                width: clrT.implicitWidth + 24; height: 32; radius: 16
                                color: view._panel; border.color: view._line; border.width: 1
                                Text { id: clrT; anchors.centerIn: parent; text: qsTr("Clear filter")
                                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: view.conditionFilter = "" }
                            }
                        }
                        Text { width: parent.width; wrapMode: Text.WordWrap
                               text: qsTr("Sighters are drawn hollow and are excluded from every counted statistic.")
                               color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                    }
                }

                // ── 3. CONDITION COMPARISON TABLE ───────────────────────
                SectionHead { text: qsTr("3 · CONDITION COMPARISON"); visible: view.positionIndex >= 0 }
                MetricTable {
                    visible: view.positionIndex >= 0
                    width: parent.width
                    rows: view.pos.byExactCondition ? view.pos.byExactCondition : []
                    shifts: view.pos.shifts ? view.pos.shifts : []
                }

                // ── 4. MPI SHIFT VIEW ───────────────────────────────────
                SectionHead { text: qsTr("4 · OBSERVED GROUP-CENTRE SHIFT"); visible: view.positionIndex >= 0 }
                Column {
                    width: parent.width; spacing: 6; visible: view.positionIndex >= 0
                    Text { width: parent.width; wrapMode: Text.WordWrap
                           text: qsTr("Measured from the %1. These describe where each group's centre sat. They are observations, not instructions.")
                                    .arg(view.pos.reference ? view.pos.reference.label : "reference")
                           color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                    Repeater {
                        model: view.pos.shifts ? view.pos.shifts : []
                        delegate: Rectangle {
                            width: parent.width; height: 46; radius: 6
                            color: view._panel; border.color: view._line; border.width: 1
                            Row {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                spacing: 10
                                Rectangle { width: 10; height: 10; radius: 5
                                            color: view.conditionColour(modelData.label)
                                            anchors.verticalCenter: parent.verticalCenter }
                                Text { width: parent.width * 0.24; text: modelData.label
                                       color: view._txt; font.family: theme.fontFamily; font.pixelSize: 12
                                       elide: Text.ElideRight
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { width: parent.width * 0.10; text: "n=" + modelData.n
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 12
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { width: parent.width * 0.42
                                       text: modelData.valid
                                             ? (view.mm(modelData.magnitudeMm) + " mm · " + modelData.directionWords)
                                             : qsTr("Withheld — %1 more shots needed").arg(modelData.shotsNeeded)
                                       color: modelData.valid ? view._txt : view._amber
                                       font.family: theme.fontFamily; font.pixelSize: 12
                                       elide: Text.ElideRight
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { text: modelData.evidence; color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 10
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
                    }
                }

                // ── 5. DIRECTION-SECTOR VIEW ────────────────────────────
                SectionHead { text: qsTr("5 · DIRECTION SECTORS"); visible: view.positionIndex >= 0 }
                Grid {
                    width: parent.width; columns: 5; rowSpacing: 8; columnSpacing: 8
                    visible: view.positionIndex >= 0
                    Repeater {
                        // All eight sectors plus Calm and No reading, so an
                        // EMPTY sector reads "No data" and never a zeroed stat.
                        model: {
                            var order = ["Calm","N","NE","E","SE","S","SW","W","NW","No reading"]
                            var have = view.pos.byDirection ? view.pos.byDirection : []
                            var out = []
                            for (var i = 0; i < order.length; ++i) {
                                var f = null
                                for (var j = 0; j < have.length; ++j)
                                    if (have[j].label === order[i]) f = have[j]
                                out.push(f ? f : { label: order[i], n: 0, empty: true })
                            }
                            return out
                        }
                        delegate: Rectangle {
                            width: (col.width - 32) / 5; height: 96; radius: 8
                            color: view._panel
                            border.color: modelData.empty ? view._line : view.conditionColour(modelData.label)
                            border.width: 1
                            opacity: modelData.empty ? 0.45 : 1.0
                            Column {
                                anchors.left: parent.left; anchors.leftMargin: 10
                                anchors.right: parent.right; anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter; spacing: 2
                                Text { text: modelData.label; color: view._txt
                                       font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true }
                                Text { visible: modelData.empty === true; text: qsTr("No data")
                                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10 }
                                Text { visible: !modelData.empty
                                       text: "n=" + modelData.n + "   " + view.metric(modelData, "meanScore", "hasMeanScore", 2)
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                                Text { visible: !modelData.empty
                                       text: qsTr("MPI ") + view.metric(modelData, "mpiXMm", "hasMpi")
                                             + " / " + view.metric(modelData, "mpiYMm", "hasMpi")
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 10 }
                                Text { visible: !modelData.empty
                                       text: qsTr("R ") + view.metric(modelData, "meanRadiusMm", "hasDispersion")
                                             + qsTr("  H ") + view.metric(modelData, "horizontalSpreadMm", "hasDispersion")
                                             + qsTr("  V ") + view.metric(modelData, "verticalSpreadMm", "hasDispersion")
                                       color: view._txtMut; font.family: "Consolas"; font.pixelSize: 10 }
                                Text { visible: !modelData.empty; text: modelData.evidence || ""
                                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 9 }
                            }
                        }
                    }
                }

                // ── 6. SPEED-BAND VIEW ──────────────────────────────────
                SectionHead { text: qsTr("6 · SPEED BANDS"); visible: view.positionIndex >= 0 }
                MetricTable {
                    visible: view.positionIndex >= 0
                    width: parent.width
                    rows: {
                        var order = ["Calm","0-2.0 m/s","2.0-4.0 m/s","4.0-7.0 m/s","over 7.0 m/s","No reading"]
                        var have = view.pos.bySpeedBand ? view.pos.bySpeedBand : []
                        var out = []
                        for (var i = 0; i < order.length; ++i)
                            for (var j = 0; j < have.length; ++j)
                                if (have[j].label === order[i]) out.push(have[j])
                        return out
                    }
                    shifts: []
                }

                // ── 7. TIMELINE ─────────────────────────────────────────
                SectionHead { text: qsTr("7 · TIMELINE") }
                Column {
                    width: parent.width; spacing: 0
                    Row {
                        width: parent.width; spacing: 8; height: 22
                        Text { width: parent.width * 0.07; text: qsTr("#");        color: view._txtMut; font.pixelSize: 10 }
                        Text { width: parent.width * 0.11; text: qsTr("TYPE");     color: view._txtMut; font.pixelSize: 10 }
                        Text { width: parent.width * 0.13; text: qsTr("POSITION"); color: view._txtMut; font.pixelSize: 10; visible: view.threeP }
                        Text { width: parent.width * 0.09; text: qsTr("SCORE");    color: view._txtMut; font.pixelSize: 10 }
                        Text { width: parent.width * 0.16; text: qsTr("X / Y mm"); color: view._txtMut; font.pixelSize: 10 }
                        Text { text: qsTr("CONDITION"); color: view._txtMut; font.pixelSize: 10 }
                    }
                    Repeater {
                        model: view.model.timeline ? view.model.timeline : []
                        delegate: Column {
                            width: parent.width
                            // boundary markers
                            Rectangle {
                                visible: modelData.phaseChangedBefore === true
                                width: parent.width; height: 20; color: "transparent"
                                Text { anchors.verticalCenter: parent.verticalCenter
                                       text: qsTr("— counted shots begin —"); color: view._green
                                       font.family: theme.fontFamily; font.pixelSize: 10; font.bold: true }
                            }
                            Rectangle {
                                visible: modelData.conditionChangedBefore === true
                                width: parent.width; height: 18; color: "transparent"
                                Text { anchors.verticalCenter: parent.verticalCenter
                                       text: qsTr("condition changed → ") + modelData.conditionLabel
                                       color: view._txtMut; font.family: theme.fontFamily
                                       font.pixelSize: 10; font.italic: true }
                            }
                            Row {
                                width: parent.width; spacing: 8; height: 22
                                Text { width: parent.width * 0.07; text: modelData.shotId
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 11 }
                                Text { width: parent.width * 0.11; text: modelData.type
                                       color: modelData.sighter ? view._txtMut : view._txt
                                       font.family: theme.fontFamily; font.pixelSize: 11 }
                                Text { width: parent.width * 0.13; text: modelData.positionName
                                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                                       visible: view.threeP }
                                Text { width: parent.width * 0.09; text: view.mm(modelData.score)
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 11 }
                                Text { width: parent.width * 0.16
                                       text: view.mm(modelData.xMm) + " / " + view.mm(modelData.yMm)
                                       color: view._txtSec; font.family: "Consolas"; font.pixelSize: 11 }
                                Row {
                                    spacing: 6
                                    Rectangle { width: 8; height: 8; radius: 4
                                                color: view.conditionColour(modelData.conditionLabel)
                                                anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: modelData.conditionLabel; color: view._txtSec
                                           font.family: theme.fontFamily; font.pixelSize: 11
                                           anchors.verticalCenter: parent.verticalCenter }
                                }
                            }
                        }
                    }
                }

                // ── 9. WHAT THE DATA SUGGESTS ───────────────────────────
                SectionHead { text: qsTr("9 · WHAT THE DATA SUGGESTS") }
                Column {
                    width: parent.width; spacing: 8
                    Repeater {
                        model: view.model.findings ? view.model.findings : []
                        delegate: Rectangle {
                            width: parent.width; height: fCol.implicitHeight + 20; radius: 8
                            color: view._panel; border.color: view._line; border.width: 1
                            Column {
                                id: fCol
                                anchors.left: parent.left; anchors.leftMargin: 12
                                anchors.right: parent.right; anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter; spacing: 4
                                Row {
                                    spacing: 8
                                    Text { text: modelData.category.toUpperCase(); color: view._red
                                           font.family: theme.fontFamily; font.pixelSize: 9
                                           font.bold: true; font.letterSpacing: 1 }
                                    Text { text: modelData.n > 0 ? ("n=" + modelData.n) : ""
                                           color: view._txtMut; font.family: "Consolas"; font.pixelSize: 10 }
                                }
                                Text { width: parent.width; wrapMode: Text.WordWrap
                                       text: modelData.text; color: view._txt
                                       font.family: theme.fontFamily; font.pixelSize: 12 }
                                Text { width: parent.width; wrapMode: Text.WordWrap
                                       visible: modelData.suggestion !== ""
                                       text: qsTr("Next session: ") + modelData.suggestion
                                       color: view._green; font.family: theme.fontFamily; font.pixelSize: 11 }
                            }
                        }
                    }
                }

                SectionHead { text: qsTr("LIMITATIONS") }
                Column {
                    width: parent.width; spacing: 4
                    Repeater {
                        model: view.model.limitations ? view.model.limitations : []
                        delegate: Text { width: parent.width; wrapMode: Text.WordWrap
                                         text: "· " + modelData; color: view._txtMut
                                         font.family: theme.fontFamily; font.pixelSize: 10; font.italic: true }
                    }
                }
            }
        }

        // ── 10. ACTIONS ─────────────────────────────────────────────────
        Row {
            id: actions
            anchors.bottom: parent.bottom; anchors.right: parent.right
            anchors.margins: 22; spacing: 10; height: 52

            Rectangle {
                width: 120; height: 52; radius: 8
                color: "transparent"; border.color: view._line; border.width: 1
                Text { anchors.centerIn: parent; text: qsTr("Home"); color: view._txtSec
                       font.family: theme.fontFamily; font.pixelSize: 13 }
                MouseArea { anchors.fill: parent; onClicked: view.homeRequested() }
            }
            Rectangle {
                width: 180; height: 52; radius: 8
                color: "transparent"; border.color: view._line; border.width: 1
                Text { anchors.centerIn: parent; text: qsTr("New Wind Map session")
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 13 }
                MouseArea { anchors.fill: parent; onClicked: view.newSessionRequested() }
            }
            Rectangle {
                width: 150; height: 52; radius: 8
                color: pdfMouse.pressed ? view._red : view._redHi
                Text { anchors.centerIn: parent; text: qsTr("Export PDF"); color: "white"
                       font.family: theme.fontFamily; font.pixelSize: 14; font.bold: true }
                MouseArea { id: pdfMouse; anchors.fill: parent; onClicked: view.exportPdfRequested() }
            }
        }
    }

    // ── small shared pieces ─────────────────────────────────────────────
    component SectionHead: Text {
        color: "#B6BCC6"; font.family: theme.fontFamily
        font.pixelSize: 11; font.bold: true; font.letterSpacing: 2; topPadding: 6
    }

    // One table shape for the condition comparison AND the speed bands, so a
    // metric cannot be formatted two different ways in two places.
    component MetricTable: Column {
        property var rows: []
        property var shifts: []
        spacing: 0

        function shiftFor(label) {
            for (var i = 0; i < shifts.length; ++i)
                if (shifts[i].label === label) return shifts[i]
            return null
        }

        Row {
            width: parent.width; spacing: 6; height: 24
            Text { width: parent.width * 0.19; text: qsTr("CONDITION"); color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.05; text: qsTr("n");         color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.08; text: qsTr("SCORE");     color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.13; text: qsTr("MPI X/Y");   color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.11; text: qsTr("SHIFT");     color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.08; text: qsTr("MEAN R");    color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.08; text: qsTr("GROUP Ø");   color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.07; text: qsTr("H");         color: "#6F7A86"; font.pixelSize: 10 }
            Text { width: parent.width * 0.07; text: qsTr("V");         color: "#6F7A86"; font.pixelSize: 10 }
            Text { text: qsTr("EVIDENCE"); color: "#6F7A86"; font.pixelSize: 10 }
        }
        Repeater {
            model: parent.rows
            delegate: Row {
                width: parent.width; spacing: 6; height: 24
                readonly property var sh: parent.parent ? null : null
                Row {
                    width: parent.width * 0.19; spacing: 6
                    Rectangle { width: 8; height: 8; radius: 4
                                color: view.conditionColour(modelData.label)
                                anchors.verticalCenter: parent.verticalCenter }
                    Text { text: modelData.label; color: "#F3F6FA"; elide: Text.ElideRight
                           width: parent.width - 14
                           font.family: theme.fontFamily; font.pixelSize: 11
                           anchors.verticalCenter: parent.verticalCenter }
                }
                Text { width: parent.width * 0.05; text: modelData.n
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { width: parent.width * 0.08; text: view.metric(modelData, "meanScore", "hasMeanScore", 2)
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { width: parent.width * 0.13
                       text: view.metric(modelData, "mpiXMm", "hasMpi") + " / " + view.metric(modelData, "mpiYMm", "hasMpi")
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text {
                    width: parent.width * 0.11
                    color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11
                    text: {
                        var s = null
                        for (var i = 0; i < shifts.length; ++i)
                            if (shifts[i].label === modelData.label) s = shifts[i]
                        if (!s) return "—"
                        return s.valid ? view.mm(s.magnitudeMm) : "—"
                    }
                }
                Text { width: parent.width * 0.08; text: view.metric(modelData, "meanRadiusMm", "hasDispersion")
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { width: parent.width * 0.08; text: view.metric(modelData, "groupDiameterMm", "hasDispersion")
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { width: parent.width * 0.07; text: view.metric(modelData, "horizontalSpreadMm", "hasDispersion")
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { width: parent.width * 0.07; text: view.metric(modelData, "verticalSpreadMm", "hasDispersion")
                       color: "#B6BCC6"; font.family: "Consolas"; font.pixelSize: 11 }
                Text { text: modelData.evidence || ""; color: "#6F7A86"
                       font.family: theme.fontFamily; font.pixelSize: 10 }
            }
        }
    }
}
