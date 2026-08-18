import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: modBusConnectorDia
    title: qsTr("Port Connector")
    header: null
    footer: null

    contentItem: Rectangle {
        id:contentRect
        anchors.fill: parent
        color: "grey"

        Column {
            anchors.centerIn: parent
            height: childrenRect.height
            width: childrenRect.width

            spacing: 10
            Text {
                text: qsTr("Please Provide a Port Number")
                color: "black"
                font.pixelSize: 15
            }

            TextField {
                id: portTextInput
                width: 100
                height: 20
                placeholderText: qsTr("e.g. COM6")
            }

            Text {
                id: connectError
                text: qsTr("Could not connect — check cable and port")
                color: "#7a0016"
                font.pixelSize: 11
                visible: false
            }

            Row {
                spacing: 8
                Button {
                    text: qsTr("Connect")
                    onClicked: {
                        console.log("**************************************", portTextInput.text)
                        MODREADER.connectedModbus(portTextInput.text)
                        mod_connected = MODREADER.isModBusConnected()
                        if (MODREADER.isModBusConnected()) {
                            connectError.visible = false
                            modBusConnectorDia.close()
                        } else {
                            connectError.visible = true
                        }
                    }
                }
                Button {
                    text: qsTr("Cancel")
                    onClicked: {
                        connectError.visible = false
                        modBusConnectorDia.close()
                    }
                }
            }
        }
    }
}
