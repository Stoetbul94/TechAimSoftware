#include "coachreportfeeder.h"
#include "coachreportbridge.h"
#include "../../ModReader/forms/tachuswidget.h"
#include <cmath>

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

bool CoachReportFeeder::analyzeDemoMatch(int kind)
{
    return runArrays(makeDemoArrays(kind), kind == 1 ? 1 : 0, true);
}

// TEMPORARY / DEV-ONLY sample data. Both variants carry coordinates + timing +
// poor shots that recover, enough for every section to populate. Remove with
// the demo buttons once the live hardware test passes.
techaim::bridge::MatchArrays CoachReportFeeder::makeDemoArrays(int kind) const
{
    using techaim::analytics::PositionType;
    MatchArrays a;
    a.shotsPerSeries = 10;
    a.flipY = m_flipY;   // honour the current toggle so Flip Y is testable here too

    // Deterministic 2-D scatter (golden-angle spiral) so the group looks like a
    // real cluster on the target and heat map — no RNG (engine stays deterministic).
    const double GOLDEN = 2.39996323;
    if (kind == 1) {
        // 50m 3P: 60 shots, equal thirds K -> P -> S. Prone tightest/best,
        // Standing widest/worst, so the position comparison is meaningful.
        a.is3P = true;
        a.singlePosition = PositionType::Unknown;
        for (int i = 0; i < 60; ++i) {
            double sc, spread, cyc; bool poor = false;
            if (i < 20)      { sc = (i % 2) ? 10.1 : 10.3; spread = 7.0;  cyc = 1.0; }        // Kneeling
            else if (i < 40) { sc = (i % 2) ? 10.5 : 10.7; spread = 4.0;  cyc = 1.0; }        // Prone
            else {                                                                            // Standing
                const int k = i - 40;
                poor = (k % 5 == 0);
                sc = poor ? 9.3 : ((k % 2) ? 9.9 : 10.2);
                spread = 13.0; cyc = -1.0;
            }
            const double ang = i * GOLDEN;
            const double rr = (poor ? spread * 1.9 : spread * std::sqrt(((i % 6) + 1) / 6.0));
            a.scores.push_back(sc);
            a.xs.push_back(rr * std::cos(ang));
            a.ys.push_back(cyc + rr * std::sin(ang));
            a.intervals.push_back(i == 0 ? -1.0 : 30.0);
        }
    } else {
        // 50m Prone: 40 shots, tight 2-D group, three poor shots that recover.
        a.is3P = false;
        a.singlePosition = PositionType::Prone;
        for (int i = 0; i < 40; ++i) {
            const bool poor = (i == 7 || i == 18 || i == 29);
            const double ang = i * GOLDEN;
            const double rr = poor ? 14.0 : 4.5 * std::sqrt(((i % 6) + 1) / 6.0);
            a.scores.push_back(poor ? 9.2 : ((i % 2) ? 10.5 : 10.6));
            a.xs.push_back(rr * std::cos(ang));
            a.ys.push_back(1.0 + rr * std::sin(ang));
            a.intervals.push_back(i == 0 ? -1.0 : 30.0);
        }
    }
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
