#pragma once

// Wind Map — domain types (Training Lab Release 2, stage 3).
//
// Spec: docs/training-lab-wind-map-implementation-spec.md
//       docs/training-lab-wind-map-spec-review.md §7 (approved decisions)
//
// Wind Map is a POST-SESSION REVIEW programme. Nothing here scores, corrects,
// advises or participates in a competition. It records what the athlete
// observed and what the shot did, and keeps the two associated honestly.
//
// QtCore-only by design: this header compiles into the GUI-free reliability
// harness, which is what proves the domain carries no presentation.
//
// FIXED POINT. The journal writer has no double overload — every numeric field
// in the event catalogue is an integer, so replay is bit-exact and hashes are
// stable. Wind speed is therefore stored in HUNDREDTHS of a metre per second
// (2.5 m/s -> 250), matching the hundredth-mm convention already used for shot
// coordinates. The implementation spec's `double speedMetresPerSecond` is
// corrected here for that reason; m/s remains the unit the operator sees.

#include <QString>

#include <cmath>
#include <limits>

namespace ta {
namespace training {

// ── Discipline scope ────────────────────────────────────────────────────────
// APPROVED: 50 m Rifle Prone and 50 m Rifle 3 Positions ONLY. Never 10 m.
// Unknown ids fail CLOSED — an unsupported discipline is never coerced into a
// supported one.
inline const char* kWindMapDisciplineProne50 = "PRONE50";
inline const char* kWindMapDiscipline3P50    = "3P50";

inline bool isWindMapDiscipline(const QString& id)
{
    return id == QLatin1String(kWindMapDisciplineProne50)
        || id == QLatin1String(kWindMapDiscipline3P50);
}

inline bool windMapDisciplineIs3P(const QString& id)
{
    return id == QLatin1String(kWindMapDiscipline3P50);
}

// ── Positions ───────────────────────────────────────────────────────────────
// Kneeling / Prone / Standing stay SEPARATE in state and in analysis. A
// combined overview may present them side by side; nothing pools them.
enum class WindMapPosition : qint8 {
    NotApplicable = 0,   // 50 m Prone — a single position, so none is named
    Kneeling      = 1,
    Prone         = 2,
    Standing      = 3,
};

inline bool isValidWindMapPosition(qint8 raw, bool is3P)
{
    if (is3P)
        return raw >= static_cast<qint8>(WindMapPosition::Kneeling)
            && raw <= static_cast<qint8>(WindMapPosition::Standing);
    return raw == static_cast<qint8>(WindMapPosition::NotApplicable);
}

inline QString windMapPositionName(WindMapPosition p)
{
    switch (p) {
    case WindMapPosition::Kneeling: return QStringLiteral("Kneeling");
    case WindMapPosition::Prone:    return QStringLiteral("Prone");
    case WindMapPosition::Standing: return QStringLiteral("Standing");
    default:                        return QString();
    }
}

// ── Wind source ─────────────────────────────────────────────────────────────
// Release 1 records Manual only. WeatherStation is RESERVED so the domain and
// the journal can carry a future source without a schema change; NO device
// integration exists and none may be added without its own approval.
enum class WindSource : quint8 {
    Manual         = 0,
    WeatherStation = 1,   // RESERVED — never produced by Release 1
};

inline bool isImplementedWindSource(WindSource s) { return s == WindSource::Manual; }

inline QString windSourceKey(WindSource s)
{
    return s == WindSource::WeatherStation ? QStringLiteral("WeatherStation")
                                           : QStringLiteral("Manual");
}

inline bool windSourceFromKey(const QString& key, WindSource* out)
{
    if (key == QLatin1String("Manual"))         { *out = WindSource::Manual;         return true; }
    if (key == QLatin1String("WeatherStation")) { *out = WindSource::WeatherStation; return true; }
    return false;   // fails closed — an unknown source is not silently Manual
}

// ── Direction ───────────────────────────────────────────────────────────────
// 0° = North, increasing CLOCKWISE. Stored as normalised integer degrees;
// N/NE/E/… are presentation labels over that number, never the stored value.

// Normalises any finite angle into [0, 360). Returns false for NaN/infinity so
// a bad value can never enter the domain as 0° (which would read as North).
inline bool normalizeWindDegrees(double raw, qint16* out)
{
    if (!std::isfinite(raw))
        return false;
    double d = std::fmod(raw, 360.0);
    if (d < 0.0)
        d += 360.0;
    // fmod of a value just below a multiple of 360 can round up to exactly
    // 360.0; fold that back to 0 so the range is closed-open.
    auto deg = static_cast<qint16>(std::llround(d));
    if (deg >= 360) deg = static_cast<qint16>(deg % 360);
    if (deg < 0)    deg = 0;
    *out = deg;
    return true;
}

// Eight 45° sectors CENTRED on the compass points, so N spans the 0° wrap:
//   N [337.5, 22.5)  NE [22.5, 67.5)  E [67.5, 112.5)  SE [112.5, 157.5)
//   S [157.5, 202.5) SW [202.5, 247.5) W [247.5, 292.5) NW [292.5, 337.5)
enum class WindSector : qint8 { N = 0, NE, E, SE, S, SW, W, NW };

inline WindSector windSectorOfDegrees(qint16 deg)
{
    // Rotate by half a sector so integer division lands each sector on its
    // centre; this is what makes the N wrap fall out arithmetically instead of
    // needing a special case.
    //
    // The shift is +22, NOT +23. Degrees are integers and the boundary is
    // 22.5, so 22 belongs to N and 23 to NE — rounding the half-sector UP
    // pushes 22 into NE and moves every boundary by one degree. (Caught by
    // the boundary cases in tst_windmap.cpp.)
    const int shifted = (static_cast<int>(deg) + 360 + 22) % 360;
    return static_cast<WindSector>((shifted / 45) % 8);
}

inline QString windSectorLabel(WindSector s)
{
    switch (s) {
    case WindSector::N:  return QStringLiteral("N");
    case WindSector::NE: return QStringLiteral("NE");
    case WindSector::E:  return QStringLiteral("E");
    case WindSector::SE: return QStringLiteral("SE");
    case WindSector::S:  return QStringLiteral("S");
    case WindSector::SW: return QStringLiteral("SW");
    case WindSector::W:  return QStringLiteral("W");
    case WindSector::NW: return QStringLiteral("NW");
    }
    return QString();
}

// Stored centre value for each sector, used by the compass-ring selector.
inline qint16 windSectorCentreDegrees(WindSector s)
{
    return static_cast<qint16>(static_cast<int>(s) * 45);
}

// ── Speed ───────────────────────────────────────────────────────────────────
// AUTHORITATIVE REPRESENTATION: hundredths of a metre per second.
//
// The journal stores this integer; the operator only ever sees m/s. The raw
// hundredths value must not be shown in the UI or a report, and QML must never
// scale it by hand — the two helpers below are the single conversion point, so
// a rounding change happens in one place or not at all.
inline constexpr qint32 kMaxWindSpeedHundredthMs = 100000;   // 1000.00 m/s ceiling

// m/s -> hundredths. Returns false (leaving *out untouched) for NaN, infinity,
// a negative speed, or anything past the ceiling. Rejection happens BEFORE
// conversion so a bad value can never overflow into a plausible-looking one.
// Rounding is std::llround — half away from zero — and is deterministic.
inline bool metresPerSecondToHundredths(double metresPerSecond, qint32* out)
{
    if (!out) return false;
    if (!std::isfinite(metresPerSecond)) return false;
    if (metresPerSecond < 0.0) return false;
    const double hundredths = std::llround(metresPerSecond * 100.0);
    if (hundredths > static_cast<double>(kMaxWindSpeedHundredthMs)) return false;
    *out = static_cast<qint32>(hundredths);
    return true;
}

// hundredths -> m/s. Total: every stored value is in range by construction.
inline double hundredthsToMetresPerSecond(qint32 hundredths)
{
    return static_cast<double>(hundredths) / 100.0;
}

// APPROVED bands. Derived AT ANALYSIS TIME from the stored raw value, so the
// boundaries can be revised later without invalidating existing sessions.
// Each boundary value belongs to the LOWER band.
enum class WindSpeedBand : qint8 {
    Calm = 0,        // ONLY when calm was explicitly recorded
    Light,           // 0      <  v <= 2.00
    Moderate,        // 2.00   <  v <= 4.00
    Strong,          // 4.00   <  v <= 7.00
    VeryStrong,      // 7.00   <  v
};

inline QString windSpeedBandLabel(WindSpeedBand b)
{
    switch (b) {
    case WindSpeedBand::Calm:       return QStringLiteral("Calm");
    case WindSpeedBand::Light:      return QStringLiteral("Light");
    case WindSpeedBand::Moderate:   return QStringLiteral("Moderate");
    case WindSpeedBand::Strong:     return QStringLiteral("Strong");
    case WindSpeedBand::VeryStrong: return QStringLiteral("Very strong");
    }
    return QString();
}

// ── The snapshot ────────────────────────────────────────────────────────────
// Immutable once attached to a shot. `valid == false` means NO READING WAS
// RECORDED — it is never inferred, back-filled, or copied from a neighbouring
// shot, including during recovery.
//
// `calm == true` is a RECORDED OBSERVATION and is deliberately distinguishable
// from "no reading": a calm shot participates in analysis as its own
// condition, whereas a no-reading shot is excluded from condition comparisons.
struct WindConditionSnapshot {
    bool       valid = false;                 // false => No wind reading recorded
    bool       calm  = false;                 // true  => direction/speed not meaningful
    qint16     directionDegrees = 0;          // [0,360). Meaningless when calm
    qint32     speedHundredthMs = 0;          // >= 0. Meaningless when calm
    WindSource source = WindSource::Manual;
    qint64     recordedMsSinceEpoch = 0;
    QString    note;                          // optional, short

    static WindConditionSnapshot noReading()
    {
        return WindConditionSnapshot{};       // valid == false
    }

    static WindConditionSnapshot calmAt(qint64 msSinceEpoch,
                                        WindSource src = WindSource::Manual,
                                        const QString& n = QString())
    {
        WindConditionSnapshot s;
        s.valid = true;
        s.calm  = true;
        // Rule 4: a Calm snapshot must NOT acquire an inferred direction.
        s.directionDegrees = 0;
        s.speedHundredthMs = 0;
        s.source = src;
        s.recordedMsSinceEpoch = msSinceEpoch;
        s.note = n;
        return s;
    }

    // Builds a non-calm reading. Returns false (leaving *out untouched) when
    // the direction is not finite or the speed is negative/out of range —
    // rule 3, and the reason invalid input can never become a silent 0°.
    static bool measured(double directionDeg, double speedMs, qint64 msSinceEpoch,
                         WindConditionSnapshot* out,
                         WindSource src = WindSource::Manual,
                         const QString& n = QString())
    {
        if (!out) return false;
        qint16 deg = 0;
        if (!normalizeWindDegrees(directionDeg, &deg))
            return false;
        qint32 hundredths = 0;
        if (!metresPerSecondToHundredths(speedMs, &hundredths))
            return false;

        WindConditionSnapshot s;
        s.valid = true;
        s.calm  = false;
        s.directionDegrees = deg;
        s.speedHundredthMs = hundredths;
        s.source = src;
        s.recordedMsSinceEpoch = msSinceEpoch;
        s.note = n;
        *out = s;
        return true;
    }

    bool hasReading() const { return valid; }
    bool isCalm()     const { return valid && calm; }
    bool isMeasured() const { return valid && !calm; }

    // The ONLY value an operator or report may see. The raw hundredths field
    // is storage, never presentation.
    double speedMetresPerSecond() const
    {
        return hundredthsToMetresPerSecond(speedHundredthMs);
    }

    WindSector sector() const { return windSectorOfDegrees(directionDegrees); }

    // Band derivation. Calm is a band only when calm was explicitly recorded —
    // a measured 0.00 m/s is a Light reading of zero, not Calm.
    WindSpeedBand band() const
    {
        if (calm) return WindSpeedBand::Calm;
        if (speedHundredthMs <= 200)  return WindSpeedBand::Light;       // <= 2.00
        if (speedHundredthMs <= 400)  return WindSpeedBand::Moderate;    // <= 4.00
        if (speedHundredthMs <= 700)  return WindSpeedBand::Strong;      // <= 7.00
        return WindSpeedBand::VeryStrong;
    }

    // Structural validity, independent of how the snapshot was built — this is
    // what the event validators and the reducer check.
    bool isStructurallyValid() const
    {
        if (!valid)
            return !calm && directionDegrees == 0 && speedHundredthMs == 0;
        if (calm)
            return directionDegrees == 0 && speedHundredthMs == 0;   // rule 4
        return directionDegrees >= 0 && directionDegrees < 360
            && speedHundredthMs >= 0 && speedHundredthMs <= kMaxWindSpeedHundredthMs;
    }

