// ─────────────────────────────────────────────────────────────────────────────
// TARGET GEOMETRY + SHOT REGISTRATION QUALIFICATION (milestone 4.6)
//
// These tests answer one question: does a shot appear where its coordinate
// says it is, on a face whose rings are the ones the rulebook specifies?
//
// The official values are duplicated here ON PURPOSE. A test that imported the
// production table would agree with it whatever it said; these constants were
// typed from the ISSF Rule Book 2026 (EDITION 2025 Second Print 07/2026,
// rule 6.3.4) and cited in
// docs/architecture/rms-target-geometry-source-register.md, so if the product
// table drifts, the two disagree and this fails.
//
// NOTHING HERE SCORES, and nothing here asks RMS to. Ring-boundary tests assert
// that a point at radius R lands on the ring drawn at radius R — a DISPLAY
// fact. The correlated fixtures carry their scores as supplied data.
// ─────────────────────────────────────────────────────────────────────────────

#include "test_support.h"

#include "rms/TargetGeometry.h"
#include "rms/dev/TargetShotFixtures.h"

#include <QSet>

#include <cmath>
#include <cstdio>

using namespace ta::rms;

namespace {

// ── the official table, typed from the rulebook ──────────────────────────
struct Official {
    const char* id;
    double tenRingDiameterMm;
    double ringStepDiameterMm;
    double blackDiameterMm;
    double innerTenDiameterMm;   // 0 = not defined by dimension
    double projectileDiameterMm;
    double ring9DiameterMm;      // independently typed, to catch a bad step
    double ring4DiameterMm;      // the face RMS draws to
};

const Official kOfficial[] = {
    // 6.3.4.3 10 m Air Rifle. Inner ten is gauge-defined, not dimensioned.
    { "issf.10m.air-rifle",   0.5,  5.0,  30.5,  0.0, 4.5,   5.5,  30.5 },
    // 6.3.4.6 10 m Air Pistol
    { "issf.10m.air-pistol", 11.5, 16.0,  59.5,  5.0, 4.5,  27.5, 107.5 },
    // 6.3.4.2 50 m Rifle. Black is 112.4 — BETWEEN the 4 and 3 rings.
    { "issf.50m.rifle",      10.4, 16.0, 112.4,  5.0, 5.6,  26.4, 106.4 },
    // 6.3.4.5 50 m Pistol. A completely different face from the 50 m rifle.
    { "issf.50m.pistol",     50.0, 50.0, 200.0, 25.0, 5.6, 100.0, 350.0 }
};

bool near(double a, double b, double tol = 1e-9)
{
    return std::fabs(a - b) <= tol;
}

double radiusOf(const QPointF& p) { return std::hypot(p.x(), p.y()); }

}  // namespace

