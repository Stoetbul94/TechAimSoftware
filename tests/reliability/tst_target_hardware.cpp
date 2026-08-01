// Tech Aim 0.9.0-RC2 — target serial selection and paper feed.
//
// Two defects from the first physical field test:
//   AUTOMATIC COM DETECTION — FAIL. RC1 opened ports in enumeration order and
//   kept the first that OPENED. A Bluetooth serial port opens with nothing
//   behind it, so COM5 could be chosen over the real CH340 on COM4.
//   AUTOMATIC PAPER FEED — FAIL. The feed at the accepted-shot path was
//   commented out; the only live call sat in a ListView delegate.
//
// Both are now decided in C++ with injected dependencies, so every case below
// runs with no hardware, no Modbus and no QML.
#include "target/TargetDeviceFingerprint.h"
#include "target/PaperFeedCoordinator.h"
#include "test_support.h"

#include <QVariantMap>
#include <QVector>

using namespace ta::target;

namespace {

SerialDeviceInfo dev(const char* port, const char* desc, const char* manu = "",
                     bool hasIds = false, quint16 vid = 0, quint16 pid = 0,
                     const char* serial = "")
{
    SerialDeviceInfo d;
    d.portName = QLatin1String(port);
    d.systemLocation = QStringLiteral("\\\\.\\%1").arg(QLatin1String(port));
    d.description = QLatin1String(desc);
    d.manufacturer = QLatin1String(manu);
    d.serialNumber = QLatin1String(serial);
    d.hasVendorId = hasIds;  d.vendorId = vid;
    d.hasProductId = hasIds; d.productId = pid;
    return d;
}

// THE OBSERVED FIELD MACHINE. This exact set is the regression case.
SerialDeviceInfo ch340()    { return dev("COM4", "USB-SERIAL CH340", "wch.cn", true, 0x1A86, 0x7523); }
SerialDeviceInfo bt5()      { return dev("COM5", "Standard Serial over Bluetooth link", "Microsoft"); }
SerialDeviceInfo bt6()      { return dev("COM6", "Standard Serial over Bluetooth link", "Microsoft"); }
QVector<SerialDeviceInfo> fieldMachine() { return { ch340(), bt5(), bt6() }; }

TargetDeviceFingerprint noneRemembered() { return TargetDeviceFingerprint(); }

} // namespace

