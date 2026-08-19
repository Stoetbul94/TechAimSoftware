#include "RangeDefinition.h"

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

QString toString(RangeMode m)
{
    return m == RangeMode::Temporary ? QStringLiteral("TEMPORARY_RANGE")
                                     : QStringLiteral("FIXED_RANGE");
}

RangeMode rangeModeFromString(const QString& s, bool* ok)
{
    if (ok) *ok = true;
    if (s == QLatin1String("TEMPORARY_RANGE")) return RangeMode::Temporary;
    if (s == QLatin1String("FIXED_RANGE"))     return RangeMode::Fixed;
    if (ok) *ok = false;
    return RangeMode::Fixed;
}

// ── LaneDefinition ───────────────────────────────────────────────────────

QString LaneDefinition::label() const
{
    return displayName.isEmpty() ? QStringLiteral("Lane %1").arg(laneNumber)
                                 : displayName;
}

QJsonObject LaneDefinition::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("laneId")]         = laneId;
    o[QStringLiteral("laneNumber")]     = laneNumber;
    o[QStringLiteral("displayName")]    = displayName;
    o[QStringLiteral("assignedNodeId")] = assignedNodeId;
    o[QStringLiteral("enabled")]        = enabled;
    o[QStringLiteral("notes")]          = notes;
    return o;
}

LaneDefinition LaneDefinition::fromJson(const QJsonObject& o)
{
    LaneDefinition l;
    l.laneId         = o.value(QStringLiteral("laneId")).toString();
    l.laneNumber     = o.value(QStringLiteral("laneNumber")).toInt(0);
    l.displayName    = o.value(QStringLiteral("displayName")).toString();
    l.assignedNodeId = o.value(QStringLiteral("assignedNodeId")).toString();
    l.enabled        = o.value(QStringLiteral("enabled")).toBool(true);
    l.notes          = o.value(QStringLiteral("notes")).toString();
    // A lane read back without an id predates the id, or was hand-edited.
    // Minting one is safe: it identifies the same physical firing point from
    // now on, and losing the lane entirely would be worse.
    if (l.laneId.isEmpty())
        l.laneId = mintId("lane-");
    return l;
}

// ── RangeDefinition ──────────────────────────────────────────────────────

int RangeDefinition::firstLaneNumber() const
{
    if (lanes.isEmpty())
        return 0;
    int n = lanes.first().laneNumber;
    for (const LaneDefinition& l : lanes)
        n = qMin(n, l.laneNumber);
    return n;
}

int RangeDefinition::lastLaneNumber() const
{
    int n = 0;
    for (const LaneDefinition& l : lanes)
        n = qMax(n, l.laneNumber);
    return n;
}

int RangeDefinition::indexOfLaneNumber(int number) const
{
    for (int i = 0; i < lanes.size(); ++i)
        if (lanes.at(i).laneNumber == number)
            return i;
    return -1;
}

const LaneDefinition* RangeDefinition::laneByNumber(int number) const
{
    const int i = indexOfLaneNumber(number);
    return i < 0 ? nullptr : &lanes.at(i);
}

const LaneDefinition* RangeDefinition::laneForNode(const QString& nodeId) const
{
    if (nodeId.isEmpty())
        return nullptr;
    for (const LaneDefinition& l : lanes)
        if (l.assignedNodeId == nodeId)
            return &l;
    return nullptr;
}

QJsonObject RangeDefinition::toJson() const
{
    QJsonArray arr;
    for (const LaneDefinition& l : lanes)
        arr.append(l.toJson());

    QJsonObject o;
    o[QStringLiteral("schemaVersion")] = kRangeSchemaVersion;
    o[QStringLiteral("rangeId")]       = rangeId;
    o[QStringLiteral("rangeName")]     = rangeName;
    o[QStringLiteral("rangeType")]     = rangeType;
    o[QStringLiteral("mode")]          = toString(mode);
    o[QStringLiteral("lanes")]         = arr;
    return o;
}

RangeDefinition RangeDefinition::fromJson(const QJsonObject& o)
{
    RangeDefinition r;
    r.rangeId   = o.value(QStringLiteral("rangeId")).toString();
    r.rangeName = o.value(QStringLiteral("rangeName")).toString();
    r.rangeType = o.value(QStringLiteral("rangeType")).toString();
    r.mode      = rangeModeFromString(o.value(QStringLiteral("mode")).toString());
    const QJsonArray arr = o.value(QStringLiteral("lanes")).toArray();
    for (const QJsonValue& v : arr)
        r.lanes.append(LaneDefinition::fromJson(v.toObject()));
    return r;
}

RangeDefinition RangeDefinition::createFixed(const QString& name,
                                             const QString& type,
                                             int firstLane, int lastLane)
{
    RangeDefinition r;
    r.rangeId   = mintId("range-");
    r.rangeName = name;
    r.rangeType = type;
    r.mode      = RangeMode::Fixed;
    if (lastLane < firstLane)
        qSwap(firstLane, lastLane);
    for (int n = firstLane; n <= lastLane; ++n) {
        LaneDefinition l;
        l.laneId = mintId("lane-");
        l.laneNumber = n;
        r.lanes.append(l);
    }
    return r;
}

RangeDefinition RangeDefinition::createTemporary(const QString& name,
                                                 const QString& type,
                                                 const QVector<QString>& nodeIds,
                                                 int firstLane)
{
    RangeDefinition r;
    r.rangeId   = mintId("range-");
    r.rangeName = name;
    r.rangeType = type;
    r.mode      = RangeMode::Temporary;
    int n = firstLane;
    for (const QString& nodeId : nodeIds) {
        LaneDefinition l;
        l.laneId = mintId("lane-");
        l.laneNumber = n++;
        // A temporary range takes the discovered order as the lane order.
        // That is a STARTING POINT, not a claim about where the stations
        // physically are — which is why the mapping stays editable.
        l.assignedNodeId = nodeId;
        r.lanes.append(l);
    }
    return r;
}

} // namespace rms
} // namespace ta
