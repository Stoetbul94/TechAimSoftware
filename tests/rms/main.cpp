// Tech Aim RMS test harness — milestone 1, read-only observer.
//
//   QT_QPA_PLATFORM is irrelevant here: the harness is QT = core network, so
//   it needs no GUI platform plugin and cannot pop a modal dialog.

#include "test_support.h"

#include <QCoreApplication>

#include <cstdio>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    std::printf("=== Tech Aim RMS tests (milestone 1 - read-only observer) ===\n");

    run_protocol_tests();
    run_monitor_tests();
    run_simulator_tests();
    run_udp_tests();
    run_range_config_tests();
    run_match_plan_tests();
    run_competition_state_tests();
    run_readonly_tests();

    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
