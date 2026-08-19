#ifndef TA_RMS_RMSJSONSTORE_H
#define TA_RMS_RMSJSONSTORE_H

// ─────────────────────────────────────────────────────────────────────────────
// How every RMS document is written to disk. One implementation, because three
// copies of "read a versioned JSON file safely" is three chances to get the
// dangerous parts subtly different.
//
// THE DANGEROUS PARTS
//   Atomic writes. Write a temporary file, then rename over the real one, so
//   either the previous document survives intact or the new one lands whole.
//   A half-written range or start list is worse than none: the operator would
//   be shown something that never existed.
//
//   A document from a NEWER RMS is refused, AND refusing blocks saving. An
//   older build that quietly showed "nothing configured" would invite the
//   operator to rebuild, and the newer version's work would be overwritten by
//   a build that could not even read it.
//
//   Unknown fields are ignored — forward compatible in the same way the wire
//   protocol is.
//
// ALL OF THIS WRITES RMS'S OWN DATA. Nothing here can reach a target node.
// ─────────────────────────────────────────────────────────────────────────────

#include <QJsonObject>
#include <QString>

namespace ta {
namespace rms {

enum class StoreError {
    None,
    NotFound,      // nothing saved yet — first run, not a fault
    Unreadable,
    Malformed,
    SchemaTooNew,  // written by a newer RMS; refuse and protect it
    WriteFailed,
    WriteBlocked   // refused because the on-disk document is newer
};

struct StoreResult {
    bool ok = true;
    StoreError error = StoreError::None;
    QString detail;

    static StoreResult success() { return StoreResult(); }
    static StoreResult failure(StoreError e, const QString& detail);
};

// The directory every RMS document lives in: RMS's OWN application data
// location, never the target application's.
QString rmsDataDir();
QString rmsDataFile(const QString& fileName);

class RmsJsonStore
{
public:
    RmsJsonStore() = default;
    explicit RmsJsonStore(const QString& path) : m_path(path) {}

    void setPath(const QString& path) { m_path = path; m_blocked = false; }
    QString path() const { return m_path; }
    bool exists() const;
    bool isWriteBlocked() const { return m_blocked; }

    // `expectedVersion` is the highest schemaVersion this build understands.
    StoreResult load(int expectedVersion, QJsonObject* out);
    // Stamps schemaVersion itself, so no caller can forget to.
    StoreResult save(int version, const QJsonObject& document);

private:
    QString m_path;
    bool m_blocked = false;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_RMSJSONSTORE_H
