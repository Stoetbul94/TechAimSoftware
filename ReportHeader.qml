import QtQuick 2.15

// Tech Aim Report System — unified report header.
// Brand wordmark + report title on the left; meta fields (athlete, discipline,
// date/time, etc.) on the right. Meta fields with empty values are hidden so
// the header adapts per discipline/event without empty placeholders.
Item {
    id: header
    property string reportTitle: "Report"
    property string athlete: ""
    property string discipline: ""
    property string sessionType: ""     // Training / Match / Qualification / Final
    property string dateText: ""
    property string timeText: ""
    property string range: ""
    property string lane: ""
    property string extraLabel: ""       // free slot (e.g. Wind / Light)
    property string extraValue: ""
    // F10: operating mode on the report so a Demo session is never mistaken for
    // an official Live result. Defaults to the running mode; a caller may pass a
    // recovered session's RECORDED mode to override.
    property string mode: (typeof OPMODE !== "undefined") ? OPMODE.badgeText : ""
    property var    extraPairs: []       // additional [{l,v}] appended after built-ins (e.g. Shots/Score/Time)
    property color  accent: "#a80038"

    implicitHeight: 78

    // meta as [label, value] pairs; empties dropped
    function metaPairs() {
        var p = []
        function add(l, v) { if (v && String(v).length) p.push({ l: l, v: v }) }
        add("Athlete", athlete)
        add("Discipline", discipline)
        add("Session", sessionType)
        add("Date", dateText)
        add("Time", timeText)
        add("Range", range)
        add("Lane", lane)
        add("Mode", mode)
        add(extraLabel, extraValue)
        for (var i = 0; i < extraPairs.length; ++i)
            add(extraPairs[i].l, extraPairs[i].v)
        return p
    }

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12
        Rectangle { width: 6; height: 44; radius: 3; color: header.accent; anchors.verticalCenter: parent.verticalCenter }
        Column {
            anchors.verticalCenter: parent.verticalCenter; spacing: 4
            // Brand logo (replaces the text wordmark). Color variant — report
            // pages are always white. mipmap keeps the PDF grab (3-4x) crisp.
            Image {
                source: "qrc:/images/logo/techaim_color.png"
                height: 26
                width: sourceSize.height > 0 ? height * sourceSize.width / sourceSize.height : 0
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }
            Text { text: header.reportTitle; color: "#191b1f"; font.pixelSize: 22; font.bold: true; font.family: "Segoe UI" }
        }
    }

    // meta fields, right-aligned two-column flow
    Grid {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        columns: 2
        columnSpacing: 26
        rowSpacing: 3
        Repeater {
            model: header.metaPairs()
            delegate: Row {
                spacing: 6
                Text { text: modelData.l + ":"; color: "#8a8f98"; font.pixelSize: 11; font.family: "Segoe UI" }
                Text { text: modelData.v; color: "#33373d"; font.pixelSize: 11; font.bold: true; font.family: "Segoe UI" }
            }
        }
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#e6e8ec" }
}
