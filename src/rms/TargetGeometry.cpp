#include "TargetGeometry.h"

#include <QtMath>

namespace ta {
namespace rms {

namespace {

// The four target standards CompetitionCatalogue.qml actually names. The ring
// figures mirror IssfTargetCanvas.qml in the foundation, so RMS draws the same
// faces the target application draws.
//
// NOTHING HERE SCORES. These dimensions place a marker; they never value one.
//
// EVERY NUMBER IS A DIAMETER, READ FROM THE OFFICIAL RULEBOOK.
// ISSF Rule Book 2026, EDITION 2025 (Second Print 07/2026), effective
// 1 July 2026, rule 6.3.4 "Official ISSF Targets". Ammunition from rules 7.4.6
// (rifle) and 8.4.4 (pistol). Full citation with page numbers and tolerances:
// docs/architecture/rms-target-geometry-source-register.md
struct Entry {
    const char* id;
    const char* name;
    double tenRingDiameterMm;
    double ringStepDiameterMm;
    double blackDiameterMm;
    double innerTenDiameterMm;      // 0 = not defined by dimension
    double projectileDiameterMm;
    int    outermostRing;
};

const Entry kStandards[] = {
    // 6.3.4.3 — 10 ring 0.5, spacing 5.0, black to the 4 ring (30.5).
    // Inner ten is defined by gauge outcome, not by a diameter, so it is 0.
    // The 4.5 mm pellet is NINE TIMES the ten ring across: on this face the
    // hole is a major object, which is why it is drawn at its true size.
    { "issf.10m.air-rifle",  "10 m Air Rifle",   0.5,  5.0,  30.5,  0.0, 4.5, 4 },

    // 6.3.4.6 — 10 ring 11.5, spacing 16.0, black to the 7 ring (59.5).
    { "issf.10m.air-pistol", "10 m Air Pistol", 11.5, 16.0,  59.5,  5.0, 4.5, 4 },

    // 6.3.4.2 — 10 ring 10.4, spacing 16.0. Black is 112.4, which lands
    // BETWEEN the 4 ring (106.4) and the 3 ring (122.4) — the rule says "part
    // of 3". Because the face is cropped at the 4 ring, the whole drawn face
    // is black, and that is correct rather than a mistake.
    { "issf.50m.rifle",      "50 m Rifle",      10.4, 16.0, 112.4,  5.0, 5.6, 4 },

    // 6.3.4.5 — 10 ring 50, spacing 50, black to the 7 ring (200). A
    // COMPLETELY DIFFERENT FACE from the 50 m rifle: five times the ten ring
    // and more than three times the spacing. This entry previously carried a
    // copy of the rifle row, which plotted every 50 m pistol shot at about
    // 4.8x its true radius from centre.
    { "issf.50m.pistol",     "50 m Pistol",     50.0, 50.0, 200.0, 25.0, 5.6, 4 }
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
        s.tenRingDiameterMm    = e.tenRingDiameterMm;
        s.ringStepDiameterMm   = e.ringStepDiameterMm;
        s.blackDiameterMm      = e.blackDiameterMm;
        s.innerTenDiameterMm   = e.innerTenDiameterMm;
        s.projectileDiameterMm = e.projectileDiameterMm;
        s.outermostRing        = e.outermostRing;
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
    m[QStringLiteral("blackRadiusFraction")] = s.blackRadiusMm() / face;
    // The projectile, as a fraction of the face, so the hole can be drawn at
    // its TRUE PHYSICAL SIZE at any view size. On a 10 m air rifle face a
    // 4.5 mm pellet is 0.148 of the face radius; on a 50 m pistol face a
    // 5.6 mm bullet is 0.016 of it. A fixed pixel marker would be wrong on
    // both, and wrong in opposite directions.
    m[QStringLiteral("projectileRadiusFraction")] = s.projectileRadiusMm() / face;
    m[QStringLiteral("projectileDiameterMm")]     = s.projectileDiameterMm;
    m[QStringLiteral("hasInnerTen")]              = s.hasInnerTen();
    if (s.hasInnerTen())
        m[QStringLiteral("innerTenRadiusFraction")] = s.innerTenRadiusMm() / face;

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
