#ifndef TA_TRAINING_PROGRAMTYPES_H
#define TA_TRAINING_PROGRAMTYPES_H

// Training Lab (T1) — pure programme types, per-discipline defaults and
// configuration validation for Technical Blocks. QtCore-only, no GUI.
//
// Source of truth: TechAim Training System — Refined Research Specification
// v0.2 (2026-07-20). Technical Blocks is Release-1 programme #1; the types
// here deliberately anticipate Call & Diagnose (T2) without implementing it.

#include <QString>
#include <QStringList>

#include "reliability/events/EventTypes.h"   // ta::rel::Discipline

namespace ta {
namespace training {

inline constexpr const char* kProgramTechnicalBlocks = "technical_blocks";

// Athlete-facing visibility of the live block (controller-owned projection —
// the journal ALWAYS stores measured values; these only gate what is shown).
enum class Visibility : quint8 {
    FullHidden = 0,      // Mode A: no score, no impact, no group during block
    GroupOnly = 1,       // Mode B: markers/group shape only, no numbers
    ImpactNoScore = 2    // Mode C: impacts visible, numerical score hidden
};

inline QString visibilityLabel(Visibility v)
{
    switch (v) {
    case Visibility::FullHidden:    return QStringLiteral("Full hidden");
    case Visibility::GroupOnly:     return QStringLiteral("Group only");
    case Visibility::ImpactNoScore: return QStringLiteral("Impact, no score");
    }
    return QStringLiteral("Full hidden");
}

// 3P block positions — explicit order and identity (never merged).
enum class Position : quint8 { Kneeling = 0, Prone = 1, Standing = 2 };

inline QString positionLabel(Position p)
{
    switch (p) {
    case Position::Kneeling: return QStringLiteral("Kneeling");
    case Position::Prone:    return QStringLiteral("Prone");
    case Position::Standing: return QStringLiteral("Standing");
    }
    return QString();
}

// One technical focus per session — an athlete INTENTION, not a diagnosis.
// The UI must never claim the focus caused a particular shot.
inline QStringList focusOptions(bool isPistol)
{
    QStringList o{
        QStringLiteral("Hold"),
        QStringLiteral("Aim"),
        QStringLiteral("Trigger"),
        QStringLiteral("Follow-through"),
        QStringLiteral("Natural point of aim"),
        QStringLiteral("Balance"),
        QStringLiteral("Rhythm"),
    };
    // Rifle-only contact terminology is not foregrounded for pistol.
    if (!isPistol) {
        o.insert(5, QStringLiteral("Head position"));
        o.insert(6, QStringLiteral("Shoulder contact"));
    }
    return o;
}

// Configuration guardrails (spec: no zero/negative, no overflow, no silent
// correction — invalid configs are REJECTED with a message, never adjusted).
inline constexpr int kMinBlocks = 1;
inline constexpr int kMaxBlocks = 12;
inline constexpr int kMinShotsPerBlock = 3;
inline constexpr int kMaxShotsPerBlock = 20;
inline constexpr int kMaxTotalShots = 120;   // reasonable Training maximum

struct TechnicalBlocksConfig {
    ta::rel::Discipline discipline = ta::rel::Discipline::None;
    int blockCount = 0;
    int shotsPerBlock = 0;
    Visibility visibility = Visibility::FullHidden;
    QString technicalFocus;
    // 3P: blocks rotate Kneeling → Prone → Standing, blocksPerPosition each.
    bool threePositions = false;
    int blocksPerPosition = 0;      // 3P only (blockCount = 3 * this)
    QString estimatedTime;          // athlete-facing hint, e.g. "35–45 min"

    int totalShots() const { return blockCount * shotsPerBlock; }

    // Position of a 1-based block index. Non-3P → Kneeling slot unused (0).
    Position positionForBlock(int blockIndex1) const
    {
        if (!threePositions || blocksPerPosition <= 0)
            return Position::Kneeling;
        const int group = (blockIndex1 - 1) / blocksPerPosition;   // 0,1,2
        return static_cast<Position>(qBound(0, group, 2));
    }

    // Validation: returns an empty string when valid, else the operator-facing
    // error. No silent correction of invalid values.
    QString validate() const
    {
        if (discipline == ta::rel::Discipline::None)
            return QStringLiteral("Select a discipline first.");
        if (blockCount < kMinBlocks || blockCount > kMaxBlocks)
            return QStringLiteral("Blocks must be between %1 and %2.")
                .arg(kMinBlocks).arg(kMaxBlocks);
        if (shotsPerBlock < kMinShotsPerBlock || shotsPerBlock > kMaxShotsPerBlock)
            return QStringLiteral("Shots per block must be between %1 and %2.")
                .arg(kMinShotsPerBlock).arg(kMaxShotsPerBlock);
        if (totalShots() > kMaxTotalShots)
            return QStringLiteral("Total shots (%1) exceed the training maximum of %2.")
                .arg(totalShots()).arg(kMaxTotalShots);
        if (threePositions && blockCount != 3 * blocksPerPosition)
            return QStringLiteral("3P blocks must divide evenly across Kneeling, Prone and Standing.");
        return QString();
    }

    // Per-discipline defaults (spec TECHNICAL BLOCK DEFAULTS).
    static TechnicalBlocksConfig defaultsFor(ta::rel::Discipline d)
    {
        TechnicalBlocksConfig c;
        c.discipline = d;
        switch (d) {
        case ta::rel::Discipline::AirRifle10m:
            c.blockCount = 5; c.shotsPerBlock = 6;
            c.estimatedTime = QStringLiteral("35–45 min");
            break;
        case ta::rel::Discipline::AirPistol10m:
            c.blockCount = 6; c.shotsPerBlock = 5;
            c.estimatedTime = QStringLiteral("35–45 min");
            break;
        case ta::rel::Discipline::Prone50m:
            c.blockCount = 5; c.shotsPerBlock = 6;
            c.estimatedTime = QStringLiteral("40–50 min");
            break;
        case ta::rel::Discipline::ThreePositions50m:
            // 2 blocks × 6 shots per position → 12 K + 12 P + 12 S = 36.
            c.threePositions = true; c.blocksPerPosition = 2;
            c.blockCount = 6; c.shotsPerBlock = 6;
            c.estimatedTime = QStringLiteral("50–70 min");
            break;
        default:
            break;
        }
        return c;
    }
};

}} // namespace ta::training

#endif // TA_TRAINING_PROGRAMTYPES_H
