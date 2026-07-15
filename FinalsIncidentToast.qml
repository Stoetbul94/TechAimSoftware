import QtQuick 2.15

// 3P FINAL HUD — transient incident toast. Listens to the controller's
// structured shotRejected events, slides down from under the top strip,
// holds briefly, fades away. Severity is a small edge bar only — no colour
// flashing, no modal, no permanent layout shift. Presentation only.
Rectangle {
    id: toast
    property var ctl

    readonly property var messages: ({
        "WindowClosed": "SHOT OUTSIDE FIRING WINDOW",
        "StageShotLimitReached": "STAGE SHOT LIMIT REACHED — SHOT NOT COUNTED",
        "DuplicateShot": "DUPLICATE SHOT IGNORED",
        "WrongTargetMode": "WRONG TARGET MODE — SHOT NOT COUNTED",
        "InvalidShotData": "INVALID SHOT DATA",
        "FinalsNotActive": "FINAL NOT ACTIVE — SHOT IGNORED"
    })
    // Hard rejections (score integrity) red; procedural ones amber.
    readonly property var severe: ({ "StageShotLimitReached": true, "InvalidShotData": true })

    property string message: ""
    property color edgeColor: "#e8b93d"

    function show(reason) {
        message = messages[reason] !== undefined ? messages[reason] : "SHOT REJECTED — " + reason
        edgeColor = severe[reason] === true ? "#d0392b" : "#e8b93d"
        anim.stop()
        anim.restart()
    }

    Connections {
        target: toast.ctl
        function onShotRejected(rej) {
            // Athlete-facing wording wins (e.g. "KNEELING COMPLETE - CHANGE TO
            // PRONE"); the diagnostic reason maps stay for everything else.
            if (rej.displayText !== undefined) {
                toast.message = rej.displayText
                toast.edgeColor = "#3ddc84"   // normal-completion guidance, not an error
                anim.stop(); anim.restart()
            } else {
                toast.show(rej.reason)
            }
        }
    }

    width: msgTxt.implicitWidth + 34
    height: 26; radius: 6
    color: "#d914151a"
    border.color: "#33353d"; border.width: 1
    opacity: 0
    visible: opacity > 0

    Rectangle {   // severity edge marker
        anchors.left: parent.left; anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        width: 4; height: parent.height - 10; radius: 2
        color: toast.edgeColor
    }
    Text {
        id: msgTxt
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: 4
        text: toast.message
        color: "#d7d8dd"; font.family: "Segoe UI"; font.pixelSize: 11; font.bold: true
        font.letterSpacing: 0.5
    }

    transform: Translate { id: slide; y: -8 }
    SequentialAnimation {
        id: anim
        ParallelAnimation {
            NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 150; easing.type: Easing.OutQuad }
            NumberAnimation { target: slide; property: "y"; from: -8; to: 0; duration: 150; easing.type: Easing.OutQuad }
        }
        PauseAnimation { duration: 2600 }
        NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 400; easing.type: Easing.InQuad }
    }
}
