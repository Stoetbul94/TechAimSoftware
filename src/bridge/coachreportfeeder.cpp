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
    const MatchArrays arrays = readGameMode(gameSubMode);
    const std::vector<techaim::analytics::ShotAnalyticsData> shots = buildShots(arrays);

    // Capture debug facts before analysis (from what was actually built).
    m_dbgShotCount   = static_cast<int>(shots.size());
    m_dbgHasCoords   = !arrays.xs.empty();
    m_dbgHasTiming   = !shots.empty() && shots.front().hasTimestamp;
    m_dbgIs3P        = arrays.is3P;
    m_dbgSinglePos   = QString::fromStdString(techaim::analytics::toString(arrays.singlePosition));
    m_dbgGameSubMode = gameSubMode;
    m_dbgPositions.clear();
    for (const auto& s : shots) {
        const QString p = QString::fromStdString(techaim::analytics::toString(s.positionType));
        if (!m_dbgPositions.contains(p)) m_dbgPositions << p;
    }
    m_dbgRan = true;

    m_bridge->analyze(shots);
    return m_bridge->valid();
}

QVariantMap CoachReportFeeder::debugInfo() const
{
    QVariantMap m;
    m["ran"]              = m_dbgRan;
    m["valid"]            = m_bridge ? m_bridge->valid() : false;
    m["shotCount"]        = m_dbgShotCount;
    m["hasCoordinates"]   = m_dbgHasCoords;
    m["hasTiming"]        = m_dbgHasTiming;
    m["is3P"]             = m_dbgIs3P;
    m["discipline"]       = m_dbgIs3P ? QStringLiteral("3P (fallback thirds)") : m_dbgSinglePos;
    m["gameSubMode"]      = m_dbgGameSubMode;
    m["positions"]        = m_dbgPositions;
    m["coordinatesFlipY"] = m_flipY;
    return m;
}
