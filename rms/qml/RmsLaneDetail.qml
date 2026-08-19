import QtQuick 2.15

// The live shot view for one selected lane. A management overview, not a
// second shooting UI: no target face, no ring geometry, no scoring. It shows
// what the node reported and how completely RMS received it.
//
// The whole pane scrolls as one Flickable. Fixed-height panes silently
// truncate the observation-quality block on a short window, and the numbers
// that admit a gap in what RMS saw are exactly the ones that must not be the
// first thing to disappear.
Rectangle {
    id: detail

    property var info: ({})          // RANGE.nodeDetail(row)
    property var shots: []           // RANGE.recentShots(row)

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
                    text: detail.field("athlete", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 14
                    color: theme.textSecondary
                }
                Text {
                    text: detail.field("programmeLabel", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 13
                    color: theme.textPrimary
                    width: parent.width
                    elide: Text.ElideRight
                }
            }

            Row {
                spacing: theme.spacingUnit
                RmsStatusPill {
                    text: detail.field("connection", "UNKNOWN")
                    tone: detail.info && detail.info.offline ? "offline"
                        : detail.field("connection") === "TARGET_CONNECTED" ? "live"
                        : "neutral"
                }
                RmsStatusPill {
                    text: detail.field("phase", "UNKNOWN")
                    tone: detail.field("phase") === "MATCH" ? "live" : "neutral"
                }
            }

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            // ── latest shot ─────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 6
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

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            // ── recent shots ────────────────────────────────────────────
            Text {
                text: "RECENT SHOTS  ·  newest first"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }

            Column {
                width: parent.width
                spacing: 0
                Repeater {
                    model: detail.shots
                    delegate: Item {
                        width: parent.width
                        height: 24
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 0
                            width: 46
                            text: "#" + modelData.sequence
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                            color: theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 52
                            width: 60
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

            // ── identity and observation quality ────────────────────────
            //
            // One key per row with a fixed label column: two columns cropped
            // the values, and a cropped "shots observed by RMS" is precisely
            // the fact that must never be lost.
            Column {
                width: parent.width
                spacing: 3

                Text {
                    text: "NODE IDENTITY AND OBSERVATION QUALITY"
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.2
                    color: theme.textSecondary
                    bottomPadding: 4
                }

                Repeater {
                    model: [
                        { k: "node id",         v: detail.field("nodeId") },
                        { k: "boot id",         v: detail.field("bootId") },
                        { k: "session id",      v: detail.field("sessionId") },
                        { k: "programme id",    v: detail.field("programmeId") },
                        { k: "ruleset id",      v: detail.field("rulesetId") },
                        { k: "target standard", v: detail.field("targetStandardId") },
                        { k: "device",          v: detail.field("deviceIdentity") },
                        { k: "shots observed",  v: detail.field("observedShots", "0")
                             + " of " + detail.field("shotsAccepted", "0")
                             + " accepted by node" },
                        { k: "unobserved",      v: detail.field("unobserved", "0") },
                        { k: "duplicates suppressed",
                          v: detail.field("duplicatesSuppressed", "0") },
                        { k: "arrived out of order", v: detail.field("outOfOrder", "0") },
                        { k: "sequence conflicts",   v: detail.field("sequenceConflicts", "0") },
                        { k: "sequence gaps",   v: detail.field("gapCount", "0")
                             + (detail.field("gapList", "") !== "—"
                                ? "  [" + detail.field("gapList", "") + "]" : "") },
                        { k: "node restarts",   v: detail.field("nodeRestarts", "0") },
                        { k: "offline episodes", v: detail.field("offlineEpisodes", "0") }
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

            Rectangle { width: parent.width; height: 1; color: theme.borderColor }

            // The boundary, restated where an operator would look for a
            // control and find none.
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
