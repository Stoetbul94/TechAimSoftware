#ifndef TA_RMS_DISPLAYCONTROLLER_H
#define TA_RMS_DISPLAYCONTROLLER_H

// ─────────────────────────────────────────────────────────────────────────────
// WHAT THE TARGET DISPLAY IS SHOWING. Presentation state, and nothing else.
//
// Every value here is RMS-LOCAL. Choosing a lane, rotating, or going full
// screen changes what this operator is looking at; it sends nothing, and no
// station can tell the difference. There is no method on this class that a
// target could act on.
//
// ═══ TRAVERSAL IS DEFINED, NOT INCIDENTAL ══════════════════════════════════
//
// PREVIOUS / NEXT / rotation all walk the SAME ordered set: the lanes the
// current filter selects, in ascending lane-number order, wrapping at both
// ends. With a plan of lanes 1, 2, 4, 5, 8, 9 that is
//
//     1 → 2 → 4 → 5 → 8 → 9 → 1
//
// and never 1 → 2 → 3, because lane 3 is not in the match. One ordered set
// feeds all three controls so they can never disagree.
//
// ═══ ROTATION NEVER FIGHTS THE OPERATOR ════════════════════════════════════
//
// Auto-rotation STOPS on any deliberate operator action: selecting a lane,
// pressing previous or next, returning to ALL TARGETS, or leaving the display
// page. A timer that keeps moving the view after somebody has chosen what to
// look at is the single most annoying thing a control-room display can do.
// ─────────────────────────────────────────────────────────────────────────────

#include "MatchPlanService.h"
#include "RangeConfigurationService.h"

#include <QObject>
#include <QVariantList>
#include <QVector>

#include <functional>

class QTimer;

namespace ta {
namespace rms {

// ALL_TARGETS     the overview grid
// SINGLE_TARGET   one lane, large
// ROTATE_TARGETS  one lane, large, advancing on a timer
//
// FOLLOW_LEADER, LEADERBOARD, TOP_3, FINALS_DIRECTOR, RANGE_STATUS_ROTATION
// and SMART_TV_CLIENT are FUTURE modes. They are named in the architecture
// note and deliberately absent here.
enum class DisplayMode { AllTargets, SingleTarget, RotateTargets };

// ALL_PHYSICAL    every configured lane — the range as it exists
// PARTICIPATING   only the lanes in the current match plan
//
// Neither filter deletes a lane from the range model; a filter changes what is
// on screen and nothing else.
enum class LaneFilter { AllPhysical, Participating };

QString toString(DisplayMode m);
QString toString(LaneFilter f);

class DisplayController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ modeLabel NOTIFY changed)
    Q_PROPERTY(bool showingAll READ showingAll NOTIFY changed)
    Q_PROPERTY(QString laneFilter READ laneFilterLabel NOTIFY changed)
    Q_PROPERTY(int selectedLane READ selectedLane NOTIFY changed)
    Q_PROPERTY(int laneCount READ laneCount NOTIFY changed)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY changed)
    Q_PROPERTY(bool rotating READ isRotating NOTIFY changed)
    Q_PROPERTY(int rotationIntervalMs READ rotationIntervalMs NOTIFY changed)
    Q_PROPERTY(bool fullScreen READ isFullScreen NOTIFY changed)
    Q_PROPERTY(bool participatingAvailable READ participatingAvailable NOTIFY changed)
    Q_PROPERTY(QString emptyReason READ emptyReason NOTIFY changed)
    // The ordered lane set as a NOTIFYING property. A Q_INVOKABLE alone cannot
    // be a live QML model: nothing tells the binding to re-read it when the
    // range is configured, so a lane strip bound to the invokable stays empty
    // for the whole session.
    Q_PROPERTY(QVariantList laneOrderList READ laneOrder NOTIFY changed)

public:
    static constexpr int kDefaultRotationMs = 10000;

    DisplayController(RangeConfigurationService* range, MatchPlanService* plans,
                      QObject* parent = nullptr);
    ~DisplayController() override;

    // ── what is on screen ────────────────────────────────────────────────
    DisplayMode mode() const { return m_mode; }
    QString modeLabel() const { return toString(m_mode); }
    bool showingAll() const { return m_mode == DisplayMode::AllTargets; }
    LaneFilter laneFilter() const { return m_filter; }
    QString laneFilterLabel() const { return toString(m_filter); }
    int selectedLane() const { return m_selectedLane; }
    int laneCount() const { return int(laneOrderNumbers().size()); }
    int selectedIndex() const;
    bool isRotating() const { return m_rotating; }
    int rotationIntervalMs() const { return m_rotationIntervalMs; }
    bool isFullScreen() const { return m_fullScreen; }
    bool participatingAvailable() const;
    // Why the display has nothing to show, or empty when it has.
    QString emptyReason() const;

    // ── operator actions (all RMS-local) ─────────────────────────────────
    Q_INVOKABLE void showAllTargets();
    Q_INVOKABLE void selectLane(int laneNumber);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void setLaneFilterLabel(const QString& filter);
    Q_INVOKABLE void setRotating(bool on);
    Q_INVOKABLE void setRotationIntervalMs(int ms);
    Q_INVOKABLE void setFullScreen(bool on);
    // The display page was left. Rotation must not keep running behind a page
    // nobody is looking at.
    Q_INVOKABLE void leaveDisplays();

    // ── queries ──────────────────────────────────────────────────────────
    Q_INVOKABLE QVariantList laneOrder() const;
    Q_INVOKABLE bool isLaneInOrder(int laneNumber) const;
    // The ordered set traversal walks. Public for the models and the tests.
    QVector<int> laneOrderNumbers() const;

    // ── rotation, deterministically ──────────────────────────────────────
    // Production drives this from a QTimer; tests call it with a virtual clock
    // so no test ever sleeps.
    void tickRotation(qint64 nowMs);
    void setClockForTesting(std::function<qint64()> clock) { m_clock = std::move(clock); }
    qint64 lastRotationMs() const { return m_lastRotationMs; }

signals:
    void changed();
    void selectedLaneChanged(int laneNumber);

private slots:
    void onRangeOrPlanChanged();

private:
    void setSelectedLaneInternal(int laneNumber);
    void advance(int delta);
    void stopRotationForOperatorAction();
    void clampSelection();
    qint64 nowMs() const;

    RangeConfigurationService* m_range = nullptr;
    MatchPlanService* m_plans = nullptr;

    DisplayMode m_mode = DisplayMode::AllTargets;
    LaneFilter  m_filter = LaneFilter::AllPhysical;
    bool m_filterChosenByOperator = false;
    int  m_selectedLane = -1;
    bool m_rotating = false;
    bool m_fullScreen = false;
    int  m_rotationIntervalMs = kDefaultRotationMs;
    qint64 m_lastRotationMs = 0;

    QTimer* m_timer = nullptr;
    std::function<qint64()> m_clock;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_DISPLAYCONTROLLER_H
