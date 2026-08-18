// RULE-AUTH-001 — the adopted competition definition is persisted with the
// session and survives recovery.
//
// The question this file answers is not "does a field round-trip". It is:
// after a crash, can the session still PROVE which competition governed it,
// without asking the catalogue, without asking the UI, and without depending on
// which language the operator was running?
//
// That matters because all three of those move. The catalogue gains a DSB 2027
// edition that may reuse rule number 1.10 with different timing. The operator
// browses another programme while the recovery dialog is open. The range runs
// in German. None of them may change what a recorded match was.

#include "qualification/QualificationController.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/journal/JournalValidator.h"
#include "test_support.h"

#include <QVariantMap>

using namespace ta::rel;

namespace {

// The DSB 1.10 60-shot course, exactly as CompetitionCatalogue resolves it and
// LoginPage hands it over.
QVariantMap dsb110()
{
    QVariantMap m;
    m[QStringLiteral("programmeId")] = QStringLiteral("dsb.10m.air-rifle.lg60");
    m[QStringLiteral("rulesetId")] = QStringLiteral("dsb");
    m[QStringLiteral("rulesetVersion")] = QStringLiteral("2026-01-01");
    m[QStringLiteral("ruleNumber")] = QStringLiteral("1.10");
    m[QStringLiteral("programmeVariant")] = QStringLiteral("60");
    m[QStringLiteral("competitionContext")] = QStringLiteral("DM_2026");
    m[QStringLiteral("scoringMode")] = QStringLiteral("DECIMAL");
    m[QStringLiteral("timingModel")] = QStringLiteral("SINGLE_MATCH_CLOCK");
    m[QStringLiteral("targetStandardId")] = QStringLiteral("issf.10m.air-rifle");
    m[QStringLiteral("disciplineId")] = QStringLiteral("AR10");
    m[QStringLiteral("distanceM")] = 10;
    m[QStringLiteral("preparationMs")] = 900000;    // 15 min
    m[QStringLiteral("matchMs")] = 4500000;         // 75 min
    return m;
}

// DSB 1.40 — 50 m three positions on ONE clock. Integer scoring in the DM 2026
// context, 105 minutes, and a declared position sequence: the schema must
// already carry that so the 1.20 sequencer does not need a format change.
QVariantMap dsb140()
{
    QVariantMap m;
    m[QStringLiteral("programmeId")] = QStringLiteral("dsb.50m.rifle.3x20");
    m[QStringLiteral("rulesetId")] = QStringLiteral("dsb");
    m[QStringLiteral("rulesetVersion")] = QStringLiteral("2026-01-01");
    m[QStringLiteral("ruleNumber")] = QStringLiteral("1.40");
    m[QStringLiteral("programmeVariant")] = QStringLiteral("3x20");
    m[QStringLiteral("competitionContext")] = QStringLiteral("DM_2026");
    m[QStringLiteral("scoringMode")] = QStringLiteral("INTEGER");
    m[QStringLiteral("timingModel")] = QStringLiteral("SINGLE_MATCH_CLOCK");
    m[QStringLiteral("targetStandardId")] = QStringLiteral("issf.50m.rifle");
    m[QStringLiteral("disciplineId")] = QStringLiteral("PRONE50");
    m[QStringLiteral("distanceM")] = 50;
    m[QStringLiteral("preparationMs")] = 900000;    // 15 min
    m[QStringLiteral("matchMs")] = 6300000;         // 105 min
    m[QStringLiteral("positionSequence")] = QStringLiteral("KNEELING,PRONE,STANDING");
    return m;
}

// Build a crashed session that adopted `authority` (empty map = legacy).
void buildSession(MemoryJournalFile& file, ManualClock& clock,
                  const QVariantMap& authority, const char* disciplineId,
                  qint64 matchMs, qint64 prepMs)
{
    QualificationController qc;
    qc.storeForTesting()->setClockForTesting(&clock);
    qc.storeForTesting()->setJournalFileForTesting(&file);
    if (!authority.isEmpty())
        qc.adoptRuleAuthority(authority);
    qc.startSession(QString::fromLatin1(disciplineId), QStringLiteral("60"),
                    QStringLiteral("A"), 60, matchMs, prepMs, -1,
                    QString(), QString());
    qc.beginPreparation();
    clock.advance(30000);
    qc.beginSighting();
    clock.advance(10000);
    qc.submitSighter(0, 0, 10.5, 1001, 0, true);
    clock.advance(10000);
    qc.beginOfficialMatch();
    clock.advance(10000);
    qc.submitOfficial(0, 0, 10.4, 2001, 0, true);
    // no closeSession — a crash.
}

SessionState replayOf(const MemoryJournalFile& file)
{
    const ValidationReport rep = JournalValidator::validateBytes(file.data);
    return ReplayEngine::replay(rep.validEnvelopes).state;
}

// Every field the persisted authority must reproduce. Used by BOTH the real
// recovery checks and the negative control, so the control tests the same
// predicate the real path passes — not a weaker one written for it.
bool authorityMatches(const RuleAuthority& a, const QVariantMap& expected)
{
    const auto s = [&expected](const char* k) {
        return expected.value(QLatin1String(k)).toString();
    };
    return a.programmeId == s("programmeId")
        && a.rulesetId == s("rulesetId")
        && a.rulesetVersion == s("rulesetVersion")
        && a.ruleNumber == s("ruleNumber")
        && a.programmeVariant == s("programmeVariant")
        && a.competitionContext == s("competitionContext")
        && a.scoringMode == s("scoringMode")
        && a.timingModel == s("timingModel")
        && a.targetStandardId == s("targetStandardId")
        && a.disciplineId == s("disciplineId")
        && a.distanceM == expected.value(QStringLiteral("distanceM")).toInt()
        && a.preparationMs == expected.value(QStringLiteral("preparationMs")).toLongLong()
        && a.matchMs == expected.value(QStringLiteral("matchMs")).toLongLong()
        && a.positionSequence == s("positionSequence");
}

} // namespace

