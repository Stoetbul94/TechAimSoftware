#ifndef TA_REL_EVENTTYPES_H
#define TA_REL_EVENTTYPES_H

// Session Reliability Layer — M1 typed event catalogue (spec §4).
//
// Every event is a plain typed struct with:
//   kType     — stable string identifier (never renamed once shipped)
//   kVersion  — payload version this build writes (pv)
//   validate()— payload-level validation, typed result
// Durability / broadcast / reducer classifications live in EventRegistry.
//
// The "reserved" group (corrections, jury, recovery, close) is fully typed
// and serializable in M1 but has no production producer yet — controllers
// gain them in M2+. Reducer support for corrections is implemented per
// spec §28 M1 scope.
//
// No QVariantMap anywhere in this model (V2 rule). All numeric payload
// fields are integers (fixed-point raw values per core/FixedPoint.h).

#include "reliability/core/ReliabilityResult.h"

#include <QString>

namespace ta {
namespace rel {

// ── shared enums ──────────────────────────────────────────────────────

// Appended AFTER Training so existing quint8 values never move (journal/
// snapshot compatibility). AirRifleFinal10m / AirPistolFinal10m are the two
// 10m individual FINAL disciplines (F1) — single-athlete training course,
// gated separately from Finals3P. See docs/issf-rules/10m-finals-shared.md.
enum class Discipline : quint8 {
    None = 0, AirPistol10m, AirRifle10m, Prone50m, ThreePositions50m,
    Finals3P, Training, AirRifleFinal10m, AirPistolFinal10m
};

inline const char* disciplineId(Discipline d)
{
    switch (d) {
    case Discipline::None:              return "NONE";
    case Discipline::AirPistol10m:      return "AP10";
    case Discipline::AirRifle10m:       return "AR10";
    case Discipline::Prone50m:          return "PRONE50";
    case Discipline::ThreePositions50m: return "3P50";
    case Discipline::Finals3P:          return "FINAL3P";
    case Discipline::Training:          return "TRAINING";
    case Discipline::AirRifleFinal10m:  return "FINAL_AR10";
    case Discipline::AirPistolFinal10m: return "FINAL_AP10";
    }
    return "NONE";
}

inline bool disciplineFromId(const QString& id, Discipline* out)
{
    if (id == QLatin1String("NONE"))       { *out = Discipline::None; return true; }
    if (id == QLatin1String("AP10"))       { *out = Discipline::AirPistol10m; return true; }
    if (id == QLatin1String("AR10"))       { *out = Discipline::AirRifle10m; return true; }
    if (id == QLatin1String("PRONE50"))    { *out = Discipline::Prone50m; return true; }
    if (id == QLatin1String("3P50"))       { *out = Discipline::ThreePositions50m; return true; }
    if (id == QLatin1String("FINAL3P"))    { *out = Discipline::Finals3P; return true; }
    if (id == QLatin1String("TRAINING"))   { *out = Discipline::Training; return true; }
    if (id == QLatin1String("FINAL_AR10")) { *out = Discipline::AirRifleFinal10m; return true; }
    if (id == QLatin1String("FINAL_AP10")) { *out = Discipline::AirPistolFinal10m; return true; }
    return false;
}

// The two 10m FINAL disciplines share one controller/state (F1).
inline bool isFinals10mDiscipline(Discipline d)
{
    return d == Discipline::AirRifleFinal10m || d == Discipline::AirPistolFinal10m;
}

enum class TimerId : quint8 { Preparation = 0, Match = 1, Stage = 2, Window = 3 };
enum class SuspendReason : quint8 { AppSuspend = 0, UserHide = 1 };
enum class CloseReason : quint8 { Clean = 0, Abort = 1, Archive = 2 };
enum class Authority : quint8 { Jury = 0, Operator = 1 };

// EST incident domain (M3 Phase A). Generic across every discipline —
// no discipline-specific values live here. See
// docs/issf-rules/est-malfunctions.md.
enum class IncidentScope : quint8 {
    IndividualTarget = 0, SelectedFiringPoints = 1, RelayWide = 2
};
enum class IncidentType : quint8 {
    TargetNotRegistering = 0, ContinuousTargetFault = 1,
    ScoringComputerFailure = 2, TargetNetworkFailure = 3,
    ApplicationCrash = 4, ComputerCrash = 5, PowerFailure = 6,
    CommunicationFailure = 7, TargetMove = 8, OtherEstFailure = 9
};
enum class IncidentStatus : quint8 {
    Open = 0, AwaitingAuthorisation = 1, Resolved = 2, Abandoned = 3
};
// The three authorised post-interruption transitions (est-malfunctions §5/§6).
// One parameterized event carries all three (anti-proliferation).
enum class RecoveryPhaseKind : quint8 {
    Preparation = 0, Sighting = 1, OfficialResume = 2
};
// Authorised Jury/RO decisions on an incident (Phase E). One parameterized
// decision event covers them all (anti-proliferation): NoAllowance is the
// explicit "no additional allowance" ruling (distinct from decision-pending);
// DurationAccepted confirms the official interruption duration; Backup* record
// the backup-score review outcome.
enum class EstDecisionKind : quint8 {
    NoAllowance = 0, DurationAccepted = 1,
    BackupAccepted = 2, BackupRejected = 3, BackupInconclusive = 4
};

// Registry classifications (spec §12 / §20 / §8).
enum class DurabilityClass : quint8 { Append, Flush, Sync };
enum class BroadcastClass : quint8 { Internal, Broadcast };
enum class ReducerClass : quint8 { Mutating, Marker, Reserved };

// ── shared payload cores ──────────────────────────────────────────────

// Typed per-discipline configuration (M1 shape; finals-specific stage
// timings arrive with the M2 controller migration as a version bump).
struct DisciplineConfig {
    qint32 officialShots = 0;
    qint32 seriesSize = 0;
    qint64 matchMs = 0;
    qint32 perPositionShots = 0;
    qint32 sighterLimit = 0;       // -1 = unlimited
    qint32 configVersion = 1;

