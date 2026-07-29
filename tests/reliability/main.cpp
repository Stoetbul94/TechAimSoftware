// Session Reliability Layer test harness (M0 storage + M1 core).
// Console binary, finals-harness pattern: PASS/FAIL per check, summary
// line, exit code 0 only when every check passed.
//
//   reliability_tests                  run everything
//   reliability_tests --write-fixtures regenerate the committed golden
//                                      fixtures (then still verify them)
//   reliability_tests --seed-windmap <absolute root>
//                                      write Wind Map review fixtures into an
//                                      ISOLATED data root (test tool; creates
//                                      DATA, never input) and exit

#include "test_support.h"

#include <QCoreApplication>

int seedWindMapSessions(const QString& root);

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const bool writeFixtures =
        app.arguments().contains(QStringLiteral("--write-fixtures"));

    // Review-fixture seeding runs INSTEAD of the tests: it drives the real
    // WindMapController to write genuine journals so a manual visual review
    // does not need hundreds of clicks to reach a 40-shot session.
    const int seedAt = app.arguments().indexOf(QStringLiteral("--seed-windmap"));
    if (seedAt >= 0) {
        const QString root = (seedAt + 1 < app.arguments().size())
                             ? app.arguments().at(seedAt + 1) : QString();
        return seedWindMapSessions(root);
    }

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
    run_incident_tests();
    run_qualification_tests();
    run_snapshot_tests();
    run_store_tests();
    run_recovery_tests();
    run_operatingmode_tests();
    run_capture_profile_tests();
    run_brand_package_tests();
    run_homepage_layout_tests();
    run_windmap_tests();
    run_windmap_recovery_tests();
    run_windmap_controller_tests();
    run_windmap_qml_tests();
    run_windmap_analytics_tests();
    run_training_parity_tests();
    run_fixture_tests(writeFixtures);

    std::printf("=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
