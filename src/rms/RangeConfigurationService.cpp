#include "RangeConfigurationService.h"

#include <QFile>

namespace ta {
namespace rms {

RangeConfigurationService::RangeConfigurationService(QObject* parent)
    : QObject(parent)
{
}

void RangeConfigurationService::setStorePath(const QString& path)
{
    m_store.setPath(path);
}

void RangeConfigurationService::load()
{
    RangeDefinition loaded;
    const RangeStoreResult r = m_store.load(&loaded);
    if (r.ok) {
        m_range = loaded;
        clearError();
    } else if (r.error == RangeStoreError::NotFound) {
        // Not a fault. This is a machine that has never had a range set up,
        // and the UI's answer is the first-run page, not an error.
        m_range = RangeDefinition();
        clearError();
    } else {
        m_range = RangeDefinition();
        setError(r.detail);
    }
    emit rangeChanged();
}

QString RangeConfigurationService::rangeModeLabel() const
{
    return m_range.mode == RangeMode::Temporary ? QStringLiteral("Temporary")
                                                : QStringLiteral("Fixed");
}

int RangeConfigurationService::assignedLaneCount() const
{
    int n = 0;
    for (const LaneDefinition& l : m_range.lanes)
        if (l.isAssigned())
            ++n;
    return n;
}

void RangeConfigurationService::setError(const QString& reason)
{
    if (m_lastError == reason)
        return;
    m_lastError = reason;
    emit lastErrorChanged();
}

void RangeConfigurationService::clearError()
{
    setError(QString());
}

bool RangeConfigurationService::commit()
{
    const RangeStoreResult r = m_store.save(m_range);
    if (!r.ok) {
        setError(r.detail);
        emit rangeChanged();
        return false;
    }
    clearError();
    emit rangeChanged();
    return true;
}

// ── configuration ────────────────────────────────────────────────────────

bool RangeConfigurationService::createFixedRange(const QString& name,
                                                 const QString& type,
                                                 int firstLane, int lastLane)
{
    if (name.trimmed().isEmpty()) {
        setError(QStringLiteral("A range needs a name."));
        emit assignmentRejected(m_lastError);
        return false;
    }
    if (firstLane < 1 || lastLane < firstLane) {
        setError(QStringLiteral("Lane numbering must run from %1 upwards.")
                     .arg(qMax(1, firstLane)));
        emit assignmentRejected(m_lastError);
        return false;
    }
    m_range = RangeDefinition::createFixed(name.trimmed(), type.trimmed(),
                                           firstLane, lastLane);
    return commit();
}

bool RangeConfigurationService::createTemporaryRange(const QString& name,
                                                     const QString& type,
                                                     const QStringList& nodeIds,
                                                     int firstLane)
{
    if (nodeIds.isEmpty()) {
        setError(QStringLiteral("No devices have been discovered to build lanes from."));
        emit assignmentRejected(m_lastError);
        return false;
    }
    QVector<QString> ids;
    for (const QString& id : nodeIds) {
        // A node discovered twice must not become two lanes.
        if (!id.isEmpty() && !ids.contains(id))
            ids.append(id);
    }
    m_range = RangeDefinition::createTemporary(
        name.trimmed().isEmpty() ? QStringLiteral("Temporary range") : name.trimmed(),
        type.trimmed(), ids, qMax(1, firstLane));
    return commit();
}

bool RangeConfigurationService::assignNodeToLane(const QString& nodeId, int laneNumber)
{
    if (!isConfigured()) {
        setError(QStringLiteral("No range is configured."));
        emit assignmentRejected(m_lastError);
        return false;
    }
    const int target = m_range.indexOfLaneNumber(laneNumber);
    if (target < 0) {
        setError(QStringLiteral("Lane %1 is not part of this range.").arg(laneNumber));
        emit assignmentRejected(m_lastError);
        return false;
    }
    if (nodeId.isEmpty()) {
        setError(QStringLiteral("No device was selected."));
        emit assignmentRejected(m_lastError);
        return false;
    }

    // Already exactly where it is being put: succeed quietly rather than
    // rewrite the file for nothing.
    if (m_range.lanes.at(target).assignedNodeId == nodeId) {
        clearError();
        return true;
    }

    // The destination lane is occupied by a DIFFERENT station. Refuse, and say
    // by whom — silently displacing the other station would be the surprising
    // and dangerous behaviour.
    if (m_range.lanes.at(target).isAssigned()) {
        setError(QStringLiteral("Lane %1 already has %2 assigned. Clear it first.")
                     .arg(laneNumber)
                     .arg(m_range.lanes.at(target).assignedNodeId));
        emit assignmentRejected(m_lastError);
        return false;
    }

    // A move. Clearing the old lane and setting the new one happen in the same
    // in-memory edit and are persisted by ONE atomic save, so the station is
    // never on two lanes and never on none.
    const int previous = m_range.indexOfLaneNumber(laneNumberForNode(nodeId));
    if (previous >= 0)
        m_range.lanes[previous].assignedNodeId.clear();
    m_range.lanes[target].assignedNodeId = nodeId;
    return commit();
}

bool RangeConfigurationService::clearLane(int laneNumber)
{
    const int i = m_range.indexOfLaneNumber(laneNumber);
    if (i < 0) {
        setError(QStringLiteral("Lane %1 is not part of this range.").arg(laneNumber));
        emit assignmentRejected(m_lastError);
        return false;
    }
    if (!m_range.lanes.at(i).isAssigned()) {
        clearError();
        return true;
    }
    m_range.lanes[i].assignedNodeId.clear();
    return commit();
}

bool RangeConfigurationService::setLaneEnabled(int laneNumber, bool enabled)
{
    const int i = m_range.indexOfLaneNumber(laneNumber);
    if (i < 0)
        return false;
    if (m_range.lanes.at(i).enabled == enabled)
        return true;
    m_range.lanes[i].enabled = enabled;
    return commit();
}

bool RangeConfigurationService::renameRange(const QString& name)
{
    if (name.trimmed().isEmpty() || !isConfigured())
        return false;
    m_range.rangeName = name.trimmed();
    return commit();
}

void RangeConfigurationService::forgetRangeForDevelopment()
{
    QFile::remove(m_store.path());
    m_range = RangeDefinition();
    clearError();
    emit rangeChanged();
}

// ── queries ──────────────────────────────────────────────────────────────

QVariantMap RangeConfigurationService::laneAt(int index) const
{
    QVariantMap m;
    if (index < 0 || index >= m_range.lanes.size())
        return m;
    const LaneDefinition& l = m_range.lanes.at(index);
    m[QStringLiteral("laneId")]         = l.laneId;
    m[QStringLiteral("laneNumber")]     = l.laneNumber;
    m[QStringLiteral("label")]          = l.label();
    m[QStringLiteral("assignedNodeId")] = l.assignedNodeId;
    m[QStringLiteral("assigned")]       = l.isAssigned();
    m[QStringLiteral("enabled")]        = l.enabled;
    m[QStringLiteral("notes")]          = l.notes;
    return m;
}

int RangeConfigurationService::laneNumberForNode(const QString& nodeId) const
{
    const LaneDefinition* l = m_range.laneForNode(nodeId);
    return l ? l->laneNumber : -1;
}

QString RangeConfigurationService::nodeForLaneNumber(int laneNumber) const
{
    const LaneDefinition* l = m_range.laneByNumber(laneNumber);
    return l ? l->assignedNodeId : QString();
}

bool RangeConfigurationService::isNodeAssigned(const QString& nodeId) const
{
    return m_range.laneForNode(nodeId) != nullptr;
}

QVariantList RangeConfigurationService::laneNumbers() const
{
    QVariantList out;
    for (const LaneDefinition& l : m_range.lanes)
        out.append(l.laneNumber);
    return out;
}

} // namespace rms
} // namespace ta
