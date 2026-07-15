import QtQuick 2.15

// 3P FINAL HUD — Layer 3: transient CRO command overlay. Driven ONLY by the
// controller's structured commandIssued events; this component manages display
// animation and nothing else. Large type, subtle dark backing, fade-in → hold
// (~2 s; longer for RESULTS ARE FINAL) → fade-out. Repeated events restart the
// animation cleanly on the single overlay instance — no stacking.
Rectangle {
    id: overlay
    property var ctl
    property real maxWidth: 480

    // Concise instruction forms (the persistent strip owns stage identity;
    // long CRO sentences are not shown permanently — spec §2).
    readonly property var shortForms: ({
        "LoadSeries": "LOAD", "LoadSingle": "LOAD",
        "StartSeries": "START", "StartSingle": "START",
        "MatchFiringStart": "START",
        "Stop": "STOP", "Unload": "UNLOAD",
        "ResultsFinal": "RESULTS ARE FINAL",
        "FiveMinutes": "FIVE MINUTES", "ThirtySeconds": "THIRTY SECONDS",
        "AthletesToLine": "ATHLETES TO THE LINE",
        "TakeYourPositions": "TAKE YOUR POSITIONS",
        "PreparationSightingStart": "PREPARATION & SIGHTING — START",
        "StageOneAnnouncement": "22 MINUTES — KNEELING & PRONE"
    })

    property bool isNotice: false

    function present(ev) {
        var t = ev.typeName
        isNotice = (t === "InfoNotice")
        cmdText.text = isNotice ? ev.text : (shortForms[t] !== undefined ? shortForms[t] : ev.text)
        anim.stop()
        anim.holdTime = t === "ResultsFinal" ? 4500 : isNotice ? 3000 : 2000
        anim.restart()
    }

    Connections {
        target: overlay.ctl
        function onCommandIssued(ev) { overlay.present(ev) }
    }

    width: Math.min(maxWidth, cmdText.implicitWidth + 56)
    height: cmdText.implicitHeight + 24
    radius: 10
    color: "#c614151a"
    border.color: "#33353d"; border.width: 1
    opacity: 0
    visible: opacity > 0

    Text {
        id: cmdText
        anchors.centerIn: parent
        width: overlay.maxWidth - 56
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: overlay.isNotice ? "#c8c9cf" : "white"
        font.family: "Segoe UI"
        font.bold: !overlay.isNotice
        font.letterSpacing: overlay.isNotice ? 0.5 : 3
        font.pixelSize: overlay.isNotice ? 15 : (text.length > 22 ? 22 : 34)
    }

    SequentialAnimation {
        id: anim
        property int holdTime: 2000
        NumberAnimation { target: overlay; property: "opacity"; to: 1; duration: 150; easing.type: Easing.OutQuad }
        PauseAnimation { duration: anim.holdTime }
        NumberAnimation { target: overlay; property: "opacity"; to: 0; duration: 450; easing.type: Easing.InQuad }
    }
}
