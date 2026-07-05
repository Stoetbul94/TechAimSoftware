#include "tachusshotbuilder.h"

namespace techaim {
namespace bridge {

using analytics::PositionType;
using analytics::ShotAnalyticsData;

DisciplineInfo resolveDiscipline(int gameRange, int gameMode, int gameSubMode)
{
    DisciplineInfo d;
    if (gameRange == 10) {
        // 10 m air disciplines.
        d.singlePosition = (gameMode == 0) ? PositionType::AirPistol : PositionType::AirRifle;
    } else if (gameRange == 50) {
        if (gameMode == 1) {                 // 50 m rifle
            if (gameSubMode == 1) {          // 3 positions
                d.is3P = true;
            } else {                         // prone
                d.singlePosition = PositionType::Prone;
            }
        }
        // 50 m pistol is not modelled by a body position -> stays Unknown.
    }
    return d;
}

std::vector<ShotAnalyticsData> buildShots(const MatchArrays& a)
{
    std::vector<ShotAnalyticsData> out;
    const int n = static_cast<int>(a.scores.size());
    if (n <= 0) return out;
    out.reserve(static_cast<size_t>(n));

    const bool hasCoords = (static_cast<int>(a.xs.size()) == n)
                        && (static_cast<int>(a.ys.size()) == n);

    // Timing: build cumulative timestamps only if every needed interval is valid.
    const bool haveIntervalArray = (static_cast<int>(a.intervals.size()) == n);
    bool timingOk = haveIntervalArray && n >= 1;
    std::vector<double> ts;
    if (timingOk) {
        ts.assign(static_cast<size_t>(n), 0.0);
        double t = 0.0;
        for (int i = 1; i < n; ++i) {        // intervals[0] ignored (first shot anchors t=0)
            const double iv = a.intervals[static_cast<size_t>(i)];
            if (iv <= 0.0) { timingOk = false; break; }   // missing/invalid -> no fabrication
            t += iv;
            ts[static_cast<size_t>(i)] = t;
        }
    }

    // FALLBACK ONLY: 3P equal-thirds split in K -> P -> S order. Replace with a
    // real position-per-shot signal from the match state machine when available.
    const int block = a.is3P ? (n / 3) : 0;

    for (int i = 0; i < n; ++i) {
        ShotAnalyticsData s;
        s.shotNumber = i + 1;
        s.seriesNumber = (a.shotsPerSeries > 0) ? (i / a.shotsPerSeries) + 1 : 1;
        s.decimalScore = a.scores[static_cast<size_t>(i)];

        if (hasCoords) {
            s.x = a.xs[static_cast<size_t>(i)];
            s.y = a.flipY ? -a.ys[static_cast<size_t>(i)] : a.ys[static_cast<size_t>(i)];
        }

        if (timingOk) {
            s.timestamp = ts[static_cast<size_t>(i)];
            s.hasTimestamp = true;
        }

        if (a.is3P) {
            s.positionType = (i < block)         ? PositionType::Kneeling
                           : (i < 2 * block)     ? PositionType::Prone
                                                 : PositionType::Standing;
        } else {
            s.positionType = a.singlePosition;
        }

        s.isValid = true;
        s.isSighter = false;
        s.isCompetitionShot = true;
        out.push_back(s);
    }
    return out;
}

} // namespace bridge
} // namespace techaim
