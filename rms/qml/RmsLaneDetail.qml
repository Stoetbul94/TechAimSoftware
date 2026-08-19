import QtQuick 2.15

// The selected lane, in depth — and the home of the engineering detail that
// used to sit on the main list.
//
// Milestone 1's diagnostics are all still here, none removed: node id, boot
// id, session id, programme/ruleset/target-standard ids, observed vs accepted,
// duplicates suppressed, out-of-order arrivals, sequence gaps, restarts and
// offline episodes. They are simply one click away instead of in front of
// someone running a relay.
Rectangle {
    id: detail

    property var info: ({})
    property var shots: []
    property bool showDiagnostics: false

    Theme { id: theme }

    color: theme.bgSurface
    border.width: 1
    border.color: theme.borderColor
    radius: theme.radiusMedium
    clip: true

    function field(key, fallback) {
        return (info && info[key] !== undefined && info[key] !== "")
               ? info[key] : (fallback !== undefined ? fallback : "—")
    }
    readonly property bool hasDevice: info && info.hasDevice === true
    readonly property bool observed: info && info.observed === true

    Flickable {
        anchors.fill: parent
        anchors.margins: theme.spacingUnit * 2.5
        contentWidth: width
        contentHeight: body.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: body
            width: parent.width
            spacing: theme.spacingUnit * 2

            // ── header ──────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 4
                Text {
                    text: detail.field("laneLabel", "No lane selected")
                    font.family: theme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    color: theme.textPrimary
                }
                Text {
                    text: detail.field("athlete", detail.hasDevice ? "—" : "")
                    font.family: theme.fontFamily
                    font.pixelSize: 14
                    color: theme.textSecondary
                }
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: detail.field("programmeLabel", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 13
                    color: theme.textPrimary
                }
            }

            Row {
                spacing: theme.spacingUnit
                RmsStatusPill {
                    text: detail.field("connection", "NO DEVICE")
                    tone: !detail.hasDevice ? "neutral"
                        : (detail.info && detail.info.offline) ? "offline"
                        : detail.field("connection") === "TARGET_CONNECTED" ? "live"
                        : "neutral"
                }
                RmsStatusPill {
                    visible: detail.field("phase", "").length > 0
                             && detail.field("phase") !== "—"
                    text: detail.field("phase", "")
                    tone: detail.field("phase") === "MATCH" ? "live" : "neutral"
                }
            }

            // ── THREE SEPARATE STATUSES ─────────────────────────────────
            // Node health, target health and where the athlete is in the
            // competition are different questions. An eliminated finalist
            // normally has a perfectly healthy station, and a dead tablet is
            // not an elimination.
            Rectangle {
                width: parent.width
                height: compCol.height + theme.spacingUnit * 2
                visible: detail.info && detail.info.competitionTerminal === true
                radius: theme.radiusSmall
                color: detail.info && detail.info.eliminated === true
                       ? Qt.rgba(0.75, 0.10, 0.10, 0.12) : theme.bgBase
                border.width: 1
                border.color: detail.info && detail.info.eliminated === true
                              ? theme.brandAccent : theme.borderColor

                Column {
                    id: compCol
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: theme.spacingUnit
                    spacing: 4

                    Text {
                        text: "COMPETITION STATUS"
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 1.2
                        color: theme.textSecondary
                    }
                    Text {
                        text: detail.field("competitionStatus", "UNKNOWN")
                        font.family: theme.fontFamily
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: detail.info && detail.info.eliminated === true
                               ? theme.brandAccent : theme.textPrimary
                    }
                    Row {
                        spacing: theme.spacingUnit * 3
                        visible: detail.field("finalRankLabel", "").length > 0
                                 || detail.field("finalScoreLabel", "—") !== "—"
                        Column {
                            spacing: 1
                            Text {
                                text: detail.field("finalRankLabel", "—")
                                font.family: theme.fontFamily
                                font.pixelSize: 17
                                color: theme.textPrimary
                            }
                            Text {
                                text: "final rank"
                                font.family: theme.fontFamily
                                font.pixelSize: 10
                                color: theme.textSecondary
                            }
                        }
                        Column {
                            spacing: 1
                            Text {
                                text: detail.field("finalScoreLabel", "—")
                                font.family: theme.fontFamily
                                font.pixelSize: 17
                                color: theme.textPrimary
                            }
                            Text {
                                // The node's score, as always. RMS adds up a
                                // final no more than it adds up a qualification.
                                text: "final score (node)"
                                font.family: theme.fontFamily
                                font.pixelSize: 10
                                color: theme.textSecondary
                            }
                        }
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "The station remains "
                              + detail.field("connection", "—")
                              + ". Node health, target health and competition "
                              + "status are separate: this lane is not faulty."
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        visible: detail.info && detail.info.competitionSimulated === true
                        text: "⚠ SIMULATED STATE — injected by a development tool. "
                              + "Protocol v1 carries no competition status, so no real "
                              + "station has reported this."
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        color: theme.brandAccent
                    }
                }
            }

            // ── PLANNED, kept beside OBSERVED and never merged into it ──
            // RMS has told the station nothing, so these two may legitimately
            // differ. Showing both is the point; picking one would be a lie
            // about which side moved.
            Rectangle {
                width: parent.width
                height: plannedCol.height + theme.spacingUnit * 2
                visible: detail.info && detail.info.inPlan === true
                radius: theme.radiusSmall
                color: detail.info && detail.info.programmeMismatch === true
                       ? Qt.rgba(0.75, 0.10, 0.10, 0.10) : theme.bgBase
                border.width: 1
                border.color: detail.info && detail.info.programmeMismatch === true
                              ? theme.brandAccent : theme.borderColor

                Column {
                    id: plannedCol
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: theme.spacingUnit
                    spacing: 3
                    Text {
                        text: "PLANNED"
                        font.family: theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 1.2
                        color: theme.textSecondary
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: detail.field("plannedAthlete", "no athlete assigned")
                        font.family: theme.fontFamily
                        font.pixelSize: 13
                        color: theme.textPrimary
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: detail.field("plannedProgramme", "")
                        font.family: theme.fontFamily
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        visible: detail.info && detail.info.programmeMismatch === true
                        text: "⚠ The station reports \"" + detail.field("programmeLabel", "—")
                              + "\". RMS did not put it there and cannot change it."
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        color: theme.brandAccent
                    }
                }
            }

            // A lane with no device is a configuration state, not a fault —
            // say which it is rather than showing an empty match panel.
            Text {
                visible: !detail.hasDevice && detail.info && detail.info.laneNumber !== undefined
                width: parent.width
                wrapMode: Text.WordWrap
                text: "No device is assigned to this lane. Assign one in Range Setup."
                font.family: theme.fontFamily
                font.pixelSize: 12
                color: theme.textSecondary
            }

            Rectangle {
                width: parent.width; height: 1; color: theme.borderColor
                visible: detail.observed
            }

            // ── latest shot ─────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
                visible: detail.observed
                Text {
                    text: "LATEST ACCEPTED SHOT"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Row {
                    spacing: theme.spacingUnit * 3
                    Column {
                        spacing: 2
                        Text {
                            text: "#" + detail.field("latestSequence", "0")
                            font.family: theme.fontFamily
                            font.pixelSize: 26
                            color: theme.textSecondary
                        }
                        Text {
                            text: "sequence"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                    Column {
                        spacing: 2
                        Text {
                            text: detail.field("latestScore", "—")
                            font.family: theme.fontFamily
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                            color: theme.brandPrimary
                        }
                        Text {
                            text: "node score"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                    Column {
                        spacing: 2
                        Text {
                            text: detail.field("scoreLabel", "—")
                            font.family: theme.fontFamily
                            font.pixelSize: 26
                            color: theme.textPrimary
                        }
                        Text {
                            text: "running total ("
                                  + detail.field("shotsAccepted", "0") + " shots)"
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width; height: 1; color: theme.borderColor
                visible: detail.observed
            }

            Text {
                visible: detail.observed
                text: "RECENT SHOTS  ·  newest first"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }

            Column {
                width: parent.width
                spacing: 0
                visible: detail.observed
                Repeater {
                    model: detail.shots
                    delegate: Item {
                        width: parent.width
                        height: 24
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 0; width: 46
                            text: "#" + modelData.sequence
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                            color: theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 52; width: 60
                            text: modelData.score
                            font.family: theme.fontFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: modelData.innerTen ? theme.brandPrimary
                                                      : theme.textPrimary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 118
                            text: "x " + modelData.x + "   y " + modelData.y
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            text: modelData.status
                            font.family: theme.fontFamily
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
                Text {
                    visible: detail.shots.length === 0
                    text: "No accepted shot observed on this lane yet."
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.textSecondary
                }
            }

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            // ── diagnostics ─────────────────────────────────────────────
            Row {
                spacing: theme.spacingUnit
                Text {
                    text: "DIAGNOSTICS"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                }
                Text {
                    text: detail.showDiagnostics ? "▾ hide" : "▸ show"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    color: theme.brandPrimary
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detail.showDiagnostics = !detail.showDiagnostics
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 3
                visible: detail.showDiagnostics

                Repeater {
                    model: [
                        { k: "lane id",          v: detail.field("laneId") },
                        { k: "assigned device",  v: detail.field("assignedNodeId") },
                        { k: "node id",          v: detail.field("nodeId") },
                        { k: "boot id",          v: detail.field("bootId") },
                        { k: "session id",       v: detail.field("sessionId") },
                        { k: "programme id",     v: detail.field("programmeId") },
                        { k: "ruleset id",       v: detail.field("rulesetId") },
                        { k: "target standard",  v: detail.field("targetStandardId") },
                        { k: "device",           v: detail.field("deviceIdentity") },
                        { k: "app version",      v: detail.field("appVersion") },
                        { k: "shots observed",   v: detail.field("observedShots", "0")
                             + " of " + detail.field("shotsAccepted", "0")
                             + " accepted by node" },
                        { k: "unobserved",       v: detail.field("unobserved", "0") },
                        { k: "duplicates suppressed",
                          v: detail.field("duplicatesSuppressed", "0") },
                        { k: "arrived out of order", v: detail.field("outOfOrder", "0") },
                        { k: "sequence conflicts",   v: detail.field("sequenceConflicts", "0") },
                        { k: "sequence gaps",    v: detail.field("gapCount", "0")
                             + (detail.field("gapList", "") !== "—"
                                ? "  [" + detail.field("gapList", "") + "]" : "") },
                        { k: "node restarts",    v: detail.field("nodeRestarts", "0") },
                        { k: "offline episodes", v: detail.field("offlineEpisodes", "0") },
                        { k: "stale status dropped", v: detail.field("staleStatusDropped", "0") },
                        { k: "stale boot dropped",   v: detail.field("staleBootDropped", "0") }
                    ]
                    delegate: Row {
                        width: parent.width
                        spacing: 8
                        Text {
                            width: 150
                            text: modelData.k
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                        Text {
                            width: parent.width - 158
                            text: modelData.v
                            font.family: theme.fontFamily
                            font.pixelSize: 11
                            color: theme.textPrimary
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "Observation only — this build cannot start, stop, reset, "
                      + "sight, match, change position, end, feed or recover the "
                      + "target node's match."
                font.family: theme.fontFamily
                font.pixelSize: 10
                color: theme.textSecondary
                bottomPadding: theme.spacingUnit
            }
        }
    }
}
