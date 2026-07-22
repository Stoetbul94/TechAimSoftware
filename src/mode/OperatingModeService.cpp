#include "OperatingModeService.h"

#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

using ta::mode::Mode;
using ta::mode::ShotSource;

OperatingModeService::OperatingModeService(QString configPath,
                                           Mode running,
                                           bool configValid,
                                           QString rawValue,
                                           QObject* parent)
    : QObject(parent)
    , m_configPath(std::move(configPath))
    , m_running(running)
    , m_selected(running)          // selection starts equal to the running mode
    , m_configValid(configValid)
    , m_rawValue(std::move(rawValue))
{
}

bool OperatingModeService::sourceAllowed(int shotSource) const
{
    const ShotSource s = (shotSource == static_cast<int>(ShotSource::Simulated))
                             ? ShotSource::Simulated
                             : ShotSource::Physical;
    return ta::mode::sourceAllowed(m_running, s);
}

void OperatingModeService::selectMode(int mode)
{
    const Mode m = (mode == static_cast<int>(Mode::Demo)) ? Mode::Demo : Mode::Live;
    if (m == m_selected && m_restartRequired == (m != m_running))
        return;
    m_selected = m;
    // A restart is required whenever the operator has selected a mode that
    // differs from the one the process is actually running. Selecting back to
    // the running mode clears the pending state.
    m_restartRequired = (m_selected != m_running);
    emit selectionChanged();
}

bool OperatingModeService::applyModeChange(bool sessionBusy)
{
    // Guard 1: never switch while a session is active. Config stays untouched.
    if (sessionBusy) {
        m_lastError = QStringLiteral("Operating mode cannot be changed while a session is active.");
        emit errorChanged();
        return false;
    }
    // Guard 2: nothing to do if the selection equals the running mode.
    if (m_selected == m_running) {
        m_restartRequired = false;
        emit selectionChanged();
        return false;
    }
    // Guard 3: atomic write. On failure the old file is retained (QSaveFile
    // never replaces the target unless commit() succeeds) and we surface it.
    QString err;
    if (!writeModeToConfig(m_configPath, m_selected, &err)) {
        m_lastError = err.isEmpty()
                          ? QStringLiteral("Could not save the operating mode setting.")
                          : err;
        emit errorChanged();
        emit configWriteFailed(m_lastError);
        return false;
    }
    m_restartRequired = true;
    emit selectionChanged();
    return true;
}

void OperatingModeService::requestRestart()
{
    emit restartRequested();
}

// Surgical, atomic edit of [App_Settings]/app_mode. Preserves every other key,
// group, comment and blank line byte-for-byte (only the app_mode value line is
// rewritten / inserted). Uses QSaveFile so a crash or power loss mid-write can
// never leave a truncated config.ini.
bool OperatingModeService::writeModeToConfig(const QString& path, Mode mode, QString* err)
{
    const QString value = ta::mode::modeToConfigString(mode);   // exactly Live/Demo
    const QString group = QStringLiteral("App_Settings");
    const QString key   = QStringLiteral("app_mode");

    // 1. Read existing content (empty if the file does not yet exist).
    QString text;
    QByteArray existing;
    QFileInfo fi(path);
    if (fi.exists()) {
        QFile in(path);
        if (!in.open(QIODevice::ReadOnly)) {
            if (err) *err = QStringLiteral("Cannot read the configuration file: %1").arg(path);
            return false;
        }
        existing = in.readAll();
        in.close();
        text = QString::fromUtf8(existing);
    }

    // 2. Detect newline convention; split, normalising each line (drop CR so we
    //    don't double it when we rejoin with the detected newline).
    const QString nl = text.contains(QLatin1String("\r\n"))
                           ? QStringLiteral("\r\n") : QStringLiteral("\n");
    QStringList lines = text.split(QLatin1Char('\n'));
    for (QString& l : lines)
        if (l.endsWith(QLatin1Char('\r'))) l.chop(1);

    // 3. Locate the [App_Settings] group and, within it, the app_mode line.
    int groupHeader = -1;
    int keyLine = -1;
    for (int i = 0; i < lines.size(); ++i) {
        const QString t = lines[i].trimmed();
        if (t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']'))) {
            const QString g = t.mid(1, t.size() - 2).trimmed();
            if (groupHeader >= 0)
                break;                       // reached the next group → stop
            if (g.compare(group, Qt::CaseInsensitive) == 0)
                groupHeader = i;
            continue;
        }
        if (groupHeader >= 0) {
            const int eq = t.indexOf(QLatin1Char('='));
            if (eq > 0
                && t.left(eq).trimmed().compare(key, Qt::CaseInsensitive) == 0) {
                keyLine = i;
                break;
            }
        }
    }

    const QString newLine = key + QLatin1Char('=') + value;
    if (keyLine >= 0) {
        lines[keyLine] = newLine;                        // replace in place
    } else if (groupHeader >= 0) {
        lines.insert(groupHeader + 1, newLine);          // insert at group top
    } else {
        // No group at all — append one. Ensure a separating blank line.
        if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
            lines.append(QString());
        lines.append(QLatin1Char('[') + group + QLatin1Char(']'));
        lines.append(newLine);
        lines.append(QString());                         // trailing newline
    }

    // 4. Atomic write.
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) *err = QStringLiteral("Cannot open the configuration file for writing: %1")
                            .arg(out.errorString());
        return false;
    }
    const QByteArray payload = lines.join(nl).toUtf8();
    if (out.write(payload) != payload.size()) {
        if (err) *err = QStringLiteral("Failed while writing the configuration file.");
        out.cancelWriting();
        return false;
    }
    if (!out.commit()) {                                 // rename-into-place
        if (err) *err = QStringLiteral("Failed to save the configuration file: %1")
                            .arg(out.errorString());
        return false;
    }
    return true;
}
