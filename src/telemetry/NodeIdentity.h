#ifndef TA_TELEMETRY_NODEIDENTITY_H
#define TA_TELEMETRY_NODEIDENTITY_H

// ─────────────────────────────────────────────────────────────────────────────
// WHO THIS TARGET STATION IS.
//
// nodeId  — generated ONCE and persisted in the application's own settings.
//           It survives application restarts, re-cabling, a new COM port, a
//           new IP address, a lane re-assignment and a different athlete,
//           because every one of those changes while the station stays the
//           same station. Anything that changes when the station does not is
//           an attribute, not an identity: RMS carries the COM port, the
//           device fingerprint and the lane as SEPARATE fields.
//
// bootId  — generated fresh for every process. Never persisted. This is what
//           lets RMS tell "the node's application restarted" from "the network
//           blinked", which are different situations needing opposite
//           handling. One NodeIdentity instance represents one run of the
//           node application; constructing a second one is, by design, exactly
//           what a restart looks like on the wire.
// ─────────────────────────────────────────────────────────────────────────────

#include <QString>

class QSettings;

namespace ta {
namespace telemetry {

class NodeIdentity
{
public:
    // Production. Uses the application's own QSettings namespace, which
    // main.cpp establishes from ProductIdentity before anything reads it.
    static NodeIdentity forApplication();

    // Tests, and any tool that must not touch the real installation.
    static NodeIdentity forSettingsFile(const QString& iniPath);

    QString nodeId() const { return m_nodeId; }
    QString bootId() const { return m_bootId; }
    // True when this construction had to mint the nodeId, i.e. first ever run
    // on this station. Logged once; never used as a behaviour switch.
    bool nodeIdWasGenerated() const { return m_generated; }

    // The settings key, named here so the migration/support tooling and the
    // tests agree on one spelling.
    static const char* settingsKey();

private:
    NodeIdentity() = default;
    static NodeIdentity build(QSettings& settings);

    QString m_nodeId;
    QString m_bootId;
    bool    m_generated = false;
};

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_NODEIDENTITY_H
