#pragma once

// ============================================================================
//  TechAim Coach Analytics — core shot types (Phase 1)
// ----------------------------------------------------------------------------
//  Pure, offline C++. No Qt, no QML, no networking, no AI. Everything here is
//  deterministic and depends only on real shot data so that every downstream
//  conclusion can be traced back to a measurable value.
//
//  Coordinate convention (target face, millimetres):
//      x : negative = left,  positive = right
//      y : negative = low,   positive = high
//      centre of target = (0, 0)
//      radial distance from centre = sqrt(x*x + y*y)
// ============================================================================

#include <string>
#include <vector>

namespace techaim {
namespace analytics {

// ---- Discipline / body position of a shot ---------------------------------
enum class PositionType {
    Prone,
    Kneeling,
    Standing,
    AirRifle,
    AirPistol,
    Unknown
};

std::string toString(PositionType p);
PositionType positionFromString(const std::string& s);

// ---- Overall performance trend --------------------------------------------
enum class TrendDirection {
    Improving,
    Stable,
    Declining
};

std::string toString(TrendDirection t);

// ---- Simple 2D point (target-face millimetres) ----------------------------
struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

// ---- One shot, as handed to the analytics engine --------------------------
//  Callers set the *has* flags for optional fields so the engine can honestly
//  report "unavailable" instead of fabricating zeros.
struct ShotAnalyticsData {
    int    shotNumber       = 0;      // 1-based order within the session
    int    seriesNumber     = 0;      // 1-based series index (0 if unknown)

    double decimalScore     = 0.0;    // e.g. 10.4
    int    integerScore     = 0;      // e.g. 10
    bool   hasIntegerScore  = false;

    double x                = 0.0;    // target-face mm, +right
    double y                = 0.0;    // target-face mm, +high

    double timestamp        = 0.0;    // seconds (any monotonic origin)
    bool   hasTimestamp     = false;

    PositionType positionType = PositionType::Unknown;

    bool   isValid          = true;   // false => dropped in validation
    bool   isSighter        = false;  // excluded from official analysis by default
    bool   isCompetitionShot= true;   // record-fire shot

    double penalty          = 0.0;    // manual correction / penalty (score units)
    bool   hasPenalty       = false;
};

} // namespace analytics
} // namespace techaim
