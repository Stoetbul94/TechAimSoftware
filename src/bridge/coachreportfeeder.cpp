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

    // Coordinates carry a -1 sentinel when the target never reported them; only
    // include them when the first shot has a genuine coordinate.
    const bool haveCoords = !(m_widget->getXCord(1) == -1.0 && m_widget->getYCord(1) == -1.0);

    a.scores.reserve(static_cast<size_t>(n));
    if (haveCoords) { a.xs.reserve(static_cast<size_t>(n)); a.ys.reserve(static_cast<size_t>(n)); }
    a.intervals.reserve(static_cast<size_t>(n));

    for (int i = 1; i <= n; ++i) {
        a.scores.push_back(m_widget->getScore(i));
        if (haveCoords) {
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
