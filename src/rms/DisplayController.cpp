#include "DisplayController.h"

#include <QDateTime>
#include <QTimer>

#include <algorithm>

namespace ta {
namespace rms {

QString toString(DisplayMode m)
{
    switch (m) {
    case DisplayMode::SingleTarget:  return QStringLiteral("SINGLE_TARGET");
    case DisplayMode::RotateTargets: return QStringLiteral("ROTATE_TARGETS");
    case DisplayMode::AllTargets:    break;
    }
    return QStringLiteral("ALL_TARGETS");
}

QString toString(LaneFilter f)
{
    return f == LaneFilter::Participating ? QStringLiteral("PARTICIPATING")
                                          : QStringLiteral("ALL_PHYSICAL");
}

DisplayController::DisplayController(RangeConfigurationService* range,
                                     MatchPlanService* plans, QObject* parent)
    : QObject(parent)
    , m_range(range)
    , m_plans(plans)
    , m_clock([] { return QDateTime::currentMSecsSinceEpoch(); })
{
    connect(m_range, &RangeConfigurationService::rangeChanged,
            this, &DisplayController::onRangeOrPlanChanged);
    connect(m_plans, &MatchPlanService::planChanged,
            this, &DisplayController::onRangeOrPlanChanged);
    onRangeOrPlanChanged();
}

DisplayController::~DisplayController() = default;

qint64 DisplayController::nowMs() const
{
    return m_clock ? m_clock() : 0;
}

bool DisplayController::participatingAvailable() const
{
    return m_plans && m_plans->hasPlan() && m_plans->selectedLaneCount() > 0;
}

QVector<int> DisplayController::laneOrderNumbers() const
{
    QVector<int> out;
    if (!m_range || !m_range->isConfigured())
        return out;

    if (m_filter == LaneFilter::Participating) {
        // Only the lanes actually in the match. NOT a fallback to everything:
        // an operator who chose "participating" and sees the whole range would
        // have no way to tell the filter had been ignored.
        for (const PlanLane& l : m_plans->current().lanes)
            out.append(l.laneNumber);
    } else {
        for (const LaneDefinition& l : m_range->range().lanes)
            out.append(l.laneNumber);
    }
    std::sort(out.begin(), out.end());
    return out;
}

QVariantList DisplayController::laneOrder() const
{
    QVariantList out;
    for (int n : laneOrderNumbers())
        out.append(n);
    return out;
}

bool DisplayController::isLaneInOrder(int laneNumber) const
{
    return laneOrderNumbers().contains(laneNumber);
}

int DisplayController::selectedIndex() const
{
    return int(laneOrderNumbers().indexOf(m_selectedLane));
}

QString DisplayController::emptyReason() const
{
    if (!m_range || !m_range->isConfigured())
        return QStringLiteral("No range is configured. Create one in Range Setup.");
    if (m_range->laneCount() == 0)
        return QStringLiteral("The range has no lanes.");
    if (m_filter == LaneFilter::Participating && !participatingAvailable())
        return QStringLiteral("No match plan has participating lanes. "
                              "Prepare one in New Match, or show all physical lanes.");
    if (laneOrderNumbers().isEmpty())
        return QStringLiteral("No lanes to display.");
    return QString();
}

void DisplayController::onRangeOrPlanChanged()
{
    // Until the operator picks a filter, follow the work: a prepared match
    // means the competition lanes are what matters; otherwise the range is.
    if (!m_filterChosenByOperator)
        m_filter = participatingAvailable() ? LaneFilter::Participating
                                            : LaneFilter::AllPhysical;
    clampSelection();
    emit changed();
}

void DisplayController::clampSelection()
{
    const QVector<int> order = laneOrderNumbers();
    if (order.isEmpty()) {
        m_selectedLane = -1;
        return;
    }
    // A lane that left the set (filter change, or the range was reconfigured
    // under us) must not leave the view pointing at nothing.
    if (!order.contains(m_selectedLane))
        m_selectedLane = order.first();
}

void DisplayController::setSelectedLaneInternal(int laneNumber)
{
    if (m_selectedLane == laneNumber)
        return;
    m_selectedLane = laneNumber;
    emit selectedLaneChanged(laneNumber);
}

void DisplayController::stopRotationForOperatorAction()
{
    if (!m_rotating)
        return;
    m_rotating = false;
    if (m_timer)
        m_timer->stop();
    if (m_mode == DisplayMode::RotateTargets)
        m_mode = DisplayMode::SingleTarget;
}

void DisplayController::showAllTargets()
{
    stopRotationForOperatorAction();
    m_mode = DisplayMode::AllTargets;
    // The selection is KEPT. Dipping into the overview and back must return to
    // the lane the operator was watching.
    emit changed();
}

void DisplayController::selectLane(int laneNumber)
{
    if (!isLaneInOrder(laneNumber))
        return;
    stopRotationForOperatorAction();
    setSelectedLaneInternal(laneNumber);
    m_mode = DisplayMode::SingleTarget;
    emit changed();
}

void DisplayController::advance(int delta)
{
    const QVector<int> order = laneOrderNumbers();
    if (order.isEmpty())
        return;
    int i = int(order.indexOf(m_selectedLane));
    if (i < 0)
        i = 0;
    else
        i = (i + delta % order.size() + order.size()) % order.size();
    setSelectedLaneInternal(order.at(i));
}

void DisplayController::next()
{
    stopRotationForOperatorAction();
    advance(1);
    if (m_mode == DisplayMode::AllTargets)
        m_mode = DisplayMode::SingleTarget;
    emit changed();
}

void DisplayController::previous()
{
    stopRotationForOperatorAction();
    advance(-1);
    if (m_mode == DisplayMode::AllTargets)
        m_mode = DisplayMode::SingleTarget;
    emit changed();
}

void DisplayController::setLaneFilterLabel(const QString& filter)
{
    const LaneFilter f = (filter == QLatin1String("PARTICIPATING"))
                             ? LaneFilter::Participating : LaneFilter::AllPhysical;
    if (m_filter == f) {
        m_filterChosenByOperator = true;
        return;
    }
    m_filter = f;
    // From here the operator owns the filter; the plan appearing or changing
    // no longer moves it under them.
    m_filterChosenByOperator = true;
    clampSelection();
    emit changed();
}

void DisplayController::setRotating(bool on)
{
    if (m_rotating == on)
        return;
    m_rotating = on;

    if (on) {
        if (laneOrderNumbers().isEmpty()) {
            m_rotating = false;
            emit changed();
            return;
        }
        clampSelection();
        m_mode = DisplayMode::RotateTargets;
        m_lastRotationMs = nowMs();
        if (!m_timer) {
            m_timer = new QTimer(this);
            // A fixed, modest tick. The interval is checked against the clock
            // rather than being the timer period, so changing the interval
            // never needs the timer restarted and never drifts.
            m_timer->setInterval(250);
            connect(m_timer, &QTimer::timeout, this, [this] { tickRotation(nowMs()); });
        }
        m_timer->start();
    } else {
        if (m_timer)
            m_timer->stop();
        if (m_mode == DisplayMode::RotateTargets)
            m_mode = DisplayMode::SingleTarget;
    }
    emit changed();
}

void DisplayController::setRotationIntervalMs(int ms)
{
    // Bounded: a one-second rotation is unreadable and a ten-minute one is not
    // a rotation.
    const int clamped = qBound(2000, ms, 600000);
    if (m_rotationIntervalMs == clamped)
        return;
    m_rotationIntervalMs = clamped;
    emit changed();
}

void DisplayController::setFullScreen(bool on)
{
    if (m_fullScreen == on)
        return;
    m_fullScreen = on;
    emit changed();
}

void DisplayController::leaveDisplays()
{
    // Leaving the page stops rotation. Nothing should keep changing state
    // behind a page nobody is looking at.
    const bool wasRotating = m_rotating;
    setRotating(false);
    if (m_fullScreen) {
        m_fullScreen = false;
        emit changed();
    } else if (!wasRotating) {
        emit changed();
    }
}

void DisplayController::tickRotation(qint64 nowUtcMs)
{
    if (!m_rotating)
        return;
    if (nowUtcMs - m_lastRotationMs < m_rotationIntervalMs)
        return;
    m_lastRotationMs = nowUtcMs;
    // The SAME ordered set previous/next walks, so rotation can never visit a
    // lane the buttons would skip.
    advance(1);
    emit changed();
}

} // namespace rms
} // namespace ta
