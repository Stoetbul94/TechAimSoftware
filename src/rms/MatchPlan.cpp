#include "MatchPlan.h"
#include "Athlete.h"

#include <QJsonArray>
#include <QUuid>

namespace ta {
namespace rms {

namespace {
QString mintId(const char* prefix)
{
    return QString::fromLatin1(prefix)
           + QUuid::createUuid().toString(QUuid::Id128).left(12).toLower();
}
} // namespace

// ── Athlete ──────────────────────────────────────────────────────────────

QJsonObject Athlete::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("athleteId")]   = athleteId;
    o[QStringLiteral("displayName")] = displayName;
    o[QStringLiteral("club")]        = club;
    o[QStringLiteral("country")]     = country;
    o[QStringLiteral("notes")]       = notes;
    o[QStringLiteral("temporary")]   = temporary;
    return o;
}

Athlete Athlete::fromJson(const QJsonObject& o)
{
    Athlete a;
    a.athleteId   = o.value(QStringLiteral("athleteId")).toString();
    a.displayName = o.value(QStringLiteral("displayName")).toString();
    a.club        = o.value(QStringLiteral("club")).toString();
    a.country     = o.value(QStringLiteral("country")).toString();
    a.notes       = o.value(QStringLiteral("notes")).toString();
    a.temporary   = o.value(QStringLiteral("temporary")).toBool(false);
    if (a.athleteId.isEmpty())
        a.athleteId = mintId("ath-");
    return a;
}

Athlete Athlete::create(const QString& displayName, bool temporary)
{
    Athlete a;
    a.athleteId = mintId("ath-");
    a.displayName = displayName.trimmed();
    a.temporary = temporary;
    return a;
}

// ── PlanStatus ───────────────────────────────────────────────────────────

QString toString(PlanStatus s)
{
    switch (s) {
    case PlanStatus::Ready:    return QStringLiteral("READY");
    case PlanStatus::Archived: return QStringLiteral("ARCHIVED");
    case PlanStatus::Draft:    break;
    }
    return QStringLiteral("DRAFT");
}

PlanStatus planStatusFromString(const QString& s)
{
    if (s == QLatin1String("READY"))    return PlanStatus::Ready;
    if (s == QLatin1String("ARCHIVED")) return PlanStatus::Archived;
    return PlanStatus::Draft;
}

// ── ProgrammeSnapshot ────────────────────────────────────────────────────

QJsonObject ProgrammeSnapshot::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("programmeId")]      = programmeId;
    o[QStringLiteral("rulesetId")]        = rulesetId;
    o[QStringLiteral("targetStandardId")] = targetStandardId;
    o[QStringLiteral("disciplineId")]     = disciplineId;
    o[QStringLiteral("distanceM")]        = distanceM;
    o[QStringLiteral("shotCount")]        = shotCount;
    o[QStringLiteral("programmeType")]    = programmeType;
    o[QStringLiteral("displayLabel")]     = displayLabel;
    return o;
}

ProgrammeSnapshot ProgrammeSnapshot::fromJson(const QJsonObject& o)
{
    ProgrammeSnapshot p;
    p.programmeId      = o.value(QStringLiteral("programmeId")).toString();
    p.rulesetId        = o.value(QStringLiteral("rulesetId")).toString();
    p.targetStandardId = o.value(QStringLiteral("targetStandardId")).toString();
    p.disciplineId     = o.value(QStringLiteral("disciplineId")).toString();
    p.distanceM        = o.value(QStringLiteral("distanceM")).toInt(0);
    p.shotCount        = o.value(QStringLiteral("shotCount")).toInt(0);
    p.programmeType    = o.value(QStringLiteral("programmeType")).toString();
    p.displayLabel     = o.value(QStringLiteral("displayLabel")).toString();
    return p;
}

// ── PlanLane ─────────────────────────────────────────────────────────────

QJsonObject PlanLane::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("laneId")]     = laneId;
    o[QStringLiteral("laneNumber")] = laneNumber;
    o[QStringLiteral("athleteId")]  = athleteId;
    return o;
}

PlanLane PlanLane::fromJson(const QJsonObject& o)
{
    PlanLane l;
    l.laneId     = o.value(QStringLiteral("laneId")).toString();
    l.laneNumber = o.value(QStringLiteral("laneNumber")).toInt(0);
    l.athleteId  = o.value(QStringLiteral("athleteId")).toString();
    return l;
}

// ── MatchPlan ────────────────────────────────────────────────────────────

int MatchPlan::assignedAthleteCount() const
{
    int n = 0;
    for (const PlanLane& l : lanes)
        if (l.hasAthlete())
            ++n;
    return n;
}

int MatchPlan::indexOfLaneNumber(int laneNumber) const
{
    for (int i = 0; i < lanes.size(); ++i)
        if (lanes.at(i).laneNumber == laneNumber)
            return i;
    return -1;
}

bool MatchPlan::hasLaneNumber(int laneNumber) const
{
    return indexOfLaneNumber(laneNumber) >= 0;
}

int MatchPlan::laneNumberForAthlete(const QString& athleteId) const
{
    if (athleteId.isEmpty())
        return -1;
    for (const PlanLane& l : lanes)
        if (l.athleteId == athleteId)
            return l.laneNumber;
    return -1;
}

QJsonObject MatchPlan::toJson() const
{
    QJsonArray arr;
    for (const PlanLane& l : lanes)
        arr.append(l.toJson());

    QJsonObject o;
    o[QStringLiteral("planId")]         = planId;
    o[QStringLiteral("planName")]       = planName;
    o[QStringLiteral("rangeId")]        = rangeId;
    o[QStringLiteral("createdAtUtcMs")] = double(createdAtUtcMs);
    o[QStringLiteral("updatedAtUtcMs")] = double(updatedAtUtcMs);
    o[QStringLiteral("programme")]      = programme.toJson();
    o[QStringLiteral("lanes")]          = arr;
    o[QStringLiteral("status")]         = toString(status);
    return o;
}

MatchPlan MatchPlan::fromJson(const QJsonObject& o)
{
    MatchPlan p;
    p.planId         = o.value(QStringLiteral("planId")).toString();
    p.planName       = o.value(QStringLiteral("planName")).toString();
    p.rangeId        = o.value(QStringLiteral("rangeId")).toString();
    p.createdAtUtcMs = qint64(o.value(QStringLiteral("createdAtUtcMs")).toDouble());
    p.updatedAtUtcMs = qint64(o.value(QStringLiteral("updatedAtUtcMs")).toDouble());
    p.programme      = ProgrammeSnapshot::fromJson(
                           o.value(QStringLiteral("programme")).toObject());
    const QJsonArray arr = o.value(QStringLiteral("lanes")).toArray();
    for (const QJsonValue& v : arr)
        p.lanes.append(PlanLane::fromJson(v.toObject()));
    p.status = planStatusFromString(o.value(QStringLiteral("status")).toString());
    // ONLINE/OFFLINE IS NEVER PERSISTED. Readiness is recomputed from live
    // telemetry after every restart; a stored "ready" would be a claim about
    // the world that stopped being true the moment RMS was closed.
    return p;
}

MatchPlan MatchPlan::create(const QString& name, const QString& rangeId, qint64 nowUtcMs)
{
    MatchPlan p;
    p.planId = mintId("plan-");
    p.planName = name.trimmed();
    p.rangeId = rangeId;
    p.createdAtUtcMs = nowUtcMs;
    p.updatedAtUtcMs = nowUtcMs;
    p.status = PlanStatus::Draft;
    return p;
}

} // namespace rms
} // namespace ta
