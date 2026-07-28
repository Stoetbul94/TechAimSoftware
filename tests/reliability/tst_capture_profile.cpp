// J.1A — documentation capture profile isolation.
//
// Every test uses temporary fixture roots. NOTHING here touches the real
// application data root; the production root is INJECTED as a fake path so
// the rejection rules can be exercised without depending on the machine.
#include "test_support.h"
#include "app/DocumentationCapture.h"
#include "reliability/storage/StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTemporaryDir>

using namespace ta::app;

namespace {

QString touch(const QDir& d, const QString& name)
{
    const QString p = d.filePath(name);
    QFile f(p);
    f.open(QIODevice::WriteOnly);
    f.write("x");
    f.close();
    return p;
}

} // namespace

void run_capture_profile_tests()
{
    std::printf("--- J.1A documentation capture profile ---\n");

    QTemporaryDir tmp;
    check(tmp.isValid(), "capture: temporary fixture root created");
    const QDir base(tmp.path());
    const QString fakeProduction = base.filePath("FakeProduction/TechAim/TechAim");
    const QString fakeInstall    = base.filePath("FakeInstall");
    QDir().mkpath(fakeProduction);
    QDir().mkpath(fakeInstall);

    // ── argument parsing ────────────────────────────────────────────────
    {
        const CaptureRequest none = parseCaptureArguments(
            QStringList() << "TechAim.exe");
        check(!none.requested && none.dataRoot.isEmpty(),
              "capture: absent flags do not request capture");

        const CaptureRequest both = parseCaptureArguments(
            QStringList() << "TechAim.exe" << "--documentation-capture"
                          << "--data-root" << "C:/caps/one");
        check(both.requested && both.dataRoot == QLatin1String("C:/caps/one"),
              "capture: both flags parsed");

        const CaptureRequest eq = parseCaptureArguments(
            QStringList() << "--documentation-capture" << "--data-root=C:/caps/two");
        check(eq.requested && eq.dataRoot == QLatin1String("C:/caps/two"),
              "capture: --data-root=VALUE form parsed");

        const CaptureRequest rootOnly = parseCaptureArguments(
            QStringList() << "--data-root" << "C:/caps/three");
        check(!rootOnly.requested,
              "capture: --data-root alone does NOT activate capture mode");
    }

    // ── rejection rules ─────────────────────────────────────────────────
    {
        check(!validateCaptureRoot("", fakeProduction, fakeInstall).ok,
              "capture: empty path rejected");
        check(!validateCaptureRoot("relative/path", fakeProduction, fakeInstall).ok,
              "capture: relative path rejected");
        check(!validateCaptureRoot(fakeProduction, fakeProduction, fakeInstall).ok,
              "capture: production data root rejected");
        check(!validateCaptureRoot(fakeProduction + "/Sessions",
                                   fakeProduction, fakeInstall).ok,
              "capture: path INSIDE the production root rejected");
        check(!validateCaptureRoot(fakeInstall, fakeProduction, fakeInstall).ok,
              "capture: installation directory rejected");
        check(!validateCaptureRoot(fakeInstall + "/sub",
                                   fakeProduction, fakeInstall).ok,
              "capture: path inside the installation directory rejected");
    }

    // ── an existing personal folder is not silently adopted ─────────────
    {
        const QString personal = base.filePath("SomePersonalFolder");
        QDir().mkpath(personal);
        touch(QDir(personal), "my_notes.txt");
        const CaptureResult r =
            validateCaptureRoot(personal, fakeProduction, fakeInstall);
        check(!r.ok, "capture: non-empty unmarked directory refused");
        check(!hasCaptureMarker(personal),
              "capture: no marker was written to the refused directory");
        check(QDir(personal).entryList(QDir::Files | QDir::Hidden).size() == 1,
              "capture: refused directory left untouched");
    }

    // ── happy path: prepare, marker, idempotence ────────────────────────
    {
        const QString good = base.filePath("CaptureProfile");
        CaptureResult r = prepareCaptureRoot(good, fakeProduction, fakeInstall,
                                             "deadbee", "deadbee");
        check(r.ok, "capture: valid root prepared");
        check(QDir(good).exists(), "capture: root created");
        check(hasCaptureMarker(good), "capture: marker written");

        // Second run must succeed and not duplicate anything.
        const int before = QDir(good).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).size();
        CaptureResult again = prepareCaptureRoot(good, fakeProduction, fakeInstall,
                                                 "deadbee", "deadbee");
        const int after = QDir(good).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).size();
        check(again.ok, "capture: second run succeeds (idempotent)");
        check(before == after, "capture: second run adds no duplicate entries");

        // A marked directory is re-adoptable even though it is now non-empty.
        check(validateCaptureRoot(good, fakeProduction, fakeInstall).ok,
              "capture: marked profile re-validates when non-empty");

        // The marker carries provenance and no personal data.
        QFile mf(good + "/" + QString::fromLatin1(kCaptureMarkerFileName));
        check(mf.open(QIODevice::ReadOnly), "capture: marker readable");
        const QByteArray blob = mf.readAll();
        mf.close();
        check(blob.contains("techaim-documentation-capture"),
              "capture: marker identifies the profile");
        check(blob.contains("deadbee"), "capture: marker records the executable SHA");
        check(!blob.contains("password") && !blob.contains("athlete"),
              "capture: marker carries no credential or athlete data");
    }

    // ── failure creates nothing ─────────────────────────────────────────
    {
        const QString shouldNotExist = base.filePath("NeverCreated");
        // Rejected because it is inside the production root path we pass in.
        const CaptureResult r = prepareCaptureRoot(
            fakeProduction + "/Nested", fakeProduction, fakeInstall, "x", "x");
        check(!r.ok, "capture: invalid request refused by prepare");
        check(!QDir(fakeProduction + "/Nested").exists(),
              "capture: nothing created for a refused request");
        check(!QDir(shouldNotExist).exists(),
              "capture: unrelated paths untouched");
    }

    // ── the override actually redirects EVERY storage location ──────────
    {
        const QString iso = base.filePath("RoutingProfile");
        QDir().mkpath(iso);
        const QString productionBefore = ta::rel::StoragePaths::productionDataRoot();

        ta::rel::StoragePaths::setRootOverrideForTesting(iso);
        check(ta::rel::StoragePaths::applicationDataRoot() == iso,
              "capture: data root follows the override");
        check(ta::rel::StoragePaths::currentSessionsDirectory().startsWith(iso),
              "capture: current sessions inside the isolated root");
        check(ta::rel::StoragePaths::archivedSessionsDirectory().startsWith(iso),
              "capture: archived sessions inside the isolated root");
        check(ta::rel::StoragePaths::corruptedSessionsDirectory().startsWith(iso),
              "capture: corrupt sessions inside the isolated root");
        check(ta::rel::StoragePaths::reportsDirectory().startsWith(iso),
              "capture: reports inside the isolated root");
        check(ta::rel::StoragePaths::logsDirectory().startsWith(iso),
              "capture: logs inside the isolated root");
        check(ta::rel::StoragePaths::backupsDirectory().startsWith(iso),
              "capture: backups inside the isolated root");
        check(ta::rel::StoragePaths::exportsDirectory().startsWith(iso),
              "capture: exports inside the isolated root");

        // productionDataRoot must keep reporting the REAL location, so the
        // validator can always refuse it.
        check(ta::rel::StoragePaths::productionDataRoot() == productionBefore,
              "capture: productionDataRoot ignores the override");
        check(!ta::rel::StoragePaths::productionDataRoot().startsWith(iso),
              "capture: production root is not the isolated root");

        // Clearing the override restores normal behaviour.
        ta::rel::StoragePaths::setRootOverrideForTesting(QString());
        check(ta::rel::StoragePaths::applicationDataRoot() == productionBefore,
              "capture: clearing the override restores the production root");
    }
}
