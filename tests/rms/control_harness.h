#ifndef TA_RMS_TEST_CONTROL_HARNESS_H
#define TA_RMS_TEST_CONTROL_HARNESS_H

// A range of simulated nodes wired to a real RangeControlCoordinator.
//
// THE POINT: nothing here shortcuts the protocol. The "network" is a function
// that hands one node's endpoint the exact bytes RMS produced and hands RMS
// back the exact bytes the node produced. Every ack in these tests therefore
// came out of the production NodeControlEndpoint, through the production
// framing, after the production HMAC handshake.
//
// The link can be CUT per node, which is how offline behaviour is tested: a
// cut link returns nothing, and "nothing" is what an unreachable node looks
// like to the coordinator.

#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/control/RangeControlCoordinator.h"
#include "rms/dev/ControlledNode.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

namespace ta {
namespace rms {
namespace test {

class ControlHarness
{
public:
    explicit ControlHarness(const QByteArray& key,
                            const QString& instanceId = QStringLiteral("RMS-1"))
        : m_key(key), m_coordinator(instanceId, key)
    {
        m_coordinator.setLink([this](const QString& nodeId, const QByteArray& frame) {
            return deliver(nodeId, frame);
        });
    }

    // Adds a lane. Each node gets its own session and its own clock offset, so
    // nothing can pass by accident through every lane sharing one clock.
    dev::ControlledNode* addNode(const QString& nodeId, const QString& laneId,
                                 qint64 clockOffsetMs = 0)
    {
        auto n = std::make_unique<dev::ControlledNode>(
            nodeId, laneId, QStringLiteral("sess-%1").arg(nodeId), m_key);
        n->setClockOffsetMs(clockOffsetMs);
        dev::ControlledNode* raw = n.get();
        m_index.insert(nodeId, int(m_nodes.size()));
        m_nodes.push_back(std::move(n));
        m_order.append(nodeId);
        return raw;
    }

    dev::ControlledNode* node(const QString& nodeId) const
    {
        const auto it = m_index.find(nodeId);
        return it == m_index.end() ? nullptr : m_nodes[*it].get();
    }

    QStringList nodeIds() const { return m_order; }

    // The CONTROL link, separate from the telemetry link. A range can lose one
    // and keep the other, and conflating them would hide exactly that case.
    void setControlLink(const QString& nodeId, bool up)
    { if (up) m_cutControl.remove(nodeId); else m_cutControl.insert(nodeId); }

    // A LOST ACKNOWLEDGEMENT: the node RECEIVES the frame, applies it and
    // journals it - and the answer never gets back. This is not the same as a
    // dead link, and the difference is the whole problem. RMS is left unable to
    // tell "it never arrived" from "it arrived and I did not hear", which is
    // why a retry must be idempotent rather than merely unlikely.
    void setSwallowReplies(const QString& nodeId, bool swallow)
    { if (swallow) m_swallow.insert(nodeId); else m_swallow.remove(nodeId); }

    // Moves whatever the nodes have produced into the monitor - the same
    // ingest a real UDP datagram takes.
    int pumpTelemetry(qint64 nowUtcMs)
    {
        int n = 0;
        for (const QString& id : m_order) {
            for (const QByteArray& d : node(id)->drainTelemetry()) {
                m_monitor.ingestDatagram(d, nowUtcMs);
                ++n;
            }
        }
        return n;
    }

    // The node's status message, which is what tells RMS how many shots the
    // node says it has accepted. Without this RMS cannot know it is behind:
    // shots it never received cannot announce their own absence.
    void pumpStatus(const QString& nodeId, qint64 nowUtcMs)
    {
        dev::ControlledNode* n = node(nodeId);
        if (!n) return;
        NodeStatus s;
        s.protocolVersion = kProtocolVersion;
        s.nodeId    = n->nodeId();
        s.bootId    = n->bootId();
        s.laneId    = n->laneId();
        s.sessionId = n->sessionId();
        s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
        s.connection = ConnectionState::TargetConnected;
        s.phase      = MatchPhase::Match;
        // NODE-AUTHORITATIVE. This is the number RMS compares its own ledger
        // against, and it counts shots the node took while RMS was not looking.
        s.shotsAccepted = n->eventCount();
        s.shotsExpected = 60;
        s.statusSeq  = ++m_statusSeq[nodeId];
        s.timestampUtcMs = nowUtcMs;
        m_monitor.ingestDatagram(encode(s), nowUtcMs);
    }
    void pumpAllStatus(qint64 nowUtcMs)
    { for (const QString& id : m_order) pumpStatus(id, nowUtcMs); }

    void setNow(qint64 nowUtcMs) { m_now = nowUtcMs; }
    qint64 now() const { return m_now; }

    RangeMonitor& monitor() { return m_monitor; }
    control::RangeControlCoordinator& coordinator() { return m_coordinator; }

    int connectAll()
    {
        int ok = 0;
        for (const QString& id : m_order)
            if (m_coordinator.connectNode(id)) ++ok;
        return ok;
    }

private:
    QByteArray deliver(const QString& nodeId, const QByteArray& frame)
    {
        if (m_cutControl.contains(nodeId))
            return QByteArray();          // unreachable: no answer at all
        dev::ControlledNode* n = node(nodeId);
        if (!n)
            return QByteArray();
        // The node processes it either way - that is what makes this a LOST
        // ACK rather than a lost command.
        const QByteArray reply = n->endpoint().onBytes(frame, m_now).reply;
        if (m_swallow.contains(nodeId))
            return QByteArray();
        return reply;
    }

    QByteArray m_key;
    qint64 m_now = 1'700'000'000'000LL;
    RangeMonitor m_monitor;
    control::RangeControlCoordinator m_coordinator;
    std::vector<std::unique_ptr<dev::ControlledNode>> m_nodes;
    QHash<QString, int> m_index;
    QStringList m_order;
    QSet<QString> m_cutControl;
    QSet<QString> m_swallow;
    QHash<QString, quint64> m_statusSeq;
};

} // namespace test
} // namespace rms
} // namespace ta

#endif // TA_RMS_TEST_CONTROL_HARNESS_H
