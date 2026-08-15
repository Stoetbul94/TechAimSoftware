import QtQuick
import QtQuick.Layouts

// ─────────────────────────────────────────────────────────────────────────────
// Target Connection status — the ONE shared component for target state.
//
// Every discipline consumes this. It is not reimplemented per screen, because
// the acquisition engine underneath is shared: Air Rifle, Air Pistol, Practice,
// Match, Finals and Training Lab all run the same TachusWidget acquisition.
//
// It binds to MODREADER's authoritative properties. It does NOT read settings,
// does NOT cache, and does NOT infer. The connection area previously showed a
// remembered "COM7" for a port that did not exist on the machine; binding to
// the live state is what prevents that.
//
// The device name is whatever Windows reports for the adapter ACTUALLY
// selected. Nothing here is specific to a CH340 - any USB-serial target is
// shown by its own description.
//
// WHY THIS EXISTS AT ALL: on 2026-08-09 the USB cable was unplugged mid-session
// and the shooting screen carried on looking healthy while acquisition was
// dead. The operator's next shot was lost with no warning. A target state the
// software knows but never shows is worth nothing to the person shooting.
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root

    // Raw state string from the engine. Bound, never assigned locally.
    readonly property string state:  MODREADER ? MODREADER.targetState  : ""
    readonly property string device: MODREADER ? MODREADER.targetDevice : ""
    readonly property string port:   MODREADER ? MODREADER.targetPort   : ""
    readonly property string detail: MODREADER ? MODREADER.targetDetail : ""

    // READY is a strong claim, decided in C++: identified target, open
    // transport, valid Modbus AND a synchronized acquisition baseline.
    readonly property bool   ready:  MODREADER ? MODREADER.targetReady : false

    // States that must stop the operator. ACQUISITION FAULT is latched in the
    // engine and is not cleared by a later normal-looking poll.
    readonly property bool   isFault: state === "ACQUISITION FAULT"
    readonly property bool   isDown:  state === "TARGET DISCONNECTED"
                                   || state === "TARGET NOT CONNECTED"
                                   || state === "TARGET NOT DETECTED"
    readonly property bool   isBusy:  state === "SCANNING"
                                   || state === "RECONNECTING"
                                   || state === "SYNCHRONIZING"
                                   || state === "TARGET DETECTED"

    // True whenever the operator must not be shooting. Screens can bind to this
    // to avoid presenting a healthy face over a dead or faulted target.
    // RECONNECTING also blocks: the cable is out and no shot can be acquired.
    // Treating it as merely "busy" would leave a compact strip on screen while
    // the athlete's shots were going nowhere.
    readonly property bool   blocksShooting: isFault || isDown
                                          || state === "RECONNECTING"

    // Compact mode collapses to a single header-height strip. The full panel
    // overlapped the LAST SHOT box on the shooting screen, hiding live scoring
    // information - unacceptable for a component whose whole purpose is to stop
    // the operator being misled. When something is WRONG it expands, because
    // then it should dominate.
    property bool compact: false
    readonly property bool expanded: !compact || blocksShooting

    implicitWidth: 260
    implicitHeight: expanded ? col.implicitHeight + theme.space.spacing12 * 2
                             : compactRow.implicitHeight + 10

    // Operator-facing wording. Deliberately plain: someone on a range at
    // distance reads a state, not a protocol term.
    readonly property string headline: {
        if (isFault)                       return qsTr("TARGET ACQUISITION ERROR")
        if (state === "TARGET DISCONNECTED") return qsTr("TARGET DISCONNECTED")
        if (state === "RECONNECTING")      return qsTr("RECONNECTING…")
        if (state === "SYNCHRONIZING")     return qsTr("SYNCHRONIZING…")
        if (state === "SCANNING")          return qsTr("SEARCHING FOR TARGET…")
        if (state === "TARGET DETECTED")   return qsTr("TARGET FOUND")
        if (state === "MANUAL SELECTION REQUIRED") return qsTr("SELECT TARGET PORT")
        if (isDown)                        return qsTr("NO TARGET")
        if (ready)                         return qsTr("READY")
        return qsTr("CONNECTING…")
    }

    readonly property color accent: {
        if (isFault) return theme.statusError
        if (isDown)  return theme.statusError
        if (isBusy)  return theme.brandAccent
        if (ready)   return theme.statusConnected
        return theme.statusDisconnected
    }

    Rectangle {
        anchors.fill: parent
        radius: theme.space.radiusSmall
        color: theme.bgSurface
        border.width: root.blocksShooting ? 2 : 1
        border.color: root.blocksShooting ? root.accent : theme.borderColor

        // A fault must not be possible to overlook at a glance.
        SequentialAnimation on opacity {
            running: root.isFault
            loops: Animation.Infinite
            NumberAnimation { to: 0.55; duration: 620 }
            NumberAnimation { to: 1.00; duration: 620 }
        }
    }

    // Single-line form, used when the panel must not cover live scoring.
    RowLayout {
        id: compactRow
        visible: !root.expanded
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 7

        Rectangle {
            width: 9; height: 9; radius: 4.5
            color: root.accent
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: root.headline
            color: theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }
        Text {
            text: {
                if (root.device !== "" && root.port !== "")
                    return root.device + " · " + root.port
                return root.device !== "" ? root.device : root.port
            }
            visible: text !== ""
            color: theme.textSecondary
            font.pixelSize: 11
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        id: col
        visible: root.expanded
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: theme.space.spacing12
        spacing: 4

        Text {
            text: qsTr("Target Connection")
            color: theme.textSecondary
            font.pixelSize: 11
            font.letterSpacing: 1
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            Rectangle {
                width: 10; height: 10; radius: 5
                color: root.accent
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: root.headline
                color: root.blocksShooting ? root.accent : theme.textPrimary
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // Device and port. Hidden entirely when there is nothing connected -
        // showing a remembered port for a device that is gone is worse than
        // showing nothing.
        Text {
            visible: root.device !== "" || root.port !== ""
            text: {
                if (root.device !== "" && root.port !== "")
                    return root.device + " · " + root.port
                return root.device !== "" ? root.device : root.port
            }
            color: theme.textSecondary
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        // The instruction the operator must act on.
        Text {
            visible: text !== ""
            text: {
                if (root.isFault)
                    return qsTr("Target and software shot counters are out of "
                              + "synchronization.\nSTOP SHOOTING. Check the target "
                              + "connection before continuing.")
                if (root.state === "TARGET DISCONNECTED" || root.state === "RECONNECTING")
                    return qsTr("Connection to the electronic target has been lost.\n"
                              + "Please reconnect the target USB cable.")
                if (root.state === "MANUAL SELECTION REQUIRED")
                    return qsTr("Several devices could be the target. Choose the port.")
                if (root.isDown)
                    return qsTr("Connect the target USB cable.")
                return ""
            }
            color: root.accent
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