    bool operator==(const DisciplineConfig& o) const
    {
        return officialShots == o.officialShots && seriesSize == o.seriesSize
            && matchMs == o.matchMs && perPositionShots == o.perPositionShots
            && sighterLimit == o.sighterLimit && configVersion == o.configVersion;
    }
    bool operator!=(const DisciplineConfig& o) const { return !(*this == o); }
};

// Core shot payload (spec §4 "ShotCore"). All values fixed-point raw ints.
struct ShotCore {
    qint16 shotNumber = 0;         // 0 = sighter
    qint16 withinStage = 0;
    qint16 stageId = 0;
    qint8  seriesIndex = 0;
    qint32 xHundredthMm = 0;
    qint32 yHundredthMm = 0;
    qint16 scoreTenths = 0;
    qint32 directionCentiDeg = 0;
    qint32 splitMs = 0;
    qint16 windowId = 0;
    qint8  targetMode = 0;
    qint64 externalId = 0;
    bool   simulated = false;

    bool operator==(const ShotCore& o) const
    {
        return shotNumber == o.shotNumber && withinStage == o.withinStage
            && stageId == o.stageId && seriesIndex == o.seriesIndex
            && xHundredthMm == o.xHundredthMm && yHundredthMm == o.yHundredthMm
            && scoreTenths == o.scoreTenths
            && directionCentiDeg == o.directionCentiDeg && splitMs == o.splitMs
            && windowId == o.windowId && targetMode == o.targetMode
            && externalId == o.externalId && simulated == o.simulated;
    }
    bool operator!=(const ShotCore& o) const { return !(*this == o); }

    ReliabilityResult validate() const
    {
        if (shotNumber < 0)
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Invalid shot payload."),
                QStringLiteral("shotNumber %1 < 0").arg(shotNumber));
        // 110 admits the controller's defensive 11.0 acceptance ceiling
        // (ISSF decimal maximum is 10.9; registerShot validates <= 11.0).
        if (scoreTenths < 0 || scoreTenths > 110)
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Invalid shot payload."),
                QStringLiteral("scoreTenths %1 outside 0..110").arg(scoreTenths));
        if (splitMs < 0)
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Invalid shot payload."),
                QStringLiteral("splitMs %1 < 0").arg(splitMs));
        return ReliabilityResult::success();
    }
};

namespace evdetail {
inline ReliabilityResult invalid(const QString& detail)
{
    return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
        QStringLiteral("Invalid event payload."), detail);
}
} // namespace evdetail

// ── M1 active events ──────────────────────────────────────────────────

struct SessionStarted {
    static constexpr const char* kType = "SessionStarted";
    static constexpr qint32 kVersion = 1;
    QString sessionId;
    qint32 schemaVersion = 1;
    QString appVersion;
    QString createdAtIso;
    QString athlete;
    QString lane;
    QString targetId;
    Discipline discipline = Discipline::None;
    QString matchType;
    DisciplineConfig config;
    QString deviceId;

