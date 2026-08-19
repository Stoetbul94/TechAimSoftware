#ifndef TA_RMS_RANGECONFIGURATIONSERVICE_H
#define TA_RMS_RANGECONFIGURATIONSERVICE_H

// ─────────────────────────────────────────────────────────────────────────────
// The one place the range configuration is changed.
//
// Every mutation goes through here, is validated here, and is persisted here.
// QML calls these invokables; nothing else edits a RangeDefinition, so there
// is exactly one implementation of the assignment rules to reason about.
//
// THIS IS THE ONLY THING RMS IS ALLOWED TO WRITE. It writes RMS's own range
// file. It sends nothing to a node, and no method here has a target-facing
// counterpart — assigning lane 4 to a station tells RMS where that station
// is, and tells the station nothing at all.
//
// ASSIGNMENT RULES
//   - one node belongs to at most one lane in a range;
//   - one lane holds at most one node;
//   - moving a node from lane 2 to lane 7 clears lane 2 and sets lane 7 in a
//     SINGLE committed change, because a half-applied move would leave one
//     station showing on two lanes, and a range officer acting on that is how
//     a shot ends up credited to the wrong athlete.
// ─────────────────────────────────────────────────────────────────────────────

#include "RangeDefinition.h"
#include "RangeStore.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace ta {
namespace rms {

class RangeConfigurationService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ isConfigured NOTIFY rangeChanged)
    Q_PROPERTY(QString rangeName READ rangeName NOTIFY rangeChanged)
    Q_PROPERTY(QString rangeType READ rangeType NOTIFY rangeChanged)
    Q_PROPERTY(QString rangeMode READ rangeModeLabel NOTIFY rangeChanged)
    Q_PROPERTY(int laneCount READ laneCount NOTIFY rangeChanged)
    Q_PROPERTY(int firstLaneNumber READ firstLaneNumber NOTIFY rangeChanged)
    Q_PROPERTY(int lastLaneNumber READ lastLaneNumber NOTIFY rangeChanged)
    Q_PROPERTY(int assignedLaneCount READ assignedLaneCount NOTIFY rangeChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    // True when the on-disk file was written by a newer RMS. The UI must say
    // so rather than silently offering to build a replacement range.
    Q_PROPERTY(bool configLocked READ isConfigLocked NOTIFY rangeChanged)

public:
    explicit RangeConfigurationService(QObject* parent = nullptr);

    void setStorePath(const QString& path);
    // Reads what is on disk. Missing is not an error — it is first run.
    void load();

    const RangeDefinition& range() const { return m_range; }
    bool isConfigured() const { return m_range.isValid(); }
    bool isConfigLocked() const { return m_store.isWriteBlocked(); }
    QString rangeName() const { return m_range.rangeName; }
    QString rangeType() const { return m_range.rangeType; }
    QString rangeModeLabel() const;
    int laneCount() const { return m_range.laneCount(); }
    int firstLaneNumber() const { return m_range.firstLaneNumber(); }
    int lastLaneNumber() const { return m_range.lastLaneNumber(); }
    int assignedLaneCount() const;
    QString configPath() const { return m_store.path(); }
    QString lastError() const { return m_lastError; }

    // ── configuration (the only writes RMS performs) ─────────────────────
    Q_INVOKABLE bool createFixedRange(const QString& name, const QString& type,
                                      int firstLane, int lastLane);
    // `nodeIds` in discovered order becomes the lane order. A STARTING POINT
    // for training and demos, never the default for a saved competition range.
    Q_INVOKABLE bool createTemporaryRange(const QString& name, const QString& type,
                                          const QStringList& nodeIds,
                                          int firstLane = 1);
    Q_INVOKABLE bool assignNodeToLane(const QString& nodeId, int laneNumber);
    Q_INVOKABLE bool clearLane(int laneNumber);
    Q_INVOKABLE bool setLaneEnabled(int laneNumber, bool enabled);
    Q_INVOKABLE bool renameRange(const QString& name);
    // Development/test only: forget the configuration so first-run can be
    // demonstrated. Never reachable from the production UI.
    Q_INVOKABLE void forgetRangeForDevelopment();

    // ── queries ──────────────────────────────────────────────────────────
    Q_INVOKABLE QVariantMap laneAt(int index) const;
    Q_INVOKABLE int laneNumberForNode(const QString& nodeId) const;
    Q_INVOKABLE QString nodeForLaneNumber(int laneNumber) const;
    Q_INVOKABLE bool isNodeAssigned(const QString& nodeId) const;
    Q_INVOKABLE QVariantList laneNumbers() const;

signals:
    void rangeChanged();
    void lastErrorChanged();
    // Emitted when an operator action was refused, with a sentence explaining
    // why. Silent refusal is not an option: the operator must know the
    // assignment did not happen.
    void assignmentRejected(const QString& reason);

private:
    bool commit();
    void setError(const QString& reason);
    void clearError();

    RangeStore m_store;
    RangeDefinition m_range;
    QString m_lastError;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGECONFIGURATIONSERVICE_H
