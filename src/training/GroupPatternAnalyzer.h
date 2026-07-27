#ifndef TA_TRAINING_GROUPPATTERNANALYZER_H
#define TA_TRAINING_GROUPPATTERNANALYZER_H

// Group Pattern Coach (T3) — pure-C++ analysis layer. It DESCRIBES the measured
// shape and movement of a completed group (per Technical Block, per 3P position)
// in language an athlete and coach can understand. It NEVER claims a technical
// cause (no trigger/breathing/NPA/position fault). Every property carries the
// sample size, measured values, the evidence used and a rule-based confidence.
//
// Requires >= 5 shots (fewer → insufficient). Guards identical points, zero
// spread and division by zero. No QML classification — this is the single
// authoritative implementation; QML only formats the result.

#include <QString>
#include <QVector>

namespace ta {
namespace training {

enum class PatternConfidence { Insufficient = 0, Low = 1, Moderate = 2, Strong = 3 };

// One measured group property (a group may have several).
struct GroupProperty {
    QString key;            // stable id, e.g. "vertical_string"
    QString label;          // human label, e.g. "Vertical spread dominant"
    QString evidence;       // measured evidence sentence (no cause)
    PatternConfidence confidence = PatternConfidence::Low;
};

struct GroupPatternResult {
    int  shotCount = 0;
    bool hasData = false;   // >= kMinShots
    // measured geometry (mm)
    double mpiXMm = 0.0, mpiYMm = 0.0;
    double radiusMm = 0.0;      // mean distance from MPI
    double diameterMm = 0.0;    // extreme spread
    double hSpreadMm = 0.0, vSpreadMm = 0.0;   // sample SD of x / y
    double hvRatio = 1.0;       // max(H,V)/min(H,V)
    double axisAngleDeg = 0.0;  // principal-axis angle (0 = horizontal, 90 = vertical)
    double centreOffsetMm = 0.0;// |MPI|
    double driftMm = 0.0;       // early→late MPI movement
    double expansionRatio = 1.0;// late mean-radius / early mean-radius
    // properties, primary (highest confidence) first
    QVector<GroupProperty> properties;
    QString primaryLabel() const { return properties.isEmpty() ? QString() : properties.front().label; }
};

inline constexpr int kMinShots = 5;

// xs/ys are shot coordinates in mm, IN SHOT ORDER. ringSpacingMm scales the
// tight/wide thresholds to the discipline (never hardcoded per target).
GroupPatternResult analyzeGroup(const QVector<double>& xs, const QVector<double>& ys,
                                double ringSpacingMm);

QString confidenceLabel(PatternConfidence c);

}} // namespace ta::training

#endif // TA_TRAINING_GROUPPATTERNANALYZER_H
