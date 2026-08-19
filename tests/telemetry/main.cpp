// RMS node telemetry — network harness.

#include <QCoreApplication>

#include <cstdio>

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const char* name, const QString& detail = QString());

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
    std::fflush(stdout);
}

void run_udp_sink_tests();

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    std::printf("=== Tech Aim node telemetry (network) ===\n");
    run_udp_sink_tests();
    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
