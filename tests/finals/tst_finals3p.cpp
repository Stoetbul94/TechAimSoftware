// 3P FINAL — Phase A acceptance test suite for Finals3PController.
// Drives the controller end-to-end in accelerated time (TECHAIM_FINALS_TIMESCALE)
// and asserts the full command/stage ordering, warning timing, LOAD->START
// delays, shot numbering, window gating, rejections, pause/abort/reset and the
// structural no-scoring guarantee. Standalone console binary — the controller
// has NO access to any QML model or qualification timer by construction; these
// tests additionally assert its only outputs are signals/events.
//
// Timing note: the controller ticks every 50 ms of real time, so at timeScale S
// one tick is 50*S scaled ms. All timing assertions therefore use a tolerance
// of a few ticks; ordering assertions are exact.

#include <QApplication>
#include <QElapsedTimer>
#include <QVariantList>
#include <QDebug>
#include <cstdio>

#include <QFile>

#include "Finals3PController.h"
#include "Finals3PTypes.h"
#include "FinalsAudioService.h"

#include <QDir>

using techaim::finals::Stage;

static int g_failures = 0;
static int g_checks = 0;

static void check(bool ok, const char* name, const QString& detail = QString())
{
    ++g_checks;
    if (ok) {
        std::printf("PASS  %s\n", name);
    } else {
        ++g_failures;
        std::printf("FAIL  %s  %s\n", name, qUtf8Printable(detail));
    }
    // Crash-diagnosable output: without this, an abnormal exit loses
    // everything after the last 4K stdio block.
    std::fflush(stdout);
}

