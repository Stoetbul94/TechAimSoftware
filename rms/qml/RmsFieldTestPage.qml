import QtQuick 2.15

// FIELD TEST — the instrument panel for a range day.
//
// Everything on this page is RMS-LOCAL. Starting a log, exporting evidence and
// running a preflight send nothing to any station: if a target is mid-match,
// nothing here can interrupt it.
//
// The engineering numbers live HERE and not on Home or Displays, so an
// operator watching a range is not reading packet counters. The one exception
// is the unseen-shot warning, which stays on the lane where it matters.
Item {
    id: page

    Theme { id: theme }

    property string section: "preflight"

    // ── section tabs ────────────────────────────────────────────────────
    Rectangle {
        id: tabs
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 1.5
        height: 44
        radius: theme.radiusMedium
        color: theme.bgSurface
        border.width: 1
        border.color: theme.borderColor

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: theme.spacingUnit * 1.5
            spacing: theme.spacingUnit

            Repeater {
                model: [
                    { id: "preflight", label: "PREFLIGHT" },
                    { id: "network",   label: "NETWORK" },
                    { id: "counters",  label: "TELEMETRY" },
                    { id: "timeline",  label: "TIMELINE" }
                ]
                delegate: Rectangle {
                    width: 118; height: 28; radius: theme.radiusSmall
                    readonly property bool chosen: page.section === modelData.id
                    color: chosen ? Qt.rgba(0.66, 0, 0.22, 0.20) : theme.bgBase
                    border.width: 1
                    border.color: chosen ? theme.brandPrimary : theme.borderColor
                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 0.8
                        color: theme.textPrimary
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.section = modelData.id
                    }
                }
            }
        }

        // ── the log, and its state, always visible ──────────────────────
        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: theme.spacingUnit * 1.5
            spacing: theme.spacingUnit

            RmsStatusPill {
                visible: FIELDLOG.active
                text: "● LOGGING  " + FIELDLOG.elapsedLabel
                      + "  ·  " + FIELDLOG.eventCount + " events"
                tone: "live"
            }
            RmsButton {
                label: FIELDLOG.active ? "STOP FIELD TEST LOG" : "START FIELD TEST LOG"
                primary: !FIELDLOG.active
                onActivated: {
                    if (FIELDLOG.active) {
                        FIELDLOG.stop()
                    } else {
                        FIELDLOG.start(nameField.value, rangeField.value,
                                       operatorField.value, notesField.value,
                                       RMS_MODE)
                        FIELDTEST.noteLogStarted()
                    }
                }
            }
            RmsButton {
                label: "EXPORT FIELD TEST"
                enabled: FIELDLOG.eventCount > 0 || FIELDLOG.active
                onActivated: FIELDTEST.exportFieldTest()
            }
        }
    }

    // ── the body ────────────────────────────────────────────────────────
    Flickable {
        id: body
        anchors { top: tabs.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        anchors.margins: theme.spacingUnit * 1.5
        clip: true
        contentHeight: content.height
        visible: page.section !== "timeline"

        Column {
            id: content
            width: body.width
            spacing: theme.spacingUnit * 1.5

            // ── export result ───────────────────────────────────────────
            Rectangle {
                width: parent.width
                height: exportCol.height + theme.spacingUnit * 2
                visible: FIELDTEST.lastExportPath.length > 0
                         || FIELDTEST.lastExportError.length > 0
                radius: theme.radiusMedium
                color: FIELDTEST.lastExportError.length > 0
                       ? Qt.rgba(0.75, 0.10, 0.10, 0.12)
                       : Qt.rgba(0.18, 0.62, 0.36, 0.12)
                border.width: 1
                border.color: FIELDTEST.lastExportError.length > 0
                              ? theme.brandAccent : theme.statusConnected
                Column {
                    id: exportCol
                    anchors.centerIn: parent
                    width: parent.width - theme.spacingUnit * 2
                    spacing: 3
                    Text {
                        text: FIELDTEST.lastExportError.length > 0
                              ? "EXPORT FAILED" : "EVIDENCE BUNDLE WRITTEN"
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        font.letterSpacing: 0.8
                        color: FIELDTEST.lastExportError.length > 0
                               ? theme.brandAccent : theme.statusConnected
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WrapAnywhere
                        text: FIELDTEST.lastExportError.length > 0
                              ? FIELDTEST.lastExportError : FIELDTEST.lastExportPath
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        color: theme.textSecondary
                    }
                }
            }

            // ── PREFLIGHT ───────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: theme.spacingUnit
                visible: page.section === "preflight"

                Text {
                    text: FIELDTEST.preflightVerdict
                    font.family: theme.fontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: FIELDTEST.preflightVerdict.indexOf("FAILED") >= 0
                           ? theme.brandAccent : theme.textPrimary
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    // NOT "range ready". RMS cannot command or certify a
                    // station, so it has no business declaring one fit.
                    text: "This checks RMS's own observation path. It does not "
                          + "certify a target station, and RMS cannot command one."
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.textSecondary
                }

                Repeater {
                    model: FIELDTEST.preflight
                    delegate: Rectangle {
                        width: content.width
                        height: 46
                        radius: theme.radiusSmall
                        color: theme.bgSurface
                        border.width: 1
                        border.color: modelData.state === "FAIL" ? theme.brandAccent
                                                                 : theme.borderColor

                        Rectangle {
                            anchors { left: parent.left; top: parent.top
                                      bottom: parent.bottom }
                            width: 4
                            radius: 2
                            color: modelData.state === "PASS"    ? theme.statusConnected
                                 : modelData.state === "FAIL"    ? theme.brandAccent
                                 : modelData.state === "WARNING" ? "#c98a2b"
                                                                 : theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: theme.spacingUnit * 1.5
                            width: 200
                            elide: Text.ElideRight
                            text: modelData.label
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                            color: theme.textPrimary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 220
                            width: 72
                            text: modelData.state
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            font.letterSpacing: 1.0
                            color: modelData.state === "PASS"    ? theme.statusConnected
                                 : modelData.state === "FAIL"    ? theme.brandAccent
                                 : modelData.state === "WARNING" ? "#c98a2b"
                                                                 : theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 300
                            anchors.right: parent.right
                            anchors.rightMargin: theme.spacingUnit
                            elide: Text.ElideRight
                            text: modelData.detail
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }
                }

                // ── test metadata ───────────────────────────────────────
                Text {
                    text: "FIELD-TEST LOG DETAILS"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Row {
                    spacing: theme.spacingUnit
                    RmsFieldInput { id: nameField;     label: "TEST NAME"; placeholder: "RMS Multi-Lane Field Test 1"; width: 260 }
                    RmsFieldInput { id: rangeField;    label: "RANGE";     placeholder: "Potchefstroom"; width: 180 }
                    RmsFieldInput { id: operatorField; label: "OPERATOR";  placeholder: "Arnold"; width: 160 }
                }
                RmsFieldInput { id: notesField; label: "NOTES"; placeholder: "optional"; width: content.width * 0.7 }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    visible: FIELDLOG.logPath.length > 0
                    text: "Log file: " + FIELDLOG.logPath
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    visible: FIELDLOG.lastError.length > 0
                    text: "⚠ " + FIELDLOG.lastError
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.brandAccent
                }
            }

            // ── NETWORK ─────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: theme.spacingUnit
                visible: page.section === "network"

                // The bind failure, stated where nobody can miss it.
                Rectangle {
                    width: parent.width
                    height: bindCol.height + theme.spacingUnit * 2
                    visible: NETDIAG.live && !NETDIAG.listening
                    radius: theme.radiusMedium
                    color: Qt.rgba(0.75, 0.10, 0.10, 0.14)
                    border.width: 1
                    border.color: theme.brandAccent
                    Column {
                        id: bindCol
                        anchors.centerIn: parent
                        width: parent.width - theme.spacingUnit * 2
                        spacing: 4
                        Text {
                            text: "⚠  RMS CANNOT OBSERVE TARGET STATIONS"
                            font.family: theme.fontFamily
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: theme.brandAccent
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "UDP " + NETDIAG.port + " could not be opened. "
                                  + "An empty range below is this fault, not quiet tablets."
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            color: theme.textPrimary
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            visible: NETDIAG.listenerError.length > 0
                            text: "Socket error: " + NETDIAG.listenerError
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "Likely causes: another application already owns the "
                                  + "port; Windows Firewall has not been allowed; the "
                                  + "adapter is down."
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }

                RmsKeyValue { label: "RMS mode";   value: NETDIAG.mode }
                RmsKeyValue { label: "UDP listener";
                              value: NETDIAG.live ? (NETDIAG.listening ? "LISTENING" : "ERROR")
                                                  : "NOT OBSERVING (demo)" }
                RmsKeyValue { label: "Port";       value: String(NETDIAG.port) }
                RmsKeyValue { label: "Host name";  value: NETDIAG.hostName }
                RmsKeyValue { label: "Protocol";   value: "v" + RMS_PROTOCOL_VERSION }

                Text {
                    text: "LOCAL NETWORK INTERFACES"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Repeater {
                    model: NETDIAG.interfaces()
                    delegate: RmsKeyValue {
                        label: modelData.name + (modelData.isWireless ? "  (Wi-Fi)" : "");
                        value: modelData.address + "   mask " + modelData.netmask
                    }
                }
                Text {
                    visible: NETDIAG.interfaces().length === 0
                    text: "No usable IPv4 interface found."
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.brandAccent
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: "If no stations appear: confirm the tablets and this PC are "
                          + "on the SAME network; confirm the access point does not "
                          + "have client isolation enabled; confirm Windows Firewall "
                          + "allows TechAimRMS.exe on a private network; confirm the "
                          + "target application is running. RMS does not change any "
                          + "firewall or network setting — that is a decision for a person."
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.textSecondary
                }
            }

            // ── TELEMETRY COUNTERS ──────────────────────────────────────
            Column {
                width: parent.width
                spacing: theme.spacingUnit
                visible: page.section === "counters"

                Text {
                    text: "OBSERVATION QUALITY"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                RmsKeyValue { label: "Shots accepted by stations";
                              value: String(FIELDTEST.counters.shotsAcceptedByNodes) }
                RmsKeyValue { label: "Shots observed by RMS";
                              value: String(FIELDTEST.counters.shotsObserved) }
                RmsKeyValue {
                    label: "UNSEEN by RMS";
                    value: String(FIELDTEST.counters.shotsUnseen);
                    alert: FIELDTEST.counters.shotsUnseen > 0
                }
                RmsKeyValue { label: "Duplicates suppressed";
                              value: String(FIELDTEST.counters.duplicatesSuppressed) }
                RmsKeyValue { label: "Out of order accepted";
                              value: String(FIELDTEST.counters.outOfOrder) }
                RmsKeyValue { label: "Sequence conflicts";
                              value: String(FIELDTEST.counters.sequenceConflicts);
                              alert: FIELDTEST.counters.sequenceConflicts > 0 }
                RmsKeyValue { label: "Node restarts";
                              value: String(FIELDTEST.counters.nodeRestarts) }
                RmsKeyValue { label: "Offline episodes";
                              value: String(FIELDTEST.counters.offlineEpisodes) }

                Text {
                    text: "STATIONS"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                RmsKeyValue { label: "Physical lanes configured";
                              value: String(FIELDTEST.counters.lanesConfigured) }
                RmsKeyValue { label: "Lanes commissioned";
                              value: String(FIELDTEST.counters.lanesAssigned) }
                RmsKeyValue { label: "Stations discovered";
                              value: String(FIELDTEST.counters.nodesDiscovered) }
                RmsKeyValue { label: "Stations online";
                              value: String(FIELDTEST.counters.nodesOnline) }
                RmsKeyValue { label: "Stations offline";
                              value: String(FIELDTEST.counters.nodesOffline);
                              alert: FIELDTEST.counters.nodesOffline > 0 }
                RmsKeyValue { label: "Unassigned devices";
                              value: String(FIELDTEST.counters.unassignedDevices) }

                Text {
                    text: "PACKETS"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                RmsKeyValue { label: "Accepted";        value: String(FIELDTEST.counters.packetsAccepted) }
                RmsKeyValue { label: "Announces";       value: String(FIELDTEST.counters.announces) }
                RmsKeyValue { label: "Status messages"; value: String(FIELDTEST.counters.statuses) }
                RmsKeyValue { label: "Shot messages";   value: String(FIELDTEST.counters.shotMessages) }
                RmsKeyValue { label: "Rejected";        value: String(FIELDTEST.counters.packetsRejected);
                              alert: FIELDTEST.counters.packetsRejected > 0 }
                RmsKeyValue { label: "  malformed";        value: String(FIELDTEST.counters.malformed) }
                RmsKeyValue { label: "  unknown version";  value: String(FIELDTEST.counters.unknownVersion) }
                RmsKeyValue { label: "  unknown type";     value: String(FIELDTEST.counters.unknownType) }
                RmsKeyValue { label: "Last packet";     value: FIELDTEST.counters.lastPacketAge }
            }
        }
    }

    // ── TIMELINE ────────────────────────────────────────────────────────
    Item {
        anchors { top: tabs.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        anchors.margins: theme.spacingUnit * 1.5
        visible: page.section === "timeline"

        Text {
            id: timelineNote
            anchors { top: parent.top; left: parent.left; right: parent.right }
            wrapMode: Text.WordWrap
            text: FIELDLOG.active
                  ? ("Recording to " + FIELDLOG.logPath
                     + "   ·   the view shows the most recent events; the file keeps all of them.")
                  : "No field-test log is running. Press START FIELD TEST LOG on the Preflight tab."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.textSecondary
        }

        ListView {
            anchors { top: timelineNote.bottom; left: parent.left
                      right: parent.right; bottom: parent.bottom }
            anchors.topMargin: theme.spacingUnit
            clip: true
            model: FIELDLOG
            spacing: 2

            delegate: Rectangle {
                width: ListView.view.width
                height: 30
                radius: theme.radiusSmall
                color: index % 2 === 0 ? theme.bgSurface : "transparent"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: theme.spacingUnit
                    width: 92
                    text: model.at
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 92 + theme.spacingUnit
                    width: 190
                    elide: Text.ElideRight
                    text: model.eventType
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 0.6
                    color: model.eventType.indexOf("OFFLINE") >= 0
                           || model.eventType.indexOf("ERROR") >= 0
                           || model.eventType.indexOf("UNSEEN") >= 0
                           || model.eventType.indexOf("CONFLICT") >= 0
                           ? theme.brandAccent : theme.textPrimary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 292 + theme.spacingUnit
                    anchors.right: parent.right
                    anchors.rightMargin: theme.spacingUnit
                    elide: Text.ElideRight
                    text: model.summary
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    color: theme.textPrimary
                }
            }
        }
    }
}
