// Wind Map — domain, events, serialization and reducer replay (stage 3).
//
// Spec: docs/training-lab-wind-map-implementation-spec.md
//
// The two properties these tests exist to protect:
//
//   1. THE SNAPSHOT IS IMMUTABLE. A shot keeps the wind it was recorded with.
//      Changing the standing condition afterwards must not reach back into a
//      shot that has already been accepted.
//   2. ABSENCE IS EXPLICIT. "No wind reading recorded" is a distinct fact from
//      a recorded calm, and is never inferred, defaulted or back-filled —
//      including through a full journal replay.
#include "test_support.h"

#include "training/WindMapTypes.h"
#include "reliability/events/EventTypes.h"
#include "reliability/events/EventSerializer.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/journal/HashChain.h"

#include <QString>
#include <cmath>
#include <limits>

using namespace ta::rel;
using namespace ta::training;

namespace {

// Same envelope shape the other serializer tests use, so a Wind Map event is
// exercised through the real journal path rather than a bespoke one.
EventEnvelope makeEnvelope(quint64 seq, const DomainEvent& payload,
                           const QByteArray& ph)
{
    EventEnvelope env;
    env.payloadVersion = eventPayloadVersion(payload);
    env.sessionId = QString::fromLatin1(testjournal::kSid);
    env.lane = QStringLiteral("L1");
    env.seq = seq;
    env.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
    env.monotonicMs = static_cast<qint64>(seq) * 1000;
    env.eventType = QString::fromLatin1(eventTypeId(payload));
    env.payload = payload;
    if (seq == 0) {
        env.appVersion = QStringLiteral("4.0.0-test");
        env.deviceId = QStringLiteral("TEST-DEVICE");
    }
    env.previousHash = ph;
    return env;
}

WindSnapshotFields measuredWind(qint16 deg, qint32 speedHundredths,
                                qint64 ms = 1000, const QString& note = QString())
{
    WindSnapshotFields w;
    w.windValid = true;
    w.windCalm = false;
    w.windDirectionDegrees = deg;
    w.windSpeedHundredthMs = speedHundredths;
    w.windSource = 0;
    w.windRecordedMs = ms;
    w.windNote = note;
    return w;
}

WindSnapshotFields calmWind(qint64 ms = 1000)
{
    WindSnapshotFields w;
    w.windValid = true;
    w.windCalm = true;
    w.windRecordedMs = ms;
    return w;
}

ShotCore shotAt(qint32 xHundredthMm, qint32 yHundredthMm, qint32 scoreTenths)
{
    ShotCore s;
    s.xHundredthMm = xHundredthMm;
    s.yHundredthMm = yHundredthMm;
    s.scoreTenths = scoreTenths;
    s.splitMs = 0;
    s.simulated = true;
    return s;
}

} // namespace

