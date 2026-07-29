import QtQuick 2.15

// Tech Aim Design System — button (UI-1).
//
// One component, three variants, so "primary / secondary / ghost" is a
// property rather than three near-identical copies drifting apart. Every
// colour comes from the token layer; nothing here is hard-coded.
//
//   TaButton { text: "Start session"; variant: TaButton.Primary; onClicked: … }
//
// States covered: default, hover, pressed, disabled, keyboard focus.
// Touch: heights come from the spacing scale and clear the 44 px floor.

Rectangle {
    id: root

    enum Variant { Primary, Secondary, Ghost }

    property int     variant: TaButton.Primary
    property string  text: ""
    property bool    enabled: true
    property var     theme: null          // required: the ancestor Theme instance
    property alias   pressed: mouse.pressed
    property alias   hovered: mouse.containsMouse
    signal clicked()

    readonly property bool _primary: variant === TaButton.Primary
    readonly property bool _ghost:   variant === TaButton.Ghost

    implicitWidth: Math.max(label.implicitWidth + 44, _ghost ? 88 : 160)
    implicitHeight: _ghost ? theme.space.touchMinimum : theme.space.controlHeightLarge
    width: implicitWidth
    height: implicitHeight
    radius: theme.space.radiusMedium

    color: {
        if (!_primary) return "transparent"
        if (!enabled) return theme.tokens.accentPrimary
        if (mouse.pressed) return theme.tokens.accentPressed
        if (mouse.containsMouse) return theme.tokens.accentHover
        return theme.tokens.accentPrimary
    }
    border.width: _primary || _ghost ? 0 : theme.space.borderThin
    border.color: mouse.containsMouse ? theme.tokens.textDisabled
                                      : theme.tokens.borderStrong
    // Disabled is opacity + no interaction, never colour alone.
    opacity: enabled ? 1.0 : theme.tokens.disabledOpacity

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root._primary ? root.theme.tokens.textOnAccent
                             : root.theme.tokens.textSecondary
        font.family:    root.theme.type.buttonText.family
        font.pixelSize: root.theme.type.buttonText.size
        font.bold:      root.theme.type.buttonText.bold
    }

    // Keyboard focus must be visible, and never communicated by fill alone.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius + 2
        color: "transparent"
        visible: root.activeFocus
        border.width: root.theme.tokens.focusOutlineWidth
        border.color: root.theme.tokens.focusOutline
    }

    activeFocusOnTab: enabled
    Keys.onReturnPressed: if (enabled) root.clicked()
    Keys.onSpacePressed:  if (enabled) root.clicked()

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: { root.forceActiveFocus(); root.clicked() }
    }
}
