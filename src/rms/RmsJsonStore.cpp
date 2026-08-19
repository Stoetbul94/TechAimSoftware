#include "RmsJsonStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace ta {
namespace rms {

StoreResult StoreResult::failure(StoreError e, const QString& detail)
{
    StoreResult r;
    r.ok = false;
    r.error = e;
    r.detail = detail;
    return r;
}

QString rmsDataDir()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty())
        root = QDir::homePath() + QStringLiteral("/.techaim-rms");
    return root;
}

QString rmsDataFile(const QString& fileName)
{
    return QDir(rmsDataDir()).filePath(fileName);
}

bool RmsJsonStore::exists() const
{
    return !m_path.isEmpty() && QFileInfo::exists(m_path);
}

StoreResult RmsJsonStore::load(int expectedVersion, QJsonObject* out)
{
    QFile f(m_path);
    if (!f.exists())
        return StoreResult::failure(StoreError::NotFound, m_path);
    if (!f.open(QIODevice::ReadOnly))
        return StoreResult::failure(StoreError::Unreadable, f.errorString());

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return StoreResult::failure(StoreError::Malformed, err.errorString());

    const QJsonObject o = doc.object();
    const int version = o.value(QStringLiteral("schemaVersion")).toInt(0);
    if (version > expectedVersion) {
        m_blocked = true;
        return StoreResult::failure(
            StoreError::SchemaTooNew,
            QStringLiteral("%1 is schema v%2; this build reads v%3")
                .arg(QFileInfo(m_path).fileName()).arg(version).arg(expectedVersion));
    }
    if (out)
        *out = o;
    return StoreResult::success();
}

StoreResult RmsJsonStore::save(int version, const QJsonObject& document)
{
    if (m_blocked)
        return StoreResult::failure(
            StoreError::WriteBlocked,
            QStringLiteral("refusing to overwrite %1 — it was written by a newer RMS")
                .arg(QFileInfo(m_path).fileName()));

    QDir().mkpath(QFileInfo(m_path).absolutePath());

    QJsonObject stamped = document;
    stamped[QStringLiteral("schemaVersion")] = version;

    // Write-temp-then-rename. There is no in-between file for the next launch
    // to read.
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return StoreResult::failure(StoreError::WriteFailed, f.errorString());
    const QByteArray bytes = QJsonDocument(stamped).toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size()) {
        f.cancelWriting();
        return StoreResult::failure(StoreError::WriteFailed, QStringLiteral("short write"));
    }
    if (!f.commit())
        return StoreResult::failure(StoreError::WriteFailed, f.errorString());
    return StoreResult::success();
}

} // namespace rms
} // namespace ta
