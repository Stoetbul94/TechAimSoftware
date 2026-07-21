import QtQuick 2.2
import QtQuick.Window 2.2

Item {
    id: name

    property int rootItemWidth:158
    property int rootItemHeight:61

    width:158
    height: 61

    property bool isPalletRedColor: true
    property bool isBackGroundBlack: true

    Image {
        id: settings_popup
        source: "qrc:/images/settings/settings_popup.png"
        x: ((parent.width/rootItemWidth)*1)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_green_highlight
        source: gameRange == 10 ? "qrc:/images/settings/pellet_color_yellow_highlight.png" : "qrc:/images/settings/pellet_color_red_highlight.png"
        x: ((parent.width/rootItemWidth)*98)
        y: ((parent.height/rootItemHeight)*32)
        opacity: isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                isPalletRedColor = true
            }
        }
    }
    Image {
        id: background_color_white_no_highlight
        source: "qrc:/images/settings/background_color_blue_no_highlight.png"
        x: ((parent.width/rootItemWidth)*123)
        y: ((parent.height/rootItemHeight)*6)
        opacity: isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_white_no_highlight
        source: "qrc:/images/settings/pellet_color_green_highlight.png"
        x: ((parent.width/rootItemWidth)*123)
        y: ((parent.height/rootItemHeight)*32)
        opacity: isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: background_color_black_no_highlight
        source: "qrc:/images/settings/background_color_black_no_highlight.png"
        x: ((parent.width/rootItemWidth)*98)
        y: ((parent.height/rootItemHeight)*6)
        opacity: !isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pellet_color_white_highlight
        source: "qrc:/images/settings/pellet_color_green_highlight_1.png"
        x: ((parent.width/rootItemWidth)*124)
        y: ((parent.height/rootItemHeight)*33)
        opacity: !isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                isPalletRedColor = false
                console.log("*************test")
            }
        }
    }
    Image {
        id: pellet_color_green_highlight_1
        source: gameRange == 10 ? "qrc:/images/settings/pellet_color_yellow_no_highlight.png" : "qrc:/images/settings/pellet_color_red_no_highlight.png"
        x: ((parent.width/rootItemWidth)*99)
        y: ((parent.height/rootItemHeight)*33)
        opacity: !isPalletRedColor
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: background_color_white_highlight
        source: "qrc:/images/settings/background_color_blue_highlight.png"
        x: ((parent.width/rootItemWidth)*124)
        y: ((parent.height/rootItemHeight)*6)
        opacity: !isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                isBackGroundBlack = false
            }
        }
    }
    Image {
        id: background_color_black_highlight
        source: "qrc:/images/settings/background_color_black_highlight.png"
        x: ((parent.width/rootItemWidth)*99)
        y: ((parent.height/rootItemHeight)*6)
        opacity: isBackGroundBlack
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                isBackGroundBlack = true
            }
        }
    }

    // png text
    Rectangle {
        anchors.fill: settings_popup
        color: "transparent"

        Rectangle {
            width: parent.width
            height: parent.height/2 - 10
            color: "transparent"
            anchors.top: parent.top
            anchors.topMargin: 5

            Text {
                text: qsTr("Background Color")
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter            }
        }
        Rectangle {
            width: parent.width
            height: parent.height/2 - 10
            color: "transparent"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10

            Text {
                text: qsTr("Pellet Color")
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // ── Motor feed times (paper-advance duration in seconds) ─────────────
    Rectangle {
        id: motorSection
        anchors.top: settings_popup.bottom
        anchors.left: settings_popup.left
        width: settings_popup.width
        height: 96
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Text {
                text: qsTr("MOTOR FEED (SECONDS)")
                color: "#9a9ba0"; font.pixelSize: 9
                font.bold: true; font.letterSpacing: 1
            }
            Row {
                spacing: 8
                Column {
                    spacing: 2
                    Text { text: qsTr("Match"); color: "white"; font.pixelSize: 9 }
                    Rectangle {
                        width: 52; height: 20; radius: 3
                        color: "#15161a"; border.color: "#3a3b40"
                        TextInput {
                            id: motorMatchInput
                            anchors.fill: parent; anchors.margins: 3
                            color: "white"; font.pixelSize: 11
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            text: APPSETTINGS.getMotor_movement_time().toFixed(1)
                        }
                    }
                }
                Column {
                    spacing: 2
                    Text { text: qsTr("Sighter"); color: "white"; font.pixelSize: 9 }
                    Rectangle {
                        width: 52; height: 20; radius: 3
                        color: "#15161a"; border.color: "#3a3b40"
                        TextInput {
                            id: motorSighterInput
                            anchors.fill: parent; anchors.margins: 3
                            color: "white"; font.pixelSize: 11
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            text: APPSETTINGS.getMotor_movement_time_sighter().toFixed(1)
                        }
                    }
                }
            }
            Rectangle {
                width: 70; height: 20; radius: 3
                color: "#e8003d"
                Text {
                    id: motorSaveText
                    anchors.centerIn: parent
                    text: qsTr("Save")
                    color: "white"; font.pixelSize: 10; font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var m = parseFloat(motorMatchInput.text)
                        var s = parseFloat(motorSighterInput.text)
                        if (isNaN(m) || m <= 0 || m > 60) m = 2.5
                        if (isNaN(s) || s <= 0 || s > 60) s = 2.5
                        motorMatchInput.text = m.toFixed(1)
                        motorSighterInput.text = s.toFixed(1)
                        APPSETTINGS.saveMotorTimes(m, s)
                        motorSaveText.text = qsTr("Saved ✓")
                        motorSavedReset.restart()
                    }
                }
                Timer {
                    id: motorSavedReset
                    interval: 1500
                    onTriggered: motorSaveText.text = qsTr("Save")
                }
            }
        }
    }

    // ── About / build identity (F9B) — embedded at compile time; lets the
    // operator confirm the release executable matches the committed source.
    Rectangle {
        id: aboutSection
        anchors.top: motorSection.bottom
        anchors.left: settings_popup.left
        anchors.topMargin: 6
        width: settings_popup.width
        height: aboutCol.implicitHeight + 16
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1
        Column {
            id: aboutCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top; anchors.margins: 8; spacing: 3
            Text {
                text: qsTr("ABOUT / BUILD")
                color: "#9a9ba0"; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1
            }
            Text {
                text: "TechAim " + (typeof BUILDINFO !== "undefined" ? BUILDINFO.version : "?")
                color: "white"; font.pixelSize: 12; font.bold: true
            }
            Text {
                text: (typeof BUILDINFO !== "undefined" ? BUILDINFO.config : "?") + qsTr(" build")
                color: "#c9ced6"; font.pixelSize: 10
            }
            Text {
                text: qsTr("Commit: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.commit : "?")
                color: "#c9ced6"; font.pixelSize: 10
            }
            Text {
                text: qsTr("Built: ") + (typeof BUILDINFO !== "undefined" ? BUILDINFO.built : "?")
                color: "#c9ced6"; font.pixelSize: 10
            }
        }
    }

    function startFromServer()
    {

    }
}
