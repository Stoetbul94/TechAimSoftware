#ifndef TA_RMS_FIELDTESTRECORDER_H
#define TA_RMS_FIELDTESTRECORDER_H

// ─────────────────────────────────────────────────────────────────────────────
// FIELD-TEST EVENT RECORDER — what RMS saw, in the order it saw it.
//
// ═══ WHAT THIS IS NOT ══════════════════════════════════════════════════════
//
// It is NOT the node's SessionStore. It is NOT a competition record. It is NOT
// an adjudication log, and a jury decision must never be written into the same
// stream as a raw observation — see docs/architecture/rms-field-test-
// instrumentation.md. It is a range-observation diary kept by RMS for the
// people debugging a range afterwards.
//
// Starting and stopping it does nothing to any target. It is RMS-local, and
// closing it mid-match affects nothing but this file.
//
// ═══ APPEND-ONLY, ONE JSON OBJECT PER LINE ═════════════════════════════════
//
// JSONL, flushed line by line. A single giant JSON document would become
// unparseable the moment the process died mid-write, which is precisely the
// run whose evidence matters most. Every completed line survives a crash.
//
// ═══ IT MUST NOT SLOW INGESTION ════════════════════════════════════════════
//
// Events are appended to an in-memory buffer and written on a timer, not
// inside the UDP read. A twenty-lane range at full rate must never wait on a
// disk. The UI keeps only the most recent `kUiEventCap` events; the file keeps
// everything.
// ─────────────────────────────────────────────────────────────────────────────

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonObject>
#include <QString>
#include <QTimer>
#include <QVector>

namespace ta {
namespace rms {

class FieldTestRecorder : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY changed)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY changed)
    Q_PROPERTY(QString testName READ testName NOTIFY changed)
    Q_PROPERTY(QString rangeName READ rangeName NOTIFY changed)
    Q_PROPERTY(QString operatorName READ operatorName NOTIFY changed)
    Q_PROPERTY(QString notes READ notes NOTIFY changed)
    Q_PROPERTY(int eventCount READ eventCount NOTIFY changed)
    Q_PROPERTY(QString elapsedLabel READ elapsedLabel NOTIFY tick)
    Q_PROPERTY(QString logPath READ logPath NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY changed)
    Q_PROPERTY(QString previousSessionId READ previousSessionId NOTIFY changed)

public:
    explicit FieldTestRecorder(QObject* parent = nullptr);
    ~FieldTestRecorder() override;

    enum Roles {
        AtRole = Qt::UserRole + 1,
        TypeRole,
        SummaryRole,
        LaneRole,
        StationRole,
        DetailRole
    };

    // ── the operator's two buttons ───────────────────────────────────────
    // `mode` is "LIVE" or "DEMO" and is stamped into the header, so a demo
    // bundle can never be mistaken for physical evidence.
    Q_INVOKABLE bool start(const QString& testName, const QString& rangeName,
                           const QString& operatorName, const QString& notes,
                           const QString& mode);
    Q_INVOKABLE void stop();

    // ── recording ────────────────────────────────────────────────────────
    // `detail` carries whatever the event type needs; nothing here is
    // interpreted, so a new field costs nothing.
    void record(const QString& type, const QString& summary,
                const QJsonObject& detail = QJsonObject());
    void recordLane(const QString& type, const QString& summary, int laneNumber,
                    const QString& nodeId, const QString& stationCode,
                    const QJsonObject& detail = QJsonObject());

    bool isActive() const { return m_active; }
    QString sessionId() const { return m_sessionId; }
    QString testName() const { return m_testName; }
    QString rangeName() const { return m_rangeName; }
    QString operatorName() const { return m_operator; }
    QString notes() const { return m_notes; }
    QString mode() const { return m_mode; }
    int eventCount() const { return m_eventCount; }
    QString elapsedLabel() const;
    qint64 elapsedMs() const;
    QString logPath() const { return m_logPath; }
    QString lastError() const { return m_lastError; }
    QString previousSessionId() const { return m_previousSessionId; }
    qint64 startedUtcMs() const { return m_startedUtcMs; }

    // The directory field-test material is written under. Created on demand.
    static QString fieldTestRoot();

    // Deterministic in tests: an injected clock removes every sleep.
    void setClockForTesting(qint64 (*fn)()) { m_clock = fn; }
    // Tests want the file on disk immediately rather than on the next tick.
    Q_INVOKABLE void flush();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Every event of the session, for the exporter. Read from the file so the
    // export is exactly what was recorded, not a second in-memory opinion.
    QVector<QJsonObject> allEvents() const;

    static constexpr int kUiEventCap = 400;

signals:
    void changed();
    void tick();

private:
    struct Entry {
        qint64 atUtcMs = 0;
        QString type;
        QString summary;
        int laneNumber = 0;
        QString stationCode;
        QString detailText;
    };

    qint64 now() const;
    void writeLine(const QJsonObject& o);
    void openLog();
    void closeLog();

    bool m_active = false;
    QString m_sessionId;
    QString m_testName;
    QString m_rangeName;
    QString m_operator;
    QString m_notes;
    QString m_mode;
    QString m_logPath;
    QString m_lastError;
    QString m_previousSessionId;
    qint64 m_startedUtcMs = 0;
    int m_eventCount = 0;

    QFile m_file;
    QVector<QJsonObject> m_pending;   // waiting to reach the disk
    QVector<Entry> m_ui;              // what the timeline view shows
    QTimer m_flushTimer;
    QTimer m_tickTimer;
    qint64 (*m_clock)() = nullptr;
};

}  // namespace rms
}  // namespace ta

#endif
