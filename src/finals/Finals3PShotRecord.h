#pragma once
// 3P FINAL — shot / incident / missing-shot record builders (header-only,
// QtCore only, unit-testable without QML). Every role that will ever exist on
// the finals models is present in EVERY record from the first append, so QML
// ListModel role-locking can never break later (Phase-B plan §2).
//
// Existing qualification role names are reused (calculatedscore, score, xmm,
// ymm, direction, timestamp, timeComsumed, position); finals-specific roles
// are prefixed. For finals, `timeComsumed` is DEFINED as the per-shot split
// in whole seconds: time since the previous accepted shot in this firing
// window, or since the window opened for the window's first shot (FIX-R3 —
// matches the qualification column semantic; was cumulative window time).

#include <QVariantMap>
#include <QDateTime>
#include <QString>

#include "Finals3PTypes.h"

namespace techaim {
namespace finals {

struct ShotContext {
    Stage stage = Stage::Idle;
    int windowId = 0;
    int targetMode = 0;              // TargetMode at receipt
    qint64 windowElapsedMs = 0;      // since the current firing window opened
    qint64 stageElapsedMs = 0;       // since stage entry
    qint64 sincePrevShotMs = 0;      // since previous accepted shot in window
    bool hasPrevInWindow = false;    // false for the first shot of a window
};

inline QVariantMap acceptedShotRecord(const ShotContext& c,
                                      double xMm, double yMm,
                                      double decimalScore,
                                      bool sighter,
                                      int finalShotNumber,     // 0 for sighters
                                      int shotWithinStage,     // 0 for sighters
                                      quint64 eventId,
                                      qint64 externalShotId,
                                      bool simulated)
{
    QVariantMap m;
    // ── qualification-compatible roles ──
    m[QStringLiteral("calculatedscore")] = QString::number(decimalScore, 'f', 1);
    m[QStringLiteral("score")]           = QString::number(decimalScore, 'f', 2);
    m[QStringLiteral("xmm")]             = xMm;
    m[QStringLiteral("ymm")]             = yMm;
    m[QStringLiteral("direction")]       = QStringLiteral("0.00");   // set by ingestion when available
    m[QStringLiteral("timestamp")]       = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    // FIX-R3: per-shot split, not cumulative window time. First shot of a
    // window counts from the window opening; hasPrevInWindow (not a zero
    // split) distinguishes it — two shots in one tick give a real 0 s split.
    m[QStringLiteral("timeComsumed")]    = int(((c.hasPrevInWindow ? c.sincePrevShotMs
                                                                   : c.windowElapsedMs) + 999) / 1000);
    m[QStringLiteral("position")]        = positionRoleFor(c.stage);
    // ── finals roles ──
    m[QStringLiteral("finalsStageId")]      = static_cast<int>(c.stage);
    m[QStringLiteral("finalsStageLabel")]   = stageName(c.stage);
    m[QStringLiteral("finalsPosition")]     = positionRoleFor(c.stage);
    m[QStringLiteral("finalsShotNumber")]   = finalShotNumber;
    m[QStringLiteral("finalsWindowId")]     = c.windowId;
    m[QStringLiteral("finalsSeriesIndex")]  = seriesIndexFor(c.stage);
    m[QStringLiteral("shotWithinStage")]    = shotWithinStage;
    m[QStringLiteral("targetModeAtReceipt")]= c.targetMode;
    m[QStringLiteral("isSighter")]          = sighter;
    m[QStringLiteral("isFinalsShot")]       = true;
    // ── event bookkeeping (not model roles; consumed by router/journal) ──
    m[QStringLiteral("eventId")]            = eventId;
    m[QStringLiteral("externalShotId")]     = externalShotId;
    m[QStringLiteral("simulated")]          = simulated;
    m[QStringLiteral("sighter")]            = sighter;               // Phase-A compat
    m[QStringLiteral("stageId")]            = static_cast<int>(c.stage);
    m[QStringLiteral("stageLabel")]         = stageName(c.stage);
    m[QStringLiteral("windowId")]           = c.windowId;
    m[QStringLiteral("decimalScore")]       = decimalScore;
    m[QStringLiteral("xMm")]                = xMm;
    m[QStringLiteral("yMm")]                = yMm;
    m[QStringLiteral("timeUsedInWindow")]   = c.windowElapsedMs;
    m[QStringLiteral("timeSincePrevShotMs")]= c.sincePrevShotMs;
    m[QStringLiteral("stageElapsedMs")]     = c.stageElapsedMs;
    return m;
}

inline QVariantMap rejectionRecord(const ShotContext& c,
                                   RejectReason reason,
                                   double xMm, double yMm,
                                   quint64 eventId,
                                   qint64 externalShotId,
                                   bool simulated)
{
    QVariantMap m;
    m[QStringLiteral("reason")]         = rejectReasonName(reason);
    m[QStringLiteral("stageId")]        = static_cast<int>(c.stage);
    m[QStringLiteral("stageLabel")]     = stageName(c.stage);
    m[QStringLiteral("finalsWindowId")] = c.windowId;
    m[QStringLiteral("xmm")]            = xMm;
    m[QStringLiteral("ymm")]            = yMm;
    m[QStringLiteral("timestamp")]      = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    m[QStringLiteral("eventId")]        = eventId;
    m[QStringLiteral("externalShotId")] = externalShotId;
    m[QStringLiteral("simulated")]      = simulated;
    return m;
}

// [P1 = Option B] Missing expected shots are separate records — NEVER entries
// in the detected-shot models. The report layer renders them as
// "DNS / Not fired — 0.0 provisional".
inline QVariantMap missingShotRecord(Stage stage, int expectedShotNumber)
{
    QVariantMap m;
    m[QStringLiteral("expectedShotNumber")] = expectedShotNumber;
    m[QStringLiteral("stageId")]            = static_cast<int>(stage);
    m[QStringLiteral("stageLabel")]         = stageName(stage);
    m[QStringLiteral("reason")]             = QStringLiteral("TimeExpired");
    m[QStringLiteral("provisionalScore")]   = 0.0;
    m[QStringLiteral("timestamp")]          = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    return m;
}

} // namespace finals
} // namespace techaim
