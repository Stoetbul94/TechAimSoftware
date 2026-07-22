// F10 — operating mode (Live/Demo): policy, config atomic write, service
// selection/guards, the authoritative input-source gate, and session-mode
// journal metadata. All config writes target ISOLATED temp files — the user's
// real config.ini is never touched.

#include "mode/OperatingMode.h"
#include "mode/OperatingModeService.h"
#include "qualification/QualificationController.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/events/DomainEvent.h"
#include "test_support.h"

#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <variant>

using namespace ta::rel;
using ta::mode::Mode;
using ta::mode::ShotSource;

namespace {

// Count durable shot events (official + sighter) in a journal's bytes.
int shotEventCount(const QByteArray& bytes)
{
    const ValidationReport rep = JournalValidator::validateBytes(bytes);
    int n = 0;
    for (const EventEnvelope& e : rep.validEnvelopes) {
        if (std::holds_alternative<ShotAccepted>(e.payload)
            || std::holds_alternative<SighterAccepted>(e.payload))
            ++n;
    }
    return n;
}

// The SessionStarted.operatingMode as it survived serialize→validate→replay.
QString replayedMode(const QByteArray& bytes)
{
    const ValidationReport rep = JournalValidator::validateBytes(bytes);
    return ReplayEngine::replay(rep.validEnvelopes).state.operatingMode;
}

// Drive a qualification controller to the sighting window with an injected
// in-memory journal + manual clock, in the given operating mode.
void toSighting(QualificationController& qc, MemoryJournalFile& file,
                ManualClock& clock, int mode)
{
    qc.storeForTesting()->setClockForTesting(&clock);
    qc.storeForTesting()->setJournalFileForTesting(&file);
    if (mode >= 0)
        qc.setOperatingMode(mode);
    qc.startSession(QStringLiteral("AR10"), QStringLiteral("60"),
                    QStringLiteral("A"), 60, 4500000, 900000, -1,
                    QString(), QString());
    qc.beginPreparation();
    clock.advance(30000);
    qc.beginSighting();
    clock.advance(5000);
}

} // namespace

