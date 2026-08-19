#ifndef TA_RMS_STATIONCODE_H
#define TA_RMS_STATIONCODE_H

// ─────────────────────────────────────────────────────────────────────────────
// STATION CODE — a short, readable name for a station, for HUMANS ONLY.
//
// ═══ IT IS NOT AN IDENTITY ═════════════════════════════════════════════════
//
// The authoritative device identity is the `nodeId`, and it stays that way.
// Nothing is persisted under a station code, nothing is looked up by one, and
// no lane mapping ever references one. It exists so an operator standing at a
// range can say "E222-403F" instead of reading out
// "TA-NODE-E368E222403F", and for no other reason.
//
// A second identity is exactly the kind of thing that quietly becomes load
// bearing, so the type is deliberately a plain function over a string rather
// than an object anyone could store.
//
// ═══ DETERMINISTIC ═════════════════════════════════════════════════════════
//
// The same nodeId always produces the same code, in this run and in every
// future run. It is a slice of the id, not a hash and not a counter: an
// operator who wrote "E222-403F" on masking tape last week must find the same
// code on screen today.
//
// ═══ COLLISIONS ═══════════════════════════════════════════════════════════
//
// Two different nodeIds CAN share a short code. An ambiguous label on a range
// is worse than a long one, so `codesFor()` takes the whole set of stations at
// once: if any two collide at the short length, EVERY code in that set grows
// until they are all distinct, and the display stays consistent between lanes.
// If they still collide at full length they are the same node, which cannot
// happen by construction.
// ─────────────────────────────────────────────────────────────────────────────

#include <QHash>
#include <QString>
#include <QStringList>

namespace ta {
namespace rms {

class StationCode
{
public:
    // The default short form: two groups of four hex, e.g. "E222-403F".
    static QString shortCode(const QString& nodeId);

    // The same, at a requested number of hex characters (rounded up to a
    // multiple of four so the grouping stays even). Used by the collision
    // resolver; callers normally want codesFor().
    static QString codeOfLength(const QString& nodeId, int hexChars);

    // THE ONE TO USE when more than one station is on screen. Returns a map of
    // nodeId -> code in which every code is unique, expanding all of them
    // together if any two would collide.
    static QHash<QString, QString> codesFor(const QStringList& nodeIds);

    // Whether the given set would collide at the default short length. Exposed
    // so diagnostics can say so rather than leaving an operator to notice.
    static bool wouldCollide(const QStringList& nodeIds);

    static constexpr int kDefaultHexChars = 8;

private:
    // The hex payload of a nodeId, without the product prefix. Anything that
    // is not a "TA-NODE-…" shaped string is returned as-is so a station from a
    // future naming scheme still gets a stable, if uglier, code.
    static QString payloadOf(const QString& nodeId);
};

}  // namespace rms
}  // namespace ta

#endif
