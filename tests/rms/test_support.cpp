#include "test_support.h"

#include <cstdio>

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const char* name, const QString& detail)
{
    ++g_checks;
    if (ok) {
        std::printf("  PASS  %s\n", name);
    } else {
        ++g_failures;
        std::printf("  FAIL  %s%s%s\n", name,
                    detail.isEmpty() ? "" : "  -- ",
                    detail.isEmpty() ? "" : qPrintable(detail));
    }
    // Flush per check: a harness that loses its output on an abrupt teardown
    // teaches you nothing.
    std::fflush(stdout);
}

void check(bool ok, const QString& name, const QString& detail)
{
    check(ok, qPrintable(name), detail);
}
