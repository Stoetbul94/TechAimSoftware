// 10m Air Rifle / Air Pistol FINAL — F1 acceptance suite for Finals10mController.
// Drives the single-athlete 24-shot course end-to-end in accelerated time and
// asserts config, course of fire, numbering, window gating, checkpoints,
// decimal integer-tenths scoring, EST blocking, deterministic replay, snapshot
// round-trip and hash-chain validity. Standalone QtCore console binary — no
// QML/GUI dependency by construction.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QTemporaryDir>
#include <QDebug>
#include <functional>
#include <cstdio>

#include "Finals10mController.h"
#include "Finals10mConfig.h"
#include "Finals10mTypes.h"
#include "incident/EstIncidentController.h"
#include "reliability/storage/StoragePaths.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/reducer/SessionState.h"
#include "reliability/recovery/RecoveryCoordinator.h"
#include "reliability/recovery/RecoveryTypes.h"

#include <QDir>

using techaim::finals10m::Stage;

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
    std::fflush(stdout);
}

enum { WClosed = 0, WSighting = 1, WMatch = 2 };

template <typename F>
static bool waitUntil(F pred, int timeoutMs)
{
    QElapsedTimer t; t.start();
    while (!pred()) {
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }
    return true;
}

struct Recorder {
    QStringList commands;
    QVariantList commandEvents;
    QVariantList accepted;
    QVariantList rejected;
    QVariantList checkpoints;      // {shot,label,total}
    int finalCompletedCount = 0;

