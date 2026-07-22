#ifndef TA_TRAINING_CALLDIAGNOSETYPES_H
#define TA_TRAINING_CALLDIAGNOSETYPES_H

// Call & Diagnose (T2) — programme configuration, per-discipline defaults and
// validation. QtCore-only, no GUI. A second Training Lab programme: the athlete
// CALLS each shot before the actual impact is revealed; the programme measures
// shot AWARENESS (call vs actual) and never diagnoses a technical cause.

#include <QString>

#include "reliability/events/EventTypes.h"    // ta::rel::Discipline
#include "TrainingProgramTypes.h"              // reuse Position / positionLabel / focus

namespace ta {
namespace training {

inline constexpr const char* kProgramCallDiagnose = "call_and_diagnose";

// Guardrails: at least a handful of shots, a sane maximum per position.
inline constexpr int kCdMinShots = 5;
inline constexpr int kCdMaxShots = 60;

struct CallDiagnoseConfig {
    ta::rel::Discipline discipline = ta::rel::Discipline::None;
    int shotCount = 0;              // called shots per position (or total, non-3P)
    QString technicalFocus;
    bool threePositions = false;    // 50m 3P: Kneeling → Prone → Standing
    QString estimatedTime;

    int totalShots() const { return threePositions ? shotCount * 3 : shotCount; }

    // For 3P the calling phases run K → P → S; non-3P is a single position.
    Position positionForIndex(int posIndex) const
    {
        if (!threePositions) return Position::Kneeling;
        return static_cast<Position>(qBound(0, posIndex, 2));
    }

    QString validate() const
    {
        if (discipline == ta::rel::Discipline::None)
            return QStringLiteral("Select a discipline first.");
        if (shotCount < kCdMinShots || shotCount > kCdMaxShots)
            return QStringLiteral("Called shots must be between %1 and %2.")
                .arg(kCdMinShots).arg(kCdMaxShots);
        return QString();
    }

    static CallDiagnoseConfig defaultsFor(ta::rel::Discipline d)
    {
        CallDiagnoseConfig c;
        c.discipline = d;
        switch (d) {
        case ta::rel::Discipline::AirRifle10m:
        case ta::rel::Discipline::AirPistol10m:
            c.shotCount = 20;
            c.estimatedTime = QStringLiteral("25–35 min");
            break;
        case ta::rel::Discipline::Prone50m:
            c.shotCount = 20;
            c.estimatedTime = QStringLiteral("30–40 min");
            break;
        case ta::rel::Discipline::ThreePositions50m:
            c.threePositions = true;
            c.shotCount = 10;          // per position → 30 called shots
            c.estimatedTime = QStringLiteral("45–60 min");
            break;
        default:
            break;
        }
        return c;
    }
};

}} // namespace ta::training

#endif // TA_TRAINING_CALLDIAGNOSETYPES_H
