#pragma once
// 10m Air Rifle / Air Pistol FINAL — configuration (F1).
//
// One config parameterises the shared Finals10mController for both disciplines
// (Option A, docs/10m-finals-architecture.md). The verified rules (6.17.2) are
// IDENTICAL for AR and AP except the post-"TAKE YOUR POSITIONS" hold
// (30 s Rifle / 10 s Pistol) and equipment/dress metadata. All rule durations
// live here in milliseconds (unscaled — the controller applies its developer
// timeScale on top).
//
// Rule-verified values (docs/issf-rules/10m-finals-shared.md):
//   • 5:00 combined preparation + sighting
//   • 2 series of 5 shots, 250 s each
//   • 14 single shots, 50 s each  → 24 shots total
//   • decimal (tenth-ring) scoring for BOTH disciplines
//   • checkpoints after shots 12/14/16/18/20/22/24
// Guideline values (rulebook 6.17.2 NOTE — authoritative detailed timings are
// the ISSF HQ "Commands and Announcements for Finals" doc, not yet obtained):
//   • intro window, 60 s post-prep gap, announcer gap, 5 s LOAD→START, warning.

#include "Finals10mTypes.h"
#include "../reliability/events/EventTypes.h"

#include <QString>
#include <QVector>

namespace techaim {
namespace finals10m {

struct Finals10mConfig {
    ta::rel::Discipline discipline = ta::rel::Discipline::AirRifleFinal10m;
    QString displayName = QStringLiteral("10m Air Rifle Final");
    QString matchType   = QStringLiteral("FINAL AR10");
    // 1 = rifle, 0 = pistol (matches loginPage.gameMode / target face select).
    int targetType = 1;

    CeremonyMode ceremonyMode = CeremonyMode::Full;
    bool stopWhenAthleteCompletes = true;   // real CRO waits for all finalists;
                                            // in single-athlete training the
                                            // window closes on completion

    // ── rule-verified course of fire (6.17.2) ────────────────────────────
    qint64 preparationSightingMs = 300000;  // 5:00 combined prep + sighting
    int    seriesCount           = 2;
    int    shotsPerSeries        = 5;
    qint64 seriesWindowMs        = 250000;   // 250 s per 5-shot series
    int    singleShotCount       = 14;
    qint64 singleShotWindowMs    = 50000;    // 50 s per single shot
    int    maximumMatchShots     = 24;       // 2×5 + 14
    bool   decimalScoring        = true;     // BOTH AR and AP finals

    // Single-athlete course checkpoints (NOT placements): after these shots.
    QVector<int> checkpointShots = {12, 14, 16, 18, 20, 22, 24};

    // ── discipline-specific (the ONLY verified course-of-fire difference) ─
    qint64 takePositionsDelayMs = 30000;     // 30 s Rifle / 10 s Pistol

    // ── guideline / provisional timings (rulebook says its timings are
    //    "guidelines"; refine when the ISSF HQ command doc is obtained) ────
    qint64 introMs        = 20000;   // presentation/introduction window
    qint64 holdSkipMs     = 3000;    // Short-ceremony hold before prep
    qint64 prepWarnMs     = 270000;  // "30 SECONDS" at 4:30 into prep+sighting
    qint64 loadDelayMs    = 5000;    // LOAD → START
    qint64 preSeriesGapMs = 60000;   // 60 s after prep STOP before series-1 LOAD
    qint64 betweenGapMs   = 15000;   // announcer gap between commanded windows

    static Finals10mConfig airRifle()
    {
        Finals10mConfig c;
        c.discipline = ta::rel::Discipline::AirRifleFinal10m;
        c.displayName = QStringLiteral("10m Air Rifle Final");
        c.matchType = QStringLiteral("FINAL AR10");
        c.targetType = 1;                 // rifle
        c.takePositionsDelayMs = 30000;   // 30 s (6.17.2 e)
        return c;
    }

    static Finals10mConfig airPistol()
    {
        Finals10mConfig c;
        c.discipline = ta::rel::Discipline::AirPistolFinal10m;
        c.displayName = QStringLiteral("10m Air Pistol Final");
        c.matchType = QStringLiteral("FINAL AP10");
        c.targetType = 0;                 // pistol
        c.takePositionsDelayMs = 10000;   // 10 s (6.17.2 e) — the ONLY
                                          // course-of-fire difference. Scoring
                                          // stays DECIMAL (never AP-qual integer).
        return c;
    }

    static Finals10mConfig forDiscipline(ta::rel::Discipline d)
    {
        return d == ta::rel::Discipline::AirPistolFinal10m ? airPistol()
                                                           : airRifle();
    }
};

} // namespace finals10m
} // namespace techaim