void run_windmap_tests()
{
    std::printf("--- Wind Map domain + events + replay (stage 3) ---\n");

    // ── direction normalisation ─────────────────────────────────────────────
    {
        qint16 d = -1;
        check(normalizeWindDegrees(0.0, &d) && d == 0, "wind: 0 normalises to 0");
        check(normalizeWindDegrees(359.0, &d) && d == 359, "wind: 359 stays 359");
        check(normalizeWindDegrees(360.0, &d) && d == 0, "wind: 360 folds to 0");
        check(normalizeWindDegrees(720.0, &d) && d == 0, "wind: 720 folds to 0");
        check(normalizeWindDegrees(-90.0, &d) && d == 270, "wind: -90 folds to 270");
        check(normalizeWindDegrees(-360.0, &d) && d == 0, "wind: -360 folds to 0");
        check(normalizeWindDegrees(450.0, &d) && d == 90, "wind: 450 folds to 90");

        // Rule 3 — a non-finite direction must NOT become a silent 0 (North).
        check(!normalizeWindDegrees(std::numeric_limits<double>::quiet_NaN(), &d),
              "wind: NaN direction is rejected, not folded to North");
        check(!normalizeWindDegrees(std::numeric_limits<double>::infinity(), &d),
              "wind: +inf direction is rejected");
        check(!normalizeWindDegrees(-std::numeric_limits<double>::infinity(), &d),
              "wind: -inf direction is rejected");
    }

    // ── all eight sectors, and the N wrap ───────────────────────────────────
    {
        struct Case { qint16 deg; WindSector expect; const char* what; };
        const Case cases[] = {
            {   0, WindSector::N,  "0 -> N" },
            {  45, WindSector::NE, "45 -> NE" },
            {  90, WindSector::E,  "90 -> E" },
            { 135, WindSector::SE, "135 -> SE" },
            { 180, WindSector::S,  "180 -> S" },
            { 225, WindSector::SW, "225 -> SW" },
            { 270, WindSector::W,  "270 -> W" },
            { 315, WindSector::NW, "315 -> NW" },
            // Boundaries. 22.5 and 337.5 are the N wrap; degrees are integers,
            // so the boundary cases are 22/23 and 337/338.
            {  22, WindSector::N,  "22 is still N (below 22.5)" },
            {  23, WindSector::NE, "23 crosses into NE" },
            { 337, WindSector::NW, "337 is still NW (below 337.5)" },
            { 338, WindSector::N,  "338 wraps into N" },
            { 359, WindSector::N,  "359 is N — the sector spans the wrap" },
            {  67, WindSector::NE, "67 is still NE" },
            {  68, WindSector::E,  "68 crosses into E" },
            { 112, WindSector::E,  "112 is still E" },
            { 113, WindSector::SE, "113 crosses into SE" },
            { 157, WindSector::SE, "157 is still SE" },
            { 158, WindSector::S,  "158 crosses into S" },
            { 202, WindSector::S,  "202 is still S" },
            { 203, WindSector::SW, "203 crosses into SW" },
            { 247, WindSector::SW, "247 is still SW" },
            { 248, WindSector::W,  "248 crosses into W" },
            { 292, WindSector::W,  "292 is still W" },
            { 293, WindSector::NW, "293 crosses into NW" },
        };
        for (const Case& c : cases)
            check(windSectorOfDegrees(c.deg) == c.expect,
                  QString(QStringLiteral("sector: %1")).arg(QLatin1String(c.what)));

        // Every sector's approved centre value round-trips to itself.
        for (int i = 0; i < 8; ++i) {
            const auto sec = static_cast<WindSector>(i);
            check(windSectorOfDegrees(windSectorCentreDegrees(sec)) == sec,
                  QString(QStringLiteral("sector: centre of %1 maps back to itself"))
                      .arg(windSectorLabel(sec)));
        }
    }

    // ── speed bands, at the exact approved boundaries ───────────────────────
    {
        auto bandOf = [](qint32 hundredths) {
            WindConditionSnapshot s;
            s.valid = true; s.calm = false; s.speedHundredthMs = hundredths;
            return s.band();
        };
        check(bandOf(1)   == WindSpeedBand::Light,      "band: 0.01 -> Light");
        check(bandOf(200) == WindSpeedBand::Light,      "band: exactly 2.00 -> Light (lower band)");
        check(bandOf(201) == WindSpeedBand::Moderate,   "band: 2.01 -> Moderate");
        check(bandOf(400) == WindSpeedBand::Moderate,   "band: exactly 4.00 -> Moderate (lower band)");
        check(bandOf(401) == WindSpeedBand::Strong,     "band: 4.01 -> Strong");
        check(bandOf(700) == WindSpeedBand::Strong,     "band: exactly 7.00 -> Strong (lower band)");
        check(bandOf(701) == WindSpeedBand::VeryStrong, "band: 7.01 -> Very strong");
        check(bandOf(5000) == WindSpeedBand::VeryStrong,"band: 50.00 -> Very strong");

        // The approved rule: exactly 0 is Calm ONLY when calm was explicitly
        // recorded. A measured zero is a Light reading of zero.
        check(bandOf(0) == WindSpeedBand::Light,
              "band: a MEASURED 0.00 is Light, not Calm");
        WindConditionSnapshot calm = WindConditionSnapshot::calmAt(1);
        check(calm.band() == WindSpeedBand::Calm,
              "band: an explicitly recorded calm is Calm");
    }

    // ── Calm vs NoReading ───────────────────────────────────────────────────
    {
        const auto none = WindConditionSnapshot::noReading();
        const auto calm = WindConditionSnapshot::calmAt(1234);

        check(!none.hasReading(), "calm/none: no-reading has no reading");
        check(calm.hasReading(),  "calm/none: calm IS a reading (rule 1)");
        check(!none.isCalm(),     "calm/none: no-reading is not calm (rule 2)");
        check(calm.isCalm(),      "calm/none: calm is calm");
        check(!none.isMeasured() && !calm.isMeasured(),
              "calm/none: neither is a measured reading");
        check(none != calm, "calm/none: the two are distinguishable");
        check(none.isStructurallyValid(), "calm/none: an empty no-reading is structurally valid");
        check(calm.isStructurallyValid(), "calm/none: calm is structurally valid");

        // Rule 4 — a calm snapshot must not acquire an inferred direction.
        check(calm.directionDegrees == 0 && calm.speedHundredthMs == 0,
              "calm: carries no direction or speed");
        WindConditionSnapshot bogus = calm;
        bogus.directionDegrees = 90;
        check(!bogus.isStructurallyValid(),
              "calm: a calm snapshot carrying a direction is rejected (rule 4)");

        // A no-reading that carries data is a contradiction and is rejected.
        WindConditionSnapshot contradictory;
        contradictory.valid = false;
        contradictory.speedHundredthMs = 250;
        check(!contradictory.isStructurallyValid(),
              "no-reading: a snapshot with valid=false may not carry data");
    }

    // ── measured() input validation ─────────────────────────────────────────
    {
        WindConditionSnapshot s;
        check(WindConditionSnapshot::measured(270.0, 2.5, 99, &s),
              "measured: a normal reading is accepted");
        check(s.directionDegrees == 270 && s.speedHundredthMs == 250,
              "measured: 2.5 m/s stores as 250 hundredths");
        check(qFuzzyCompare(s.speedMetresPerSecond(), 2.5),
              "measured: hundredths convert back to m/s");

        check(WindConditionSnapshot::measured(-90.0, 1.0, 1, &s) && s.directionDegrees == 270,
              "measured: negative direction is normalised, not rejected");

        // Rule 3 — negatives and non-finites are rejected outright.
        check(!WindConditionSnapshot::measured(0.0, -0.1, 1, &s),
              "measured: a negative speed is rejected");
        check(!WindConditionSnapshot::measured(0.0, std::numeric_limits<double>::quiet_NaN(), 1, &s),
              "measured: NaN speed is rejected");
        check(!WindConditionSnapshot::measured(std::numeric_limits<double>::quiet_NaN(), 1.0, 1, &s),
              "measured: NaN direction is rejected");
        check(!WindConditionSnapshot::measured(0.0, 1.0e9, 1, &s),
              "measured: an absurd speed is rejected");
    }

    // ── discipline and position validation (rules 8/10) ─────────────────────
    {
        check(isWindMapDiscipline(QStringLiteral("PRONE50")), "scope: 50m Prone supported");
        check(isWindMapDiscipline(QStringLiteral("3P50")),    "scope: 50m 3P supported");
        for (const char* bad : { "AR10", "AP10", "FINAL3P", "FINAL_AR10", "", "prone50" })
            check(!isWindMapDiscipline(QString::fromLatin1(bad)),
                  QString(QStringLiteral("scope: '%1' is rejected"))
                      .arg(QString::fromLatin1(bad)));
        check(windMapDisciplineIs3P(QStringLiteral("3P50")), "scope: 3P50 is 3P");
        check(!windMapDisciplineIs3P(QStringLiteral("PRONE50")), "scope: PRONE50 is not 3P");

        check(isValidWindMapPosition(0, false), "position: Prone uses 0");
        check(!isValidWindMapPosition(1, false), "position: Prone rejects a 3P position");
        for (qint8 p = 1; p <= 3; ++p)
            check(isValidWindMapPosition(p, true),
                  QString(QStringLiteral("position: 3P accepts %1")).arg(int(p)));
        check(!isValidWindMapPosition(0, true), "position: 3P rejects 0");
        check(!isValidWindMapPosition(4, true), "position: 3P rejects 4");
    }

    // ── WindSource ──────────────────────────────────────────────────────────
    {
        check(isImplementedWindSource(WindSource::Manual), "source: Manual is implemented");
        check(!isImplementedWindSource(WindSource::WeatherStation),
              "source: WeatherStation is reserved and NOT implemented");
        WindSource out = WindSource::WeatherStation;
        check(windSourceFromKey(QStringLiteral("Manual"), &out) && out == WindSource::Manual,
              "source: Manual round-trips by key");
        check(windSourceFromKey(QStringLiteral("WeatherStation"), &out)
              && out == WindSource::WeatherStation,
              "source: the reserved key is addressable");
        check(!windSourceFromKey(QStringLiteral("Anemometer"), &out),
              "source: an unknown key fails closed, not silently Manual");
    }

    // ── event payload validation ────────────────────────────────────────────
    {
        WindMapSessionStarted ok;
        ok.disciplineId = QStringLiteral("3P50"); ok.is3P = true;
        check(ok.validate().ok, "event: a valid 3P session start passes");

        WindMapSessionStarted bad = ok;
        bad.disciplineId = QStringLiteral("AR10");
        check(!bad.validate().ok, "event: a 10m discipline is rejected (rule 8)");

        WindMapSessionStarted mismatched;
        mismatched.disciplineId = QStringLiteral("PRONE50"); mismatched.is3P = true;
        check(!mismatched.validate().ok,
              "event: is3P disagreeing with the discipline is rejected");

        WindMapShotAccepted shot;
        shot.shot = shotAt(100, -50, 102);
        shot.shotId = 1; shot.position = 0;
        static_cast<WindSnapshotFields&>(shot) = measuredWind(270, 250);
        check(shot.validate().ok, "event: a measured counted shot passes");

        WindMapShotAccepted noId = shot; noId.shotId = 0;
        check(!noId.validate().ok, "event: shotId < 1 is rejected");

        WindMapShotAccepted badPos = shot; badPos.position = 9;
        check(!badPos.validate().ok, "event: an unknown position is rejected");

        WindMapShotAccepted calmWithDir = shot;
        static_cast<WindSnapshotFields&>(calmWithDir) = calmWind();
        calmWithDir.windDirectionDegrees = 90;
        check(!calmWithDir.validate().ok,
              "event: a calm snapshot carrying a direction is rejected (rule 4)");

        WindMapShotAccepted absentWithData = shot;
        absentWithData.windValid = false;
        check(!absentWithData.validate().ok,
              "event: a no-reading snapshot carrying data is rejected (rule 2)");

        WindMapShotAccepted absent = shot;
        static_cast<WindSnapshotFields&>(absent) = WindSnapshotFields{};
        check(absent.validate().ok,
              "event: a genuine no-reading shot is valid — absence is recordable");

        WindMapPositionChanged same; same.fromPosition = 1; same.toPosition = 1;
        check(!same.validate().ok, "event: a position change to itself is rejected");

        WindMapSessionCompleted neg; neg.countedShots = -1;
        check(!neg.validate().ok, "event: a negative count is rejected");
    }

    // ── serialization round-trip ────────────────────────────────────────────
    {
        // Round-trip through the REAL journal path: serialize the hash-covered
        // core, complete the envelope, parse it back, and re-serialize. If the
        // bytes differ, the event does not survive a journal write/read.
        auto roundTrip = [](const DomainEvent& in, const char* what) {
            EventEnvelope env = makeEnvelope(7, in, QByteArray(32, 'a'));
            QByteArray core;
            check(EventSerializer::serializeCoreWithoutCurrentHash(env, &core).ok,
                  QString(QStringLiteral("serialize: %1 serialises"))
                      .arg(QLatin1String(what)));
            env.currentHash = HashChain::computeLineHash(env.previousHash, core);
            QByteArray full;
            check(EventSerializer::serializeCompleteEnvelope(env, &full).ok,
                  QString(QStringLiteral("serialize: %1 completes"))
                      .arg(QLatin1String(what)));

            EventEnvelope back;
            const auto r = EventSerializer::deserializeEnvelope(full, &back);
            check(r.ok, QString(QStringLiteral("serialize: %1 deserialises"))
                            .arg(QLatin1String(what)), r.error.technicalDetail);
            if (!r.ok) return;
            QByteArray core2;
            EventSerializer::serializeCoreWithoutCurrentHash(back, &core2);
            check(core2 == core,
                  QString(QStringLiteral("serialize: %1 round-trips byte-exactly"))
                      .arg(QLatin1String(what)));
        };

        WindMapSessionStarted started;
        started.disciplineId = QStringLiteral("3P50");
        started.is3P = true;
        started.positionSequence = QStringLiteral("K-P-S");
        roundTrip(started, "WindMapSessionStarted");

        WindConditionChanged changed;
        static_cast<WindSnapshotFields&>(changed) = measuredWind(135, 375, 5000,
                                                                QStringLiteral("gusting"));
        roundTrip(changed, "WindConditionChanged (measured, with note)");

        WindConditionChanged calmChange;
        static_cast<WindSnapshotFields&>(calmChange) = calmWind(6000);
        roundTrip(calmChange, "WindConditionChanged (calm)");

        WindConditionChanged absent;   // valid == false
        roundTrip(absent, "WindConditionChanged (no reading)");

        WindMapShotAccepted counted;
        counted.shot = shotAt(-342, 108, 102);
        counted.shotId = 17; counted.position = 3;
        static_cast<WindSnapshotFields&>(counted) = measuredWind(270, 250, 7000);
        roundTrip(counted, "WindMapShotAccepted");

        WindMapSighterAccepted sighter;
        sighter.shot = shotAt(10, 20, 95);
        sighter.shotId = 1; sighter.position = 1;
        static_cast<WindSnapshotFields&>(sighter) = WindSnapshotFields{};
        roundTrip(sighter, "WindMapSighterAccepted (no reading)");

        WindMapPositionChanged moved; moved.fromPosition = 1; moved.toPosition = 2;
        roundTrip(moved, "WindMapPositionChanged");

        WindMapSessionCompleted done;
        done.countedShots = 40; done.sighterShots = 6; done.conditionChanges = 9;
        roundTrip(done, "WindMapSessionCompleted");
    }

    // ── a no-reading snapshot survives serialisation as ABSENT ──────────────
    {
        WindMapShotAccepted absent;
        absent.shot = shotAt(0, 0, 100);
        absent.shotId = 1;
        absent.position = 0;
        EventEnvelope env = makeEnvelope(1, absent, QByteArray(32, 'a'));
        QByteArray line;
        EventSerializer::serializeCoreWithoutCurrentHash(env, &line);

        check(line.contains("\"windValid\":false"),
              "serialize: a missing reading is written explicitly, not omitted");
        check(!line.contains("windDirDeg"),
              "serialize: a missing reading writes no direction to be misread as North");

        env.currentHash = HashChain::computeLineHash(env.previousHash, line);
        QByteArray full;
        EventSerializer::serializeCompleteEnvelope(env, &full);
        EventEnvelope back;
        check(EventSerializer::deserializeEnvelope(full, &back).ok,
              "serialize: absent reading reads back");
        const auto* got = std::get_if<WindMapShotAccepted>(&back.payload);
        check(got != nullptr, "serialize: absent reading keeps its type");
        if (got) {
            check(!got->windValid, "serialize: absence survives the round trip");
            check(!got->windCalm, "serialize: absence is not read back as calm");
            check(got->windDirectionDegrees == 0 && got->windSpeedHundredthMs == 0,
                  "serialize: absence carries no inferred values");
        }
    }
}