// Spin the event loop until pred() or timeout (real ms). Returns pred() result.
template <typename F>
static bool waitUntil(F pred, int timeoutMs)
{
    QElapsedTimer t; t.start();
    while (!pred()) {
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return true;
}

struct Recorder {
    QStringList commands;          // typeName sequence
    QVariantList commandEvents;    // full event maps
    QList<int> stageOrder;         // distinct stage entries
    QVariantList accepted;         // shotAccepted maps
    QVariantList rejected;         // shotRejected maps
    int finalCompletedCount = 0;
    int transitionRejectedCount = 0;
    int reportRequestedCount = 0;
    QVariantMap lastTransitionRejected;

    void attach(Finals3PController* c)
    {
        QObject::connect(c, &Finals3PController::commandIssued, [this](const QVariantMap& e) {
            commands << e.value("typeName").toString();
            commandEvents << e;
        });
        QObject::connect(c, &Finals3PController::phaseChanged, [this, c]() {
            if (stageOrder.isEmpty() || stageOrder.last() != c->stageId())
                stageOrder << c->stageId();
        });
        QObject::connect(c, &Finals3PController::shotAccepted, [this](const QVariantMap& s) {
            accepted << s;
        });
        QObject::connect(c, &Finals3PController::shotRejected, [this](const QVariantMap& r) {
            rejected << r;
        });
        QObject::connect(c, &Finals3PController::finalCompleted, [this]() { ++finalCompletedCount; });
        QObject::connect(c, &Finals3PController::reportRequested, [this]() { ++reportRequestedCount; });
        QObject::connect(c, &Finals3PController::transitionRejected, [this](const QVariantMap& i) {
            ++transitionRejectedCount;
            lastTransitionRejected = i;
        });
    }

    qint64 issuedAt(const QString& type, int occurrence = 1) const
    {
        int n = 0;
        for (const QVariant& v : commandEvents) {
            const QVariantMap m = v.toMap();
            if (m.value("typeName").toString() == type && ++n == occurrence)
                return m.value("issuedAt").toLongLong();
        }
        return -1;
    }
    int countOf(const QString& type) const
    {
        int n = 0;
        for (const QString& c : commands) if (c == type) ++n;
        return n;
    }
};

static bool inStage(Finals3PController& c, Stage s)
{
    return c.stageId() == static_cast<int>(s);
}

// ── The main end-to-end run: Full ceremony, athlete completes all 35 ────────
static void runFullFinal()
{
    Finals3PController c;
    check(qFuzzyCompare(c.timeScale(), 60.0), "env TECHAIM_FINALS_TIMESCALE applied");
    Recorder r; r.attach(&c);
    const qint64 tolMs = 12000;   // a few 50ms ticks at scale 60 (3s scaled each)

    // Idle: shots rejected, no state corruption from invalid calls.
    c.simulateShot();
    check(r.rejected.size() == 1 && c.stageId() == 0, "Idle: shot rejected, state intact");
    c.advanceStage1();
    check(c.targetMode() == 0, "Idle: advance ignored (transition rejected)");

    c.startFinal();
    check(inStage(c, Stage::Ceremony), "startFinal -> Ceremony (Full)");
    c.startFinal();   // repeated command must not corrupt
    check(inStage(c, Stage::Ceremony) && r.countOf("AthletesToLine") == 1,
          "repeated startFinal ignored");
    c.simulateShot();
    check(r.rejected.size() == 2
              && r.rejected.last().toMap().value("reason") == "WindowClosed",
          "Ceremony: shot rejected WindowClosed");

    // Ceremony -> prep: intro then TAKE YOUR POSITIONS then 30 s hold (separate
    // from the 5-minute prep timer).
    check(waitUntil([&]{ return inStage(c, Stage::KneelingPrepSight); }, 8000),
          "reached KneelingPrepSight");
    const qint64 tAth  = r.issuedAt("AthletesToLine");
    const qint64 tTake = r.issuedAt("TakeYourPositions");
    const qint64 tPrep = r.issuedAt("PreparationSightingStart");
    check(tAth >= 0 && tTake > tAth && tPrep > tTake, "ceremony command order");
    check(tTake - tAth >= 20000 && tTake - tAth < 20000 + tolMs,
          "intro duration before TAKE YOUR POSITIONS", QString::number(tTake - tAth));
    check(tPrep - tTake >= 30000 && tPrep - tTake < 30000 + tolMs,
          "30 s hold separate from 5:00 prep", QString::number(tPrep - tTake));

    check(c.windowState() == 1 && c.targetMode() == 0,
          "prep: SightingOpen, target SIGHTER");
    c.simulateShot();
    check(r.accepted.size() == 1 && r.accepted.last().toMap().value("sighter").toBool(),
          "prep sighter accepted without official number");
    c.advanceStage1();
    check(c.targetMode() == 0 && inStage(c, Stage::KneelingPrepSight),
          "prep: advance illegal (transition rejected, stage intact)");

    // Prep warning + end (auto-MATCH = the only automatic switch).
    check(waitUntil([&]{ return inStage(c, Stage::KneelingMatch); }, 10000),
          "prep -> KneelingMatch");
    const qint64 tWarn = r.issuedAt("ThirtySeconds", 1);
    check(tWarn - tPrep >= 270000 && tWarn - tPrep < 270000 + tolMs,
          "prep 30-second warning at 4:30", QString::number(tWarn - tPrep));
    check(c.targetMode() == 1, "auto-switch to MATCH at prep end (EST officer)");

    // Stage 1: MATCH FIRING START 5 s after the announcement; 22:00 clock.
    check(waitUntil([&]{ return c.windowState() == 2; }, 5000),
          "kneeling match window opened");
    const qint64 tAnn   = r.issuedAt("StageOneAnnouncement");
    const qint64 tStart = r.issuedAt("MatchFiringStart");
    check(tStart - tAnn >= 5000 && tStart - tAnn < 5000 + tolMs,
          "5 s announcement -> MATCH FIRING START", QString::number(tStart - tAnn));

    // Kneeling 1-10, then the 11th is hard-blocked.
    for (int i = 0; i < 10; ++i) c.simulateShot();
    check(c.stageLabel().endsWith("COMPLETE"),
          "HUD label flips to KNEELING COMPLETE on shot 10", c.stageLabel());
    check(c.primaryActionEnabled() && c.primaryActionLabel().startsWith("CHANGE TO PRONE"),
          "primary action ready the moment shot 10 lands", c.primaryActionLabel());
    c.simulateShot();   // 11th
    check(r.rejected.last().toMap().value("reason") == "StageShotLimitReached",
          "11th kneeling shot rejected StageShotLimitReached");
    check(r.rejected.last().toMap().value("displayText").toString().startsWith("KNEELING COMPLETE"),
          "post-completion rejection guides next action (athlete wording)");
    check(c.shotsInStage() == 10, "kneeling limited to 10");

    // Athlete-controlled transitions via the single stage-aware action.
    check(c.advanceLabel().startsWith("CHANGE TO PRONE") && c.advanceLabel().endsWith("SIGHTERS"),
          "advance label: kneeling", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::ProneSighting) && c.windowState() == 1,
          "advance -> ProneSighting (SightingOpen)");
    c.simulateShot();
    check(r.accepted.last().toMap().value("sighter").toBool(), "prone sighter accepted");
    check(c.advanceLabel() == QStringLiteral("START PRONE MATCH"),
          "advance label: prone sighting", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::ProneMatch) && c.windowState() == 2,
          "advance -> ProneMatch (MatchOpen)");
    for (int i = 0; i < 10; ++i) c.simulateShot();
    check(c.shotsInStage() == 10, "prone limited to 10");
    c.simulateShot();   // 21st in Prone Match
    check(r.rejected.last().toMap().value("displayText").toString().startsWith("PRONE COMPLETE"),
          "post-completion prone rejection guides next action");
    check(c.advanceLabel().startsWith("CHANGE TO STANDING") && c.advanceLabel().endsWith("SIGHTERS"),
          "advance label: prone match", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::StandingSighting), "advance -> StandingSighting");
    c.simulateShot();
    check(r.accepted.last().toMap().value("sighter").toBool(), "standing sighter accepted");
    check(c.advanceLabel().isEmpty(), "standing sighting: no manual advance", c.advanceLabel());
    check(!c.primaryActionEnabled() && c.primaryActionLabel().contains("WAIT FOR STOP"),
          "standing sighting: WAIT FOR STOP status", c.primaryActionLabel());
    {
        const int trBefore = r.transitionRejectedCount;
        c.advanceStage1();
        check(r.transitionRejectedCount == trBefore + 1
                  && inStage(c, Stage::StandingSighting) && c.windowState() == 1,
              "standing sighting: manual advance rejected, sighting stays open");
    }
    c.simulateShot();
    check(r.accepted.last().toMap().value("sighter").toBool(),
          "unlimited standing sighters until 22:00");

    // Stage-1 warnings and STOP on the continuous 22:00 clock.
    check(waitUntil([&]{ return inStage(c, Stage::StandingSeries1); }, 40000),
          "stage-1 STOP -> StandingSeries1");
    check(c.targetMode() == 1, "controller sets Match/Ready at the 22:00 STOP");
    const qint64 tW5  = r.issuedAt("FiveMinutes");
    const qint64 tW30 = r.issuedAt("ThirtySeconds", 2);
    check(tW5 - tStart >= 1020000 && tW5 - tStart < 1020000 + tolMs,
          "stage-1 FIVE MINUTES at 17:00", QString::number(tW5 - tStart));
    check(tW30 - tStart >= 1290000 && tW30 - tStart < 1290000 + tolMs,
          "stage-1 THIRTY SECONDS at 21:30", QString::number(tW30 - tStart));

    // Series 1: LOAD -> 5 s -> START -> 5 shots -> STOP.
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 1 window opened");
    const qint64 tL1 = r.issuedAt("LoadSeries", 1);
    const qint64 tS1 = r.issuedAt("StartSeries", 1);
    check(tS1 - tL1 >= 5000 && tS1 - tL1 < 5000 + tolMs,
          "series 1 LOAD->START 5 s", QString::number(tS1 - tL1));
    for (int i = 0; i < 5; ++i) c.simulateShot();
    check(inStage(c, Stage::StandingSeries2), "series 1 complete -> series 2");

    // Series 2: same cycle; elimination notice follows.
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 2 window opened");
    const qint64 tL2 = r.issuedAt("LoadSeries", 2);
    const qint64 tS2 = r.issuedAt("StartSeries", 2);
    check(tS2 - tL2 >= 5000 && tS2 - tL2 < 5000 + tolMs,
          "series 2 LOAD->START 5 s", QString::number(tS2 - tL2));
    for (int i = 0; i < 5; ++i) c.simulateShot();

    // Singles 31-35: five LOAD->START cycles, one-shot 50 s windows.
    for (int k = 0; k < 5; ++k) {
        check(waitUntil([&]{ return c.windowState() == 2; }, 10000),
              qUtf8Printable(QStringLiteral("single %1 window opened").arg(31 + k)));
        const qint64 tL = r.issuedAt("LoadSingle", k + 1);
        const qint64 tS = r.issuedAt("StartSingle", k + 1);
        check(tS - tL >= 5000 && tS - tL < 5000 + tolMs,
              qUtf8Printable(QStringLiteral("single %1 LOAD->START 5 s").arg(31 + k)));
        c.simulateShot();
        check(c.windowState() == 0,
              qUtf8Printable(QStringLiteral("single %1 window closed after 1 shot").arg(31 + k)));
    }

    check(waitUntil([&]{ return inStage(c, Stage::Complete); }, 5000), "reached Complete");
    check(r.finalCompletedCount == 1, "finalCompleted emitted once");

    // D3: the primary action flips to VIEW REPORT on completion; executing it
    // emits the reportRequested intent (QML opens the finals report tab) and
    // never re-enters the stage machine.
    check(c.primaryActionVisible() && c.primaryActionEnabled()
              && c.primaryActionLabel().startsWith("VIEW REPORT"),
          "D3: VIEW REPORT primary action after RESULTS ARE FINAL",
          c.primaryActionLabel());
    c.executePrimaryAction();
    check(r.reportRequestedCount == 1 && inStage(c, Stage::Complete),
          "D3: executePrimaryAction emits reportRequested, state intact");
    check(c.officialShotCount() == 35, "officialShotCount is 35 at completion");
    {
        // Phase C: every official stage carries a persisted Complete verdict.
        bool allComplete = true; QString bad;
        const int officials[] = { 3, 5, 7, 8, 9, 10, 11, 12, 13 };
        for (int id : officials)
            if (c.stageStatus(id) != 2) { allComplete = false; bad += QString::number(id) + " "; }
        check(allComplete, "all official stages persisted Complete", bad);
        check(c.missingShots().isEmpty(), "completion-driven run has no MissingShot records");
    }

    // Exact command ordering (structured events, not just final state).
    const QStringList expected = {
        "AthletesToLine", "TakeYourPositions", "PreparationSightingStart",
        "ThirtySeconds", "Stop", "StageOneAnnouncement", "MatchFiringStart",
        "FiveMinutes", "ThirtySeconds", "Stop",
        "LoadSeries", "StartSeries", "Stop",
        "LoadSeries", "StartSeries", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "Unload", "ResultsFinal"
    };
    check(r.commands == expected, "full command-event ordering",
          r.commands.join(","));

    // Exact stage-entry ordering.
    const QList<int> expectedStages = {
        static_cast<int>(Stage::Ceremony), static_cast<int>(Stage::KneelingPrepSight),
        static_cast<int>(Stage::KneelingMatch), static_cast<int>(Stage::ProneSighting),
        static_cast<int>(Stage::ProneMatch), static_cast<int>(Stage::StandingSighting),
        static_cast<int>(Stage::StandingSeries1), static_cast<int>(Stage::StandingSeries2),
        static_cast<int>(Stage::StandingSingle1), static_cast<int>(Stage::StandingSingle2),
        static_cast<int>(Stage::StandingSingle3), static_cast<int>(Stage::StandingSingle4),
        static_cast<int>(Stage::StandingSingle5), static_cast<int>(Stage::Complete)
    };
    check(r.stageOrder == expectedStages, "stage-entry ordering");

    // Continuous official numbering 1-35; sighters carry no numbers; every
    // event is flagged simulated; no synthetic shots exist.
    QList<int> numbers;
    int sighters = 0;
    bool allSimulated = true;
    for (const QVariant& v : r.accepted) {
        const QVariantMap m = v.toMap();
        if (!m.value("simulated").toBool()) allSimulated = false;
        if (m.value("sighter").toBool()) { ++sighters; continue; }
        numbers << m.value("finalShotNumber").toInt();
    }
    QList<int> expectNums;
    for (int i = 1; i <= 35; ++i) expectNums << i;
    check(numbers == expectNums, "official shot numbers 1..35 continuous");
    check(sighters == 4, "4 sighting shots accepted without numbers");
    check(allSimulated, "no scoring: every Phase-A event flagged simulated");

    // B1: schema completeness — every plan-§2 role on the FIRST accepted record.
    {
        const QVariantMap first = r.accepted.first().toMap();
        const QStringList roles = {
            "calculatedscore","score","xmm","ymm","direction","timestamp",
            "timeComsumed","position","finalsStageId","finalsStageLabel",
            "finalsPosition","finalsShotNumber","finalsWindowId",
            "finalsSeriesIndex","shotWithinStage","targetModeAtReceipt",
            "isSighter","isFinalsShot" };
        bool allPresent = true;
        QString missing;
        for (const QString& role : roles)
            if (!first.contains(role)) { allPresent = false; missing += role + " "; }
        check(allPresent, "shot record schema complete on first record", missing);
    }
    check(c.missingShots().isEmpty(), "complete final: no MissingShot records");

    // RMS hooks exist and do not disturb local state.
    const int stageBefore = c.stageId();
    c.startPhaseFromServer(QVariantMap());
    c.stopPhaseFromServer(QVariantMap());
    c.synchroniseClock(0, 0);
    c.applyAuthoritativeState(QVariantMap());
    check(c.stageId() == stageBefore, "RMS hook stubs do not break local state");

    // D1: report assembled from controller-stored state only (complete run).
    {
        QVariantMap meta;
        meta[QStringLiteral("athlete")] = QStringLiteral("TEST ATHLETE");
        const QVariantMap rep = c.buildReport(meta);
        const QVariantMap sum = rep.value("summary").toMap();
        check(rep.value("completionStatus") == "COMPLETE",
              "D1 complete run: completionStatus COMPLETE",
              rep.value("completionStatus").toString());
        check(sum.value("officialShotCount").toInt() == 35
                  && sum.value("sighterCount").toInt() == 4
                  && sum.value("missingCount").toInt() == 0
                  && sum.value("athlete") == "TEST ATHLETE",
              "D1 complete run: summary counts + meta passthrough");
        const QVariantList shots = rep.value("shots").toList();
        bool ordered = shots.size() == 35;
        for (int i = 0; ordered && i < 35; ++i) {
            const QVariantMap s = shots.at(i).toMap();
            if (s.value("number").toInt() != i + 1 || s.value("provisional").toBool())
                ordered = false;
        }
        check(ordered, "D1 complete run: 35 real shot rows ordered 1..35");
        const QVariantList stages = rep.value("stages").toList();
        bool stagesOk = stages.size() == 9;
        for (const QVariant& v : stages) {
            const QVariantMap st = v.toMap();
            if (st.value("fired").toInt() != st.value("expected").toInt()
                    || st.value("statusName") != "Complete")
                stagesOk = false;
        }
        check(stagesOk, "D1 complete run: 9 stages, fired==expected, all Complete");
        check(rep.value("timeline").toList().size() == r.commands.size(),
              "D1 complete run: timeline mirrors the full command history");
        check(c.officialShotRecords().size() == 35 && c.sighterCount() == 4,
              "D1: controller retained 35 official records + sighter count");
    }
}

