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

bool CoachReportFeeder::runArrays(const MatchArrays& arrays, int gameSubMode, bool demo)
{
    if (!m_bridge) return false;
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
    m_dbgRan  = true;
    m_dbgDemo = demo;

    m_bridge->analyze(shots);
    return m_bridge->valid();
}

bool CoachReportFeeder::analyzeCurrentMatch(int gameSubMode)
{
    if (!m_widget || !m_bridge) return false;
    return runArrays(readGameMode(gameSubMode), gameSubMode, false);
}

bool CoachReportFeeder::analyzeDemoMatch()
{
    return runArrays(makeDemoArrays(), 0, true);
}

// TEMPORARY / DEV-ONLY sample data: 20 prone record shots with coordinates,
// timing intervals, four poor shots each followed by a recovery, a slight high
// bias, and a couple of rushed/delayed intervals — enough for every section to
// populate. Remove with the demo button once the live hardware test passes.
MatchArrays CoachReportFeeder::makeDemoArrays() const
{
    MatchArrays a;
    a.shotsPerSeries = 10;
    a.is3P = false;
    a.singlePosition = techaim::analytics::PositionType::Prone;
    a.flipY = m_flipY;   // honour the current toggle so Flip Y is testable here too
    a.scores = { 10.5, 10.6, 9.2, 10.7, 10.4, 10.6, 9.0, 10.8, 10.5, 10.3,
                 10.6, 9.4, 10.7, 10.5, 10.4, 10.6, 10.2, 9.1, 10.8, 10.5 };
    a.xs =     {  2.0, -1.0,  8.0,  1.0, -2.0,  3.0, -11.0,  0.0,  2.0, -3.0,
                  1.0,  9.0, -1.0,  2.0,  0.0, -2.0,  3.0, -8.0,  1.0,  0.0 };
    a.ys =     {  3.0,  2.0, -10.0, 4.0,  3.0,  1.0,  9.0,  2.0,  5.0,  1.0,
                  3.0,  7.0,  2.0,  4.0,  1.0,  3.0,  2.0, -9.0,  5.0,  3.0 };
    // intervals[0] is ignored (first shot anchors t=0); others must be > 0.
    a.intervals = { 0.0, 32.0, 28.0, 46.0, 30.0, 26.0, 55.0, 18.0, 31.0, 29.0,
                    33.0, 48.0, 27.0, 30.0, 25.0, 34.0, 29.0, 52.0, 19.0, 30.0 };
    return a;
}

QVariantMap CoachReportFeeder::debugInfo() const
{
    QVariantMap m;
    m["ran"]              = m_dbgRan;
    m["demo"]             = m_dbgDemo;
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
