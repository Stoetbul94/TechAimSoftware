#include "GroupPatternAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace ta {
namespace training {

QString confidenceLabel(PatternConfidence c)
{
    switch (c) {
    case PatternConfidence::Insufficient: return QStringLiteral("insufficient");
    case PatternConfidence::Low:          return QStringLiteral("low");
    case PatternConfidence::Moderate:     return QStringLiteral("moderate");
    case PatternConfidence::Strong:       return QStringLiteral("strong");
    }
    return QStringLiteral("low");
}

namespace {

double mean(const QVector<double>& v)
{
    if (v.isEmpty()) return 0.0;
    double s = 0.0; for (double x : v) s += x; return s / v.size();
}

double sampleSd(const QVector<double>& v, double m)
{
    if (v.size() < 2) return 0.0;
    double ss = 0.0; for (double x : v) { const double d = x - m; ss += d * d; }
    return std::sqrt(ss / (v.size() - 1));
}

double median(QVector<double> v)
{
    if (v.isEmpty()) return 0.0;
    std::sort(v.begin(), v.end());
    const int n = v.size();
    return (n % 2 == 1) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// confidence from an evidence ratio r (>= 1) and the sample size n.
PatternConfidence conf(double r, int n)
{
    if (n < kMinShots) return PatternConfidence::Insufficient;
    if (r >= 2.5 && n >= 8) return PatternConfidence::Strong;
    if (r >= 1.8) return PatternConfidence::Moderate;
    return PatternConfidence::Low;
}

} // namespace

GroupPatternResult analyzeGroup(const QVector<double>& xs, const QVector<double>& ys,
                                double ringSpacingMm)
{
    GroupPatternResult r;
    const int n = std::min(xs.size(), ys.size());
    r.shotCount = n;
    if (ringSpacingMm <= 0.0) ringSpacingMm = 2.5;
    if (n < kMinShots) {
        r.hasData = false;
        r.properties.append({ QStringLiteral("insufficient"),
                              QStringLiteral("Not enough shots for a group pattern"),
                              QStringLiteral("A group pattern needs at least %1 counted shots (%2 available).")
                                  .arg(kMinShots).arg(n),
                              PatternConfidence::Insufficient });
        return r;
    }
    r.hasData = true;

    // ── MPI + spreads ───────────────────────────────────────────────────
    r.mpiXMm = mean(xs);
    r.mpiYMm = mean(ys);
    r.hSpreadMm = sampleSd(xs, r.mpiXMm);
    r.vSpreadMm = sampleSd(ys, r.mpiYMm);
    r.centreOffsetMm = std::sqrt(r.mpiXMm * r.mpiXMm + r.mpiYMm * r.mpiYMm);

    // radius (mean dist from MPI) + diameter (extreme spread) + per-shot dist
    QVector<double> dist; dist.reserve(n);
    double sumR = 0.0, maxPair = 0.0;
    for (int i = 0; i < n; ++i) {
        const double d = std::sqrt((xs[i] - r.mpiXMm) * (xs[i] - r.mpiXMm)
                                 + (ys[i] - r.mpiYMm) * (ys[i] - r.mpiYMm));
        dist.append(d); sumR += d;
    }
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            const double d = std::sqrt((xs[i] - xs[j]) * (xs[i] - xs[j])
                                     + (ys[i] - ys[j]) * (ys[i] - ys[j]));
            if (d > maxPair) maxPair = d;
        }
    r.radiusMm = sumR / n;
    r.diameterMm = maxPair;

    // ── covariance / principal axis ─────────────────────────────────────
    double sxx = 0.0, syy = 0.0, sxy = 0.0;
    for (int i = 0; i < n; ++i) {
        const double dx = xs[i] - r.mpiXMm, dy = ys[i] - r.mpiYMm;
        sxx += dx * dx; syy += dy * dy; sxy += dx * dy;
    }
    sxx /= n; syy /= n; sxy /= n;
    // eigenvalues of the 2x2 covariance
    const double tr = sxx + syy;
    const double det = sxx * syy - sxy * sxy;
    const double disc = std::sqrt(std::max(0.0, tr * tr / 4.0 - det));
    const double l1 = tr / 2.0 + disc;   // major
    const double l2 = tr / 2.0 - disc;   // minor
    // principal-axis angle
    double angle = 0.0;
    if (std::fabs(sxy) > 1e-9 || std::fabs(sxx - syy) > 1e-9)
        angle = 0.5 * std::atan2(2.0 * sxy, sxx - syy) * 180.0 / M_PI;
    r.axisAngleDeg = std::fabs(angle);   // 0 horizontal … 90 vertical
    const double anisotropy = (l2 > 1e-9) ? std::sqrt(l1 / l2) : (l1 > 1e-9 ? 99.0 : 1.0);

