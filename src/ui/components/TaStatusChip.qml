import QtQuick 2.15

// Tech Aim Design System — status chip (UI-1).
//
// A READOUT, not a button. It reports connection state, operating mode and
// similar. Colours are semantic tokens, so the four states cannot drift apart
// across screens the way the hand-rolled chips did.
//
//   TaStatusChip { theme: theme; state: TaStatusChip.Success; text: "Connected" }
//
// RULE: a chip must never contradict the control beside it. "Connected" next
// to an empty COM-port field was a real UI-0 finding and is prohibited.

Rectangle {
    id: root

    enum Kind { Neutral, Success, Warning, Error }

    property int    kind: TaStatusChip.Neutral
    property string text: ""
    property bool   showDot: true
    property var    theme: null           // required: the ancestor Theme instance
    // Set true ONLY when tapping the chip does something; it then becomes a
    // control and must meet the 44 px touch floor.
    property bool   interactive: false
    signal clicked()

    readonly property color _fg: {
        switch (kind) {
        case TaStatusChip.Success: return theme.tokens.successText
        case TaStatusChip.Warning: return theme.tokens.warningText
        case TaStatusChip.Error:   return theme.tokens.errorText
        default:                   return theme.tokens.textDisabled
        }
    }
    readonly property color _bg: {
        switch (kind) {
        case TaStatusChip.Success: return theme.tokens.successBackground
        case TaStatusChip.Warning: return theme.tokens.warningBackground
        case TaStatusChip.Error:   return theme.tokens.errorBackground
        default:                   return theme.tokens.surfaceSecondary
        }
    }

    implicitWidth: row.implicitWidth + 24
    implicitHeight: interactive ? theme.space.touchMinimum : 30
    width: implicitWidth
    height: implicitHeight
    radius: theme.space.radiusSmall
    color: _bg
    border.width: theme.space.borderThin
    border.color: kind === TaStatusChip.Neutral ? theme.tokens.borderSubtle : _fg

    Row {
        id: row
        anchors.centerIn: parent
        spacing: root.theme.space.spacing8

        Rectangle {
            width: 7; height: 7; radius: 4
            color: root._fg
            visible: root.showDot
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.text
            color: root._fg
            font.family:       root.theme.type.statusText.family
            font.pixelSize:    root.theme.type.statusText.size
            font.bold:         root.theme.type.statusText.bold
            font.letterSpacing: root.theme.type.statusText.spacing
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
