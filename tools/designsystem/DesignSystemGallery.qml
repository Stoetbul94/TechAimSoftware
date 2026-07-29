import QtQuick 2.15
import QtQuick.Window 2.15
// Relative, NOT qrc: — this file is run by the standalone `qml` runtime and
// is deliberately absent from qml.qrc, so it must resolve from the filesystem.
import "../../src/ui/theme"
import "../../src/ui/components"

// Tech Aim Design System — DEVELOPMENT-ONLY component gallery (UI-1).
//
// NOT part of the application. It is not in qml.qrc, it is not reachable from
// any operator or athlete navigation, and it does not ship. It exists so the
// token layer and the shared components can be looked at without launching a
// match.
//
// Run standalone:
//     qml tools/designsystem/DesignSystemGallery.qml
//
// It builds its own Theme instance, exactly the way main.qml does, so what it
// shows is what the application resolves.

Window {
    id: win
    visible: true
    width: 1180
    height: 900
    title: "Tech Aim Design System — DEVELOPMENT PREVIEW (not the application)"
    color: theme.tokens.backgroundPrimary

    // A local facade with the same shape the application's Theme exposes
    // (`theme.tokens` / `theme.type` / `theme.space`). The gallery does NOT
    // instantiate the root Theme.qml, because that file imports the compiled
    // resource path and would not resolve outside the application binary. The
    // token objects themselves are the real ones, so what is shown here is
    // exactly what the application resolves.
    QtObject {
        id: theme
        readonly property DesignTokens tokens: DesignTokens { }
        readonly property Typography   type:   Typography   { }
        readonly property Spacing      space:  Spacing      { }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.height + 48
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            x: 24; y: 24
            width: parent.width - 48
            spacing: theme.space.spacing24

            Text {
                text: "DEVELOPMENT PREVIEW — NOT THE APPLICATION"
                color: theme.tokens.warningText
                font.family: theme.type.label.family
                font.pixelSize: theme.type.label.size
                font.bold: true
                font.letterSpacing: theme.type.label.spacing
            }
            Text {
                text: "Tech Aim Design System"
                color: theme.tokens.textPrimary
                font.family: theme.type.displayTitle.family
                font.pixelSize: theme.type.displayTitle.size
                font.bold: true
            }

            // ── PALETTE ──────────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme; title: "Colour tokens"
                content: Flow {
                    width: parent.width; spacing: theme.space.spacing8
                    Repeater {
                        model: [
                            { n: "accentPrimary",       c: theme.tokens.accentPrimary },
                            { n: "accentHover",         c: theme.tokens.accentHover },
                            { n: "accentPressed",       c: theme.tokens.accentPressed },
                            { n: "accentSubtle",        c: theme.tokens.accentSubtle },
                            { n: "logo-only #BF1919",   c: theme.tokens.brandLogoSecondary },
                            { n: "backgroundPrimary",   c: theme.tokens.backgroundPrimary },
                            { n: "surfacePrimary",      c: theme.tokens.surfacePrimary },
                            { n: "surfaceSecondary",    c: theme.tokens.surfaceSecondary },
                            { n: "surfaceElevated",     c: theme.tokens.surfaceElevated },
                            { n: "inputBackground",     c: theme.tokens.inputBackground },
                            { n: "borderSubtle",        c: theme.tokens.borderSubtle },
                            { n: "borderStrong",        c: theme.tokens.borderStrong },
                            { n: "textPrimary",         c: theme.tokens.textPrimary },
                            { n: "textSecondary",       c: theme.tokens.textSecondary },
                            { n: "textDisabled",        c: theme.tokens.textDisabled },
                            { n: "successText",         c: theme.tokens.successText },
                            { n: "warningText",         c: theme.tokens.warningText },
                            { n: "errorText",           c: theme.tokens.errorText },
                            { n: "infoText",            c: theme.tokens.infoText },
                            { n: "focusOutline",        c: theme.tokens.focusOutline }
                        ]
                        delegate: Column {
                            width: 176; spacing: 4
                            Rectangle {
                                width: 176; height: 46
                                radius: theme.space.radiusSmall
                                color: modelData.c
                                border.width: 1; border.color: theme.tokens.borderSubtle
                            }
                            Text {
                                text: modelData.n
                                color: theme.tokens.textSecondary
                                font.family: theme.type.helperText.family
                                font.pixelSize: theme.type.helperText.size
                            }
                            Text {
                                text: String(modelData.c).toUpperCase()
                                color: theme.tokens.textDisabled
                                font.family: theme.type.numericMetric.family
                                font.pixelSize: 10
                            }
                        }
                    }
                }
            }

            // ── TYPOGRAPHY ───────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme; title: "Typography roles"
                content: Column {
                    width: parent.width; spacing: theme.space.spacing8
                    Repeater {
                        model: [
                            { n: "displayTitle",  r: theme.type.displayTitle },
                            { n: "pageTitle",     r: theme.type.pageTitle },
                            { n: "sectionTitle",  r: theme.type.sectionTitle },
                            { n: "cardTitle",     r: theme.type.cardTitle },
                            { n: "body",          r: theme.type.body },
                            { n: "bodyStrong",    r: theme.type.bodyStrong },
                            { n: "helperText",    r: theme.type.helperText },
                            { n: "label",         r: theme.type.label },
                            { n: "buttonText",    r: theme.type.buttonText },
                            { n: "numericMetric", r: theme.type.numericMetric },
                            { n: "statusText",    r: theme.type.statusText }
                        ]
                        delegate: Row {
                            spacing: theme.space.spacing16
                            Text {
                                width: 150
                                text: modelData.n
                                color: theme.tokens.textDisabled
                                font.family: theme.type.helperText.family
                                font.pixelSize: theme.type.helperText.size
                            }
                            Text {
                                text: modelData.n === "numericMetric"
                                      ? "10.9  9.8  10.4  —  598"
                                      : "10m Air Pistol — 60 Shots"
                                color: theme.tokens.textPrimary
                                font.family:        modelData.r.family
                                font.pixelSize:     modelData.r.size
                                font.bold:          modelData.r.bold
                                font.letterSpacing: modelData.r.spacing
                            }
                        }
                    }
                }
            }

            // ── BUTTONS ──────────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme
                title: "Buttons — default / disabled (hover, press and Tab focus are live)"
                content: Row {
                    spacing: theme.space.spacing12
                    TaButton { theme: theme; variant: TaButton.Primary;   text: "Start session" }
                    TaButton { theme: theme; variant: TaButton.Secondary; text: "Load saved session" }
                    TaButton { theme: theme; variant: TaButton.Ghost;     text: "Settings" }
                    TaButton { theme: theme; variant: TaButton.Primary;   text: "Disabled"; enabled: false }
                }
            }

            // ── STATUS CHIPS ─────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme; title: "Status chips"
                content: Row {
                    spacing: theme.space.spacing12
                    TaStatusChip { theme: theme; kind: TaStatusChip.Success; text: "Connected" }
                    TaStatusChip { theme: theme; kind: TaStatusChip.Neutral; text: "Demo · not needed" }
                    TaStatusChip { theme: theme; kind: TaStatusChip.Warning; text: "Restart required" }
                    TaStatusChip { theme: theme; kind: TaStatusChip.Error;   text: "Not connected" }
                    TaStatusChip { theme: theme; kind: TaStatusChip.Success; text: "Tappable (44px)"; interactive: true }
                }
            }

            // ── BANNERS ──────────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme; title: "Warning and error banners"
                content: Column {
                    width: parent.width; spacing: theme.space.spacing8
                    TaWarningBanner {
                        width: parent.width; theme: theme; kind: TaWarningBanner.Warning
                        text: "Changing mode requires an application restart."
                    }
                    TaWarningBanner {
                        width: parent.width; theme: theme; kind: TaWarningBanner.Error
                        text: "The target hardware is not responding. Check the target connection and try again."
                    }
                    TaWarningBanner {
                        width: parent.width; theme: theme; kind: TaWarningBanner.Info
                        text: "Demo sessions are for testing and cannot be treated as Live target results."
                    }
                    // German expansion sample — see Screen_Layout_Rules.md §6.
                    TaWarningBanner {
                        width: parent.width; theme: theme; kind: TaWarningBanner.Warning
                        text: "Betriebsart · Netzwerkfreigabe — Das Ändern der Betriebsart erfordert einen Neustart der Anwendung."
                    }
                }
            }

            // ── EVENT CARDS ──────────────────────────────────────────────
            GallerySection {
                width: parent.width; th: theme; title: "Event cards — rest and selected"
                content: Column {
                    width: parent.width; spacing: theme.space.spacing8
                    Repeater {
                        model: [
                            { b: "60",  t: "10m Air Pistol — 60 Shots", s: "ISSF 2026 · 60 shots · 75 min", sel: true  },
                            { b: "F24", t: "10m Air Pistol — FINAL (24)", s: "ISSF 10m Final · decimal · on command", sel: false },
                            { b: "TL",  t: "Training Lab", s: "Technical Blocks · Shot calling · Group analysis", sel: false }
                        ]
                        delegate: Rectangle {
                            width: parent.width
                            height: theme.space.cardHeight
                            radius: theme.space.radiusMedium
                            color: modelData.sel ? theme.tokens.accentSubtle : theme.tokens.surfaceSecondary
                            border.width: modelData.sel ? theme.space.borderSelected : theme.space.borderThin
                            border.color: modelData.sel ? theme.tokens.accentPrimary : theme.tokens.borderSubtle
                            Row {
                                anchors.left: parent.left; anchors.leftMargin: theme.space.spacing12
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: theme.space.spacing12
                                Rectangle {
                                    width: 38; height: 38; radius: 19
                                    color: modelData.sel ? theme.tokens.accentPrimary : theme.tokens.borderStrong
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        anchors.centerIn: parent; text: modelData.b
                                        color: theme.tokens.textOnAccent
                                        font.family: theme.type.numericMetric.family
                                        font.pixelSize: 12; font.bold: true
                                    }
                                }
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter; spacing: 4
                                    Text {
                                        text: modelData.t
                                        color: modelData.sel ? theme.tokens.textPrimary : theme.tokens.textSecondary
                                        font.family: theme.type.cardTitle.family
                                        font.pixelSize: theme.type.cardTitle.size
                                        font.bold: modelData.sel
                                    }
                                    Text {
                                        text: modelData.s
                                        color: theme.tokens.textDisabled
                                        font.family: theme.type.helperText.family
                                        font.pixelSize: theme.type.helperText.size
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: theme.space.spacing24 }
        }
    }

    // Small local section wrapper — keeps the gallery readable without adding
    // a component to the shipping set.
    component GallerySection: Rectangle {
        property var    th: null
        property string title: ""
        property alias  content: holder.data

        implicitHeight: header.height + holder.childrenRect.height + 3 * th.space.spacing16
        height: implicitHeight
        color: th.tokens.surfacePrimary
        radius: th.space.radiusLarge
        border.width: th.space.borderThin
        border.color: th.tokens.borderSubtle

        Text {
            id: header
            anchors.top: parent.top; anchors.topMargin: th.space.spacing16
            anchors.left: parent.left; anchors.leftMargin: th.space.panelPadding
            text: parent.title
            color: th.tokens.textPrimary
            font.family: th.type.sectionTitle.family
            font.pixelSize: th.type.sectionTitle.size
            font.bold: true
        }
        Item {
            id: holder
            anchors.top: header.bottom; anchors.topMargin: th.space.spacing16
            anchors.left: parent.left;  anchors.leftMargin: th.space.panelPadding
            anchors.right: parent.right; anchors.rightMargin: th.space.panelPadding
            height: childrenRect.height
        }
    }
}