// ── Ceremony modes, pause/resume, abort/reset, unfired shots ────────────────
static void runSecondaryChecks()
{
    // Short ceremony: no introductions, straight to the 30 s hold.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(1);
        Recorder r; r.attach(&c);
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::Ceremony)
                  && r.commands.first() == "TakeYourPositions",
              "Short ceremony: hold only, no introductions");
        c.abortFinal();
        check(c.stageId() == static_cast<int>(Stage::Aborted) && c.windowState() == 0,
              "abort -> Aborted, window closed");
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::Aborted),
              "startFinal ignored while Aborted");
        c.resetFinal();
        check(c.stageId() == static_cast<int>(Stage::Idle), "reset -> Idle");
    }

    // Skip ceremony: straight to preparation/sighting. Pause preserves time.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(2);
        Recorder r; r.attach(&c);
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::KneelingPrepSight)
                  && r.commands.first() == "PreparationSightingStart",
              "Skip ceremony: straight to prep/sighting");

        c.pauseTrainingSimulation();
        const qint64 remA = c.remainingMs();
        QElapsedTimer t; t.start();
        while (t.elapsed() < 400) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        const qint64 remB = c.remainingMs();
        check(remA == remB, "pause freezes remaining time",
              QString("%1 vs %2").arg(remA).arg(remB));
        c.resumeTrainingSimulation();
        t.restart();
        while (t.elapsed() < 300) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        check(c.remainingMs() < remB, "resume: countdown continues");
        check(!waitUntil([&]{ return false; }, 50) && c.paused() == false, "resume clears paused");

        // Unfired shots: fire only 3 kneeling shots; at 22:00 the window simply
        // closes — no synthetic 0.0 shots are fabricated ([P1] disabled).
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::KneelingMatch)
                                    && c.windowState() == 2; }, 10000),
              "unfired-run reached kneeling match");
        c.simulateShot(); c.simulateShot(); c.simulateShot();
        const int acceptedBefore = r.accepted.size();
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSeries1); }, 30000),
              "stage-1 expiry with unfired shots -> series 1");
        check(r.accepted.size() == acceptedBefore,
              "no synthetic shots created at expiry (P1=B: no model entries)");
        check(c.missingShots().size() == 17,
              "MissingShot records for 7 kneeling + 10 prone unfired",
              QString::number(c.missingShots().size()));
        c.abortFinal();
        c.registerShot(1, 1, 9.0, 50);
        check(r.rejected.last().toMap().value("reason") == "FinalsNotActive",
              "shot after abort rejected FinalsNotActive");
    }

    // Under-limit advance confirmation + registerShot validation chain.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(2);
        Recorder r; r.attach(&c);
        int confirmAsks = 0;
        QObject::connect(&c, &Finals3PController::advanceConfirmationRequired,
                         [&confirmAsks](int, int) { ++confirmAsks; });
        c.startFinal();
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::KneelingMatch)
                                    && c.windowState() == 2; }, 10000),
              "B1 run reached kneeling match window");

        c.registerShot(1.5, -2.0, 10.4, 100);
        check(!r.accepted.isEmpty()
                  && r.accepted.last().toMap().value("finalsShotNumber").toInt() == 1
                  && r.accepted.last().toMap().value("calculatedscore").toString() == "10.4",
              "registerShot accepted with schema values");
        c.registerShot(2.0, 2.0, 9.6, 100);
        check(r.rejected.last().toMap().value("reason") == "DuplicateShot",
              "repeated externalShotId rejected DuplicateShot");
        c.registerShot(0, 0, -1.0, 101);
        check(r.rejected.last().toMap().value("reason") == "InvalidShotData",
              "invalid score rejected InvalidShotData");

        // Hard-block workflow (FIX2): under-limit advance is rejected; there
        // is no production confirmation; devForceAdvanceStage1 is the only
        // (developer) bypass.
        check(c.primaryActionVisible() && !c.primaryActionEnabled()
                  && c.primaryActionLabel().startsWith("KNEELING")
                  && c.primaryActionLabel().contains("1 / 10"),
              "primary action shows progress, disabled under limit",
              c.primaryActionLabel());
        c.advanceStage1();
        check(r.transitionRejectedCount == 1
                  && r.lastTransitionRejected.value("reason") == "StageIncomplete"
                  && c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "under-limit advance hard-blocked (StageIncomplete)");
        c.executePrimaryAction();
        check(c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "disabled primary action is a no-op");
        c.confirmStage1Advance();
        check(c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "confirmStage1Advance inert under hard block");
        check(confirmAsks == 0, "no confirmation flow exists in production");

        // Duplicate identity (FIX1): a replayed id changes nothing; identical
        // coordinates with a NEW id are a new shot.
        {
            const int cntBefore = c.officialShotCount();
            const double totBefore = c.cumulativeTotal();
            c.registerShot(1.5, -2.0, 9.6, 100);   // replayed id
            check(c.officialShotCount() == cntBefore
                      && qFuzzyCompare(c.cumulativeTotal(), totBefore),
                  "duplicate rejection alters neither count nor total");
        }
        c.registerShot(1.5, -2.0, 9.6, 103);       // same coords, new id
        check(r.accepted.last().toMap().value("finalsShotNumber").toInt() == 2,
              "identical coordinates with a new event id accepted");
        check(r.accepted.last().toMap().value("xmm").toDouble() == 1.5
                  && r.accepted.last().toMap().value("ymm").toDouble() == -2.0,
              "xmm/ymm passthrough exact (no substitution, no inversion)");
        check(c.officialShotCount() == 2, "officialShotCount tracks accepts");

        c.devForceAdvanceStage1();
        check(c.stageId() == static_cast<int>(Stage::ProneSighting),
              "developer force-advance bypasses the hard block");
        // New sighting window resets the per-window duplicate guard: id 102
        // (lower than 103) is accepted because the window changed.
        c.registerShot(0.5, 0.5, 9.9, 102);
        check(r.accepted.last().toMap().value("isSighter").toBool()
                  && r.accepted.last().toMap().value("finalsShotNumber").toInt() == 0,
              "window-scoped duplicate state: new window accepts a new sequence");

        // B4: decimal totals over ACCEPTED OFFICIAL shots only.
        check(qFuzzyCompare(c.cumulativeTotal(), 20.0),
              "cumulative total exact (sighters excluded)",
              QString::number(c.cumulativeTotal()));
        check(qFuzzyCompare(c.stageSubtotal() + 1.0, 1.0),
              "stage subtotal reset on stage entry",
              QString::number(c.stageSubtotal()));

        // B4: template record carries the complete role union.
        {
            const QVariantMap tpl = c.templateShotRecord();
            const QStringList roles = {
                "calculatedscore","score","xmm","ymm","direction","timestamp",
                "timeComsumed","position","finalsStageId","finalsStageLabel",
                "finalsPosition","finalsShotNumber","finalsWindowId",
                "finalsSeriesIndex","shotWithinStage","targetModeAtReceipt",
                "isSighter","isFinalsShot" };
            bool ok = true; QString missing;
            for (const QString& role : roles)
                if (!tpl.contains(role)) { ok = false; missing += role + " "; }
            check(ok, "templateShotRecord covers the full role union", missing);
        }

        // B4: session journal exists and recorded the accepted shots.
        {
            QFile jf(QStringLiteral("finals_session.jsonl"));
            bool okOpen = jf.open(QIODevice::ReadOnly | QIODevice::Text);
            const QString all = okOpen ? QString::fromUtf8(jf.readAll()) : QString();
            check(okOpen && all.count(QStringLiteral("\"journalType\":\"shotAccepted\"")) == 3
                       && all.contains(QStringLiteral("\"journalType\":\"stageEntered\""))
                       && all.contains(QStringLiteral("\"journalType\":\"finalStarted\"")),
              "journal: one line per accepted shot + stage entries");
        }
        c.abortFinal();
    }
}

