// Android support evidence collection (SUPPORT-AND-001).
//
// The desktop answer is a batch file, which does not exist on a tablet. These
// tests assert the collection half actually collects - and, just as important,
// that it REPORTS what it found rather than producing a plausible empty
// directory. A support bundle that silently contains nothing is worse than no
// bundle, because it ends the investigation instead of starting it.

#include "test_support.h"

#include "app/SupportBundle.h"
#include "reliability/storage/StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstdio>

using ta::SupportBundle;
using ta::SupportBundleResult;
using ta::rel::StoragePaths;

static void writeFile(const QString& path, const QString& body)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << body;
        f.close();
    }
}

void run_support_bundle_tests()
{
    printf("\n--- Android support export (SUPPORT-AND-001) ---\n");
    fflush(stdout);

    QTemporaryDir tmp;
    check(tmp.isValid(), "SUPPORT-AND-001: isolated data root created");
    if (!tmp.isValid())
        return;

    // Everything below runs against an isolated root: the test must never
    // read or write the developer's real session store.
    const QString saved = StoragePaths::applicationDataRoot();
    StoragePaths::setRootOverrideForTesting(tmp.path());

    // Representative evidence of each class the bundle promises to collect.
    writeFile(QDir(StoragePaths::currentSessionsDirectory()).filePath("session_a.jsonl"),
              QStringLiteral("{\"type\":\"SessionStarted\"}\n"));
    writeFile(QDir(StoragePaths::archivedSessionsDirectory()).filePath("2026/08/session_a.jsonl"),
              QStringLiteral("{\"type\":\"SessionStarted\"}\n"));
    writeFile(QDir(StoragePaths::corruptedSessionsDirectory()).filePath("session_bad.jsonl"),
              QStringLiteral("{\n"));
    writeFile(QDir(StoragePaths::matchRecordsDirectory()).filePath("Match_test.tch"),
              QStringLiteral("<root/>\n"));
    writeFile(QDir(StoragePaths::logsDirectory()).filePath("tachus_log.log"),
              QStringLiteral("log line\n"));
    writeFile(QDir(StoragePaths::settingsDirectory()).filePath("config.ini"),
              QStringLiteral("[General]\napp_mode=Demo\n"));

    SupportBundle sb;
    const SupportBundleResult r = sb.create(QStringLiteral("2026-08-31-000000"));

    check(r.ok, "SUPPORT-AND-001: the bundle is created", r.failureReason);
    check(!r.path.isEmpty() && QDir(r.path).exists(),
          "SUPPORT-AND-001: and the directory exists", r.path);

    if (r.ok) {
        QDir d(r.path);
        const QStringList files = d.entryList(QDir::Files);

        check(files.contains(QStringLiteral("support-identity.txt")),
              "SUPPORT-AND-001: build identity is always written first, so an "
              "otherwise empty bundle still says which build produced it");
        check(files.contains(QStringLiteral("WHAT-WAS-COLLECTED.txt")),
              "SUPPORT-AND-001: and what was collected is stated in the bundle");

        // The two journals share a name and come from different folders. A
        // flattening copy that did not prefix would have kept only one.
        const int journals =
            files.filter(QStringLiteral(".jsonl")).count();
        check(journals == 3,
              "SUPPORT-AND-001: all three journals survive, including two that "
              "share a filename across Current and Archive",
              QString::number(journals));

        check(files.filter(QStringLiteral(".tch")).count() == 1,
              "SUPPORT-AND-001: the saved match record is collected");
        check(files.filter(QStringLiteral(".log")).count() == 1,
              "SUPPORT-AND-001: the application log is collected");
        check(files.filter(QStringLiteral("config.ini")).count() == 1,
              "SUPPORT-AND-001: the configuration is collected");

        // Counts, not just presence: "none found" must be a stated result.
        QFile w(d.filePath(QStringLiteral("WHAT-WAS-COLLECTED.txt")));
        QString what;
        if (w.open(QIODevice::ReadOnly | QIODevice::Text))
            what = QString::fromUtf8(w.readAll());
        check(what.contains(QStringLiteral("session journals (current): 1")),
              "SUPPORT-AND-001: each evidence class is reported as a COUNT, so "
              "zero is stated rather than left to be inferred", what.left(120));
        check(what.contains(QStringLiteral("searched under:")),
              "SUPPORT-AND-001: and the root it searched is named");

        QFile idf(d.filePath(QStringLiteral("support-identity.txt")));
        QString ident;
        if (idf.open(QIODevice::ReadOnly | QIODevice::Text))
            ident = QString::fromUtf8(idf.readAll());
        check(ident.contains(QStringLiteral("Version"))
              && ident.contains(QStringLiteral("Commit")),
              "SUPPORT-AND-001: the identity names version and commit");
        check(ident.contains(QStringLiteral("NOT IMPLEMENTED")),
              "SUPPORT-AND-001: and states that target acquisition is not "
              "implemented, so an absence of shot data does not read as a fault");
    }

    // An empty store must still yield a usable bundle rather than an error.
    {
        QTemporaryDir empty;
        StoragePaths::setRootOverrideForTesting(empty.path());
        SupportBundle sb2;
        const SupportBundleResult r2 = sb2.create(QStringLiteral("empty"));
        check(r2.ok,
              "SUPPORT-AND-001: an empty store still produces a bundle - the "
              "identity alone is worth returning", r2.failureReason);
    }

    StoragePaths::setRootOverrideForTesting(QString());
    (void)saved;
}
