import QtQuick 2.15

// ─────────────────────────────────────────────────────────────────────────────
// TechAim dialog — THE one modal dialog visual for the whole application.
//
// Replaces every native QMessageBox / QtQuick.Dialogs MessageDialog: dark
// TechAim card (matches FloatingWindow/ReportWindow chrome), rounded corners,
// soft shadow, accent-coloured SVG icon per type, optional expandable details,
// right-aligned buttons, fade+scale animation, full keyboard support
// (Enter = default button, Esc = cancel when allowed). Purely visual — it
// carries no application logic; callers receive the pressed button's result
// string via finished().
//
// Drive it through TechAimDialogManager (id: dialogManager in main.qml);
// pages should not instantiate this directly.
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 5000

    // ── configuration (set by the manager before open()) ────────────────
    property string type: "info"     // info | warning | error | success | confirm | question | custom
    property string title: ""
    property string message: ""
    property string details: ""      // optional; hidden behind "Show Details"
    // [{ label, result, accent }] — accent: filled accent button (default action)
    property var buttons: [{ label: "OK", result: "ok", accent: true }]
    property string defaultResult: "ok"   // Enter activates this result ("" = none)
    property string cancelResult: "ok"    // Esc / scrim produce this ("" = Esc disabled)
    property int autoDismissMs: 0         // > 0: auto-close with defaultResult

    signal finished(string result)

    // ── design tokens (TechAim dark chrome) ─────────────────────────────
    readonly property color cardColor: "#1f2026"
    readonly property color cardBorder: "#3a3b42"
    readonly property color inkPrimary: "#f2f3f5"
    readonly property color inkSecondary: "#b6b9c0"
    readonly property color inkMuted: "#8a8f98"
    readonly property var accentByType: ({
        "info":     "#2f6fd0",
        "warning":  "#c77700",
        "error":    "#d0392b",
        "success":  "#1f8a4c",
        "confirm":  "#a80038",
        "question": "#2f6fd0",
        "custom":   "#a80038"
    })
    readonly property color accent: accentByType[type] !== undefined
                                    ? accentByType[type] : "#a80038"

    // Clean SVG glyphs (white, drawn on the accent disc). Never OS icons.
    readonly property var iconByType: ({
        "info":     "M11 10h2v7h-2zm0-3h2v2h-2z",
        "warning":  "M11 9h2v5h-2zm0 7h2v2h-2z",
        "error":    "M17 8.4 15.6 7 12 10.6 8.4 7 7 8.4 10.6 12 7 15.6 8.4 17 12 13.4 15.6 17 17 15.6 13.4 12z",
        "success":  "M10 15.2 6.8 12l-1.4 1.4L10 18l9-9-1.4-1.4z",
        "confirm":  "M10 15.2 6.8 12l-1.4 1.4L10 18l9-9-1.4-1.4z",
        "question": "M11.1 14.1c0-1.1.4-1.9 1.4-2.8.8-.8 1.3-1.3 1.3-2.1 0-.9-.7-1.5-1.8-1.5-1 0-1.7.5-2.2 1.4L8 8.1C8.8 6.8 10.2 6 12.1 6c2.3 0 3.9 1.3 3.9 3.1 0 1.3-.6 2.1-1.6 3-.9.8-1.2 1.3-1.2 2.2v.3h-2.1v-.5zM11 16.5h2.2v2.2H11z"
    })
    function iconSource() {
        var p = iconByType[type] !== undefined ? iconByType[type] : iconByType["info"]
        return "data:image/svg+xml;utf8," +
               "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>" +
               "<path fill='white' d='" + p + "'/></svg>"
    }

    property bool detailsOpen: false

    // ── lifecycle ────────────────────────────────────────────────────────
    function open() {
        detailsOpen = false
        root.visible = true
        hideAnim.stop()
        showAnim.restart()
        card.forceActiveFocus()
        if (autoDismissMs > 0) autoTimer.restart()
        else autoTimer.stop()
    }
    function closeWith(result) {
        if (!root.visible) return
        autoTimer.stop()
        hideAnim.resultPending = result
        showAnim.stop()
        hideAnim.restart()
    }

    Timer {
        id: autoTimer
        interval: root.autoDismissMs > 0 ? root.autoDismissMs : 1
        onTriggered: root.closeWith(root.defaultResult)
    }

    // ── modal scrim (~40 %, swallows input) ─────────────────────────────
    Rectangle {
        id: scrim
        anchors.fill: parent
        color: "#000000"
        opacity: 0
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onWheel: function (w) { w.accepted = true }
        onClicked: if (root.cancelResult.length > 0) root.closeWith(root.cancelResult)
    }

    // ── card ─────────────────────────────────────────────────────────────
    // Soft shadow: two offset translucent layers (same technique as the
    // report cards — no GraphicalEffects dependency, works everywhere).
    Rectangle {
        anchors.fill: card; anchors.topMargin: 6; anchors.leftMargin: 2; anchors.rightMargin: -2
        radius: card.radius + 2; color: "#33000000"
        opacity: card.opacity; scale: card.scale
    }
    Rectangle {
        anchors.fill: card; anchors.topMargin: 3
        radius: card.radius + 1; color: "#4d000000"
        opacity: card.opacity; scale: card.scale
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 480
        height: col.height + 44
        radius: 13
        color: root.cardColor
        border.color: root.cardBorder
        border.width: 1
        opacity: 0
        scale: 0.95
        focus: true

        // top accent hairline under the rounded corners
        Rectangle {
            anchors.top: parent.top; anchors.topMargin: 1
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 26; height: 2; radius: 1
            color: root.accent
        }

        Keys.onEscapePressed: if (root.cancelResult.length > 0) root.closeWith(root.cancelResult)
        Keys.onReturnPressed: if (root.defaultResult.length > 0) root.closeWith(root.defaultResult)
        Keys.onEnterPressed:  if (root.defaultResult.length > 0) root.closeWith(root.defaultResult)
        MouseArea { anchors.fill: parent }   // keep card clicks off the scrim

        Column {
            id: col
            x: 22; y: 22
            width: parent.width - 44
            spacing: 14

            // icon + title
            Row {
                width: parent.width
                spacing: 12
                Rectangle {
                    width: 34; height: 34; radius: 17
                    color: root.accent
                    anchors.verticalCenter: parent.verticalCenter
                    Image {
                        anchors.centerIn: parent
                        source: root.iconSource()
                        width: 20; height: 20
                        sourceSize.width: 40; sourceSize.height: 40   // hi-DPI crisp
                        smooth: true
                    }
                }
                Text {
                    width: parent.width - 46
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    color: root.inkPrimary
                    font.family: "Segoe UI"; font.pixelSize: 17; font.bold: true
                    wrapMode: Text.WordWrap
                }
            }

            // message
            Text {
                width: parent.width
                text: root.message
                color: root.inkSecondary
                font.family: "Segoe UI"; font.pixelSize: 13
                lineHeight: 1.25
                wrapMode: Text.WordWrap
                visible: root.message.length > 0
            }

            // optional details (collapsed by default)
            Column {
                width: parent.width
                spacing: 6
                visible: root.details.length > 0
                Text {
                    text: (root.detailsOpen ? "Hide Details ▲" : "Show Details ▼")
                    color: root.inkMuted
                    font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.detailsOpen = !root.detailsOpen
                    }
                }
                Rectangle {
                    width: parent.width
                    height: root.detailsOpen ? detailsText.implicitHeight + 20 : 0
                    radius: 8
                    color: "#17181c"
                    border.color: "#2c2d34"; border.width: root.detailsOpen ? 1 : 0
                    clip: true
                    Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    Text {
                        id: detailsText
                        x: 10; y: 10; width: parent.width - 20
                        text: root.details
                        color: root.inkMuted
                        font.family: "Consolas"; font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }

            // buttons — right-aligned; the accent button is the default action
            Row {
                anchors.right: parent.right
                spacing: 8
                Repeater {
                    model: root.buttons
                    delegate: Rectangle {
                        width: btnLabel.implicitWidth + 34
                        height: 32
                        radius: 8
                        color: modelData.accent === true
                               ? (btnMA.pressed ? Qt.darker(root.accent, 1.25) : root.accent)
                               : (btnMA.pressed ? "#2a2b30" : (btnMA.containsMouse ? "#2c2d33" : "#26272c"))
                        border.color: modelData.accent === true ? "transparent" : "#3a3b42"
                        border.width: 1
                        Text {
                            id: btnLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            color: modelData.accent === true ? "white" : "#d7d8dd"
                            font.family: "Segoe UI"; font.pixelSize: 12
                            font.bold: modelData.accent === true
                        }
                        MouseArea {
                            id: btnMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.closeWith(modelData.result)
                        }
                    }
                }
            }
        }
    }

    // ── fade + scale (0.95 -> 1.0, ~200 ms) ─────────────────────────────
    ParallelAnimation {
        id: showAnim
        NumberAnimation { target: card;  property: "opacity"; to: 1;   duration: 200; easing.type: Easing.OutCubic }
        NumberAnimation { target: card;  property: "scale";   to: 1.0; duration: 200; easing.type: Easing.OutCubic }
        NumberAnimation { target: scrim; property: "opacity"; to: 0.4; duration: 200; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
        id: hideAnim
        property string resultPending: ""
        ParallelAnimation {
            NumberAnimation { target: card;  property: "opacity"; to: 0;    duration: 180; easing.type: Easing.InCubic }
            NumberAnimation { target: card;  property: "scale";   to: 0.95; duration: 180; easing.type: Easing.InCubic }
            NumberAnimation { target: scrim; property: "opacity"; to: 0;    duration: 180; easing.type: Easing.InCubic }
        }
        ScriptAction {
            script: {
                root.visible = false
                root.finished(hideAnim.resultPending)
            }
        }
    }
}