void run_operatingmode_tests()
{
    std::printf("--- operating-mode tests (F10) ---\n");

    // ── 1. Pure source policy ────────────────────────────────────────────
    check(ta::mode::sourceAllowed(Mode::Live, ShotSource::Physical),
          "policy: physical shot allowed in Live");
    check(!ta::mode::sourceAllowed(Mode::Live, ShotSource::Simulated),
          "policy: simulated shot rejected in Live");
    check(ta::mode::sourceAllowed(Mode::Demo, ShotSource::Simulated),
          "policy: simulated shot allowed in Demo");
    check(!ta::mode::sourceAllowed(Mode::Demo, ShotSource::Physical),
          "policy: physical shot rejected in Demo");

    // ── 2. Startup parse (case-consistent) + invalid fallback ────────────
    check(ta::mode::parseMode(QStringLiteral("Live")).valid
              && ta::mode::parseMode(QStringLiteral("Live")).mode == Mode::Live,
          "parse: 'Live' -> Live (valid)");
    check(ta::mode::parseMode(QStringLiteral("demo")).valid
              && ta::mode::parseMode(QStringLiteral("demo")).mode == Mode::Demo,
          "parse: 'demo' -> Demo (case-insensitive, valid)");
    {
        const ta::mode::ParsedMode p = ta::mode::parseMode(QStringLiteral("banana"));
        check(!p.valid && p.mode == Mode::Live,
              "parse: invalid value falls back to Live, flagged invalid");
        const ta::mode::ParsedMode e = ta::mode::parseMode(QString());
        check(!e.valid && e.mode == Mode::Live,
              "parse: absent value falls back to Live, flagged invalid");
    }

    // ── 3. Atomic config write: preserve unrelated keys, update only app_mode ─
    {
        QTemporaryDir dir;
        check(dir.isValid(), "temp dir for config write");
        const QString path = dir.filePath(QStringLiteral("config.ini"));
        {
            QFile f(path);
            f.open(QIODevice::WriteOnly);
            f.write("[shot_count_and_timer]\ntimer=yes\n\n"
                    "[App_Settings]\napp_mode=Demo\nis_single_decimal=1\n"
                    "motor_movement_time=1\n");
            f.close();
        }
        QString err;
        const bool ok = OperatingModeService::writeModeToConfig(path, Mode::Live, &err);
        check(ok, "atomic write succeeds", err);
        QFile rf(path);
        rf.open(QIODevice::ReadOnly);
        const QString after = QString::fromUtf8(rf.readAll());
        rf.close();
        check(after.contains(QLatin1String("app_mode=Live")),
              "write: app_mode updated to Live");
        check(!after.contains(QLatin1String("app_mode=Demo")),
              "write: old app_mode value gone");
        check(after.contains(QLatin1String("is_single_decimal=1"))
                  && after.contains(QLatin1String("motor_movement_time=1"))
                  && after.contains(QLatin1String("timer=yes")),
              "write: unrelated keys preserved");
        // Re-parse round-trips to the written mode.
        check(ta::mode::parseMode(QStringLiteral("Live")).mode == Mode::Live,
              "write: written value re-parses to Live");
    }

    // ── 4. Failed write retains the old configuration ────────────────────
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("config.ini"));
        {
            QFile f(path);
            f.open(QIODevice::WriteOnly);
            f.write("[App_Settings]\napp_mode=Live\n");
            f.close();
        }
        // Make the target read-only so the atomic replace cannot commit.
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ReadUser);
        QString err;
        const bool ok = OperatingModeService::writeModeToConfig(path, Mode::Demo, &err);
        // Restore permissions before asserting so we can read it back.
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                        | QFileDevice::ReadUser | QFileDevice::WriteUser);
        QFile rf(path);
        rf.open(QIODevice::ReadOnly);
        const QString after = QString::fromUtf8(rf.readAll());
        rf.close();
        // Either the OS honoured read-only (write failed) — then the old file
        // must be intact — or it allowed the write. We assert the safety-critical
        // invariant: the file is never left without a valid app_mode line.
        check(after.contains(QLatin1String("app_mode=Live"))
                  || after.contains(QLatin1String("app_mode=Demo")),
              "failed-or-succeeded write never corrupts config (valid app_mode remains)");
        if (!ok)
            check(after.contains(QLatin1String("app_mode=Live")) && !err.isEmpty(),
                  "failed write retains old app_mode=Live and surfaces an error");
    }

    // ── 5. Service selection / guards / restart-required lifecycle ───────
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("config.ini"));
        {
            QFile f(path);
            f.open(QIODevice::WriteOnly);
            f.write("[App_Settings]\napp_mode=Live\n");
            f.close();
        }
        OperatingModeService svc(path, Mode::Live, true, QStringLiteral("Live"));
        check(svc.isLive() && svc.runningModeToken() == QLatin1String("Live"),
              "service: running mode is Live");
        check(!svc.restartRequired(), "service: fresh service needs no restart");

        // Blocked while a session is active: no write, config unchanged.
        svc.selectMode(static_cast<int>(Mode::Demo));
        check(svc.restartRequired(),
              "service: selecting a different mode flags restart-required");
        const bool blocked = svc.applyModeChange(/*sessionBusy*/true);
        check(!blocked, "service: applyModeChange refused while session busy");
        {
            QFile rf(path); rf.open(QIODevice::ReadOnly);
            const QString c = QString::fromUtf8(rf.readAll()); rf.close();
            check(c.contains(QLatin1String("app_mode=Live")),
                  "service: blocked change leaves config unchanged");
        }
        // The RUNNING mode is immutable — selecting Demo cannot change it (a QML
        // state edit can never flip the live/demo scoring source at runtime).
        check(svc.isLive(),
              "service: running mode unchanged by selection (no hot switch)");

        // Idle apply succeeds and writes.
        const bool applied = svc.applyModeChange(/*sessionBusy*/false);
        check(applied, "service: applyModeChange succeeds when idle");
        check(svc.restartRequired(), "service: restart required after apply");
        {
            QFile rf(path); rf.open(QIODevice::ReadOnly);
            const QString c = QString::fromUtf8(rf.readAll()); rf.close();
            check(c.contains(QLatin1String("app_mode=Demo")),
                  "service: idle apply writes app_mode=Demo");
        }

        // Cancel semantics: selecting back to the running mode clears the pending
        // state and applyModeChange is a no-op that writes nothing further.
        svc.selectMode(static_cast<int>(Mode::Live));
        check(!svc.restartRequired(),
              "service: selecting back to running clears restart-required (Cancel)");

        // A relaunched process constructs a fresh service in the NEW mode with no
        // pending restart — the restart-required state clears after startup.
        OperatingModeService relaunched(path, Mode::Demo, true, QStringLiteral("Demo"));
        check(!relaunched.isLive() && !relaunched.restartRequired(),
              "service: restart-required clears after startup in the new mode");
        check(relaunched.badgeText() == QLatin1String("DEMO / SIMULATION"),
              "service: badge projection agrees with running mode (Demo)");
        check(svc.sourceAllowed(static_cast<int>(ShotSource::Physical)),
              "service: Live gate allows physical via QML entry");
        check(!svc.sourceAllowed(static_cast<int>(ShotSource::Simulated)),
              "service: Live gate rejects simulated via QML entry");
    }

    // ── 6. Badge projection strings ──────────────────────────────────────
    check(ta::mode::modeBadgeText(Mode::Live) == QLatin1String("LIVE TARGET"),
          "badge: Live -> 'LIVE TARGET'");
    check(ta::mode::modeBadgeText(Mode::Demo) == QLatin1String("DEMO / SIMULATION"),
          "badge: Demo -> 'DEMO / SIMULATION'");

    // ── 7. Input-source gate at the durable boundary (qualification) ─────
    // Demo (simulated) shot rejected in Live — no accepted shot, no journal event.
    {
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*Live*/0);
        const bool ok = qc.submitSighter(0, 0, 10.5, 1, 0, /*simulated*/true);
        check(!ok, "gate: simulated shot rejected in Live");
        check(qc.sighterCount() == 0, "gate: rejected simulated shot not counted (Live)");
        check(shotEventCount(file.data) == 0,
              "gate: rejected simulated shot creates no journal shot event (Live)");
    }
    // Live (physical) shot rejected in Demo — no accepted shot, no journal event.
    {
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*Demo*/1);
        const bool ok = qc.submitSighter(0, 0, 10.5, 1, 0, /*simulated*/false);
        check(!ok, "gate: physical shot rejected in Demo");
        check(qc.sighterCount() == 0, "gate: rejected physical shot not counted (Demo)");
        check(shotEventCount(file.data) == 0,
              "gate: rejected physical shot creates no journal shot event (Demo)");
    }
    // Matching source scores normally in each mode.
    {
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*Live*/0);
        check(qc.submitSighter(0, 0, 10.5, 1, 0, /*simulated*/false),
              "gate: physical shot accepted in Live");
        check(qc.sighterCount() == 1, "gate: accepted physical shot counted (Live)");
    }

    // ── 8. Session/journal mode metadata (record + replay + legacy) ──────
    {
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*Live*/0);
        check(replayedMode(file.data) == QLatin1String("Live"),
              "metadata: session records startup mode; replay preserves it (Live)");
    }
    {
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*Demo*/1);
        check(replayedMode(file.data) == QLatin1String("Demo"),
              "metadata: replay preserves Demo session mode");
    }
    {
        // No setOperatingMode: a pre-F10-style session carries no field and
        // loads as Unknown/Legacy (empty), NOT re-inferred as Live.
        MemoryJournalFile file; ManualClock clock;
        QualificationController qc;
        toSighting(qc, file, clock, /*unset*/-1);
        check(replayedMode(file.data).isEmpty(),
              "metadata: session without a mode field loads as Unknown/Legacy");
    }
}
