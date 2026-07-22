#ifndef TA_MODE_OPERATINGMODE_H
#define TA_MODE_OPERATINGMODE_H

// F10 — Operating mode (Live target vs Demo simulation).
//
// This header is the PURE, authoritative policy for the operating mode. It is
// QtCore-only (QString/Qt::CaseInsensitive) and has NO QObject, NO GUI, NO
// filesystem — so it can be unit-tested and shared by controllers without
// pulling in the service. Three orthogonal concepts are kept deliberately
// separate throughout the codebase (do not conflate them):
//
//   * Operating mode  — Live target input or Demo simulation (THIS file).
//   * Build identity  — which source commit produced the executable (F9B).
//   * Session type     — discipline / course being run.
//
// The single scoring rule: exactly one shot source may score in a given mode.
// Live accepts only Physical target shots; Demo accepts only Simulated ones.

#include <QString>
#include <Qt>

namespace ta { namespace mode {

enum class Mode { Live, Demo };

// Where a shot originated. The gate below rejects the source that does not
// match the running mode BEFORE the shot is durably accepted.
enum class ShotSource { Physical, Simulated };

// Canonical config token for a mode. config.ini stores exactly "Live"/"Demo".
inline QString modeToConfigString(Mode m)
{
    return m == Mode::Live ? QStringLiteral("Live") : QStringLiteral("Demo");
}

// Short human badge label.
inline QString modeBadgeText(Mode m)
{
    return m == Mode::Live ? QStringLiteral("LIVE TARGET")
                           : QStringLiteral("DEMO / SIMULATION");
}

struct ParsedMode {
    Mode mode = Mode::Live;   // safe fallback (see F10 STARTUP VALIDATION)
    bool valid = false;       // false → raw was missing/unknown
    QString raw;              // the original token, for logging
};

// Parse a config value case-consistently. Unknown/empty → invalid, and the
// caller falls back to Live (matching the product's existing "default Live"
// safety posture) WITHOUT silently enabling Demo input.
inline ParsedMode parseMode(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.compare(QLatin1String("Live"), Qt::CaseInsensitive) == 0)
        return { Mode::Live, true, raw };
    if (t.compare(QLatin1String("Demo"), Qt::CaseInsensitive) == 0)
        return { Mode::Demo, true, raw };
    return { Mode::Live, false, raw };
}

// THE authoritative input-source gate. Consulted at the durable-acceptance
// seam of every discipline; a wrong-source shot is rejected before it can
// create an accepted-shot or a journal event.
inline bool sourceAllowed(Mode m, ShotSource s)
{
    return (m == Mode::Live && s == ShotSource::Physical)
        || (m == Mode::Demo && s == ShotSource::Simulated);
}

// ── Session metadata ─────────────────────────────────────────────────────
// A session records the mode it STARTED in; that stays authoritative for the
// session and is never re-inferred from the current config. Sessions written
// before F10 carry no field and replay as Unknown/Legacy.
enum class SessionMode { Unknown, Live, Demo };

inline QString sessionModeToken(SessionMode s)
{
    switch (s) {
    case SessionMode::Live: return QStringLiteral("Live");
    case SessionMode::Demo: return QStringLiteral("Demo");
    default:                return QStringLiteral("Unknown");
    }
}

inline SessionMode sessionModeFromToken(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.compare(QLatin1String("Live"), Qt::CaseInsensitive) == 0) return SessionMode::Live;
    if (t.compare(QLatin1String("Demo"), Qt::CaseInsensitive) == 0) return SessionMode::Demo;
    return SessionMode::Unknown;
}

inline SessionMode sessionModeFromMode(Mode m)
{
    return m == Mode::Live ? SessionMode::Live : SessionMode::Demo;
}

// Report/label wording for a session's recorded mode.
inline QString sessionModeLabel(SessionMode s)
{
    switch (s) {
    case SessionMode::Live: return QStringLiteral("LIVE TARGET SESSION");
    case SessionMode::Demo: return QStringLiteral("DEMO SESSION · SIMULATED INPUT");
    default:                return QStringLiteral("LEGACY SESSION · MODE UNKNOWN");
    }
}

}} // namespace ta::mode

#endif // TA_MODE_OPERATINGMODE_H
