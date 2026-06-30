import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: modBusConnectorDia
    title: "Port Connector"
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
                text: "Please Provide a Port Number"
                color: "black"
                font.pixelSize: 15
            }

            TextField {
                id: portTextInput
                width: 100
                height: 20
                placeholderText: "port number"
            }

            Button {
                text: "Connect"
                onClicked: {
                    console.log("**************************************", portTextInput.text)
                    MODREADER.connectedModbus(portTextInput.text)
                    mod_connected = MODREADER.isModBusConnected()
                    if (MODREADER.isModBusConnected())
                        modBusConnectorDia.close()
                }
            }
        }
    }
}