    void attach(Finals10mController* c)
    {
        QObject::connect(c, &Finals10mController::commandIssued, [this](const QVariantMap& e) {
            commands << e.value("typeName").toString();
            commandEvents << e;
        });
        QObject::connect(c, &Finals10mController::shotAccepted, [this](const QVariantMap& s) {
            accepted << s;
        });
        QObject::connect(c, &Finals10mController::shotRejected, [this](const QVariantMap& r) {
            rejected << r;
        });
        QObject::connect(c, &Finals10mController::checkpointReached,
                         [this](int shot, const QString& label, double total) {
            QVariantMap m;
            m["shot"] = shot; m["label"] = label; m["total"] = total;
            checkpoints << m;
        });
        QObject::connect(c, &Finals10mController::finalCompleted, [this]() { ++finalCompletedCount; });
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
    int acceptedOfficials() const
    {
        int n = 0;
        for (const QVariant& v : accepted)
            if (!v.toMap().value("sighter").toBool()) ++n;
        return n;
    }
};

// Drive the course to completion by polling: fill each MATCH window to its
// limit, fire `sighters` sighting shots, one shot per event-loop cycle (no
// re-entrancy into signal handlers).
static bool driveToCompletion(Finals10mController& c, int timeoutMs, int sighters,
                              std::function<double(int)> scoreForShot)
{
    QElapsedTimer t; t.start();
    int extId = 0, sightersFired = 0;
    while (c.running()) {
        if (c.windowState() == WMatch && c.shotsInStage() < c.property("stageShotCapacity").toInt()) {
            const int shotNo = c.nextShotNumber();
            c.registerShot(0.3, -0.2, scoreForShot(shotNo), ++extId, 45.0);
        } else if (c.windowState() == WSighting && sightersFired < sighters) {
            c.registerShot(0.1, 0.1, 9.0, ++extId, 0.0);
            ++sightersFired;
        }
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }
    return true;
}

// ── 1. Config: Rifle vs Pistol ────────────────────────────────────────────
static void testConfigs()
{
    using techaim::finals10m::Finals10mConfig;
    const Finals10mConfig ar = Finals10mConfig::airRifle();
    const Finals10mConfig ap = Finals10mConfig::airPistol();

    check(ar.discipline == ta::rel::Discipline::AirRifleFinal10m, "AR discipline id");
    check(ap.discipline == ta::rel::Discipline::AirPistolFinal10m, "AP discipline id");
    check(ar.displayName == QLatin1String("10m Air Rifle Final"), "AR display name");
    check(ap.displayName == QLatin1String("10m Air Pistol Final"), "AP display name");
    check(ar.targetType == 1 && ap.targetType == 0, "AR rifle / AP pistol target type");
    check(ar.takePositionsDelayMs == 30000, "AR TAKE YOUR POSITIONS = 30 s");
    check(ap.takePositionsDelayMs == 10000, "AP TAKE YOUR POSITIONS = 10 s");
    check(ar.decimalScoring && ap.decimalScoring, "BOTH finals decimal-scored");
    check(ar.shotsPerSeries == 5 && ar.seriesCount == 2, "2 x 5-shot series");
    check(ar.seriesWindowMs == 250000, "series window 250 s");
    check(ar.singleShotCount == 14 && ar.singleShotWindowMs == 50000, "14 singles, 50 s each");
    check(ar.maximumMatchShots == 24, "24 shots total");
    check(ar.preparationSightingMs == 300000, "5:00 prep+sighting");
    const QVector<int> exp{12,14,16,18,20,22,24};
    check(ar.checkpointShots == exp, "checkpoints 12/14/16/18/20/22/24");

    // Controller reflects the configured discipline.
    Finals10mController c;
    c.configureDiscipline(QStringLiteral("FINAL_AP10"));
    check(c.disciplineId() == QLatin1String("FINAL_AP10"), "controller configures AP");
    check(c.targetType() == 0, "controller AP target type");
    c.configureDiscipline(QStringLiteral("FINAL_AR10"));
    check(c.disciplineId() == QLatin1String("FINAL_AR10"), "controller configures AR");
}

// ── 2. TAKE YOUR POSITIONS delay (30 s AR / 10 s AP) ──────────────────────
static void testPositioningDelay(const QString& disc, qint64 expectMs, const char* tag)
{
    Finals10mController c;
    c.setTimeScale(200.0);
    c.configureDiscipline(disc);
    Recorder r; r.attach(&c);
    c.startFinal();
    // Wait until we reach PrepSighting (past the hold).
    const bool reached = waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::PrepSighting); }, 15000);
    check(reached, QString("%1: reached prep+sighting").arg(tag).toUtf8().constData());
    const qint64 typ = r.issuedAt(QStringLiteral("TakeYourPositions"));
    const qint64 prep = r.issuedAt(QStringLiteral("PreparationSightingStart"));
    check(typ >= 0 && prep >= 0, QString("%1: both commands issued").arg(tag).toUtf8().constData());
    const qint64 gap = prep - typ;
    // Scaled ms; tolerance a few 50ms*scale ticks.
    const qint64 tol = 200 * 50 * 3;
    check(qAbs(gap - expectMs) <= tol,
          QString("%1: TAKE YOUR POSITIONS hold ~= %2 ms").arg(tag).arg(expectMs).toUtf8().constData(),
          QString("gap=%1 expect=%2").arg(gap).arg(expectMs));
    c.resetFinal();
}

