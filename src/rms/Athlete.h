#ifndef TA_RMS_ATHLETE_H
#define TA_RMS_ATHLETE_H

// ─────────────────────────────────────────────────────────────────────────────
// A SMALL RMS START LIST — deliberately not a federation database.
//
// Enough to put a name on a lane and have it survive a restart, and no more.
// Licences, classifications, categories, eligibility and national records are
// all federation concerns; inventing an RMS-local version of any of them would
// create a second source of truth that no federation recognises.
//
// This is RMS's own data. It has nothing to do with the athlete name the node
// reports in its telemetry, which remains an OBSERVATION of what that station
// was told locally.
// ─────────────────────────────────────────────────────────────────────────────

#include <QJsonObject>
#include <QString>

namespace ta {
namespace rms {

struct Athlete {
    // Stable identity, minted once. Two people may share a display name — the
    // id is what an assignment refers to, never the name.
    QString athleteId;
    QString displayName;
    QString club;
    QString country;
    QString notes;
    // Quick field-test entry. Recorded so a start list assembled at the firing
    // point is distinguishable from one entered deliberately, without being
    // treated differently anywhere.
    bool    temporary = false;

    bool isValid() const { return !athleteId.isEmpty() && !displayName.trimmed().isEmpty(); }

    QJsonObject toJson() const;
    static Athlete fromJson(const QJsonObject& o);
    static Athlete create(const QString& displayName, bool temporary = false);
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_ATHLETE_H
