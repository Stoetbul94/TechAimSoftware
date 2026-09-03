#ifndef TA_RMS_COMMANDJOURNAL_H
#define TA_RMS_COMMANDJOURNAL_H

// THE NODE'S DURABLE HANDLED-COMMAND STORE.
//
// R2B found that command idempotency lived only inside one process: the
// handled-command cache died with the node, so a commandId retried across a
// restart executed a SECOND time. For a START_AT or a paper feed that is a
// physical consequence, not a bookkeeping error. This closes it.
//
// WHAT IS PERSISTED, AND WHY THAT MUCH. Not merely "this id was seen". A
// duplicate must be answerable with the ORIGINAL outcome - accepted or refused,
// with which reason and which resulting state - because RMS retried precisely
// because it does not know what happened. "ALREADY_EXECUTED" would leave the
// operator no better informed than the lost ack did.
//
// WHAT IS NEVER PERSISTED. No key, no MAC, no nonce, no handshake material. The
// journal answers "what did this node already do", and none of those help with
// that. Asserted by serialising the document and looking.
//
// RETENTION IS BOUNDED, AND BOUNDED SAFELY. An unbounded store is a disk leak a
// peer could drive. But expiry has a hard rule above the bounds: an entry for a
// command that could still be dangerously re-executed is NEVER evicted to make
// room. See the retention contract below.

#include "rms/RmsJsonStore.h"
#include "rms/control/ControlProtocol.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace ta {
namespace rms {
namespace control {

// One command this node has already handled, and what came of it.
struct HandledCommand {
    QString commandId;
    QString commandType;
    QString nodeId;
    QString sessionId;        // the session current WHEN IT WAS HANDLED
    bool    accepted = false;
    QString reasonCode;
    QString message;
    QJsonObject resultingState;
    qint64  processedAtUtcMs = 0;

    QJsonObject toJson() const;
    static HandledCommand fromJson(const QJsonObject& o);

    // Rebuilds the acknowledgement the first execution produced. `duplicate` is
    // set, so RMS can tell "this is what happened" from "this just happened".
    Ack toAck(qint64 nowUtcMs) const;
};

// Command types whose re-execution could change the node's state or move
// hardware, and which therefore MUST survive a restart.
//
// PING, REQUEST_STATUS and REQUEST_REPLAY are excluded deliberately: they read
// and change nothing, so repeating one is harmless and journalling them would
// spend the retention budget on traffic that never needed protecting.
bool isDurableCommand(const QString& commandType);

class CommandJournal
{
public:
    // ── the retention contract (§3) ───────────────────────────────────────
    //
    // 1. HARD RULE, ABOVE THE BOUNDS. An entry that is durable AND belongs to
    //    the CURRENT session is never evicted, by count or by age. While that
    //    session can still be acted on, dropping its history would re-arm the
    //    exact double-execution this class exists to prevent.
    // 2. COUNT. At most kMaxEntries. Over budget, the OLDEST evictable entry
    //    goes first. If every entry is protected by rule 1, nothing is evicted
    //    and `retainedOverBudget()` counts it - visible, never silent.
    // 3. AGE. Evictable entries older than kRetentionMs are pruned. Two days
    //    outlives any competition day plus the night after it.
    static constexpr int    kMaxEntries  = 512;
    static constexpr qint64 kRetentionMs = 48LL * 60 * 60 * 1000;

    // The session the node is currently running. Entries for it are protected.
    // Set by the node; never inferred here.
    void setCurrentSession(const QString& sessionId);
    QString currentSession() const { return m_currentSession; }

    bool recall(const QString& commandId, HandledCommand* out) const;
    void record(const HandledCommand& h);

    // Prunes by age. Called on the same tick the node already runs.
    int pruneOlderThan(qint64 nowUtcMs);

    int size() const { return m_order.size(); }
    int evicted() const { return m_evicted; }
    int retainedOverBudget() const { return m_retainedOverBudget; }
    int durableCount() const;

    // ── durability ────────────────────────────────────────────────────────
    // Only DURABLE entries are written. Diagnostics stay in memory and die
    // with the process, which is exactly what they deserve.
    QJsonObject saveState() const;
    void        loadState(const QJsonObject& o);
    StoreResult saveTo(RmsJsonStore& store) const;
    StoreResult loadFrom(RmsJsonStore& store);

    static constexpr int kSchemaVersion = 1;

private:
    // Rule 1: protected from every form of eviction.
    bool isProtected(const HandledCommand& h) const;
    void enforceCountBound();

    QHash<QString, HandledCommand> m_byId;
    QList<QString> m_order;          // oldest first
    QString m_currentSession;
    int m_evicted = 0;
    int m_retainedOverBudget = 0;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_COMMANDJOURNAL_H
