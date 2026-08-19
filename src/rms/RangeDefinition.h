#ifndef TA_RMS_RANGEDEFINITION_H
#define TA_RMS_RANGEDEFINITION_H

// ─────────────────────────────────────────────────────────────────────────────
// THE PHYSICAL RANGE — what exists, as distinct from what is switched on.
//
// This is the distinction milestone 3 is built around. Up to now RMS knew only
// "nodes I can currently hear". A ten-lane range is a ten-lane range at 06:00
// with nothing powered up, and it is still a ten-lane range when four tablets
// are flat. Lanes 7 to 10 do not stop existing because nobody has switched
// them on; they are OFFLINE, which is a different and much more useful thing
// for a range officer to be told.
//
// So the range definition is CONFIGURATION, persisted by RMS, and the node
// telemetry is OBSERVATION, which comes and goes. The two are joined by one
// stable key and nothing else:
//
//     laneId  ↔  nodeId
//
// NOT lane ↔ IP, NOT lane ↔ COM port, NOT lane ↔ bootId. Every one of those
// changes while the station stays the same station — which is precisely why
// the node carries a persisted nodeId at all.
//
// NO SESSION STATE LIVES HERE. No athlete, no programme, no score, no phase.
// Those are observed from telemetry and belong to the node; putting any of
// them in the range definition would make the range file go stale the moment
// a match started.
// ─────────────────────────────────────────────────────────────────────────────

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace ta {
namespace rms {

// FIXED     a real installation, saved and reused. Lanes exist whether or not
//           a node is online, and the operator owns the mapping.
// TEMPORARY built from whatever was discovered — training, a demo, a portable
//           installation. Editable afterwards, and deliberately NOT what a
//           saved competition range does on its own.
enum class RangeMode { Fixed, Temporary };

QString toString(RangeMode m);
RangeMode rangeModeFromString(const QString& s, bool* ok = nullptr);

// One physical firing point.
struct LaneDefinition {
    // Stable identity, minted once. Survives renumbering: an operator who
    // re-numbers lanes 1-10 as 11-20 has not built a new range.
    QString laneId;
    int     laneNumber = 0;
    QString displayName;        // optional override; empty = "Lane <number>"
    QString assignedNodeId;     // empty = no station assigned
    bool    enabled = true;     // a lane taken out of service is still a lane
    QString notes;

    bool isAssigned() const { return !assignedNodeId.isEmpty(); }
    QString label() const;

    QJsonObject toJson() const;
    static LaneDefinition fromJson(const QJsonObject& o);
};

struct RangeDefinition {
    QString rangeId;
    QString rangeName;
    // Free text, e.g. "10 m" / "50 m" / "10 m + 50 m". Deliberately NOT an
    // enum: a range's distances are a property of the venue, not a rule, and
    // hard-coding a list here would be a second competition catalogue.
    QString rangeType;
    RangeMode mode = RangeMode::Fixed;
    QVector<LaneDefinition> lanes;

    bool isValid() const { return !rangeId.isEmpty() && !lanes.isEmpty(); }
    int laneCount() const { return int(lanes.size()); }
    int firstLaneNumber() const;
    int lastLaneNumber() const;

    const LaneDefinition* laneByNumber(int number) const;
    const LaneDefinition* laneForNode(const QString& nodeId) const;
    int indexOfLaneNumber(int number) const;

    QJsonObject toJson() const;
    static RangeDefinition fromJson(const QJsonObject& o);

    // Builders. Both mint fresh lane identities; neither reads or writes disk.
    static RangeDefinition createFixed(const QString& name, const QString& type,
                                       int firstLane, int lastLane);
    static RangeDefinition createTemporary(const QString& name,
                                           const QString& type,
                                           const QVector<QString>& nodeIds,
                                           int firstLane = 1);
};

// The persisted document version. Bump ONLY with a migration.
constexpr int kRangeSchemaVersion = 1;

} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGEDEFINITION_H
