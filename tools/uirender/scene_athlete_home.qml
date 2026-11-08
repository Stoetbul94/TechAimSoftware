import QtQuick 2.15
import "../.."
import "../../src/ui/theme"
// Evidence scene for UI-DEC-016 / UI-DEC-017: the SETA athlete home, driven by
// the REAL CompetitionCatalogue and the REAL DesignTokens.
//   panel 1  dark theme, Live, target ready, name typed, 1.20 3x20 selected
//   panel 2  light theme, Live, target NOT connected, nothing chosen yet
//   panel 3  light theme, Demo, Display settings open
//   panel 4  dark theme, operator PIN pad open
// Render: UIRENDER_BRAND=SETA uirender.exe scene_athlete_home.qml out.png 1366 2950
Item {
    id: root
    width: 1366; height: 2950

    // Render with UIRENDER_BRAND=SETA so PRODUCT carries the SETA package.
    property bool ready: true

    // Stand-in for main.qml's Theme instance: just what the home screens read.
    QtObject {
        id: theme
        property string fontFamily: Qt.application.font.family
        property bool homeLight: false
        function setHomeLight(on) { homeLight = on }
        property DesignTokens homeTokens: DesignTokens { light: theme.homeLight }
    }
    DesignTokens { id: darkTok;  light: false }
    DesignTokens { id: lightTok; light: true }

    CompetitionCatalogue { id: cat }
    readonly property var ids: ["dsb.10m.air-rifle.lg40", "dsb.10m.air-rifle.lg60",
                                "dsb.10m.air-rifle.3x10", "dsb.10m.air-rifle.3x20",
                                "dsb.10m.air-pistol.lp40", "dsb.10m.air-pistol.lp60"]

    Loader {
        active: root.ready
        sourceComponent: Column {
            spacing: 18
            Rectangle { width: 1366; height: 0 }
            AthleteHome {
                width: 1366; height: 724
                tok: darkTok; catalogue: cat; programmeIds: root.ids
                liveMode: true; targetReady: true
                athleteName: "Max Mustermann"
                selectedId: "dsb.10m.air-rifle.3x20"
            }
            AthleteHome {
                width: 1366; height: 724
                tok: lightTok; catalogue: cat; programmeIds: root.ids
                liveMode: true; targetReady: false
            }
            AthleteHome {
                width: 1366; height: 724
                tok: darkTok; catalogue: cat; programmeIds: root.ids
                liveMode: true; targetReady: true; operatorPin: "2468"
                pinOpen: true
            }
            AthleteHome {
                id: third
                width: 1366; height: 724
                tok: lightTok; catalogue: cat; programmeIds: root.ids
                liveMode: false; athleteName: "Anna Schmidt"
                displayOpen: true
                Component.onCompleted: theme.homeLight = true
            }
        }
    }
}
