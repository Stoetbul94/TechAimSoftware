#include "CompetitionState.h"

namespace ta {
namespace rms {

QString toString(CompetitionStatus s)
{
    switch (s) {
    case CompetitionStatus::Active:     return QStringLiteral("ACTIVE");
    case CompetitionStatus::Waiting:    return QStringLiteral("WAITING");
    case CompetitionStatus::Finished:   return QStringLiteral("FINISHED");
    case CompetitionStatus::Eliminated: return QStringLiteral("ELIMINATED");
    case CompetitionStatus::Unknown:    break;
    }
    return QStringLiteral("UNKNOWN");
}

CompetitionStatus competitionStatusFromString(const QString& s, bool* ok)
{
    if (ok) *ok = true;
    if (s == QLatin1String("ACTIVE"))     return CompetitionStatus::Active;
    if (s == QLatin1String("WAITING"))    return CompetitionStatus::Waiting;
    if (s == QLatin1String("FINISHED"))   return CompetitionStatus::Finished;
    if (s == QLatin1String("ELIMINATED")) return CompetitionStatus::Eliminated;
    // Anything else is not understood, and "not understood" is reported as
    // such. Guessing here would be the same mistake as inferring elimination.
    if (ok) *ok = false;
    return CompetitionStatus::Unknown;
}

QString CompetitionState::rankLabel() const
{
    if (rank <= 0)
        return QString();
    // English ordinals. A translated presentation formats its own; this is the
    // fallback the diagnostics and the text renderer use.
    const int lastTwo = rank % 100;
    if (lastTwo >= 11 && lastTwo <= 13)
        return QStringLiteral("%1th").arg(rank);
    switch (rank % 10) {
    case 1:  return QStringLiteral("%1st").arg(rank);
    case 2:  return QStringLiteral("%1nd").arg(rank);
    case 3:  return QStringLiteral("%1rd").arg(rank);
    default: return QStringLiteral("%1th").arg(rank);
    }
}

QString CompetitionState::scoreLabel() const
{
    // Reported or not — never computed. RMS does not add up a final score any
    // more than it adds up a qualification one.
    return finalScoreReported ? QString::number(finalScore, 'f', 1)
                              : QStringLiteral("—");
}

} // namespace rms
} // namespace ta
