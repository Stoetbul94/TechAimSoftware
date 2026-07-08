import QtQuick 2.15

// Tech Aim Report System — per-series card.
// White rounded card, subtle shadow/border, a header strip with the series
// title + a right-aligned chip row (score, inner-10, MPI, group), and a body
// slot that the caller fills (target plot + shot table). Purely presentational
// so it renders identically in every report and stays print-safe. The caller
// keeps ownership of the data (chips are passed in), so no calculation lives here.
Item {
    id: card
    property string seriesTitle: "Series"
    property var    chips: []            // array of { l, v, unit, kind }  kind: "" | "bad"
    property color  accent: "#a80038"    // Tech Aim maroon
    default property alias content: body.data

    implicitHeight: 340

    // subtle shadow
    Rectangle {
        anchors.fill: face
        anchors.topMargin: 3
        radius: face.radius
        color: "#12000000"
    }
    Rectangle {
        id: face
        anchors.fill: parent
        radius: 12
        color: "#ffffff"
        border.color: "#e6e8ec"
        border.width: 1

        // ── Header strip ──────────────────────────────────────────────────
        Item {
            id: head
            anchors.top: parent.top
            width: parent.width
            height: 42

            Rectangle {
                width: 4; radius: 2; height: 22; color: card.accent
                anchors.left: parent.left; anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: card.seriesTitle
                anchors.left: parent.left; anchors.leftMargin: 28
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 15; font.bold: true; color: "#191b1f"; font.family: "Segoe UI"
            }
            Row {
                anchors.right: parent.right; anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 16
                Repeater {
                    model: card.chips
                    delegate: Row {
                        spacing: 4
                        Text {
                            text: modelData.l + ":"
                            color: "#8a8f98"; font.pixelSize: 10; font.family: "Segoe UI"
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 1
                        }
                        Text {
                            text: modelData.v
                            color: modelData.kind === "bad" ? "#bf1919" : "#191b1f"
                            font.pixelSize: 13; font.bold: true; font.family: "Segoe UI"
                        }
                        Text {
                            visible: modelData.unit !== undefined && String(modelData.unit).length > 0
                            text: modelData.unit ? modelData.unit : ""
                            color: "#b3b8c0"; font.pixelSize: 9; font.family: "Segoe UI"
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 2
                        }
                    }
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#eceef1" }
        }

        // ── Body slot (caller supplies target + shot table) ───────────────
        Item {
            id: body
            anchors.top: head.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 14
        }
    }
}
