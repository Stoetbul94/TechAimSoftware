// M1 — golden fixtures (step 16). Every fixture is generated
// DETERMINISTICALLY here (fixed sid, timestamps, scores), committed under
// tests/reliability/fixtures/, and verified BYTE-FOR-BYTE plus by expected
// validation classification on every run.
//
// Regeneration is explicit and never happens during a normal run:
//     reliability_tests --write-fixtures
// rewrites the committed files from the generators below (documented in
// fixtures/README.md). If a generator changes, the diff shows up in git.

#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "test_support.h"

#include <QDir>
#include <QFile>

using namespace ta::rel;

namespace {

QString fixturesDir()
{
    return QString::fromUtf8(RELIABILITY_FIXTURES_DIR);
}

QByteArray readFixture(const QString& name, bool* ok)
{
    QFile f(fixturesDir() + QLatin1Char('/') + name);
    if (!f.open(QIODevice::ReadOnly)) {
        *ok = false;
        return QByteArray();
    }
    *ok = true;
    return f.readAll();
}

bool writeFixture(const QString& name, const QByteArray& bytes)
{
    QDir().mkpath(fixturesDir());
    QFile f(fixturesDir() + QLatin1Char('/') + name);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(bytes) == bytes.size();
}

// Writes a script, folding state so snapshots/completions stay coherent.
QByteArray buildJournal(Discipline discipline, const QString& matchType,
                        const QVector<DomainEvent>& tail,
                        bool withSnapshot, bool complete)
{
    MemoryJournalFile file;
    JournalWriter writer(testjournal::identity(), &file);
    SessionState state;
    qint64 tm = 0;
    QVector<DomainEvent> script;
    script.append(DomainEvent(testjournal::sessionStarted(
        discipline, matchType, QString::fromUtf8("Arnold Bailie"))));
    script.append(tail);
    for (const DomainEvent& e : script) {
        const AppendOutcome out =
            writer.append(e, QString::fromLatin1(testjournal::kWall), tm);
        check(out.ok, "fixture append", out.error.technicalDetail);
        if (!out.ok)
            return QByteArray();
        const ReduceResult r = SessionReducer::apply(state, out.envelope);
        check(r.ok, "fixture reduce", r.error.technicalDetail);
        if (!r.ok)
            return QByteArray();
        state = r.state;
        tm += 1000;
    }
    if (withSnapshot) {
        const AppendOutcome out = writer.append(
            DomainEvent(buildStateSnapshot(state)),
            QString::fromLatin1(testjournal::kWall), tm);
        check(out.ok, "fixture snapshot append", out.error.technicalDetail);
        const ReduceResult r = SessionReducer::apply(state, out.envelope);
        check(r.ok, "fixture snapshot reduce", r.error.technicalDetail);
        state = r.state;
        tm += 1000;
    }
    if (complete) {
        MatchCompleted done;
        done.totalTenths = state.totalTenths;
        done.officialCount = static_cast<qint16>(state.officials.size());
        const AppendOutcome out = writer.append(DomainEvent(done),
            QString::fromLatin1(testjournal::kWall), tm);
        check(out.ok, "fixture completion append", out.error.technicalDetail);
        tm += 1000;
        const AppendOutcome closed = writer.append(
            DomainEvent(SessionClosed{CloseReason::Clean}),
            QString::fromLatin1(testjournal::kWall), tm);
        check(closed.ok, "fixture close append", closed.error.technicalDetail);
    }
    return file.data;
}

QByteArray buildQualificationClean()
{
    return buildJournal(Discipline::Prone50m, QStringLiteral("60"), {
        DomainEvent(PreparationStarted{0}),
        DomainEvent(SightingStarted{0}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 99)}),
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(TimerStarted{TimerId::Match, 3000000}),
        DomainEvent(ShotAccepted{testjournal::shot(1, 103)}),
        DomainEvent(ShotAccepted{testjournal::shot(2, 105)}),
        DomainEvent(ShotAccepted{testjournal::shot(3, 100)}),
        DomainEvent(StageCompleted{1, 2}),
    }, false, true);
}

QByteArray buildFinalsClean()
{
    return buildJournal(Discipline::Finals3P, QStringLiteral("FINAL 35"), {
        DomainEvent(PreparationStarted{0}),
        DomainEvent(SightingStarted{0}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 102)}),
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
        DomainEvent(ShotAccepted{testjournal::shot(2, 98)}),
        DomainEvent(PositionChanged{1}),
        DomainEvent(ShotAccepted{testjournal::shot(3, 101)}),
    }, true, true);
}

QByteArray buildSighterOfficial()
{
    return buildJournal(Discipline::AirRifle10m, QStringLiteral("60"), {
        DomainEvent(SightingStarted{0}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 95)}),
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
    }, false, false);
}

QByteArray buildCorrectedShot()
{
    // shots land at seq 2 and 3; both corrections reference them
    return buildJournal(Discipline::Prone50m, QStringLiteral("60"), {
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
        DomainEvent(ShotAccepted{testjournal::shot(2, 96)}),
        DomainEvent(ShotRescored{2, 105, false, 0, 0,
                                 QStringLiteral("Jury decision 7.11.2"),
                                 Authority::Jury}),
        DomainEvent(ShotInvalidated{3, QStringLiteral("Cross shot"),
                                    Authority::Jury}),
    }, false, false);
}

