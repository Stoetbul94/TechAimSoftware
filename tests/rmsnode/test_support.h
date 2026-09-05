#ifndef TA_RMSNODE_TEST_SUPPORT_H
#define TA_RMSNODE_TEST_SUPPORT_H

// Same console-harness contract the other Tech Aim harnesses use: one binary,
// one check counter, a summary line, exit code 0 only when every check passed.

#include <QString>

extern int g_checks;
extern int g_failures;

void check(bool ok, const char* name, const QString& detail = QString());
void check(bool ok, const QString& name, const QString& detail = QString());

void run_physical_replay_tests();

#endif