void run_target_geometry_tests()
{
    // ── A. the definitions exist, and only the ones we qualified ─────────
    std::printf("\n-- geometry definitions --\n");
    {
        const QStringList supported = TargetGeometry::supportedStandards();
        check(supported.size() == 4,
              "four target standards are supported", QString::number(supported.size()));
        for (const Official& o : kOfficial) {
            check(supported.contains(QString::fromLatin1(o.id)),
                  QStringLiteral("supported: %1").arg(QLatin1String(o.id)));
        }
        // A standard nobody qualified must not quietly acquire a face.
        check(!TargetGeometry::isSupported(QStringLiteral("issf.300m.standard")),
              "300 m is NOT claimed — it was never qualified");
        check(!TargetGeometry::isSupported(QStringLiteral("issf.25m.rapid-fire")),
              "25 m rapid fire is NOT claimed");
    }

    // ── B/C. ring dimensions match the rulebook, in DIAMETERS ────────────
    std::printf("\n-- ring dimensions against the official register --\n");
    for (const Official& o : kOfficial) {
        const QString id = QString::fromLatin1(o.id);
        const TargetSpec s = TargetGeometry::specFor(id);
        check(s.supported, QStringLiteral("%1 is supported").arg(id));
        if (!s.supported)
            continue;

        check(near(s.tenRingDiameterMm, o.tenRingDiameterMm),
              QStringLiteral("%1: 10 ring diameter %2 mm").arg(id).arg(o.tenRingDiameterMm),
              QString::number(s.tenRingDiameterMm));
        check(near(s.ringStepDiameterMm, o.ringStepDiameterMm),
              QStringLiteral("%1: ring spacing %2 mm diameter").arg(id).arg(o.ringStepDiameterMm),
              QString::number(s.ringStepDiameterMm));
        check(near(s.blackDiameterMm, o.blackDiameterMm),
              QStringLiteral("%1: black %2 mm diameter").arg(id).arg(o.blackDiameterMm),
              QString::number(s.blackDiameterMm));
        check(near(s.innerTenDiameterMm, o.innerTenDiameterMm),
              QStringLiteral("%1: inner ten %2 mm diameter").arg(id).arg(o.innerTenDiameterMm),
              QString::number(s.innerTenDiameterMm));
        check(near(s.projectileDiameterMm, o.projectileDiameterMm),
              QStringLiteral("%1: projectile %2 mm").arg(id).arg(o.projectileDiameterMm),
              QString::number(s.projectileDiameterMm));

        // Independently typed rings, so a wrong STEP cannot pass by agreeing
        // with itself.
        check(near(s.ringDiameterMm(9), o.ring9DiameterMm),
              QStringLiteral("%1: 9 ring diameter %2 mm").arg(id).arg(o.ring9DiameterMm),
              QString::number(s.ringDiameterMm(9)));
        check(near(s.ringDiameterMm(4), o.ring4DiameterMm),
              QStringLiteral("%1: 4 ring diameter %2 mm").arg(id).arg(o.ring4DiameterMm),
              QString::number(s.ringDiameterMm(4)));

        // C. DIAMETER vs RADIUS — the classic 2x error, asserted directly.
        check(near(s.ringRadiusMm(10), o.tenRingDiameterMm / 2.0),
              QStringLiteral("%1: the 10 ring RADIUS is half its diameter").arg(id));
        check(near(s.faceRadiusMm(), o.ring4DiameterMm / 2.0),
              QStringLiteral("%1: the face radius is half the 4 ring diameter").arg(id));
        check(near(s.blackRadiusMm(), o.blackDiameterMm / 2.0),
              QStringLiteral("%1: the black RADIUS is half its diameter").arg(id));
        check(near(s.projectileRadiusMm(), o.projectileDiameterMm / 2.0),
              QStringLiteral("%1: the projectile RADIUS is half its calibre").arg(id));
        // If a diameter were ever used as a radius the face would be twice as
        // big; state the expected number outright.
        check(s.faceRadiusMm() < o.ring4DiameterMm,
              QStringLiteral("%1: the face radius is NOT the diameter").arg(id));
    }

    // ── the two defects this milestone found, asserted so they stay fixed ─
    std::printf("\n-- the defects this milestone corrected --\n");
    {
        const TargetSpec pistol = TargetGeometry::specFor(QStringLiteral("issf.50m.pistol"));
        const TargetSpec rifle  = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        check(!near(pistol.tenRingDiameterMm, rifle.tenRingDiameterMm),
              "50 m PISTOL is not the 50 m RIFLE face — the ten rings differ",
              QStringLiteral("%1 vs %2").arg(pistol.tenRingDiameterMm)
                                        .arg(rifle.tenRingDiameterMm));
        check(near(pistol.tenRingDiameterMm, 50.0),
              "...and the pistol ten ring is the official 50 mm");
        check(pistol.faceRadiusMm() > rifle.faceRadiusMm() * 3.0,
              "...so the pistol face is several times the rifle face",
              QStringLiteral("%1 vs %2").arg(pistol.faceRadiusMm())
                                        .arg(rifle.faceRadiusMm()));

        // The 50 m rifle black does NOT land on a ring edge (rule: "part of 3").
        check(near(rifle.blackRadiusMm(), 56.2),
              "50 m rifle black radius is 56.2 mm", QString::number(rifle.blackRadiusMm()));
        check(rifle.blackRadiusMm() > rifle.faceRadiusMm(),
              "...which is beyond the drawn face, so the whole face is black");
        bool onARing = false;
        for (int r = 1; r <= 10; ++r)
            if (near(rifle.blackRadiusMm(), rifle.ringRadiusMm(r), 1e-6))
                onARing = true;
        check(!onARing,
              "...and it lands between rings, which a ring index could not express");
    }

    // ── D. the centre ────────────────────────────────────────────────────
    std::printf("\n-- shot centre mapping --\n");
    for (const Official& o : kOfficial) {
        const TargetSpec s = TargetGeometry::specFor(QString::fromLatin1(o.id));
        const QPointF c = TargetGeometry::normalise(s, 0.0, 0.0);
        check(near(c.x(), 0.0) && near(c.y(), 0.0),
              QStringLiteral("%1: (0,0) is the exact centre").arg(QLatin1String(o.id)));
        // And in pixels, at a size with an inset, the centre is the view centre.
        const QPointF px = TargetGeometry::toView(c, 400.0, 8.0);
        check(near(px.x(), 200.0) && near(px.y(), 200.0),
              QStringLiteral("%1: ...and the pixel centre too").arg(QLatin1String(o.id)));
    }

    // ── E/F/G/H. the four axes ───────────────────────────────────────────
    std::printf("\n-- axes --\n");
    for (const Official& o : kOfficial) {
        const QString id = QString::fromLatin1(o.id);
        const TargetSpec s = TargetGeometry::specFor(id);
        const double face = s.faceRadiusMm();
        const double d = face / 2.0;          // half way out, on each axis

        const QPointF px = TargetGeometry::normalise(s, d, 0.0);
        check(near(px.x(), 0.5) && near(px.y(), 0.0),
              QStringLiteral("%1: +x goes right, by exactly half the face").arg(id));

        const QPointF nx = TargetGeometry::normalise(s, -d, 0.0);
        check(near(nx.x(), -0.5) && near(nx.y(), 0.0),
              QStringLiteral("%1: -x goes left, by the same amount").arg(id));

        // TELEMETRY +Y IS UP; SCREEN +Y IS DOWN. The flip happens once, here.
        const QPointF py = TargetGeometry::normalise(s, 0.0, d);
        check(near(py.y(), -0.5) && near(py.x(), 0.0),
              QStringLiteral("%1: +y is UP the screen (negative screen y)").arg(id),
              QString::number(py.y()));

        const QPointF ny = TargetGeometry::normalise(s, 0.0, -d);
        check(near(ny.y(), 0.5) && near(ny.x(), 0.0),
              QStringLiteral("%1: -y is DOWN the screen").arg(id));

        check(near(px.x(), -nx.x()) && near(py.y(), -ny.y()),
              QStringLiteral("%1: the axes are symmetric about the centre").arg(id));
    }

    // ── I/J. diagonals, and equal radius staying equal ───────────────────
    std::printf("\n-- diagonals and radial fidelity --\n");
    for (const Official& o : kOfficial) {
        const QString id = QString::fromLatin1(o.id);
        const TargetSpec s = TargetGeometry::specFor(id);
        const double d = s.faceRadiusMm() * 0.4;

        const QPointF q1 = TargetGeometry::normalise(s,  d,  d);
        const QPointF q2 = TargetGeometry::normalise(s,  d, -d);
        const QPointF q3 = TargetGeometry::normalise(s, -d,  d);
        const QPointF q4 = TargetGeometry::normalise(s, -d, -d);

        check(q1.x() > 0 && q1.y() < 0, QStringLiteral("%1: (+x,+y) is up-right").arg(id));
        check(q2.x() > 0 && q2.y() > 0, QStringLiteral("%1: (+x,-y) is down-right").arg(id));
        check(q3.x() < 0 && q3.y() < 0, QStringLiteral("%1: (-x,+y) is up-left").arg(id));
        check(q4.x() < 0 && q4.y() > 0, QStringLiteral("%1: (-x,-y) is down-left").arg(id));

        const double r1 = radiusOf(q1);
        check(near(r1, radiusOf(q2)) && near(r1, radiusOf(q3)) && near(r1, radiusOf(q4)),
              QStringLiteral("%1: all four diagonals are the SAME radius").arg(id));

        // K. aspect ratio: an equal mm offset on x and y is an equal
        // normalised offset. A stretched target fails here.
        check(near(std::fabs(q1.x()), std::fabs(q1.y())),
              QStringLiteral("%1: x and y share one scale — the face is round").arg(id));
    }

    // ── ring boundaries: a point at radius R lands on the ring at R ───────
    std::printf("\n-- ring boundary registration --\n");
    for (const Official& o : kOfficial) {
        const QString id = QString::fromLatin1(o.id);
        const TargetSpec s = TargetGeometry::specFor(id);
        const double face = s.faceRadiusMm();

        for (int ring = 4; ring <= 10; ++ring) {
            const double r = s.ringRadiusMm(ring);
            const QPointF n = TargetGeometry::normalise(s, r, 0.0);
            // The renderer draws ring `ring` at fraction r/face; a shot at
            // radius r must normalise to exactly that fraction.
            check(near(n.x(), r / face),
                  QStringLiteral("%1: a shot at the %2 ring radius sits on the %2 ring")
                      .arg(id).arg(ring));
        }
        // ...and the same on the y axis, where the flip could hide an error.
        const double r9 = s.ringRadiusMm(9);
        const QPointF n9 = TargetGeometry::normalise(s, 0.0, r9);
        check(near(std::fabs(n9.y()), r9 / face),
              QStringLiteral("%1: and on the y axis too").arg(id));
    }

    // ── N. the projectile, at its true size ──────────────────────────────
    std::printf("\n-- projectile footprint --\n");
    for (const Official& o : kOfficial) {
        const QString id = QString::fromLatin1(o.id);
        const TargetSpec s = TargetGeometry::specFor(id);
        const double fraction = s.projectileRadiusMm() / s.faceRadiusMm();
        check(fraction > 0.0, QStringLiteral("%1: the projectile has a size").arg(id));
        // The renderer multiplies this fraction by the face radius in pixels.
        // At 300 px of face the drawn radius must be the physical one.
        const double drawnPx = fraction * 300.0;
        const double expectPx = s.projectileRadiusMm() / s.faceRadiusMm() * 300.0;
        check(near(drawnPx, expectPx),
              QStringLiteral("%1: drawn radius follows the physical one").arg(id));
    }
    {
        // The proportions that make this matter, stated as numbers.
        const TargetSpec ar = TargetGeometry::specFor(QStringLiteral("issf.10m.air-rifle"));
        const TargetSpec pp = TargetGeometry::specFor(QStringLiteral("issf.50m.pistol"));
        const double arF = ar.projectileRadiusMm() / ar.faceRadiusMm();
        const double ppF = pp.projectileRadiusMm() / pp.faceRadiusMm();
        check(arF > 0.14 && arF < 0.16,
              "a 4.5 mm pellet is about 15% of a 10 m air rifle face",
              QString::number(arF));
        check(ppF < 0.02,
              "...and about 1.6% of a 50 m pistol face",
              QString::number(ppF));
        check(arF > ppF * 8.0,
              "so ONE fixed marker size cannot serve both — which is why there is none");
        // The pellet dwarfs the ten ring on the air rifle face. This is the
        // reason a shot CENTRE can look outside the ring its score names.
        check(ar.projectileDiameterMm > ar.tenRingDiameterMm * 8.0,
              "a 4.5 mm pellet is nine times the 0.5 mm ten ring across",
              QStringLiteral("%1 vs %2").arg(ar.projectileDiameterMm)
                                        .arg(ar.tenRingDiameterMm));
    }

    // ── L/M. scale invariance — card and full screen agree ───────────────
    std::printf("\n-- scale invariance --\n");
    {
        const TargetSpec s = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        const QPointF n = TargetGeometry::normalise(s, 17.0, -9.0);

        // Card, single view, full screen: three sizes, one relative position.
        const double sizes[] = { 160.0, 300.0, 600.0, 1000.0 };
        for (double sz : sizes) {
            const QPointF px = TargetGeometry::toView(n, sz, 4.0);
            const double radiusPx = sz / 2.0 - 4.0;
            const double relX = (px.x() - sz / 2.0) / radiusPx;
            const double relY = (px.y() - sz / 2.0) / radiusPx;
            check(near(relX, n.x(), 1e-9) && near(relY, n.y(), 1e-9),
                  QStringLiteral("at %1 px the shot holds the same relative place").arg(sz));
        }
        // The normalised value itself never depended on a size at all — which
        // is what makes a drift between views impossible rather than unlikely.
        check(near(radiusOf(n), radiusOf(TargetGeometry::normalise(s, 17.0, -9.0))),
              "normalisation is resolution-independent by construction");
    }

    // ── O. an unknown standard ───────────────────────────────────────────
    std::printf("\n-- unknown standards --\n");
    {
        const TargetSpec u = TargetGeometry::specFor(QStringLiteral("issf.300m.rifle"));
        check(!u.supported, "an unqualified standard is not supported");
        check(near(u.faceRadiusMm(), 0.0), "...and has no face to draw");
        const QPointF n = TargetGeometry::normalise(u, 12.0, 4.0);
        check(near(n.x(), 0.0) && near(n.y(), 0.0),
              "...and places nothing rather than guessing a scale");
        const TargetSpec e = TargetGeometry::specFor(QString());
        check(!e.supported, "an empty standard is not supported either");
    }

    // ── P. off the printed face ──────────────────────────────────────────
    std::printf("\n-- off-face shots --\n");
    {
        const TargetSpec s = TargetGeometry::specFor(QStringLiteral("issf.10m.air-pistol"));
        const double face = s.faceRadiusMm();

        check(TargetGeometry::isWithinFace(s, face * 0.99, 0.0), "a shot inside the face is inside");
        check(!TargetGeometry::isWithinFace(s, face * 1.02, 0.0), "a shot past the edge is outside");
        check(TargetGeometry::isWithinFace(s, face, 0.0), "the edge itself counts as on the face");

        // The TRUE coordinate is never modified — normalise() is honest even
        // when the shot is off the card.
        const QPointF trueN = TargetGeometry::normalise(s, face * 1.5, 0.0);
        check(near(trueN.x(), 1.5),
              "the true normalised radius is preserved past the edge",
              QString::number(trueN.x()));

        // Only the RENDERING position is pulled to the rim, and the direction
        // survives so the chevron points the right way.
        const QPointF clamped = TargetGeometry::normaliseClamped(s, face * 1.5, face * 1.5);
        check(near(radiusOf(clamped), 1.0), "the drawn position is held at the rim");
        check(clamped.x() > 0 && clamped.y() < 0,
              "...in the true direction of the shot, up and to the right");
        const QPointF inside = TargetGeometry::normaliseClamped(s, face * 0.3, 0.0);
        check(near(inside.x(), 0.3), "a shot already inside is not moved at all");
    }

    // ── Q. correlated fixtures ───────────────────────────────────────────
    //
    // The fixtures carry a coordinate AND the score that belongs with it. RMS
    // does not compute the relationship — it is asserted here so a regenerated
    // fixture set that lost its correlation cannot pass unnoticed.
    std::printf("\n-- correlated fixtures --\n");
    {
        using namespace ta::rms::dev;
        check(kFixtureTargetCount == 4, "fixtures exist for all four standards");

        QSet<QString> covered;
        int totalShots = 0;
        for (int i = 0; i < kFixtureTargetCount; ++i) {
            const FixtureTarget& t = kFixtureTargets[i];
            const QString id = QString::fromLatin1(t.targetStandardId);
            covered.insert(id);
            const TargetSpec s = TargetGeometry::specFor(id);
            check(s.supported, QStringLiteral("fixture target %1 is a real standard").arg(id));
            check(t.shotCount > 12,
                  QStringLiteral("%1 has enough fixtures to be a picture").arg(id),
                  QString::number(t.shotCount));
            totalShots += t.shotCount;

            // The relationship the generator encoded: score falls as radius
            // grows, at one ring per ring-step, offset by the pellet radius.
            // Checked HERE, in a test, never in the product.
            for (int j = 0; j < t.shotCount; ++j) {
                const FixtureShot& f = t.shots[j];
                const double r = std::hypot(f.xMm, f.yMm);
                double expect = 9.0
                    + ((s.ringStepDiameterMm / 2.0) + s.ringRadiusMm(10)
                       + s.projectileRadiusMm() - r) / (s.ringStepDiameterMm / 2.0);
                if (expect > 10.9) expect = 10.9;
                if (expect < 0.0)  expect = 0.0;
                if (!near(f.score, std::round(expect * 10.0) / 10.0, 0.051)) {
                    check(false,
                          QStringLiteral("%1 fixture %2 is correlated").arg(id).arg(j),
                          QStringLiteral("score %1 at r=%2 expected %3")
                              .arg(f.score).arg(r).arg(expect));
                    break;
                }
                if (j == t.shotCount - 1)
                    check(true, QStringLiteral("%1: every fixture score matches its "
                                               "coordinate").arg(id));
            }

            // A centre fixture must exist and must be the best score there is.
            bool hasCentre = false;
            for (int j = 0; j < t.shotCount; ++j)
                if (near(t.shots[j].xMm, 0.0) && near(t.shots[j].yMm, 0.0)) {
                    hasCentre = true;
                    check(t.shots[j].score >= 10.8,
                          QStringLiteral("%1: the centre fixture scores at the top").arg(id),
                          QString::number(t.shots[j].score));
                }
            check(hasCentre, QStringLiteral("%1: a dead-centre fixture exists").arg(id));
        }
        check(covered.size() == 4, "every standard has its own fixtures");
        check(totalShots > 80, "the fixture set is substantial",
              QString::number(totalShots));

        // THE POINT OF ALL THIS: on the air rifle face a shot that scores 10.0
        // has its CENTRE far outside the ten ring, because ISSF scores by the
        // pellet's edge. A display that draws only a centre dot makes that look
        // like a bug; drawing the true footprint makes it legible.
        const TargetSpec ar = TargetGeometry::specFor(QStringLiteral("issf.10m.air-rifle"));
        const double tenBoundary = ar.ringRadiusMm(10) + ar.projectileRadiusMm();
        check(tenBoundary > ar.ringRadiusMm(10) * 9.0,
              "a 10.0 on air rifle has its centre TEN TIMES the ten ring radius out",
              QStringLiteral("10.0 at r=%1 mm, ten ring radius %2 mm")
                  .arg(tenBoundary).arg(ar.ringRadiusMm(10)));
        check(tenBoundary > ar.ringRadiusMm(10) && tenBoundary < ar.ringRadiusMm(9),
              "...outside the ten ring entirely, though inside the nine",
              QStringLiteral("r=%1, ten %2, nine %3")
                  .arg(tenBoundary).arg(ar.ringRadiusMm(10)).arg(ar.ringRadiusMm(9)));
        // The same fact on the pistol face, where it is far less dramatic.
        const TargetSpec ap = TargetGeometry::specFor(QStringLiteral("issf.10m.air-pistol"));
        const double apTen = ap.ringRadiusMm(10) + ap.projectileRadiusMm();
        check(apTen < ap.ringRadiusMm(10) * 1.5,
              "on air pistol the same offset is under half the ten ring radius",
              QStringLiteral("10.0 at r=%1, ten ring %2").arg(apTen).arg(ap.ringRadiusMm(10)));
    }

    // ── T. competition state cannot touch geometry ───────────────────────
    std::printf("\n-- geometry is independent of everything else --\n");
    {
        // There is no path by which a lane's competition state could reach the
        // geometry: specFor() takes a standard id and nothing else. Stated as
        // a test so a future signature change has to argue with it.
        const TargetSpec a = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        const TargetSpec b = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        check(near(a.faceRadiusMm(), b.faceRadiusMm())
                  && near(a.tenRingDiameterMm, b.tenRingDiameterMm),
              "the same standard always yields the same face — no hidden state");
        check(near(a.faceRadiusMm(), 53.2),
              "50 m rifle face radius is 53.2 mm whatever the lane is doing",
              QString::number(a.faceRadiusMm()));
    }
}

