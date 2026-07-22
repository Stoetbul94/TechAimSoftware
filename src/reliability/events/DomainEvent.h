#ifndef TA_REL_DOMAINEVENT_H
#define TA_REL_DOMAINEVENT_H

// Session Reliability Layer — the DomainEvent variant (M1, spec §4).
// A deliberately plain std::variant: exhaustive switches, debuggable
// alternatives, no template metaprogramming beyond std::visit.

#include "EventTypes.h"

#include <variant>

namespace ta {
namespace rel {

using DomainEvent = std::variant<
    // M1 active
    SessionStarted,
    SessionConfigured,
    AthleteAssigned,
    PreparationStarted,
    SightingStarted,
    OfficialMatchStarted,
    ShotAccepted,
    SighterAccepted,
    StageCompleted,
    PositionChanged,
    TimerStarted,
    TimerPaused,
    TimerResumed,
    TimerExpired,
    MatchCompleted,
    SessionSuspended,
    SessionResumed,
    StateSnapshot,
    // reserved (typed now, produced M2+)
    ShotInvalidated,
    ShotRescored,
    SeriesAdjusted,
    CrossShotRecorded,
    EquipmentMalfunctionRecorded,
    PenaltyIssued,
    RecoveryStarted,
    RecoveryCompleted,
    SessionClosed,
    // M2 — finals flow + persistence markers (appended at the END so M1
    // variant indexes never move)
    StageEntered,
    StageStatusChanged,
    TargetModeChanged,
    WindowOpened,
    WindowClosed,
    CommandIssued,
    ShotRejected,
    MissingShotRecorded,
    PersistenceDegraded,
    PersistenceRestored,
    AuxEventsDropped,
    CleanShutdown,
    // M3 Phase A — generic EST incident + Jury-decision events (appended at
    // the END so all prior variant indexes never move)
    EstIncidentRaised,
    TimeCreditGranted,
    RecoveryPhaseEntered,
    EstIncidentResolved,
    // Phase E — Jury decision + live target reassignment (appended at the END
    // so all prior variant indexes never move)
    EstDecisionRecorded,
    TargetReassigned,
    // Training Lab (T1) — appended at the END so all prior variant indexes and
    // journal hashes never move. Training reuses SessionStarted/SessionClosed.
    TrainingSessionStarted,
    TrainingBlockStarted,
    TrainingShotAccepted,
    TrainingBlockCompleted,
    TrainingNoteSaved,
    TrainingCompleted,
    TrainingSighterAccepted,
    TrainingSighterPhaseStarted>;

// Stable type identifier of the alternative currently held.
inline const char* eventTypeId(const DomainEvent& event)
{
    return std::visit([](const auto& e) { return e.kType; }, event);
}

// Payload version this build writes for the held alternative.
inline qint32 eventPayloadVersion(const DomainEvent& event)
{
    return std::visit([](const auto& e) { return e.kVersion; }, event);
}

// Payload-level validation of the held alternative.
inline ReliabilityResult validateEvent(const DomainEvent& event)
{
    return std::visit([](const auto& e) { return e.validate(); }, event);
}

} // namespace rel
} // namespace ta

#endif // TA_REL_DOMAINEVENT_H
