#ifndef TACHUSSHOTBUILDER_H
#define TACHUSSHOTBUILDER_H

// ============================================================================
//  Tachus match data -> ShotAnalyticsData (PURE, Qt-free, unit-testable)
// ----------------------------------------------------------------------------
//  Mechanical conversion only: parallel per-shot arrays extracted from the
//  TachusWidget game-mode history become the engine's ShotAnalyticsData. This
//  layer is deliberately Qt-free and widget-free so it can be unit-tested with
//  g++ alone (the widget itself needs MainWindow + hardware to instantiate).
//
//  DOCUMENTED ASSUMPTIONS (surfaced for review — correctness depends on these):
//    - Coordinates are passed through as target-face units and treated as mm by
//      the engine. If the hardware unit differs by a constant scale, only the
//      absolute mm / heat-map-bin labels shift; the analytical conclusions
//      (scores, patterns, relative geometry) are unaffected.
//    - Coordinate convention is assumed to match the engine: +x right, +y high.
//      A y-orientation flip, if the hardware uses +y down, belongs HERE (set
//      flipY) — not in the engine.
//    - Timing is built from per-shot "time consumed" values (interval to each
//      shot); the first shot anchors t=0. If any interval is missing/invalid,
//      timing is reported unavailable rather than fabricated.
//    - 3-position (3P) shots are split into equal thirds in K -> P -> S order
//      (the codebase's confirmed 3P state machine). Single-position disciplines
//      carry one resolved position for every shot.
// ============================================================================

#include <vector>
#include "ShotAnalyticsTypes.h"

namespace techaim {
namespace bridge {

// Parallel per-shot arrays (index 0 == shot 1), as read from the game-mode
// history. `xs`/`ys` empty (or size != scores) => no coordinates. `intervals`
// empty (or size != scores) => no timing. `intervals[i]` is the seconds taken
// for shot i (time consumed); intervals[0] is ignored (first shot anchors t=0).
struct MatchArrays {
    std::vector<double> scores;
    std::vector<double> xs;
    std::vector<double> ys;
    std::vector<double> intervals;
    int  shotsPerSeries = 10;
    bool is3P           = false;
    bool flipY          = false;   // set true if the source uses +y = down
    analytics::PositionType singlePosition = analytics::PositionType::Unknown;
};

// Resolved discipline (position handling) from the login-page selections.
struct DisciplineInfo {
    bool is3P = false;
    analytics::PositionType singlePosition = analytics::PositionType::Unknown;
};

// gameRange: 10 | 50   gameMode: 0 = pistol, 1 = rifle   gameSubMode: 0 = prone/air, 1 = 3P
DisciplineInfo resolveDiscipline(int gameRange, int gameMode, int gameSubMode);

// The pure conversion. Never fabricates coordinates or timing.
std::vector<analytics::ShotAnalyticsData> buildShots(const MatchArrays& a);

} // namespace bridge
} // namespace techaim

#endif // TACHUSSHOTBUILDER_H
