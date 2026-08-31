// B3 — the authoritative competition clock.
//
// These tests exist because the defect they cover cannot be reproduced by
// looking at the application: it only appears when Android stops delivering
// timer events, and a headless harness cannot be backgrounded. ManualClock
// makes it reproducible - "the UI ticked N times while M seconds passed" is
// just two numbers that need not agree.

#include "test_support.h"

#include "app/CompetitionClock.h"
#include "reliability/store/MonotonicClock.h"

#include <QtGlobal>
#include <cstdio>

using ta::CompetitionClock;
using ta::rel::ManualClock;

static void chk(bool ok, const char* name, const QString& detail = QString())
{
    check(ok, name, detail);
}

void run_competition_clock_tests()
{
    printf("\n--- B3 authoritative competition clock (CLOCK-AUTH-001) ---\n");
    fflush(stdout);

    // ── the defect, stated as a test ──────────────────────────────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);

        c.start(0);
        // 600 real seconds pass. The UI ticks ZERO times, because Android
        // backgrounded the activity for the whole ten minutes.
        mc.advance(600 * 1000);
        chk(c.elapsedSeconds() == 600,
            "CLOCK-AUTH-001: ten minutes backgrounded with NO ui tick still "
            "costs ten minutes of competition time",
            QString::number(c.elapsedSeconds()));
    }

    // ── a tick that fires late must not double-count ──────────────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(3 * 1000);
        const int a = c.elapsedSeconds();
        const int b = c.elapsedSeconds();   // the UI reads twice in one second
        chk(a == 3 && b == 3,
            "CLOCK-AUTH-001: reading the clock twice does not advance it",
            QString("%1/%2").arg(a).arg(b));
    }

    // ── whole seconds only: the countdown must not jump early ─────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(999);
        chk(c.elapsedSeconds() == 0,
            "CLOCK-AUTH-001: 999 ms is not yet a second");
        mc.advance(1);
        chk(c.elapsedSeconds() == 1,
            "CLOCK-AUTH-001: 1000 ms is");
    }

    // ── stop freezes where it actually is, not at the last tick ───────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(7400);            // 7.4 s
        c.stop();
        chk(c.elapsedSeconds() == 7,
            "CLOCK-AUTH-001: stop freezes the whole seconds actually elapsed",
            QString::number(c.elapsedSeconds()));
        mc.advance(60 * 1000);       // an hour of wall time while stopped
        chk(c.elapsedSeconds() == 7,
            "CLOCK-AUTH-001: a stopped clock accrues nothing",
            QString::number(c.elapsedSeconds()));
    }

    // ── pause/resume is not a restart ─────────────────────────────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(30 * 1000);
        c.stop();
        mc.advance(5 * 60 * 1000);   // paused for five minutes
        c.start(c.elapsedSeconds());  // resume, as the QML caller does
        mc.advance(10 * 1000);
        chk(c.elapsedSeconds() == 40,
            "CLOCK-AUTH-001: a paused match resumes at 30+10, it does not "
            "restart and does not bill the pause",
            QString::number(c.elapsedSeconds()));
    }

    // ── the Jury credit case: elapsed may legitimately go negative ─────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(120 * 1000);
        // Jury grants time: remaining is pushed above the original duration,
        // which the existing code expresses as a negative elapsed.
        c.reanchor(-60);
        chk(c.elapsedSeconds() == -60,
            "CLOCK-AUTH-001: a Jury credit may set a NEGATIVE elapsed and it "
            "is preserved, not clamped",
            QString::number(c.elapsedSeconds()));
        mc.advance(30 * 1000);
        chk(c.elapsedSeconds() == -30,
            "CLOCK-AUTH-001: and it counts up from there",
            QString::number(c.elapsedSeconds()));
    }

    // ── re-anchor while running: the assignment must survive the next read ─
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(0);
        mc.advance(50 * 1000);
        c.reanchor(200);             // restored from a saved match
        chk(c.elapsedSeconds() == 200,
            "CLOCK-AUTH-001: a restored elapsed value is not silently undone "
            "by the next read",
            QString::number(c.elapsedSeconds()));
        mc.advance(5 * 1000);
        chk(c.elapsedSeconds() == 205,
            "CLOCK-AUTH-001: and time continues from the restored value",
            QString::number(c.elapsedSeconds()));
        chk(c.running(),
            "CLOCK-AUTH-001: re-anchoring does not stop a running clock");
    }

    // ── starting from a non-zero anchor (crash recovery) ──────────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        c.start(1234);
        mc.advance(6 * 1000);
        chk(c.elapsedSeconds() == 1240,
            "CLOCK-AUTH-001: recovery starts from the recovered elapsed, not "
            "from zero",
            QString::number(c.elapsedSeconds()));
    }

    // ── a fresh clock is not running and reads zero ───────────────────────
    {
        ManualClock mc; mc.start();
        CompetitionClock c; c.setClockForTesting(&mc);
        chk(!c.running(), "CLOCK-AUTH-001: a fresh clock is not running");
        mc.advance(10 * 1000);
        chk(c.elapsedSeconds() == 0,
            "CLOCK-AUTH-001: and accrues nothing before it is started",
            QString::number(c.elapsedSeconds()));
    }
}
