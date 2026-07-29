import QtQuick 2.15

// Tech Aim Design System — typography roles (UI-1).
//
// The audit measured 22 distinct font.pixelSize values across 66 QML files,
// with 9/10/11/12 used interchangeably for what is visually the same role.
// A screen should ask for a ROLE ("this is a card title"), not a number.
//
// Reached as `theme.type.<role>.size` / `.bold` / `.spacing`.
//
// FONTS: both families ship with Windows, so nothing here introduces a
// distribution dependency and no proprietary font is required.
//   uiFamily      — Segoe UI, the Windows system face
//   numericFamily — Consolas, FUNCTIONAL use only (see below)

QtObject {
    id: type

    readonly property string uiFamily:      "Segoe UI"
    readonly property string numericFamily: "Consolas"

    // Consolas is permitted ONLY where fixed-width alignment does a job:
    // series score columns, shot tables, anything read down a column. Using it
    // for decorative "technical" styling on prose or single values was flagged
    // as a defect in the UI-0 audit — it makes the product look like console
    // output. `numericMetric` below is the sanctioned role.

    readonly property QtObject displayTitle: QtObject {
        readonly property int    size: 28
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject pageTitle: QtObject {
        readonly property int    size: 22
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject sectionTitle: QtObject {
        readonly property int    size: 16
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject cardTitle: QtObject {
        readonly property int    size: 14
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject body: QtObject {
        readonly property int    size: 13
        readonly property bool   bold: false
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject bodyStrong: QtObject {
        readonly property int    size: 13
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject helperText: QtObject {
        readonly property int    size: 11
        readonly property bool   bold: false
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    // Micro-label above a field or section. Uppercase + wide tracking is the
    // Tech Aim signature; it is what makes the dense panels scannable.
    readonly property QtObject label: QtObject {
        readonly property int    size: 10
        readonly property bool   bold: true
        readonly property real   spacing: 2.0
        readonly property string family: type.uiFamily
    }
    readonly property QtObject buttonText: QtObject {
        readonly property int    size: 14
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.uiFamily
    }
    // The sanctioned fixed-width role: tabular figures only.
    readonly property QtObject numericMetric: QtObject {
        readonly property int    size: 14
        readonly property bool   bold: true
        readonly property real   spacing: 0
        readonly property string family: type.numericFamily
    }
    readonly property QtObject statusText: QtObject {
        readonly property int    size: 11
        readonly property bool   bold: true
        readonly property real   spacing: 0.5
        readonly property string family: type.uiFamily
    }

    // German runs materially longer than English — "Betriebsart" for "Mode",
    // "Netzwerkfreigabe" for "Share". Layouts sized to English text clip when
    // the catalogue is switched. This is the allowance Screen_Layout_Rules.md
    // requires labels to be laid out against.
    readonly property real germanExpansionFactor: 1.35
}