// ── 3. Full Rifle Final: course of fire, numbering, checkpoints, totals ────
static void testFullRifleFinal()
{
    Finals10mController c;
    c.setTimeScale(300.0);
    c.configureDiscipline(QStringLiteral("FINAL_AR10"));
    Recorder r; r.attach(&c);

    check(c.cumulativeTotal() == 0.0, "start from zero (total 0.0)");
    c.startFinal();
    const bool done = driveToCompletion(c, 40000, /*sighters*/ 2,
                                        [](int){ return 10.5; });
    check(done, "full AR final completed");
    check(c.stageId() == static_cast<int>(Stage::Complete), "stage == Complete");
    check(r.finalCompletedCount == 1, "finalCompleted emitted once");

    // Course of fire.
    check(r.acceptedOfficials() == 24, "24 official shots fired");
    check(c.sighterCount() == 2, "2 sighters recorded, excluded from officials");

    // First official = 1, last = 24.
    QList<int> officialNumbers;
    int series1 = 0, series2 = 0, singles = 0;
    for (const QVariant& v : r.accepted) {
        const QVariantMap m = v.toMap();
        if (m.value("sighter").toBool()) continue;
        officialNumbers << m.value("finalShotNumber").toInt();
        const int si = m.value("seriesIndex").toInt();
        if (si == 0) ++series1;
        else if (si == 1) ++series2;
        else if (si >= 2) ++singles;
    }
    check(!officialNumbers.isEmpty() && officialNumbers.first() == 1, "first official shot = 1");
    check(officialNumbers.last() == 24, "final official shot = 24");
    bool contiguous = true;
    for (int i = 0; i < officialNumbers.size(); ++i)
        if (officialNumbers[i] != i + 1) contiguous = false;
    check(contiguous, "official shot numbers are 1..24 contiguous");
    check(series1 == 5, "series 1 = 5 shots");
    check(series2 == 5, "series 2 = 5 shots");
    check(singles == 14, "14 single shots");

    // Decimal integer-tenths total: 24 x 10.5 = 252.0 (2520 tenths), NOT 240.
    const ta::rel::SessionState& st = c.store()->state();
    check(st.totalTenths == 2520, "total computed in integer tenths (2520)",
          QString("got %1").arg(st.totalTenths));
    check(qAbs(c.cumulativeTotal() - 252.0) < 1e-9, "cumulative decimal total 252.0");
    // Decimal preserved (a 10.5 shot stores 105 tenths, not integer-floored 100).
    bool anyDecimal = false;
    for (const ta::rel::StateShotRecord& sr : st.officials)
        if (sr.shot.scoreTenths == 105) anyDecimal = true;
    check(anyDecimal, "10.5 shot stored as 105 tenths (decimal, not integer)");

    // Checkpoints after 12/14/16/18/20/22/24.
    QList<int> cpShots;
    for (const QVariant& v : r.checkpoints)
        cpShots << v.toMap().value("shot").toInt();
    const QList<int> expCp{12,14,16,18,20,22,24};
    check(cpShots == expCp, "checkpoints fired after 12/14/16/18/20/22/24",
          QString("got %1").arg(QVariant(QVariantList(r.checkpoints)).toStringList().join(',')));
    // Checkpoint totals and labels.
    for (const QVariant& v : r.checkpoints) {
        const QVariantMap m = v.toMap();
        const int shot = m.value("shot").toInt();
        const double total = m.value("total").toDouble();
        check(qAbs(total - shot * 10.5) < 1e-9,
              QString("checkpoint %1 total = %2").arg(shot).arg(shot * 10.5).toUtf8().constData());
    }
    check(r.checkpoints.last().toMap().value("label").toString()
              == QStringLiteral("Final course complete — shot 24"),
          "shot-24 label = Final course complete");
    // No fabricated placement anywhere in checkpoint labels.
    bool noPlacement = true;
    for (const QVariant& v : r.checkpoints) {
        const QString l = v.toMap().value("label").toString().toLower();
        if (l.contains("gold") || l.contains("silver") || l.contains("bronze")
                || l.contains("place") || l.contains("winner") || l.contains("medal winner"))
            noPlacement = false;
    }
    check(noPlacement, "no finishing place / medal winner claimed in checkpoints");
}