QByteArray buildTailTruncated()
{
    QByteArray bytes = buildSighterOfficial();
    bytes.chop(37);   // deterministic torn tail inside the final line
    return bytes;
}

QByteArray buildCorruptInternal()
{
    QByteArray bytes = buildQualificationClean();
    // flip one digit inside an INTERIOR official shot payload
    const QByteArray before = QByteArrayLiteral("\"scoreTenths\":103");
    const QByteArray after = QByteArrayLiteral("\"scoreTenths\":193");
    check(bytes.contains(before), "corrupt-internal fixture target present");
    bytes.replace(before, after);
    return bytes;
}

QByteArray buildUnsupportedVersion()
{
    QByteArray bytes = buildSighterOfficial();
    // append a copy of the final line with pv patched to 99 — the line is
    // unreadable by this build, exactly the newer-version signature
    const int lastLf = bytes.lastIndexOf('\n', bytes.size() - 2);
    QByteArray lastLine = bytes.mid(lastLf + 1);
    lastLine.replace("\"pv\":1", "\"pv\":99");
    bytes += lastLine;
    return bytes;
}

struct FixtureSpec {
    const char* name;
    QByteArray (*build)();
    JournalClassification expected;
    int expectedValidPrefix;   // -1 = don't check
};

const FixtureSpec kFixtures[] = {
    {"fixture_qualification_clean.jsonl", buildQualificationClean,
     JournalClassification::Clean, 12},
    {"fixture_finals_clean.jsonl", buildFinalsClean,
     JournalClassification::Clean, 12},
    {"fixture_sighter_official.jsonl", buildSighterOfficial,
     JournalClassification::Clean, 5},
    {"fixture_corrected_shot.jsonl", buildCorrectedShot,
     JournalClassification::Clean, 6},
    {"fixture_tail_truncated.jsonl", buildTailTruncated,
     JournalClassification::TailTruncated, 4},
    {"fixture_corrupt_internal.jsonl", buildCorruptInternal,
     JournalClassification::CorruptInternal, -1},
    {"fixture_unsupported_version.jsonl", buildUnsupportedVersion,
     JournalClassification::UnsupportedVersion, 5},
};

} // namespace

void run_fixture_tests(bool regenerate)
{
    std::printf("--- golden fixture tests%s ---\n",
                regenerate ? " (REGENERATING)" : "");

    for (const FixtureSpec& spec : kFixtures) {
        const QByteArray generated = spec.build();
        check(!generated.isEmpty(), "fixture generator produced bytes",
              QLatin1String(spec.name));

        if (regenerate) {
            check(writeFixture(QLatin1String(spec.name), generated),
                  "fixture written", QLatin1String(spec.name));
            std::printf("      wrote %s (%lld bytes)\n", spec.name,
                        static_cast<long long>(generated.size()));
        }

        bool readOk = false;
        const QByteArray committed = readFixture(QLatin1String(spec.name), &readOk);
        check(readOk, "committed fixture readable", QLatin1String(spec.name));
        // byte-for-byte: the committed golden equals the deterministic
        // generator output on this platform/build
        check(committed == generated, "fixture bytes match generator exactly",
              QLatin1String(spec.name));

        const ValidationReport rep = JournalValidator::validateBytes(committed);
        check(rep.classification == spec.expected,
              "fixture classification as expected",
              QStringLiteral("%1: %2 (expected %3)")
                  .arg(QLatin1String(spec.name))
                  .arg(QLatin1String(journalClassificationName(rep.classification)))
                  .arg(QLatin1String(journalClassificationName(spec.expected))));
        if (spec.expectedValidPrefix >= 0)
            check(rep.validPrefixCount == spec.expectedValidPrefix,
                  "fixture valid-prefix length as expected",
                  QStringLiteral("%1: %2 (expected %3)")
                      .arg(QLatin1String(spec.name))
                      .arg(rep.validPrefixCount)
                      .arg(spec.expectedValidPrefix));
    }

    // fixtures replay: the clean finals fixture folds to a coherent state
    {
        bool ok = false;
        const QByteArray bytes =
            readFixture(QStringLiteral("fixture_finals_clean.jsonl"), &ok);
        if (ok) {
            const ValidationReport rep = JournalValidator::validateBytes(bytes);
            SessionState state;
            bool folded = true;
            for (const EventEnvelope& e : rep.validEnvelopes) {
                const ReduceResult r = SessionReducer::apply(state, e);
                if (!r.ok) {
                    folded = false;
                    break;
                }
                state = r.state;
            }
            check(folded && state.lifecycle == Lifecycle::Closed
                      && state.officials.size() == 3
                      && state.totalTenths == 104 + 98 + 101,
                  "finals fixture replays to the expected closed state",
                  QString::number(state.totalTenths));
        }
    }
}