    bool operator==(const WindConditionSnapshot& o) const
    {
        return valid == o.valid && calm == o.calm
            && directionDegrees == o.directionDegrees
            && speedHundredthMs == o.speedHundredthMs
            && source == o.source
            && recordedMsSinceEpoch == o.recordedMsSinceEpoch
            && note == o.note;
    }
    bool operator!=(const WindConditionSnapshot& o) const { return !(*this == o); }
};

// ── Session phase ───────────────────────────────────────────────────────────
// STAGE 5 CORRECTION. Stage 3 declared a coarse four-value phase
// (Setup/Recording/Review/Completed) and nothing carried it into the journal.
// That is not sufficient: after a crash, "Recording" cannot tell a SIGHTER
// phase from a COUNTED phase, and deriving it from the recorded shots is
// ambiguous exactly when it matters — counted shots begun but none fired yet
// would resume as Sighters and the next real shot would be misclassified as a
// sighter. The phase is therefore an explicit, durable value carried by
// WindMapPhaseChanged and projected as SessionState::wmPhase.
//
// Idle is a controller-only value: it means "no session", so it is never
// journalled and never appears in a projection of an active session.
enum class WindMapPhase : qint8 {
    Idle           = 0,
    Setup          = 1,   // configured, condition may be set, no shots yet
    Sighters       = 2,   // sighters — recorded, never counted in statistics
    CountedShots   = 3,   // the counted shots of the current position
    PositionReview = 4,   // 3P only — between positions
    SessionReview  = 5,   // capture finished, reviewing before completion
    Completed      = 6,
};

inline bool isJournalledWindMapPhase(qint8 raw)
{
    return raw >= static_cast<qint8>(WindMapPhase::Setup)
        && raw <= static_cast<qint8>(WindMapPhase::Completed);
}

inline QString windMapPhaseName(WindMapPhase p)
{
    switch (p) {
    case WindMapPhase::Idle:           return QStringLiteral("Idle");
    case WindMapPhase::Setup:          return QStringLiteral("Setup");
    case WindMapPhase::Sighters:       return QStringLiteral("Sighters");
    case WindMapPhase::CountedShots:   return QStringLiteral("Counted shots");
    case WindMapPhase::PositionReview: return QStringLiteral("Position review");
    case WindMapPhase::SessionReview:  return QStringLiteral("Session review");
    case WindMapPhase::Completed:      return QStringLiteral("Completed");
    }
    return QString();
}

// The ONLY legal transitions. Everything else fails CLOSED — an unknown or
// out-of-order transition is refused, never clamped to the nearest legal one.
//
// PositionReview exists only in 3P; the caller supplies is3P so a Prone
// session cannot enter it. Completed is terminal.
inline bool windMapTransitionAllowed(WindMapPhase from, WindMapPhase to, bool is3P)
{
    if (from == to) return false;
    switch (from) {
    case WindMapPhase::Idle:
        return to == WindMapPhase::Setup;
    case WindMapPhase::Setup:
        return to == WindMapPhase::Sighters || to == WindMapPhase::CountedShots;
    case WindMapPhase::Sighters:
        return to == WindMapPhase::CountedShots;
    case WindMapPhase::CountedShots:
        return (is3P && to == WindMapPhase::PositionReview)
            || to == WindMapPhase::SessionReview;
    case WindMapPhase::PositionReview:
        // Next position restarts at its own sighters (or straight to counted).
        return is3P && (to == WindMapPhase::Sighters
                     || to == WindMapPhase::CountedShots
                     || to == WindMapPhase::SessionReview);
    case WindMapPhase::SessionReview:
        return to == WindMapPhase::Completed;
    case WindMapPhase::Completed:
        return false;   // terminal
    }
    return false;
}

} // namespace training
} // namespace ta