// ── Timeout-driven mode (Phase C): expiries instead of completions ─────────
static void runTimeoutFinal()
{
    Finals3PController c;
    c.setTimeScale(240.0);
    c.setCeremonyMode(2);   // skip ceremony
    Recorder r; r.attach(&c);
    int id = 1000;          // strictly increasing detection ids for this run

    c.startFinal();
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::KneelingMatch)
                                && c.windowState() == 2; }, 15000),
          "timeout run reached kneeling match window");
    check(c.property("stageShotCapacity").toInt() == 10, "kneeling capacity from controller",
          QString::number(c.property("stageShotCapacity").toInt()));

    // 3 kneeling shots at 9.5 each, then developer-forced through Stage 1.
    for (int i = 0; i < 3; ++i) c.registerShot(0.5, 0.5, 9.5, ++id);
    c.devForceAdvanceStage1();   // -> ProneSighting
    c.devForceAdvanceStage1();   // -> ProneMatch (0 shots)
    c.devForceAdvanceStage1();   // -> StandingSighting
    check(c.stageId() == static_cast<int>(Stage::StandingSighting),
          "dev-forced to standing sighting");

    // Stage-1 expiry: K Incomplete (3/10), P Incomplete (0/10), 17 missing.
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSeries1); }, 20000),
          "stage-1 expiry -> series 1");
    check(c.stageStatus(3) == 3 && c.stageStatus(5) == 3,
          "kneeling and prone persisted Incomplete");
    check(c.missingShots().size() == 17, "17 MissingShot records after stage 1",
          QString::number(c.missingShots().size()));

    // Shot during the announcer/LOAD phase (window still closed) is rejected.
    check(c.windowState() == 0, "series 1 window closed during LOAD phase");
    c.registerShot(0, 0, 9.0, ++id);
    check(r.rejected.last().toMap().value("reason") == "WindowClosed",
          "shot during LOAD rejected");

    // Series 1: two shots (21, 22), then the 250 s window expires.
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 1 window opened");
    check(c.property("stageShotCapacity").toInt() == 5, "series capacity from controller");
    c.registerShot(1.0, 1.0, 10.2, ++id);
    c.registerShot(-1.0, 0.5, 10.1, ++id);
    check(r.accepted.last().toMap().value("finalsShotNumber").toInt() == 22,
          "series 1 numbering 21-22");
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSeries2); }, 10000),
          "series 1 timeout -> series 2 (single transition)");
    check(c.stageStatus(7) == 3, "series 1 persisted Incomplete");
    check(c.missingShots().size() == 20, "MissingShot records 23-25 added",
          QString::number(c.missingShots().size()));
    {
        const QVariantMap last = c.missingShots().last().toMap();
        check(last.value("expectedShotNumber").toInt() == 25
                  && last.value("reason") == "TimeExpired",
              "missing shot numbers/reason correct");
    }
    check(qFuzzyCompare(c.cumulativeTotal(), 3 * 9.5 + 10.2 + 10.1),
          "cumulative exact after series-1 timeout", QString::number(c.cumulativeTotal()));

    // Series 2: completion-driven inside the timeout run (26-30, Complete).
    // Shot 27 fires ~72 scaled-seconds after 26 to pin the FIX-R3 per-shot
    // split semantics (26 = since window open; 27 = the gap; 28-30 ≈ 0 —
    // the old cumulative definition would give 28-30 the same large value).
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 2 window opened");
    c.registerShot(0.2, -0.2, 10.0, ++id);                          // shot 26
    {
        QElapsedTimer gap; gap.start();
        while (gap.elapsed() < 300) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    for (int i = 0; i < 4; ++i) c.registerShot(0.2, -0.2, 10.0, ++id);   // 27-30
    check(r.accepted.last().toMap().value("finalsShotNumber").toInt() == 30,
          "series 2 numbering 26-30");
    check(c.stageStatus(8) == 2, "series 2 persisted Complete");
    {
        auto splitOf = [&r](int shotNumber) {
            for (const QVariant& v : r.accepted) {
                const QVariantMap m = v.toMap();
                if (m.value("finalsShotNumber").toInt() == shotNumber)
                    return m.value("timeComsumed").toInt();
            }
            return -1;
        };
        check(splitOf(26) >= 0 && splitOf(26) <= 40,
              "FIX-R3: first shot of a window counts from window open",
              QString::number(splitOf(26)));
        check(splitOf(27) >= 48 && splitOf(27) <= 120,
              "FIX-R3: split measures the gap to the previous shot",
              QString::number(splitOf(27)));
        check(splitOf(30) >= 0 && splitOf(30) <= 30,
              "FIX-R3: per-shot split does NOT accumulate window time",
              QString::number(splitOf(30)));
    }

    // Single 31: no shot -> expiry -> Incomplete + one MissingShot; progresses.
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSingle2); }, 15000),
          "single 31 timeout -> single 32");
    check(c.stageStatus(9) == 3, "single 31 persisted Incomplete");
    check(c.missingShots().size() == 21, "one MissingShot for single 31");

    // Single 32: fired -> Complete, window closes after the single shot.
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSingle2)
                                && c.windowState() == 2; }, 10000),
          "single 32 window opened");
    check(c.property("stageShotCapacity").toInt() == 1, "single capacity from controller");
    c.registerShot(0.1, 0.1, 10.6, ++id);
    check(r.accepted.last().toMap().value("finalsShotNumber").toInt() == 32
              && r.accepted.last().toMap().value("shotWithinStage").toInt() == 1,
          "single 32 accepted with shotWithinStage 1");
    check(c.stageStatus(10) == 2, "single 32 persisted Complete");

    // Singles 33-35 all expire; the final still completes after stage 35.
    check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::Complete); }, 20000),
          "timeout run reaches Complete only after single 35");
    check(c.stageStatus(11) == 3 && c.stageStatus(12) == 3 && c.stageStatus(13) == 3,
          "singles 33-35 persisted Incomplete");
    check(c.missingShots().size() == 24, "24 MissingShot records total",
          QString::number(c.missingShots().size()));
    check(c.officialShotCount() == 11, "11 official shots accepted",
          QString::number(c.officialShotCount()));
    check(qFuzzyCompare(c.cumulativeTotal(), 3 * 9.5 + 10.2 + 10.1 + 5 * 10.0 + 10.6),
          "cumulative exact at completion", QString::number(c.cumulativeTotal()));
    check(r.countOf("Stop") == 9, "STOP exactly once per closed window (9 total)",
          QString::number(r.countOf("Stop")));
    check(r.commands.last() == "ResultsFinal"
              && r.commands.at(r.commands.size() - 2) == "Unload",
          "ending order STOP -> UNLOAD -> RESULTS ARE FINAL");

    // Journal order matches controller event order (commands as recorded).
    {
        QFile jf(QStringLiteral("finals_session.jsonl"));
        QStringList journalCmds;
        if (jf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!jf.atEnd()) {
                const QString line = QString::fromUtf8(jf.readLine());
                if (line.contains(QStringLiteral("\"journalType\":\"command\""))) {
                    const int i = line.indexOf(QStringLiteral("\"typeName\":\""));
                    if (i >= 0) {
                        const int st = i + 12;
                        journalCmds << line.mid(st, line.indexOf('"', st) - st);
                    }
                }
            }
        }
        check(journalCmds == r.commands, "journal command order matches controller order",
              QString::number(journalCmds.size()) + " vs " + QString::number(r.commands.size()));
    }

    // D1: report over an incomplete run — DNS rows merged per [P1 = Option B].
    {
        const QVariantMap rep = c.buildReport();
        const QVariantMap sum = rep.value("summary").toMap();
        check(rep.value("completionStatus") == "INCOMPLETE",
              "D1 timeout run: completionStatus INCOMPLETE",
              rep.value("completionStatus").toString());
        check(sum.value("officialShotCount").toInt() == 11
                  && sum.value("missingCount").toInt() == 24,
              "D1 timeout run: 11 official / 24 missing in summary");
        check(qFuzzyCompare(sum.value("cumulativeTotal").toDouble(),
                            3 * 9.5 + 10.2 + 10.1 + 5 * 10.0 + 10.6),
              "D1 timeout run: summary total matches controller total");
        const QVariantList shots = rep.value("shots").toList();
        bool ordered = shots.size() == 35;
        int provisional = 0;
        for (int i = 0; ordered && i < 35; ++i) {
            const QVariantMap s = shots.at(i).toMap();
            if (s.value("number").toInt() != i + 1)
                ordered = false;
            if (s.value("provisional").toBool()) {
                ++provisional;
                if (!qFuzzyIsNull(s.value("score").toDouble()))
                    ordered = false;   // DNS rows are 0.0 provisional
            }
        }
        check(ordered && provisional == 24,
              "D1 timeout run: 35 rows 1..35 with 24 provisional 0.0 DNS rows",
              QString("%1 rows, %2 provisional").arg(shots.size()).arg(provisional));
        // Real row values survive assembly: shot 32 was 10.6 decimal.
        bool shot32Ok = false;
        for (const QVariant& v : shots) {
            const QVariantMap s = v.toMap();
            if (s.value("number").toInt() == 32)
                shot32Ok = !s.value("provisional").toBool()
                        && qFuzzyCompare(s.value("score").toDouble(), 10.6);
        }
        check(shot32Ok, "D1 timeout run: real shot 32 keeps its 10.6 decimal score");
        const QVariantList stages = rep.value("stages").toList();
        bool kneelOk = false, s2Ok = false;
        for (const QVariant& v : stages) {
            const QVariantMap st = v.toMap();
            if (st.value("stageId").toInt() == 3)
                kneelOk = st.value("fired").toInt() == 3
                       && st.value("statusName") == "Incomplete"
                       && qFuzzyCompare(st.value("subtotal").toDouble(), 3 * 9.5);
            if (st.value("stageId").toInt() == 8)
                s2Ok = st.value("fired").toInt() == 5
                    && st.value("statusName") == "Complete";
        }
        check(kneelOk, "D1 timeout run: kneeling stage 3/10 Incomplete, subtotal 28.5");
        check(s2Ok, "D1 timeout run: series 2 stage 5/5 Complete");
        check(rep.value("incidents").toList().size() == r.rejected.size(),
              "D1 timeout run: every rejection appears as a report incident",
              QString::number(rep.value("incidents").toList().size()));
        // Immutable output: rebuilding from the same stored state is identical.
        check(c.buildReport() == rep, "D1: rebuild from unchanged state is identical");
    }
}