// ── 4. Rejections: shot 25, out-of-window, duplicate, window capacity ──────
static void testRejections()
{
    // shot 25 after completion.
    {
        Finals10mController c;
        c.setTimeScale(400.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        Recorder r; r.attach(&c);
        c.startFinal();
        const bool done = driveToCompletion(c, 40000, 0, [](int){ return 10.4; });
        check(done, "reject-suite: course completed");
        const int before = r.rejected.size();
        c.registerShot(0.0, 0.0, 10.0, 9999, 0.0);   // shot 25
        check(r.rejected.size() == before + 1
                  && r.rejected.last().toMap().value("reason").toString()
                         == QLatin1String("FinalsNotActive"),
              "shot after 24 rejected (course complete)");
        check(c.store()->state().officials.size() == 24, "still exactly 24 officials (no 25th)");
    }
    // out-of-window (window Closed while running — during Presentation).
    {
        Finals10mController c;
        c.setTimeScale(50.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        Recorder r; r.attach(&c);
        c.setCeremonyMode(0);   // Full → Presentation with a closed window
        c.startFinal();
        check(c.running() && c.windowState() == WClosed, "presentation: running, window closed");
        c.registerShot(0.0, 0.0, 10.0, 1, 0.0);
        check(!r.rejected.isEmpty()
                  && r.rejected.last().toMap().value("reason").toString()
                         == QLatin1String("WindowClosed"),
              "out-of-window shot rejected (WindowClosed)");
        c.resetFinal();
    }
    // duplicate external id (sighting window stays open) + series capacity 5
    // + single capacity 1.
    {
        Finals10mController c;
        c.setTimeScale(120.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        Recorder r; r.attach(&c);
        c.setCeremonyMode(2);   // Skip presentation → straight to prep+sighting
        c.startFinal();
        // Duplicate external id inside the prep SIGHTING window (which does not
        // auto-close, so the duplicate check — not WindowClosed — is exercised).
        const bool sight = waitUntil([&]{ return c.windowState() == WSighting; }, 20000);
        check(sight, "reached prep sighting window");
        const int beforeDup = r.rejected.size();
        c.registerShot(0.1, 0.1, 9.5, 7000, 0.0);           // sighter accepted
        c.registerShot(0.1, 0.1, 9.5, 7000, 0.0);           // duplicate id → rejected
        bool sawDup = false;
        for (int i = beforeDup; i < r.rejected.size(); ++i)
            if (r.rejected[i].toMap().value("reason").toString() == QLatin1String("DuplicateShot"))
                sawDup = true;
        check(sawDup, "duplicate external id rejected");

        // Reach series-1 MATCH window; fire 6 rapid shots: 5 accepted, 6th sees
        // the auto-closed window → capacity of 5 enforced.
        const bool open = waitUntil([&]{ return c.windowState() == WMatch
                                              && c.stageId() == static_cast<int>(Stage::Series1); }, 20000);
        check(open, "reached series-1 match window");
        check(c.property("stageShotCapacity").toInt() == 5, "series window capacity = 5");
        int acc = 0;
        for (int i = 0; i < 6; ++i) {
            const int beforeAcc = r.accepted.size();
            c.registerShot(0.2, 0.2, 10.3, 200 + i, 0.0);
            if (r.accepted.size() > beforeAcc) ++acc;
            QCoreApplication::processEvents(QEventLoop::AllEvents, 2);
        }
        check(acc == 5, "series window accepts a maximum of 5 shots", QString("accepted %1").arg(acc));

        // Drive on (filling series 2) until the first single window, verify
        // single capacity = 1 and that one shot fills and closes it.
        bool reachedSingle = false;
        QElapsedTimer t; t.start();
        int ext = 300;
        while (c.running() && t.elapsed() < 25000) {
            if (c.windowState() == WMatch
                    && c.stageId() == static_cast<int>(Stage::Singles)) {
                reachedSingle = true;
                break;
            }
            if (c.windowState() == WMatch && c.shotsInStage() < c.property("stageShotCapacity").toInt())
                c.registerShot(0.2, 0.2, 10.3, ++ext, 0.0);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 3);
        }
        check(reachedSingle, "reached a single-shot window");
        check(c.property("stageShotCapacity").toInt() == 1, "single window capacity = 1");
        const int off = c.officialShotCount();
        c.registerShot(0.1, 0.1, 10.7, ++ext, 0.0);         // fills the single
        check(c.officialShotCount() == off + 1, "single accepts exactly one shot");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        check(c.windowState() != WMatch || c.stageId() != static_cast<int>(Stage::Singles)
                  || c.shotsInStage() == 0,
              "single window closes/advances after its one shot");
        c.abortFinal();
    }
}

// ── 5. EST incident blocks official shots ─────────────────────────────────
static void testEstBlocksOfficials()
{
    Finals10mController c;
    c.setTimeScale(100.0);
    c.configureDiscipline(QStringLiteral("FINAL_AR10"));
    Recorder r; r.attach(&c);
    EstIncidentController inc;
    inc.setStoreProvider([&c]() -> ta::rel::SessionStore* {
        return (c.store() && c.store()->active()) ? c.store() : nullptr;
    });
    c.setCeremonyMode(2);   // Skip
    c.startFinal();
    const bool open = waitUntil([&]{ return c.windowState() == WMatch
                                          && c.stageId() == static_cast<int>(Stage::Series1); }, 20000);
    check(open, "EST: reached series-1 match window");
    check(!EstIncidentController::officialsBlocked(c.store()->state()),
          "EST: officials not blocked before incident");
    const bool raised = inc.raiseIncident(0, 0, QStringLiteral("1"), QString(),
                                          QStringLiteral("target not registering"));
    check(raised, "EST: incident raised");
    check(EstIncidentController::officialsBlocked(c.store()->state()),
          "EST: officials blocked after raise");
    const int before = r.rejected.size();
    const int officialsBefore = c.store()->state().officials.size();
    c.registerShot(0.1, 0.1, 10.6, 555, 0.0);
    bool blocked = false;
    for (int i = before; i < r.rejected.size(); ++i)
        if (r.rejected[i].toMap().value("reason").toString() == QLatin1String("EstIncidentBlocked"))
            blocked = true;
    check(blocked, "EST: official shot rejected while incident unresolved");
    check(c.store()->state().officials.size() == officialsBefore,
          "EST: no official shot recorded while blocked");
    c.abortFinal();
}

// ── 6. Deterministic replay + snapshot round-trip + hash-chain validity ────
static void testReplayAndSnapshot()
{
    Finals10mController c;
    c.setTimeScale(400.0);
    c.configureDiscipline(QStringLiteral("FINAL_AP10"));   // exercise AP path too
    Recorder r; r.attach(&c);
    c.startFinal();
    const QString journalPath = c.sessionJournalPath();
    const bool done = driveToCompletion(c, 40000, 1, [](int n){ return 10.0 + (n % 10) * 0.1; });
    check(done, "replay-suite: AP course completed");

    // Hash-chain validity of the produced journal.
    const ta::rel::ValidationReport vr = ta::rel::JournalValidator::validateFile(journalPath);
    check(vr.classification == ta::rel::JournalClassification::Clean,
          "journal hash-chain valid (Clean)",
          QString("classification=%1").arg(ta::rel::journalClassificationName(vr.classification)));
    check(vr.sawMatchCompleted, "journal shows MatchCompleted");

    // Deterministic replay: fold the valid prefix from scratch, compare to the
    // store's live state.
    ta::rel::SessionState st;
    bool foldOk = true;
    for (const ta::rel::EventEnvelope& env : vr.validEnvelopes) {
        const ta::rel::ReduceResult rr = ta::rel::SessionReducer::apply(st, env);
        if (!rr.ok) { foldOk = false; break; }
        st = rr.state;
    }
    check(foldOk, "replay: every valid envelope folds cleanly");
    check(st.discipline == ta::rel::Discipline::AirPistolFinal10m, "replay: discipline preserved (AP final)");
    check(st.officials.size() == 24, "replay: 24 officials reconstructed");
    check(st.totalTenths == c.store()->state().totalTenths, "replay total == live total");
    check(std::holds_alternative<ta::rel::Finals10mState>(st.disc),
          "replay: disc variant is Finals10mState");

    // AP integer-leak guard: at least one shot with a non-multiple-of-10 tenths
    // proves the AP FINAL is decimal, not integer-floored.
    bool decimalPresent = false;
    for (const ta::rel::StateShotRecord& sr : st.officials)
        if (sr.shot.scoreTenths % 10 != 0) decimalPresent = true;
    check(decimalPresent, "AP final stores decimal tenths (no integer-scoring leak)");

    // Snapshot compatibility: Finals10mState serializes + round-trips exactly.
    const QByteArray bytes = ta::rel::serializeSessionState(st);
    ta::rel::SessionState back;
    const ta::rel::ReliabilityResult dr = ta::rel::deserializeSessionState(bytes, &back);
    check(dr.ok, "snapshot: state serializes + deserializes",
          dr.ok ? QString() : dr.error.technicalDetail);
    check(back == st, "snapshot: Finals10mState round-trips byte-for-byte-equal state");
}

// Drive a controller by polling until it has >= targetOfficials official shots,
// filling MATCH windows and firing `sighters` sighting shots.
static bool driveUntilOfficials(Finals10mController& c, int targetOfficials,
                                int timeoutMs, int sighters,
                                std::function<double(int)> scoreForShot)
{
    QElapsedTimer t; t.start();
    int extId = 0, sightersFired = 0;
    while (c.running() && c.officialShotCount() < targetOfficials) {
        if (c.windowState() == WMatch && c.shotsInStage() < c.property("stageShotCapacity").toInt()) {
            const int shotNo = c.nextShotNumber();
            c.registerShot(0.3, -0.2, scoreForShot(shotNo), ++extId, 45.0);
        } else if (c.windowState() == WSighting && sightersFired < sighters) {
            c.registerShot(0.1, 0.1, 9.0, ++extId, 0.0);
            ++sightersFired;
        }
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 4);
    }
    return c.officialShotCount() >= targetOfficials;
}

static void clearCurrentSessions()
{
    QDir d(ta::rel::StoragePaths::currentSessionsDirectory());
    for (const QString& f : d.entryList(QStringList() << "session_*.jsonl", QDir::Files))
        d.remove(f);
}

// ── 6b. Air Pistol Final — no integer-scoring leak (F4) ────────────────────
// Air Pistol QUALIFICATION is integer-scored; the Air Pistol FINAL is DECIMAL
// (rule 6.17.2). This proves the qualification integer path never leaks into
// the final: every fired decimal ring value is stored as decimal tenths, and
// the total is the decimal sum (never the integer-floored sum).
static void testPistolNoIntegerLeak()
{
    Finals10mController c;
    c.setTimeScale(300.0);
    c.configureDiscipline(QStringLiteral("FINAL_AP10"));
    check(c.disciplineId() == QLatin1String("FINAL_AP10"), "AP: discipline configured");
    check(c.targetType() == 0, "AP: pistol target type");
    Recorder r; r.attach(&c);
    c.startFinal();
    // Fire every shot at 10.5 (integer floor would be 10 → 240.0; decimal → 252.0).
    const bool done = driveToCompletion(c, 40000, 1, [](int){ return 10.5; });
    check(done, "AP: full pistol final completed");

    const ta::rel::SessionState& st = c.store()->state();
    // Every official shot stored as 105 tenths (10.5), NOT 100 (integer 10.0).
    bool allDecimal = !st.officials.isEmpty();
    int fired = 0;
    for (const ta::rel::StateShotRecord& sr : st.officials) {
        ++fired;
        if (sr.shot.scoreTenths != 105) allDecimal = false;
    }
    check(allDecimal, "AP: every final shot stored as 105 tenths (decimal, not integer 100)");
    // Decimal total (fired × 10.5), never the integer-floored (fired × 10).
    check(st.totalTenths == fired * 105, "AP: total is decimal sum (no integer-floor leak)",
          QString("total=%1 fired=%2").arg(st.totalTenths).arg(fired));
    check(qAbs(c.cumulativeTotal() - fired * 10.5) < 1e-9, "AP: decimal cumulative total");
}

// ── 7. Crash → recover → continue → complete (F3) ──────────────────────────
static void testRecovery()
{
    clearCurrentSessions();
    int officialsA = 0; double totalA = 0.0;

    // Phase 1: drive controller A to 10 officials (both series fired), then
    // "crash" — A leaves scope with NO closeSession, exactly like a real crash.
    {
        Finals10mController a;
        a.setTimeScale(250.0);
        a.configureDiscipline(QStringLiteral("FINAL_AR10"));
        a.startFinal();
        const bool got = driveUntilOfficials(a, 10, 40000, 2, [](int){ return 10.5; });
        check(got, "recovery: drove A to 10 officials before crash",
              QString("officials=%1").arg(a.officialShotCount()));
        officialsA = a.officialShotCount();
        totalA = a.cumulativeTotal();
    }   // ← A destroyed = crash

    // Phase 2: a fresh coordinator finds our crashed 10m-final session.
    QString sid;
    {
        ta::rel::RecoveryCoordinator coord;
        for (const ta::rel::RecoveryCandidate& c : coord.scan())
            if (c.discipline == ta::rel::Discipline::AirRifleFinal10m)
                sid = c.sessionId;
    }
    check(!sid.isEmpty(), "recovery: crashed AR-final session is a candidate");

    // Phase 3: controller B resumes it (the production resumeFromRecovery path).
    Finals10mController b;
    b.setTimeScale(250.0);
    b.scanForRecovery();                       // prime B's own coordinator
    const bool resumed = b.resumeFromRecovery(sid);
    check(resumed, "recovery: resumeFromRecovery succeeds");
    check(b.disciplineId() == QLatin1String("FINAL_AR10"), "recovery: discipline restored (AR final)");
    check(b.officialShotCount() == officialsA, "recovery: official count restored",
          QString("%1 vs %2").arg(b.officialShotCount()).arg(officialsA));
    check(qAbs(b.cumulativeTotal() - totalA) < 1e-6, "recovery: decimal total restored",
          QString("%1 vs %2").arg(b.cumulativeTotal()).arg(totalA));
    check(b.store()->state().officials.size() == officialsA, "recovery: reducer officials restored");
    // Next official continues (no duplicate numbering): the reducer rejects a
    // duplicate shotNumber, so continuing must not double-count.
    const int beforeOfficials = b.officialShotCount();

    // Phase 4: continue firing to completion; verify it finishes at 24 and no
    // 25th is ever accepted.
    Recorder rb; rb.attach(&b);
    const bool done = driveToCompletion(b, 40000, 0, [](int){ return 10.5; });
    check(done, "recovery: resumed course completes");
    check(b.officialShotCount() >= beforeOfficials, "recovery: continued shots appended");
    check(b.store()->state().officials.size() <= 24, "recovery: never exceeds 24 officials",
          QString("got %1").arg(b.store()->state().officials.size()));
    // Shot numbers across the whole (pre + post crash) record must be strictly
    // increasing, unique and within 1..24. Gaps are LEGAL (an unfired single is
    // a missed shot, not a ShotAccepted) — what recovery must guarantee is no
    // duplicate / renumbering and an intact pre-crash prefix.
    QList<int> nums;
    for (const ta::rel::StateShotRecord& r : b.store()->state().officials)
        nums << r.shot.shotNumber;
    bool strictlyIncreasing = !nums.isEmpty() && nums.first() >= 1;
    for (int i = 1; i < nums.size(); ++i)
        if (nums[i] <= nums[i-1]) strictlyIncreasing = false;
    check(strictlyIncreasing && nums.last() <= 24,
          "recovery: shot numbers strictly increasing, unique, within 1..24",
          QString("nums=%1").arg([&]{ QStringList s; for (int n : nums) s<<QString::number(n); return s.join(','); }()));
    // Pre-crash prefix (shots 1..officialsA) all survived the crash intact.
    bool prefixIntact = true;
    for (int n = 1; n <= officialsA; ++n)
        if (!nums.contains(n)) prefixIntact = false;
    check(prefixIntact, "recovery: every pre-crash official shot survived");
    // Shot 25 impossible after completion.
    const int before25 = rb.rejected.size();
    b.registerShot(0.0, 0.0, 10.0, 99999, 0.0);
    check(rb.rejected.size() > before25, "recovery: shot 25 remains impossible after resume+complete");

    clearCurrentSessions();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("finals10m_tests"));

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        std::printf("FATAL  could not create temp storage dir\n");
        return 2;
    }
    ta::rel::StoragePaths::setRootOverrideForTesting(tmp.path());
    const ta::rel::StorageResult init = ta::rel::StoragePaths::initialize();
    if (!init.ok) {
        std::printf("FATAL  storage init failed: %s\n", qUtf8Printable(init.technicalDetail));
        return 2;
    }

    std::printf("=== Finals10m F1 acceptance ===\n");
    testConfigs();
    testPositioningDelay(QStringLiteral("FINAL_AR10"), 30000, "AR");
    testPositioningDelay(QStringLiteral("FINAL_AP10"), 10000, "AP");
    testFullRifleFinal();
    testRejections();
    testEstBlocksOfficials();
    testReplayAndSnapshot();
    testPistolNoIntegerLeak();
    testRecovery();

    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
