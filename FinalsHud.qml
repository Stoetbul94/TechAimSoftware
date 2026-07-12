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

    // ── Layer 3: CRO command overlay (transient, upper-middle) ──────────
    FinalsCommandOverlay {
        ctl: hud.ctl
        anchors.horizontalCenter: parent.horizontalCenter
        y: strip.height + hud.height * 0.08
        maxWidth: hud.width * 0.7
    }

    // ── Layer 2: contextual performance block (bottom-right, clear of the
    //    shot-value circle and the central scoring area) ──────────────────
    FinalsPerformanceBlock {
        ctl: hud.ctl
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Math.max(150, hud.height * 0.22)
    }

    // ── Athlete ADVANCE control (contextual pill, bottom centre) ────────
    FinalsAdvanceControl {
        ctl: hud.ctl
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
    }

    // ── Incident toasts (slide from under the strip, fade away) ─────────
    FinalsIncidentToast {
        ctl: hud.ctl
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: strip.bottom
        anchors.topMargin: 8
    }

    // ── Developer drawer — config-gated, never in production ─────────────
    FinalsDeveloperDrawer {
        ctl: hud.ctl
        visible: hud.developerMode
        anchors.right: parent.right
        anchors.top: strip.bottom
        anchors.topMargin: 8
        anchors.rightMargin: 8
    }
}
