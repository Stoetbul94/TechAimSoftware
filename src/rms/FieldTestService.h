#ifndef TA_RMS_FIELDTESTSERVICE_H
#define TA_RMS_FIELDTESTSERVICE_H

// ─────────────────────────────────────────────────────────────────────────────
// FIELD-TEST SERVICE — the instrument panel for a range day.
//
// It watches the monitor and the configuration, turns the changes worth
// remembering into timeline events, answers the preflight questions, and
// writes the evidence bundle at the end.
//
// ═══ IT OBSERVES; IT DOES NOT ACT ══════════════════════════════════════════
//
// Nothing here transmits, and nothing here changes what RMS believes. It reads
// the monitor's records and the range configuration and writes to RMS's own
// disk. Starting or stopping a field-test log has no effect on any target: if
// this whole class were deleted mid-match, every station would carry on and
// every score would be unchanged.
//
// ═══ WHY IT DIFFS RATHER THAN LISTENS TO EVERYTHING ════════════════════════
//
// `nodeChanged` fires on every heartbeat. A timeline built from that signal
// would be thousands of identical lines with the interesting three buried in
// them. So the service keeps a snapshot per node and records only TRANSITIONS:
// went offline, came back, restarted, target connected, shots became unseen.
// ─────────────────────────────────────────────────────────────────────────────

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "rms/TargetNodeRecord.h"

namespace ta {
namespace rms {

class RangeMonitor;
class RangeConfigurationService;
class MatchPlanService;
class FieldTestRecorder;
class NetworkDiagnostics;

class FieldTestService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap counters READ counters NOTIFY changed)
    Q_PROPERTY(QVariantList preflight READ preflight NOTIFY changed)
    Q_PROPERTY(QString preflightVerdict READ preflightVerdict NOTIFY changed)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY changed)
    Q_PROPERTY(QString lastExportError READ lastExportError NOTIFY changed)

public:
    FieldTestService(RangeMonitor* monitor,
                     RangeConfigurationService* range,
                     MatchPlanService* plans,
                     FieldTestRecorder* recorder,
                     NetworkDiagnostics* network,
                     QObject* parent = nullptr);

    // ── station identity, for every surface that shows a station ─────────
    // Codes are computed across ALL known stations at once so two lanes can
    // never be labelled ambiguously. See StationCode.
    Q_INVOKABLE QString stationCode(const QString& nodeId) const;
    Q_INVOKABLE bool stationCodesCollide() const;

    // ── the instrument panel ────────────────────────────────────────────
    QVariantMap counters() const;
    QVariantList preflight() const;
    QString preflightVerdict() const;

    // Everything known about one lane, for Lane detail → Diagnostics.
    Q_INVOKABLE QVariantMap laneDiagnostics(int laneNumber) const;

    // ── evidence ────────────────────────────────────────────────────────
    // Writes the bundle and returns its directory, or an empty string on
    // failure with the reason in lastExportError().
    Q_INVOKABLE QString exportFieldTest();
    QString lastExportPath() const { return m_lastExportPath; }
    QString lastExportError() const { return m_lastExportError; }

    // Called by the application when the mode is known, so the log and the
    // bundle can be stamped with it.
    void setMode(const QString& mode, bool live);

    // Deterministic in tests.
    void setClockForTesting(qint64 (*fn)()) { m_clock = fn; }
    // The application drives this on the same timer it evaluates liveness on;
    // the service compares state and records what actually changed.
    Q_INVOKABLE void poll();

    // Records the opening context of a freshly started log: what RMS is, what
    // the socket is doing, and which stations are already known. Without it a
    // log started mid-session would begin with no way to interpret it.
    Q_INVOKABLE void noteLogStarted();

signals:
    void changed();

private:
    struct Snapshot {
        ConnectionState connection = ConnectionState::Unknown;
        QString bootId;
        int shotsAcceptedByNode = 0;
        int observed = 0;
        int unseen = 0;
        int worstUnseen = 0;   // the deepest gap so far, so flapping stays quiet
        int laneNumber = 0;
        bool known = false;
    };

    qint64 now() const;
    void onNodeAdded(const QString& nodeId);
    void onShot(const QString& nodeId, int sequence);
    void refreshCodes();
    QJsonObject rangeSnapshotJson() const;
    QJsonObject laneMappingsJson() const;
    QJsonObject nodeSummaryJson() const;
    QJsonObject diagnosticsJson() const;
    QJsonObject summaryJson() const;
    QString summaryText() const;
    QString shotsCsv() const;
    int laneNumberFor(const QString& nodeId) const;

    RangeMonitor* m_monitor = nullptr;
    RangeConfigurationService* m_range = nullptr;
    MatchPlanService* m_plans = nullptr;
    FieldTestRecorder* m_recorder = nullptr;
    NetworkDiagnostics* m_network = nullptr;

    QHash<QString, QString> m_codes;      // nodeId -> station code
    QHash<QString, Snapshot> m_snapshots; // nodeId -> last known state
    QHash<int, QString> m_assigned;       // laneNumber -> nodeId, as last seen
    QString m_mode = QStringLiteral("LIVE");
    bool m_live = true;
    QString m_lastExportPath;
    QString m_lastExportError;
    qint64 (*m_clock)() = nullptr;
};

}  // namespace rms
}  // namespace ta

#endif
