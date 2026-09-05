#include "rms/control/ControlAuth.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>

namespace ta {
namespace rms {
namespace control {

QString makeNonce()
{
    QByteArray raw(16, Qt::Uninitialized);
    QRandomGenerator::system()->generate(raw.begin(), raw.end());
    return QString::fromLatin1(raw.toHex());
}

QString computeMac(const QByteArray& rangeKey,
                   const QString& rmsNonce,
                   const QString& nodeNonce,
                   const QString& nodeId,
                   const QString& rmsInstanceId)
{
    // A separator that cannot appear in any of the fields. Without one,
    // ("ab","c") and ("a","bc") would produce the same MAC, and an attacker who
    // controls one field could shift bytes between them.
    const QByteArray sep("\x1f", 1);

    QByteArray msg;
    msg += rmsNonce.toUtf8();       msg += sep;
    msg += nodeNonce.toUtf8();      msg += sep;
    msg += nodeId.toUtf8();         msg += sep;
    msg += rmsInstanceId.toUtf8();

    QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
    mac.setKey(rangeKey);
    mac.addData(msg);
    return QString::fromLatin1(mac.result().toHex());
}

bool macEquals(const QString& a, const QString& b)
{
    const QByteArray x = a.toLatin1();
    const QByteArray y = b.toLatin1();
    // Compare a fixed number of bytes with no early exit. Differing lengths
    // still fail, but they fail in the same time as a wrong byte.
    const int n = qMax(x.size(), y.size());
    if (n == 0)
        return false;                      // two empty MACs are not a match
    quint8 diff = quint8(x.size() ^ y.size());
    for (int i = 0; i < n; ++i) {
        const quint8 xa = i < x.size() ? quint8(x[i]) : 0;
        const quint8 yb = i < y.size() ? quint8(y[i]) : 0;
        diff |= quint8(xa ^ yb);
    }
    return diff == 0;
}

QByteArray loadRangeKey(const QString& path, QString* errorOut)
{
    const auto fail = [errorOut](const QString& why) {
        if (errorOut) *errorOut = why;
        return QByteArray();
    };
    if (!QFileInfo::exists(path))
        return fail(QStringLiteral("range key not configured"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("range key exists but cannot be read"));
    const QByteArray key = QByteArray::fromHex(f.readAll().trimmed());
    // Short or corrupt is a FAILURE, never a fallback to no authentication.
    if (key.size() < 32)
        return fail(QStringLiteral("range key is too short to use"));
    if (errorOut) errorOut->clear();
    return key;
}

QByteArray loadOrCreateRangeKey(const QString& path, QString* errorOut)
{
    const auto fail = [errorOut](const QString& why) {
        if (errorOut) *errorOut = why;
        return QByteArray();
    };

    QFileInfo fi(path);
    if (fi.exists()) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return fail(QStringLiteral("range key exists but cannot be read"));
        const QByteArray hex = f.readAll().trimmed();
        const QByteArray key = QByteArray::fromHex(hex);
        // A short or corrupt key is a FAILURE, never a fallback to "no
        // authentication". Degrading to open control on a bad key file would
        // turn a configuration mistake into an unprotected range.
        if (key.size() < 32)
            return fail(QStringLiteral("range key is too short to use"));
        if (errorOut) errorOut->clear();
        return key;
    }

    if (!QDir().mkpath(fi.absolutePath()))
        return fail(QStringLiteral("cannot create the key directory"));

    QByteArray key(32, Qt::Uninitialized);
    QRandomGenerator::system()->generate(key.begin(), key.end());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return fail(QStringLiteral("cannot write a new range key"));
    f.write(key.toHex());
    f.write("\n");
    f.close();

    if (errorOut) errorOut->clear();
    return key;
}

} // namespace control
} // namespace rms
} // namespace ta
