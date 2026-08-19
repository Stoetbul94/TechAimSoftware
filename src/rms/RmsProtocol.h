#ifndef TA_RMS_PROTOCOL_H
#define TA_RMS_PROTOCOL_H

// ─────────────────────────────────────────────────────────────────────────────
// TECH AIM RANGE MANAGEMENT SYSTEM — target-node ⇄ RMS wire protocol, v1.
//
// MILESTONE 1 IS READ-ONLY. This header describes THREE node→range messages
// and NOTHING ELSE. There is deliberately no command message, no command
// encoder and no acknowledgement type: see docs/architecture/
// rms-milestone-1-readonly.md §"Future control design" for the conceptual
// design that is explicitly NOT implemented here.
//
// THE TARGET NODE REMAINS AUTHORITATIVE.
//   - The node computes the score. `authoritativeScore` is transported, never
//     recalculated. RMS carries rawX/rawY for DISPLAY AND DIAGNOSTICS ONLY.
//   - The node owns sequence integrity, its SessionStore, recovery and paper
//     feed. If RMS crashes, closes or is unplugged, the match continues.
//
// TRANSPORT. One JSON object per UDP datagram on the EXISTING lane broadcast
// port 7755 (see the networking audit in the milestone document — `sender.cpp`
// already broadcasts there today). Port 7756 is the node's INBOUND control
// port and is named here only so that it can be asserted unused.
//
// VERSIONING. `protocolVersion` is mandatory on every message. A decoder
// ignores fields it does not know (forward compatible) and REJECTS a version
// it does not implement (never guesses). Rejections are counted, not silent.
//
// IDENTITY. `nodeId` is a stable, persisted identity. It is NOT the COM port,
// NOT the IP address and NOT the lane name — all three change on reconnect,
// re-cabling or re-assignment. `bootId` changes on every node process start,
// which is how RMS distinguishes "the node restarted" from "the network
// blinked".
// ─────────────────────────────────────────────────────────────────────────────

#include <QByteArray>
#include <QString>

namespace ta {
namespace rms {

// The only protocol version this build implements.
constexpr int kProtocolVersion = 1;

// Node → range telemetry. The port `sender.cpp` already broadcasts on.
constexpr quint16 kObservationPort = 7755;

// Range → node control. RESERVED. Milestone 1 must never bind or send here;
// tst_readonly.cpp asserts that no RMS source references it as a destination.
constexpr quint16 kReservedCommandPort = 7756;

enum class MessageType {
    Unknown,
    NodeAnnounce,   // "node.announce" — identity, sent on boot and periodically
    NodeStatus,     // "node.status"   — heartbeat + reconstructable state
    AcceptedShot    // "shot.accepted" — one authoritative accepted shot
};

// Link and target health. Orthogonal to MatchPhase: a node can be
// TargetDisconnected while its match phase is still Match.
// Offline is DERIVED BY RMS from heartbeat silence — a node never sends it.
enum class ConnectionState {
    Unknown,
    Online,
    TargetConnected,
    TargetDisconnected,
    Reconnecting,
    Offline
};

// Where the node's local match state machine is. The node owns this; RMS
// only mirrors it.
enum class MatchPhase {
    Unknown,
    Idle,
    Preparation,
    Sighting,
    Match,
    PositionChange,
    Complete,
    RecoveryRequired
};

QString toString(ConnectionState s);
QString toString(MatchPhase p);
ConnectionState connectionStateFromString(const QString& s);
MatchPhase matchPhaseFromString(const QString& s);

struct NodeAnnounce {
    int     protocolVersion = 0;
    QString nodeId;           // stable persisted identity — never the COM port
    QString bootId;           // new per process start → detects node restart
    QString laneId;           // empty when the node has no lane assigned yet
    QString deviceIdentity;   // target device fingerprint (serial / firmware)
    QString appVersion;
    QString productIdentity;  // "Tech Aim", "SETA", …
    qint64  timestampUtcMs = 0;
};

struct NodeStatus {
    int     protocolVersion = 0;
    QString nodeId;
    QString bootId;
    QString laneId;
    QString sessionId;        // empty when no session is open
    QString programmeId;      // STABLE identity — never a translated label
    QString rulesetId;
    QString targetStandardId;
    QString athleteName;      // presentation only
    QString position;         // "", "PRONE", "STANDING", "KNEELING"
    ConnectionState connection = ConnectionState::Unknown;
    MatchPhase      phase      = MatchPhase::Unknown;
    int     shotsAccepted = 0;    // NODE-AUTHORITATIVE accepted count
    int     shotsExpected = -1;   // -1 = unlimited
    double  totalScore    = 0.0;  // NODE-AUTHORITATIVE running total
    QString health;               // free-text node health note
    quint64 statusSeq     = 0;    // monotonic per boot → discards stale status
    qint64  timestampUtcMs = 0;
};

struct AcceptedShot {
    int     protocolVersion = 0;
    QString eventId;          // globally unique → transport-level dedup
    QString nodeId;
    QString bootId;
    QString laneId;
    QString sessionId;
    QString programmeId;
    QString position;
    int     shotSequence = 0;     // 1-based, monotonic WITHIN a session
    double  rawXMm = 0.0;         // diagnostics/display only
    double  rawYMm = 0.0;         // diagnostics/display only
    double  authoritativeScore = 0.0;  // COMPUTED BY THE NODE. Never re-scored.
    int     integerScore = -1;         // -1 when not applicable
    bool    innerTen = false;
    qint64  timestampUtcMs = 0;
    QString acquisitionStatus;    // "ACCEPTED" | "RECOVERED" | "MANUAL"
};

// One decoded datagram. `type == Unknown` means the payload was rejected and
// `rejectReason` says why — nothing is ever half-accepted.
struct DecodedMessage {
    MessageType  type = MessageType::Unknown;
    QString      rejectReason;
    NodeAnnounce announce;
    NodeStatus   status;
    AcceptedShot shot;
};

DecodedMessage decode(const QByteArray& datagram);

// Encoders exist so the development simulator and the tests speak EXACTLY the
// wire format the decoder reads — there is no second description of the
// protocol to drift. They encode node→range telemetry only; encoding a
// command is not possible because no command type exists.
QByteArray encode(const NodeAnnounce& m);
QByteArray encode(const NodeStatus& m);
QByteArray encode(const AcceptedShot& m);

} // namespace rms
} // namespace ta

#endif // TA_RMS_PROTOCOL_H
