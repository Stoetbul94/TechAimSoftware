// Tech Aim RMS-node harness.
//
// The physical session from 2026-09-05 is the whole point of this binary: it
// replays real evidence through the real conversion and the real RMS ingest.

#include "test_support.h"

#include <QCoreApplication>

#include <cstdio>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    std::printf("=== Tech Aim RMS-node tests (physical evidence) ===\n");
    run_physical_replay_tests();
    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
