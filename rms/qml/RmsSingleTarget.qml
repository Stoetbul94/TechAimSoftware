import QtQuick 2.15

// SINGLE TARGET — one lane, large.
//
// The face is the point of the screen; everything else is arranged around it.
// Lane navigation sits with the target, so changing lanes never means going
// back through a menu.
Item {
    id: single

    property var lane: ({})
    // In full screen the shell already provides navigation, so the page-level
    // footer is suppressed rather than duplicated.
    property bool showNavigation: true
    signal previousLane()
    signal nextLane()
    signal allTargets()

    Theme { id: theme }

    function f(key, fallback) {
        return (lane && lane[key] !== undefined && lane[key] !== "")
               ? lane[key] : (fallback !== undefined ? fallback : "")
    }
    readonly property bool online: lane && lane.online === true
    readonly property bool terminal: lane && lane.competitionTerminal === true
    readonly property bool eliminated: lane && lane.eliminated === true
    readonly property int unseen: lane && lane.unseenShotCount ? lane.unseenShotCount : 0

    // ── heading ─────────────────────────────────────────────────────────
    Column {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: theme.spacingUnit * 2
        spacing: 4

        Row {
            spacing: theme.spacingUnit * 2
            Text {
                text: single.f("laneLabel", "—")
                font.family: theme.fontFamily
                font.pixelSize: 26
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: single.f("athlete", "—")
                font.family: theme.fontFamily
                font.pixelSize: 22
                color: theme.textSecondary
            }
        }
        Row {
            spacing: theme.spacingUnit * 2
            Text {
                text: single.f("programmeLabel", single.f("plannedProgramme", "—"))
                font.family: theme.fontFamily
                font.pixelSize: 14
                color: theme.textPrimary
            }
            Text {
                visible: single.f("phase", "").length > 0
                text: "·  " + single.f("phase", "")
                      + (single.f("position", "").length > 0
                             ? "  ·  " + single.f("position", "") : "")
                font.family: theme.fontFamily
                font.pixelSize: 14
                color: theme.textSecondary
            }
        }
        // The plan and the station disagree. Stated once, plainly; RMS did not
        // put that programme on the station and cannot change it.
        Text {
            visible: single.lane && single.lane.programmeMismatch === true
            width: parent.width
            wrapMode: Text.WordWrap
            text: "⚠ The station reports a different programme from the plan ("
                  + single.f("plannedProgramme", "—") + ")."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.brandAccent
        }
        Text {
            visible: single.lane && single.lane.athleteMismatch === true
            width: parent.width
            wrapMode: Text.WordWrap
            text: "⚠ The station reports a different athlete ("
                  + single.f("observedAthlete", "—") + ")."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.brandAccent
        }
    }

    // ── the face ────────────────────────────────────────────────────────
    Item {
        id: faceArea
        anchors {
            top: heading.bottom; bottom: navigation.top
            left: parent.left; right: figures.left
            margins: theme.spacingUnit * 2
        }

        RmsTargetView {
            id: target
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
            targetStandardId: single.f("targetStandardId", "")
            shots: (single.lane && single.lane.shots) ? single.lane.shots : []
            showLastShotLabel: false
            stale: single.f("hasDevice", false) && !single.online
        }

        // A terminal competition state is stated on a solid panel, not straight
        // onto the face: ring numbers reading through the words made the place
        // and the SIMULATED caveat hard to read, and a caveat that cannot be
        // read is not a caveat. The face stays visible around it — dimmed, not
        // deleted.
        Rectangle {
            anchors.fill: target
            visible: single.terminal
            radius: width / 2
            color: Qt.rgba(0, 0, 0, 0.72)
        }

        Rectangle {
            anchors.centerIn: target
            visible: single.terminal
            width: Math.min(target.width * 0.82, terminalCol.implicitWidth + theme.spacingUnit * 6)
            height: terminalCol.implicitHeight + theme.spacingUnit * 4
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.width: 1
            border.color: single.eliminated ? theme.brandAccent : theme.borderColor

            Column {
                id: terminalCol
                anchors.centerIn: parent
                width: parent.width - theme.spacingUnit * 4
                spacing: 8
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: single.f("athlete", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 22
                    color: theme.textSecondary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: single.f("competitionStatus", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 34
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2.0
                    color: single.eliminated ? theme.brandAccent : theme.textPrimary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: single.f("finalRankLabel", "").length > 0
                    text: single.f("finalRankLabel", "").toUpperCase() + " PLACE"
                    font.family: theme.fontFamily
                    font.pixelSize: 18
                    color: theme.textSecondary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: single.f("finalScoreLabel", "—") !== "—"
                    text: single.f("finalScoreLabel", "")
                    font.family: theme.fontFamily
                    font.pixelSize: 30
                    font.weight: Font.DemiBold
                    color: theme.textPrimary
                }
                // On its own strip, because this is the sentence that stops a
                // reader mistaking a development state for a result.
                Rectangle {
                    visible: single.lane && single.lane.competitionSimulated === true
                    width: parent.width
                    height: simText.implicitHeight + 10
                    radius: theme.radiusSmall
                    color: theme.brandAccent
                    Text {
                        id: simText
                        anchors.centerIn: parent
                        width: parent.width - 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "SIMULATED STATE — no station reported this"
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        font.letterSpacing: 0.8
                        color: theme.textOnBrand
                    }
                }
            }
        }
    }

    // ── figures ─────────────────────────────────────────────────────────
    Column {
        id: figures
        anchors {
            top: heading.bottom; right: parent.right; bottom: navigation.top
            margins: theme.spacingUnit * 2
        }
        width: 260
        spacing: theme.spacingUnit * 2

        Column {
            spacing: 2
            Text {
                text: "LAST SHOT"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }
            Row {
                spacing: theme.spacingUnit
                Text {
                    text: single.f("lastShotScore", "—")
                    font.family: theme.fontFamily
                    font.pixelSize: 40
                    font.weight: Font.DemiBold
                    color: theme.brandPrimary
                }
                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 8
                    visible: single.f("lastShotSequence", 0) > 0
                    text: "#" + single.f("lastShotSequence", 0)
                    font.family: theme.fontFamily
                    font.pixelSize: 14
                    color: theme.textSecondary
                }
            }
        }

        Column {
            spacing: 2
            Text {
                text: "SHOTS"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }
            Text {
                text: single.f("shotsLabel", "—")
                font.family: theme.fontFamily
                font.pixelSize: 30
                color: theme.textPrimary
            }
        }

        Column {
            spacing: 2
            Text {
                text: "TOTAL"
                font.family: theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                color: theme.textSecondary
            }
            Text {
                text: single.f("nodeTotalLabel", "—")
                font.family: theme.fontFamily
                font.pixelSize: 34
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }
            Text {
                text: "reported by the target station"
                font.family: theme.fontFamily
                font.pixelSize: 9
                color: theme.textSecondary
            }
        }

        // Only shown when it matters, and never allowed to look like a result.
        Rectangle {
            visible: single.unseen > 0
            width: parent.width
            height: unseenCol.height + theme.spacingUnit * 2
            radius: theme.radiusSmall
            color: Qt.rgba(0.75, 0.10, 0.10, 0.12)
            border.width: 1
            border.color: theme.brandAccent
            Column {
                id: unseenCol
                anchors.centerIn: parent
                width: parent.width - theme.spacingUnit * 2
                spacing: 3
                Text {
                    text: "⚠ " + single.unseen + " SHOTS NOT OBSERVED"
                    font.family: theme.fontFamily
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    color: theme.brandAccent
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: "This target shows the "
                          + single.f("observedShotCount", 0)
                          + " impacts RMS received. It is not the whole match — "
                          + "the station accepted "
                          + single.f("shotsAccepted", 0) + "."
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
                Text {
                    text: "Observed sum " + single.f("observedTotalLabel", "—")
                          + "  ·  station total " + single.f("nodeTotalLabel", "—")
                    font.family: theme.fontFamily
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
            }
        }

        Row {
            spacing: theme.spacingUnit
            RmsStatusPill {
                text: single.f("connection", "—")
                tone: !single.f("hasDevice", false) ? "neutral"
                    : single.online ? "live" : "offline"
            }
            RmsStatusPill {
                visible: single.f("competitionStatus", "UNKNOWN") !== "UNKNOWN"
                text: single.f("competitionStatus", "")
                tone: single.eliminated ? "warn" : "neutral"
            }
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: single.f("hasDevice", false) && !single.online
            text: single.f("statusText", "")
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.textSecondary
        }
    }

    // ── navigation ──────────────────────────────────────────────────────
    Row {
        id: navigation
        anchors { left: parent.left; bottom: parent.bottom }
        anchors.margins: theme.spacingUnit * 2
        visible: single.showNavigation
        height: visible ? 32 : 0
        spacing: theme.spacingUnit

        RmsButton { label: "◀ PREVIOUS"; onActivated: single.previousLane() }
        RmsButton { label: "ALL TARGETS"; onActivated: single.allTargets() }
        RmsButton { label: "NEXT ▶"; onActivated: single.nextLane() }
    }
}
