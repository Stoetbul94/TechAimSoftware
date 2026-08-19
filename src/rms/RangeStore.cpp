#include "RangeStore.h"
#include "RmsJsonStore.h"

#include <QJsonObject>

namespace ta {
namespace rms {

namespace {

// One implementation of "read a versioned RMS document safely" now lives in
// RmsJsonStore; this maps its result onto the range-specific vocabulary the
// configuration service and its tests already speak.
RangeStoreError mapError(StoreError e)
{
    switch (e) {
    case StoreError::NotFound:     return RangeStoreError::NotFound;
    case StoreError::Unreadable:   return RangeStoreError::Unreadable;
    case StoreError::Malformed:    return RangeStoreError::Malformed;
    case StoreError::SchemaTooNew: return RangeStoreError::SchemaTooNew;
    case StoreError::WriteFailed:  return RangeStoreError::WriteFailed;
    case StoreError::WriteBlocked: return RangeStoreError::WriteBlocked;
    case StoreError::None:         break;
    }
    return RangeStoreError::None;
}

} // namespace

RangeStoreResult RangeStoreResult::failure(RangeStoreError e, const QString& detail)
{
    RangeStoreResult r;
    r.ok = false;
    r.error = e;
    r.detail = detail;
    return r;
}

QString RangeStore::defaultPath()
{
    return rmsDataFile(QStringLiteral("range.json"));
}

bool RangeStore::exists() const
{
    RmsJsonStore s(path());
    return s.exists();
}

RangeStoreResult RangeStore::load(RangeDefinition* out)
{
    RmsJsonStore s(path());
    QJsonObject doc;
    const StoreResult r = s.load(kRangeSchemaVersion, &doc);
    if (!r.ok) {
        if (r.error == StoreError::SchemaTooNew)
            m_blocked = true;
        return RangeStoreResult::failure(mapError(r.error), r.detail);
    }

    const RangeDefinition range = RangeDefinition::fromJson(doc);
    if (!range.isValid())
        return RangeStoreResult::failure(RangeStoreError::Malformed,
                                         QStringLiteral("no rangeId or no lanes"));
    if (out)
        *out = range;
    return RangeStoreResult::success();
}

RangeStoreResult RangeStore::save(const RangeDefinition& range)
{
    if (m_blocked)
        return RangeStoreResult::failure(
            RangeStoreError::WriteBlocked,
            QStringLiteral("refusing to overwrite a range file written by a newer RMS"));

    RmsJsonStore s(path());
    const StoreResult r = s.save(kRangeSchemaVersion, range.toJson());
    if (!r.ok)
        return RangeStoreResult::failure(mapError(r.error), r.detail);
    return RangeStoreResult::success();
}

} // namespace rms
} // namespace ta
