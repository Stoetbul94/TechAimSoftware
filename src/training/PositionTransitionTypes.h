#ifndef TA_TRAINING_POSITIONTRANSITIONTYPES_H
#define TA_TRAINING_POSITIONTRANSITIONTYPES_H

// Position Transition (T4) — programme configuration, position sequence and the
// neutral per-position setup checklists. QtCore-only, no GUI. 50m Rifle 3
// Positions ONLY. It measures repeatable position rebuilding (setup timing,
// sighters, first-shot readiness, early-group quality); it never diagnoses a
// technical cause. Checklists record whether the athlete CONSCIOUSLY checked a
// setup element — checking an item never "proves" the setup was correct.

#include <QString>
#include <QStringList>
#include <QVector>

#include "TrainingProgramTypes.h"   // Position / positionLabel

namespace ta {
namespace training {

inline constexpr const char* kProgramPositionTransition = "position_transition";

inline constexpr int kPtMinShots = 3;
inline constexpr int kPtMaxShots = 10;
inline constexpr int kPtMinRepeats = 1;
inline constexpr int kPtMaxRepeats = 4;

// Neutral, position-specific setup confirmations (measured awareness, not a
// diagnosis). Kept here so QML never invents its own wording.
inline QStringList positionChecklist(Position p)
{
    switch (p) {
    case Position::Kneeling:
        return { QStringLiteral("Rifle / sling configuration checked"),
                 QStringLiteral("Kneeling roll placement checked"),
                 QStringLiteral("Support elbow position checked"),
                 QStringLiteral("Head / eye position checked"),
                 QStringLiteral("Natural alignment checked"),
                 QStringLiteral("Comfort and stability checked") };
    case Position::Prone:
        return { QStringLiteral("Sling configuration checked"),
                 QStringLiteral("Body / rifle alignment checked"),
                 QStringLiteral("Support elbow placement checked"),
                 QStringLiteral("Shoulder contact checked"),
                 QStringLiteral("Head / eye position checked"),
                 QStringLiteral("Natural alignment checked") };
    case Position::Standing:
        return { QStringLiteral("Foot position checked"),
                 QStringLiteral("Balance checked"),
                 QStringLiteral("Support arm / contact checked"),
                 QStringLiteral("Shoulder contact checked"),
                 QStringLiteral("Head / eye position checked"),
                 QStringLiteral("Natural alignment checked") };
    }
    return {};
}

struct PositionTransitionConfig {
    QVector<int> sequence;        // position ids in order (0 K, 1 P, 2 S)
    int verificationShots = 5;
    int repeats = 1;
    int checklistMode = 0;        // 0 self-check, 1 coach-assisted, 2 disabled
    QString technicalFocus;
    QString estimatedTime = QStringLiteral("30–45 min");

    static PositionTransitionConfig defaults()
    {
        PositionTransitionConfig c;
        c.sequence = { 0, 1, 2 };     // Kneeling → Prone → Standing
        c.verificationShots = 5;
        c.repeats = 1;
        return c;
    }

    QString sequenceString() const   // "K,P,S"
    {
        QStringList ids;
        for (int p : sequence)
            ids << positionLabel(static_cast<Position>(qBound(0, p, 2))).left(1);
        return ids.join(QLatin1Char(','));
    }
    QString sequenceArrowText() const  // "KNEELING → PRONE → STANDING"
    {
        QStringList names;
        for (int p : sequence)
            names << positionLabel(static_cast<Position>(qBound(0, p, 2))).toUpper();
        return names.join(QStringLiteral(" → "));
    }

    static QVector<int> parseSequence(const QString& s)
    {
        QVector<int> out;
        const QStringList parts = s.split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& p : parts) {
            const QChar c = p.trimmed().isEmpty() ? QChar() : p.trimmed().at(0).toUpper();
            if (c == QLatin1Char('K')) out.append(0);
            else if (c == QLatin1Char('P')) out.append(1);
            else if (c == QLatin1Char('S')) out.append(2);
        }
        return out;
    }

    QString validate() const
    {
        if (sequence.isEmpty())
            return QStringLiteral("Select at least one position.");
        for (int p : sequence)
            if (p < 0 || p > 2) return QStringLiteral("Invalid position in the sequence.");
        if (verificationShots < kPtMinShots || verificationShots > kPtMaxShots)
            return QStringLiteral("Verification shots must be between %1 and %2.")
                .arg(kPtMinShots).arg(kPtMaxShots);
        if (repeats < kPtMinRepeats || repeats > kPtMaxRepeats)
            return QStringLiteral("Repeats must be between %1 and %2.")
                .arg(kPtMinRepeats).arg(kPtMaxRepeats);
        return QString();
    }
};

}} // namespace ta::training

#endif // TA_TRAINING_POSITIONTRANSITIONTYPES_H
