import QtQuick 2.15

// MATCH REVIEW — two separate questions, answered separately.
//
//   IS THE PLAN COMPLETE?   did the operator fill it in?
//   IS THE RANGE READY?     are the stations answering with live targets?
//
// And a third that this build cannot answer at all: HAS A TARGET LOADED THE
// MATCH? RMS has no command channel, so LOAD RANGE is shown disabled and
// labelled rather than omitted — an operator needs to know the step exists and
// that it is not available, not be left wondering where it went.
Item {
    id: review

    Theme { id: theme }

    property var r: PLANS.readiness()

    Connections {
        target: PLANS
        function onPlanChanged() { review.r = PLANS.readiness() }
    }
    // Node health moves without the plan changing, so the review refreshes on
    // observation too.
    Connections {
        target: PLANLANES
        function onChanged() { review.r = PLANS.readiness() }
    }

    function num(key) { return r && r[key] !== undefined ? r[key] : 0 }

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        spacing: theme.spacingUnit * 2

        Text {
            text: "MATCH REVIEW"
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            color: theme.textSecondary
        }

        Grid {
            columns: 2
            rowSpacing: 6
            columnSpacing: theme.spacingUnit * 3
            Repeater {
                model: [
                    { k: "Programme", v: PLANS.programmeLabel.length > 0
                                          ? PLANS.programmeLabel : "— not chosen —" },
                    { k: "Range",     v: RANGECONFIG.rangeName },
                    { k: "Lanes",     v: review.num("selectedLanes") + " selected" },
                    { k: "Athletes",  v: review.num("athletesAssigned") + " / "
                                          + review.num("selectedLanes") + " assigned" },
                    { k: "Stations",  v: review.num("nodesOnline") + " online, "
                                          + review.num("nodesOffline") + " offline" },
                    { k: "Targets",   v: review.num("targetsConnected") + " connected, "
                                          + review.num("targetsUnavailable") + " unavailable" }
                ]
                delegate: Row {
                    spacing: theme.spacingUnit
                    Text {
                        width: 110
                        text: modelData.k
                        font.family: theme.fontFamily
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                    Text {
                        text: modelData.v
                        font.family: theme.fontFamily
                        font.pixelSize: 13
                        color: theme.textPrimary
                    }
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: theme.borderColor }

        Text {
            text: "STATUS"
            font.family: theme.fontFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            color: theme.textSecondary
        }

        Row {
            spacing: theme.spacingUnit
            RmsStatusPill {
                text: review.r && review.r.planComplete ? "✓ PLAN COMPLETE"
                                                        : "⚠ PLAN INCOMPLETE"
                tone: review.r && review.r.planComplete ? "live" : "warn"
            }
            RmsStatusPill {
                text: review.r && review.r.rangeReady ? "✓ RANGE READY"
                                                      : "⚠ RANGE NOT FULLY READY"
                tone: review.r && review.r.rangeReady ? "live" : "warn"
            }
        }

        // The claim RMS must never make.
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "TARGET MATCH LOADED: no. RMS cannot load a match onto a target "
                  + "station — no command channel exists. The status above is this "
                  + "plan's readiness, not any station's."
            font.family: theme.fontFamily
            font.pixelSize: 11
            color: theme.textSecondary
        }

        Column {
            width: parent.width
            spacing: 3
            Repeater {
                model: review.r ? review.r.issues : []
                delegate: Text {
                    text: "⚠  Lane " + modelData.laneNumber + " — "
                          + modelData.reason.toLowerCase()
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                    color: theme.brandAccent
                }
            }
        }

        Text {
            visible: review.num("programmeMismatches") > 0
            text: "⚠  " + review.num("programmeMismatches")
                  + " station(s) report a different programme from this plan."
            font.family: theme.fontFamily
            font.pixelSize: 12
            color: theme.brandAccent
        }

        Rectangle { width: parent.width; height: 1; color: theme.borderColor }

        Row {
            spacing: theme.spacingUnit
            RmsButton {
                label: "LOAD RANGE"
                enabled: false
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Available when range control is enabled — a later milestone."
                font.family: theme.fontFamily
                font.pixelSize: 11
                color: theme.textSecondary
            }
        }
    }
}
