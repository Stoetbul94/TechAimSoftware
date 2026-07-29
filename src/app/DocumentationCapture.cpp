#include "DocumentationCapture.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace ta {
namespace app {

const char* const kCaptureMarkerFileName = ".techaim-documentation-capture";

namespace {

const char* const kFlagCapture  = "--documentation-capture";
const char* const kFlagDataRoot = "--data-root";
const int kProfileVersion = 1;

QString cleaned(const QString& p)
{
    if (p.isEmpty()) return QString();
    return QDir::cleanPath(QDir::fromNativeSeparators(p));
}

// True when `child` IS `parent` or lives underneath it. Case-insensitive on
// Windows, where the same directory reaches us with differing case.
bool isSameOrInside(const QString& child, const QString& parent)
{
    if (parent.isEmpty() || child.isEmpty()) return false;
    const QString c = cleaned(child);
    const QString p = cleaned(parent);
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    if (c.compare(p, cs) == 0) return true;
    return c.startsWith(p.endsWith(QLatin1Char('/')) ? p : p + QLatin1Char('/'), cs);
}

CaptureResult fail(const QString& op, const QString& tech, const QString& root = QString())
{
    CaptureResult r;
    r.ok = false;
    r.operatorMessage = op;
    r.technicalDetail = tech;
    r.resolvedRoot = root;
    return r;
}

} // namespace

CaptureRequest parseCaptureArguments(const QStringList& args)
{
    CaptureRequest req;
    for (int i = 0; i < args.size(); ++i) {
        const QString a = args.at(i);
        if (a == QLatin1String(kFlagCapture)) {
            req.requested = true;
        } else if (a == QLatin1String(kFlagDataRoot)) {
            if (i + 1 < args.size()) req.dataRoot = args.at(i + 1);
        } else if (a.startsWith(QLatin1String(kFlagDataRoot) + QLatin1String("="))) {
            req.dataRoot = a.mid(int(qstrlen(kFlagDataRoot)) + 1);
        }
    }
    return req;
}

bool hasCaptureMarker(const QString& dir)
{
    const QString marker = cleaned(dir) + QLatin1Char('/')
                         + QLatin1String(kCaptureMarkerFileName);
    QFile f(marker);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return false;
    const QByteArray blob = f.readAll();
    f.close();
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(blob, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    return doc.object().value(QStringLiteral("profile")).toString()
           == QLatin1String("techaim-documentation-capture");
}

CaptureResult validateCaptureRoot(const QString& requestedRoot,
                                  const QString& productionRoot,
                                  const QString& installDir)
{
    const QString root = cleaned(requestedRoot);

    if (root.isEmpty())
        return fail(QStringLiteral("A documentation capture data root is required."),
                    QStringLiteral("--data-root was empty or absent"));

    if (!QDir::isAbsolutePath(root))
        return fail(QStringLiteral("The documentation capture data root must be an absolute path."),
                    QStringLiteral("relative path rejected: %1").arg(root), root);

    if (isSameOrInside(root, productionRoot))
        return fail(QStringLiteral("The documentation capture data root must not be the "
                                   "normal Tech Aim data location."),
                    QStringLiteral("refused production root: requested=%1 production=%2")
                        .arg(root, cleaned(productionRoot)), root);

    if (isSameOrInside(root, installDir))
        return fail(QStringLiteral("The documentation capture data root must not be inside "
                                   "the application installation directory."),
                    QStringLiteral("refused install dir: requested=%1 install=%2")
                        .arg(root, cleaned(installDir)), root);

    // An EXISTING non-empty directory is only acceptable when it is already a
    // capture profile. This stops an arbitrary personal folder being adopted
    // (and later written into) by mistake.
    QDir d(root);
    if (d.exists()) {
        const QStringList entries =
            d.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
        if (!entries.isEmpty() && !hasCaptureMarker(root))
            return fail(QStringLiteral("The documentation capture data root already contains "
                                       "data and is not a capture profile."),
                        QStringLiteral("non-empty, unmarked directory refused: %1 (%2 entries)")
                            .arg(root).arg(entries.size()), root);
    }

    CaptureResult r;
    r.ok = true;
    r.resolvedRoot = root;
    r.technicalDetail = QStringLiteral("capture root validated: %1").arg(root);
    return r;
}

CaptureResult prepareCaptureRoot(const QString& requestedRoot,
                                 const QString& productionRoot,
                                 const QString& installDir,
                                 const QString& executableSha,
                                 const QString& applicationBaseline)
{
    CaptureResult v = validateCaptureRoot(requestedRoot, productionRoot, installDir);
    if (!v.ok) return v;                       // create NOTHING on failure

    const QString root = v.resolvedRoot;
    if (!QDir().mkpath(root))
        return fail(QStringLiteral("The documentation capture data root could not be created."),
                    QStringLiteral("mkpath failed: %1").arg(root), root);

    QJsonObject o;
    o.insert(QStringLiteral("profile"),
             QStringLiteral("techaim-documentation-capture"));
    o.insert(QStringLiteral("profileVersion"), kProfileVersion);
    o.insert(QStringLiteral("created"),
             QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    o.insert(QStringLiteral("executableSha"), executableSha);
    o.insert(QStringLiteral("applicationBaseline"), applicationBaseline);
    o.insert(QStringLiteral("note"),
             QStringLiteral("Isolated documentation capture profile. Synthetic Demo "
                            "data only. Contains no credentials and no personal data."));

    const QString marker = root + QLatin1Char('/')
                         + QLatin1String(kCaptureMarkerFileName);
    QFile f(marker);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return fail(QStringLiteral("The documentation capture marker could not be written."),
                    QStringLiteral("marker open failed: %1").arg(marker), root);
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();

    v.technicalDetail = QStringLiteral("capture profile ready: %1").arg(root);
    return v;
}

} // namespace app
} // namespace ta
