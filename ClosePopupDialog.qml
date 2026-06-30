import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs

Dialog {
    id: closeDia
    title: "Confirm"
    header: null
    footer: null

    signal cancel()
    signal discard()
    signal save(string fileName)

    contentItem: Rectangle {
        id: mainRect
        anchors.fill: parent
        color: "grey"

        Text {
            id: infoText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("The Match is finished.")
        }
        Text {
            id: infoText1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: infoText.bottom
            anchors.topMargin: 10
            text: qsTr("Do you want to save your match?")
        }

        Button {
            id: cancelButton
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10

            text: "Cancel"

            onClicked: {
                cancel()
            }
        }

        Button {
            id: discardButton
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.right: cancelButton.left
            anchors.rightMargin: 10

            text: "Discard"

            onClicked: {
                discard()
            }
        }

        Button {
            id: saveButton
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.right: discardButton.left
            anchors.rightMargin: 10

            text: "Save"

            onClicked: {
                //fileDialog.visible = true
                save("test")
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please provide a file name"
        fileMode: FileDialog.SaveFile
        currentFolder: Qt.resolvedUrl(".")

        onAccepted: {
            console.log("You chose: " + fileDialog.selectedFiles)
            save(fileDialog.selectedFiles)
        }
        onRejected: {
            console.log("Canceled")
        }
    }
}
