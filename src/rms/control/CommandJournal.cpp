#include "rms/control/CommandJournal.h"

#include <QJsonArray>

namespace ta {
namespace rms {
namespace control {

QJsonObject HandledCommand::toJson() const
{
    // Every field here answers "what did this node already do". None of them
    // is handshake material, and none may become so.
    return QJsonObject{
        {"commandId", commandId}, {"commandType", commandType},
        {"nodeId", nodeId}, {"sessionId", sessionId},
        {"accepted", accepted}, {"reasonCode", reasonCode},
        {"message", message}, {"resultingState", resultingState},
        {"processedAtUtcMs", double(processedAtUtcMs)}};
}

HandledCommand HandledCommand::fromJson(const QJsonObject& o)
{
    HandledCommand h;
    h.commandId   = o.value("commandId").toString();
    h.commandType = o.value("commandType").toString();
    h.nodeId      = o.value("nodeId").toString();
    h.sessionId   = o.value("sessionId").toString();
    h.accepted    = o.value("accepted").toBool();
    h.reasonCode  = o.value("reasonCode").toString();
    h.message     = o.value("message").toString();
    h.resultingState = o.value("resultingState").toObject();
    h.processedAtUtcMs = qint64(o.value("processedAtUtcMs").toDouble());
    return h;
}

Ack HandledCommand::toAck(qint64 nowUtcMs) const
{
    Ack a;
    a.controlProtocolVersion = kControlProtocolVersion;
    a.commandId = commandId;
    a.nodeId    = nodeId;
    // THE ORIGINAL OUTCOME, not a bare "already executed". RMS retried because
    // it does not know what happened; telling it only that the id was seen
    // would leave it exactly as ignorant as the lost ack did.
    a.accepted  = accepted;
    a.reasonCode = reasonCode;
    a.message   = message;
    a.resultingState = resultingState;
    a.nodeTimestampUtcMs = nowUtcMs;
    a.duplicate = true;
    return a;
}

bool isDurableCommand(const QString& commandType)
{
    // Everything that changes state or moves hardware. FEED_PAPER is included
    // even though it is capability-gated off: when a node adapter turns it on,
    // it must arrive already protected rather than needing a second change.
    return commandType == QLatin1String(cmd::kAssignAthlete)
        || commandType == QLatin1String(cmd::kPrepareSession)
        || commandType == QLatin1String(cmd::kStartAt)
        || commandType == QLatin1String(cmd::kStop)
        || commandType == QLatin1String(cmd::kFeedPaper);
}

void CommandJournal::setCurrentSession(const QString& sessionId)
{
    m_currentSession = sessionId;
    // A session change can UNPROTECT entries that were protected a moment ago,
    // so the count bound is re-applied here rather than waiting for the next
    // write. Otherwise a long-lived node would sit permanently over budget.
    enforceCountBound();
}

bool CommandJournal::isProtected(const HandledCommand& h) const
{
    // Rule 1. Note the current session being empty protects nothing: a node
    // with no session open has nothing that can be dangerously re-executed.
    return !m_currentSession.isEmpty()
        && h.sessionId == m_currentSession
        && isDurableCommand(h.commandType);
}

bool CommandJournal::recall(const QString& commandId, HandledCommand* out) const
{
    const auto it = m_byId.constFind(commandId);
    if (it == m_byId.constEnd())
        return false;
    if (out) *out = it.value();
    return true;
}

void CommandJournal::record(const HandledCommand& h)
{
    if (h.commandId.isEmpty())
        return;
    if (!m_byId.contains(h.commandId))
        m_order.append(h.commandId);
    m_byId.insert(h.commandId, h);
    enforceCountBound();
}

void CommandJournal::enforceCountBound()
{
    // Oldest evictable first. A protected entry is SKIPPED, not evicted, so a
    // burst of protected commands cannot push an earlier protected one out.
    int i = 0;
    while (m_order.size() > kMaxEntries && i < m_order.size()) {
        const QString& id = m_order.at(i);
        const auto it = m_byId.constFind(id);
        if (it != m_byId.constEnd() && isProtected(it.value())) {
            ++i;                       // skip it and look further along
            continue;
        }
        m_byId.remove(id);
        m_order.removeAt(i);
        ++m_evicted;
    }
    if (m_order.size() > kMaxEntries) {
        // Everything left is protected. Keeping it is the correct answer -
        // silently dropping a command that can still be re-executed is the
        // failure this class exists to prevent - but it must be VISIBLE.
        ++m_retainedOverBudget;
    }
}

int CommandJournal::pruneOlderThan(qint64 nowUtcMs)
{
    int removed = 0;
    for (int i = 0; i < m_order.size();) {
        const QString id = m_order.at(i);
        const auto it = m_byId.constFind(id);
        if (it == m_byId.constEnd()) { m_order.removeAt(i); continue; }
        const HandledCommand& h = it.value();
        const bool old = h.processedAtUtcMs > 0
                         && (nowUtcMs - h.processedAtUtcMs) > kRetentionMs;
        if (old && !isProtected(h)) {     // rule 1 outranks the age bound
            m_byId.remove(id);
            m_order.removeAt(i);
            ++removed;
            ++m_evicted;
            continue;
        }
        ++i;
    }
    return removed;
}

int CommandJournal::durableCount() const
{
    int n = 0;
    for (const QString& id : m_order)
        if (isDurableCommand(m_byId.value(id).commandType)) ++n;
    return n;
}

QJsonObject CommandJournal::saveState() const
{
    QJsonArray a;
    for (const QString& id : m_order) {
        const HandledCommand h = m_byId.value(id);
        // Diagnostics are not written. They change nothing, so nothing is at
        // risk if they are forgotten across a restart.
        if (isDurableCommand(h.commandType))
            a.append(h.toJson());
    }
    return QJsonObject{{"currentSession", m_currentSession}, {"handled", a}};
}

void CommandJournal::loadState(const QJsonObject& o)
{
    m_byId.clear();
    m_order.clear();
    m_currentSession = o.value("currentSession").toString();
    const QJsonArray a = o.value("handled").toArray();
    for (const QJsonValue& v : a) {
        const HandledCommand h = HandledCommand::fromJson(v.toObject());
        if (h.commandId.isEmpty())
            continue;
        m_byId.insert(h.commandId, h);
        m_order.append(h.commandId);
    }
    enforceCountBound();
}

StoreResult CommandJournal::saveTo(RmsJsonStore& store) const
{
    return store.save(kSchemaVersion, saveState());
}

StoreResult CommandJournal::loadFrom(RmsJsonStore& store)
{
    QJsonObject doc;
    const StoreResult r = store.load(kSchemaVersion, &doc);
    // A failed read loads NOTHING. Half a journal is worse than none: it would
    // claim protection for the commands it did read and leave the rest exposed,
    // with no way to tell which was which.
    if (r.ok)
        loadState(doc);
    return r;
}

} // namespace control
} // namespace rms
} // namespace ta
