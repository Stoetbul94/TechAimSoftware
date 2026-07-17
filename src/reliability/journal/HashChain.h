#ifndef TA_REL_HASHCHAIN_H
#define TA_REL_HASHCHAIN_H

// Session Reliability Layer — chained SHA-256 integrity (M1, spec §5 + D8).
//
// Representation: full SHA-256 computed internally, stored TRUNCATED to
// 128 bits = 32 lowercase hex characters. Truncation is explicitly
// mandated by the implementation specification (decision D8: "SHA-256
// truncated 128-bit, chained … 32-hex h/ph fields") — documented here so
// the choice is auditable.
//
// Chain procedure (every line, uniform):
//   input  = ph_ascii_bytes + core_bytes
//   h      = lowercase_hex(SHA256(input))[0..31]
// where core_bytes is the deterministic serialization of the envelope
// WITHOUT `h` (it already contains `ph`), and ph_ascii_bytes is the same
// 32-hex previous hash that appears inside the core.
//
// Genesis (defined precisely): the first line (seq 0) uses
// ph = 32 x '0'. Its hash input is therefore the genesis material
// "000...0" + core_bytes — session-derived because the core carries the
// session UUID. No other special-casing exists.

#include <QByteArray>

namespace ta {
namespace rel {

namespace HashChain {

// 32 ASCII '0' characters — the seq-0 previous hash.
QByteArray genesisPreviousHash();

// hex(SHA256(previousHashHex + coreBytes)) truncated to 32 chars.
QByteArray computeLineHash(const QByteArray& previousHashHex,
                           const QByteArray& coreBytes);

} // namespace HashChain

} // namespace rel
} // namespace ta

#endif // TA_REL_HASHCHAIN_H
