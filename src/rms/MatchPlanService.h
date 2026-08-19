#ifndef TA_RMS_MATCHPLANSERVICE_H
#define TA_RMS_MATCHPLANSERVICE_H

// ─────────────────────────────────────────────────────────────────────────────
// The one place a match plan is created or changed.
//
// IT PREPARES; IT DOES NOT COMMAND. Nothing in this class transmits anything.
// Selecting a programme, choosing lanes and assigning athletes all write RMS's
// own plan file and nothing else. No station is told, asked or configured.
//
// READINESS HERE IS OBSERVATIONAL. "RMS PLAN READY" means the operator has
// filled the plan in and the stations LOOK healthy from the outside. It is not
// and must never be presented as "the target has loaded the match", because no
// command exists that could load one. The two are named separately for exactly
// that reason.
//
// PLANNED vs OBSERVED are kept apart. Where the plan says one programme and a
// station reports another, that is reported as a mismatch. It is never
// silently resolved in the plan's favour: RMS did not put that programme on
// the station and cannot change it.
// ─────────────────────────────────────────────────────────────────────────────

#include "AthleteRegistry.h"
#include "MatchPlan.h"
#include "RangeConfigurationService.h"
#include "RangeMonitor.h"
#include "RmsJsonStore.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <functional>

namespace ta {
namespace rms {

// Why one participating lane is or is not ready. Ordered by severity: the
// first thing wrong is the thing the operator must fix.
enum class LaneReadiness {
    Ready,
    NoDevice,            // no station assigned to the physical lane
    NodeOffline,         // station assigned, not being heard
    TargetDisconnected,  // station heard, its target is not connected
    NoAthlete            // everything healthy, nobody on the lane
};

QString toString(LaneReadiness r);

// How the station's reported programme compares with the plan's.
enum class ProgrammeMatch {
    Unknown,    // no plan programme, or nothing reported yet
    Offline,    // the station is not being heard
    Matches,
    Mismatch
};

QString toString(ProgrammeMatch m);

class MatchPlanService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasPlan READ hasPlan NOTIFY planChanged)
    Q_PROPERTY(QString planId READ planId NOTIFY planChanged)
    Q_PROPERTY(QString planName READ planName NOTIFY planChanged)
    Q_PROPERTY(QString planStatus READ planStatusLabel NOTIFY planChanged)
    Q_PROPERTY(QString programmeId READ programmeId NOTIFY planChanged)
    Q_PROPERTY(QString programmeLabel READ programmeLabel NOTIFY planChanged)
    Q_PROPERTY(bool programmeSelected READ programmeSelected NOTIFY planChanged)
    Q_PROPERTY(int selectedLaneCount READ selectedLaneCount NOTIFY planChanged)
    Q_PROPERTY(int assignedAthleteCount READ assignedAthleteCount NOTIFY planChanged)
    Q_PROPERTY(int planCount READ planCount NOTIFY plansChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:
    MatchPlanService(RangeConfigurationService* range, RangeMonitor* monitor,
                     AthleteRegistry* athletes, QObject* parent = nullptr);

    void setStorePath(const QString& path);
    void load();

    // Tests inject a clock so timestamps are deterministic.
    void setClockForTesting(std::function<qint64()> clock) { m_clock = std::move(clock); }

    // ── the plan being edited ────────────────────────────────────────────
    bool hasPlan() const { return m_current.isValid(); }
    QString planId() const { return m_current.planId; }
    QString planName() const { return m_current.planName; }
    QString planStatusLabel() const { return toString(m_current.status); }
    QString programmeId() const { return m_current.programme.programmeId; }
    QString programmeLabel() const { return m_current.programme.displayLabel; }
    bool programmeSelected() const { return m_current.programme.isValid(); }
    int selectedLaneCount() const { return m_current.laneCount(); }
    int assignedAthleteCount() const { return m_current.assignedAthleteCount(); }
    int planCount() const { return int(m_plans.size()); }
    QString lastError() const { return m_lastError; }
    QString configPath() const { return m_store.path(); }
    const MatchPlan& current() const { return m_current; }

    // ── editing ──────────────────────────────────────────────────────────
    Q_INVOKABLE QString createPlan(const QString& name);
    Q_INVOKABLE bool openPlan(const QString& planId);
    Q_INVOKABLE bool renamePlan(const QString& name);
    Q_INVOKABLE bool deletePlan(const QString& planId);
    Q_INVOKABLE bool archivePlan(const QString& planId);

    // Programme identity comes from CompetitionCatalogue.qml — the one
    // catalogue on this branch. Passed as IDs; the label is a snapshot for
    // historical display and never drives a decision.
    Q_INVOKABLE bool setProgramme(const QString& programmeId, const QString& rulesetId,
                                  const QString& targetStandardId,
                                  const QString& disciplineId, int distanceM,
                                  int shotCount, const QString& programmeType,
                                  const QString& displayLabel);

    Q_INVOKABLE bool selectLane(int laneNumber, bool selected);
    Q_INVOKABLE int  selectAllOnlineLanes();
    Q_INVOKABLE bool clearLaneSelection();

    Q_INVOKABLE bool assignAthlete(const QString& athleteId, int laneNumber);
    Q_INVOKABLE bool clearAthlete(int laneNumber);

    Q_INVOKABLE bool markReady();
    Q_INVOKABLE bool markDraft();

    // ── queries ──────────────────────────────────────────────────────────
    Q_INVOKABLE bool isLaneSelected(int laneNumber) const;
    Q_INVOKABLE QString athleteOnLane(int laneNumber) const;
    Q_INVOKABLE int laneNumberForAthlete(const QString& athleteId) const;
    Q_INVOKABLE QVariantMap readiness() const;
    Q_INVOKABLE QVariantList allPlans() const;
    Q_INVOKABLE QVariantMap planSummary(const QString& planId) const;

    // Readiness for one PARTICIPATING lane. Lanes outside the plan are Ready
    // by definition — they are not taking part.
    LaneReadiness laneReadiness(int laneNumber) const;
    ProgrammeMatch programmeMatchForLane(int laneNumber) const;
    // Which plan, if any, still has this athlete on a lane.
    QString planNameUsingAthlete(const QString& athleteId) const;

signals:
    void planChanged();
    void plansChanged();
    void lastErrorChanged();
    void rejected(const QString& reason);

private:
    bool commit();
    void setError(const QString& reason);
    void sortLanes();
    void storeCurrentIntoList();
    qint64 nowMs() const;

    RangeConfigurationService* m_range = nullptr;
    RangeMonitor* m_monitor = nullptr;
    AthleteRegistry* m_athletes = nullptr;

    RmsJsonStore m_store;
    QVector<MatchPlan> m_plans;
    MatchPlan m_current;
    QString m_lastError;
    std::function<qint64()> m_clock;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_MATCHPLANSERVICE_H
