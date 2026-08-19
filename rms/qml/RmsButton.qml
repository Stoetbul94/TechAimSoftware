import QtQuick 2.15

// A plain action button. A disabled one is visibly inert — RMS has controls it
// deliberately does not have, and those must never look pressable.
Rectangle {
    id: button

    property string label: ""
    property bool primary: false
    property bool enabled: true
    signal activated()

    function activate() { if (enabled) activated() }

    Theme { id: theme }

    implicitWidth: text.implicitWidth + 32
    implicitHeight: 32
    width: implicitWidth
    height: implicitHeight
    radius: theme.radiusSmall
    color: !enabled ? theme.bgBase
         : primary ? theme.brandPrimary
                   : theme.bgSurfaceAlt
    border.width: 1
    border.color: !enabled ? Qt.rgba(1, 1, 1, 0.06)
                : primary ? theme.brandPrimary
                          : theme.borderColor

    Text {
        id: text
        anchors.centerIn: parent
        text: button.label
        font.family: theme.fontFamily
        font.pixelSize: 11
        font.letterSpacing: 1.1
        color: !button.enabled ? theme.statusDisconnected
             : button.primary ? theme.textOnBrand
                              : theme.textPrimary
    }

    MouseArea {
        anchors.fill: parent
        enabled: button.enabled
        cursorShape: button.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: button.activated()
    }
}
