#ifndef TA_REL_SESSIONSTATE_H
#define TA_REL_SESSIONSTATE_H

// Session Reliability Layer — typed authoritative session state (M1,
// spec §7, reduced to the M1 event catalogue).
//
// SessionState is a VALUE type: pure data plus invariant helpers, no
// behaviour, no QObject. Discipline-specific state is a std::variant —
// typed and exhaustively switched (the untyped-map design was explicitly
// rejected in V2). Totals are REDUCER-OWNED: no other component computes
// them (spec §8); they are always derived from the shot records.

#include "reliability/core/ReliabilityResult.h"
#include "reliability/events/EventTypes.h"

#include <QMap>
#include <QString>
#include <QVector>

#include <variant>

namespace ta {
namespace rel {

enum class Lifecycle : quint8 { None = 0, Active, Suspended, Complete, Closed };
enum class MatchPhase : quint8 { None = 0, Preparation, Sighting, OfficialMatch };

struct StateShotRecord {
    ShotCore shot;
    quint64 seq = 0;               // envelope seq of the accepting event
    bool invalidated = false;
    qint16 rescoredTenths = -1;    // -1 = never rescored

    // The value official totals are derived from (spec §8: corrections
    // mark records, never delete them).
    qint32 effectiveTenths() const
    {
        if (invalidated)
            return 0;
        return rescoredTenths >= 0 ? rescoredTenths : shot.scoreTenths;
    }

    bool operator==(const StateShotRecord& o) const
    {
        return shot == o.shot && seq == o.seq && invalidated == o.invalidated
            && rescoredTenths == o.rescoredTenths;
    }
};

struct CrossShotRec {
    ShotCore shot;
    quint64 seq = 0;
    QString sourceLane;
    bool operator==(const CrossShotRec& o) const
    { return shot == o.shot && seq == o.seq && sourceLane == o.sourceLane; }
};

struct CorrectionEntry {
    quint64 targetSeq = 0;
    QString type;                  // event type id of the correction
    QString reason;
    qint16 fromTenths = 0;
    qint16 toTenths = 0;
    bool operator==(const CorrectionEntry& o) const
    {
        return targetSeq == o.targetSeq && type == o.type && reason == o.reason
            && fromTenths == o.fromTenths && toTenths == o.toTenths;
    }
};

struct AdjustmentEntry {
    qint16 stageId = -1;           // -1 = whole-match (penalties)
    qint32 deltaTenths = 0;
    QString kind;                  // "SeriesAdjusted" | "PenaltyIssued"
    QString reason;
    bool operator==(const AdjustmentEntry& o) const
    {
        return stageId == o.stageId && deltaTenths == o.deltaTenths
            && kind == o.kind && reason == o.reason;
    }
};

struct IncidentEntry {
    QString kind;                  // "EquipmentMalfunction" etc.
    QString note;
    qint64 allowedTimeMs = -1;
    quint64 seq = 0;
    bool operator==(const IncidentEntry& o) const
    {
        return kind == o.kind && note == o.note
            && allowedTimeMs == o.allowedTimeMs && seq == o.seq;
    }
};

struct TimerState {
    bool active = false;
    TimerId timerId = TimerId::Preparation;
    qint64 durationMs = 0;
    qint64 startedAtMonoMs = 0;
    bool paused = false;
    qint64 pausedAtMonoMs = 0;
    qint64 pausedAccumMs = 0;
    bool operator==(const TimerState& o) const
    {
        return active == o.active && timerId == o.timerId
            && durationMs == o.durationMs && startedAtMonoMs == o.startedAtMonoMs
            && paused == o.paused && pausedAtMonoMs == o.pausedAtMonoMs
            && pausedAccumMs == o.pausedAccumMs;
    }
};

// Discipline-specific state (M1: only fields the current catalogue feeds).
struct QualificationState {
    qint8 positionIndex = -1;
    bool sighterMode = false;
    qint32 version = 1;
    bool operator==(const QualificationState& o) const
    {
        return positionIndex == o.positionIndex && sighterMode == o.sighterMode
            && version == o.version;
    }
};

struct Finals3PState {
    qint16 stageId = 0;
    qint16 windowId = 0;
    qint32 shotsInStage = 0;
    qint32 version = 1;
    bool operator==(const Finals3PState& o) const
    {
        return stageId == o.stageId && windowId == o.windowId
            && shotsInStage == o.shotsInStage && version == o.version;
    }
};

struct TrainingState {
    qint32 version = 1;
    bool operator==(const TrainingState& o) const { return version == o.version; }
};

using DisciplineState =
    std::variant<std::monostate, QualificationState, Finals3PState, TrainingState>;

// Serialized state format version (StateSnapshot payloads).
inline constexpr qint32 kSessionStateVersion = 1;

struct SessionState {
    // identity & configuration
    QString sessionId;
    qint32 schemaVersion = 1;
    QString appVersion, createdAtIso, athlete, lane, targetId, deviceId;
    Discipline discipline = Discipline::None;
    QString matchType;
    DisciplineConfig config;
    // lifecycle
    bool started = false;
    Lifecycle lifecycle = Lifecycle::None;
    MatchPhase phase = MatchPhase::None;
    qint16 currentStageId = -1;
    qint8 positionIndex = -1;
    // records
    QVector<StateShotRecord> officials, sighters;
    QVector<CrossShotRec> crossShots;
    QVector<CorrectionEntry> corrections;
    QVector<AdjustmentEntry> adjustments;
    QVector<IncidentEntry> incidents;
    // totals — reducer-owned, always derived from the records above
    qint32 totalTenths = 0;
    QMap<qint16, qint32> stageSubtotalTenths;
    QMap<qint16, qint8> stageStatuses;
    // timing
    TimerState timer;
    // reliability metadata
    quint64 lastSeq = 0;

    bool operator==(const SessionState& o) const;
    bool operator!=(const SessionState& o) const { return !(*this == o); }

    DisciplineState disc;
};

// Deterministic (byte-stable) serialization of the full state — the
// StateSnapshot payload bytes. Same ordering guarantees as the envelope
// serializer.
QByteArray serializeSessionState(const SessionState& state);
ReliabilityResult deserializeSessionState(const QByteArray& json,
                                          SessionState* out);

// Convenience: a StateSnapshot payload capturing `state` (integrity
// metadata + serialized bytes). lastAppliedSeq = state.lastSeq.
StateSnapshot buildStateSnapshot(const SessionState& state);

} // namespace rel
} // namespace ta

#endif // TA_REL_SESSIONSTATE_H
