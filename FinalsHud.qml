import QtQuick 2.15

// 3P FINAL — HUD shell (Option A: top strip + contextual layers).
// Pure presentation: composes the HUD components and binds them to the finals
// controller. Contains NO scoring, timing or transition logic; every value and
// event comes from `ctl` (FINALS3P in the app, a mock in the test harness).
Item {
    id: hud

    // The finals controller (FINALS3P) or a state mock in the HUD harness.
    property var ctl
    // Developer/testing controls gate — from APPSETTINGS, never hardcoded.
    property bool developerMode: false

    // ── Layer 1 + 4: persistent top strip with compact progress ─────────
    FinalsTopStrip {
        id: strip
        ctl: hud.ctl
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        // ~6% of the target height, clamped to sane pixel sizes (spec §1/§11).
        height: Math.max(38, Math.min(46, hud.height * 0.06))
    }

    // Layers 2 (performance block), 3 (command overlay), athlete ADVANCE,
    // incident toasts and the developer drawer are added in HUD2/HUD3.
}
