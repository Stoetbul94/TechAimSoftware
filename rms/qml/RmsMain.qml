import QtQuick 2.15
import QtQuick.Window 2.15

// TECH AIM RANGE MANAGEMENT SYSTEM — the application shell.
//
// Tech Aim branding only: the shared Theme, the Tech Aim wordmark, no SETA
// asset and no customer-specific product identity.
//
// Two states, and the shell picks between them: a machine with no range
// configured gets the first-run page, and everything else gets the navigation.
// Dropping an operator straight into a device monitor would teach them that a
// range is "the tablets that are awake", which is the misconception the whole
// milestone exists to remove.
//
// There is no control anywhere here. The only writes are RMS's own range
// configuration.
Window {
    id: root
    visible: true
    width: 1440
    height: 880
    minimumWidth: 1180
    minimumHeight: 720
    title: "Tech Aim — Range Management System"
    color: theme.bgBase

    Theme { id: theme }

    property string currentPage: "home"

    // A development hook so a capture can be taken of a named page without a
    // human clicking. Never reachable from the UI.
    Component.onCompleted: {
        if (typeof RMS_INITIAL_PAGE !== "undefined" && RMS_INITIAL_PAGE.length > 0)
            currentPage = RMS_INITIAL_PAGE
    }

    // ── header ──────────────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 72
        color: theme.bgSurface

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color: theme.borderColor
        }

        Image {
            id: logo
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: theme.spacingUnit * 3
            source: "qrc:/images/logo/techaim_white.png"
            fillMode: Image.PreserveAspectFit
            height: 32
            mipmap: true
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: logo.right
            anchors.leftMargin: theme.spacingUnit * 3
            spacing: 2
            Text {
                text: "RANGE MANAGEMENT SYSTEM"
                font.family: theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                color: theme.textPrimary
            }
            Text {
                text: "The target node remains authoritative. RMS observes and configures; it does not score and does not command."
                font.family: theme.fontFamily
                font.pixelSize: 11
                color: theme.textSecondary
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: theme.spacingUnit * 3
            spacing: theme.spacingUnit

            RmsStatusPill {
                visible: RMS_SIMULATED
                text: "SIMULATED RANGE"
                tone: "warn"
            }
            RmsStatusPill {
                text: "OBSERVING UDP " + RMS_OBSERVATION_PORT + "  ·  v" + RMS_PROTOCOL_VERSION
                tone: "neutral"
            }
            RmsStatusPill {
                text: "READ-ONLY OBSERVER  ·  NO CONTROL"
                tone: "neutral"
            }
        }
    }

    // ── first run ───────────────────────────────────────────────────────
    RmsFirstRunPage {
        anchors { top: header.bottom; left: parent.left
                  right: parent.right; bottom: parent.bottom }
        visible: !RANGECONFIG.configured
        onCreated: root.currentPage = "setup"
    }

    // ── configured: rail + page ─────────────────────────────────────────
    RmsNavRail {
        id: rail
        anchors { top: header.bottom; left: parent.left; bottom: parent.bottom }
        visible: RANGECONFIG.configured
        currentPage: root.currentPage
        onNavigate: function(page) { root.currentPage = page }
    }

    Loader {
        id: pageLoader
        anchors { top: header.bottom; left: rail.right
                  right: parent.right; bottom: parent.bottom }
        visible: RANGECONFIG.configured
        sourceComponent: root.currentPage === "live"     ? livePage
                       : root.currentPage === "setup"    ? setupPage
                       : root.currentPage === "displays" ? displaysPage
                       : root.currentPage === "athletes" ? athletesPage
                       : root.currentPage === "results"  ? resultsPage
                       : root.currentPage === "settings" ? settingsPage
                                                         : homePage
    }

    Component { id: homePage
        RmsHomePage { onNavigate: function(t) { root.currentPage = t } } }
    Component { id: livePage;     RmsLiveRangePage {} }
    Component { id: setupPage;    RmsRangeSetupPage {} }
    Component { id: displaysPage; RmsDisplaysPage {} }

    Component {
        id: athletesPage
        RmsPlaceholderPage {
            title: "ATHLETES"
            milestone: "NEXT MILESTONE — ATHLETE + SESSION + PROGRAMME ASSIGNMENT"
            summary: "An athlete register, and the assignment of an athlete and a "
                     + "programme to a lane for a session."
            notes: [
                "Not built. No athlete database exists in this build.",
                "Athlete names shown on the Live Range are OBSERVED from node "
                + "telemetry — they are what the station reports, not an RMS record.",
                "Assignment will still be configuration: RMS will record who is on "
                + "which lane, and will not tell the station anything."
            ]
        }
    }
    Component {
        id: resultsPage
        RmsPlaceholderPage {
            title: "RESULTS"
            milestone: "A LATER MILESTONE"
            summary: "Aggregated results and rankings across the range."
            notes: [
                "Not built.",
                "Every score will remain the node's own accepted score. RMS "
                + "aggregates; it never computes one."
            ]
        }
    }
    Component {
        id: settingsPage
        RmsPlaceholderPage {
            title: "SETTINGS"
            milestone: "A LATER MILESTONE"
            summary: "RMS application preferences — network, appearance and "
                     + "diagnostics options."
            notes: [
                "Not built.",
                "Range and lane configuration lives in RANGE SETUP, not here.",
                "Range configuration file: " + RANGECONFIG.configPath
            ]
        }
    }
}
