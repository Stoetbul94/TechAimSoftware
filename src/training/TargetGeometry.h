#ifndef TA_TRAINING_TARGETGEOMETRY_H
#define TA_TRAINING_TARGETGEOMETRY_H

// Training Lab — authoritative ISSF target geometry (mm) for the analysis
// layers (Call & Diagnose ring-spacing normalisation, adaptive comparison
// bounds; Group Pattern Coach ring context). Values derive from the target
// FACE widths the scoring pipeline already uses (CenterPane: 10m rifle 45.5,
// 10m pistol 155.5, 50m rifle 154.4 mm across) and the ISSF ten-ring radii:
//   ring spacing (radial) = (faceRadius − tenRingRadius) / 9
//     10m Air Rifle : (22.75 − 0.25)/9 ≈ 2.5 mm
//     10m Air Pistol: (77.75 − 5.75)/9 ≈ 8.0 mm
//     50m Rifle     : (77.20 − 5.20)/9 ≈ 8.0 mm
// These are nominal ISSF figures; a physical calibration may refine them
// (cf. the 50m radOf10Ring note in CLAUDE.md). Kept in ONE place so QML never
// recomputes ring geometry.

#include "reliability/events/EventTypes.h"   // ta::rel::Discipline

namespace ta {
namespace training {

// Radial distance (mm) between consecutive scoring rings for the discipline.
inline double issfRingSpacingMm(ta::rel::Discipline d)
{
    switch (d) {
    case ta::rel::Discipline::AirRifle10m:      return 2.5;
    case ta::rel::Discipline::AirPistol10m:     return 8.0;
    case ta::rel::Discipline::Prone50m:         return 8.0;
    case ta::rel::Discipline::ThreePositions50m:return 8.0;
    default:                                    return 2.5;   // safe non-zero
    }
}

// Outer radius (mm) of the scoring face (ring 1 outer edge).
inline double issfFaceRadiusMm(ta::rel::Discipline d)
{
    switch (d) {
    case ta::rel::Discipline::AirRifle10m:      return 22.75;
    case ta::rel::Discipline::AirPistol10m:     return 77.75;
    case ta::rel::Discipline::Prone50m:         return 77.20;
    case ta::rel::Discipline::ThreePositions50m:return 77.20;
    default:                                    return 25.0;
    }
}

}} // namespace ta::training

#endif // TA_TRAINING_TARGETGEOMETRY_H