    const double minSpread = std::max(1e-6, std::min(r.hSpreadMm, r.vSpreadMm));
    r.hvRatio = std::max(r.hSpreadMm, r.vSpreadMm) / minSpread;

    // ── ordered drift (early → late MPI) ────────────────────────────────
    const int third = std::max(1, n / 3);
    double ex = 0, ey = 0, lx = 0, ly = 0;
    for (int i = 0; i < third; ++i) { ex += xs[i]; ey += ys[i]; }
    for (int i = n - third; i < n; ++i) { lx += xs[i]; ly += ys[i]; }
    ex /= third; ey /= third; lx /= third; ly /= third;
    r.driftMm = std::sqrt((lx - ex) * (lx - ex) + (ly - ey) * (ly - ey));

    // ── expansion (late vs early mean radius) ───────────────────────────
    const int half = std::max(1, n / 2);
    double er = 0, lr = 0;
    for (int i = 0; i < half; ++i) er += dist[i];
    for (int i = n - half; i < n; ++i) lr += dist[i];
    er /= half; lr /= half;
    r.expansionRatio = (er > 1e-6) ? lr / er : 1.0;

    // ── robust isolated-outlier detection (MAD of per-shot distance) ────
    const double medDist = median(dist);
    QVector<double> absdev; absdev.reserve(n);
    for (double d : dist) absdev.append(std::fabs(d - medDist));
    const double mad = median(absdev);
    double maxDist = 0.0, secondMax = 0.0;
    for (double d : dist) { if (d > maxDist) { secondMax = maxDist; maxDist = d; } else if (d > secondMax) secondMax = d; }
    const bool isolated = (mad > 1e-6) && (maxDist > medDist + 3.5 * mad) && (maxDist > 1.8 * std::max(1e-6, secondMax));

    // ── two-cluster separation (largest gap along the principal axis) ───
    QVector<double> proj; proj.reserve(n);
    const double ca = std::cos(angle * M_PI / 180.0), sa = std::sin(angle * M_PI / 180.0);
    for (int i = 0; i < n; ++i) proj.append((xs[i] - r.mpiXMm) * ca + (ys[i] - r.mpiYMm) * sa);
    std::sort(proj.begin(), proj.end());
    double maxGap = 0.0; int gapIdx = -1;
    QVector<double> gaps; gaps.reserve(n - 1);
    for (int i = 1; i < n; ++i) {
        const double g = proj[i] - proj[i - 1];
        gaps.append(g);
        if (g > maxGap) { maxGap = g; gapIdx = i; }
    }
    // bimodality: the largest gap dwarfs the typical spacing AND both sides hold
    // at least two shots (a robust two-cluster test that does not depend on the
    // overall SD, which a symmetric bimodal set inflates).
    const double medGap = std::max(1e-6, median(gaps));
    const bool twoClusters = (n >= 6) && (maxGap > 4.0 * medGap)
                             && gapIdx >= 2 && (n - gapIdx) >= 2;

    // ── property assembly (measured only, no cause) ─────────────────────
    const double ring = ringSpacingMm;
    auto ringsPhrase = [ring](double mm) {
        return QStringLiteral("%1 mm (~%2 ring spacings)").arg(mm, 0, 'f', 1).arg(mm / ring, 0, 'f', 1);
    };

