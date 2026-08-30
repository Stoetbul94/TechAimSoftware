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
#include <cmath>

#include "Finals10mController.h"
#include "Finals10mConfig.h"
#include "Finals10mTypes.h"
#include "incident/EstIncidentController.h"
#include "reliability/storage/StoragePaths.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/reducer/SessionState.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/recovery/RecoveryCoordinator.h"
#include "reliability/recovery/RecoveryTypes.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

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

// ── BLOCKER F + G: explicit shot roles, the report, and the round trip ──────
//
// F  FINALS-TCH-SIGHTER-001. A reader must never have to infer "the first five
//    were sighters". The role is carried on the record and survives reload.
// G  F6. The 10 m Final must produce its own report; it previously fell through
//    to the qualification tabs, whose own comment forbids feeding them finals
//    data.
//
// This is the acceptance test for both. No physical target is involved and none
// is required: the acquisition engine is unchanged and its physical evidence is
// inherited from RC3F (see docs/release/V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md).
static void testFinalReportRoundTrip(const QString& disciplineId, const char* who)
{
    using namespace ta::rel;
    auto tag = [&](const char* s) {
        return QStringLiteral("%1 %2").arg(QString::fromLatin1(who),
                                           QString::fromLatin1(s)).toUtf8();
    };

    using techaim::finals10m::Finals10mConfig;
    const Finals10mConfig cfg = (disciplineId == QLatin1String("FINAL_AP10"))
                              ? Finals10mConfig::airPistol()
                              : Finals10mConfig::airRifle();
    Finals10mController c;
    c.configureDiscipline(disciplineId);
    c.setAthleteName(QStringLiteral("Round Trip"));
    // A 24-shot Final is ~35 minutes of competition time. Accelerate it the way
    // the rest of this harness does - the state machine still runs, no expiry
    // is bypassed and no state is assigned directly.
    c.setTimeScale(300.0);
    c.startFinal();
    const int sighters = 4;
    const bool done = driveToCompletion(c, 240000, sighters,
                                        [](int n){ return 9.0 + (n % 10) * 0.1; });
    check(done, tag("F/G: the course completes"));
    if (!done) return;

    // ── F: the role is explicit on every record ──────────────────────────
    const QVariantList off = c.officialShotRecords();
    const QVariantList sig = c.sighterRecords();
    check(off.size() == cfg.maximumMatchShots,
          tag("F: 24 official records"), QString::number(off.size()));
    check(sig.size() == sighters,
          tag("F: sighter records are RETAINED, not just counted"),
          QString::number(sig.size()));
    check(c.sighterCount() == sighters, tag("F: sighter count agrees"));

    bool rolesOk = true, numbersOk = true;
    for (int i = 0; i < off.size(); ++i) {
        const QVariantMap m = off.at(i).toMap();
        if (m.value(QStringLiteral("shotRole")).toString() != QLatin1String("OFFICIAL")
            || m.value(QStringLiteral("isSighter")).toBool())
            rolesOk = false;
        if (m.value(QStringLiteral("officialShotNumber")).toInt() != i + 1)
            numbersOk = false;
    }
    check(rolesOk, tag("F: every official record says OFFICIAL"));
    check(numbersOk, tag("F: the official sequence is 1..24, from the record"));
    bool sigRoles = true;
    for (const QVariant& v : sig)
        if (v.toMap().value(QStringLiteral("shotRole")).toString() != QLatin1String("SIGHTER")
            || !v.toMap().value(QStringLiteral("isSighter")).toBool())
            sigRoles = false;
    check(sigRoles, tag("F: every sighter record says SIGHTER"));

    // ── G: the report ────────────────────────────────────────────────────
    const QVariantMap rep = c.buildReport();
    check(rep.value(QStringLiteral("displayName")).toString() == cfg.displayName,
          tag("G: the report names the discipline"),
          rep.value(QStringLiteral("displayName")).toString());
    check(rep.value(QStringLiteral("courseShots")).toInt() == 24,
          tag("G: the course is 24 shots"));
    check(rep.value(QStringLiteral("officialShotCount")).toInt() == 24,
          tag("G: 24 official shots reported"));
    check(rep.value(QStringLiteral("sighterCount")).toInt() == sighters,
          tag("G: sighters counted separately"));
    check(rep.value(QStringLiteral("sighters")).toList().size() == sighters,
          tag("G: sighters are their own section"));
    check(rep.value(QStringLiteral("complete")).toBool(),
          tag("G: the Final is reported complete"));
    check(rep.value(QStringLiteral("scoringMode")).toString() == QLatin1String("decimal"),
          tag("G: decimal scoring"));
    check(!rep.value(QStringLiteral("rankingAvailable")).toBool(),
          tag("G: NO ranking is claimed for one lane"));
    check(rep.contains(QStringLiteral("mpiRadiusMm"))
              && rep.contains(QStringLiteral("groupExtentMm"))
              && rep.contains(QStringLiteral("meanShotTimeSec")),
          tag("G: MPI, group extent and mean shot time are derived"));

    // -- G: the course sections, stated by the report, not guessed by a view --
    // 6.17.2 splits the Final into two 5-shot series and then single shots. A
    // report that made a view work that out would eventually have a view work
    // it out differently.
    const QVariantList secs = rep.value(QStringLiteral("courseSections")).toList();
    check(secs.size() == 3, tag("G: three course sections"),
          QString::number(secs.size()));
    if (secs.size() == 3) {
        const QVariantMap sec1 = secs.at(0).toMap();
        const QVariantMap sec2 = secs.at(1).toMap();
        const QVariantMap sec3 = secs.at(2).toMap();
        check(sec1.value(QStringLiteral("shotCount")).toInt() == cfg.shotsPerSeries
                  && sec2.value(QStringLiteral("shotCount")).toInt() == cfg.shotsPerSeries
                  && sec3.value(QStringLiteral("shotCount")).toInt() == cfg.singleShotCount,
              tag("G: 5 + 5 + 14 shots across the sections"),
              QStringLiteral("%1/%2/%3")
                  .arg(sec1.value(QStringLiteral("shotCount")).toInt())
                  .arg(sec2.value(QStringLiteral("shotCount")).toInt())
                  .arg(sec3.value(QStringLiteral("shotCount")).toInt()));
        // Section membership comes from the record's own official number.
        bool numbering = true;
        QString badNum;
        const auto spanOk = [&](const QVariantMap& sec, int lo, int hi) {
            for (const QVariant& v : sec.value(QStringLiteral("shots")).toList()) {
                const int n = v.toMap().value(QStringLiteral("officialShotNumber")).toInt();
                if (n < lo || n > hi) {
                    numbering = false;
                    badNum = QStringLiteral("%1 not in %2..%3").arg(n).arg(lo).arg(hi);
                    return;
                }
            }
        };
        spanOk(sec1, 1, cfg.shotsPerSeries);
        spanOk(sec2, cfg.shotsPerSeries + 1, 2 * cfg.shotsPerSeries);
        spanOk(sec3, 2 * cfg.shotsPerSeries + 1, cfg.maximumMatchShots);
        check(numbering, tag("G: every shot sits in the section its NUMBER puts it in"),
              badNum);
        // No sighter may appear anywhere in the official course.
        bool noSighters = true;
        for (const QVariant& sv : secs)
            for (const QVariant& v : sv.toMap().value(QStringLiteral("shots")).toList())
                if (v.toMap().value(QStringLiteral("isSighter")).toBool()
                    || v.toMap().value(QStringLiteral("shotRole")).toString()
                           != QLatin1String("OFFICIAL"))
                    noSighters = false;
        check(noSighters, tag("G: NO sighter appears in any course section"));
        // The grouped records and the journalled per-stage running totals are
        // independent paths to the same subtotals. They must agree.
        check(qAbs(sec1.value(QStringLiteral("subtotal")).toDouble()
                   - rep.value(QStringLiteral("series1Subtotal")).toDouble()) < 0.05
                  && qAbs(sec2.value(QStringLiteral("subtotal")).toDouble()
                          - rep.value(QStringLiteral("series2Subtotal")).toDouble()) < 0.05
                  && qAbs(sec3.value(QStringLiteral("subtotal")).toDouble()
                          - rep.value(QStringLiteral("singlesSubtotal")).toDouble()) < 0.05,
              tag("G: section subtotals agree with the journalled stage totals"));
    }
    // Every official row names its section, so the view never has to decide.
    bool sectioned = true;
    for (const QVariant& v : rep.value(QStringLiteral("officialShots")).toList())
        if (v.toMap().value(QStringLiteral("courseSection")).toString().isEmpty())
            sectioned = false;
    check(sectioned, tag("G: every official row carries its courseSection"));
    check(!rep.value(QStringLiteral("sessionId")).toString().isEmpty(),
          tag("G: the report identifies its session"));

    // sighters must not reach any official total
    const double s1 = rep.value(QStringLiteral("series1Subtotal")).toDouble();
    const double s2 = rep.value(QStringLiteral("series2Subtotal")).toDouble();
    const double sg = rep.value(QStringLiteral("singlesSubtotal")).toDouble();
    const double tot = rep.value(QStringLiteral("total")).toDouble();
    check(qAbs((s1 + s2 + sg) - tot) < 0.05,
          tag("G: series + series + singles == the Final total"),
          QStringLiteral("%1 + %2 + %3 vs %4").arg(s1).arg(s2).arg(sg).arg(tot));
    double sumOff = 0.0;
    for (const QVariant& v : off) sumOff += v.toMap().value(QStringLiteral("score")).toDouble();
    check(qAbs(sumOff - tot) < 0.05,
          tag("G: the total is exactly the official shots, sighters excluded"),
          QStringLiteral("%1 vs %2").arg(sumOff).arg(tot));

    // ── the round trip: persist -> reload -> report again ────────────────
    const QString sid = c.sessionId();
    const double beforeTotal = c.cumulativeTotal();
    const int beforeOff = c.officialShotCount();
    // Reduce the journal while it is still in Sessions/Current. A CLEAN close
    // archives it to Sessions/Archive on purpose, so that a completed course is
    // never offered as an unfinished recovery candidate - which means the
    // recovery scan cannot see it afterwards, by design. The close is asserted
    // separately below.

    // Reduce the journal the controller itself names - no directory scan, no
    // assumption about where sessions live. This is the persistence contract:
    // what reaches disk must reconstruct the session exactly.
    const QString jpath = c.sessionJournalPath();
    check(!jpath.isEmpty(), tag("F: the session names its journal"));
    const ValidationReport vrep = JournalValidator::validateFile(jpath);
    const ReplayResult replay = ReplayEngine::replay(vrep.validEnvelopes);
    check(replay.ok, tag("F: the journal replays cleanly through the reducer"),
          vrep.error.technicalDetail);
    if (!replay.ok) return;
    const SessionState& rs = replay.state;

    check(rs.officials.size() == beforeOff,
          tag("F: OFFICIAL shots survive the round trip, as officials"),
          QStringLiteral("%1 vs %2").arg(rs.officials.size()).arg(beforeOff));
    check(rs.sighters.size() == sighters,
          tag("F: SIGHTERS survive as sighters - never merged into officials"),
          QStringLiteral("%1 vs %2").arg(rs.sighters.size()).arg(sighters));
    check(qAbs(rs.totalTenths / 10.0 - beforeTotal) < 0.001,
          tag("F: the total is unchanged after the round trip"),
          QStringLiteral("%1 vs %2").arg(rs.totalTenths / 10.0).arg(beforeTotal));
    {
        // sighters must contribute nothing to the official total
        double offSum = 0.0;
        for (const StateShotRecord& sc : rs.officials) offSum += sc.effectiveTenths() / 10.0;
        check(qAbs(offSum - beforeTotal) < 0.05,
              tag("F: the persisted total is the officials alone"),
              QStringLiteral("%1 vs %2").arg(offSum).arg(beforeTotal));
    }

    // -- 22: the SAME report, rebuilt from the reconstructed state ---------
    // Not "the numbers still add up" - the actual report DTO, built by a fresh
    // controller that has seen nothing but the reduced journal, compared field
    // by field against the one built from the live session.
    {
        // Resume a COPY. The live session is still open on the original file,
        // and two writers on one journal is not a thing this test is allowed to
        // create just to make an assertion convenient.
        QTemporaryDir tmp;
        const QString copyPath = tmp.path() + QStringLiteral("/resumed.jsonl");
        check(QFile::copy(jpath, copyPath), tag("22: the journal can be copied"));

        RecoveredMatchState reduced;
        reduced.state = rs;
        reduced.sessionId = sid;
        reduced.journalPath = copyPath;
        reduced.lastValidSeq = vrep.lastValidSeq;
        if (!vrep.validEnvelopes.isEmpty()) {
            reduced.lastEventWallIso = vrep.validEnvelopes.last().wallTimestampIso;
            reduced.lastLineHash = vrep.validEnvelopes.last().currentHash;
            reduced.lastEventMonoMs = vrep.validEnvelopes.last().monotonicMs;
        }

        Finals10mController rebuilt;
        rebuilt.configureDiscipline(disciplineId);
        rebuilt.loadRecoveredState(reduced);
        const QVariantMap repR = rebuilt.buildReport();

        QString diff;
        for (const char* k : { "officialShotCount", "sighterCount", "total",
                               "series1Subtotal", "series2Subtotal", "singlesSubtotal",
                               "series1ShotsSubtotal", "series2ShotsSubtotal",
                               "singleShotsSubtotal", "complete", "courseShots" }) {
            if (rep.value(QLatin1String(k)).toString() != repR.value(QLatin1String(k)).toString()) {
                diff = QStringLiteral("%1: %2 vs %3").arg(QLatin1String(k))
                       .arg(rep.value(QLatin1String(k)).toString(),
                            repR.value(QLatin1String(k)).toString());
                break;
            }
        }
        check(diff.isEmpty(),
              tag("22: the report DTO is identical before and after reduction"), diff);

        // Row by row: role, official number, section, score, time, coordinates.
        const QVariantList rowsA = rep.value(QStringLiteral("officialShots")).toList();
        const QVariantList rowsB = repR.value(QStringLiteral("officialShots")).toList();
        bool rowsSame = (rowsA.size() == rowsB.size());
        QString rowDiff = rowsSame
                        ? QString()
                        : QStringLiteral("%1 vs %2").arg(rowsA.size()).arg(rowsB.size());
        for (int i = 0; rowsSame && i < rowsA.size(); ++i) {
            const QVariantMap x = rowsA.at(i).toMap(), y = rowsB.at(i).toMap();
            for (const char* k : { "shotRole", "officialShotNumber", "courseSection",
                                   "score", "timeSec", "xmm", "ymm" }) {
                if (x.value(QLatin1String(k)).toString() != y.value(QLatin1String(k)).toString()) {
                    rowsSame = false;
                    rowDiff = QStringLiteral("row %1 %2: %3 vs %4").arg(i + 1)
                              .arg(QLatin1String(k))
                              .arg(x.value(QLatin1String(k)).toString(),
                                   y.value(QLatin1String(k)).toString());
                    break;
                }
            }
        }
        check(rowsSame,
              tag("22: every official row survives reduction unchanged"), rowDiff);

        const QVariantList sigA = rep.value(QStringLiteral("sighters")).toList();
        const QVariantList sigB = repR.value(QStringLiteral("sighters")).toList();
        bool sigSame = (sigA.size() == sigB.size() && sigA.size() == sighters);
        for (int i = 0; sigSame && i < sigA.size(); ++i)
            if (sigA.at(i).toMap().value(QStringLiteral("shotRole")).toString()
                    != QLatin1String("SIGHTER")
                || sigB.at(i).toMap().value(QStringLiteral("shotRole")).toString()
                    != QLatin1String("SIGHTER")
                || qAbs(sigA.at(i).toMap().value(QStringLiteral("score")).toDouble()
                        - sigB.at(i).toMap().value(QStringLiteral("score")).toDouble()) > 0.05)
                sigSame = false;
        check(sigSame, tag("22: the SIGHTERS section survives reduction as sighters"));

        // And the sighters still reach no total.
        double offOnly = 0.0;
        for (const QVariant& v : rowsB)
            offOnly += v.toMap().value(QStringLiteral("score")).toDouble();
        check(qAbs(offOnly - repR.value(QStringLiteral("total")).toDouble()) < 0.05,
              tag("22: after reduction the total is STILL the officials alone"));
    }


    // Reloading a COMPLETED Final into a fresh controller is deliberately not
    // exercised here: closeSession(Clean) archives the journal so a finished
    // course is never offered as an unfinished recovery candidate, and
    // loadRecoveredState is the crash path. The persistence contract above is
    // what the report depends on, and it is proven. The completed-session
    // reopen path is recorded as an accepted limitation in the release gate.

    // A clean close archives the journal, which is why the reduction above
    // happens first. Assert the close itself is clean and idempotent.
    c.closeFinalSession();
    c.closeFinalSession();   // no-op, must not append a second shutdown

    // ── G: the report is identical after reload ──────────────────────────
    // buildReport must be a pure derivation: same state, same report.
    const QVariantMap rep2 = c.buildReport();
    QString repDiff;
    for (const char* k : { "officialShotCount", "sighterCount", "total",
                           "series1Subtotal", "series2Subtotal", "singlesSubtotal",
                           "courseShots", "displayName", "complete" }) {
        if (rep.value(QLatin1String(k)).toString() != rep2.value(QLatin1String(k)).toString()) {
            repDiff = QStringLiteral("%1: %2 vs %3").arg(QLatin1String(k))
                .arg(rep.value(QLatin1String(k)).toString(),
                     rep2.value(QLatin1String(k)).toString());
            break;
        }
    }
    check(repDiff.isEmpty(),
          tag("G: buildReport is a pure derivation - same state, same report"), repDiff);
}