// ── Phase D4: deterministic audio service + ceremony polish ────────────────
static void runD4Checks()
{
    // Pure cue -> clip resolution: lower-cased, stateless, no randomness.
    check(FinalsAudioService::clipPathForCue("Stop", "C:/clips")
              == QStringLiteral("C:/clips/stop.wav"),
          "D4 audio: cue resolves to <dir>/<cueid>.wav (lower-case)",
          FinalsAudioService::clipPathForCue("Stop", "C:/clips"));
    check(FinalsAudioService::clipPathForCue("LoadSeries", "d")
              == FinalsAudioService::clipPathForCue("LoadSeries", "d"),
          "D4 audio: identical cue resolves identically (deterministic)");
    check(FinalsAudioService::clipPathForCue("", "C:/clips").isEmpty(),
          "D4 audio: empty cue resolves to nothing");

    // Fallback semantics: missing clip -> beep fallback; present clip -> clip.
    {
        FinalsAudioService svc;
        const QString dir = QStringLiteral("tst_audio_clips");
        QDir().mkpath(dir);
        QFile::remove(dir + QStringLiteral("/stop.wav"));
        svc.setClipsDir(dir);
        int played = 0; bool lastFb = false;
        QObject::connect(&svc, &FinalsAudioService::cuePlayed,
                         [&](const QString&, bool fb) { ++played; lastFb = fb; });
        svc.playCue(QStringLiteral("stop"));
        check(played == 1 && lastFb && svc.lastCueUsedFallback(),
              "D4 audio: missing clip -> system-beep fallback");
        {
            QFile f(dir + QStringLiteral("/stop.wav"));
            f.open(QIODevice::WriteOnly);
            f.write("RIFF");   // existence is what resolution keys on
        }
        svc.playCue(QStringLiteral("stop"));
        check(played == 2 && !lastFb && !svc.lastCueUsedFallback(),
              "D4 audio: present clip -> clip path, no fallback");
        svc.setEnabled(false);
        svc.playCue(QStringLiteral("stop"));
        check(played == 2, "D4 audio: disabled service plays nothing");
    }

    // Ceremony polish: with a configured athlete the Full ceremony announces
    // the name right after ATHLETES TO THE LINE; the cue ids stay lower-cased
    // command type names (the audio contract).
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setAthleteName(QStringLiteral("  Jane Doe "));
        Recorder r; r.attach(&c);
        c.startFinal();
        check(r.commands.size() >= 2
                  && r.commands.at(0) == "AthletesToLine"
                  && r.commands.at(1) == "InfoNotice",
              "D4 ceremony: introduction announced after ATHLETES TO THE LINE",
              r.commands.join(","));
        const QString intro = r.commandEvents.at(1).toMap().value("text").toString();
        check(intro.startsWith("INTRODUCING") && intro.endsWith("JANE DOE"),
              "D4 ceremony: athlete name announced upper-case, trimmed", intro);
        check(r.commandEvents.at(0).toMap().value("audioCueId").toString()
                  == "athletestoline",
              "D4 audio: command events carry lower-case audioCueId");
        c.abortFinal();
    }
    // No-name runs are covered by runFullFinal's exact command ordering: the
    // ceremony sequence there is byte-identical to Phase A (no InfoNotice).
}

int main(int argc, char** argv)
{
    qputenv("TECHAIM_FINALS_TIMESCALE", "60");
    qputenv("QT_QPA_PLATFORM", "minimal");
    QApplication app(argc, argv);

    std::printf("=== Finals3PController Phase A acceptance tests ===\n");
    runFullFinal();
    runSecondaryChecks();
    runTimeoutFinal();
    runD4Checks();

    std::printf("=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
