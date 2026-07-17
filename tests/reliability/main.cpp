// Session Reliability Layer test harness (M0 storage + M1 core).
// Console binary, finals-harness pattern: PASS/FAIL per check, summary
// line, exit code 0 only when every check passed.
//
//   reliability_tests                  run everything
//   reliability_tests --write-fixtures regenerate the committed golden
//                                      fixtures (then still verify them)

#include "test_support.h"

#include <QCoreApplication>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const bool writeFixtures =
        app.arguments().contains(QStringLiteral("--write-fixtures"));

    std::printf("=== Session Reliability tests (M0 storage + M1 core) ===\n");

    run_storagepaths_tests();
    run_fixedpoint_tests();
    run_event_tests();
    run_serializer_tests();
    run_hashchain_tests();
    run_writer_tests();
    run_reader_tests();
    run_validator_tests();
    run_reducer_tests();
    run_snapshot_tests();
    run_fixture_tests(writeFixtures);

    std::printf("=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
