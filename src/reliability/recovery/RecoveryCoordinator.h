#ifndef TA_REL_RECOVERYCOORDINATOR_H
#define TA_REL_RECOVERYCOORDINATOR_H

// Session Reliability Layer — recovery coordinator (M3, spec §3/§15).
//
// Owns the recovery process end-to-end: it discovers unfinished sessions in
// Sessions/Current, validates each journal, classifies it, and replays the
// valid prefix through the reducer to rebuild state. Controllers never
// perform recovery themselves — they only receive a RecoveredMatchState.
//
// QtCore-only + QObject (for QML-facing invokables). The heavy lifting
// (validate, replay) is delegated to JournalValidator / ReplayEngine so the
// coordinator stays a thin, testable orchestrator.

#include "RecoveryTypes.h"
#include "reliability/storage/StoragePaths.h"

#include <QObject>
#include <QVariantList>
#include <QVector>

namespace ta {
namespace rel {

class RecoveryCoordinator : public QObject {
    Q_OBJECT
public:
    explicit RecoveryCoordinator(QObject* parent = nullptr);

    // Scan Sessions/Current for unfinished sessions. Clean-shutdown sessions
    // (saw SessionClosed) are auto-archived and NOT returned. Empty journals
    // are ignored. The result is newest-first by last-modified time.
    QVector<RecoveryCandidate> scan();

    // QML-facing: the same scan as a QVariantList of maps for the dialog.
    Q_INVOKABLE QVariantList scanForQml();

    // Replay a scanned candidate to a RecoveredMatchState (reducer rebuild).
    // Returns false with a typed error if the session cannot be rebuilt.
    bool buildRecoveredState(const QString& sessionId,
                             RecoveredMatchState* out, ErrorInfo* err);

    // Archive a session out of Sessions/Current: discarded=false → normal
    // Sessions/Archive/yyyy/MM; discarded=true (or corrupt) → Sessions/Corrupt
    // (the byte-exact original is preserved). Used by the dialog's
    // Discard / View-then-close actions and for auto-archiving complete ones.
    StorageResult archiveOrDiscard(const QString& sessionId, bool discarded);

    // Test seam: scan a specific directory instead of Sessions/Current.
    void setScanDirectoryForTesting(const QString& dir) { m_scanDirOverride = dir; }

signals:
    // Emitted by scan() when candidates exist (for main.cpp to open the UI).
    void recoveryChoicesReady(int candidateCount);

private:
    QString scanDirectory() const;
    RecoveryCandidate classifyFile(const QString& path);
    RecoveryCandidate* find(const QString& sessionId);

    QString m_scanDirOverride;
    QVector<RecoveryCandidate> m_candidates;   // cached from the last scan()
};

} // namespace rel
} // namespace ta

#endif // TA_REL_RECOVERYCOORDINATOR_H
