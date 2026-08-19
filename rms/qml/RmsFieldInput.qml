import QtQuick 2.15

// A labelled single-line input. Nothing here is required: a field test with no
// operator name is still a field test, and a form that refuses to start is
// worse than a bundle with a blank field in it.
Item {
    id: field

    property string label: ""
    property string placeholder: ""
    property alias value: input.text

    Theme { id: theme }

    width: 240
    height: 46

    Text {
        id: cap
        anchors { top: parent.top; left: parent.left }
        text: field.label
        font.family: theme.fontFamily
        font.pixelSize: 9
        font.letterSpacing: 1.0
        color: theme.textSecondary
    }
    Rectangle {
        anchors { top: cap.bottom; topMargin: 3; left: parent.left
                  right: parent.right; bottom: parent.bottom }
        radius: theme.radiusSmall
        color: theme.bgBase
        border.width: 1
        border.color: input.activeFocus ? theme.brandPrimary : theme.borderColor

        TextInput {
            id: input
            anchors.fill: parent
            anchors.leftMargin: theme.spacingUnit
            anchors.rightMargin: theme.spacingUnit
            verticalAlignment: TextInput.AlignVCenter
            clip: true
            selectByMouse: true
            font.family: theme.fontFamily
            font.pixelSize: 12
            color: theme.textPrimary
            selectionColor: theme.brandPrimary
            selectedTextColor: theme.textOnBrand
        }
        Text {
            anchors.fill: parent
            anchors.leftMargin: theme.spacingUnit
            verticalAlignment: Text.AlignVCenter
            visible: input.text.length === 0 && !input.activeFocus
            text: field.placeholder
            font.family: theme.fontFamily
            font.pixelSize: 12
            color: theme.textSecondary
            opacity: 0.6
        }
    }
}
