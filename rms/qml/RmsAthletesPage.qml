import QtQuick 2.15

// ATHLETES — RMS's own start list.
//
// Deliberately small: a name, a club, a country, a note. Licences,
// classifications and eligibility are federation concerns, and an RMS-local
// version of any of them would be a second source of truth no federation
// recognises.
Item {
    id: page

    Theme { id: theme }

    property string selectedId: ""
    property string rejection: ""

    Connections {
        target: ATHLETES
        function onRejected(reason) { page.rejection = reason }
        function onAthletesChanged() { page.rejection = "" }
    }

    Column {
        id: head
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 2.5
        spacing: theme.spacingUnit

        Text {
            text: "ATHLETES"
            font.family: theme.fontFamily
            font.pixelSize: 22
            font.weight: Font.DemiBold
            color: theme.textPrimary
        }
        Text {
            text: ATHLETEMODEL.count + " on the start list"
            font.family: theme.fontFamily
            font.pixelSize: 12
            color: theme.textSecondary
        }

        Row {
            spacing: 6
            Rectangle {
                width: 300; height: 34
                color: theme.bgBase
                border.width: 1
                border.color: nameField.activeFocus ? theme.brandPrimary : theme.borderColor
                radius: theme.radiusSmall
                TextInput {
                    id: nameField
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    verticalAlignment: TextInput.AlignVCenter
                    font.family: theme.fontFamily
                    font.pixelSize: 14
                    color: theme.textPrimary
                    selectByMouse: true
                    onAccepted: addBtn.activate()
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: nameField.text.length === 0
                        text: "Athlete name"
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        color: theme.statusDisconnected
                    }
                }
            }
            Rectangle {
                width: 180; height: 34
                color: theme.bgBase
                border.width: 1
                border.color: clubField.activeFocus ? theme.brandPrimary : theme.borderColor
                radius: theme.radiusSmall
                TextInput {
                    id: clubField
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    verticalAlignment: TextInput.AlignVCenter
                    font.family: theme.fontFamily
                    font.pixelSize: 14
                    color: theme.textPrimary
                    selectByMouse: true
                    onAccepted: addBtn.activate()
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: clubField.text.length === 0
                        text: "Club (optional)"
                        font.family: theme.fontFamily
                        font.pixelSize: 14
                        color: theme.statusDisconnected
                    }
                }
            }
            RmsButton {
                id: addBtn
                label: "ADD"
                primary: true
                enabled: nameField.text.trim().length > 0
                onActivated: {
                    ATHLETES.addAthlete(nameField.text, clubField.text, "", false)
                    nameField.text = ""
                    clubField.text = ""
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: page.rejection.length > 0
            text: page.rejection
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.brandAccent
        }
    }

    ListView {
        anchors { top: head.bottom; topMargin: theme.spacingUnit
                  left: parent.left; bottom: parent.bottom }
        anchors.leftMargin: theme.spacingUnit * 2.5
        anchors.bottomMargin: theme.spacingUnit * 2.5
        width: Math.min(parent.width - theme.spacingUnit * 5, 820)
        clip: true
        spacing: 5
        model: ATHLETEMODEL

        delegate: Rectangle {
            width: ListView.view.width
            height: 46
            radius: theme.radiusSmall
            color: theme.bgSurface
            border.width: 1
            border.color: theme.borderColor

            Text {
                id: nm
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: theme.spacingUnit * 1.5
                width: 260
                elide: Text.ElideRight
                text: model.displayName
                font.family: theme.fontFamily
                font.pixelSize: 14
                color: theme.textPrimary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: nm.right
                width: 180
                elide: Text.ElideRight
                text: model.club.length > 0 ? model.club : "—"
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textSecondary
            }
            RmsStatusPill {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: removeBtn.left
                anchors.rightMargin: theme.spacingUnit
                visible: model.assigned
                text: "ON LANE " + model.assignedLane
                tone: "live"
            }
            RmsButton {
                id: removeBtn
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: theme.spacingUnit * 1.5
                label: "REMOVE"
                onActivated: ATHLETES.removeAthlete(model.athleteId)
            }
        }

        Text {
            anchors.centerIn: parent
            visible: ATHLETEMODEL.count === 0
            text: "No athletes yet. Add one above — a name is enough."
            font.family: theme.fontFamily
            font.pixelSize: 13
            color: theme.textSecondary
        }
    }
}