    ReliabilityResult validate() const
    {
        if (sessionId.isEmpty())
            return evdetail::invalid(QStringLiteral("SessionStarted.sessionId empty"));
        if (schemaVersion < 1)
            return evdetail::invalid(QStringLiteral("SessionStarted.schemaVersion < 1"));
        if (createdAtIso.isEmpty())
            return evdetail::invalid(QStringLiteral("SessionStarted.createdAtIso empty"));
        return ReliabilityResult::success();
    }
};

struct SessionConfigured {
    static constexpr const char* kType = "SessionConfigured";
    static constexpr qint32 kVersion = 1;
    Discipline discipline = Discipline::None;
    QString matchType;
    DisciplineConfig config;

    ReliabilityResult validate() const
    {
        if (discipline == Discipline::None)
            return evdetail::invalid(QStringLiteral("SessionConfigured.discipline is NONE"));
        return ReliabilityResult::success();
    }
};

struct AthleteAssigned {
    static constexpr const char* kType = "AthleteAssigned";
    static constexpr qint32 kVersion = 1;
    QString athlete;
    QString lane;
    QString targetId;

    ReliabilityResult validate() const
    {
        if (athlete.isEmpty())
            return evdetail::invalid(QStringLiteral("AthleteAssigned.athlete empty"));
        return ReliabilityResult::success();
    }
};