void run_target_hardware_tests()
{
    fputs("\n--- RC2 target serial selection + paper feed ---\n", stdout);

    // ══ SERIAL SELECTION ═══════════════════════════════════════════════════

    // 1. no ports at all
    {
        const SelectionResult r = TargetDeviceSelector::select({}, noneRemembered());
        check(r.outcome == SelectionOutcome::NoCandidates, "1. no ports -> no candidates");
        check(r.selected.portName.isEmpty(), "1. nothing is selected");
    }

    // 2. ONLY Bluetooth ports - the RC1 trap
    {
        const SelectionResult r = TargetDeviceSelector::select({ bt5(), bt6() }, noneRemembered());
        check(r.outcome == SelectionOutcome::NoCandidates,
              "2. only Bluetooth ports -> no candidates (never auto-connect)");
        check(r.rejected.size() == 2, "2. both Bluetooth ports are rejected");
        for (const CandidateDevice& c : r.rejected)
            check(c.reject == RejectReason::BluetoothLink,
                  "2. rejection reason is recorded as the Bluetooth link");
    }

    // 3 + 4. THE FIELD REGRESSION: COM4 CH340 + COM5/COM6 Bluetooth -> COM4
    {
        const SelectionResult r = TargetDeviceSelector::select(fieldMachine(), noneRemembered());
        check(r.outcome == SelectionOutcome::AutoConnect,
              "3. the observed field machine yields a confident automatic choice");
        check(r.selected.portName == QLatin1String("COM4"),
              "4. COM4 (USB-SERIAL CH340) is selected, NOT the first enumerated port",
              r.selected.portName);
        check(r.candidates.size() == 1, "4. exactly one candidate survives filtering");
        check(r.rejected.size() == 2, "4. both Bluetooth links are rejected before any port opens");
    }

    // 4b. enumeration ORDER must not matter - Bluetooth first is the worst case
    {
        const SelectionResult r = TargetDeviceSelector::select({ bt5(), bt6(), ch340() },
                                                              noneRemembered());
        check(r.selected.portName == QLatin1String("COM4"),
              "4b. Bluetooth enumerated first still yields COM4", r.selected.portName);
    }

    // 5. a remembered target is found
    {
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ch340());
        check(fp.valid, "5. a fingerprint is built from the CH340");
        const SelectionResult r = TargetDeviceSelector::select(fieldMachine(), fp);
        check(r.outcome == SelectionOutcome::AutoConnect, "5. remembered target auto-connects");
        check(r.selected.portName == QLatin1String("COM4"), "5. and it is COM4");
        check(r.candidates.first().matchesRemembered, "5. the match is flagged as remembered");
    }

    // 6. THE COM-NUMBER CHANGE. Same adapter, new socket, new COM number.
    {
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ch340());
        SerialDeviceInfo moved = ch340();
        moved.portName = QStringLiteral("COM9");
        moved.systemLocation = QStringLiteral("\\\\.\\COM9");
        const SelectionResult r = TargetDeviceSelector::select({ bt5(), moved, bt6() }, fp);
        check(r.outcome == SelectionOutcome::AutoConnect,
              "6. the same adapter on a NEW COM number is still recognised");
        check(r.selected.portName == QLatin1String("COM9"),
              "6. and its current port name is used", r.selected.portName);
        check(fp.lastKnownPortName == QLatin1String("COM4"),
              "6. the stored port name is informational only and did not block the match");
    }

    // 7. WITH a serial number -> strongest identity
    {
        const SerialDeviceInfo withSerial =
            dev("COM4", "USB-SERIAL CH340", "wch.cn", true, 0x1A86, 0x7523, "TA-0001");
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(withSerial);
        check(fp.strength == FingerprintStrength::SerialVidPid,
              "7. serial + VID + PID is the strongest identity",
              fingerprintStrengthName(fp.strength));
        // Two identical adapters, only one with the stored serial number.
        SerialDeviceInfo other = withSerial; other.portName = QStringLiteral("COM7");
        other.serialNumber = QStringLiteral("TA-0002");
        const SelectionResult r = TargetDeviceSelector::select({ other, withSerial }, fp);
        check(r.selected.portName == QLatin1String("COM4"),
              "7. the serial number distinguishes two otherwise identical adapters",
              r.selected.portName);
    }

    // 8. WITHOUT a serial number -> VID/PID + manufacturer + description
    {
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ch340());
        check(fp.strength == FingerprintStrength::VidPidManufacturerDescription,
              "8. no serial number falls back to VID+PID+manufacturer+description",
              fingerprintStrengthName(fp.strength));
        check(fp.matches(ch340()), "8. and it still matches the same adapter");
    }

    // 9. several plausible USB candidates -> ASK, never guess
    {
        const SerialDeviceInfo ftdi = dev("COM8", "USB Serial Port (FT232R)", "FTDI",
                                          true, 0x0403, 0x6001);
        const SelectionResult r = TargetDeviceSelector::select({ ch340(), ftdi, bt5() },
                                                              noneRemembered());
        check(r.outcome == SelectionOutcome::NeedsUserChoice,
              "9. two plausible adapters require the operator to confirm");
        check(r.candidates.size() == 2, "9. both are offered");
        check(r.selected.portName.isEmpty(), "9. nothing is auto-selected");
    }

    // 10. a remembered choice settles case 9 without asking again
    {
        const SerialDeviceInfo ftdi = dev("COM8", "USB Serial Port (FT232R)", "FTDI",
                                          true, 0x0403, 0x6001);
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ftdi);
        const SelectionResult r = TargetDeviceSelector::select({ ch340(), ftdi, bt5() }, fp);
        check(r.outcome == SelectionOutcome::AutoConnect,
              "10. a remembered manual choice is honoured next time");
        check(r.selected.portName == QLatin1String("COM8"),
              "10. even though another plausible adapter is present", r.selected.portName);
    }

    // 11. forgetting restores the ask
    {
        const SerialDeviceInfo ftdi = dev("COM8", "USB Serial Port (FT232R)", "FTDI",
                                          true, 0x0403, 0x6001);
        const SelectionResult r = TargetDeviceSelector::select({ ch340(), ftdi },
                                                              noneRemembered());
        check(r.outcome == SelectionOutcome::NeedsUserChoice,
              "11. after Forget, the operator is asked again rather than guessed at");
    }

    // 12. target absent, then connected later
    {
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ch340());
        const SelectionResult before = TargetDeviceSelector::select({ bt5(), bt6() }, fp);
        check(before.outcome == SelectionOutcome::NoCandidates,
              "12. target absent at startup -> not detected");
        const SelectionResult after = TargetDeviceSelector::select(fieldMachine(), fp);
        check(after.outcome == SelectionOutcome::AutoConnect,
              "12. and it is picked up when plugged in later");
    }

    // 13. busy/occupied port is ranked down but still offered
    {
        SerialDeviceInfo busy = ch340();
        busy.busyKnown = true; busy.busy = true;
        const SelectionResult r = TargetDeviceSelector::select({ busy, bt5() }, noneRemembered());
        check(r.outcome == SelectionOutcome::AutoConnect,
              "13. an occupied port is still offered - isBusy() is unreliable on Windows");
        check(r.candidates.first().score < 150,
              "13. but it is ranked lower than a free one");
    }

    // 14. a port with no identity at all is not offered automatically
    {
        const SelectionResult r = TargetDeviceSelector::select({ dev("COM3", "", "") },
                                                              noneRemembered());
        check(r.outcome == SelectionOutcome::NoCandidates,
              "14. a port with no VID/PID and no description is not auto-selected");
        check(r.rejected.first().reject == RejectReason::NoUsbIdentity,
              "14. and the reason is recorded");
    }

    // 15. determinism - the same list always gives the same answer
    {
        const SelectionResult a = TargetDeviceSelector::select(fieldMachine(), noneRemembered());
        const SelectionResult b = TargetDeviceSelector::select(fieldMachine(), noneRemembered());
        check(a.selected.portName == b.selected.portName,
              "15. selection is deterministic");
    }

    // 16. fingerprint survives a QSettings round trip
    {
        const TargetDeviceFingerprint fp = TargetDeviceFingerprint::fromDevice(ch340());
        const TargetDeviceFingerprint back = TargetDeviceFingerprint::fromMap(fp.toMap());
        check(back.valid && back.strength == fp.strength,
              "16. a stored fingerprint round-trips through QSettings");
        check(back.matches(ch340()), "16. and still matches after reload");
    }

    // 17. a corrupt stored fingerprint is refused, not obeyed
    {
        QVariantMap bad; bad[QStringLiteral("valid")] = true;
        bad[QStringLiteral("strength")] = 99;
        check(!TargetDeviceFingerprint::fromMap(bad).valid,
              "17. an out-of-range stored strength is refused");
    }

    // 18. the Bluetooth filter is wording-tolerant
    {
        check(looksLikeBluetooth(dev("COM5", "Bluetooth Serial Port", "")),
              "18. 'Bluetooth Serial Port' is rejected");
        check(looksLikeBluetooth(dev("COM6", "Serial over RFCOMM", "")),
              "18. an RFCOMM link is rejected");
        check(looksLikeBluetooth(dev("COM7", "Some Port", "BLUETOOTH Inc")),
              "18. the manufacturer field is checked too");
        check(!looksLikeBluetooth(ch340()), "18. the CH340 is NOT rejected");
    }

    // ══ PAPER FEED ═════════════════════════════════════════════════════════

    struct Rig {
        QVector<double> commands;      // one entry per motor command issued
        QStringList logLines;
        PaperFeedCoordinator co;
        Rig() {
            co.setMotorCommand([this](double s) { commands.append(s); return true; });
            co.setLogSink([this](const QString& l) { logLines.append(l); });
            FeedContext c;
            c.liveMode = true; c.targetConnected = true; c.replaying = false;
            c.matchDurationSeconds = 1.0;      // Arnold's actual settings
            c.sighterDurationSeconds = 1.0;
            co.setContext(c);
            co.beginSession(QStringLiteral("S1"));
        }
    };
    auto shot = [](qint64 id, ShotKind k) {
        FeedRequest r; r.shotIdentity = id; r.kind = k; return r;
    };

    // 19-20. one accepted shot -> exactly one feed, per shot type
    {
        Rig rig;
        rig.co.onShotAccepted(shot(1, ShotKind::Sighter));
        check(rig.commands.size() == 1, "19. one accepted sighter -> exactly one motor command");
        rig.co.onShotAccepted(shot(2, ShotKind::Counted));
        check(rig.commands.size() == 2, "20. one accepted counted shot -> exactly one more");
    }

    // 21. THE SIGHTER-DURATION DEFECT. RC1 passed the match time for both.
    {
        Rig rig;
        FeedContext c = rig.co.context();
        c.sighterDurationSeconds = 0.5;    // deliberately different
        c.matchDurationSeconds = 2.0;
        rig.co.setContext(c);
        rig.co.onShotAccepted(shot(1, ShotKind::Sighter));
        rig.co.onShotAccepted(shot(2, ShotKind::Counted));
        check(rig.commands.size() == 2, "21. two shots -> two commands");
        check(qAbs(rig.commands[0] - 0.5) < 1e-9,
              "21. the SIGHTER feeds for the sighter duration",
              QString::number(rig.commands[0]));
        check(qAbs(rig.commands[1] - 2.0) < 1e-9,
              "21. the COUNTED shot feeds for the match duration",
              QString::number(rig.commands[1]));
    }

    // 22. duplicate shot identity -> one feed only
    {
        Rig rig;
        rig.co.onShotAccepted(shot(7, ShotKind::Counted));
        rig.co.onShotAccepted(shot(7, ShotKind::Counted));   // repeated frame
        check(rig.commands.size() == 1, "22. a repeated shot identity feeds only once");
        check(rig.co.duplicatesPrevented() == 1, "22. and the duplicate is counted");
    }

    // 23. Demo mode never drives the motor
    {
        Rig rig;
        FeedContext c = rig.co.context(); c.liveMode = false; rig.co.setContext(c);
        const FeedDecision d = rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        check(rig.commands.isEmpty(), "23. a Demo shot issues NO physical motor command");
        check(d.skip == FeedSkipReason::NotLiveMode, "23. and the reason is recorded");
    }

    // 24. recovery replay never feeds - the worst possible failure
    {
        Rig rig;
        FeedContext c = rig.co.context(); c.replaying = true; rig.co.setContext(c);
        for (int i = 1; i <= 10; ++i) rig.co.onShotAccepted(shot(i, ShotKind::Counted));
        check(rig.commands.isEmpty(),
              "24. replaying ten recovered shots issues NO motor command");
    }

    // 25. restart does not feed for shots already recorded
    {
        Rig rig;
        FeedContext c = rig.co.context(); c.replaying = true; rig.co.setContext(c);
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));   // recovered
        c.replaying = false; rig.co.setContext(c);
        rig.co.onShotAccepted(shot(2, ShotKind::Counted));   // genuinely new
        check(rig.commands.size() == 1,
              "25. after recovery only the NEW shot feeds");
    }

    // 26. disconnected target -> no command, logged
    {
        Rig rig;
        FeedContext c = rig.co.context(); c.targetConnected = false; rig.co.setContext(c);
        const FeedDecision d = rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        check(rig.commands.isEmpty(), "26. no motor command while disconnected");
        check(d.skip == FeedSkipReason::TargetDisconnected, "26. and the reason is recorded");
    }

    // 27. duration zero disables that shot type only
    {
        Rig rig;
        FeedContext c = rig.co.context(); c.sighterDurationSeconds = 0.0; rig.co.setContext(c);
        rig.co.onShotAccepted(shot(1, ShotKind::Sighter));
        check(rig.commands.isEmpty(), "27. sighter duration 0 -> no sighter feed");
        rig.co.onShotAccepted(shot(2, ShotKind::Counted));
        check(rig.commands.size() == 1, "27. counted shots still feed");
    }

    // 28. rapid shots are serialised, never overlapped
    {
        Rig rig;
        int maxDepthSeen = 0;
        rig.co.setMotorCommand([&](double s) {
            // Re-entrancy probe: a shot arriving mid-feed must queue, not
            // start a second overlapping register write.
            rig.commands.append(s);
            if (rig.commands.size() == 1) {
                rig.co.onShotAccepted(shot(99, ShotKind::Counted));
                maxDepthSeen = rig.co.queuedCount();
            }
            return true;
        });
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        check(rig.commands.size() == 2,
              "28. a shot arriving mid-feed is preserved and fed after",
              QString::number(rig.commands.size()));
        check(maxDepthSeen >= 1, "28. it was queued rather than run concurrently");
        check(!rig.co.motorBusy(), "28. the motor is released when the queue drains");
    }

    // 29. sessions scope the duplicate set
    {
        Rig rig;
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        rig.co.beginSession(QStringLiteral("S2"));
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));   // same id, new session
        check(rig.commands.size() == 2,
              "29. shot 1 of a NEW session is not suppressed by the old session");
    }

    // 30. manual feed still works and is logged distinctly
    {
        Rig rig;
        check(rig.co.requestManualFeed(1.0), "30. manual feed works");
        check(rig.commands.size() == 1, "30. and issues one command");
        bool tagged = false;
        for (const QString& l : rig.logLines)
            if (l.contains(QLatin1String("manual feed"), Qt::CaseInsensitive)) tagged = true;
        check(tagged, "30. manual feed is logged distinctly from automatic feeding");
    }

    // 31. manual feed does not consume a shot identity
    {
        Rig rig;
        rig.co.requestManualFeed(1.0);
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        check(rig.commands.size() == 2,
              "31. a manual feed does not suppress the next automatic feed");
    }

    // 32. motor failure is reported, and the shot is not lost
    {
        Rig rig;
        rig.co.setMotorCommand([&](double s) { rig.commands.append(s); return false; });
        rig.co.onShotAccepted(shot(1, ShotKind::Counted));
        check(rig.commands.size() == 1, "32. the command was attempted");
        check(rig.co.feedsIssued() == 0, "32. and not counted as a successful feed");
        bool logged = false;
        for (const QString& l : rig.logLines)
            if (l.contains(QLatin1String("FAILED"))) logged = true;
        check(logged, "32. the motor failure is logged");
    }

    // 33. duration validation
    {
        QString why;
        check(qAbs(PaperFeedCoordinator::sanitiseDuration(1.0, &why) - 1.0) < 1e-9,
              "33. a 1.0 s decimal duration is preserved exactly");
        check(why.isEmpty(), "33. and needs no correction");
        check(PaperFeedCoordinator::sanitiseDuration(-3.0, &why) == 0.0,
              "33. a negative duration is refused, not applied");
        check(!why.isEmpty(), "33. and the refusal is explained");
        check(PaperFeedCoordinator::sanitiseDuration(0.0) == 0.0,
              "33. zero is legal and means disabled");
        check(PaperFeedCoordinator::sanitiseDuration(9999.0) == kMaxFeedSeconds,
              "33. an absurd duration is clamped to the ceiling");
    }

    // 34. no motor command bound -> nothing happens, nothing crashes
    {
        PaperFeedCoordinator bare;
        FeedContext c; c.liveMode = true; c.targetConnected = true;
        c.matchDurationSeconds = 1.0;
        bare.setContext(c);
        bare.onShotAccepted(shot(1, ShotKind::Counted));
        check(bare.feedsIssued() == 0, "34. with no motor bound nothing is issued and nothing crashes");
    }
}
