#ifndef TA_RMS_MATCHPLAN_H
#define TA_RMS_MATCHPLAN_H

// ─────────────────────────────────────────────────────────────────────────────
// WHAT THE OPERATOR INTENDS. Kept strictly apart from what the range IS
// (RangeDefinition) and from what the stations REPORT (RangeMonitor).
//
//   RangeDefinition   the physical range: lanes, and which station stands on
//                     each. Configuration. No athlete, no programme, no score.
//   MatchPlan         a competition being prepared: a programme, the lanes
//                     taking part, and who is on them. Also configuration.
//   node telemetry    what is actually happening. Observation.
//
// A PLAN CHANGES NOTHING ON A TARGET. Saving one records an intention; it does
// not load, start or configure a station, because RMS has no command channel.
// Where the plan and the telemetry disagree, that disagreement is INFORMATION
// and is shown as such — never reconciled by pretending the plan won.
//
// planId IS NOT A NODE sessionId. A node's sessionId identifies one match on
// one station; a plan spans several stations and will, once commands exist,
// outlive and enclose several node sessions. Conflating them now would make
// that impossible later.
// ─────────────────────────────────────────────────────────────────────────────

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace ta {
namespace rms {

// DRAFT  being prepared; may be incomplete.
// READY  the operator has reviewed it and considers it complete.
// There is deliberately NO RUNNING state: running would mean RMS had told the
// stations something, and it cannot.
enum class PlanStatus { Draft, Ready, Archived };

QString toString(PlanStatus s);
PlanStatus planStatusFromString(const QString& s);

// Programme identity, snapshotted into the plan so it stays stable if the
// catalogue is ever revised. IDs drive every decision; `displayLabel` is a
// snapshot for historical display only and must never be compared or switched
// on (QML-LANG-001).
//
// NO SCORING FORMULA AND NO TARGET GEOMETRY IS COPIED HERE. Those live in the
// target application and are the node's authority alone.
struct ProgrammeSnapshot {
    QString programmeId;
    QString rulesetId;
    QString targetStandardId;
    QString disciplineId;
    int     distanceM = 0;
    int     shotCount = 0;        // -1 = unlimited
    QString programmeType;        // "OFFICIAL" | "PRESET"
    QString displayLabel;

    bool isValid() const { return !programmeId.isEmpty(); }
    bool isOfficial() const { return programmeType == QLatin1String("OFFICIAL"); }

    QJsonObject toJson() const;
    static ProgrammeSnapshot fromJson(const QJsonObject& o);
};

// One participating physical lane, and who the operator intends to put on it.
struct PlanLane {
    QString laneId;         // the RangeDefinition lane this refers to
    int     laneNumber = 0;
    QString athleteId;      // empty = selected but nobody assigned yet

    bool hasAthlete() const { return !athleteId.isEmpty(); }

    QJsonObject toJson() const;
    static PlanLane fromJson(const QJsonObject& o);
};

struct MatchPlan {
    QString planId;
    QString planName;
    QString rangeId;          // the range this was prepared against
    qint64  createdAtUtcMs = 0;
    qint64  updatedAtUtcMs = 0;
    ProgrammeSnapshot programme;
    QVector<PlanLane> lanes;  // participating lanes, in lane-number order
    PlanStatus status = PlanStatus::Draft;

    bool isValid() const { return !planId.isEmpty(); }
    int  laneCount() const { return int(lanes.size()); }
    int  assignedAthleteCount() const;
    bool hasLaneNumber(int laneNumber) const;
    int  indexOfLaneNumber(int laneNumber) const;
    // The lane an athlete is on in THIS plan, or -1. An athlete may be on at
    // most one participating lane.
    int  laneNumberForAthlete(const QString& athleteId) const;

    QJsonObject toJson() const;
    static MatchPlan fromJson(const QJsonObject& o);
    static MatchPlan create(const QString& name, const QString& rangeId, qint64 nowUtcMs);
};

// The persisted document versions. Bump ONLY with a migration.
constexpr int kAthleteSchemaVersion = 1;
constexpr int kMatchPlanSchemaVersion = 1;

} // namespace rms
} // namespace ta

#endif // TA_RMS_MATCHPLAN_H