struct PreparationStarted {
    static constexpr const char* kType = "PreparationStarted";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("PreparationStarted.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct SightingStarted {
    static constexpr const char* kType = "SightingStarted";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("SightingStarted.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct OfficialMatchStarted {
    static constexpr const char* kType = "OfficialMatchStarted";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("OfficialMatchStarted.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct ShotAccepted {
    static constexpr const char* kType = "ShotAccepted";
    static constexpr qint32 kVersion = 1;
    ShotCore shot;
    ReliabilityResult validate() const
    {
        if (shot.shotNumber < 1)
            return evdetail::invalid(
                QStringLiteral("ShotAccepted.shotNumber %1 < 1 (0 is reserved for sighters)")
                    .arg(shot.shotNumber));
        return shot.validate();
    }
};

struct SighterAccepted {
    static constexpr const char* kType = "SighterAccepted";
    static constexpr qint32 kVersion = 1;
    ShotCore shot;
    ReliabilityResult validate() const
    {
        if (shot.shotNumber != 0)
            return evdetail::invalid(
                QStringLiteral("SighterAccepted.shotNumber %1 != 0").arg(shot.shotNumber));
        return shot.validate();
    }
};

struct StageCompleted {
    static constexpr const char* kType = "StageCompleted";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    qint8 status = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("StageCompleted.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct PositionChanged {
    static constexpr const char* kType = "PositionChanged";
    static constexpr qint32 kVersion = 1;
    qint8 positionIndex = 0;   // 3P: 0=K 1=P 2=S
    ReliabilityResult validate() const
    {
        if (positionIndex < 0 || positionIndex > 2)
            return evdetail::invalid(
                QStringLiteral("PositionChanged.positionIndex %1 outside 0..2")
                    .arg(positionIndex));
        return ReliabilityResult::success();
    }
};

struct TimerStarted {
    static constexpr const char* kType = "TimerStarted";
    static constexpr qint32 kVersion = 1;
    TimerId timerId = TimerId::Preparation;
    qint64 durationMs = 0;
    ReliabilityResult validate() const
    {
        if (durationMs <= 0)
            return evdetail::invalid(
                QStringLiteral("TimerStarted.durationMs %1 <= 0").arg(durationMs));
        return ReliabilityResult::success();
    }
};

struct TimerPaused {
    static constexpr const char* kType = "TimerPaused";
    static constexpr qint32 kVersion = 1;
    TimerId timerId = TimerId::Preparation;
    qint64 atMonoMs = 0;
    ReliabilityResult validate() const
    {
        if (atMonoMs < 0)
            return evdetail::invalid(QStringLiteral("TimerPaused.atMonoMs < 0"));
        return ReliabilityResult::success();
    }
};

struct TimerResumed {
    static constexpr const char* kType = "TimerResumed";
    static constexpr qint32 kVersion = 1;
    TimerId timerId = TimerId::Preparation;
    qint64 atMonoMs = 0;
    ReliabilityResult validate() const
    {
        if (atMonoMs < 0)
            return evdetail::invalid(QStringLiteral("TimerResumed.atMonoMs < 0"));
        return ReliabilityResult::success();
    }
};

struct TimerExpired {
    static constexpr const char* kType = "TimerExpired";
    static constexpr qint32 kVersion = 1;
    TimerId timerId = TimerId::Preparation;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

struct MatchCompleted {
    static constexpr const char* kType = "MatchCompleted";
    static constexpr qint32 kVersion = 1;
    qint32 totalTenths = 0;
    qint16 officialCount = 0;
    ReliabilityResult validate() const
    {
        if (totalTenths < 0 || officialCount < 0)
            return evdetail::invalid(QStringLiteral("MatchCompleted negative totals"));
        return ReliabilityResult::success();
    }
};

struct SessionSuspended {
    static constexpr const char* kType = "SessionSuspended";
    static constexpr qint32 kVersion = 1;
    SuspendReason reason = SuspendReason::AppSuspend;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

struct SessionResumed {
    static constexpr const char* kType = "SessionResumed";
    static constexpr qint32 kVersion = 1;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

// Full-state snapshot (spec §13). The state itself travels as the UTF-8
// bytes of the deterministic SessionState serialization, embedded as a
// JSON STRING — this keeps events/ independent of reducer/ (layering,
// spec §29) and makes hash re-serialization trivially byte-stable.
struct StateSnapshot {
    static constexpr const char* kType = "StateSnapshot";
    static constexpr qint32 kVersion = 1;
    qint32 stateVersion = 1;
    quint64 lastAppliedSeq = 0;    // state == fold of events 0..lastAppliedSeq
    qint32 officialCount = 0;      // integrity cross-checks (reducer-verified)
    qint32 sighterCount = 0;
    qint32 totalTenths = 0;
    QByteArray stateJson;          // deterministic serializeSessionState() bytes

    ReliabilityResult validate() const
    {
        if (stateVersion < 1)
            return evdetail::invalid(QStringLiteral("StateSnapshot.stateVersion < 1"));
        if (stateJson.isEmpty())
            return evdetail::invalid(QStringLiteral("StateSnapshot.stateJson empty"));
        if (officialCount < 0 || sighterCount < 0)
            return evdetail::invalid(QStringLiteral("StateSnapshot negative counts"));
        return ReliabilityResult::success();
    }
};

// ── reserved typed events (no production producer until M2+) ──────────

struct ShotInvalidated {
    static constexpr const char* kType = "ShotInvalidated";
    static constexpr qint32 kVersion = 1;
    quint64 targetSeq = 0;         // seq of the ShotAccepted envelope
    QString reason;
    Authority authority = Authority::Jury;
    ReliabilityResult validate() const
    {
        if (reason.isEmpty())
            return evdetail::invalid(QStringLiteral("ShotInvalidated.reason empty"));
        return ReliabilityResult::success();
    }
};

struct ShotRescored {
    static constexpr const char* kType = "ShotRescored";
    static constexpr qint32 kVersion = 1;
    quint64 targetSeq = 0;
    qint16 newScoreTenths = 0;
    bool hasNewCoordinates = false;
    qint32 newXHundredthMm = 0;
    qint32 newYHundredthMm = 0;
    QString reason;
    Authority authority = Authority::Jury;
    ReliabilityResult validate() const
    {
        if (newScoreTenths < 0 || newScoreTenths > 109)
            return evdetail::invalid(
                QStringLiteral("ShotRescored.newScoreTenths outside 0..109"));
        if (reason.isEmpty())
            return evdetail::invalid(QStringLiteral("ShotRescored.reason empty"));
        return ReliabilityResult::success();
    }
};

struct SeriesAdjusted {
    static constexpr const char* kType = "SeriesAdjusted";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    qint32 deltaTenths = 0;
    QString reason;
    Authority authority = Authority::Jury;
    ReliabilityResult validate() const
    {
        if (reason.isEmpty())
            return evdetail::invalid(QStringLiteral("SeriesAdjusted.reason empty"));
        return ReliabilityResult::success();
    }
};

struct CrossShotRecorded {
    static constexpr const char* kType = "CrossShotRecorded";
    static constexpr qint32 kVersion = 1;
    ShotCore shot;
    QString sourceLane;
    ReliabilityResult validate() const { return shot.validate(); }
};

struct EquipmentMalfunctionRecorded {
    static constexpr const char* kType = "EquipmentMalfunctionRecorded";
    static constexpr qint32 kVersion = 1;
    QString note;
    qint64 allowedTimeMs = -1;     // -1 = no allowance
    ReliabilityResult validate() const
    {
        if (note.isEmpty())
            return evdetail::invalid(
                QStringLiteral("EquipmentMalfunctionRecorded.note empty"));
        return ReliabilityResult::success();
    }
};

struct PenaltyIssued {
    static constexpr const char* kType = "PenaltyIssued";
    static constexpr qint32 kVersion = 1;
    qint32 deltaTenths = 0;        // usually negative
    QString rule;
    Authority authority = Authority::Jury;
    ReliabilityResult validate() const
    {
        if (rule.isEmpty())
            return evdetail::invalid(QStringLiteral("PenaltyIssued.rule empty"));
        return ReliabilityResult::success();
    }
};

struct RecoveryStarted {
    static constexpr const char* kType = "RecoveryStarted";
    static constexpr qint32 kVersion = 1;
    quint64 fromSeq = 0;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

struct RecoveryCompleted {
    static constexpr const char* kType = "RecoveryCompleted";
    static constexpr qint32 kVersion = 1;
    quint64 resumedAtSeq = 0;
    bool truncatedTail = false;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

struct SessionClosed {
    static constexpr const char* kType = "SessionClosed";
    static constexpr qint32 kVersion = 1;
    CloseReason reason = CloseReason::Clean;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

// ── M2 additions — finals flow + persistence markers ──────────────────
// Appended AFTER every M1 type so existing variant indexes never move.

struct StageEntered {
    static constexpr const char* kType = "StageEntered";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("StageEntered.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct StageStatusChanged {
    static constexpr const char* kType = "StageStatusChanged";
    static constexpr qint32 kVersion = 1;
    qint16 stageId = 0;
    qint8 status = 0;
    ReliabilityResult validate() const
    {
        if (stageId < 0)
            return evdetail::invalid(QStringLiteral("StageStatusChanged.stageId < 0"));
        return ReliabilityResult::success();
    }
};

struct TargetModeChanged {
    static constexpr const char* kType = "TargetModeChanged";
    static constexpr qint32 kVersion = 1;
    qint8 mode = 0;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

struct WindowOpened {
    static constexpr const char* kType = "WindowOpened";
    static constexpr qint32 kVersion = 1;
    qint16 windowId = 0;
    ReliabilityResult validate() const
    {
        if (windowId < 0)
            return evdetail::invalid(QStringLiteral("WindowOpened.windowId < 0"));
        return ReliabilityResult::success();
    }
};

struct WindowClosed {
    static constexpr const char* kType = "WindowClosed";
    static constexpr qint32 kVersion = 1;
    qint16 windowId = 0;
    ReliabilityResult validate() const
    {
        if (windowId < 0)
            return evdetail::invalid(QStringLiteral("WindowClosed.windowId < 0"));
        return ReliabilityResult::success();
    }
};

struct CommandIssued {
    static constexpr const char* kType = "CommandIssued";
    static constexpr qint32 kVersion = 1;
    qint32 commandId = 0;
    qint16 commandType = 0;
    QString text;
    qint16 sequenceNumber = 0;
    QString audioCueId;
    qint64 issuedAtMs = 0;
    qint64 effectiveAtMs = 0;
    ReliabilityResult validate() const
    {
        if (text.isEmpty())
            return evdetail::invalid(QStringLiteral("CommandIssued.text empty"));
        if (issuedAtMs < 0 || effectiveAtMs < 0)
            return evdetail::invalid(
                QStringLiteral("CommandIssued negative timestamps"));
        return ReliabilityResult::success();
    }
};

struct ShotRejected {
    static constexpr const char* kType = "ShotRejected";
    static constexpr qint32 kVersion = 1;
    QString reason;
    qint64 externalId = 0;
    qint32 xHundredthMm = 0;
    qint32 yHundredthMm = 0;
    bool simulated = false;
    ReliabilityResult validate() const
    {
        if (reason.isEmpty())
            return evdetail::invalid(QStringLiteral("ShotRejected.reason empty"));
        return ReliabilityResult::success();
    }
};

struct MissingShotRecorded {
    static constexpr const char* kType = "MissingShotRecorded";
    static constexpr qint32 kVersion = 1;
    qint16 expectedNumber = 0;
    qint16 stageId = 0;
    QString reason;
    ReliabilityResult validate() const
    {
        if (expectedNumber < 1)
            return evdetail::invalid(
                QStringLiteral("MissingShotRecorded.expectedNumber < 1"));
        if (reason.isEmpty())
            return evdetail::invalid(
                QStringLiteral("MissingShotRecorded.reason empty"));
        return ReliabilityResult::success();
    }
};

// Persistence health markers (spec §9C–E). Written by the SessionStore
// itself; validator allows sequence GAPS only between a Degraded/Restored
// pair (recorded aux drops are the only source of gaps).
struct PersistenceDegraded {
    static constexpr const char* kType = "PersistenceDegraded";
    static constexpr qint32 kVersion = 1;
    qint32 queuedCount = 0;
    ReliabilityResult validate() const
    {
        if (queuedCount < 0)
            return evdetail::invalid(
                QStringLiteral("PersistenceDegraded.queuedCount < 0"));
        return ReliabilityResult::success();
    }
};

struct PersistenceRestored {
    static constexpr const char* kType = "PersistenceRestored";
    static constexpr qint32 kVersion = 1;
    qint32 queuedCount = 0;
    ReliabilityResult validate() const
    {
        if (queuedCount < 0)
            return evdetail::invalid(
                QStringLiteral("PersistenceRestored.queuedCount < 0"));
        return ReliabilityResult::success();
    }
};

struct AuxEventsDropped {
    static constexpr const char* kType = "AuxEventsDropped";
    static constexpr qint32 kVersion = 1;
    quint64 firstSeq = 0;
    quint64 lastSeq = 0;
    qint32 count = 0;
    ReliabilityResult validate() const
    {
        if (count < 1)
            return evdetail::invalid(QStringLiteral("AuxEventsDropped.count < 1"));
        if (lastSeq < firstSeq)
            return evdetail::invalid(
                QStringLiteral("AuxEventsDropped.lastSeq < firstSeq"));
        return ReliabilityResult::success();
    }
};

struct CleanShutdown {
    static constexpr const char* kType = "CleanShutdown";
    static constexpr qint32 kVersion = 1;
    ReliabilityResult validate() const { return ReliabilityResult::success(); }
};

// ── M3 Phase A — generic EST incident + Jury-decision events ───────────
// Appended AFTER every prior type so existing variant indexes never move.
// These are DISCIPLINE-AGNOSTIC (est-malfunctions.md §7/§8). None of them
// mutate the competition timer or official totals — a Jury-authorised
// allowance is recorded as data; the controller applies it on resume
// (est-malfunctions.md §5: no automatic time allowance).

// Opens an EST/range-caused incident. `interruptionStartUtc` is a persisted
// UTC wall-clock instant (survives reboot) — distinct from the envelope's
// tw (when this event was journalled).
struct EstIncidentRaised {
    static constexpr const char* kType = "EstIncidentRaised";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    IncidentType incidentType = IncidentType::OtherEstFailure;
    IncidentScope scope = IncidentScope::IndividualTarget;
    QString firingPoint;              // may be empty
    QString relayId;                  // may be empty
    QString interruptionStartUtc;     // ISO-8601 UTC (persisted wall-clock)
    QString reason;
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(QStringLiteral("EstIncidentRaised.incidentId empty"));
        if (interruptionStartUtc.isEmpty())
            return evdetail::invalid(
                QStringLiteral("EstIncidentRaised.interruptionStartUtc empty"));
        return ReliabilityResult::success();
    }
};

// A distinct, authorised time-credit action (est-malfunctions.md §5.3). Does
// NOT auto-extend the clock — recorded against the incident for the
// controller to apply on resume.
struct TimeCreditGranted {
    static constexpr const char* kType = "TimeCreditGranted";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    qint64 durationMs = 0;            // >= 0
    Authority authority = Authority::Jury;
    QString authorisedBy;            // official identifier, non-empty
    QString reason;
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(QStringLiteral("TimeCreditGranted.incidentId empty"));
        if (durationMs < 0)
            return evdetail::invalid(
                QStringLiteral("TimeCreditGranted.durationMs %1 < 0").arg(durationMs));
        if (authorisedBy.isEmpty())
            return evdetail::invalid(
                QStringLiteral("TimeCreditGranted.authorisedBy empty"));
        return ReliabilityResult::success();
    }
};

// One authorised post-interruption phase transition: the 5-minute prep, the
// unlimited recovery-sighting period, or the official-resume gate
// (est-malfunctions.md §6). durationMs: prep = allowance ms; sighting = 0
// (unlimited); officialResume = 0.
struct RecoveryPhaseEntered {
    static constexpr const char* kType = "RecoveryPhaseEntered";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    RecoveryPhaseKind phase = RecoveryPhaseKind::Preparation;
    qint64 durationMs = 0;            // >= 0
    Authority authority = Authority::Jury;
    QString authorisedBy;            // non-empty (authorised action)
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(
                QStringLiteral("RecoveryPhaseEntered.incidentId empty"));
        if (durationMs < 0)
            return evdetail::invalid(
                QStringLiteral("RecoveryPhaseEntered.durationMs %1 < 0").arg(durationMs));
        if (authorisedBy.isEmpty())
            return evdetail::invalid(
                QStringLiteral("RecoveryPhaseEntered.authorisedBy empty"));
        return ReliabilityResult::success();
    }
};

// An authorised Jury/RO decision on an open incident (Phase E). Records the
// tri-state allowance ruling (a deliberate NoAllowance is journalled — the
// absence of TimeCreditGranted never implies it), the officially accepted
// interruption duration, and backup-score review outcomes. Never mutates
// clocks or scores.
struct EstDecisionRecorded {
    static constexpr const char* kType = "EstDecisionRecorded";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    EstDecisionKind decision = EstDecisionKind::NoAllowance;
    qint64 acceptedDurationMs = -1;   // official duration (-1 = not part of
                                      // this decision)
    Authority authority = Authority::Jury;
    QString authorisedBy;            // official identifier, non-empty
    QString reason;
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(QStringLiteral("EstDecisionRecorded.incidentId empty"));
        if (acceptedDurationMs < -1)
            return evdetail::invalid(
                QStringLiteral("EstDecisionRecorded.acceptedDurationMs < -1"));
        if (authorisedBy.isEmpty())
            return evdetail::invalid(
                QStringLiteral("EstDecisionRecorded.authorisedBy empty"));
        return ReliabilityResult::success();
    }
};

// Authorised move to a reserve target/firing point DURING an open incident
// (Phase E — Phase A folded the move into Resolve; the live workflow records
// it when it happens; the envelope `tw` is the time of reassignment).
struct TargetReassigned {
    static constexpr const char* kType = "TargetReassigned";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    QString originalTarget;
    QString reserveTarget;
    Authority authority = Authority::Jury;
    QString authorisedBy;
    QString reason;
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(QStringLiteral("TargetReassigned.incidentId empty"));
        if (reserveTarget.isEmpty())
            return evdetail::invalid(
                QStringLiteral("TargetReassigned.reserveTarget empty"));
        if (authorisedBy.isEmpty())
            return evdetail::invalid(
                QStringLiteral("TargetReassigned.authorisedBy empty"));
        return ReliabilityResult::success();
    }
};

// Closes an incident with the official record (est-malfunctions.md §7/§9).
// Carries the target-move + backup-review + Jury/RO note fields folded in.
struct EstIncidentResolved {
    static constexpr const char* kType = "EstIncidentResolved";
    static constexpr qint32 kVersion = 1;
    QString incidentId;
    IncidentStatus status = IncidentStatus::Resolved;   // Resolved | Abandoned
    qint64 calculatedDurationMs = -1;         // system estimate (-1 = unknown)
    qint64 officiallyAcceptedDurationMs = 0;  // Jury-accepted, >= 0
    QString systemRestoredUtc;                // ISO-8601 UTC (may be empty)
    bool targetMoved = false;
    QString originalTarget;
    QString reserveTarget;
    bool backupScoreReviewed = false;
    QString juryNote;
    QString rangeOfficerNote;
    QString incidentReportRef;
    ReliabilityResult validate() const
    {
        if (incidentId.isEmpty())
            return evdetail::invalid(QStringLiteral("EstIncidentResolved.incidentId empty"));
        if (officiallyAcceptedDurationMs < 0)
            return evdetail::invalid(
                QStringLiteral("EstIncidentResolved.officiallyAcceptedDurationMs < 0"));
        return ReliabilityResult::success();
    }
};

} // namespace rel
} // namespace ta

#endif // TA_REL_EVENTTYPES_H
