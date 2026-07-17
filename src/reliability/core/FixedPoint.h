#ifndef TA_REL_FIXEDPOINT_H
#define TA_REL_FIXEDPOINT_H

// Session Reliability Layer — authoritative fixed-point value types (M1,
// spec §6). Floating point is FORBIDDEN as an authoritative persisted value;
// these types are the only sanctioned representation. Conversion from
// double happens exactly once, at event creation, through fromDouble().
//
// Rounding rules (spec §6):
//   - decimal quantities (score tenths, coordinate/direction hundredths):
//     HALF AWAY FROM ZERO on the decimal value the double represents.
//     Binary representation error is compensated deterministically (see
//     scaledHalfAwayFromZero) so that e.g. -1.235 mm -> -124 hundredths
//     even though (-1.235 * 100) is stored as -123.499999... in IEEE-754.
//   - millisecond quantities: TRUNCATION toward zero (ms are native
//     integers in every producer; fromDouble exists for completeness).
//
// All types serialize as their raw integer (serialized()). toDisplayDouble()
// is for presentation only and must never round-trip back into storage.

#include <QtGlobal>

#include <cmath>
#include <limits>

namespace ta {
namespace rel {

enum class FixedPointStatus {
    Ok = 0,
    NotFinite,   // NaN or +-Inf input
    Overflow     // outside the representation's range
};

namespace fpdetail {

// Deterministic half-away-from-zero rounding of (value * scale).
// The epsilon compensates for the binary representation error of decimal
// inputs: it is proportional to the magnitude (1e-12 relative, 1e-9 floor),
// far below any physically meaningful distinction at these scales, and the
// identical computation runs on every platform (IEEE-754 double).
inline bool scaledHalfAwayFromZero(double value, qint64 scale, qint64* out)
{
    if (!std::isfinite(value))
        return false;
    const double scaled = value * static_cast<double>(scale);
    if (!std::isfinite(scaled))
        return false;
    const double eps = std::fabs(scaled) * 1e-12 + 1e-9;
    const double magnitude = std::floor(std::fabs(scaled) + 0.5 + eps);
    if (magnitude > 9.2e18)   // beyond qint64 — reject before the cast UB
        return false;
    *out = static_cast<qint64>(scaled < 0.0 ? -magnitude : magnitude);
    return true;
}

// Deterministic truncation toward zero of (value * scale).
inline bool scaledTruncate(double value, qint64 scale, qint64* out)
{
    if (!std::isfinite(value))
        return false;
    const double scaled = value * static_cast<double>(scale);
    if (!std::isfinite(scaled) || std::fabs(scaled) > 9.2e18)
        return false;
    *out = static_cast<qint64>(std::trunc(scaled));
    return true;
}

} // namespace fpdetail

// CRTP base: one implementation, five distinct incompatible types.
// Construction from a raw integer is EXPLICIT; there is deliberately no
// constructor, assignment or comparison taking double.
template <typename Derived, typename Rep, qint64 ScaleV, bool HalfAwayV>
class FixedPointValue {
public:
    using RepType = Rep;
    static constexpr qint64 kScale = ScaleV;

    constexpr FixedPointValue() = default;
    constexpr explicit FixedPointValue(Rep raw) : m_raw(raw) {}

    constexpr Rep raw() const { return m_raw; }
    constexpr qint64 serialized() const { return static_cast<qint64>(m_raw); }

    // Display/formatting ONLY — never authoritative.
    double toDisplayDouble() const
    {
        return static_cast<double>(m_raw) / static_cast<double>(ScaleV);
    }

    // The single sanctioned double -> fixed-point conversion boundary.
    static FixedPointStatus fromDouble(double units, Derived* out)
    {
        if (!std::isfinite(units))
            return FixedPointStatus::NotFinite;
        qint64 scaled = 0;
        const bool inRange = HalfAwayV
            ? fpdetail::scaledHalfAwayFromZero(units, ScaleV, &scaled)
            : fpdetail::scaledTruncate(units, ScaleV, &scaled);
        if (!inRange)
            return FixedPointStatus::Overflow;   // exceeds even 64-bit raw
        return fromRaw64(scaled, out);
    }

    // Overflow-checked narrowing from a 64-bit raw value (deserialization).
    static FixedPointStatus fromRaw64(qint64 raw, Derived* out)
    {
        if (raw < static_cast<qint64>(std::numeric_limits<Rep>::min())
            || raw > static_cast<qint64>(std::numeric_limits<Rep>::max()))
            return FixedPointStatus::Overflow;
        *out = Derived(static_cast<Rep>(raw));
        return FixedPointStatus::Ok;
    }

    friend constexpr bool operator==(const Derived& a, const Derived& b)
    { return a.raw() == b.raw(); }
    friend constexpr bool operator!=(const Derived& a, const Derived& b)
    { return a.raw() != b.raw(); }
    friend constexpr bool operator<(const Derived& a, const Derived& b)
    { return a.raw() < b.raw(); }

private:
    Rep m_raw = 0;
};

// 0.1 ring — e.g. 10.4 -> raw 104.
struct ScoreTenths : FixedPointValue<ScoreTenths, qint16, 10, true> {
    using FixedPointValue::FixedPointValue;
};

// 0.01 mm from target centre — e.g. -1.235 mm -> raw -124.
struct CoordinateHundredthMm
    : FixedPointValue<CoordinateHundredthMm, qint32, 100, true> {
    using FixedPointValue::FixedPointValue;
};

// 0.01 degree — e.g. 287.34 deg -> raw 28734.
struct CentiDegrees : FixedPointValue<CentiDegrees, qint32, 100, true> {
    using FixedPointValue::FixedPointValue;
};

// Native milliseconds (durations, elapsed) — truncation per spec §6.
struct ElapsedMilliseconds
    : FixedPointValue<ElapsedMilliseconds, qint64, 1, false> {
    using FixedPointValue::FixedPointValue;
};

// Native milliseconds (shot-to-shot splits) — truncation per spec §6.
struct SplitMilliseconds
    : FixedPointValue<SplitMilliseconds, qint32, 1, false> {
    using FixedPointValue::FixedPointValue;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_FIXEDPOINT_H
