#ifndef DOCUMENTATIONCAPTURE_H
#define DOCUMENTATIONCAPTURE_H

#include <QString>
#include <QStringList>

// ─────────────────────────────────────────────────────────────────────────
// Documentation capture profile (J.1A).
//
// A DEVELOPER / DOCUMENTATION facility, not an athlete feature. It exists so
// manual screenshots can be taken without touching the real session archive
// or showing a real athlete.
//
//   TechAim.exe --documentation-capture --data-root "<absolute path>"
//
// BOTH flags are required. Absent either one, the application behaves exactly
// as it always has and uses the production data root. There is no Settings
// UI for this, and there is no silent fallback: if the requested root fails
// validation the application reports the reason and EXITS rather than
// quietly writing into production.
//
// The profile is Demo-only. Live is refused, so physical target input can
// never be recorded into a capture profile.
// ─────────────────────────────────────────────────────────────────────────

namespace ta {
namespace app {

// Marker file written into an isolated root so the application can tell a
// capture profile apart from an arbitrary personal folder.
extern const char* const kCaptureMarkerFileName;   // ".techaim-documentation-capture"

struct CaptureRequest {
    bool    requested = false;   // --documentation-capture was given
    QString dataRoot;            // --data-root value, verbatim
};

struct CaptureResult {
    bool    ok = false;
    QString resolvedRoot;        // absolute, cleaned
    QString operatorMessage;     // shown/logged on failure
    QString technicalDetail;
};

// Parses the two flags out of the application arguments. Unknown arguments
// are ignored; this never consumes anything else.
CaptureRequest parseCaptureArguments(const QStringList& args);

// Validates the requested root WITHOUT creating anything.
//
// Rejects, in this order:
//   - an empty path
//   - a relative path
//   - the production data root, or any path inside it
//   - the application installation directory, or any path inside it
//   - an existing NON-EMPTY directory that has no valid capture marker
//
// productionRoot / installDir are injected so this is unit-testable without
// touching the real machine.
CaptureResult validateCaptureRoot(const QString& requestedRoot,
                                  const QString& productionRoot,
                                  const QString& installDir);

// Validates, then creates the directory tree and writes/refreshes the marker.
// Idempotent: running twice against the same root succeeds and does not
// duplicate anything.
CaptureResult prepareCaptureRoot(const QString& requestedRoot,
                                 const QString& productionRoot,
                                 const QString& installDir,
                                 const QString& executableSha,
                                 const QString& applicationBaseline);

// True when the directory carries a marker this build recognises.
bool hasCaptureMarker(const QString& dir);

} // namespace app
} // namespace ta

#endif // DOCUMENTATIONCAPTURE_H
