import QtQuick 2.15

// DISPLAYS — the shell only.
//
// The spectator display server is NOT built. This page exists so the
// navigation it will live under is settled now, and so nobody has to redesign
// the shell around it later. Every control here is inert and says so; none of
// them is a stub that looks like it works.
Item {
    id: page

    Theme { id: theme }

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 3
        spacing: theme.spacingUnit * 2

        Text {
            text: "DISPLAYS"
            font.family: theme.fontFamily
            font.pixelSize: 24
            font.weight: Font.DemiBold
            font.letterSpacing: 1.4
            color: theme.textPrimary
        }

        Rectangle {
            width: parent.width
            height: 62
            radius: theme.radiusMedium
            color: Qt.rgba(0.75, 0.10, 0.10, 0.10)
            border.width: 1
            border.color: theme.brandAccent
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: theme.spacingUnit * 2
                wrapMode: Text.WordWrap
                text: "Display management arrives in a later milestone. Nothing on "
                      + "this page is connected to a display; the layout exists so "
                      + "the navigation does not have to be redesigned around it."
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textPrimary
            }
        }

        Text {
            text: "PLANNED DISPLAY TARGETS"
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            color: theme.textSecondary
        }

        Flow {
            width: parent.width
            spacing: theme.spacingUnit
            Repeater {
                model: [
                    { name: "ALL",              note: "every configured display" },
                    { name: "MAIN TV",          note: "range-side scoreboard" },
                    { name: "CLUBHOUSE",        note: "spectator feed" },
                    { name: "FINALS",           note: "finals presentation" },
                    { name: "ATHLETE DISPLAYS", note: "per-lane athlete view" }
                ]
                delegate: Rectangle {
                    width: 220
                    height: 66
                    radius: theme.radiusMedium
                    color: theme.bgBase
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.06)
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: theme.spacingUnit * 2
                        anchors.right: parent.right
                        anchors.rightMargin: theme.spacingUnit
                        spacing: 3
                        Text {
                            text: modelData.name
                            font.family: theme.fontFamily
                            font.pixelSize: 13
                            font.letterSpacing: 1.1
                            color: theme.statusDisconnected
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: modelData.note
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }
        }

        Row {
            spacing: theme.spacingUnit
            Repeater {
                model: ["◀ PREVIOUS DISPLAY", "NEXT DISPLAY ▶"]
                delegate: Rectangle {
                    width: 190; height: 34
                    radius: theme.radiusSmall
                    color: theme.bgBase
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.06)
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        font.letterSpacing: 1.1
                        color: theme.statusDisconnected
                    }
                }
            }
        }

        Text {
            text: "Not implemented — no display server exists in this build."
            font.family: theme.fontFamily
            font.pixelSize: 10
            color: theme.textSecondary
        }
    }
}
