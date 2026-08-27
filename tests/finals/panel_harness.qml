import QtQuick 2.15
import "../.."   // resolve Finals3PRightPanel.qml from the repository root

// Automated harness for the 50 m 3P Final right panel.
//
// This mounts the REAL Finals3PRightPanel.qml against the REAL
// Finals3PController (exposed as the context property CTL by
// tst_finals3p.cpp), so the panel is exercised the way the application
// exercises it — the controller drives, QML bindings re-evaluate, and the test
// reads back rendered text through objectName.
//
// It is deliberately NOT the mock used by hud_harness.qml. A mock proves the
// panel renders what you tell it; this proves the panel renders what the
// competition controller actually says, through the full 35-shot course.
//
// `theme` is provided here as a root property because the application supplies
// it the same way (main.qml instantiates Theme once and every descendant
// resolves `theme` by ancestor scope). Only the tokens the panel reads are
// stubbed, with the real dark values, so a token rename fails this harness
// rather than silently rendering a transparent colour.
Item {
    id: root
    width: 420
    height: 800

    property QtObject theme: QtObject {
        property QtObject tokens: QtObject {
            property color surfacePrimary:   "#15171C"
            property color surfaceSecondary: "#1B1E24"
            property color surfaceElevated:  "#1F2026"
            property color borderSubtle:     "#2A2E36"
            property color accentPrimary:    "#A80038"
            property color textPrimary:      "#F3F6FA"
            property color textSecondary:    "#AAB4C0"
            property color textDisabled:     "#6F7A86"
            property color successText:      "#20C997"
            property color warningText:      "#E8A13D"
            property color textOnAccent:     "#FFFFFF"
        }
    }

    Finals3PRightPanel {
        objectName: "panel"
        anchors.fill: parent
        ctl: CTL
    }
}
