#include "rms/FieldTestRecorder.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTextStream>

namespace ta {
namespace rms {

namespace {

constexpr int kFlushIntervalMs = 500;   // off the ingestion path
constexpr int kTickIntervalMs  = 1000;  // the elapsed clock on screen

QString stampFor(qint64 utcMs)
{
    return QDateTime::fromMSecsSinceEpoch(utcMs, Qt::UTC)
        .toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzZ"));
}

}  // namespace

FieldTestRecorder::FieldTestRecorder(QObject* parent)
    : QAbstractListModel(parent)
{
    m_flushTimer.setInterval(kFlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &FieldTestRecorder::flush);
    m_tickTimer.setInterval(kTickIntervalMs);
    connect(&m_tickTimer, &QTimer::timeout, this, &FieldTestRecorder::tick);
}

FieldTestRecorder::~FieldTestRecorder()
{
    // A test that ends because the application closed is still a test whose
    // evidence matters; write what is buffered before going.
    if (m_active) {
        record(QStringLiteral("RMS_STOP"), QStringLiteral("RMS closed"));
        flush();
    }
    closeLog();
}

qint64 FieldTestRecorder::now() const
{
    return m_clock ? m_clock() : QDateTime::currentMSecsSinceEpoch();
}

QString FieldTestRecorder::fieldTestRoot()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString dir = QDir(base).filePath(QStringLiteral("field-tests"));
    QDir().mkpath(dir);
    return dir;
}

bool FieldTestRecorder::start(const QString& testName, const QString& rangeName,
                              const QString& operatorName, const QString& notes,
                              const QString& mode)
{
    if (m_active)
        return false;

    // A NEW session id every time. RMS cannot reconstruct events it never
    // received, so a restart begins a new segment and NAMES the one before it
    // rather than pretending the gap did not happen.
    m_previousSessionId = m_sessionId;
    const qint64 t = now();
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(t, Qt::UTC);
    m_sessionId = QStringLiteral("FT-%1-%2")
                      .arg(dt.toString(QStringLiteral("yyyyMMdd")))
                      .arg(dt.toString(QStringLiteral("HHmmss")));
    m_testName = testName.trimmed();
    m_rangeName = rangeName.trimmed();
    m_operator = operatorName.trimmed();
    m_notes = notes.trimmed();
    m_mode = mode.isEmpty() ? QStringLiteral("UNKNOWN") : mode;
    m_startedUtcMs = t;
    m_eventCount = 0;
    m_lastError.clear();

    beginResetModel();
    m_ui.clear();
    endResetModel();
    m_pending.clear();

    // A NEW session gets a NEW file. Two logs started inside the same second
    // would otherwise share a name and, opened for append, would interleave
    // into one unreadable stream.
    QString base = QDir(fieldTestRoot()).filePath(m_sessionId + QStringLiteral("-events.jsonl"));
    for (int n = 2; QFile::exists(base) && n < 100; ++n) {
        base = QDir(fieldTestRoot())
                   .filePath(QStringLiteral("%1-%2-events.jsonl").arg(m_sessionId).arg(n));
    }
    m_logPath = base;
    openLog();
    if (!m_lastError.isEmpty()) {
        m_active = false;
        emit changed();
        return false;
    }

    m_active = true;

    // The header line: everything needed to interpret the rest of the file.
    QJsonObject header;
    header[QStringLiteral("at")] = stampFor(t);
    header[QStringLiteral("eventType")] = QStringLiteral("FIELD_TEST_STARTED");
    header[QStringLiteral("summary")] = QStringLiteral("Field-test log started");
    QJsonObject d;
    d[QStringLiteral("sessionId")] = m_sessionId;
    d[QStringLiteral("testName")] = m_testName;
    d[QStringLiteral("rangeName")] = m_rangeName;
    d[QStringLiteral("operator")] = m_operator;
    d[QStringLiteral("notes")] = m_notes;
    // STAMPED AT THE TOP OF EVERY FILE. A demo log must never be mistaken for
    // physical range evidence, and the first line is where anyone looks.
    d[QStringLiteral("mode")] = m_mode;
    d[QStringLiteral("simulated")] = (m_mode != QLatin1String("LIVE"));
    if (!m_previousSessionId.isEmpty())
        d[QStringLiteral("previousTestSessionId")] = m_previousSessionId;
    header[QStringLiteral("detail")] = d;
    writeLine(header);

    m_flushTimer.start();
    m_tickTimer.start();
    flush();
    emit changed();
    return true;
}

void FieldTestRecorder::stop()
{
    if (!m_active)
        return;
    record(QStringLiteral("FIELD_TEST_STOPPED"), QStringLiteral("Field-test log stopped"));
    flush();
    m_flushTimer.stop();
    m_tickTimer.stop();
    closeLog();
    m_active = false;
    emit changed();
}

void FieldTestRecorder::record(const QString& type, const QString& summary,
                               const QJsonObject& detail)
{
    recordLane(type, summary, 0, QString(), QString(), detail);
}

void FieldTestRecorder::recordLane(const QString& type, const QString& summary,
                                   int laneNumber, const QString& nodeId,
                                   const QString& stationCode,
                                   const QJsonObject& detail)
{
    if (!m_active)
        return;

    const qint64 t = now();
    QJsonObject o;
    o[QStringLiteral("at")] = stampFor(t);
    o[QStringLiteral("atUtcMs")] = t;
    o[QStringLiteral("eventType")] = type;
    o[QStringLiteral("summary")] = summary;
    if (laneNumber > 0)
        o[QStringLiteral("laneNumber")] = laneNumber;
    if (!nodeId.isEmpty())
        o[QStringLiteral("nodeId")] = nodeId;
    if (!stationCode.isEmpty())
        o[QStringLiteral("stationCode")] = stationCode;
    if (!detail.isEmpty())
        o[QStringLiteral("detail")] = detail;
    writeLine(o);

    Entry e;
    e.atUtcMs = t;
    e.type = type;
    e.summary = summary;
    e.laneNumber = laneNumber;
    e.stationCode = stationCode;
    if (!detail.isEmpty()) {
        e.detailText = QString::fromUtf8(
            QJsonDocument(detail).toJson(QJsonDocument::Compact));
    }

    // Newest first, and BOUNDED: the file is the record, the view is a window.
    // An unbounded QML list would grow all day on a twenty-lane range.
    beginInsertRows(QModelIndex(), 0, 0);
    m_ui.prepend(e);
    endInsertRows();
    if (m_ui.size() > kUiEventCap) {
        beginRemoveRows(QModelIndex(), m_ui.size() - 1, m_ui.size() - 1);
        m_ui.removeLast();
        endRemoveRows();
    }

    ++m_eventCount;
    emit changed();
}

void FieldTestRecorder::writeLine(const QJsonObject& o)
{
    m_pending.append(o);
}

void FieldTestRecorder::openLog()
{
    closeLog();
    m_file.setFileName(m_logPath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_lastError = QStringLiteral("cannot open %1: %2")
                          .arg(m_logPath, m_file.errorString());
    }
}

void FieldTestRecorder::closeLog()
{
    if (m_file.isOpen())
        m_file.close();
}

void FieldTestRecorder::flush()
{
    if (m_pending.isEmpty())
        return;
    if (!m_file.isOpen()) {
        m_pending.clear();
        return;
    }
    QByteArray blob;
    for (const QJsonObject& o : m_pending) {
        blob += QJsonDocument(o).toJson(QJsonDocument::Compact);
        blob += '\n';
    }
    m_pending.clear();
    if (m_file.write(blob) < 0)
        m_lastError = m_file.errorString();
    // One flush per batch, not per event: the point of buffering is that the
    // UDP reader never waits for a disk.
    m_file.flush();
}

qint64 FieldTestRecorder::elapsedMs() const
{
    if (!m_active || m_startedUtcMs <= 0)
        return 0;
    return qMax<qint64>(0, now() - m_startedUtcMs);
}

QString FieldTestRecorder::elapsedLabel() const
{
    const qint64 s = elapsedMs() / 1000;
    return QStringLiteral("%1:%2:%3")
        .arg(s / 3600, 2, 10, QLatin1Char('0'))
        .arg((s / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'));
}

QVector<QJsonObject> FieldTestRecorder::allEvents() const
{
    QVector<QJsonObject> out;
    QFile f(m_logPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        // A partial final line is exactly what a crash leaves behind. Skip it
        // and keep every complete record before it — the reason for JSONL.
        if (err.error == QJsonParseError::NoError && doc.isObject())
            out.append(doc.object());
    }
    return out;
}

int FieldTestRecorder::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_ui.size();
}

QVariant FieldTestRecorder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ui.size())
        return QVariant();
    const Entry& e = m_ui.at(index.row());
    switch (role) {
    case AtRole:
        return QDateTime::fromMSecsSinceEpoch(e.atUtcMs)
            .toString(QStringLiteral("HH:mm:ss.zzz"));
    case TypeRole:      return e.type;
    case SummaryRole:   return e.summary;
    case LaneRole:      return e.laneNumber;
    case StationRole:   return e.stationCode;
    case DetailRole:    return e.detailText;
    default:            return QVariant();
    }
}

QHash<int, QByteArray> FieldTestRecorder::roleNames() const
{
    return {
        { AtRole,      "at" },
        { TypeRole,    "eventType" },
        { SummaryRole, "summary" },
        { LaneRole,    "laneNumber" },
        { StationRole, "stationCode" },
        { DetailRole,  "detailText" }
    };
}

}  // namespace rms
}  // namespace ta
