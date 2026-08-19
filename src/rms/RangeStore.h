#ifndef TA_RMS_RANGESTORE_H
#define TA_RMS_RANGESTORE_H

// ─────────────────────────────────────────────────────────────────────────────
// Where the range configuration lives on disk.
//
// IN RMS'S OWN NAMESPACE, never the target application's. RMS is a separate
// product that may not even run on a machine that has the node application
// installed, and a range definition written into the node's AppData would be
// lost the moment the two were separated.
//
// WRITES ARE ATOMIC. Write a temporary file, flush it, then rename over the
// real one. A half-written range file is worse than no range file: the
// operator would be shown a range that does not exist, and the alternative to
// this is losing a ten-lane configuration to a power cut mid-save.
//
// A DOCUMENT FROM A NEWER RMS IS REFUSED, NOT GUESSED AT — and refusing also
// BLOCKS saving. Otherwise the older build would show first-run setup, the
// operator would rebuild the range, and the newer version's configuration
// would be silently overwritten. Reporting "this file was written by a newer
// version" and stopping is the only safe answer.
// ─────────────────────────────────────────────────────────────────────────────

#include "RangeDefinition.h"

#include <QString>

namespace ta {
namespace rms {

enum class RangeStoreError {
    None,
    NotFound,          // no range configured yet — this is first run, not a fault
    Unreadable,
    Malformed,
    SchemaTooNew,      // written by a newer RMS; refuse and protect it
    WriteFailed,
    WriteBlocked       // refused because the on-disk document is newer
};

struct RangeStoreResult {
    bool ok = true;
    RangeStoreError error = RangeStoreError::None;
    QString detail;

    static RangeStoreResult success() { return RangeStoreResult(); }
    static RangeStoreResult failure(RangeStoreError e, const QString& detail);
};

class RangeStore
{
public:
    RangeStore() = default;

    // Production: <AppLocalDataLocation>/range.json, resolved from the
    // application identity RMS's main() sets.
    static QString defaultPath();

    // Tests and tooling point this at a scratch file. Must be set before use.
    void setPath(const QString& path) { m_path = path; m_blocked = false; }
    QString path() const { return m_path.isEmpty() ? defaultPath() : m_path; }

    bool exists() const;
    RangeStoreResult load(RangeDefinition* out);
    RangeStoreResult save(const RangeDefinition& range);

    // True once a newer document has been seen; saving stays refused until the
    // path changes, so an old build cannot clobber a new one's configuration.
    bool isWriteBlocked() const { return m_blocked; }

private:
    QString m_path;
    bool    m_blocked = false;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGESTORE_H
