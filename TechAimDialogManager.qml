import QtQuick 2.15

// ─────────────────────────────────────────────────────────────────────────────
// TechAim dialog manager — the app-facing API for every message / warning /
// confirmation popup. One instance lives in main.qml (id: dialogManager) and
// resolves everywhere via QML ancestor scope, exactly like windowManager.
//
//   dialogManager.showInformation(title, message)
//   dialogManager.showWarning(title, message)
//   dialogManager.showError(title, message, details)
//   dialogManager.showSuccess(title, message)
//   dialogManager.showConfirmation(title, message, function(ok) { ... })
//   dialogManager.showQuestion(title, message, function(yes) { ... })
//   dialogManager.show({ type, title, message, details, buttons,
//                        defaultResult, cancelResult, autoDismissMs,
//                        onResult })
//
// Dialogs queue: one is ever visible; the next opens when the current one
// closes. The manager owns NO application logic — callbacks belong to the
// caller and receive the pressed button's result.
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: manager
    anchors.fill: parent

    property var pending: []          // queued configs (mutated in place)
    property bool busy: false
    property var activeCallback: null

    // ── generic entry point ──────────────────────────────────────────────
    function show(cfg) {
        if (!cfg) return
        manager.pending.push(cfg)
        if (!manager.busy) manager._next()
    }

    function _next() {
        if (manager.pending.length === 0) { manager.busy = false; return }
        manager.busy = true
        var cfg = manager.pending.shift()
        manager.activeCallback = (typeof cfg.onResult === "function") ? cfg.onResult : null
        dlg.type          = cfg.type || "info"
        dlg.title         = cfg.title || ""
        dlg.message       = cfg.message || ""
        dlg.details       = cfg.details || ""
        dlg.buttons       = (cfg.buttons && cfg.buttons.length > 0)
                            ? cfg.buttons
                            : [{ label: qsTr("OK"), result: "ok", accent: true }]
        dlg.defaultResult = cfg.defaultResult !== undefined ? cfg.defaultResult : "ok"
        dlg.cancelResult  = cfg.cancelResult  !== undefined ? cfg.cancelResult  : "ok"
        dlg.autoDismissMs = cfg.autoDismissMs || 0
        dlg.open()
    }

    // ── convenience API ──────────────────────────────────────────────────
    function showInformation(title, message, details) {
        manager.show({ type: "info", title: title, message: message, details: details || "" })
    }
    function showWarning(title, message, details) {
        manager.show({ type: "warning", title: title, message: message, details: details || "" })
    }
    function showError(title, message, details) {
        manager.show({ type: "error", title: title, message: message, details: details || "" })
    }
    function showSuccess(title, message, details) {
        manager.show({ type: "success", title: title, message: message, details: details || "" })
    }
    // callback(true) on OK, callback(false) on Cancel/Esc
    function showConfirmation(title, message, callback, okLabel, cancelLabel) {
        manager.show({
            type: "confirm", title: title, message: message,
            buttons: [
                { label: cancelLabel || qsTr("Cancel"), result: "cancel", accent: false },
                { label: okLabel || qsTr("OK"),         result: "ok",     accent: true }
            ],
            defaultResult: "ok", cancelResult: "cancel",
            onResult: function (r) { if (callback) callback(r === "ok") }
        })
    }
    // callback(true) on Yes, callback(false) on No/Esc
    function showQuestion(title, message, callback, yesLabel, noLabel) {
        manager.show({
            type: "question", title: title, message: message,
            buttons: [
                { label: noLabel || qsTr("No"),   result: "no",  accent: false },
                { label: yesLabel || qsTr("Yes"), result: "yes", accent: true }
            ],
            defaultResult: "yes", cancelResult: "no",
            onResult: function (r) { if (callback) callback(r === "yes") }
        })
    }

    TechAimDialog {
        id: dlg
        onFinished: function (result) {
            var cb = manager.activeCallback
            manager.activeCallback = null
            if (cb) cb(result)
            manager._next()
        }
    }
}
