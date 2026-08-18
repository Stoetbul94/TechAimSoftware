import QtQuick 2.15

// Wind Map — athlete-facing target graphic (Stage 6.1.1, UI-WIND-005).
//
// Replaces the abstract dark rectangle. Every mark on this plot appears in the
// legend, and the orientation, scale and centre are all drawn explicitly so an
// athlete can read it without being told how.
//
// It calculates NOTHING. Every coordinate, centre and radius comes from the
// analysis model; this maps millimetres to pixels and draws.
//
// It is NOT an aiming instruction: there is no arrow, no correction and no
// suggested hold. It shows where shots landed and where their centre sat.
Item {
    id: plot

    // From the analysis model — never computed here.
    property var shots: []             // model.shotRows, already position-filtered
    property var groups: []            // pos.byExactCondition
    property var reference: null       // pos.reference
    property string conditionFilter: ""
    property bool showSighters: false
    property var colourFor: null       // function(label) -> colour

    // The half-width of the drawn field in mm. Derived ONCE per data change,
    // not per binding — the Stage 6.1 version recomputed this inside every
    // shot's x/y binding, which is O(n²) evaluations for n shots.
    property real spanMm: 20
    function recomputeSpan() {
        var mx = 8
        for (var i = 0; i < shots.length; ++i) {
            var s = shots[i]
            if (s.sighter && !showSighters) continue
            if (plot.conditionFilter !== "" && s.conditionLabel !== plot.conditionFilter) continue
            mx = Math.max(mx, Math.abs(s.xMm), Math.abs(s.yMm))
        }
        // Round up to a tidy 5 mm step so the scale marker reads cleanly.
        plot.spanMm = Math.ceil((mx * 1.25) / 5) * 5
    }
    onShotsChanged: recomputeSpan()
    onShowSightersChanged: recomputeSpan()
    onConditionFilterChanged: recomputeSpan()
    Component.onCompleted: recomputeSpan()

    readonly property real fieldR: Math.min(width, height) / 2 - 26
    function px(mm) { return width / 2 + (mm / plot.spanMm) * plot.fieldR }
    function py(mm) { return height / 2 - (mm / plot.spanMm) * plot.fieldR }

    readonly property color _line:   "#2A2E36"
    readonly property color _ring:   "#3A4150"
    readonly property color _txtMut: "#6F7A86"
    readonly property color _txtSec: "#B6BCC6"

    Rectangle {
        anchors.fill: parent
        color: "#0E1014"; radius: 8
        border.color: plot._line; border.width: 1
    }

    // ── target rings ────────────────────────────────────────────────────
    // Evenly spaced reference rings at tidy millimetre steps, so the graphic
    // reads as a target rather than a scatter chart. The ISSF ring geometry
    // itself belongs to the scoring pipeline and is not duplicated here.
    Repeater {
        model: [0.25, 0.5, 0.75, 1.0]
        delegate: Rectangle {
            width: 2 * modelData * plot.fieldR; height: width; radius: width / 2
            x: plot.width / 2 - width / 2
            y: plot.height / 2 - height / 2
            color: "transparent"
            border.color: plot._ring
            border.width: modelData === 1.0 ? 1 : 1
            opacity: modelData === 1.0 ? 0.9 : 0.45
        }
    }

    // ── target centre ───────────────────────────────────────────────────
    Rectangle { width: 15; height: 1; color: plot._txtSec
                x: plot.width / 2 - 7; y: plot.height / 2 }
    Rectangle { width: 1; height: 15; color: plot._txtSec
                x: plot.width / 2; y: plot.height / 2 - 7 }

    // ── orientation, spelled out ────────────────────────────────────────
    Text { text: qsTr("HIGH"); color: plot._txtMut; font.pixelSize: 9; font.bold: true
           font.letterSpacing: 1
           anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top
           anchors.topMargin: 6 }
    Text { text: qsTr("LOW"); color: plot._txtMut; font.pixelSize: 9; font.bold: true
           font.letterSpacing: 1
           anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom
           anchors.bottomMargin: 6 }
    Text { text: qsTr("LEFT"); color: plot._txtMut; font.pixelSize: 9; font.bold: true
           font.letterSpacing: 1
           anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left
           anchors.leftMargin: 4 }
    Text { text: qsTr("RIGHT"); color: plot._txtMut; font.pixelSize: 9; font.bold: true
           font.letterSpacing: 1
           anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right
           anchors.rightMargin: 4 }

    // ── millimetre scale marker ─────────────────────────────────────────
    Item {
        anchors.left: parent.left; anchors.leftMargin: 12
        anchors.bottom: parent.bottom; anchors.bottomMargin: 22
        width: plot.fieldR / 2; height: 12
        Rectangle { width: parent.width; height: 1; color: plot._txtSec
                    anchors.verticalCenter: parent.verticalCenter }
        Rectangle { width: 1; height: 7; color: plot._txtSec; x: 0
                    anchors.verticalCenter: parent.verticalCenter }
        Rectangle { width: 1; height: 7; color: plot._txtSec; x: parent.width - 1
                    anchors.verticalCenter: parent.verticalCenter }
        Text {
            anchors.bottom: parent.top; anchors.left: parent.left
            text: (plot.spanMm / 2).toFixed(0) + qsTr(" mm")
            color: plot._txtSec; font.pixelSize: 9
        }
    }

    // ── mean-radius circle, only where the sample supports one ──────────
    Repeater {
        model: plot.groups
        delegate: Rectangle {
            visible: modelData.hasDispersion === true && modelData.hasMpi === true
                     && (plot.conditionFilter === "" || plot.conditionFilter === modelData.label)
            width: 2 * (modelData.meanRadiusMm / plot.spanMm) * plot.fieldR
            height: width; radius: width / 2
            x: plot.px(modelData.mpiXMm) - width / 2
            y: plot.py(modelData.mpiYMm) - height / 2
            color: "transparent"; border.width: 1
            border.color: plot.colourFor ? plot.colourFor(modelData.label) : "white"
            opacity: 0.40
        }
    }

    // ── shots ───────────────────────────────────────────────────────────
    Repeater {
        model: plot.shots
        delegate: Rectangle {
            visible: (!modelData.sighter || plot.showSighters)
                     && (plot.conditionFilter === ""
                         || plot.conditionFilter === modelData.conditionLabel)
            width: modelData.sighter ? 7 : 9
            height: width; radius: width / 2
            x: plot.px(modelData.xMm) - width / 2
            y: plot.py(modelData.yMm) - height / 2
            // A sighter is a HOLLOW ring of the same colour — stated in the
            // legend, so a hollow marker is never unexplained.
            color: modelData.sighter
                   ? "transparent"
                   : (plot.colourFor ? plot.colourFor(modelData.conditionLabel) : "white")
            border.width: modelData.sighter ? 2 : 0
            border.color: plot.colourFor ? plot.colourFor(modelData.conditionLabel) : "white"
        }
    }

    // ── each group's centre ─────────────────────────────────────────────
    Repeater {
        model: plot.groups
        delegate: Item {
            visible: modelData.hasMpi === true
                     && (plot.conditionFilter === "" || plot.conditionFilter === modelData.label)
            x: plot.px(modelData.mpiXMm); y: plot.py(modelData.mpiYMm)
            Rectangle { width: 14; height: 2; x: -7; y: -1
                        color: plot.colourFor ? plot.colourFor(modelData.label) : "white" }
            Rectangle { width: 2; height: 14; x: -1; y: -7
                        color: plot.colourFor ? plot.colourFor(modelData.label) : "white" }
        }
    }

    // ── the reference centre a shift is measured FROM ───────────────────
    Rectangle {
        visible: plot.reference !== null && plot.reference !== undefined
                 && plot.reference.valid === true
        width: 18; height: 18; radius: 9
        x: plot.px(plot.reference && plot.reference.valid ? plot.reference.xMm : 0) - 9
        y: plot.py(plot.reference && plot.reference.valid ? plot.reference.yMm : 0) - 9
        color: "transparent"; border.color: "#FFFFFF"; border.width: 2
    }
}
