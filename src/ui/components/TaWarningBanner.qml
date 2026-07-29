import QtQuick 2.15

// Tech Aim Design System — inline warning / error banner (UI-1).
//
// Exists because the shipped build rendered "Changing mode requires an
// application restart" at ~8 px in muted grey — the least visible text on the
// screen, despite being one of the most consequential things it said.
//
// RULE: if it changes what the operator must do next, it gets a banner, and a
// banner is never smaller than helperText (11 px).
//
//   TaWarningBanner { theme: theme; kind: TaWarningBanner.Warning
//                     text: "Changing mode requires an application restart." }

Rectangle {
    id: root

    enum Kind { Info, Warning, Error }

    property int    kind: TaWarningBanner.Warning
    property string text: ""
    property var    theme: null           // required: the ancestor Theme instance

    readonly property color _fg: {
        switch (kind) {
        case TaWarningBanner.Error: return theme.tokens.errorText
        case TaWarningBanner.Info:  return theme.tokens.infoText
        default:                    return theme.tokens.warningText
        }
    }
    readonly property color _bg: {
        switch (kind) {
        case TaWarningBanner.Error: return theme.tokens.errorBackground
        case TaWarningBanner.Info:  return theme.tokens.infoBackground
        default:                    return theme.tokens.warningBackground
        }
    }
    readonly property string _glyph: {
        switch (kind) {
        case TaWarningBanner.Error: return "✕"   // ✕
        case TaWarningBanner.Info:  return "ℹ"   // ℹ
        default:                    return "⚠"   // ⚠
        }
    }

    implicitHeight: body.implicitHeight + 2 * theme.space.spacing12
    height: implicitHeight
    radius: theme.space.radiusSmall
    color: _bg
    border.width: theme.space.borderThin
    border.color: _fg

    Text {
        id: glyph
        anchors.left: parent.left
        anchors.leftMargin: root.theme.space.spacing12
        anchors.top: parent.top
        anchors.topMargin: root.theme.space.spacing12
        text: root._glyph
        color: root._fg
        font.pixelSize: root.theme.type.body.size + 2
    }

    Text {
        id: body
        anchors.left: glyph.right
        anchors.leftMargin: root.theme.space.spacing8
        anchors.right: parent.right
        anchors.rightMargin: root.theme.space.spacing12
        anchors.top: parent.top
        anchors.topMargin: root.theme.space.spacing12
        text: root.text
        color: root._fg
        // Never below helperText. German wraps freely; the banner grows.
        font.family:    root.theme.type.body.family
        font.pixelSize: root.theme.type.body.size
        wrapMode: Text.WordWrap
    }
}
