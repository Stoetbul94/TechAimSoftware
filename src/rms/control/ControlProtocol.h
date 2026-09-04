#ifndef TA_RMS_CONTROLPROTOCOL_H
#define TA_RMS_CONTROLPROTOCOL_H

// ─────────────────────────────────────────────────────────────────────────────
// RMS CONTROL PLANE — wire contract, v1.
//
// This is the SHARED contract for the authenticated control/replay channel,
// exactly as RmsProtocol.h is for read-only telemetry. Both ends compile these
// bytes from this file; a second hand-maintained copy would drift, and a
// control protocol that drifts silently sends the wrong command to the wrong
// lane.
//
// TELEMETRY IS UNTOUCHED. UDP 7755 remains node→range, read-only and
// unauthenticated. Nothing here changes it.
//
// THE PORT, HONESTLY. UDP 7756 is NOT free: receiverTachus.cpp binds it for the
// target application's historical startMatchFromServer path, which predates
// RMS. The control channel therefore uses TCP 7756 - a different socket - and
// the legacy UDP listener keeps running untouched. Retiring that legacy path is
// the node's decision and its own reviewed change, not this milestone's.
//
// FRAMING. TCP is a byte stream; one read() is not one message. Every frame is
// a 4-byte big-endian length followed by that many bytes of UTF-8 JSON. The
// length is VALIDATED BEFORE ANYTHING IS ALLOCATED, so a peer cannot make us
// reserve 4 GiB by claiming to.
//
// The framing here is deliberately transport-free: it takes bytes and returns
// frames. That is what lets partial frames, two-frames-in-one-read, oversize
// frames and mid-frame disconnects be tested deterministically, without a
// socket and without timing.
// ─────────────────────────────────────────────────────────────────────────────

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace ta {
namespace rms {
namespace control {

// Independent of the telemetry protocolVersion and of any application version.
// A peer that speaks a version we do not implement is REJECTED, never silently
// downgraded.
constexpr int kControlProtocolVersion = 1;

// TCP. See the port note above.
constexpr quint16 kControlPort = 7756;

// Size limits. The absolute cap is checked against the DECLARED length before
// the body is read, so an oversize claim costs nothing.
constexpr int kMaxFrameBytes     = 256 * 1024;
constexpr int kMaxHandshakeBytes = 8 * 1024;
constexpr int kMaxCommandBytes   = 16 * 1024;
constexpr int kMaxAckBytes       = 16 * 1024;
constexpr int kMaxReplayBytes    = 256 * 1024;

// Replay batching. A session of any length is fetched as successive batches;
// nothing assumes one response holds everything.
constexpr int kMaxReplayEvents = 200;

enum class MessageType {
    Unknown,
    Hello,          // RMS  → node : version, instance id, nonce
    Challenge,      // node → RMS  : identity, capabilities, node nonce
    Auth,           // RMS  → node : the MAC
    AuthResult,     // node → RMS  : accepted / rejected(reason)
    Command,        // RMS  → node
    Ack,            // node → RMS
    ReplayBatch     // node → RMS  : answer to REQUEST_REPLAY
};

QString messageTypeName(MessageType t);
MessageType messageTypeFromName(const QString& s);

// ── framing ──────────────────────────────────────────────────────────────
// Feed bytes in; take whole frames out. Everything about TCP's byte-stream
// nature lives here and nowhere else.
class FrameReader
{
public:
    enum class Status { Ok, Oversize, Malformed };

    // Appends bytes and extracts every complete frame now available.
    // Returns Oversize (and stops) if a declared length exceeds the cap - the
    // caller must close the connection; the stream cannot be resynchronised
    // because the length itself was the thing we could not trust.
    Status append(const QByteArray& bytes, QList<QByteArray>* framesOut);

    // Bytes held from an incomplete frame. A clean disconnect with a non-empty
    // remainder is a TRUNCATED message, not an empty one.
    int pendingBytes() const { return m_buffer.size(); }
    bool hasPartialFrame() const { return !m_buffer.isEmpty(); }
    void reset() { m_buffer.clear(); }

private:
    QByteArray m_buffer;
};

// Wraps a payload in its length prefix.
QByteArray frame(const QByteArray& payload);

// ── messages ─────────────────────────────────────────────────────────────

struct Hello {
    int     controlProtocolVersion = 0;
    QString rmsInstanceId;
    QString rmsNonce;
};

struct Challenge {
    int         controlProtocolVersion = 0;
    QString     nodeId;
    QString     bootId;
    QString     product;
    QString     appVersion;
    QString     commit;
    QStringList capabilities;
    QString     nodeNonce;
};

struct Auth {
    int     controlProtocolVersion = 0;
    QString mac;              // hex HMAC-SHA256. The KEY never travels.
};

struct AuthResult {
    int     controlProtocolVersion = 0;
    bool    accepted = false;
    QString reasonCode;       // machine-readable; "OK" when accepted
    QString nodeId;
};

struct Command {
    int     controlProtocolVersion = 0;
    QString commandId;        // RMS-generated, stable across a retry
    QString nodeId;
    QString laneId;
    QString sessionId;
    QString commandType;
    qint64  issuedAtUtcMs = 0;
    QJsonObject payload;
};

struct Ack {
    int     controlProtocolVersion = 0;
    QString commandId;
    QString nodeId;
    bool    accepted = false;
    QString reasonCode;
    QString message;
    QJsonObject resultingState;
    qint64  nodeTimestampUtcMs = 0;
    // True when this command id had already been handled: the action was NOT
    // performed again and this is the original outcome.
    bool    duplicate = false;
};

struct ReplayBatch {
    int     controlProtocolVersion = 0;
    QString requestId;
    QString sessionId;
    int     fromSequence = 0;
    QList<QJsonObject> events;   // ORIGINAL event objects, ids preserved
    bool    hasMore = false;
    int     nextSequence = 0;
};

// One decoded frame. Unknown means it was rejected and rejectReason says why;
// nothing is ever half-accepted.
struct DecodedControl {
    MessageType type = MessageType::Unknown;
    QString     rejectReason;
    Hello       hello;
    Challenge   challenge;
    Auth        auth;
    AuthResult  authResult;
    Command     command;
    Ack         ack;
    ReplayBatch replay;
};

DecodedControl decodeControl(const QByteArray& payload);

QByteArray encode(const Hello& m);
QByteArray encode(const Challenge& m);
QByteArray encode(const Auth& m);
QByteArray encode(const AuthResult& m);
QByteArray encode(const Command& m);
QByteArray encode(const Ack& m);
QByteArray encode(const ReplayBatch& m);

// ── command vocabulary (§15) ─────────────────────────────────────────────
// Finite on purpose. FEED_PAPER exists in the grammar but is capability-gated
// OFF by default: it moves physical hardware and does not become an
// operator-visible range command until a node adapter is physically validated.
namespace cmd {
extern const char* kPing;
extern const char* kRequestStatus;
extern const char* kRequestReplay;
extern const char* kAssignAthlete;
extern const char* kPrepareSession;
extern const char* kStartAt;
extern const char* kStop;
extern const char* kFeedPaper;
}

// ── capabilities (§11) ───────────────────────────────────────────────────
namespace cap {
extern const char* kStatus;
extern const char* kEventReplay;
extern const char* kAthleteAssignment;
extern const char* kSessionPrepare;
extern const char* kStartAt;
extern const char* kStop;
extern const char* kPaperFeed;
}

// ── reason codes ─────────────────────────────────────────────────────────
namespace reason {
extern const char* kOk;
extern const char* kUnsupportedCapability;
extern const char* kUnknownCommand;
extern const char* kNotAuthenticated;
extern const char* kBadVersion;
extern const char* kBadNode;
extern const char* kAuthFailed;
extern const char* kStaleCommand;
extern const char* kMalformed;
extern const char* kPreconditionFailed;
}

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_CONTROLPROTOCOL_H
