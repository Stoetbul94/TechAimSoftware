#include "coachreportfeeder.h"
#include "coachreportbridge.h"
#include "../../ModReader/forms/tachuswidget.h"

using namespace techaim::bridge;

CoachReportFeeder::CoachReportFeeder(TachusWidget* widget, CoachReportBridge* bridge, QObject* parent)
    : QObject(parent), m_widget(widget), m_bridge(bridge)
{
}

void CoachReportFeeder::setCoordinatesFlipY(bool f)
{
    if (m_flipY == f) return;
    m_flipY = f;
    emit flipYChanged();
}

int CoachReportFeeder::matchShotCount() const
{
    if (!m_widget) return 0;
    int n = m_widget->getCurrentMatchTotalShotsCount();
    if (n <= 0) n = m_widget->getShootCount();   // fall back to the live counter
    return n < 0 ? 0 : n;
}

MatchArrays CoachReportFeeder::readGameMode(int gameSubMode) const
{
    MatchArrays a;
    if (!m_widget) return a;

    const int n = matchShotCount();
    if (n <= 0) return a;

    a.shotsPerSeries = m_widget->getShotPerSeries();
    const DisciplineInfo d =
        resolveDiscipline(m_widget->getGame_range(), m_widget->getGamemode(), gameSubMode);
    a.is3P           = d.is3P;
    a.singlePosition = d.singlePosition;
    a.flipY          = m_flipY;

    // ACQ-SENTINEL-003. This used to compare the first shot's coordinates
    // against minus one, which was correct while minus one was the "no
    // coordinate" marker and became silently WRONG the moment the accessors
    // started answering NaN. A NaN compares equal to nothing, including that
    // marker, so haveCoords was true even when the application held no
    // coordinates at all, and the loop below pushed NaN into the analytics
    // engine - MPI, group size and every derived figure on the Coach Report.
    //
    // Ask the authority instead of recognising a number. The first shot is the
    // cheap probe, and each shot is checked again below, because a match whose
    // capture stopped part-way must contribute the shots it has and nothing it
    // does not.
    const bool haveCoords = m_widget->coordinateHasValue(1);

    a.scores.reserve(static_cast<size_t>(n));
    if (haveCoords) { a.xs.reserve(static_cast<size_t>(n)); a.ys.reserve(static_cast<size_t>(n)); }
    a.intervals.reserve(static_cast<size_t>(n));

    for (int i = 1; i <= n; ++i) {
        a.scores.push_back(m_widget->getScore(i));
        if (haveCoords) {
            // Per shot, not once for the match: capture can stop part-way, and
            // a NaN reaching the analytics engine would poison every aggregate
            // computed from it.
            if (!m_widget->coordinateHasValue(i))
                break;
            a.xs.push_back(m_widget->getXCord(i));
            a.ys.push_back(m_widget->getYCord(i));
        }
        a.intervals.push_back(m_widget->getTime(i));   // per-shot time consumed; getTime(i)=list[i-1]
    }
    return a;
}

bool CoachReportFeeder::analyzeCurrentMatch(int gameSubMode)
{
    if (!m_widget || !m_bridge) return false;
    m_bridge->analyze(buildShots(readGameMode(gameSubMode)));
    return m_bridge->valid();
}
