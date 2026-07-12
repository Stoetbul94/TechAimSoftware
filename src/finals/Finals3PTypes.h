#pragma once
// 3P FINAL — shared types for the dedicated finals domain.
// ISSF Rule Book 2026 Edition 2025, Second Print 07/2026, effective 1 July 2026.
// See docs/3p-finals-discipline.md. This domain is fully separate from
// qualification (is3PMatch) and every other discipline.

#include <QString>

namespace techaim {
namespace finals {

// Order matters: stage progression and the UI stepper derive from it.
enum class Stage {
    Idle = 0,
    Ceremony,            // intro + "TAKE YOUR POSITIONS" + 30 s hold
    KneelingPrepSight,   // 5:00 combined preparation and sighting
    KneelingMatch,       // shots 1-10 (22:00 stage-1 clock)
    ProneSighting,       // athlete-initiated, unlimited
    ProneMatch,          // shots 11-20
    StandingSighting,    // unlimited, remaining stage-1 time
    StandingSeries1,     // shots 21-25, 250 s on command
    StandingSeries2,     // shots 26-30, 250 s on command
    StandingSingle1,     // shot 31, 50 s on command
    StandingSingle2,     // shot 32
    StandingSingle3,     // shot 33
    StandingSingle4,     // shot 34
    StandingSingle5,     // shot 35
    Complete,
    Aborted
};

enum class WindowState {
    Closed = 0,
    SightingOpen,
    MatchOpen
};

enum class TargetMode {
    Sighter = 0,
    Match
};

enum class CommandType {
    AthletesToLine = 0,
    TakeYourPositions,
    PreparationSightingStart,
    ThirtySeconds,
    Stop,
    StageOneAnnouncement,
    MatchFiringStart,
    FiveMinutes,
    LoadSeries,
    StartSeries,
    LoadSingle,
    StartSingle,
    Unload,
    ResultsFinal,
    InfoNotice           // announcer/elimination information lines
};

enum class CeremonyMode {
    Full = 0,   // introductions + 30 s hold
    Short,      // 30 s hold only
    Skip        // straight to preparation/sighting
};

enum class RejectReason {
    WindowClosed = 0,
    WrongTargetMode,
    StageShotLimitReached,
    DuplicateShot,
    InvalidShotData,
    FinalsNotActive
};

inline QString rejectReasonName(RejectReason r)
{
    switch (r) {
    case RejectReason::WindowClosed:          return QStringLiteral("WindowClosed");
    case RejectReason::WrongTargetMode:       return QStringLiteral("WrongTargetMode");
    case RejectReason::StageShotLimitReached: return QStringLiteral("StageShotLimitReached");
    case RejectReason::DuplicateShot:         return QStringLiteral("DuplicateShot");
    case RejectReason::InvalidShotData:       return QStringLiteral("InvalidShotData");
    case RejectReason::FinalsNotActive:       return QStringLiteral("FinalsNotActive");
    }
    return QStringLiteral("Unknown");
}

// Position role value compatible with the qualification models (0=K 1=P 2=S).
inline int positionRoleFor(Stage s)
{
    switch (s) {
    case Stage::Ceremony:
    case Stage::KneelingPrepSight:
    case Stage::KneelingMatch:     return 0;
    case Stage::ProneSighting:
    case Stage::ProneMatch:        return 1;
    default:                       return 2;
    }
}

// Finals series index: 0 K, 1 P, 2 S1, 3 S2, 4..8 singles 31..35 (-1 sighting).
inline int seriesIndexFor(Stage s)
{
    switch (s) {
    case Stage::KneelingMatch:     return 0;
    case Stage::ProneMatch:        return 1;
    case Stage::StandingSeries1:   return 2;
    case Stage::StandingSeries2:   return 3;
    case Stage::StandingSingle1:   return 4;
    case Stage::StandingSingle2:   return 5;
    case Stage::StandingSingle3:   return 6;
    case Stage::StandingSingle4:   return 7;
    case Stage::StandingSingle5:   return 8;
    default:                       return -1;
    }
}

inline QString stageName(Stage s)
{
    switch (s) {
    case Stage::Idle:              return QStringLiteral("Idle");
    case Stage::Ceremony:          return QStringLiteral("Ceremony");
    case Stage::KneelingPrepSight: return QStringLiteral("KneelingPrepSight");
    case Stage::KneelingMatch:     return QStringLiteral("KneelingMatch");
    case Stage::ProneSighting:     return QStringLiteral("ProneSighting");
    case Stage::ProneMatch:        return QStringLiteral("ProneMatch");
    case Stage::StandingSighting:  return QStringLiteral("StandingSighting");
    case Stage::StandingSeries1:   return QStringLiteral("StandingSeries1");
    case Stage::StandingSeries2:   return QStringLiteral("StandingSeries2");
    case Stage::StandingSingle1:   return QStringLiteral("StandingSingle1");
    case Stage::StandingSingle2:   return QStringLiteral("StandingSingle2");
    case Stage::StandingSingle3:   return QStringLiteral("StandingSingle3");
    case Stage::StandingSingle4:   return QStringLiteral("StandingSingle4");
    case Stage::StandingSingle5:   return QStringLiteral("StandingSingle5");
    case Stage::Complete:          return QStringLiteral("Complete");
    case Stage::Aborted:           return QStringLiteral("Aborted");
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
    case CommandType::StageOneAnnouncement:     return QStringLiteral("StageOneAnnouncement");
    case CommandType::MatchFiringStart:         return QStringLiteral("MatchFiringStart");
    case CommandType::FiveMinutes:              return QStringLiteral("FiveMinutes");
    case CommandType::LoadSeries:               return QStringLiteral("LoadSeries");
    case CommandType::StartSeries:              return QStringLiteral("StartSeries");
    case CommandType::LoadSingle:               return QStringLiteral("LoadSingle");
    case CommandType::StartSingle:              return QStringLiteral("StartSingle");
    case CommandType::Unload:                   return QStringLiteral("Unload");
    case CommandType::ResultsFinal:             return QStringLiteral("ResultsFinal");
    case CommandType::InfoNotice:               return QStringLiteral("InfoNotice");
    }
    return QStringLiteral("Unknown");
}

} // namespace finals
} // namespace techaim
