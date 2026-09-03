// THE NODE'S DURABLE HANDLED-COMMAND JOURNAL (R2C §2, §3, §6).
//
// R2B found that command idempotency lived only inside one process. This suite
// holds the store that fixes it to three things:
//
//   1. it remembers ENOUGH - the original outcome, not merely "seen before";
//   2. it is BOUNDED, so it cannot become a disk leak a peer could drive;
//   3. the bound NEVER evicts an entry whose command could still be
//      dangerously re-executed. That rule outranks both the count and the age.
//
// The third is the one that matters. A store that stayed within its budget by
// forgetting the START_AT of the match currently running would satisfy every
// other test here and re-arm the exact failure it exists to prevent.

#include "test_support.h"

#include "rms/control/CommandJournal.h"

#include <QJsonDocument>
#include <QTemporaryDir>

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;

namespace {

HandledCommand entry(const QString& id, const char* type, const QString& session,
                     qint64 at, bool accepted = true)
{
    HandledCommand h;
    h.commandId = id;
    h.commandType = QLatin1String(type);
    h.nodeId = QStringLiteral("TA-NODE-001");
    h.sessionId = session;
    h.accepted = accepted;
    h.reasonCode = QLatin1String(accepted ? reason::kOk : reason::kPreconditionFailed);
    h.message = QStringLiteral("m-%1").arg(id);
    h.resultingState = QJsonObject{{"phase", "STARTED"}};
    h.processedAtUtcMs = at;
    return h;
}

void testWhichCommandsAreDurable()
{
    // State-changing and physical commands must survive; reads need not.
    check(isDurableCommand(QLatin1String(cmd::kAssignAthlete)),
          "journal: ASSIGN_ATHLETE is durable");
    check(isDurableCommand(QLatin1String(cmd::kPrepareSession)),
          "journal: PREPARE_SESSION is durable");
    check(isDurableCommand(QLatin1String(cmd::kStartAt)),
          "journal: START_AT is durable");
    check(isDurableCommand(QLatin1String(cmd::kStop)),
          "journal: STOP is durable");
    check(isDurableCommand(QLatin1String(cmd::kFeedPaper)),
          "journal: FEED_PAPER is durable before it is ever enabled");

    check(!isDurableCommand(QLatin1String(cmd::kPing)),
          "journal: PING is not journalled - repeating it is harmless");
    check(!isDurableCommand(QLatin1String(cmd::kRequestStatus)),
          "journal: REQUEST_STATUS is not journalled");
    check(!isDurableCommand(QLatin1String(cmd::kRequestReplay)),
          "journal: REQUEST_REPLAY is not journalled - it reads and changes nothing");
}

void testRecallReproducesTheOriginalOutcome()
{
    CommandJournal j;
    j.setCurrentSession(QStringLiteral("sess-1"));
    j.record(entry(QStringLiteral("cmd-1"), cmd::kStartAt, QStringLiteral("sess-1"), 1000));
    // A REFUSAL is remembered too. A retried command that was refused must be
    // refused again with the same reason, not quietly allowed the second time.
    j.record(entry(QStringLiteral("cmd-2"), cmd::kStop, QStringLiteral("sess-1"), 1100, false));

    HandledCommand got;
    check(j.recall(QStringLiteral("cmd-1"), &got), "journal: an accepted command is recalled");
    check(got.accepted, "journal: recalled as accepted");
    check(got.resultingState.value("phase").toString() == QLatin1String("STARTED"),
          "journal: the RESULTING STATE comes back, not just the fact of it");

    const Ack a = got.toAck(9999);
    check(a.duplicate, "journal: the rebuilt ack is marked duplicate");
    check(a.accepted, "journal: and carries the ORIGINAL outcome");
    check(a.reasonCode == QLatin1String(reason::kOk),
          "journal: with the original reason code, not ALREADY_EXECUTED");
    check(a.commandId == QLatin1String("cmd-1"), "journal: for the right command");
    check(a.nodeTimestampUtcMs == 9999, "journal: stamped now, not then");

    check(j.recall(QStringLiteral("cmd-2"), &got), "journal: a refused command is recalled");
    check(!got.accepted && got.toAck(1).accepted == false,
          "journal: a refusal is replayed as a refusal");

    check(!j.recall(QStringLiteral("never-sent"), &got),
          "journal: an unknown id is not recalled");
}

void testCountBoundIsEnforced()
{
    CommandJournal j;
    // No current session, so nothing is protected and the bound applies cleanly.
    for (int i = 0; i < CommandJournal::kMaxEntries + 60; ++i)
        j.record(entry(QStringLiteral("c-%1").arg(i), cmd::kAssignAthlete,
                       QStringLiteral("sess-old"), 1000 + i));

    check(j.size() == CommandJournal::kMaxEntries,
          "journal: the count bound holds - it does not grow forever");
    check(j.evicted() == 60, "journal: exactly the overflow was evicted");
    check(j.retainedOverBudget() == 0, "journal: nothing had to be retained over budget");

    HandledCommand got;
    check(!j.recall(QStringLiteral("c-0"), &got), "journal: the oldest went first");
    check(j.recall(QStringLiteral("c-%1").arg(CommandJournal::kMaxEntries + 59), &got),
          "journal: the newest is still there");
}

void testCurrentSessionIsNeverEvicted()
{
    CommandJournal j;
    j.setCurrentSession(QStringLiteral("live"));

    // The command that must not be forgotten: the running match's START_AT.
    j.record(entry(QStringLiteral("the-start"), cmd::kStartAt,
                   QStringLiteral("live"), 1000));

    // Then a flood from finished sessions, far past the bound.
    for (int i = 0; i < CommandJournal::kMaxEntries + 200; ++i)
        j.record(entry(QStringLiteral("old-%1").arg(i), cmd::kAssignAthlete,
                       QStringLiteral("sess-finished"), 2000 + i));

    HandledCommand got;
    check(j.recall(QStringLiteral("the-start"), &got),
          "journal: the LIVE session's START_AT survived a flood past the bound");
    check(j.size() <= CommandJournal::kMaxEntries,
          "journal: and the store still respects its bound");

    // Age cannot evict it either.
    const qint64 muchLater = 1000 + CommandJournal::kRetentionMs * 4;
    j.pruneOlderThan(muchLater);
    check(j.recall(QStringLiteral("the-start"), &got),
          "journal: nor does the age bound evict a live session's command");

    // Once the session is over it becomes evictable like anything else -
    // otherwise the store could never shrink.
    j.setCurrentSession(QStringLiteral("a-later-session"));
    j.pruneOlderThan(muchLater);
    check(!j.recall(QStringLiteral("the-start"), &got),
          "journal: once its session is finished, it can finally be pruned");
}

void testEverythingProtectedIsVisible()
{
    CommandJournal j;
    j.setCurrentSession(QStringLiteral("live"));
    // More protected entries than the budget. Keeping them is CORRECT - the
    // alternative is dropping a command that can still be re-executed - but it
    // must be counted rather than hidden.
    for (int i = 0; i < CommandJournal::kMaxEntries + 5; ++i)
        j.record(entry(QStringLiteral("p-%1").arg(i), cmd::kStartAt,
                       QStringLiteral("live"), 3000 + i));

    check(j.size() == CommandJournal::kMaxEntries + 5,
          "journal: protected entries are kept even over budget");
    check(j.retainedOverBudget() > 0,
          "journal: and the over-budget retention is REPORTED, never silent");
    check(j.evicted() == 0, "journal: nothing dangerous was dropped to make room");
}

void testAgeBound()
{
    CommandJournal j;
    j.setCurrentSession(QStringLiteral("live"));
    const qint64 t0 = 1'700'000'000'000LL;
    j.record(entry(QStringLiteral("stale"), cmd::kStop, QStringLiteral("done"), t0));
    j.record(entry(QStringLiteral("fresh"), cmd::kStop, QStringLiteral("done"),
                   t0 + CommandJournal::kRetentionMs));

    const int pruned = j.pruneOlderThan(t0 + CommandJournal::kRetentionMs + 1000);
    check(pruned == 1, "journal: exactly the aged-out entry was pruned");
    HandledCommand got;
    check(!j.recall(QStringLiteral("stale"), &got), "journal: the old one is gone");
    check(j.recall(QStringLiteral("fresh"), &got), "journal: the recent one is kept");
}

void testPersistenceRoundTrip()
{
    QTemporaryDir dir;
    check(dir.isValid(), "journal: temporary directory");
    const QString path = dir.path() + QStringLiteral("/command_journal.json");

    {
        CommandJournal j;
        j.setCurrentSession(QStringLiteral("sess-1"));
        j.record(entry(QStringLiteral("durable-1"), cmd::kStartAt,
                       QStringLiteral("sess-1"), 5000));
        j.record(entry(QStringLiteral("durable-2"), cmd::kAssignAthlete,
                       QStringLiteral("sess-1"), 5100, false));
        j.record(entry(QStringLiteral("chatter"), cmd::kPing,
                       QStringLiteral("sess-1"), 5200));
        check(j.durableCount() == 2, "journal: two durable entries, one diagnostic");

        RmsJsonStore store(path);
        const StoreResult r = j.saveTo(store);
        check(r.ok, "journal: saved", r.detail);
    }

    {
        CommandJournal j;
        RmsJsonStore store(path);
        const StoreResult r = j.loadFrom(store);
        check(r.ok, "journal: reloaded", r.detail);

        HandledCommand got;
        check(j.recall(QStringLiteral("durable-1"), &got), "journal: START_AT survived");
        check(got.accepted && got.reasonCode == QLatin1String(reason::kOk),
              "journal: with its outcome intact");
        check(got.resultingState.value("phase").toString() == QLatin1String("STARTED"),
              "journal: and its resulting state");
        check(j.recall(QStringLiteral("durable-2"), &got), "journal: the refusal survived");
        check(!got.accepted, "journal: still a refusal");

        // Diagnostics were NOT written. Forgetting a PING across a restart
        // costs nothing, and spending the retention budget on heartbeats would
        // crowd out the entries that matter.
        check(!j.recall(QStringLiteral("chatter"), &got),
              "journal: the diagnostic was not persisted");
        check(j.currentSession() == QLatin1String("sess-1"),
              "journal: the session it applied to came back with it");
    }

    // NO SECRET MATERIAL. Not by convention - by looking.
    {
        CommandJournal j;
        j.setCurrentSession(QStringLiteral("sess-1"));
        j.record(entry(QStringLiteral("x"), cmd::kStartAt, QStringLiteral("sess-1"), 1));
        const QByteArray raw =
            QJsonDocument(j.saveState()).toJson(QJsonDocument::Compact);
        check(!raw.contains("mac") && !raw.contains("nonce") && !raw.contains("key"),
              "journal: the persisted document holds no mac, nonce or key");
    }

    // A document from a newer build is refused rather than half-read.
    {
        CommandJournal j;
        RmsJsonStore store(path);
        QJsonObject doc;
        const StoreResult r = store.load(0, &doc);
        check(!r.ok && r.error == StoreError::SchemaTooNew,
              "journal: an older build refuses a newer document");
    }
}

} // namespace

void run_journal_tests()
{
    std::printf("\n-- the node command journal --\n");
    testWhichCommandsAreDurable();
    testRecallReproducesTheOriginalOutcome();
    testCountBoundIsEnforced();
    testCurrentSessionIsNeverEvicted();
    testEverythingProtectedIsVisible();
    testAgeBound();
    testPersistenceRoundTrip();
    std::fflush(stdout);
}
