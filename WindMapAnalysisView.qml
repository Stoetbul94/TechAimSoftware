import QtQuick 2.15
import QtQuick.Controls 2.15

// Wind Map — completed-session ANALYSIS (Stage 6.1.1 redesign).
//
// UI-WIND-003..008. The Stage 6.1 version was technically complete and
// unreadable: five section pills over a second identical-looking position row,
// eight equal statistic tiles, an abstract plot, raw MPI/spread numbers, and
// every section instantiated at once.
//
// THREE questions this screen must answer, in this order:
//   1. What happened?
//   2. How strong is the evidence?
//   3. What should I test next session?
//
// WindMapAnalyticsEngine remains the ONLY calculation authority. This file
// reads WINDMAP.analysisModel() — cached in C++, ~0.001 ms per fetch — and
// formats it. It computes no metric and no threshold.
//
// PERFORMANCE. Each page is behind a Loader, so opening Summary does not build
// the condition cards, the target graphic or a 48-row table. Shot Details uses
// a ListView, which creates only the delegates on screen.
Item {
    id: view
    property var ctl: null                   // WINDMAP
    signal newSessionRequested()
    signal homeRequested()

    // ── development-only instrumentation ─────────────────────────────────
    // Gated on config.ini [App_Settings] developer_mode=1, which is OFF in
    // production. It logs to stderr and NEVER renders anything: an operator
    // must not see engineering timings on a training screen.
    readonly property bool devTiming: (typeof APPSETTINGS !== "undefined")
                                      && APPSETTINGS.getDeveloperMode()
    property var _t0: 0
    property var _pageT: 0
    property var _seen: ({})
    onPageChanged: {
        // Recorded AFTER the mark so the first open reads as first.
        Qt.callLater(function () { view._seen[view.page] = true })
    }
    onPositionIndexChanged: view._mark("position filter switch", Date.now())
    function _mark(what, since) {
        if (!view.devTiming) return
        console.log("WINDMAP-PERF | " + what + " | "
                    + (Date.now() - since).toFixed(0) + " ms")
    }

    // ── the model, fetched once ──────────────────────────────────────────
    property var model: ({})
    property bool ready: false
    function refresh() {
        var t = Date.now()
        view.ready = false
        view.model = ctl ? ctl.analysisModel() : ({})
        view.ready = (view.model && view.model.session !== undefined)
        view._mark("model fetch (cached read)", t)
        view._mark("completion -> analysis ready", view._t0)
    }
    onVisibleChanged: {
        if (!visible) return
        view._t0 = Date.now()
        prepareTimer.restart()
    }
    // A single deferred tick so the loading state can paint before the model is
    // fetched. The fetch itself is a cached read; this exists so the athlete
    // never sees an unexplained blank frame.
    Timer { id: prepareTimer; interval: 16; onTriggered: view.refresh() }

    // ── navigation: THREE pages, one position filter ─────────────────────
    property string page: "summary"           // summary | compare | details
    property int positionIndex: -1            // -1 = Session Overview

    readonly property var session:   model.session   ? model.session   : ({})
    readonly property var summary:   model.summary   ? model.summary   : ({})
    readonly property var positions: model.positions ? model.positions : []
    readonly property bool threeP: session.threePositions === true
    // The position currently filtered to, or the whole session.
    readonly property var pos: (positionIndex >= 0 && positionIndex < positions.length)
                               ? positions[positionIndex]
                               : (positions.length > 0 ? positions[0] : ({}))
    readonly property bool sessionScope: positionIndex < 0
    readonly property string scopeName: view.sessionScope
        ? qsTr("Session Overview")
        : (view.pos.positionName ? view.pos.positionName : "")

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

    // Condition colours live ONLY inside data visualisations. They are chosen
    // in the blue/violet/teal range so a condition can never be mistaken for
    // an error or warning state.
    function conditionColour(label) {
        if (label === "Calm") return "#4EA8DE"
        if (label === "No reading") return "#8E9AAF"
        var h = 0
        for (var i = 0; i < label.length; ++i) h = (h * 37 + label.charCodeAt(i)) % 360
        return Qt.hsla(((h % 200) + 160) / 360, 0.55, 0.62, 1.0)
    }
    function mm(v, d) { return v === undefined ? "—" : Number(v).toFixed(d === undefined ? 1 : d) }
    function metric(g, key, flag, d) { return g && g[flag] === true ? mm(g[key], d) : "—" }

    // right / left and high / low, so nobody has to read a sign.
    function acrossWords(dx) {
        if (dx === undefined) return "—"
        var v = Math.abs(dx).toFixed(1)
        if (Math.abs(dx) < 0.05) return qsTr("centred left to right")
        return v + " mm " + (dx >= 0 ? qsTr("right") : qsTr("left"))
    }
    function upWords(dy) {
        if (dy === undefined) return "—"
        var v = Math.abs(dy).toFixed(1)
        if (Math.abs(dy) < 0.05) return qsTr("centred high to low")
        return v + " mm " + (dy >= 0 ? qsTr("high") : qsTr("low"))
    }
    function centreWords(g) {
        if (!g || g.hasMpi !== true) return qsTr("Not enough shots to report a centre")
        return view.acrossWords(g.mpiXMm) + ", " + view.upWords(g.mpiYMm)
    }
    function plural(n, one, many) { return n === 1 ? one : many }
    function sampleNote(g) {
        if (!g || g.n === undefined) return ""
        if (g.hasDispersion === true) return ""
        if (g.hasMpi !== true) {
            var nm = g.shotsNeededForMpi
            return qsTr("%1 %2 recorded. %3 more %4 required before a group centre can be reported.")
                     .arg(g.n).arg(view.plural(g.n, qsTr("shot"), qsTr("shots")))
                     .arg(nm).arg(view.plural(nm, qsTr("shot is"), qsTr("shots are")))
        }
        var nd = g.shotsNeededForDispersion
        return qsTr("%1 shots recorded. %2 more %3 required for a group comparison.")
                 .arg(g.n).arg(nd).arg(view.plural(nd, qsTr("shot is"), qsTr("shots are")))
    }

    // ── UI-WIND-006: VERDICTS, scoped to what is selected ────────────────
    // Stage 6.1.3: the view renders WindMapVerdict records. It composes no
    // verdict text of its own — every sentence below comes from the engine.
    //
    // A session-level comparison is NEVER presented as a position result. When
    // a position is selected only that position's verdicts show; the session
    // comparison appears on Session Overview, explicitly labelled.
    function verdictsForScope() {
        var all = view.model.verdicts ? view.model.verdicts : []
        var out = []
        for (var i = 0; i < all.length; ++i) {
            var v = all[i]
            if (view.sessionScope) { out.push(v); continue }
            if (v.scopeIsSession === true) continue          // not this position's
            if (v.position !== undefined && v.position !== view.pos.position) continue
            out.push(v)
        }
        return out
    }
    // The engine already returns verdicts in priority order, so the primary is
    // simply the first one in scope.
    function primaryVerdict() {
        var v = view.verdictsForScope()
        return v.length > 0 ? v[0] : null
    }
    function secondaryVerdicts() {
        var v = view.verdictsForScope()
        return v.length > 1 ? v.slice(1) : []
    }
    // The verdict attached to one condition card, if the engine raised one.
    function verdictForCondition(label) {
        var v = view.verdictsForScope()
        for (var i = 0; i < v.length; ++i)
            if (v[i].comparedCondition === label) return v[i]
        return null
    }

    // The group the visual and headline describe: the best-evidenced one.
    function leadGroup() {
        var g = view.pos.byExactCondition ? view.pos.byExactCondition : []
        return g.length > 0 ? g[0] : null
    }

    Rectangle { anchors.fill: parent; color: "#EA0F1116" }
    MouseArea { anchors.fill: parent }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(1180, parent.width - 40)
        height: Math.min(870, parent.height - 30)
        color: view._card; radius: 12; border.color: view._line; border.width: 1

        // ── header ──────────────────────────────────────────────────────
        Column {
            id: head
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 20; spacing: 10

            Row {
                width: parent.width; spacing: 12
                Column {
                    width: parent.width - 220; spacing: 2
                    Text { text: qsTr("WIND MAP — ANALYSIS"); color: view._txt
                           font.family: theme.fontFamily; font.pixelSize: 22; font.bold: true }
                    Text { text: (view.session.disciplineName || "") + "   ·   "
                                 + (view.session.athlete || "") + "   ·   "
                                 + (view.session.operatingMode || "")
                           color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 11 }
                }
                Text { text: qsTr("NOT AN OFFICIAL\nCOMPETITION RESULT")
                       color: view._amber; horizontalAlignment: Text.AlignRight
                       font.family: theme.fontFamily; font.pixelSize: 9
                       font.bold: true; font.letterSpacing: 1 }
            }

            // THREE primary pages. Tech Aim red marks the selected page — the
            // only red in the navigation, so there is one primary selection.
            Row {
                spacing: 8
                Repeater {
                    model: [ { l: qsTr("SUMMARY"), k: "summary" },
                             { l: qsTr("COMPARE CONDITIONS"), k: "compare" },
                             { l: qsTr("SHOT DETAILS"), k: "details" } ]
                    delegate: Rectangle {
                        width: pgT.implicitWidth + 30; height: 36; radius: 6
                        color: view.page === modelData.k ? view._red : "transparent"
                        border.color: view.page === modelData.k ? view._red : view._line
                        border.width: 1
                        Text { id: pgT; anchors.centerIn: parent; text: modelData.l
                               color: view.page === modelData.k ? "white" : view._txtSec
                               font.family: theme.fontFamily; font.pixelSize: 12
                               font.bold: true; font.letterSpacing: 1 }
                        MouseArea { anchors.fill: parent; onClicked: view.page = modelData.k }
                    }
                }
            }

            // ONE position filter, visibly a filter — a label plus a restrained
            // segmented control, never a second row of primary-looking pills.
            Row {
                spacing: 10; visible: view.threeP
                Text { text: qsTr("Position:"); color: view._txtMut
                       font.family: theme.fontFamily; font.pixelSize: 11
                       anchors.verticalCenter: parent.verticalCenter }
                Rectangle {
                    height: 28; radius: 4; color: view._panel
                    border.color: view._line; border.width: 1
                    width: segRow.width + 2
                    anchors.verticalCenter: parent.verticalCenter
                    Row {
                        id: segRow
                        anchors.centerIn: parent; spacing: 0
                        Repeater {
                            model: {
                                var t = [{ label: qsTr("Session Overview"), idx: -1 }]
                                for (var i = 0; i < view.positions.length; ++i)
                                    t.push({ label: view.positions[i].positionName, idx: i })
                                return t
                            }
                            delegate: Rectangle {
                                width: segT.implicitWidth + 22; height: 26
                                color: view.positionIndex === modelData.idx ? "#2A2E36" : "transparent"
                                Text { id: segT; anchors.centerIn: parent; text: modelData.label
                                       color: view.positionIndex === modelData.idx
                                              ? view._txt : view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 11
                                       font.bold: view.positionIndex === modelData.idx }
                                MouseArea { anchors.fill: parent
                                            onClicked: view.positionIndex = modelData.idx }
                            }
                        }
                    }
                }
                Text { text: qsTr("filters the page below")
                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                       font.italic: true; anchors.verticalCenter: parent.verticalCenter }
            }
        }

        // ── UI-WIND-003: an explained loading state, never a blank page ──
        Column {
            anchors.centerIn: parent
            spacing: 12; visible: !view.ready
            Text { anchors.horizontalCenter: parent.horizontalCenter
                   text: qsTr("Preparing your Wind Map analysis…")
                   color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 14 }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 220; height: 4; radius: 2; color: view._panel
                Rectangle {
                    width: parent.width * 0.4; height: parent.height; radius: 2
                    color: view._red
                    SequentialAnimation on x {
                        running: !view.ready; loops: Animation.Infinite
                        NumberAnimation { from: 0; to: 132; duration: 700; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 132; to: 0; duration: 700; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }

        // ── the pages, each LAZY ─────────────────────────────────────────
        Loader {
            id: pageLoader
            anchors.top: head.bottom; anchors.topMargin: 10
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: actions.top
            anchors.leftMargin: 20; anchors.rightMargin: 20; anchors.bottomMargin: 10
            visible: view.ready
            active: view.ready
            sourceComponent: view.page === "summary" ? summaryPage
                           : view.page === "compare" ? comparePage : detailsPage
            onSourceComponentChanged: view._pageT = Date.now()
            onLoaded: view._mark("page open: " + view.page
                                 + (view._seen[view.page] ? " (repeat)" : " (first)"),
                                 view._pageT)
            Component.onCompleted: view._pageT = Date.now()
        }

        // ── actions ──────────────────────────────────────────────────────
        Row {
            id: actions
            anchors.bottom: parent.bottom; anchors.right: parent.right
            anchors.margins: 20; spacing: 10; height: 48

            Rectangle {
                width: 110; height: 48; radius: 6
                color: "transparent"; border.color: view._line; border.width: 1
                Text { anchors.centerIn: parent; text: qsTr("Home"); color: view._txtSec
                       font.family: theme.fontFamily; font.pixelSize: 13 }
                MouseArea { anchors.fill: parent; onClicked: view.homeRequested() }
            }
            Rectangle {
                width: 190; height: 48; radius: 6
                color: "transparent"; border.color: view._line; border.width: 1
                Text { anchors.centerIn: parent; text: qsTr("New Wind Map session")
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 13 }
                MouseArea { anchors.fill: parent; onClicked: view.newSessionRequested() }
            }
            // UI-WIND-008: the PDF is Stage 6.2. A disabled, subdued control
            // that says so — not a primary-styled button opening a placeholder.
            Rectangle {
                width: 170; height: 48; radius: 6
                color: "transparent"; border.color: view._line; border.width: 1
                opacity: 0.45
                Text { anchors.centerIn: parent; text: qsTr("PDF — COMING NEXT")
                       color: view._txtMut; font.family: theme.fontFamily
                       font.pixelSize: 12; font.bold: true }
                // No MouseArea: it is not actionable, so it does not respond.
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════════
    // PAGE 1 — SUMMARY
    // ══════════════════════════════════════════════════════════════════════
    Component {
        id: summaryPage
        Flickable {
            contentWidth: width; contentHeight: sumCol.implicitHeight
            clip: true; boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                id: sumCol
                width: parent.width; spacing: 14; bottomPadding: 14

                property var lead: view.leadGroup()
                property var find: view.primaryVerdict()

                // 1 · WHAT HAPPENED
                Rectangle {
                    width: parent.width; height: whCol.implicitHeight + 26; radius: 8
                    color: view._panel; border.color: view._red; border.width: 1
                    Column {
                        id: whCol
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter; spacing: 6
                        Row {
                            spacing: 10
                            Text { text: qsTr("WHAT HAPPENED"); color: view._red
                                   font.family: theme.fontFamily; font.pixelSize: 10
                                   font.bold: true; font.letterSpacing: 2 }
                            // UI-WIND-006: the scope is always visible.
                            Rectangle {
                                visible: sumCol.find !== null
                                width: scT.implicitWidth + 14; height: 16; radius: 8
                                color: "transparent"
                                border.color: (sumCol.find && sumCol.find.scopeIsSession)
                                              ? view._amber : view._green
                                border.width: 1
                                Text { id: scT; anchors.centerIn: parent
                                       text: sumCol.find ? sumCol.find.scopeLabel : ""
                                       color: (sumCol.find && sumCol.find.scopeIsSession)
                                              ? view._amber : view._green
                                       font.pixelSize: 8; font.bold: true; font.letterSpacing: 1 }
                            }
                        }
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: sumCol.find ? sumCol.find.headline
                                              : qsTr("No counted shots were recorded in this selection.")
                            color: view._txt; font.family: theme.fontFamily
                            font.pixelSize: 16
                        }
                        Text {
                            visible: sumCol.find && sumCol.find.observedPattern !== ""
                            width: parent.width; wrapMode: Text.WordWrap
                            text: sumCol.find ? sumCol.find.observedPattern : ""
                            color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 12
                        }
                        // EVIDENCE, with the plain-language explanation of the
                        // level — these are product states, not confidence
                        // intervals, and the explanation says so.
                        Row {
                            spacing: 8; visible: sumCol.find !== null
                            Rectangle {
                                width: evT2.implicitWidth + 16; height: 18; radius: 9
                                color: "transparent"; border.width: 1
                                border.color: (sumCol.find && sumCol.find.evidence === "Comparative")
                                              ? view._green : view._amber
                                Text { id: evT2; anchors.centerIn: parent
                                       text: sumCol.find ? sumCol.find.evidence.toUpperCase() : ""
                                       color: (sumCol.find && sumCol.find.evidence === "Comparative")
                                              ? view._green : view._amber
                                       font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: sumCol.find && sumCol.find.sampleCountCompared > 0
                                      ? qsTr("%1 shots").arg(sumCol.find.sampleCountCompared) : ""
                                color: view._txtMut; font.family: "Consolas"; font.pixelSize: 11
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Text {
                            visible: sumCol.find !== null
                            width: parent.width; wrapMode: Text.WordWrap
                            text: sumCol.find ? sumCol.find.evidenceExplanation : ""
                            color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 11
                        }
                    }
                }

                // 2 · WHAT THIS MEANS
                Column {
                    width: parent.width; spacing: 4
                    Text { text: qsTr("WHAT THIS MEANS"); color: view._txtMut
                           font.family: theme.fontFamily; font.pixelSize: 10
                           font.bold: true; font.letterSpacing: 2 }
                    Text {
                        width: parent.width; wrapMode: Text.WordWrap
                        // Straight from the verdict — the view words nothing.
                        text: sumCol.find ? sumCol.find.interpretation
                                          : qsTr("Record some counted shots to see an analysis.")
                        color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 13
                    }
                }

                // 3 · NEXT TRAINING STEP
                Rectangle {
                    width: parent.width; height: ntCol.implicitHeight + 22; radius: 8
                    color: "#0d2018"; border.color: view._green; border.width: 1
                    Column {
                        id: ntCol
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter; spacing: 4
                        Text { text: qsTr("NEXT TRAINING STEP"); color: view._green
                               font.family: theme.fontFamily; font.pixelSize: 10
                               font.bold: true; font.letterSpacing: 2 }
                        Text { width: parent.width; wrapMode: Text.WordWrap
                               text: sumCol.find && sumCol.find.nextTrainingStep !== ""
                                     ? sumCol.find.nextTrainingStep
                                     : qsTr("Record every condition you observe, and repeat a condition "
                                          + "often enough to compare it with itself.")
                               color: view._txt; font.family: theme.fontFamily; font.pixelSize: 13 }
                    }
                }

                // COACH DECISION — only where the verdict raises one.
                Rectangle {
                    visible: sumCol.find !== null && sumCol.find.coachDecision !== ""
                    width: parent.width; height: cdCol.implicitHeight + 22; radius: 8
                    color: view._panel; border.color: view._amber; border.width: 1
                    Column {
                        id: cdCol
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter; spacing: 4
                        Text { text: qsTr("COACH DECISION"); color: view._amber
                               font.family: theme.fontFamily; font.pixelSize: 10
                               font.bold: true; font.letterSpacing: 2 }
                        Text { width: parent.width; wrapMode: Text.WordWrap
                               text: sumCol.find ? sumCol.find.coachDecision : ""
                               color: view._txt; font.family: theme.fontFamily; font.pixelSize: 13 }
                    }
                }

                // 4 · EVIDENCE — no more than four prominent values
                Text { text: qsTr("EVIDENCE"); color: view._txtMut
                       font.family: theme.fontFamily; font.pixelSize: 10
                       font.bold: true; font.letterSpacing: 2; topPadding: 4 }
                Row {
                    width: parent.width; spacing: 10
                    Repeater {
                        model: [
                            { k: qsTr("COUNTED SHOTS"), v: "" + (view.summary.countedShots || 0) },
                            { k: qsTr("CONDITIONS USED"), v: "" + (view.summary.uniqueConditions || 0) },
                            { k: qsTr("POSITIONS"),
                              v: "" + ((view.session.positionsRepresented || []).length || 1) },
                            { k: qsTr("DATA QUALITY"), v: (view.summary.countedNoReading || 0) === 0
                                                          ? qsTr("Complete") : qsTr("Partial") }
                        ]
                        delegate: Rectangle {
                            width: (sumCol.width - 30) / 4; height: 74; radius: 8
                            color: view._panel; border.color: view._line; border.width: 1
                            Column {
                                anchors.centerIn: parent; spacing: 4; width: parent.width - 16
                                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                                       text: modelData.v; color: view._txt; elide: Text.ElideRight
                                       font.family: "Consolas"; font.pixelSize: 20; font.bold: true }
                                Text { width: parent.width; horizontalAlignment: Text.AlignHCenter
                                       text: modelData.k; color: view._txtMut; elide: Text.ElideRight
                                       font.family: theme.fontFamily; font.pixelSize: 9
                                       font.bold: true; font.letterSpacing: 1 }
                            }
                        }
                    }
                }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: qsTr("Data quality: ") + (view.summary.dataQuality || "—")
                             + (sumCol.find && sumCol.find.shotsNeeded > 0
                                ? qsTr("  ·  %1 more shots would allow a comparison.")
                                     .arg(sumCol.find.shotsNeeded) : "")
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }

                // expandable session details — the values that used to compete
                // with the headline as equal tiles
                Rectangle {
                    width: parent.width; radius: 6; color: "transparent"
                    border.color: view._line; border.width: 1
                    height: detCol.height + 16
                    property bool open: false
                    Column {
                        id: detCol
                        anchors.left: parent.left; anchors.leftMargin: 12
                        anchors.right: parent.right; anchors.rightMargin: 12
                        anchors.top: parent.top; anchors.topMargin: 8
                        spacing: 6
                        Text {
                            text: (parent.parent.open ? "▾  " : "▸  ") + qsTr("SESSION DETAILS")
                            color: view._txtSec; font.family: theme.fontFamily
                            font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
                        }
                        Grid {
                            visible: detCol.parent.open
                            columns: 3; rowSpacing: 4; columnSpacing: 20
                            Repeater {
                                model: [
                                    { k: qsTr("Sighters"), v: "" + (view.summary.sighterShots || 0) },
                                    { k: qsTr("Condition entries"), v: "" + (view.summary.conditionEntries || 0) },
                                    { k: qsTr("Shots with a reading"), v: "" + (view.summary.countedWithReading || 0) },
                                    { k: qsTr("Calm shots"), v: "" + (view.summary.countedCalm || 0) },
                                    { k: qsTr("No-reading shots"), v: "" + (view.summary.countedNoReading || 0) },
                                    { k: qsTr("Positions recorded"),
                                      v: (view.session.positionsRepresented || []).join(", ") || "—" }
                                ]
                                delegate: Text {
                                    text: modelData.k + ":  " + modelData.v
                                    color: view._txtSec; font.family: theme.fontFamily
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                    MouseArea { anchors.fill: parent; anchors.bottomMargin: parent.height - 28
                                onClicked: parent.open = !parent.open }
                }

                // 5 · VISUAL RESULT
                Text { text: qsTr("VISUAL RESULT") + "   ·   " + view.scopeName
                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                       font.bold: true; font.letterSpacing: 2; topPadding: 6 }
                Row {
                    width: parent.width; spacing: 16

                    Loader {
                        // Built only when the Summary page is open.
                        width: sumCol.width * 0.46; height: width
                        active: true
                        sourceComponent: Component {
                            WindMapTargetPlot {
                                shots: {
                                    var all = view.model.shotRows ? view.model.shotRows : []
                                    if (view.sessionScope) return all
                                    var out = []
                                    for (var i = 0; i < all.length; ++i)
                                        if (all[i].position === view.pos.position) out.push(all[i])
                                    return out
                                }
                                groups: view.pos.byExactCondition ? view.pos.byExactCondition : []
                                reference: view.pos.reference ? view.pos.reference : null
                                colourFor: view.conditionColour
                            }
                        }
                    }

                    Column {
                        width: sumCol.width * 0.5; spacing: 8

                        // OBSERVED RESULT card
                        Rectangle {
                            width: parent.width; height: orCol.implicitHeight + 22; radius: 8
                            color: view._panel; border.color: view._line; border.width: 1
                            Column {
                                id: orCol
                                anchors.left: parent.left; anchors.leftMargin: 14
                                anchors.right: parent.right; anchors.rightMargin: 14
                                anchors.verticalCenter: parent.verticalCenter; spacing: 5
                                Text { text: qsTr("OBSERVED RESULT"); color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 9
                                       font.bold: true; font.letterSpacing: 1 }
                                Row {
                                    spacing: 8
                                    Rectangle { width: 10; height: 10; radius: 5
                                                visible: sumCol.lead !== null
                                                color: sumCol.lead
                                                       ? view.conditionColour(sumCol.lead.label) : "transparent"
                                                anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: sumCol.lead ? sumCol.lead.label : qsTr("No condition recorded")
                                           color: view._txt; font.family: theme.fontFamily
                                           font.pixelSize: 15; font.bold: true
                                           anchors.verticalCenter: parent.verticalCenter }
                                }
                                Text { text: sumCol.lead
                                             ? qsTr("%1 counted shots").arg(sumCol.lead.n) : ""
                                       color: view._txtSec; font.family: theme.fontFamily
                                       font.pixelSize: 12 }
                                Text { text: qsTr("Group centre:"); color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 10; topPadding: 2 }
                                Text { width: parent.width; wrapMode: Text.WordWrap
                                       text: view.centreWords(sumCol.lead)
                                       color: view._txt; font.family: theme.fontFamily; font.pixelSize: 13 }
                                Text { visible: sumCol.lead && sumCol.lead.hasDispersion === true
                                       text: qsTr("Group size (widest two shots):"); color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 10; topPadding: 2 }
                                Text { visible: sumCol.lead && sumCol.lead.hasDispersion === true
                                       text: view.metric(sumCol.lead, "groupDiameterMm", "hasDispersion") + " mm"
                                       color: view._txt; font.family: theme.fontFamily; font.pixelSize: 13 }
                                Text { visible: view.sampleNote(sumCol.lead) !== ""
                                       width: parent.width; wrapMode: Text.WordWrap
                                       text: view.sampleNote(sumCol.lead)
                                       color: view._amber; font.family: theme.fontFamily
                                       font.pixelSize: 11; topPadding: 2 }
                                Text { text: qsTr("Evidence: ") + (sumCol.lead ? sumCol.lead.evidence : "—")
                                       color: view._txtSec; font.family: theme.fontFamily
                                       font.pixelSize: 11; topPadding: 2 }
                            }
                        }

                        // LEGEND — every marker style explained
                        Rectangle {
                            width: parent.width; height: lgCol.implicitHeight + 20; radius: 8
                            color: "transparent"; border.color: view._line; border.width: 1
                            Column {
                                id: lgCol
                                anchors.left: parent.left; anchors.leftMargin: 14
                                anchors.right: parent.right; anchors.rightMargin: 14
                                anchors.verticalCenter: parent.verticalCenter; spacing: 4
                                Text { text: qsTr("LEGEND"); color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 9
                                       font.bold: true; font.letterSpacing: 1 }
                                Repeater {
                                    model: view.pos.byExactCondition ? view.pos.byExactCondition : []
                                    delegate: Row {
                                        spacing: 8
                                        Rectangle { width: 9; height: 9; radius: 5
                                                    color: view.conditionColour(modelData.label)
                                                    anchors.verticalCenter: parent.verticalCenter }
                                        Text { text: qsTr("Counted shot — %1  (n=%2)")
                                                       .arg(modelData.label).arg(modelData.n)
                                               color: view._txtSec; font.family: theme.fontFamily
                                               font.pixelSize: 10
                                               anchors.verticalCenter: parent.verticalCenter }
                                    }
                                }
                                Row { spacing: 8
                                    Rectangle { width: 9; height: 9; radius: 5; color: "transparent"
                                                border.color: view._txtSec; border.width: 2
                                                anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: qsTr("Hollow ring — sighter (not counted)")
                                           color: view._txtSec; font.family: theme.fontFamily
                                           font.pixelSize: 10
                                           anchors.verticalCenter: parent.verticalCenter } }
                                Row { spacing: 8
                                    Item { width: 9; height: 9
                                           anchors.verticalCenter: parent.verticalCenter
                                           Rectangle { width: 9; height: 2; y: 4; color: view._txtSec }
                                           Rectangle { width: 2; height: 9; x: 4; color: view._txtSec } }
                                    Text { text: qsTr("Cross — that condition's group centre")
                                           color: view._txtSec; font.family: theme.fontFamily
                                           font.pixelSize: 10
                                           anchors.verticalCenter: parent.verticalCenter } }
                                Row { spacing: 8
                                    Rectangle { width: 11; height: 11; radius: 6; color: "transparent"
                                                border.color: "white"; border.width: 2
                                                anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: qsTr("White circle — reference centre (%1)")
                                                   .arg(view.pos.reference ? view.pos.reference.label : "—")
                                           color: view._txtSec; font.family: theme.fontFamily
                                           font.pixelSize: 10
                                           anchors.verticalCenter: parent.verticalCenter } }
                                Row { spacing: 8
                                    Rectangle { width: 11; height: 11; radius: 6; color: "transparent"
                                                border.color: view._txtMut; border.width: 1
                                                opacity: 0.6
                                                anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: qsTr("Faint circle — average distance from the group centre")
                                           color: view._txtSec; font.family: theme.fontFamily
                                           font.pixelSize: 10
                                           anchors.verticalCenter: parent.verticalCenter } }
                                Text { text: qsTr("Thin rings and the centre cross are target reference only.")
                                       color: view._txtMut; font.family: theme.fontFamily
                                       font.pixelSize: 9; font.italic: true }
                            }
                        }
                    }
                }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: qsTr("The plot shows where your shots landed and where each condition's "
                                + "group centre sat. It is a record, not an aiming instruction.")
                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                       font.italic: true }

                // limitations
                Column {
                    width: parent.width; spacing: 3; topPadding: 6
                    Repeater {
                        model: view.model.limitations ? view.model.limitations : []
                        delegate: Text { width: parent.width; wrapMode: Text.WordWrap
                                         text: "· " + modelData; color: view._txtMut
                                         font.family: theme.fontFamily; font.pixelSize: 10 }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════════
    // PAGE 2 — COMPARE CONDITIONS
    // ══════════════════════════════════════════════════════════════════════
    Component {
        id: comparePage
        Flickable {
            contentWidth: width; contentHeight: cmpCol.implicitHeight
            clip: true; boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                id: cmpCol
                width: parent.width; spacing: 10; bottomPadding: 14

                Text { text: qsTr("CONDITIONS RECORDED") + "   ·   " + view.scopeName
                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                       font.bold: true; font.letterSpacing: 2 }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: qsTr("Each card describes one condition you recorded. Comparisons are "
                                + "measured from the %1.")
                                .arg(view.pos.reference ? view.pos.reference.label : qsTr("reference"))
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }

                Repeater {
                    model: view.pos.byExactCondition ? view.pos.byExactCondition : []
                    delegate: Rectangle {
                        width: cmpCol.width; radius: 8
                        color: view._panel; border.color: view._line; border.width: 1
                        height: cardCol.implicitHeight + 24
                        property bool techOpen: false
                        property var shift: {
                            var sh = view.pos.shifts ? view.pos.shifts : []
                            for (var i = 0; i < sh.length; ++i)
                                if (sh[i].label === modelData.label) return sh[i]
                            return null
                        }
                        Column {
                            id: cardCol
                            anchors.left: parent.left; anchors.leftMargin: 16
                            anchors.right: parent.right; anchors.rightMargin: 16
                            anchors.top: parent.top; anchors.topMargin: 12
                            spacing: 5

                            Row {
                                spacing: 10
                                Rectangle { width: 12; height: 12; radius: 6
                                            color: view.conditionColour(modelData.label)
                                            anchors.verticalCenter: parent.verticalCenter }
                                Text { text: modelData.label; color: view._txt
                                       font.family: theme.fontFamily; font.pixelSize: 16; font.bold: true
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { text: qsTr("%1 shots").arg(modelData.n); color: view._txtSec
                                       font.family: theme.fontFamily; font.pixelSize: 12
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                            // The athlete-relative description, shown BESIDE the
                            // recorded compass value and only when a firing
                            // direction was recorded.
                            Text {
                                visible: modelData.hasRelativeWind === true
                                text: modelData.relativeWind; color: view._txtSec
                                font.family: theme.fontFamily; font.pixelSize: 12; font.italic: true
                            }
                            Grid {
                                columns: 2; rowSpacing: 3; columnSpacing: 28
                                Text { text: qsTr("Average score:  ") + view.metric(modelData, "meanScore", "hasMeanScore", 2)
                                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 12 }
                                Text { text: qsTr("Group size:  ")
                                             + (modelData.hasDispersion === true
                                                ? view.mm(modelData.groupDiameterMm) + " mm" : "—")
                                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 12 }
                            }
                            Text { width: parent.width; wrapMode: Text.WordWrap
                                   text: qsTr("Group centre:  ") + view.centreWords(modelData)
                                   color: view._txt; font.family: theme.fontFamily; font.pixelSize: 12 }
                            Text { width: parent.width; wrapMode: Text.WordWrap
                                   text: {
                                       var s = parent.parent.shift
                                       if (!s) return qsTr("Comparison:  this is the reference condition.")
                                       if (s.valid !== true)
                                           return qsTr("Comparison:  not enough shots — %1 more required.")
                                                    .arg(s.shotsNeeded)
                                       return qsTr("Comparison:  centre sat %1 from the reference (%2, %3)")
                                                .arg(view.mm(s.magnitudeMm) + " mm")
                                                .arg(view.acrossWords(s.dxMm)).arg(view.upWords(s.dyMm))
                                   }
                                   color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 12 }
                            Row {
                                spacing: 8
                                Text { text: qsTr("Evidence:"); color: view._txtMut
                                       font.family: theme.fontFamily; font.pixelSize: 11
                                       anchors.verticalCenter: parent.verticalCenter }
                                Rectangle {
                                    width: evT.implicitWidth + 14; height: 18; radius: 9
                                    color: "transparent"; border.width: 1
                                    border.color: modelData.hasDispersion === true ? view._green : view._amber
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { id: evT; anchors.centerIn: parent; text: modelData.evidence
                                           color: modelData.hasDispersion === true ? view._green : view._amber
                                           font.pixelSize: 9; font.bold: true }
                                }
                            }
                            Text { visible: view.sampleNote(modelData) !== ""
                                   width: parent.width; wrapMode: Text.WordWrap
                                   text: view.sampleNote(modelData); color: view._amber
                                   font.family: theme.fontFamily; font.pixelSize: 11 }

                            // This condition's OWN verdict, where the engine
                            // raised one. The session verdict is never repeated
                            // here — verdictForCondition matches on the compared
                            // condition, so a card without one shows nothing.
                            Column {
                                width: parent.width; spacing: 3
                                property var cv: view.verdictForCondition(modelData.label)
                                visible: cv !== null
                                Rectangle { width: parent.width; height: 1; color: view._line }
                                Row {
                                    spacing: 8
                                    Text { text: parent.parent.cv ? parent.parent.cv.category.toUpperCase() : ""
                                           color: view._red; font.family: theme.fontFamily
                                           font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                                    Text { text: parent.parent.cv ? parent.parent.cv.evidence : ""
                                           color: view._txtMut; font.family: theme.fontFamily
                                           font.pixelSize: 9 }
                                }
                                Text { width: parent.width; wrapMode: Text.WordWrap
                                       text: parent.cv ? parent.cv.headline : ""
                                       color: view._txt; font.family: theme.fontFamily; font.pixelSize: 12 }
                                Text { width: parent.width; wrapMode: Text.WordWrap
                                       text: parent.cv ? parent.cv.interpretation : ""
                                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                                Text { visible: parent.cv && parent.cv.nextTrainingStep !== ""
                                       width: parent.width; wrapMode: Text.WordWrap
                                       text: qsTr("Next: ") + (parent.cv ? parent.cv.nextTrainingStep : "")
                                       color: view._green; font.family: theme.fontFamily; font.pixelSize: 11 }
                            }

                            // technical values, hidden by default with definitions
                            Text {
                                text: (parent.parent.techOpen ? "▾  " : "▸  ")
                                      + qsTr("SHOW TECHNICAL MEASUREMENTS")
                                color: view._txtMut; font.family: theme.fontFamily
                                font.pixelSize: 10; font.bold: true; topPadding: 4
                                MouseArea { anchors.fill: parent
                                            onClicked: cardCol.parent.techOpen = !cardCol.parent.techOpen }
                            }
                            Column {
                                visible: cardCol.parent.techOpen
                                width: parent.width; spacing: 3
                                Repeater {
                                    model: [
                                        { l: "MPI X / Y",
                                          v: view.metric(modelData, "mpiXMm", "hasMpi") + " / "
                                             + view.metric(modelData, "mpiYMm", "hasMpi") + " mm",
                                          d: qsTr("Average centre of the recorded shot group") },
                                        { l: qsTr("Mean radius"),
                                          v: view.metric(modelData, "meanRadiusMm", "hasDispersion") + " mm",
                                          d: qsTr("Average distance of each shot from the group centre") },
                                        { l: qsTr("Group diameter"),
                                          v: view.metric(modelData, "groupDiameterMm", "hasDispersion") + " mm",
                                          d: qsTr("Distance between the two widest shots") },
                                        { l: qsTr("Horizontal spread"),
                                          v: view.metric(modelData, "horizontalSpreadMm", "hasDispersion") + " mm",
                                          d: qsTr("Total left-to-right width") },
                                        { l: qsTr("Vertical spread"),
                                          v: view.metric(modelData, "verticalSpreadMm", "hasDispersion") + " mm",
                                          d: qsTr("Total high-to-low height") },
                                        { l: qsTr("Score standard deviation"),
                                          v: view.metric(modelData, "scoreStdDev", "hasDispersion", 2),
                                          d: qsTr("How much the scores varied around their average") }
                                    ]
                                    delegate: Row {
                                        width: parent.width; spacing: 10
                                        Text { width: parent.width * 0.26; text: modelData.l
                                               color: view._txtSec; font.family: theme.fontFamily
                                               font.pixelSize: 11 }
                                        Text { width: parent.width * 0.18; text: modelData.v
                                               color: view._txt; font.family: "Consolas"; font.pixelSize: 11 }
                                        Text { width: parent.width * 0.5; text: modelData.d
                                               color: view._txtMut; font.family: theme.fontFamily
                                               font.pixelSize: 10; elide: Text.ElideRight }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════════
    // PAGE 3 — SHOT DETAILS (virtualised)
    // ══════════════════════════════════════════════════════════════════════
    Component {
        id: detailsPage
        Item {
            Column {
                id: dtHead
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                spacing: 6
                Text { text: qsTr("SHOT DETAILS") + "   ·   " + view.scopeName
                       color: view._txtMut; font.family: theme.fontFamily; font.pixelSize: 10
                       font.bold: true; font.letterSpacing: 2 }
                Text { width: parent.width; wrapMode: Text.WordWrap
                       text: qsTr("Every recorded shot with the condition that was standing when it "
                                + "was fired. Supporting evidence for the analysis above.")
                       color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11 }
                Text { visible: view.session.hasFiringDirection !== true
                       width: parent.width; wrapMode: Text.WordWrap
                       text: view.session.relativeWindNote ? view.session.relativeWindNote : ""
                       color: view._txtMut; font.family: theme.fontFamily
                       font.pixelSize: 10; font.italic: true }
                Row {
                    width: parent.width; spacing: 8; height: 22
                    Text { width: parent.width * 0.06; text: qsTr("#"); color: view._txtMut; font.pixelSize: 10 }
                    Text { width: parent.width * 0.10; text: qsTr("TYPE"); color: view._txtMut; font.pixelSize: 10 }
                    Text { width: parent.width * 0.11; text: qsTr("POSITION"); color: view._txtMut
                           font.pixelSize: 10; visible: view.threeP }
                    Text { width: parent.width * 0.08; text: qsTr("SCORE"); color: view._txtMut; font.pixelSize: 10 }
                    Text { width: parent.width * 0.16; text: qsTr("ACROSS / UP"); color: view._txtMut; font.pixelSize: 10 }
                    Text { text: qsTr("CONDITION WHEN FIRED"); color: view._txtMut; font.pixelSize: 10 }
                }
                Rectangle { width: parent.width; height: 1; color: view._line }
            }

            // A ListView creates only the delegates on screen — a 48-shot or
            // 200-shot session costs the same as a 10-shot one.
            ListView {
                id: shotList
                anchors.top: dtHead.bottom; anchors.topMargin: 6
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                clip: true
                cacheBuffer: 400
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                model: {
                    var all = view.model.shotRows ? view.model.shotRows : []
                    if (view.sessionScope) return all
                    var out = []
                    for (var i = 0; i < all.length; ++i)
                        if (all[i].position === view.pos.position) out.push(all[i])
                    return out
                }
                delegate: Column {
                    width: shotList.width
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
                        Text { width: parent.width * 0.06; text: modelData.shotId
                               color: view._txtSec; font.family: "Consolas"; font.pixelSize: 11 }
                        Text { width: parent.width * 0.10; text: modelData.type
                               color: modelData.sighter ? view._txtMut : view._txt
                               font.family: theme.fontFamily; font.pixelSize: 11 }
                        Text { width: parent.width * 0.11; text: modelData.positionName
                               color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                               visible: view.threeP }
                        Text { width: parent.width * 0.08; text: view.mm(modelData.score)
                               color: view._txtSec; font.family: "Consolas"; font.pixelSize: 11 }
                        Text { width: parent.width * 0.16
                               text: view.acrossWords(modelData.xMm) + ", " + view.upWords(modelData.yMm)
                               color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 10
                               elide: Text.ElideRight }
                        Row {
                            spacing: 6
                            Rectangle { width: 8; height: 8; radius: 4
                                        color: view.conditionColour(modelData.conditionLabel)
                                        anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.conditionLabel
                                         + (modelData.hasRelativeWind === true
                                            ? "   (" + modelData.relativeWind + ")" : "")
                                         + (modelData.note && modelData.note !== ""
                                            ? "   — " + modelData.note : "")
                                   color: view._txtSec; font.family: theme.fontFamily; font.pixelSize: 11
                                   anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
            }
        }
    }
}
