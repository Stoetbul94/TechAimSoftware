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
            anchors.verticalCenter: parent.verticalCenter; spacing: 2
            Row {
                spacing: 6
                Text { text: "TECH AIM"; color: header.accent; font.bold: true; font.pixelSize: 20; font.family: "Segoe UI"; font.letterSpacing: 1 }
                Text { text: "· Electronic Target"; color: "#9aa0aa"; font.pixelSize: 12; font.family: "Segoe UI"; anchors.baseline: parent.children[0].baseline }
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