// ── R3B: the full scoring region, and the physical shots that exposed it ────
//
// The first physical test produced two valid low shots - 3.4 at r = 18.83 mm
// and 4.3 at r = 16.71 mm - which rendered as arrows at the edge of a face
// drawn to 15.25 mm. Nothing was wrong with the score or the coordinates; the
// drawn face is deliberately cropped at the 4 ring, and those shots were simply
// outside the crop.
//
// These checks pin BOTH facts: the crop is unchanged, and the full scoring
// region is now available to a view that wants to fit to it.
void run_scoring_region_tests()
{
    std::printf("\n-- the full 10 m scoring region (R3B) --\n");

    const ta::rms::TargetSpec s =
        ta::rms::TargetGeometry::specFor(QStringLiteral("issf.10m.air-rifle"));
    check(s.supported, "region: the 10 m air rifle standard is known");

    // The crop is UNCHANGED. Every existing lane tile renders as before.
    check(qAbs(s.faceRadiusMm() - 15.25) < 0.001,
          "region: the drawn face is still cropped at the 4 ring (15.25 mm)",
          QStringLiteral("%1").arg(s.faceRadiusMm()));

    // The card scores out to the 1 ring: 0.5 + 9 x 5.0 = 45.5 mm diameter.
    check(qAbs(s.scoringRadiusMm() - 22.75) < 0.001,
          "region: but the scoring region reaches the 1 ring (22.75 mm)",
          QStringLiteral("%1").arg(s.scoringRadiusMm()));
    check(s.scoringRadiusMm() > s.faceRadiusMm(),
          "region: the scoring region is LARGER than the drawn tile");

    // THE TWO PHYSICAL SHOTS. Both are valid shots on the card, and both fall
    // outside the cropped tile - which is exactly why they looked wrong.
    const double rShot1 = 18.82976367350371;   // scored 3.4
    const double rShot4 = 16.714065932620944;  // scored 4.3
    check(rShot1 > s.faceRadiusMm(),
          "region: the 3.4 shot is outside the CROPPED face - hence the arrow");
    check(s.withinScoringRegion(rShot1),
          "region: but it is inside the SCORING region - a valid shot, not an error");
    check(rShot4 > s.faceRadiusMm(),
          "region: the 4.3 shot is outside the cropped face too");
    check(s.withinScoringRegion(rShot4),
          "region: and is also a valid scoring shot");

    // A shot genuinely off the card is still distinguishable from those two.
    check(!s.withinScoringRegion(30.0),
          "region: a shot beyond the card is still reported as outside it");

    // The bridge exposes both, so a renderer can choose without arithmetic.
    ta::rms::TargetGeometryBridge bridge;
    const QVariantMap m = bridge.specFor(QStringLiteral("issf.10m.air-rifle"));
    check(qAbs(m.value("faceRadiusMm").toDouble() - 15.25) < 0.001,
          "region: the bridge still reports the cropped face unchanged");
    check(qAbs(m.value("scoringRadiusMm").toDouble() - 22.75) < 0.001,
          "region: and now also reports the scoring radius");
    const double factor = m.value("scoringRadiusFraction").toDouble();
    check(qAbs(factor - (22.75 / 15.25)) < 0.0001,
          "region: with the factor a detail view needs to fit to it",
          QStringLiteral("%1").arg(factor));

    const QVariantList scoringRings = m.value("scoringRings").toList();
    check(scoringRings.size() == 10,
          "region: all ten scoring rings are described",
          QStringLiteral("%1").arg(scoringRings.size()));
    int croppedAway = 0;
    for (const QVariant& v : scoringRings)
        if (v.toMap().value("outsideDrawnFace").toBool()) ++croppedAway;
    check(croppedAway == 3,
          "region: rings 1-3 are marked as cropped from the compact tile",
          QStringLiteral("%1").arg(croppedAway));
    // The outermost scoring ring sits exactly at the scoring radius.
    check(qAbs(scoringRings.first().toMap().value("fraction").toDouble() - 1.0) < 0.0001,
          "region: the 1 ring lands exactly on the scoring radius");
    std::fflush(stdout);
}
