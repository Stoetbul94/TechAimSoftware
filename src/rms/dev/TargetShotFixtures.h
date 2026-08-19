#ifndef TA_RMS_DEV_TARGETSHOTFIXTURES_H
#define TA_RMS_DEV_TARGETSHOTFIXTURES_H

// GENERATED FILE - do not edit by hand.
//   python tools/fixtures/generate_target_fixtures.py
//
// CORRELATED FIXTURE DATA - TEST/DEMO AUTHORITATIVE INPUT.
// Each shot carries a coordinate AND the score that belongs to it. The
// correlation was done by the generator, outside the RMS runtime, from
// the trusted engine's formula and the official ISSF Rule Book 2026
// geometry. RMS reads these as opaque authoritative values and derives
// nothing from them: there is no coordinate-to-score function in this
// product and this header does not introduce one.
//
// Rule source: ISSF Rule Book 2026, EDITION 2025 (Second Print 07/2026), effective 1 July 2026; rule 6.3.4, 7.4.6, 8.4.4
// Scoring source: CenterPane.qml::calculateShootingSocre() (node, trusted)

namespace ta { namespace rms { namespace dev {

struct FixtureShot { double xMm; double yMm; double score; };
struct FixtureTarget {
    const char* targetStandardId;
    const FixtureShot* shots;
    int shotCount;
};

// issf.10m.air-pistol - face radius 53.750 mm, projectile 4.5 mm
inline const FixtureShot kFixtures_issf_10m_air_pistol[] = {
    {     0.000,     0.000, 10.9 },   // dead centre
    {     8.000,     0.000, 10.0 },   // +x on the 10 boundary
    {    -8.000,     0.000, 10.0 },   // -x on the 10 boundary
    {     0.000,     8.000, 10.0 },   // +y on the 10 boundary
    {     0.000,    -8.000, 10.0 },   // -y on the 10 boundary
    {    16.000,     0.000,  9.0 },   // +x on the 9 boundary
    {   -16.000,     0.000,  9.0 },   // -x on the 9 boundary
    {     0.000,    16.000,  9.0 },   // +y on the 9 boundary
    {     0.000,   -16.000,  9.0 },   // -y on the 9 boundary
    {    24.000,     0.000,  8.0 },   // +x on the 8 boundary
    {   -24.000,     0.000,  8.0 },   // -x on the 8 boundary
    {     0.000,    24.000,  8.0 },   // +y on the 8 boundary
    {     0.000,   -24.000,  8.0 },   // -y on the 8 boundary
    {    11.314,    11.314,  9.0 },   // diagonal +x+y at the 9 radius
    {    11.314,   -11.314,  9.0 },   // diagonal +x-y at the 9 radius
    {   -11.314,    11.314,  9.0 },   // diagonal -x+y at the 9 radius
    {   -11.314,   -11.314,  9.0 },   // diagonal -x-y at the 9 radius
    {     3.080,     5.456, 10.2 },   // group shot 1
    {    -4.224,     1.848, 10.4 },   // group shot 2
    {     1.056,    -6.248, 10.2 },   // group shot 3
    {     5.808,    -1.232, 10.3 },   // group shot 4
    {    -2.376,    -4.840, 10.3 },   // group shot 5
    {    -5.368,     3.872, 10.2 },   // group shot 6
    {     0.440,     2.904, 10.6 },   // group shot 7
    {     3.872,     0.440, 10.5 },   // group shot 8
    {    -1.144,     0.704, 10.8 },   // group shot 9
    {     1.936,    -2.552, 10.6 },   // group shot 10
    {    63.425,    18.812,  2.7 },   // off the drawn face
};

// issf.10m.air-rifle - face radius 15.250 mm, projectile 4.5 mm
inline const FixtureShot kFixtures_issf_10m_air_rifle[] = {
    {     0.000,     0.000, 10.9 },   // dead centre
    {     2.500,     0.000, 10.0 },   // +x on the 10 boundary
    {    -2.500,     0.000, 10.0 },   // -x on the 10 boundary
    {     0.000,     2.500, 10.0 },   // +y on the 10 boundary
    {     0.000,    -2.500, 10.0 },   // -y on the 10 boundary
    {     5.000,     0.000,  9.0 },   // +x on the 9 boundary
    {    -5.000,     0.000,  9.0 },   // -x on the 9 boundary
    {     0.000,     5.000,  9.0 },   // +y on the 9 boundary
    {     0.000,    -5.000,  9.0 },   // -y on the 9 boundary
    {     7.500,     0.000,  8.0 },   // +x on the 8 boundary
    {    -7.500,     0.000,  8.0 },   // -x on the 8 boundary
    {     0.000,     7.500,  8.0 },   // +y on the 8 boundary
    {     0.000,    -7.500,  8.0 },   // -y on the 8 boundary
    {     3.536,     3.536,  9.0 },   // diagonal +x+y at the 9 radius
    {     3.536,    -3.536,  9.0 },   // diagonal +x-y at the 9 radius
    {    -3.536,     3.536,  9.0 },   // diagonal -x+y at the 9 radius
    {    -3.536,    -3.536,  9.0 },   // diagonal -x-y at the 9 radius
    {     0.962,     1.705, 10.2 },   // group shot 1
    {    -1.320,     0.578, 10.4 },   // group shot 2
    {     0.330,    -1.952, 10.2 },   // group shot 3
    {     1.815,    -0.385, 10.3 },   // group shot 4
    {    -0.743,    -1.513, 10.3 },   // group shot 5
    {    -1.677,     1.210, 10.2 },   // group shot 6
    {     0.138,     0.908, 10.6 },   // group shot 7
    {     1.210,     0.138, 10.5 },   // group shot 8
    {    -0.358,     0.220, 10.8 },   // group shot 9
    {     0.605,    -0.797, 10.6 },   // group shot 10
    {    17.995,     5.337,  3.5 },   // off the drawn face
};

// issf.50m.pistol - face radius 175.000 mm, projectile 5.6 mm
inline const FixtureShot kFixtures_issf_50m_pistol[] = {
    {     0.000,     0.000, 10.9 },   // dead centre
    {    27.800,     0.000, 10.0 },   // +x on the 10 boundary
    {   -27.800,     0.000, 10.0 },   // -x on the 10 boundary
    {     0.000,    27.800, 10.0 },   // +y on the 10 boundary
    {     0.000,   -27.800, 10.0 },   // -y on the 10 boundary
    {    52.800,     0.000,  9.0 },   // +x on the 9 boundary
    {   -52.800,     0.000,  9.0 },   // -x on the 9 boundary
    {     0.000,    52.800,  9.0 },   // +y on the 9 boundary
    {     0.000,   -52.800,  9.0 },   // -y on the 9 boundary
    {    77.800,     0.000,  8.0 },   // +x on the 8 boundary
    {   -77.800,     0.000,  8.0 },   // -x on the 8 boundary
    {     0.000,    77.800,  8.0 },   // +y on the 8 boundary
    {     0.000,   -77.800,  8.0 },   // -y on the 8 boundary
    {    37.335,    37.335,  9.0 },   // diagonal +x+y at the 9 radius
    {    37.335,   -37.335,  9.0 },   // diagonal +x-y at the 9 radius
    {   -37.335,    37.335,  9.0 },   // diagonal -x+y at the 9 radius
    {   -37.335,   -37.335,  9.0 },   // diagonal -x-y at the 9 radius
    {     9.625,    17.050, 10.3 },   // group shot 1
    {   -13.200,     5.775, 10.5 },   // group shot 2
    {     3.300,   -19.525, 10.3 },   // group shot 3
    {    18.150,    -3.850, 10.4 },   // group shot 4
    {    -7.425,   -15.125, 10.4 },   // group shot 5
    {   -16.775,    12.100, 10.3 },   // group shot 6
    {     1.375,     9.075, 10.7 },   // group shot 7
    {    12.100,     1.375, 10.6 },   // group shot 8
    {    -3.575,     2.200, 10.9 },   // group shot 9
    {     6.050,    -7.975, 10.7 },   // group shot 10
    {   206.500,    61.250,  2.5 },   // off the drawn face
};

// issf.50m.rifle - face radius 53.200 mm, projectile 5.6 mm
inline const FixtureShot kFixtures_issf_50m_rifle[] = {
    {     0.000,     0.000, 10.9 },   // dead centre
    {     8.000,     0.000, 10.0 },   // +x on the 10 boundary
    {    -8.000,     0.000, 10.0 },   // -x on the 10 boundary
    {     0.000,     8.000, 10.0 },   // +y on the 10 boundary
    {     0.000,    -8.000, 10.0 },   // -y on the 10 boundary
    {    16.000,     0.000,  9.0 },   // +x on the 9 boundary
    {   -16.000,     0.000,  9.0 },   // -x on the 9 boundary
    {     0.000,    16.000,  9.0 },   // +y on the 9 boundary
    {     0.000,   -16.000,  9.0 },   // -y on the 9 boundary
    {    24.000,     0.000,  8.0 },   // +x on the 8 boundary
    {   -24.000,     0.000,  8.0 },   // -x on the 8 boundary
    {     0.000,    24.000,  8.0 },   // +y on the 8 boundary
    {     0.000,   -24.000,  8.0 },   // -y on the 8 boundary
    {    11.314,    11.314,  9.0 },   // diagonal +x+y at the 9 radius
    {    11.314,   -11.314,  9.0 },   // diagonal +x-y at the 9 radius
    {   -11.314,    11.314,  9.0 },   // diagonal -x+y at the 9 radius
    {   -11.314,   -11.314,  9.0 },   // diagonal -x-y at the 9 radius
    {     3.080,     5.456, 10.2 },   // group shot 1
    {    -4.224,     1.848, 10.4 },   // group shot 2
    {     1.056,    -6.248, 10.2 },   // group shot 3
    {     5.808,    -1.232, 10.3 },   // group shot 4
    {    -2.376,    -4.840, 10.3 },   // group shot 5
    {    -5.368,     3.872, 10.2 },   // group shot 6
    {     0.440,     2.904, 10.6 },   // group shot 7
    {     3.872,     0.440, 10.5 },   // group shot 8
    {    -1.144,     0.704, 10.8 },   // group shot 9
    {     1.936,    -2.552, 10.6 },   // group shot 10
    {    62.776,    18.620,  2.8 },   // off the drawn face
};

inline const FixtureTarget kFixtureTargets[] = {
    { "issf.10m.air-pistol", kFixtures_issf_10m_air_pistol, 28 },
    { "issf.10m.air-rifle", kFixtures_issf_10m_air_rifle, 28 },
    { "issf.50m.pistol", kFixtures_issf_50m_pistol, 28 },
    { "issf.50m.rifle", kFixtures_issf_50m_rifle, 28 },
};
inline constexpr int kFixtureTargetCount = 4;

} } }  // namespace ta::rms::dev

#endif
