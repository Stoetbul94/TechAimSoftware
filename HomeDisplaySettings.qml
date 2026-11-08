import QtQuick 2.15

// UI-DEC-017 — Display settings for the HOME screens: theme and language.
//
// Theme switches theme.homeTokens only (athlete home + operator homepage);
// the match screen, HUDs and reports stay dark until they are migrated to
// tokens. Language uses the existing LanguageService and applies everywhere.
// Nothing here can reach scoring, rules or the target connection.
Rectangle {
    id: panel
    property var tok: theme.homeTokens
    // Inputs rather than direct reads, so the panel can be rendered as
    // evidence without the application's LanguageService.
    property var languages: typeof LANGUAGE !== "undefined" ? LANGUAGE.availableLanguages : []
    property string currentLanguage: typeof LANGUAGE !== "undefined" ? LANGUAGE.languageCode : "en"
    property bool betaTranslation: typeof LANGUAGE !== "undefined" && LANGUAGE.isBetaTranslation
    property bool restartRequired: typeof LANGUAGE !== "undefined" && LANGUAGE.restartRequired
    signal closeRequested()

    width: 340
    height: content.implicitHeight + 40
    radius: 12
    color: tok.surfacePrimary
    border.color: tok.borderStrong; border.width: 1

    // Swallow clicks so they do not fall through to the page underneath.
    MouseArea { anchors.fill: parent }

    Column {
        id: content
        x: 20; y: 20
        width: parent.width - 40
        spacing: 14

        Item {
            width: parent.width; height: 28
            Text {
                anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Display"); color: tok.textPrimary
                font.family: theme.fontFamily; font.pixelSize: 20; font.bold: true
            }
            Rectangle {
                objectName: "homeDisplayClose"
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                width: 44; height: 44; radius: 8
                color: closeMouse.pressed ? tok.surfaceElevated : "transparent"
                Text { anchors.centerIn: parent; text: "×"; color: tok.textSecondary; font.pixelSize: 26 }
                MouseArea { id: closeMouse; anchors.fill: parent; onClicked: panel.closeRequested() }
            }
        }

        Text {
            text: qsTr("THEME"); color: tok.textSecondary
            font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true; font.letterSpacing: 1
        }
        Row {
            width: parent.width; spacing: 8
            Repeater {
                model: [ { light: false, label: qsTr("Dark") }, { light: true, label: qsTr("Light") } ]
                Rectangle {
                    objectName: modelData.light ? "homeThemeLight" : "homeThemeDark"
                    property bool selected: theme.homeLight === modelData.light
                    width: (parent.width - 8) / 2; height: 52; radius: 8
                    color: selected ? tok.accentPrimary : tok.surfaceSecondary
                    border.color: selected ? tok.accentPrimary : tok.borderSubtle; border.width: 1
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: parent.selected ? tok.textOnAccent : tok.textPrimary
                        font.family: theme.fontFamily; font.pixelSize: 16; font.bold: parent.selected
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: theme.setHomeLight(modelData.light) }
                }
            }
        }

        Text {
            text: qsTr("LANGUAGE"); color: tok.textSecondary
            font.family: theme.fontFamily; font.pixelSize: 12; font.bold: true; font.letterSpacing: 1
        }
        Column {
            width: parent.width; spacing: 8
            Repeater {
                model: panel.languages
                Rectangle {
                    objectName: "homeLanguage_" + modelData.code
                    property bool selected: modelData.code === panel.currentLanguage
                    width: parent.width; height: 52; radius: 8
                    color: selected ? tok.accentPrimary : tok.surfaceSecondary
                    border.color: selected ? tok.accentPrimary : tok.borderSubtle; border.width: 1
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label; elide: Text.ElideRight
                        color: parent.selected ? tok.textOnAccent : tok.textPrimary
                        font.family: theme.fontFamily; font.pixelSize: 16; font.bold: parent.selected
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: LANGUAGE.selectLanguage(modelData.code) }
                }
            }
        }
        // The German catalogue is a beta: say so rather than implying a
        // reviewed translation (same wording as SettingsPage).
        Text {
            visible: panel.betaTranslation
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("German is a beta translation awaiting native review. "
                       + "Untranslated text stays in English.")
            color: tok.textSecondary; font.family: theme.fontFamily; font.pixelSize: 12
        }
        Text {
            visible: panel.restartRequired
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("Some text changes language after the application is restarted.")
            color: tok.warningText; font.family: theme.fontFamily; font.pixelSize: 12
        }
    }
}