void run_rule_authority_tests()
{
    std::printf("--- rule authority persistence (RULE-AUTH-001) ---\n");

    // ── DSB 1.10: create → persist → reload ──────────────────────────────
    {
        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, dsb110(), "AR10", 4500000, 900000);

        check(file.data.contains("\"ruleAuthority\""),
              "RULE-AUTH-001: the journal header carries the adopted authority");
        check(file.data.contains("\"ruleNumber\":\"1.10\"")
              && file.data.contains("\"rulesetVersion\":\"2026-01-01\""),
              "RULE-AUTH-001: rule number and ruleset EDITION are both on disk - "
              "a 2027 edition reusing 1.10 cannot be mistaken for this session");

        const SessionState s = replayOf(file);
        check(s.ruleAuthority.isPresent(),
              "RULE-AUTH-001: DSB 1.10 reloads with its authority present");
        check(authorityMatches(s.ruleAuthority, dsb110()),
              "RULE-AUTH-001: DSB 1.10 reloads identical - programme, ruleset, "
              "version, rule, variant, context, scoring, timing model, target "
              "standard and BOTH adopted durations",
              s.ruleAuthority.auditLine());
        check(s.ruleAuthority.matchMs == 4500000
              && s.ruleAuthority.preparationMs == 900000,
              "RULE-AUTH-001: 75-minute match and 15-minute preparation are the "
              "SESSION's, not a fresh getTimeCount() lookup");
    }

    // ── DSB 1.40: three positions, one clock, integer ────────────────────
    {
        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, dsb140(), "PRONE50", 6300000, 900000);
        const SessionState s = replayOf(file);
        check(authorityMatches(s.ruleAuthority, dsb140()),
              "RULE-AUTH-001: DSB 1.40 reloads identical - 3x20, DM_2026, "
              "INTEGER, SINGLE_MATCH_CLOCK, 105 minutes, 50 m rifle standard",
              s.ruleAuthority.auditLine());
        check(s.ruleAuthority.positionSequence
                  == QLatin1String("KNEELING,PRONE,STANDING"),
              "RULE-AUTH-001: the position sequence persists, so the schema "
              "already carries what the 1.20 sequencer will need");
        check(s.ruleAuthority.ruleNumber == QLatin1String("1.40")
              && s.ruleAuthority.rulesetId == QLatin1String("dsb"),
              "RULE-AUTH-001: a recovered 1.40 is still DSB 1.40 - never a "
              "generic 60-shot 50 m match");
    }

    // ── DSB 1.60: 3x40 on ONE clock ──────────────────────────────────────
    // The course is persisted because it is a RULE. A recovered 120-shot
    // session that had to re-derive its course would divide 120 by three and
    // arrive at 3x40 by luck; a 1.40 session resumed the same way would be
    // wrong, and neither should depend on arithmetic.
    {
        QVariantMap m = dsb140();
        m[QStringLiteral("programmeId")] = QStringLiteral("dsb.50m.rifle.3x40");
        m[QStringLiteral("ruleNumber")] = QStringLiteral("1.60");
        m[QStringLiteral("programmeVariant")] = QStringLiteral("3x40");
        m[QStringLiteral("matchMs")] = 9900000;          // 165 min
        m[QStringLiteral("shotsPerPosition")] = QStringLiteral("40,40,40");

        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, m, "PRONE50", 9900000, 900000);
        const SessionState s = replayOf(file);
        check(s.ruleAuthority.ruleNumber == QLatin1String("1.60")
              && s.ruleAuthority.programmeVariant == QLatin1String("3x40")
              && s.ruleAuthority.matchMs == 9900000
              && s.ruleAuthority.preparationMs == 900000
              && s.ruleAuthority.scoringMode == QLatin1String("INTEGER")
              && s.ruleAuthority.timingModel == QLatin1String("SINGLE_MATCH_CLOCK"),
              "RULE-AUTH-001: DSB 1.60 reloads as 3x40 on a 165-minute master "
              "clock, integer scored", s.ruleAuthority.auditLine());
        check(s.ruleAuthority.shotsPerPosition == QLatin1String("40,40,40"),
              "RULE-AUTH-001: and its COURSE reloads as 40/40/40 - never "
              "re-derived from the shot count",
              s.ruleAuthority.shotsPerPosition);
        check(s.ruleAuthority.positionDurationsMs.isEmpty(),
              "RULE-AUTH-001: 1.60 persists NO per-position clocks, so nothing "
              "can run it as an independent-clock course");
    }

    // ── LEGACY: a session with no profile ────────────────────────────────
    {
        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, QVariantMap(), "AR10", 4500000, 900000);

        check(!file.data.contains("\"ruleAuthority\""),
              "RULE-AUTH-001: a session with no profile writes NO authority "
              "field, so every journal recorded before this exists re-serialises "
              "byte-identically and keeps its hash chain");

        const SessionState s = replayOf(file);
        check(!s.ruleAuthority.isPresent(),
              "RULE-AUTH-001: absence reloads as LEGACY - explicitly absent, "
              "never a fabricated identity");
        check(s.ruleAuthority.auditLine() == QLatin1String("ruleset=LEGACY"),
              "RULE-AUTH-001: the support log says LEGACY rather than pretending "
              "to know the ruleset", s.ruleAuthority.auditLine());
        check(s.officials.size() == 1 && s.sighters.size() == 1
              && s.config.matchMs == 4500000,
              "RULE-AUTH-001: a legacy session still loads and behaves exactly "
              "as before - missing metadata is not a reason to reject it");
    }

    // ── snapshot round-trip, and the v1-v6 snapshot that predates the field ─
    {
        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, dsb140(), "PRONE50", 6300000, 900000);
        const SessionState s = replayOf(file);

        const QByteArray bytes = serializeSessionState(s);
        SessionState back;
        const ReliabilityResult r = deserializeSessionState(bytes, &back);
        check(r.ok && back.ruleAuthority == s.ruleAuthority,
              "RULE-AUTH-001: the authority survives a state SNAPSHOT too - "
              "otherwise a snapshotted session would lose its rules at the "
              "point the fold is replaced by the snapshot");

        // Strip the key exactly as a state v6 snapshot lacks it.
        QByteArray older = bytes;
        const int at = older.indexOf("\"ruleAuthority\":{");
        check(at > 0, "RULE-AUTH-001: authority present in the snapshot bytes");
        if (at > 0) {
            const int end = older.indexOf('}', at);
            older.remove(at, end - at + 2);   // key, object and trailing comma
            SessionState legacy;
            const ReliabilityResult lr = deserializeSessionState(older, &legacy);
            check(lr.ok && !legacy.ruleAuthority.isPresent(),
                  "RULE-AUTH-001: a snapshot without the key loads as LEGACY "
                  "instead of failing - old snapshots keep working",
                  lr.ok ? QString() : lr.error.technicalDetail);
        }
    }

    // ── language invariance ──────────────────────────────────────────────
    {
        // The same competition, handed over by an English UI and a German one.
        // Display text rides along in the map; none of it may reach the disk.
        QVariantMap en = dsb110();
        en[QStringLiteral("displayName")] = QStringLiteral("DSB 1.10 AIR RIFLE 60");
        en[QStringLiteral("subtitle")] = QStringLiteral("Official DSB course");
        QVariantMap de = dsb110();
        de[QStringLiteral("displayName")] = QStringLiteral("DSB 1.10 LUFTGEWEHR 60");
        de[QStringLiteral("subtitle")] = QStringLiteral("Offizielles DSB-Programm");

        MemoryJournalFile fEn, fDe;
        ManualClock cEn, cDe;
        buildSession(fEn, cEn, en, "AR10", 4500000, 900000);
        buildSession(fDe, cDe, de, "AR10", 4500000, 900000);

        // Session id and wall clock differ per session by design, so compare
        // the AUTHORITY OBJECT itself - the part that claims what competition
        // this was.
        const auto authorityBytes = [](const QByteArray& journal) {
            const int at = journal.indexOf("\"ruleAuthority\":{");
            return at < 0 ? QByteArray()
                          : journal.mid(at, journal.indexOf('}', at) - at + 1);
        };
        check(!authorityBytes(fEn.data).isEmpty()
              && authorityBytes(fEn.data) == authorityBytes(fDe.data),
              "RULE-AUTH-001: the persisted authority is byte-identical in "
              "English and German - machine values only",
              QString::fromUtf8(authorityBytes(fDe.data)));
        check(!fEn.data.contains("LUFTGEWEHR") && !fEn.data.contains("Offizielles")
              && !fDe.data.contains("LUFTGEWEHR") && !fDe.data.contains("Offizielles"),
              "RULE-AUTH-001: no translated display string is persisted as "
              "authority - a German journal must not describe a different "
              "competition than an English one");
    }

    // ── the CURRENT UI SELECTION cannot override an ADOPTED session ──────
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
        qc.adoptRuleAuthority(dsb110());
        qc.startSession(QStringLiteral("AR10"), QStringLiteral("60"),
                        QStringLiteral("A"), 60, 4500000, 900000, -1,
                        QString(), QString());
        qc.beginPreparation();
        qc.beginSighting();

        // The operator now browses another programme. This is the ordinary
        // case, not an abuse: the recovery dialog sits on top of LoginPage.
        qc.adoptRuleAuthority(dsb140());

        const QVariantMap live = qc.sessionRuleAuthority();
        check(live.value(QStringLiteral("ruleNumber")).toString()
                  == QLatin1String("1.10")
              && live.value(QStringLiteral("matchMs")).toLongLong() == 4500000,
              "RULE-AUTH-001: the LIVE session keeps the rules it adopted while "
              "another programme is browsed",
              live.value(QStringLiteral("ruleNumber")).toString());

        const SessionState s = replayOf(file);
        check(authorityMatches(s.ruleAuthority, dsb110()),
              "RULE-AUTH-001: and the journal - the thing recovery reads - still "
              "says 1.10, so the recovered session's authority wins over the "
              "current selection");
    }

    // ── NEGATIVE CONTROL ─────────────────────────────────────────────────
    // The previous behaviour cleared the profile on resume, which is exactly
    // what this work replaces. Run the SAME predicate the real checks use
    // against a session whose authority was cleared: it must FAIL. Without
    // this, a check that always passed would look like proof.
    {
        MemoryJournalFile file;
        ManualClock clock;
        buildSession(file, clock, dsb140(), "PRONE50", 6300000, 900000);
        SessionState s = replayOf(file);
        check(authorityMatches(s.ruleAuthority, dsb140()),
              "RULE-AUTH-001 control: the predicate passes on the real path");

        s.ruleAuthority = RuleAuthority();   // the OLD resume behaviour
        check(!authorityMatches(s.ruleAuthority, dsb140()),
              "RULE-AUTH-001 control: clearing the authority on resume FAILS the "
              "same predicate - the old behaviour cannot pass these tests");
        check(!s.ruleAuthority.isPresent()
              && s.ruleAuthority.auditLine() == QLatin1String("ruleset=LEGACY"),
              "RULE-AUTH-001 control: a cleared session reports LEGACY, so the "
              "loss would be visible in the support log rather than silent");

        // And the fallback the block existed to prevent: a cleared 1.40 has no
        // 105-minute authority left, so the engine would reach for the legacy
        // shot-count clock - a different competition, recorded as if correct.
        check(s.ruleAuthority.matchMs == 0,
              "RULE-AUTH-001 control: clearing leaves NO adopted duration, which "
              "is precisely the fallback this persistence prevents");
    }
}
