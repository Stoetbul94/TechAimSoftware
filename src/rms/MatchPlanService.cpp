#include "MatchPlanService.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>

namespace ta {
namespace rms {

QString toString(LaneReadiness r)
{
    switch (r) {
    case LaneReadiness::NoDevice:           return QStringLiteral("NO DEVICE");
    case LaneReadiness::NodeOffline:        return QStringLiteral("NODE OFFLINE");
    case LaneReadiness::TargetDisconnected: return QStringLiteral("TARGET DISCONNECTED");
    case LaneReadiness::NoAthlete:          return QStringLiteral("NO ATHLETE");
    case LaneReadiness::Ready:              break;
    }
    return QStringLiteral("READY");
}

QString toString(ProgrammeMatch m)
{
    switch (m) {
    case ProgrammeMatch::Matches:  return QStringLiteral("MATCHES PLAN");
    case ProgrammeMatch::Mismatch: return QStringLiteral("DOES NOT MATCH PLAN");
    case ProgrammeMatch::Offline:  return QStringLiteral("OFFLINE");
    case ProgrammeMatch::Unknown:  break;
    }
    return QStringLiteral("UNKNOWN");
}

MatchPlanService::MatchPlanService(RangeConfigurationService* range,
                                   RangeMonitor* monitor, AthleteRegistry* athletes,
                                   QObject* parent)
    : QObject(parent)
    , m_range(range)
    , m_monitor(monitor)
    , m_athletes(athletes)
    , m_clock([] { return QDateTime::currentMSecsSinceEpoch(); })
{
    m_store.setPath(rmsDataFile(QStringLiteral("plans.json")));
    // The registry asks us before deleting somebody, so a lane can never end
    // up pointing at an athlete who no longer exists.
    if (m_athletes)
        m_athletes->setInUseCheck([this](const QString& id) {
            return planNameUsingAthlete(id);
        });
}

qint64 MatchPlanService::nowMs() const
{
    return m_clock ? m_clock() : 0;
}

void MatchPlanService::setStorePath(const QString& path)
{
    m_store.setPath(path);
}

void MatchPlanService::load()
{
    QJsonObject doc;
    const StoreResult r = m_store.load(kMatchPlanSchemaVersion, &doc);
    m_plans.clear();
    m_current = MatchPlan();

    if (r.ok) {
        const QJsonArray arr = doc.value(QStringLiteral("plans")).toArray();
        for (const QJsonValue& v : arr) {
            const MatchPlan p = MatchPlan::fromJson(v.toObject());
            if (p.isValid())
                m_plans.append(p);
        }
        const QString currentId = doc.value(QStringLiteral("currentPlanId")).toString();
        for (const MatchPlan& p : m_plans)
            if (p.planId == currentId)
                m_current = p;
        setError(QString());
    } else if (r.error == StoreError::NotFound) {
        setError(QString());
    } else {
        setError(r.detail);
    }
    emit plansChanged();
    emit planChanged();
}

void MatchPlanService::setError(const QString& reason)
{
    if (m_lastError == reason)
        return;
    m_lastError = reason;
    emit lastErrorChanged();
}

void MatchPlanService::sortLanes()
{
    std::sort(m_current.lanes.begin(), m_current.lanes.end(),
              [](const PlanLane& a, const PlanLane& b) {
                  return a.laneNumber < b.laneNumber;
              });
}

void MatchPlanService::storeCurrentIntoList()
{
    for (int i = 0; i < m_plans.size(); ++i) {
        if (m_plans.at(i).planId == m_current.planId) {
            m_plans[i] = m_current;
            return;
        }
    }
    if (m_current.isValid())
        m_plans.append(m_current);
}

bool MatchPlanService::commit()
{
    m_current.updatedAtUtcMs = nowMs();
    storeCurrentIntoList();

    QJsonArray arr;
    for (const MatchPlan& p : m_plans)
        arr.append(p.toJson());
    QJsonObject doc;
    doc[QStringLiteral("plans")] = arr;
    doc[QStringLiteral("currentPlanId")] = m_current.planId;

    const StoreResult r = m_store.save(kMatchPlanSchemaVersion, doc);
    if (!r.ok) {
        setError(r.detail);
        emit planChanged();
        emit plansChanged();
        return false;
    }
    setError(QString());
    emit planChanged();
    emit plansChanged();
    return true;
}

// ── editing ──────────────────────────────────────────────────────────────

QString MatchPlanService::createPlan(const QString& name)
{
    if (name.trimmed().isEmpty()) {
        setError(QStringLiteral("A match needs a name."));
        emit rejected(m_lastError);
        return QString();
    }
    if (!m_range || !m_range->isConfigured()) {
        setError(QStringLiteral("Configure a range before preparing a match."));
        emit rejected(m_lastError);
        return QString();
    }
    m_current = MatchPlan::create(name, m_range->range().rangeId, nowMs());
    if (!commit())
        return QString();
    return m_current.planId;
}

bool MatchPlanService::openPlan(const QString& planId)
{
    for (const MatchPlan& p : m_plans) {
        if (p.planId == planId) {
            m_current = p;
            return commit();
        }
    }
    return false;
}

bool MatchPlanService::renamePlan(const QString& name)
{
    if (!hasPlan() || name.trimmed().isEmpty())
        return false;
    m_current.planName = name.trimmed();
    return commit();
}

bool MatchPlanService::deletePlan(const QString& planId)
{
    for (int i = 0; i < m_plans.size(); ++i) {
        if (m_plans.at(i).planId != planId)
            continue;
        // Only a draft may be deleted outright. A reviewed plan is archived
        // instead, so a decision the operator already made is not thrown away
        // by one click.
        if (m_plans.at(i).status == PlanStatus::Ready) {
            setError(QStringLiteral("\"%1\" is marked ready. Archive it instead.")
                         .arg(m_plans.at(i).planName));
            emit rejected(m_lastError);
            return false;
        }
        m_plans.remove(i);
        if (m_current.planId == planId)
            m_current = MatchPlan();
        return commit();
    }
    return false;
}

bool MatchPlanService::archivePlan(const QString& planId)
{
    for (int i = 0; i < m_plans.size(); ++i) {
        if (m_plans.at(i).planId != planId)
            continue;
        m_plans[i].status = PlanStatus::Archived;
        if (m_current.planId == planId)
            m_current.status = PlanStatus::Archived;
        return commit();
    }
    return false;
}

bool MatchPlanService::setProgramme(const QString& programmeId, const QString& rulesetId,
                                    const QString& targetStandardId,
                                    const QString& disciplineId, int distanceM,
                                    int shotCount, const QString& programmeType,
                                    const QString& displayLabel)
{
    if (!hasPlan()) {
        setError(QStringLiteral("No match is being prepared."));
        emit rejected(m_lastError);
        return false;
    }
    if (programmeId.isEmpty()) {
        setError(QStringLiteral("Choose a programme."));
        emit rejected(m_lastError);
        return false;
    }
    ProgrammeSnapshot p;
    p.programmeId      = programmeId;
    p.rulesetId        = rulesetId;
    p.targetStandardId = targetStandardId;
    p.disciplineId     = disciplineId;
    p.distanceM        = distanceM;
    p.shotCount        = shotCount;
    p.programmeType    = programmeType;
    p.displayLabel     = displayLabel;
    m_current.programme = p;
    // Changing the programme un-readies the plan: the operator reviewed a
    // different match.
    if (m_current.status == PlanStatus::Ready)
        m_current.status = PlanStatus::Draft;
    return commit();
}

bool MatchPlanService::selectLane(int laneNumber, bool selected)
{
    if (!hasPlan()) {
        setError(QStringLiteral("No match is being prepared."));
        emit rejected(m_lastError);
        return false;
    }
    const LaneDefinition* lane = m_range->range().laneByNumber(laneNumber);
    if (!lane) {
        setError(QStringLiteral("Lane %1 is not part of this range.").arg(laneNumber));
        emit rejected(m_lastError);
        return false;
    }

    const int at = m_current.indexOfLaneNumber(laneNumber);
    if (selected) {
        if (at >= 0)
            return true;
        PlanLane pl;
        pl.laneId = lane->laneId;
        pl.laneNumber = laneNumber;
        m_current.lanes.append(pl);
        sortLanes();
    } else {
        if (at < 0)
            return true;
        m_current.lanes.remove(at);
    }
    if (m_current.status == PlanStatus::Ready)
        m_current.status = PlanStatus::Draft;
    return commit();
}

int MatchPlanService::selectAllOnlineLanes()
{
    if (!hasPlan() || !m_range->isConfigured())
        return 0;
    int added = 0;
    for (const LaneDefinition& lane : m_range->range().lanes) {
        if (!lane.isAssigned() || !lane.enabled)
            continue;
        const TargetNodeRecord* r = m_monitor->nodeById(lane.assignedNodeId);
        if (!r || r->isOffline())
            continue;
        if (m_current.hasLaneNumber(lane.laneNumber))
            continue;
        PlanLane pl;
        pl.laneId = lane.laneId;
        pl.laneNumber = lane.laneNumber;
        m_current.lanes.append(pl);
        ++added;
    }
    if (added > 0) {
        sortLanes();
        if (m_current.status == PlanStatus::Ready)
            m_current.status = PlanStatus::Draft;
        commit();
    }
    return added;
}

bool MatchPlanService::clearLaneSelection()
{
    if (!hasPlan())
        return false;
    m_current.lanes.clear();
    if (m_current.status == PlanStatus::Ready)
        m_current.status = PlanStatus::Draft;
    return commit();
}

bool MatchPlanService::assignAthlete(const QString& athleteId, int laneNumber)
{
    if (!hasPlan()) {
        setError(QStringLiteral("No match is being prepared."));
        emit rejected(m_lastError);
        return false;
    }
    const int at = m_current.indexOfLaneNumber(laneNumber);
    if (at < 0) {
        setError(QStringLiteral("Lane %1 is not taking part in this match.").arg(laneNumber));
        emit rejected(m_lastError);
        return false;
    }
    if (athleteId.isEmpty())
        return clearAthlete(laneNumber);
    if (m_athletes && !m_athletes->byId(athleteId)) {
        setError(QStringLiteral("That athlete is not on the start list."));
        emit rejected(m_lastError);
        return false;
    }

    // ONE ATHLETE, ONE LANE. Somebody cannot shoot lane 1 and lane 4 in the
    // same match, and a plan that says they can would have a range officer
    // looking for a person who is not there.
    const int existing = m_current.laneNumberForAthlete(athleteId);
    if (existing >= 0 && existing != laneNumber) {
        const QString name = m_athletes ? m_athletes->displayNameFor(athleteId)
                                        : athleteId;
        setError(QStringLiteral("%1 is already on lane %2 in this match. "
                                "Clear that lane first, or move them.")
                     .arg(name).arg(existing));
        emit rejected(m_lastError);
        return false;
    }

    m_current.lanes[at].athleteId = athleteId;
    if (m_current.status == PlanStatus::Ready)
        m_current.status = PlanStatus::Draft;
    return commit();
}

bool MatchPlanService::clearAthlete(int laneNumber)
{
    const int at = m_current.indexOfLaneNumber(laneNumber);
    if (at < 0)
        return false;
    if (m_current.lanes.at(at).athleteId.isEmpty())
        return true;
    m_current.lanes[at].athleteId.clear();
    if (m_current.status == PlanStatus::Ready)
        m_current.status = PlanStatus::Draft;
    return commit();
}

bool MatchPlanService::markReady()
{
    if (!hasPlan())
        return false;
    if (!m_current.programme.isValid()) {
        setError(QStringLiteral("Choose a programme before marking the match ready."));
        emit rejected(m_lastError);
        return false;
    }
    if (m_current.lanes.isEmpty()) {
        setError(QStringLiteral("Select at least one lane."));
        emit rejected(m_lastError);
        return false;
    }
    if (m_current.assignedAthleteCount() != m_current.laneCount()) {
        setError(QStringLiteral("Every participating lane needs an athlete (%1 of %2 assigned).")
                     .arg(m_current.assignedAthleteCount()).arg(m_current.laneCount()));
        emit rejected(m_lastError);
        return false;
    }
    // READY MEANS THE PLAN IS COMPLETE — not that any station has loaded it.
    // Node health is deliberately NOT a condition here: a tablet may be
    // switched on minutes before the start, and refusing to save a finished
    // plan because of that would be unhelpful. The review screen reports range
    // health separately.
    m_current.status = PlanStatus::Ready;
    return commit();
}

bool MatchPlanService::markDraft()
{
    if (!hasPlan())
        return false;
    m_current.status = PlanStatus::Draft;
    return commit();
}

// ── queries ──────────────────────────────────────────────────────────────

bool MatchPlanService::isLaneSelected(int laneNumber) const
{
    return m_current.hasLaneNumber(laneNumber);
}

QString MatchPlanService::athleteOnLane(int laneNumber) const
{
    const int at = m_current.indexOfLaneNumber(laneNumber);
    return at < 0 ? QString() : m_current.lanes.at(at).athleteId;
}

int MatchPlanService::laneNumberForAthlete(const QString& athleteId) const
{
    return m_current.laneNumberForAthlete(athleteId);
}

LaneReadiness MatchPlanService::laneReadiness(int laneNumber) const
{
    const int at = m_current.indexOfLaneNumber(laneNumber);
    if (at < 0)
        return LaneReadiness::Ready;   // not taking part

    const LaneDefinition* lane = m_range ? m_range->range().laneByNumber(laneNumber)
                                         : nullptr;
    if (!lane || !lane->isAssigned())
        return LaneReadiness::NoDevice;

    const TargetNodeRecord* r = m_monitor->nodeById(lane->assignedNodeId);
    if (!r || r->isOffline())
        return LaneReadiness::NodeOffline;
    if (r->connection == ConnectionState::TargetDisconnected)
        return LaneReadiness::TargetDisconnected;
    if (!m_current.lanes.at(at).hasAthlete())
        return LaneReadiness::NoAthlete;
    return LaneReadiness::Ready;
}

ProgrammeMatch MatchPlanService::programmeMatchForLane(int laneNumber) const
{
    if (!m_current.programme.isValid() || !m_current.hasLaneNumber(laneNumber))
        return ProgrammeMatch::Unknown;
    const LaneDefinition* lane = m_range ? m_range->range().laneByNumber(laneNumber)
                                         : nullptr;
    if (!lane || !lane->isAssigned())
        return ProgrammeMatch::Unknown;
    const TargetNodeRecord* r = m_monitor->nodeById(lane->assignedNodeId);
    if (!r || r->isOffline())
        return ProgrammeMatch::Offline;
    if (r->programmeId.isEmpty())
        return ProgrammeMatch::Unknown;   // nothing selected on the station yet
    // COMPARED BY STABLE ID, never by label.
    return r->programmeId == m_current.programme.programmeId
               ? ProgrammeMatch::Matches : ProgrammeMatch::Mismatch;
}

QVariantMap MatchPlanService::readiness() const
{
    QVariantMap m;
    const int lanes = m_current.laneCount();
    int online = 0, offline = 0, targetsConnected = 0, targetsUnavailable = 0;
    int ready = 0, mismatches = 0;
    QVariantList issues;

    for (const PlanLane& pl : m_current.lanes) {
        const LaneReadiness lr = laneReadiness(pl.laneNumber);
        if (lr == LaneReadiness::Ready)
            ++ready;
        else {
            QVariantMap issue;
            issue[QStringLiteral("laneNumber")] = pl.laneNumber;
            issue[QStringLiteral("reason")] = toString(lr);
            issues.append(issue);
        }

        const LaneDefinition* lane = m_range ? m_range->range().laneByNumber(pl.laneNumber)
                                             : nullptr;
        const TargetNodeRecord* r = (lane && lane->isAssigned())
                                        ? m_monitor->nodeById(lane->assignedNodeId)
                                        : nullptr;
        if (r && !r->isOffline()) {
            ++online;
            if (r->connection == ConnectionState::TargetConnected)
                ++targetsConnected;
            else
                ++targetsUnavailable;
        } else {
            ++offline;
            ++targetsUnavailable;
        }
        if (programmeMatchForLane(pl.laneNumber) == ProgrammeMatch::Mismatch)
            ++mismatches;
    }

    m[QStringLiteral("selectedLanes")]      = lanes;
    m[QStringLiteral("athletesAssigned")]   = m_current.assignedAthleteCount();
    m[QStringLiteral("nodesOnline")]        = online;
    m[QStringLiteral("nodesOffline")]       = offline;
    m[QStringLiteral("targetsConnected")]   = targetsConnected;
    m[QStringLiteral("targetsUnavailable")] = targetsUnavailable;
    m[QStringLiteral("lanesReady")]         = ready;
    m[QStringLiteral("programmeMismatches")]= mismatches;
    m[QStringLiteral("issues")]             = issues;

    // THE PLAN is complete when the operator has filled it in.
    const bool planComplete = m_current.programme.isValid() && lanes > 0
                              && m_current.assignedAthleteCount() == lanes;
    // THE RANGE is ready when every participating lane is answering with a
    // connected target. These are separate answers to separate questions, and
    // neither of them says a target has LOADED anything.
    const bool rangeReady = lanes > 0 && ready == lanes;

    m[QStringLiteral("planComplete")] = planComplete;
    m[QStringLiteral("rangeReady")]   = rangeReady;
    m[QStringLiteral("planStatus")]   = toString(m_current.status);
    // Named to make the distinction impossible to miss on screen.
    m[QStringLiteral("rmsPlanReady")] = planComplete && m_current.status == PlanStatus::Ready;
    m[QStringLiteral("targetMatchLoaded")] = false;
    m[QStringLiteral("targetMatchLoadedNote")] =
        QStringLiteral("RMS cannot load a match onto a target station — no command "
                       "channel exists. This is the plan's readiness, not the "
                       "station's.");
    return m;
}

QString MatchPlanService::planNameUsingAthlete(const QString& athleteId) const
{
    if (athleteId.isEmpty())
        return QString();
    for (const MatchPlan& p : m_plans) {
        if (p.status == PlanStatus::Archived)
            continue;
        if (p.laneNumberForAthlete(athleteId) >= 0)
            return p.planName;
    }
    // The plan being edited may not be in the saved list yet.
    if (m_current.isValid() && m_current.status != PlanStatus::Archived
        && m_current.laneNumberForAthlete(athleteId) >= 0)
        return m_current.planName;
    return QString();
}

QVariantMap MatchPlanService::planSummary(const QString& planId) const
{
    QVariantMap m;
    for (const MatchPlan& p : m_plans) {
        if (p.planId != planId)
            continue;
        m[QStringLiteral("planId")]         = p.planId;
        m[QStringLiteral("planName")]       = p.planName;
        m[QStringLiteral("status")]         = toString(p.status);
        m[QStringLiteral("programmeId")]    = p.programme.programmeId;
        m[QStringLiteral("programmeLabel")] = p.programme.displayLabel;
        m[QStringLiteral("laneCount")]      = p.laneCount();
        m[QStringLiteral("athleteCount")]   = p.assignedAthleteCount();
        m[QStringLiteral("current")]        = (p.planId == m_current.planId);
        break;
    }
    return m;
}

QVariantList MatchPlanService::allPlans() const
{
    QVariantList out;
    for (const MatchPlan& p : m_plans)
        out.append(planSummary(p.planId));
    return out;
}

} // namespace rms
} // namespace ta
