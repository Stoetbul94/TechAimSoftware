#ifndef TECHAIM_SUPPORTBUNDLE_H
#define TECHAIM_SUPPORTBUNDLE_H

// Android support evidence collection.
//
// WHY THIS EXISTS. On Windows the operator runs Collect-Logs.cmd, which is a
// batch file and therefore does not exist on a tablet. Without an in-app path
// the only route to a tablet's journals is adb - a developer tool an ordinary
// customer cannot be asked to use, and one that will not be available at a
// range. A physical evaluation whose evidence cannot be returned is not an
// evaluation.
//
// WHAT IT COLLECTS. The same classes the desktop bundle collects, resolved
// through the storage layer rather than guessed at: build identity, device and
// OS information, session journals, saved match records, application logs and
// the configuration. Nothing else - no personal files, no credentials.
//
// WHAT IT DOES NOT DO, and this is deliberate rather than an oversight:
//
//   It does not SHARE the bundle off the device. Handing a file to another
//   Android app requires a FileProvider declared in the manifest and a
//   share Intent, which is Java the repository does not yet have. Collection
//   and delivery are separate problems and this solves the first one; the
//   second is recorded as an open gap rather than half-implemented.
//
//   It does not compress. QtCore has no archiver, and inventing one to avoid
//   naming the gap above would be the wrong trade. The bundle is a directory.
//
// The result is a directory the user can be told the path of, that a later
// share step can hand over whole, and that adb can pull today.
//
// QtCore only, so the reliability harness can exercise it on either platform.

#include <QObject>
#include <QString>
#include <QStringList>

namespace ta {

struct SupportBundleResult {
    bool        ok = false;
    QString     path;              // the created bundle directory
    QStringList collected;         // human-readable "what was found" lines
    QString     failureReason;     // set only when ok == false
};

class SupportBundle : public QObject
{
    Q_OBJECT
public:
    explicit SupportBundle(QObject* parent = nullptr);

    // Create a bundle under the application's support-bundle directory.
    // `stamp` names it; callers pass a timestamp. Injectable so the test does
    // not depend on the wall clock.
    SupportBundleResult create(const QString& stamp);

    // QML entry point. Returns the created directory, or an empty string on
    // failure; the reason is logged rather than swallowed.
    Q_INVOKABLE QString createNow();

    // The identity block written into every bundle. Exposed so a test can
    // assert its content without creating files.
    static QString identityReport();

private:
    // Copies every regular file under `srcDir` (recursively) into
    // `destDir`, flattening the tree with a prefix so two files of the same
    // name from different folders cannot overwrite each other. Returns the
    // number copied; a missing source is 0, not an error.
    static int copyTree(const QString& srcDir, const QString& destDir,
                        const QString& prefix);
};

} // namespace ta

#endif // TECHAIM_SUPPORTBUNDLE_H
