// M1 — fixed-point type tests (step 4): rounding (positive / negative /
// midpoint), overflow, round-trip, required conversion vectors.

#include "reliability/core/FixedPoint.h"
#include "test_support.h"

#include <cmath>

using namespace ta::rel;

void run_fixedpoint_tests()
{
    std::printf("--- fixed-point tests ---\n");

    // required vectors
    {
        ScoreTenths s;
        check(ScoreTenths::fromDouble(10.4, &s) == FixedPointStatus::Ok
                  && s.raw() == 104,
              "10.4 -> 104 tenths", QString::number(s.raw()));
        CoordinateHundredthMm c;
        check(CoordinateHundredthMm::fromDouble(-1.235, &c) == FixedPointStatus::Ok
                  && c.raw() == -124,
              "-1.235 mm -> -124 hundredths (half away from zero)",
              QString::number(c.raw()));
    }

    // positive rounding
    {
        ScoreTenths s;
        ScoreTenths::fromDouble(10.44, &s);
        check(s.raw() == 104, "positive round down (10.44 -> 104)");
        ScoreTenths::fromDouble(10.46, &s);
        check(s.raw() == 105, "positive round up (10.46 -> 105)");
        ScoreTenths::fromDouble(10.45, &s);
        check(s.raw() == 105, "positive midpoint away from zero (10.45 -> 105)");
    }
    // negative rounding
    {
        CoordinateHundredthMm c;
        CoordinateHundredthMm::fromDouble(-0.004, &c);
        check(c.raw() == 0, "negative round toward zero (-0.004 -> 0)");
        CoordinateHundredthMm::fromDouble(-0.006, &c);
        check(c.raw() == -1, "negative round away (-0.006 -> -1)");
        CoordinateHundredthMm::fromDouble(-0.005, &c);
        check(c.raw() == -1, "negative midpoint away from zero (-0.005 -> -1)");
        CoordinateHundredthMm::fromDouble(2.675, &c);
        check(c.raw() == 268,
              "representation-error compensation (2.675 -> 268)",
              QString::number(c.raw()));
    }
    // ms types truncate (spec §6)
    {
        SplitMilliseconds ms;
        SplitMilliseconds::fromDouble(123.9, &ms);
        check(ms.raw() == 123, "split ms truncates toward zero (123.9 -> 123)");
        SplitMilliseconds::fromDouble(-123.9, &ms);
        check(ms.raw() == -123, "negative ms truncates toward zero (-123.9 -> -123)");
        ElapsedMilliseconds el;
        check(ElapsedMilliseconds::fromDouble(5000.0, &el) == FixedPointStatus::Ok
                  && el.raw() == 5000,
              "elapsed ms exact");
    }
    // overflow + non-finite
    {
        ScoreTenths s;
        check(ScoreTenths::fromDouble(1.0e9, &s) == FixedPointStatus::Overflow,
              "i16 overflow detected");
        check(ScoreTenths::fromDouble(std::nan(""), &s) == FixedPointStatus::NotFinite,
              "NaN rejected");
        check(ScoreTenths::fromDouble(INFINITY, &s) == FixedPointStatus::NotFinite,
              "Inf rejected");
        check(ScoreTenths::fromRaw64(40000, &s) == FixedPointStatus::Overflow,
              "raw64 narrowing overflow detected");
        check(ScoreTenths::fromRaw64(-40000, &s) == FixedPointStatus::Overflow,
              "raw64 negative narrowing overflow detected");
        CoordinateHundredthMm c;
        check(CoordinateHundredthMm::fromDouble(1.0e30, &c)
                  == FixedPointStatus::Overflow,
              "i32 overflow detected");
    }
    // round-trip: raw -> display double -> raw
    {
        for (qint16 raw : {qint16(0), qint16(104), qint16(-124), qint16(109)}) {
            ScoreTenths s(raw);
            ScoreTenths back;
            ScoreTenths::fromDouble(s.toDisplayDouble(), &back);
            check(back.raw() == raw, "score round-trip raw==display==raw",
                  QString::number(raw));
        }
        CoordinateHundredthMm c(-12345);
        CoordinateHundredthMm back;
        CoordinateHundredthMm::fromDouble(c.toDisplayDouble(), &back);
        check(back.raw() == -12345, "coordinate round-trip");
    }
    // serialization is the raw integer
    {
        ScoreTenths s(104);
        check(s.serialized() == 104, "serialized() is the raw integer");
        CentiDegrees d;
        CentiDegrees::fromDouble(287.34, &d);
        check(d.serialized() == 28734, "centidegrees serialize as integers",
              QString::number(d.serialized()));
    }
}
