#include "test_support.h"

#include <cstdio>

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const char* name, const QString& detail)
{
    ++g_checks;
    if (!ok)
        ++g_failures;
    // Flushed per check: a harness that buffers loses its output to a crash,
    // and the last line before a crash is usually the one that matters.
    std::printf("  %s  %s%s\n", ok ? "PASS" : "FAIL", name,
                detail.isEmpty() ? ""
                                 : qUtf8Printable(QStringLiteral("  [%1]").arg(detail)));
    std::fflush(stdout);
}

void check(bool ok, const QString& name, const QString& detail)
{
    check(ok, qUtf8Printable(name), detail);
}
