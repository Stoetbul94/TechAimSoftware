#ifndef TA_RMS_PROGRAMMEDISPLAY_H
#define TA_RMS_PROGRAMMEDISPLAY_H

// ─────────────────────────────────────────────────────────────────────────────
// Turning a stable `programmeId` into something a range officer can read.
//
// THE DIRECTION IS ONE-WAY AND MUST STAY ONE-WAY. Display text is DERIVED
// FROM the identity; the identity is never derived from, matched against or
// looked up by display text. QML-LANG-001 is the reason: a translated string
// used as a key once put a 10 m Air Pistol session into the rifle scoring
// branch. RMS has no scoring branch to corrupt, but a dashboard that labels
// lane 4 with the wrong event is a competition incident of its own.
//
// This is a pure function of the id — deliberately NOT a second copy of
// CompetitionCatalogue.qml. A lookup table here would be a duplicate
// description of the programme set, and duplicate descriptions drift.
// ─────────────────────────────────────────────────────────────────────────────

#include <QString>

namespace ta {
namespace rms {

class ProgrammeDisplay
{
public:
    // "issf.10m.air-rifle.qualification60" -> "10 m Air Rifle · Qualification 60"
    // An id this function does not recognise is returned unchanged: showing
    // the raw stable id is always honest, inventing a friendly name is not.
    static QString describe(const QString& programmeId);

    // The distance/discipline part alone, for narrow dashboard columns.
    static QString shortDescribe(const QString& programmeId);

    // Whether the PROGRAMME is an official competition course. Decided by
    // rulesetId, never by the words in the name — "techaim.*" presets shoot
    // on an ISSF target under ISSF scoring but are not ISSF events.
    static bool isOfficialProgramme(const QString& rulesetId);
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_PROGRAMMEDISPLAY_H
