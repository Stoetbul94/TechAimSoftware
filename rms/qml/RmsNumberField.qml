import QtQuick 2.15

// A small stepper. Typed input is accepted but clamped, so a lane range can
// never be committed backwards or at zero.
Column {
    id: field

    property string label: ""
    property int value: 1
    property int minimum: 1
    property int maximum: 200
    signal valueEdited(int v)

    Theme { id: theme }

    spacing: 4

    function commit(v) {
        var clamped = Math.max(minimum, Math.min(maximum, v))
        input.text = String(clamped)
        if (clamped !== value)
            field.valueEdited(clamped)
    }

    Text {
        text: field.label
        font.family: theme.fontFamily
        font.pixelSize: 10
        color: theme.textSecondary
    }

    Row {
        spacing: 4
        Rectangle {
            width: 70
            height: 32
            color: theme.bgBase
            border.width: 1
            border.color: input.activeFocus ? theme.brandPrimary : theme.borderColor
            radius: theme.radiusSmall
            TextInput {
                id: input
                anchors.fill: parent
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                text: String(field.value)
                font.family: theme.fontFamily
                font.pixelSize: 15
                color: theme.textPrimary
                selectByMouse: true
                validator: IntValidator { bottom: 1; top: 200 }
                onEditingFinished: field.commit(parseInt(text) || field.minimum)
            }
        }
        Column {
            spacing: 2
            Repeater {
                model: [ {t: "+", d: 1}, {t: "−", d: -1} ]
                delegate: Rectangle {
                    width: 22; height: 15
                    color: theme.bgSurfaceAlt
                    border.width: 1
                    border.color: theme.borderColor
                    radius: 2
                    Text {
                        anchors.centerIn: parent
                        text: modelData.t
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        color: theme.textSecondary
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: field.commit(field.value + modelData.d)
                    }
                }
            }
        }
    }
}
