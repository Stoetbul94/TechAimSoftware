#ifndef TA_RMS_CONTROLAUTH_H
#define TA_RMS_CONTROLAUTH_H

// RMS control authentication — HMAC-SHA256 over a pre-shared range key.
//
// WHAT THIS DEFENDS. Without it, any device on the range Wi-Fi could start,
// stop or reassign a lane. That is the actual risk a control channel creates,
// and it is why no state-changing command exists until this does.
//
// THE MAC:
//     HMAC-SHA256( rangeKey,  rmsNonce ‖ nodeNonce ‖ nodeId ‖ rmsInstanceId )
//
// Every term earns its place:
//   rmsNonce   - so a node cannot fix the challenge and precompute
//   nodeNonce  - so a recorded exchange cannot be replayed AT the node
//   nodeId     - so a valid exchange captured at lane 3 cannot be replayed at
//                lane 4. This is what binds identity to the handshake rather
//                than merely asserting it afterwards
//   instanceId - so two RMS instances produce different MACs
//
// THE KEY NEVER TRAVELS. Only the MAC crosses the wire, and only the MAC is
// ever compared. The key is not logged, not committed, and not printed by any
// diagnostic in this file.
//
// NO CUSTOM CRYPTOGRAPHY. QMessageAuthenticationCode with Sha256.

#include <QByteArray>
#include <QString>

namespace ta {
namespace rms {
namespace control {

// Fresh random nonce, hex. 16 bytes: far beyond birthday-collision concern for
// a per-connection value, and short enough to stay inside the handshake limit.
QString makeNonce();

// The shared computation, used by BOTH ends. One implementation, because two
// would eventually disagree about the separator and fail only in the field.
QString computeMac(const QByteArray& rangeKey,
                   const QString& rmsNonce,
                   const QString& nodeNonce,
                   const QString& nodeId,
                   const QString& rmsInstanceId);

// Constant-time comparison. A length-or-content early exit leaks how much of a
// guess was right, which over many attempts is how a MAC gets brute-forced one
// byte at a time.
bool macEquals(const QString& a, const QString& b);

// Loads the range key, creating one if absent. 32 random bytes, hex.
//
// NOT derived from a range name, a serial or anything else guessable: a key
// anyone can reconstruct is not a key. Returns empty on failure, and an empty
// key must be treated as "cannot authenticate", never as "no authentication
// required".
QByteArray loadOrCreateRangeKey(const QString& path, QString* errorOut = nullptr);

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_CONTROLAUTH_H
