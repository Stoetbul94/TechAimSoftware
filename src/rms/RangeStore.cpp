#include "RangeStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace ta {
namespace rms {

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
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty())
        root = QDir::homePath() + QStringLiteral("/.techaim-rms");
    return QDir(root).filePath(QStringLiteral("range.json"));
}

bool RangeStore::exists() const
{
    return QFileInfo::exists(path());
}

RangeStoreResult RangeStore::load(RangeDefinition* out)
{
    const QString p = path();
    QFile f(p);
    if (!f.exists())
        return RangeStoreResult::failure(RangeStoreError::NotFound, p);
    if (!f.open(QIODevice::ReadOnly))
        return RangeStoreResult::failure(RangeStoreError::Unreadable, f.errorString());

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return RangeStoreResult::failure(RangeStoreError::Malformed, err.errorString());

    const QJsonObject o = doc.object();
    const int version = o.value(QStringLiteral("schemaVersion")).toInt(0);
    if (version > kRangeSchemaVersion) {
        m_blocked = true;
        return RangeStoreResult::failure(
            RangeStoreError::SchemaTooNew,
            QStringLiteral("range.json is schema v%1; this build reads v%2")
                .arg(version).arg(kRangeSchemaVersion));
    }
    // An older or unversioned document is read with the fields this build
    // knows. Unknown keys are ignored — forward compatible in the same way the
    // wire protocol is.

    const RangeDefinition r = RangeDefinition::fromJson(o);
    if (!r.isValid())
        return RangeStoreResult::failure(RangeStoreError::Malformed,
                                         QStringLiteral("no rangeId or no lanes"));
    if (out)
        *out = r;
    return RangeStoreResult::success();
}

RangeStoreResult RangeStore::save(const RangeDefinition& range)
{
    if (m_blocked)
        return RangeStoreResult::failure(
            RangeStoreError::WriteBlocked,
            QStringLiteral("refusing to overwrite a range file written by a newer RMS"));

    const QString p = path();
    QDir().mkpath(QFileInfo(p).absolutePath());

    // QSaveFile is write-temp-then-rename: either the previous configuration
    // survives intact or the new one lands whole. There is no in-between file
    // for the next launch to read.
    QSaveFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return RangeStoreResult::failure(RangeStoreError::WriteFailed, f.errorString());
    const QByteArray bytes = QJsonDocument(range.toJson()).toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size()) {
        f.cancelWriting();
        return RangeStoreResult::failure(RangeStoreError::WriteFailed,
                                         QStringLiteral("short write"));
    }
    if (!f.commit())
        return RangeStoreResult::failure(RangeStoreError::WriteFailed, f.errorString());
    return RangeStoreResult::success();
}

} // namespace rms
} // namespace ta