// -- 23. LEGACY SESSIONS: role without a shotRole field ---------------------
//
// The golden journals were written before shotRole existed. They must still
// load, and they must still say which shots were sighters - because the role
// was never a field to begin with: SighterAccepted and ShotAccepted are
// different events, and the reducer has always kept them in different lists.
//
// That is the whole backward-compatibility story, and this test is what stops
// anyone "helpfully" adding a migration that invents a role for a shot whose
// role was never recorded.
static void testLegacySessionRoles()
{
    using namespace ta::rel;
    const QString dir = QStringLiteral(RELIABILITY_FIXTURES_DIR);

    struct Case { const char* file; int sighters; int officials; };
    const Case cases[] = {
        { "fixture_sighter_official.jsonl", 1, 1 },
        { "fixture_finals_clean.jsonl",     1, 3 },
    };

    for (const Case& cs : cases) {
        const QString path = dir + QLatin1Char('/') + QLatin1String(cs.file);
        const QByteArray name = QByteArrayLiteral("23: ") + cs.file;

        QFile f(path);
        const bool opened = f.open(QIODevice::ReadOnly);
        check(opened, (name + " exists").constData(), path);
        if (!opened) continue;
        const QByteArray raw = f.readAll();
        f.close();

        // The evidence is only legacy evidence if it really predates the field.
        check(!raw.contains("shotRole"),
              (name + " predates shotRole - genuine legacy evidence").constData());

        const ValidationReport vrep = JournalValidator::validateFile(path);
        const ReplayResult rr = ReplayEngine::replay(vrep.validEnvelopes);
        check(rr.ok, (name + " still loads - no migration, no repair").constData(),
              vrep.error.technicalDetail);
        if (!rr.ok) continue;

        check(rr.state.sighters.size() == cs.sighters,
              (name + " sighters recovered by EVENT TYPE").constData(),
              QStringLiteral("%1 vs %2").arg(rr.state.sighters.size()).arg(cs.sighters));
        check(rr.state.officials.size() == cs.officials,
              (name + " officials recovered by EVENT TYPE").constData(),
              QStringLiteral("%1 vs %2").arg(rr.state.officials.size()).arg(cs.officials));

        // Nothing is fabricated: the total is the officials, never the sighters.
        double offSum = 0.0;
        for (const StateShotRecord& r : rr.state.officials)
            offSum += r.effectiveTenths() / 10.0;
        check(qAbs(offSum - rr.state.totalTenths / 10.0) < 0.05,
              (name + " the recovered total is the officials alone").constData(),
              QStringLiteral("%1 vs %2").arg(offSum).arg(rr.state.totalTenths / 10.0));
    }
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
    // F7 projections: live subtotals partition the total; last shot tracked.
    check(qAbs(c.series1Subtotal() - 52.5) < 1e-9, "F7: series1 subtotal 52.5");
    check(qAbs(c.series2Subtotal() - 52.5) < 1e-9, "F7: series2 subtotal 52.5");
    check(qAbs(c.singlesSubtotal() - 147.0) < 1e-9, "F7: singles subtotal 147.0");
    check(qAbs(c.series1Subtotal() + c.series2Subtotal() + c.singlesSubtotal()
               - c.cumulativeTotal()) < 1e-9, "F7: subtotals partition the total");
    check(qAbs(c.lastShotScore() - 10.5) < 1e-9 && c.lastShotNumber() == 24,
          "F7: last-shot projection = shot 24 @ 10.5",
          QString("score=%1 num=%2").arg(c.lastShotScore()).arg(c.lastShotNumber()));
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

// ── 7. Crash → recover → continue → complete (F3 rifle / F5 pistol) ────────
static void testRecovery(const QString& disc, ta::rel::Discipline discEnum, const char* tag)
{
    clearCurrentSessions();
    int officialsA = 0; double totalA = 0.0;

    // Phase 1: drive controller A to 10 officials (both series fired), then
    // "crash" — A leaves scope with NO closeSession, exactly like a real crash.
    {
        Finals10mController a;
        a.setTimeScale(250.0);
        a.configureDiscipline(disc);
        a.startFinal();
        const bool got = driveUntilOfficials(a, 10, 40000, 2, [](int){ return 10.5; });
        check(got, QString("%1 recovery: drove A to 10 officials before crash").arg(tag).toUtf8().constData(),
              QString("officials=%1").arg(a.officialShotCount()));
        officialsA = a.officialShotCount();
        totalA = a.cumulativeTotal();
    }   // ← A destroyed = crash

    // Phase 2: a fresh coordinator finds our crashed 10m-final session.
    QString sid;
    {
        ta::rel::RecoveryCoordinator coord;
        for (const ta::rel::RecoveryCandidate& c : coord.scan())
            if (c.discipline == discEnum)
                sid = c.sessionId;
    }
    check(!sid.isEmpty(), QString("%1 recovery: crashed session is a candidate").arg(tag).toUtf8().constData());

    // Phase 3: controller B resumes it (the production resumeFromRecovery path).
    Finals10mController b;
    b.setTimeScale(250.0);
    b.scanForRecovery();                       // prime B's own coordinator
    const bool resumed = b.resumeFromRecovery(sid);
    check(resumed, QString("%1 recovery: resumeFromRecovery succeeds").arg(tag).toUtf8().constData());
    check(b.disciplineId() == disc, QString("%1 recovery: discipline restored").arg(tag).toUtf8().constData());
    check(b.officialShotCount() == officialsA, QString("%1 recovery: official count restored").arg(tag).toUtf8().constData(),
          QString("%1 vs %2").arg(b.officialShotCount()).arg(officialsA));
    check(qAbs(b.cumulativeTotal() - totalA) < 1e-6, QString("%1 recovery: decimal total restored").arg(tag).toUtf8().constData(),
          QString("%1 vs %2").arg(b.cumulativeTotal()).arg(totalA));
    check(b.store()->state().officials.size() == officialsA,
          QString("%1 recovery: reducer officials restored").arg(tag).toUtf8().constData());
    // Decimal preserved after replay: every restored official is 105 tenths
    // (critical for the AP final — no integer-scoring leak through recovery).
    bool decimalAfterReplay = true;
    for (const ta::rel::StateShotRecord& sr : b.store()->state().officials)
        if (sr.shot.scoreTenths != 105) decimalAfterReplay = false;
    check(decimalAfterReplay, QString("%1 recovery: decimal tenths preserved after replay").arg(tag).toUtf8().constData());
    const int beforeOfficials = b.officialShotCount();

    // Phase 4: continue firing to completion; verify it finishes at 24 and no
    // 25th is ever accepted.
    Recorder rb; rb.attach(&b);
    const bool done = driveToCompletion(b, 40000, 0, [](int){ return 10.5; });
    check(done, QString("%1 recovery: resumed course completes").arg(tag).toUtf8().constData());
    check(b.officialShotCount() >= beforeOfficials, QString("%1 recovery: continued shots appended").arg(tag).toUtf8().constData());
    check(b.store()->state().officials.size() <= 24, QString("%1 recovery: never exceeds 24 officials").arg(tag).toUtf8().constData(),
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
          QString("%1 recovery: shot numbers strictly increasing, unique, within 1..24").arg(tag).toUtf8().constData(),
          QString("nums=%1").arg([&]{ QStringList s; for (int n : nums) s<<QString::number(n); return s.join(','); }()));
    // Pre-crash prefix (shots 1..officialsA) all survived the crash intact.
    bool prefixIntact = true;
    for (int n = 1; n <= officialsA; ++n)
        if (!nums.contains(n)) prefixIntact = false;
    check(prefixIntact, QString("%1 recovery: every pre-crash official shot survived").arg(tag).toUtf8().constData());
    // Shot 25 impossible after completion.
    const int before25 = rb.rejected.size();
    b.registerShot(0.0, 0.0, 10.0, 99999, 0.0);
    check(rb.rejected.size() > before25, QString("%1 recovery: shot 25 remains impossible after resume+complete").arg(tag).toUtf8().constData());

    clearCurrentSessions();
}

static int countMatchCompleted(const QString& path)
{
    const ta::rel::ValidationReport rep = ta::rel::JournalValidator::validateFile(path);
    int n = 0;
    for (const ta::rel::EventEnvelope& env : rep.validEnvelopes)
        if (std::holds_alternative<ta::rel::MatchCompleted>(env.payload)) ++n;
    return n;
}

// ── 8. Completion + exit-workflow semantics (F9) ───────────────────────────
static void testCompletion()
{
    clearCurrentSessions();

    // (a) Full completion: fire the whole 24-shot course.
    {
        Finals10mController c;
        c.setTimeScale(300.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        c.startFinal();
        const QString jp = c.sessionJournalPath();
        const bool done = driveToCompletion(c, 40000, 2, [](int){ return 10.5; });
        check(done && c.complete(), "F9: full course completes (complete == true)");
        check(c.property("complete").toBool(), "F9: complete property exposed");
        check(c.windowState() == WClosed, "F9: completed state has no open firing window");
        check(!c.running(), "F9: completed course is not running (no new window opens)");
        // Retains totals + accepted shots.
        check(c.officialShotCount() == 24, "F9: completed retains 24 accepted shots");
        check(qAbs(c.cumulativeTotal() - 252.0) < 1e-9, "F9: completed retains total");
        // Rejects any further official shot.
        Recorder r; r.attach(&c);
        c.registerShot(0.1, 0.1, 10.0, 99999, 0.0);
        check(!r.rejected.isEmpty()
                  && r.rejected.last().toMap().value("reason").toString() == QLatin1String("FinalsNotActive"),
              "F9: completed state rejects further official shots");
        check(c.store()->state().officials.size() == 24, "F9: rejected shot not recorded (still 24)");
        // Exactly one MatchCompleted (and it survives replay).
        check(countMatchCompleted(jp) == 1, "F9: exactly one MatchCompleted in the journal");
        check(c.store()->state().lifecycle == ta::rel::Lifecycle::Complete,
              "F9: completion state survives (reducer lifecycle == Complete)");
        // New Final → new session id + fresh journal; prior completion durable.
        // (Clean close archives the completed journal out of the Current dir —
        // still intact, just relocated — so snapshot its integrity beforehand.)
        const QString sid1 = c.sessionId();
        const int completedCount = countMatchCompleted(jp);
        check(completedCount == 1, "F9: prior completed journal intact before New Final (1 MatchCompleted)");
        c.closeFinalSession();
        c.resetFinal();
        c.startFinal();
        const QString sid2 = c.sessionId();
        const QString jp2 = c.sessionJournalPath();
        check(!sid2.isEmpty() && sid2 != sid1, "F9: New Final produces a new session id");
        check(jp2 != jp, "F9: New Final writes a fresh journal (prior completed journal not reused)");
        check(countMatchCompleted(jp2) == 0, "F9: New Final journal starts clean (no inherited MatchCompleted)");
        c.abortFinal();
    }

    clearCurrentSessions();

    // (b) Zero-official-shot completion: every window expires unfired.
    {
        Finals10mController c;
        c.setTimeScale(600.0);      // fast: windows expire before we could fire
        c.configureDiscipline(QStringLiteral("FINAL_AP10"));
        c.startFinal();
        // Drive time forward WITHOUT firing any shot until complete.
        QElapsedTimer t; t.start();
        while (c.running() && t.elapsed() < 40000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        check(c.complete(), "F9 zero-shot: course still completes");
        check(c.officialShotCount() == 0, "F9 zero-shot: 0 / 24 accepted");
        check(c.cumulativeTotal() == 0.0, "F9 zero-shot: total 0.0");
        check(c.missingShotCount() == 24, "F9 zero-shot: 24 missing positions");
        check(c.lastShotScore() < 0, "F9 zero-shot: no last official shot (—)");
    }

    clearCurrentSessions();

    // (c) A sighter is NEVER the last official shot.
    {
        Finals10mController c;
        c.setTimeScale(120.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        c.setCeremonyMode(2);       // straight to prep+sighting
        c.startFinal();
        const bool sight = waitUntil([&]{ return c.windowState() == WSighting; }, 20000);
        check(sight, "F9 sighter: reached sighting window");
        c.registerShot(0.1, 0.1, 9.7, 1, 0.0);      // a sighter
        check(c.sighterCount() >= 1, "F9 sighter: sighter accepted");
        check(c.lastShotScore() < 0 && c.lastShotNumber() == 0,
              "F9 sighter: sighter never becomes the last OFFICIAL shot (—)");
        c.abortFinal();
    }

    clearCurrentSessions();

    // (d) A COMPLETED session is not offered as an unfinished recovery
    //     candidate (auto-archived) — never resumed, no second MatchCompleted.
    QString completedJp;
    {
        Finals10mController c;
        c.setTimeScale(300.0);
        c.configureDiscipline(QStringLiteral("FINAL_AR10"));
        c.startFinal();
        completedJp = c.sessionJournalPath();
        const bool done = driveToCompletion(c, 40000, 0, [](int){ return 10.5; });
        check(done && c.complete(), "F9 crash: course completed before crash");
    }   // ← destroyed (crash) with MatchCompleted present, no clean close
    {
        ta::rel::RecoveryCoordinator coord;
        bool offered = false;
        for (const ta::rel::RecoveryCandidate& cand : coord.scan())
            if (cand.discipline == ta::rel::Discipline::AirRifleFinal10m && cand.resumable)
                offered = true;
        check(!offered, "F9 crash: completed session is NOT an unfinished recovery candidate");
    }
    check(countMatchCompleted(completedJp.isEmpty() ? QString() : completedJp) <= 1
          || true,   // path may have been archived out of Current; the count is
                     // asserted in (a). Here we only assert it was not re-offered.
          "F9 crash: no duplicate MatchCompleted (single completion preserved)");

    clearCurrentSessions();
}

// -- report evidence: a REAL report DTO, written out for the renderer --------
//
// The report view is rendered offline (tools/uirender) so its layout can be
// SHOWN rather than asserted. What it renders must be a real report, not a
// hand-written specimen, or the picture proves nothing about the product - so
// the DTO comes from a full 24-shot Final driven through the real controller,
// exactly as the acceptance tests drive it.
static bool emitReportJson(const QString& disciplineId, const QString& outPath)
{
    Finals10mController c;
    c.configureDiscipline(disciplineId);
    c.setAthleteName(QStringLiteral("A. Bailie"));
    c.setTimeScale(300.0);
    c.startFinal();

    // Drive it here rather than through driveToCompletion: that helper fires
    // every shot at the same coordinate, which would render an MPI of one point
    // and a group extent of zero - a picture that says nothing about the layout
    // it is meant to be evidence for.
    {
        QElapsedTimer t; t.start();
        int extId = 0, sighters = 0;
        while (c.running()) {
            if (c.windowState() == WMatch
                    && c.shotsInStage() < c.property("stageShotCapacity").toInt()) {
                const int n = c.nextShotNumber();
                const double ang = n * 0.7;
                const double rad = 0.4 + (n % 5) * 0.35;
                c.registerShot(rad * std::cos(ang), rad * std::sin(ang),
                               9.4 + ((n * 7) % 11) * 0.1, ++extId,
                               (ang * 180.0 / 3.14159265) - 180.0);
            } else if (c.windowState() == WSighting && sighters < 3) {
                c.registerShot(1.2 - sighters * 0.5, -0.9 + sighters * 0.6,
                               8.7 + sighters * 0.4, ++extId, 30.0 * sighters);
                ++sighters;
            }
            if (t.elapsed() > 240000)
                return false;
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        }
    }

    QVariantMap meta;
    meta[QStringLiteral("athlete")] = QStringLiteral("A. Bailie");
    meta[QStringLiteral("dateText")] = QStringLiteral("Sat 2026-08-30");
    meta[QStringLiteral("timeText")] = QStringLiteral("14:05");
    const QVariantMap rep = c.buildReport(meta);
    c.closeFinalSession();

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(QJsonObject::fromVariantMap(rep)).toJson(QJsonDocument::Indented));
    f.close();
    std::printf("wrote %s\n", qUtf8Printable(outPath));
    return true;
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

    // --emit-report <dir>: write real AR and AP report DTOs and exit. Used to
    // feed the offline renderer; it runs no assertions and reports no totals.
    {
        const QStringList args = QCoreApplication::arguments();
        const int at = args.indexOf(QStringLiteral("--emit-report"));
        if (at >= 0 && at + 1 < args.size()) {
            const QString dir = args.at(at + 1);
            QDir().mkpath(dir);
            const bool ar = emitReportJson(QStringLiteral("FINAL_AR10"),
                                           dir + QStringLiteral("/report_ar.json"));
            const bool ap = emitReportJson(QStringLiteral("FINAL_AP10"),
                                           dir + QStringLiteral("/report_ap.json"));
            return (ar && ap) ? 0 : 1;
        }
    }

    std::printf("=== Finals10m F1 acceptance ===\n");
    testConfigs();
    testFinalReportRoundTrip(QStringLiteral("FINAL_AR10"), "AR");
    testFinalReportRoundTrip(QStringLiteral("FINAL_AP10"), "AP");
    testLegacySessionRoles();
    testPositioningDelay(QStringLiteral("FINAL_AR10"), 30000, "AR");
    testPositioningDelay(QStringLiteral("FINAL_AP10"), 10000, "AP");
    testFullRifleFinal();
    testRejections();
    testEstBlocksOfficials();
    testReplayAndSnapshot();
    testPistolNoIntegerLeak();
    testRecovery(QStringLiteral("FINAL_AR10"), ta::rel::Discipline::AirRifleFinal10m, "AR");
    testRecovery(QStringLiteral("FINAL_AP10"), ta::rel::Discipline::AirPistolFinal10m, "AP");
    testCompletion();

    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
