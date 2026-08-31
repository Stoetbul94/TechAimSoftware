#include "app/SupportBundle.h"

#include "app/ProductIdentity.h"
#include "platform/PlatformService.h"
#include "reliability/storage/StoragePaths.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSysInfo>
#include <QTextStream>

namespace ta {

SupportBundle::SupportBundle(QObject* parent)
    : QObject(parent)
{
}

QString SupportBundle::identityReport()
{
    const app::ProductIdentity& p = app::identity();
    QString s;
    QTextStream out(&s);
    out << p.fullProductName << " support bundle\n";
    out << "Generated       : "
        << QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) << " UTC\n";
    out << "Product         : " << p.fullProductName << "\n";
    out << "Version         : " << p.version << "\n";
    out << "Release channel : " << p.releaseChannel << "\n";
#ifdef APP_GIT_SHA
    out << "Commit          : " << QLatin1String(APP_GIT_SHA) << "\n";
#else
    out << "Commit          : (not compiled in)\n";
#endif
    out << "Limitation      : " << p.fieldTestNotice << "\n";
    out << "Shell           : "
        << platform::shellName(platform::currentShell()) << "\n";
    // Device and OS. On a tablet these are the first questions support asks
    // and the last thing a tester can reliably recite from memory.
    out << "Device          : " << QSysInfo::prettyProductName() << "\n";
    out << "Kernel          : " << QSysInfo::kernelType() << " "
        << QSysInfo::kernelVersion() << "\n";
    out << "Architecture    : " << QSysInfo::currentCpuArchitecture() << "\n";
    out << "Machine         : " << QSysInfo::machineHostName() << "\n";
    out << "Data root       : " << rel::StoragePaths::applicationDataRoot() << "\n";
    out << "\n";
    // Stated, not implied: a bundle taken from a build with no transport must
    // not read as though acquisition was working and simply quiet.
    out << "TARGET TRANSPORT: see docs/android/ANDROID-TARGET-TRANSPORT-DECISION.md\n";
    out << "  Android USB target acquisition is NOT IMPLEMENTED in this build.\n";
    out << "  An absence of shot data is expected and is not a fault report.\n";
    return s;
}

int SupportBundle::copyTree(const QString& srcDir, const QString& destDir,
                            const QString& prefix)
{
    QDir src(srcDir);
    if (!src.exists())
        return 0;
    int copied = 0;
    const QFileInfoList entries =
        src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : entries) {
        if (fi.isDir()) {
            copied += copyTree(fi.absoluteFilePath(), destDir,
                               prefix + fi.fileName() + QLatin1Char('-'));
        } else {
            // Flattened with a prefix: two journals of the same name from
            // Sessions/Current and Sessions/Archive must both survive.
            const QString target =
                QDir(destDir).filePath(prefix + fi.fileName());
            if (QFile::copy(fi.absoluteFilePath(), target))
                ++copied;
        }
    }
    return copied;
}

SupportBundleResult SupportBundle::create(const QString& stamp)
{
    SupportBundleResult r;
    const QString root = rel::StoragePaths::supportBundlesDirectory();
    const QString dir  = QDir(root).filePath(QStringLiteral("Support-") + stamp);

    if (!QDir().mkpath(dir)) {
        r.failureReason = QStringLiteral("could not create %1").arg(dir);
        qWarning().noquote() << "SupportBundle:" << r.failureReason;
        return r;
    }

    // 1. identity, always written first so an otherwise empty bundle still
    //    says which build produced it.
    QFile id(QDir(dir).filePath(QStringLiteral("support-identity.txt")));
    if (id.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&id) << identityReport();
        id.close();
        r.collected << QStringLiteral("build identity: written");
    } else {
        r.failureReason = QStringLiteral("could not write the identity file");
        qWarning().noquote() << "SupportBundle:" << r.failureReason;
        return r;
    }

    // 2. the evidence classes, each reported with a COUNT so "none found" is
    //    a stated result rather than a silent absence.
    struct Src { const char* label; QString path; QString prefix; };
    const QList<Src> sources = {
        { "session journals (current)",  rel::StoragePaths::currentSessionsDirectory(),   QStringLiteral("Current-") },
        { "session journals (archive)",  rel::StoragePaths::archivedSessionsDirectory(),  QStringLiteral("Archive-") },
        { "session journals (corrupt)",  rel::StoragePaths::corruptedSessionsDirectory(), QStringLiteral("Corrupt-") },
        { "saved match records (.tch)",  rel::StoragePaths::matchRecordsDirectory(),      QStringLiteral("Match-") },
        { "application logs",            rel::StoragePaths::logsDirectory(),              QStringLiteral("Log-") },
        { "configuration",               rel::StoragePaths::settingsDirectory(),          QStringLiteral("Settings-") },
    };
    for (const Src& s : sources) {
        const int n = copyTree(s.path, dir, s.prefix);
        r.collected << QStringLiteral("%1: %2").arg(QLatin1String(s.label)).arg(n);
    }

    // 3. what was collected, in the bundle, so the reader does not have to
    //    infer it from the file list.
    QFile what(QDir(dir).filePath(QStringLiteral("WHAT-WAS-COLLECTED.txt")));
    if (what.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ws(&what);
        for (const QString& line : r.collected)
            ws << line << "\n";
        ws << "\nsearched under: " << rel::StoragePaths::applicationDataRoot() << "\n";
        what.close();
    }

    r.ok   = true;
    r.path = dir;
    return r;
}

QString SupportBundle::createNow()
{
    const QString stamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd-HHmmss"));
    const SupportBundleResult r = create(stamp);
    if (!r.ok) {
        qWarning().noquote() << "SupportBundle FAILED:" << r.failureReason;
        return QString();
    }
    qInfo().noquote() << "Support bundle written to" << r.path;
    for (const QString& line : r.collected)
        qInfo().noquote() << "  " << line;
    return r.path;
}

} // namespace ta
