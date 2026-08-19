#ifndef TA_RMS_TEST_SUPPORT_H
#define TA_RMS_TEST_SUPPORT_H

// Console harness support for the RMS read-only observer, following the
// established finals/reliability harness pattern: one binary, one check
// counter, per-module run functions, a summary line, exit code 0 only when
// every check passed.

#include <QString>

extern int g_checks;
extern int g_failures;

void check(bool ok, const char* name, const QString& detail = QString());
void check(bool ok, const QString& name, const QString& detail = QString());

void run_protocol_tests();
void run_monitor_tests();
void run_simulator_tests();
void run_readonly_tests();
void run_range_config_tests();
void run_match_plan_tests();
void run_competition_state_tests();
void run_udp_tests();

#endif // TA_RMS_TEST_SUPPORT_H