    // isolated outlier (usually a strong, specific finding)
    if (isolated) {
        r.properties.append({ QStringLiteral("isolated_outlier"),
            QStringLiteral("One shot isolated from the group"),
            QStringLiteral("One shot was %1 from the group centre, well beyond the others (typical %2).")
                .arg(ringsPhrase(maxDist)).arg(ringsPhrase(medDist)),
            conf(maxDist / std::max(1e-6, medDist), n) });
    }
    // two clusters
    if (twoClusters) {
        r.properties.append({ QStringLiteral("two_clusters"),
            QStringLiteral("Two visible clusters"),
            QStringLiteral("The shots separated into two groups with a %1 gap along the main axis.")
                .arg(ringsPhrase(maxGap)),
            conf(maxGap / medGap, n) });
    }
    // directional string (H / V / diagonal) when clearly anisotropic
    if (r.hvRatio >= 1.6 || anisotropy >= 1.6) {
        QString key, label;
        if (r.axisAngleDeg <= 25.0) { key = QStringLiteral("horizontal_string"); label = QStringLiteral("Horizontal spread dominant"); }
        else if (r.axisAngleDeg >= 65.0) { key = QStringLiteral("vertical_string"); label = QStringLiteral("Vertical spread dominant"); }
        else { key = QStringLiteral("diagonal_string"); label = QStringLiteral("Diagonal spread dominant"); }
        r.properties.append({ key, label,
            QStringLiteral("Spread was %1× greater along one axis (H %2 mm, V %3 mm).")
                .arg(std::max(r.hvRatio, anisotropy), 0, 'f', 1).arg(r.hSpreadMm, 0, 'f', 1).arg(r.vSpreadMm, 0, 'f', 1),
            conf(std::max(r.hvRatio, anisotropy), n) });
    }
    // progressive drift through the block
    if (r.driftMm >= 0.75 * ring && r.driftMm >= 0.6 * r.radiusMm) {
        r.properties.append({ QStringLiteral("progressive_drift"),
            QStringLiteral("Group centre drifted through the block"),
            QStringLiteral("The group centre moved %1 from the first third to the last third (%2 %3).")
                .arg(ringsPhrase(r.driftMm))
                .arg(std::fabs(lx - ex) >= std::fabs(ly - ey) ? (lx >= ex ? "rightward" : "leftward")
                                                              : (ly >= ey ? "upward" : "downward"))
                .arg(QStringLiteral("overall")),
            conf(r.driftMm / std::max(1e-6, r.radiusMm) + 1.0, n) });
    }
    // group expansion / contraction through the block
    if (r.expansionRatio >= 1.5) {
        r.properties.append({ QStringLiteral("group_expansion"),
            QStringLiteral("Group widened later in the block"),
            QStringLiteral("Later shots sat %1× further from the centre than early shots.")
                .arg(r.expansionRatio, 0, 'f', 1),
            conf(r.expansionRatio, n) });
    } else if (r.expansionRatio <= 0.66 && r.expansionRatio > 0.0) {
        r.properties.append({ QStringLiteral("group_contraction"),
            QStringLiteral("Group tightened later in the block"),
            QStringLiteral("Later shots sat closer to the centre than early shots (ratio %1).")
                .arg(r.expansionRatio, 0, 'f', 2),
            conf(1.0 / std::max(1e-6, r.expansionRatio), n) });
    }

    // baseline shape (always present): tight/wide × centred/offset
    const bool tight = (r.diameterMm <= 2.0 * ring);
    const bool centred = (r.centreOffsetMm <= 0.5 * ring);
    if (r.properties.isEmpty() || tight || (!isolated && !twoClusters)) {
        QString key, label, ev;
        if (tight && centred) {
            key = QStringLiteral("tight_centred"); label = QStringLiteral("Tight and centred group");
            ev = QStringLiteral("Group diameter %1, centred within %2.")
                    .arg(ringsPhrase(r.diameterMm)).arg(ringsPhrase(r.centreOffsetMm));
        } else if (tight && !centred) {
            key = QStringLiteral("tight_offset"); label = QStringLiteral("Tight but offset group");
            ev = QStringLiteral("Group diameter %1, centre offset %2 from the middle.")
                    .arg(ringsPhrase(r.diameterMm)).arg(ringsPhrase(r.centreOffsetMm));
        } else {
            key = QStringLiteral("wide_group"); label = QStringLiteral("Wide group");
            ev = QStringLiteral("Group diameter %1 with no single dominant direction.")
                    .arg(ringsPhrase(r.diameterMm));
        }
        r.properties.append({ key, label, ev, PatternConfidence::Moderate });
    }

    // primary first — sort by confidence (stable)
    std::stable_sort(r.properties.begin(), r.properties.end(),
                     [](const GroupProperty& a, const GroupProperty& b) {
                         return static_cast<int>(a.confidence) > static_cast<int>(b.confidence);
                     });
    return r;
}

}} // namespace ta::training
