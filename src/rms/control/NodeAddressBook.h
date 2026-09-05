#ifndef TA_RMS_NODEADDRESSBOOK_H
#define TA_RMS_NODEADDRESSBOOK_H

// WHERE EACH NODE WAS LAST HEARD FROM.
//
// RMS dials the node, so it needs somewhere to dial. That is all this is: a
// nodeId -> last-seen address map, learned from the telemetry a node already
// broadcasts.
//
// THE ADDRESS IS NOT IDENTITY, AND THIS CLASS IS THE PLACE THAT RULE IS EASIEST
// TO BREAK. A node is its nodeId. The address is only a hint about where to
// find it, and after connecting, the control client VERIFIES the authenticated
// nodeId is the one it meant to reach and drops the connection if it is not.
// Two consequences follow, and both are enforced here rather than hoped for:
//
//   * this map is deliberately SEPARATE from TargetNodeRecord. The observation
//     record is what RMS knows about a lane, and an address does not belong in
//     it - putting it there would invite code to compare lanes by address;
//   * an address that moves is not a new node. DHCP renews, a tablet changes
//     from wired to Wi-Fi: the nodeId is unchanged, so the entry is UPDATED.
//
// It decodes only enough of a datagram to learn the nodeId. It never ingests,
// never scores and never touches the ledger.

#include <QHash>
#include <QHostAddress>
#include <QString>
#include <QStringList>

namespace ta {
namespace rms {
namespace control {

class NodeAddressBook
{
public:
    struct Entry {
        QHostAddress address;
        qint64 lastSeenUtcMs = 0;
        int    updates = 0;      // how often the address CHANGED, not was seen
    };

    // Learns from one telemetry datagram. Returns the nodeId it belonged to, or
    // an empty string when the payload carried none.
    QString observe(const QByteArray& payload, const QHostAddress& sender,
                    qint64 nowUtcMs);

    bool has(const QString& nodeId) const { return m_book.contains(nodeId); }
    QHostAddress addressFor(const QString& nodeId) const
    { return m_book.value(nodeId).address; }
    Entry entryFor(const QString& nodeId) const { return m_book.value(nodeId); }
    QStringList knownNodes() const { return m_book.keys(); }
    int size() const { return m_book.size(); }

    // How many times a known node's address has changed under it. Surfaced
    // because a lane whose address keeps moving is a network story an operator
    // may need, not something to hide.
    int addressChanges() const { return m_addressChanges; }

private:
    QHash<QString, Entry> m_book;
    int m_addressChanges = 0;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_NODEADDRESSBOOK_H
