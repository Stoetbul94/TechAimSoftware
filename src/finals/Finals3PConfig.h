#pragma once
// 3P FINAL — configuration. All rule durations in one place (milliseconds,
// unscaled — the controller applies the developer timeScale on top).
// ISSF Rule Book 2026 Edition 2025, Second Print 07/2026, effective 1 July 2026.

#include "Finals3PTypes.h"

namespace techaim {
namespace finals {

struct Finals3PConfig {
    CeremonyMode ceremonyMode = CeremonyMode::Full;

    // Training-mode behaviour switches (see design doc).
    bool stopSeriesWhenAthleteCompletes = true;   // real CRO waits for all finalists
    bool fillUnfiredShotsWithZero      = false;   // [P1 — UNRESOLVED] keep disabled

    // Rule durations.
    qint64 introMs        = 20000;    // ceremony introductions (simulation length)
    qint64 holdMs         = 30000;    // "TAKE YOUR POSITIONS" kneeling hold
    qint64 prepSightMs    = 300000;   // 5:00 combined preparation and sighting
    qint64 prepWarnMs     = 270000;   // "30 SECONDS" at 4:30 elapsed
    qint64 stage1Ms       = 1320000;  // 22:00 kneeling/prone/standing-sight block
    qint64 stage1Warn1Ms  = 1020000;  // "FIVE MINUTES" at elapsed 17:00
    qint64 stage1Warn2Ms  = 1290000;  // "THIRTY SECONDS" at elapsed 21:30
    qint64 loadDelayMs    = 5000;     // LOAD -> START
    qint64 seriesMs       = 250000;   // each 5-shot standing series
    qint64 singleMs       = 50000;    // each standing single shot
    qint64 preSeriesGapMs = 30000;    // gap after stage-1 STOP before series 1 LOAD
    qint64 betweenGapMs   = 15000;    // announcer gap between commanded windows

    // Shot limits per stage.
    int kneelingShots = 10;
    int proneShots    = 10;
    int seriesShots   = 5;
    int singleShots   = 1;
};

} // namespace finals
} // namespace techaim
