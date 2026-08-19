#include "TargetGeometry.h"

#include <QtMath>

namespace ta {
namespace rms {

namespace {

// The four target standards CompetitionCatalogue.qml actually names. The ring
// figures mirror IssfTargetCanvas.qml in the foundation, so RMS draws the same
// faces the target application draws.
//
// NOTHING HERE SCORES. These radii place a marker; they never value one.
struct Entry {
    const char* id;
    const char* name;
    double tenRingRadiusMm;
    double ringStepMm;
    int    outermostRing;
    int    blackRing;
};

const Entry kStandards[] = {
    // 10 m air rifle: a 0.5 mm ten ring, 2.5 mm steps, black out to the 4.
    { "issf.10m.air-rifle",  "10 m Air Rifle",  0.25, 2.5, 4, 4 },
    // 10 m air pistol: 11.5 mm ten ring, 8 mm steps, black out to the 7.
    { "issf.10m.air-pistol", "10 m Air Pistol", 5.75, 8.0, 4, 7 },
    // 50 m rifle. The 5.2 mm ten ring is the value the foundation renders and
    // carries the open confirmation recorded in CLAUDE.md.
    { "issf.50m.rifle",      "50 m Rifle",      5.2,  8.0, 4, 5 },
    // 50 m pistol shares the 50 m face geometry in the foundation renderer.
    { "issf.50m.pistol",     "50 m Pistol",     5.2,  8.0, 4, 5 }
};

} // namespace

TargetSpec TargetGeometry::specFor(const QString& targetStandardId)
{
    for (const Entry& e : kStandards) {
        if (targetStandardId != QLatin1String(e.id))
            continue;
        TargetSpec s;
        s.targetStandardId  = targetStandardId;
        s.displayName       = QString::fromLatin1(e.name);
        s.tenRingRadiusMm   = e.tenRingRadiusMm;
        s.ringStepMm        = e.ringStepMm;
        s.outermostRing     = e.outermostRing;
        s.blackRing         = e.blackRing;
        s.supported         = true;
        return s;
    }

    // Unknown. Deliberately NOT a default face: drawing a 10 m target for a
    // 50 m shot would misplace every marker and look completely convincing.
    TargetSpec unknown;
    unknown.targetStandardId = targetStandardId;
    unknown.displayName = targetStandardId.isEmpty()
                              ? QStringLiteral("No target standard reported")
                              : targetStandardId;
    unknown.supported = false;
    return unknown;
}

bool TargetGeometry::isSupported(const QString& targetStandardId)
{
    return specFor(targetStandardId).supported;
}

QStringList TargetGeometry::supportedStandards()
{
    QStringList out;
    for (const Entry& e : kStandards)
        out << QString::fromLatin1(e.id);
    return out;
}

QPointF TargetGeometry::normalise(const TargetSpec& spec, double xMm, double yMm)
{
    const double face = spec.faceRadiusMm();
    if (!spec.supported || face <= 0.0)
        return QPointF(0.0, 0.0);
    // y flips here, once, so no caller has to remember to.
    const double ny = spec.yAxisUp ? -yMm : yMm;
    return QPointF(xMm / face, ny / face);
}

bool TargetGeometry::isWithinFace(const TargetSpec& spec, double xMm, double yMm)
{
    const double face = spec.faceRadiusMm();
    if (!spec.supported || face <= 0.0)
        return false;
    return std::hypot(xMm, yMm) <= face;
}

QPointF TargetGeometry::normaliseClamped(const TargetSpec& spec, double xMm, double yMm)
{
    const QPointF n = normalise(spec, xMm, yMm);
    const double r = std::hypot(n.x(), n.y());
    if (r <= 1.0 || r <= 0.0)
        return n;
    // Held at the edge rather than dropped. A wild shot or a cross-fire is
    // precisely what an operator needs to see; it is flagged by
    // isWithinFace(), not hidden by clipping it away.
    return QPointF(n.x() / r, n.y() / r);
}

QPointF TargetGeometry::toView(const QPointF& normalised, double sizePx, double marginPx)
{
    const double radius = qMax(0.0, sizePx / 2.0 - marginPx);
    const double centre = sizePx / 2.0;
    return QPointF(centre + normalised.x() * radius,
                   centre + normalised.y() * radius);
}

QVariantMap TargetGeometryBridge::specFor(const QString& targetStandardId) const
{
    const TargetSpec s = TargetGeometry::specFor(targetStandardId);
    QVariantMap m;
    m[QStringLiteral("targetStandardId")] = s.targetStandardId;
    m[QStringLiteral("supported")]        = s.supported;
    m[QStringLiteral("displayName")]      = s.displayName;
    if (!s.supported)
        return m;

    const double face = s.faceRadiusMm();
    m[QStringLiteral("faceRadiusMm")]        = face;
    m[QStringLiteral("outermostRing")]       = s.outermostRing;
    m[QStringLiteral("blackRing")]           = s.blackRing;
    m[QStringLiteral("blackRadiusFraction")] = s.blackRadiusMm() / face;

    // One entry per drawn ring, outermost first, as a fraction of the face.
    QVariantList rings;
    for (int ring = s.outermostRing; ring <= 10; ++ring) {
        QVariantMap r;
        r[QStringLiteral("ring")]     = ring;
        r[QStringLiteral("fraction")] = s.ringRadiusMm(ring) / face;
        r[QStringLiteral("inBlack")]  = s.ringRadiusMm(ring) <= s.blackRadiusMm();
        rings.append(r);
    }
    m[QStringLiteral("rings")] = rings;
    return m;
}

bool TargetGeometryBridge::isSupported(const QString& targetStandardId) const
{
    return TargetGeometry::isSupported(targetStandardId);
}

QStringList TargetGeometryBridge::supportedStandards() const
{
    return TargetGeometry::supportedStandards();
}

} // namespace rms
} // namespace ta
