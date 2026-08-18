import QtQuick 2.15
import "../.."
// Before/after for the operating-mode pills under the SETA palette.
//
// BEFORE: the brand accent carried BOTH meanings - it bordered and lettered the
// Demo pill, so a blue "selected" signal sat on top of the red "this is Demo"
// signal and the two competed.
// AFTER: colour states WHAT MODE IT IS (red Demo / green Live) and a single
// restrained accent edge states only WHICH IS SELECTED.
Item {
    width: 760; height: 320
    property color accent: "#00539E"
    property color errText: "#D0392B"
    property color errBg:   "#2A0B10"
    property color okText:  "#20C997"
    property color okBg:    "#0D2018"
    property color input:   "#1D2026"
    property color border:  "#2A2E36"
    property color txt:     "#F3F6FA"
    property color mut:     "#6F7A86"

    Rectangle { anchors.fill: parent; color: "#15171C" }

    Column {
        x: 20; y: 16; spacing: 18
        Repeater {
            model: ["BEFORE  —  accent borders and letters the Demo pill",
                    "AFTER  —  semantic fill + one accent edge strip"]
            Column {
                spacing: 8
                Text { text: modelData; color: "#25B0E6"; font.pixelSize: 12; font.bold: true }
                Row {
                    spacing: 8
                    // Live pill (selected in neither case here)
                    Rectangle {
                        width: 352; height: 52; radius: 6
                        color: input; border.color: border; border.width: 1
                        Column {
                            anchors.centerIn: parent; spacing: 1
                            Text { text: "LIVE TARGET"; color: txt; font.pixelSize: 12; font.bold: true
                                   anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "Physical target"; color: mut; font.pixelSize: 10
                                   anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }
                    // Demo pill, SELECTED
                    Rectangle {
                        width: 352; height: 52; radius: 6
                        color: errBg
                        border.color: index === 0 ? accent : errText
                        border.width: 2
                        Rectangle {
                            visible: index === 1
                            anchors.left: parent.left; anchors.top: parent.top
                            anchors.bottom: parent.bottom; anchors.margins: 2
                            width: 4; radius: 2; color: accent
                        }
                        Column {
                            anchors.centerIn: parent; spacing: 1
                            Text { text: "DEMO / SIMULATION"
                                   color: index === 0 ? accent : errText
                                   font.pixelSize: 12; font.bold: true
                                   anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "Simulated clicks"; color: mut; font.pixelSize: 10
                                   anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }
                }
            }
        }
    }
}
