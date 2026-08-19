#ifndef TA_RMS_TARGETGEOMETRY_H
#define TA_RMS_TARGETGEOMETRY_H

// ─────────────────────────────────────────────────────────────────────────────
// TARGET FACE GEOMETRY — enough to DRAW a target and place a shot on it.
//
// ═══ THIS IS NOT SCORING. ══════════════════════════════════════════════════
//
// There is deliberately no function here that takes a coordinate and returns a
// score, and there must never be one. RMS displays the `authoritativeScore`
// the node attached to the shot; the ring radii below exist so a shot lands in
// the right PLACE on the picture, never so a value can be derived from where
// it landed. The node's `CenterPane.qml::calculateShootingSocre()` is the only
// scoring authority in the product family, and a second implementation here
// would be the most dangerous thing this file could contain.
//
// ═══ WHERE THE NUMBERS COME FROM ═══════════════════════════════════════════
//
// The ring specifications match the foundation's own renderer,
// `IssfTargetCanvas.qml`, so RMS draws the same faces the target application
// draws. They are not re-derived here.
//
// The 50 m ten-ring radius (5.2 mm) carries the open item already recorded in
// CLAUDE.md — it awaits official rulebook confirmation or physical
// calibration. It is used for DRAWING only, so an error there moves a marker
// slightly; it can never move a score.
//
// ═══ AXIS CONVENTION ═══════════════════════════════════════════════════════
//
// Telemetry x/y are millimetres from centre with **y positive upwards**. That
// is what the foundation's renderers assume (`py = cy - scale * y` in both
// IssfTargetCanvas.qml and FinalsReportTarget.qml), so RMS follows it rather
// than inventing a second convention.
//
// The hardware-level confirmation of that sign is still an open question in
// the project's notes. It is worth one deliberate check at the field test:
// fire a high shot and confirm it renders high. `yAxisUp` exists so the answer
// is a one-line change and not an archaeology exercise.
// ─────────────────────────────────────────────────────────────────────────────

#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace ta {
namespace rms {

struct TargetSpec {
    QString targetStandardId;
    QString displayName;
    // Outer radius of the 10 ring, in millimetres.
    double  tenRingRadiusMm = 0.0;
    // Radial step between consecutive ring edges, in millimetres.
    double  ringStepMm = 0.0;
    // Lowest-numbered ring drawn — the edge of the printed face.
    int     outermostRing = 4;
    // The ring whose outer edge is the edge of the black aiming mark.
    int     blackRing = 4;
    // Telemetry y is positive upwards; screens are positive downwards.
    bool    yAxisUp = true;
    bool    supported = false;

    double ringRadiusMm(int ring) const
    {
        return tenRingRadiusMm + double(10 - ring) * ringStepMm;
    }
    double faceRadiusMm() const { return ringRadiusMm(outermostRing); }
    double blackRadiusMm() const { return ringRadiusMm(blackRing); }
};

class TargetGeometry
{
public:
    // An unrecognised standard returns a spec with `supported == false`. The
    // display shows a neutral placeholder for it and NEVER falls back to a
    // different target — drawing a 10 m face for a 50 m shot would put every
    // marker in the wrong place and look entirely plausible while doing it.
    static TargetSpec specFor(const QString& targetStandardId);
    static bool isSupported(const QString& targetStandardId);
    static QStringList supportedStandards();

    // Millimetres → normalised face coordinates: centre (0,0), face edge at
    // radius 1.0, y already flipped for screen use. Independent of view size,
    // which is what lets the same shot mean the same thing in a small card and
    // a full-screen view.
    static QPointF normalise(const TargetSpec& spec, double xMm, double yMm);

    // Normalised → pixels within a square view of `sizePx`, inset by `marginPx`.
    static QPointF toView(const QPointF& normalised, double sizePx, double marginPx = 0.0);

    // Whether the shot falls on the printed face at all. A shot outside it is
    // still drawn — clamped to the edge and flagged — because a cross-fire or
    // a wild shot is exactly the thing an operator needs to see, not hide.
    static bool isWithinFace(const TargetSpec& spec, double xMm, double yMm);

    // Normalised, clamped to the face edge for rendering. Returns the
    // unclamped value when it is already inside.
    static QPointF normaliseClamped(const TargetSpec& spec, double xMm, double yMm);
};

// The same geometry, reachable from QML so the renderer draws rings at their
// real radii instead of carrying a second copy of the table that could drift
// from this one.
class TargetGeometryBridge : public QObject
{
    Q_OBJECT
public:
    explicit TargetGeometryBridge(QObject* parent = nullptr) : QObject(parent) {}

    // { supported, displayName, tenRingRadiusMm, ringStepMm, outermostRing,
    //   blackRing, faceRadiusMm, blackRadiusFraction, ringFractions[] }
    // Radii are returned as FRACTIONS of the face radius, which is what a
    // renderer needs and keeps millimetre arithmetic out of QML.
    Q_INVOKABLE QVariantMap specFor(const QString& targetStandardId) const;
    Q_INVOKABLE bool isSupported(const QString& targetStandardId) const;
    Q_INVOKABLE QStringList supportedStandards() const;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_TARGETGEOMETRY_H
