#pragma once
// 10m Air Rifle / Air Pistol FINAL — shared types (F1).
//
// ISSF Rule Book Edition 2025 (Second Print 07/2026), effective 1 July 2026,
// rule 6.17.2 (10m AR & AP individual finals). Verified rules:
// docs/issf-rules/10m-finals-shared.md. Architecture:
// docs/10m-finals-architecture.md.
//
// This domain is a SINGLE-ATHLETE training course (F1). It is fully separate
// from the 50m 3P Final (techaim::finals) and from every qualification
// discipline. Multi-athlete ranking / eliminations / ties / shoot-offs are
// the future Finals10mCompetitionCoordinator's responsibility and are NOT
// modelled here — the elimination points are single-athlete course
// CHECKPOINTS only (no finishing place is ever assigned locally).

#include <QString>

namespace techaim {
namespace finals10m {

// Linear course, fully command-driven. No positions, no shared stage clock.
// Singles are one compact stage carrying a 1..14 index (m_singleIndex).
enum class Stage {
    Idle = 0,
    Presentation,     // intro + "TAKE YOUR POSITIONS" + hold (30s AR / 10s AP)
    PrepSighting,     // 5:00 combined preparation and sighting (unlimited)
    Series1,          // shots 1-5, 250 s on command
    Series2,          // shots 6-10, 250 s on command
    Singles,          // shots 11-24, 50 s each on command (index 1..14)
    Complete,
    Aborted
};

enum class WindowState { Closed = 0, SightingOpen, MatchOpen };

enum class TargetMode { Sighter = 0, Match };

// Stable internal command identifiers. Displayed/spoken wording is the ISSF
// rulebook 6.17.2 text (provisional operator text — the authoritative detailed
// timings/announcements are the separate ISSF HQ "Commands and Announcements
// for Finals" document, not yet obtained; see the rule-gap report). No audio
// assets are shipped in F1.
enum class CommandType {
    AthletesToLine = 0,
    TakeYourPositions,
    PreparationSightingStart,
    ThirtySeconds,
    Stop,
    Unload,
    LoadSeries,
    StartSeries,
    LoadSingle,
    StartSingle,
    ResultsFinal,
    InfoNotice,          // announcer information lines
    CheckpointNotice     // single-athlete course checkpoint (never a placement)
};

enum class CeremonyMode {
    Full = 0,   // introductions + hold
    Short,      // hold only
    Skip        // straight to preparation/sighting
};

enum class StageStatus {
    NotStarted = 0,
    InProgress,
    Complete,
    Incomplete,   // window/stage expired with expected shots unfired
    Aborted
};

enum class RejectReason {
    WindowClosed = 0,
    WrongTargetMode,
    StageShotLimitReached,
    DuplicateShot,
    InvalidShotData,
    FinalsNotActive,
    // An unresolved EST incident requires an authorised decision before
    // official shots may continue (generic Phase-E authority model).
    EstIncidentBlocked
};

inline QString stageName(Stage s)
{
    switch (s) {
    case Stage::Idle:         return QStringLiteral("Idle");
    case Stage::Presentation: return QStringLiteral("Presentation");
    case Stage::PrepSighting: return QStringLiteral("PrepSighting");
    case Stage::Series1:      return QStringLiteral("Series1");
    case Stage::Series2:      return QStringLiteral("Series2");
    case Stage::Singles:      return QStringLiteral("Singles");
    case Stage::Complete:     return QStringLiteral("Complete");
    case Stage::Aborted:      return QStringLiteral("Aborted");
    }
    return QStringLiteral("Unknown");
}

inline QString stageStatusName(StageStatus s)
{
    switch (s) {
    case StageStatus::NotStarted: return QStringLiteral("NotStarted");
    case StageStatus::InProgress: return QStringLiteral("InProgress");
    case StageStatus::Complete:   return QStringLiteral("Complete");
    case StageStatus::Incomplete: return QStringLiteral("Incomplete");
    case StageStatus::Aborted:    return QStringLiteral("Aborted");
    }
    return QStringLiteral("Unknown");
}

inline QString commandTypeName(CommandType c)
{
    switch (c) {
    case CommandType::AthletesToLine:           return QStringLiteral("AthletesToLine");
    case CommandType::TakeYourPositions:        return QStringLiteral("TakeYourPositions");
    case CommandType::PreparationSightingStart: return QStringLiteral("PreparationSightingStart");
    case CommandType::ThirtySeconds:            return QStringLiteral("ThirtySeconds");
    case CommandType::Stop:                     return QStringLiteral("Stop");
    case CommandType::Unload:                   return QStringLiteral("Unload");
    case CommandType::LoadSeries:               return QStringLiteral("LoadSeries");
    case CommandType::StartSeries:              return QStringLiteral("StartSeries");
    case CommandType::LoadSingle:               return QStringLiteral("LoadSingle");
    case CommandType::StartSingle:              return QStringLiteral("StartSingle");
    case CommandType::ResultsFinal:             return QStringLiteral("ResultsFinal");
    case CommandType::InfoNotice:               return QStringLiteral("InfoNotice");
    case CommandType::CheckpointNotice:         return QStringLiteral("CheckpointNotice");
    }
    return QStringLiteral("Unknown");
}

inline QString rejectReasonName(RejectReason r)
{
    switch (r) {
    case RejectReason::WindowClosed:          return QStringLiteral("WindowClosed");
    case RejectReason::WrongTargetMode:       return QStringLiteral("WrongTargetMode");
    case RejectReason::StageShotLimitReached: return QStringLiteral("StageShotLimitReached");
    case RejectReason::DuplicateShot:         return QStringLiteral("DuplicateShot");
    case RejectReason::InvalidShotData:       return QStringLiteral("InvalidShotData");
    case RejectReason::FinalsNotActive:       return QStringLiteral("FinalsNotActive");
    case RejectReason::EstIncidentBlocked:    return QStringLiteral("EstIncidentBlocked");
    }
    return QStringLiteral("Unknown");
}

// Fine-grained reducer/event stage id (distinct per firing stage so per-series
// and per-single subtotals stay separate; no collision with prep=1):
//   PrepSighting = 1, Series1 = 2, Series2 = 3, Single n = 3 + n (4..17).
inline int fineStageId(Stage s, int singleIndex)
{
    switch (s) {
    case Stage::PrepSighting: return 1;
    case Stage::Series1:      return 2;
    case Stage::Series2:      return 3;
    case Stage::Singles:      return 3 + singleIndex;   // singleIndex 1..14
    default:                  return 0;
    }
}

// LOCKED once shipped (mirrors the 3P finalsSeriesIndex discipline; a separate
// schema, it does NOT touch the 3P one): sighting = -1, Series1 = 0,
// Series2 = 1, Single n = 1 + n (2..15).
inline int seriesIndexFor(Stage s, int singleIndex)
{
    switch (s) {
    case Stage::Series1: return 0;
    case Stage::Series2: return 1;
    case Stage::Singles: return 1 + singleIndex;        // singleIndex 1..14
    default:             return -1;
    }
}

} // namespace finals10m
} // namespace techaim
