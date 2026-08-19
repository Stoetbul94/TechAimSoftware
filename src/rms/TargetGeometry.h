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
// The OFFICIAL RULEBOOK, not another renderer. ISSF Rule Book 2026, EDITION
// 2025 (Second Print 07/2026), effective 1 July 2026, rule 6.3.4 for the
// faces and rules 7.4.6 / 8.4.4 for the ammunition. Every value is cited with
// its rule, page and tolerance in
// docs/architecture/rms-target-geometry-source-register.md.
//
// This file previously mirrored `IssfTargetCanvas.qml` instead. That is how it
// came to draw a 50 m RIFLE face for 50 m pistol — the foundation renderer has
// no pistol entry and falls through to its rifle default. Mirroring a renderer
// copies its mistakes; citing the rulebook does not.
//
// The 50 m ten ring is 10.4 mm DIAMETER (rule 6.3.4.2), which confirms the
// project's long-standing 5.2 mm RADIUS as correct.
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

    // ── OFFICIAL VALUES, ALL DIAMETERS ───────────────────────────────────
    // The rulebook states DIAMETERS, so this struct stores diameters and
    // converts. A diameter silently used as a radius is the classic 2x error
    // in target software, and the only defence is to never let the two share
    // a name.
    double  tenRingDiameterMm = 0.0;
    // Diameter difference between consecutive ring edges.
    double  ringStepDiameterMm = 0.0;
    // Outer edge of the black aiming mark, as a DIAMETER. Not a ring index:
    // the 50 m rifle's black falls BETWEEN rings (rule 6.3.4.2 says "part of
    // 3"), so a ring number cannot express it.
    double  blackDiameterMm = 0.0;
    // Inner ten, as a DIAMETER. Zero when the discipline does not define one
    // by dimension - 10 m Air Rifle defines it by gauge outcome instead.
    double  innerTenDiameterMm = 0.0;
    // The projectile, from the ammunition rules. Used to draw the hole at its
    // true size; never to decide what the hole is worth.
    double  projectileDiameterMm = 0.0;

    // ── DRAWING CHOICES ──────────────────────────────────────────────────
    // Lowest-numbered ring DRAWN. Real cards carry rings out to 1; a lane tile
    // that drew all of them would waste most of its area on rings a match
    // rarely touches, so the face is cropped here and the crop is a display
    // decision, not a claim about the card.
    int     outermostRing = 4;
    // Telemetry y is positive upwards; screens are positive downwards.
    bool    yAxisUp = true;
    bool    supported = false;

    // ── DERIVED, IN RADII ────────────────────────────────────────────────
    double ringDiameterMm(int ring) const
    {
        return tenRingDiameterMm + double(10 - ring) * ringStepDiameterMm;
    }
    double ringRadiusMm(int ring) const { return ringDiameterMm(ring) / 2.0; }
    double faceRadiusMm() const  { return ringRadiusMm(outermostRing); }
    double blackRadiusMm() const { return blackDiameterMm / 2.0; }
    double innerTenRadiusMm() const { return innerTenDiameterMm / 2.0; }
    double projectileRadiusMm() const { return projectileDiameterMm / 2.0; }
    bool hasInnerTen() const { return innerTenDiameterMm > 0.0; }
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

    // { supported, displayName, faceRadiusMm, outermostRing,
    //   blackRadiusFraction, projectileRadiusFraction, projectileDiameterMm,
    //   hasInnerTen, innerTenRadiusFraction, rings[{ring, fraction, inBlack}] }
    // Radii are returned as FRACTIONS of the face radius, which is what a
    // renderer needs and keeps millimetre arithmetic out of QML.
    Q_INVOKABLE QVariantMap specFor(const QString& targetStandardId) const;
    Q_INVOKABLE bool isSupported(const QString& targetStandardId) const;
    Q_INVOKABLE QStringList supportedStandards() const;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_TARGETGEOMETRY_H
