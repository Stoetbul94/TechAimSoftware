// Android USB transport state machine (USB-AND-001).
//
// §28 requires these to run with no USB hardware, and that is not a
// convenience - it is the only way several of these cases can be tested at
// all. Permission denial, a read failure mid-session and a detach at exactly
// the wrong moment cannot be provoked on demand with a real cable, and they
// are precisely the cases where a transport quietly lies.
//
// The scripted bridge below IS the Android side for the purposes of these
// tests. It is deliberately dumb: it returns what the test told it to return,
// so every assertion is about the state machine and never about Android.

#include "test_support.h"

#include "target/AndroidUsbTransport.h"

#include <QByteArray>
#include <QList>
#include <cstdio>

using ta::AndroidUsbTransport;
using ta::IUsbDeviceBridge;
using ta::SerialLineConfig;
using ta::UsbDeviceId;
using State = ta::AndroidUsbTransport::State;

namespace {

const UsbDeviceId kCh340{0x1A86, 0x7523};
const UsbDeviceId kCh341{0x1A86, 0x5523};
const UsbDeviceId kNotOurs{0x2341, 0x0043};   // an Arduino: serial, not ours

class ScriptedBridge : public IUsbDeviceBridge
{
public:
    QList<UsbDeviceId> devices;
    bool permission      = false;
    bool requestAccepted = true;
    bool openSucceeds    = true;
    int  readResult      = 0;
    int  writeResult     = 0;
    QString error;

    int  openCalls  = 0;
    int  closeCalls = 0;
    int  permissionRequests = 0;
    SerialLineConfig lastConfig;

    QList<UsbDeviceId> enumerate() override { return devices; }
    bool hasPermission(const UsbDeviceId&) override { return permission; }
    bool requestPermission(const UsbDeviceId&) override
    { ++permissionRequests; return requestAccepted; }
    bool open(const UsbDeviceId&, const SerialLineConfig& cfg) override
    { ++openCalls; lastConfig = cfg; return openSucceeds; }
    void close() override { ++closeCalls; }
    int  write(const QByteArray&) override { return writeResult; }
    int  read(QByteArray* out, int, int) override
    {
        // A REAL failing driver does not helpfully clear your buffer. It
        // returns an error and leaves whatever was there. The test models that,
        // because the transport is what must refuse to pass it on.
        if (readResult < 0) {
            if (out) out->append("STALE");
            return -1;
        }
        if (out) out->append(QByteArray(readResult, 'x'));
        return readResult;
    }
    QString lastError() const override { return error; }
};

} // namespace

void run_android_usb_tests()
{
    printf("\n--- Android USB transport (USB-AND-001) ---\n");
    fflush(stdout);

    // ── device identity: a whitelist, not "anything with endpoints" ───────
    {
        check(ta::isSupportedCh340(kCh340),
              "USB-AND-001: CH340 (VID 0x1A86 PID 0x7523) is recognised");
        check(ta::isSupportedCh340(kCh341),
              "USB-AND-001: CH341 (PID 0x5523) is recognised");
        check(!ta::isSupportedCh340(kNotOurs),
              "USB-AND-001: another USB serial device is NOT adopted as a "
              "target merely because it is a serial device",
              ta::describeUsbId(kNotOurs));
    }

    // ── the settings that reach the chip are the FIELD settings ───────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(1000);
        check(t.state() == State::Ready, "USB-AND-001: opens when permitted");
        check(b.lastConfig.baud == 19200 && b.lastConfig.parity == 'E'
              && b.lastConfig.dataBits == 8 && b.lastConfig.stopBits == 1
              && b.lastConfig.rts == false,
              "USB-AND-001: the port is configured 19200/Even/8/1 with RTS "
              "deasserted - the field values, not the legacy 9600/None",
              QString("%1/%2/%3/%4 rts=%5").arg(b.lastConfig.baud)
                  .arg(QChar(b.lastConfig.parity)).arg(b.lastConfig.dataBits)
                  .arg(b.lastConfig.stopBits).arg(b.lastConfig.rts));
    }

    // ── no device ─────────────────────────────────────────────────────────
    {
        ScriptedBridge b;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(t.state() == State::NoDevice && !t.isReady(),
              "USB-AND-001: no device means NoDevice and not ready");
    }

    // ── §10 app started with the target ALREADY attached ──────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(500);
        check(t.state() == State::Ready,
              "USB-AND-001: a target attached BEFORE launch reaches Ready on "
              "the first poll - no unplug/replug required");
    }

    // ── §9 permission granted ─────────────────────────────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = false;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(t.state() == State::PermissionRequested,
              "USB-AND-001: an unpermitted device asks, and WAITS");
        check(b.openCalls == 0,
              "USB-AND-001: and does not open anything before the answer");
        b.permission = true;
        t.onPermissionResult(true, 100);
        check(t.state() == State::Ready,
              "USB-AND-001: granting permission proceeds to Ready");
    }

    // ── §9 permission denied: stated, and not re-asked forever ────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = false;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        t.onPermissionResult(false, 10);
        check(t.state() == State::PermissionDenied,
              "USB-AND-001: denial is a stated state, not a silent failure");
        const int asked = b.permissionRequests;
        for (int i = 0; i < 20; ++i) t.poll(100 + i);
        check(b.permissionRequests == asked,
              "USB-AND-001: twenty further polls ask ZERO more times - a 1 Hz "
              "poll must not become an infinite permission dialog",
              QString::number(b.permissionRequests - asked));
        // Re-attaching is a new user action, so asking again is reasonable.
        b.permission = true;
        t.onDeviceAttached(500);
        check(t.state() == State::Ready,
              "USB-AND-001: re-attaching the cable clears the denial and "
              "retries");
    }

    // ── READY means READY: open failure is not Ready ──────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        b.openSucceeds = false; b.error = QStringLiteral("claim failed");
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(t.state() == State::Error && !t.isReady(),
              "USB-AND-001: a device that is present and permitted but will "
              "not open is NOT Ready");
        check(t.lastError().contains(QStringLiteral("claim failed")),
              "USB-AND-001: and the reason is carried, not discarded",
              t.lastError());
    }

    // ── §11 hot attach ────────────────────────────────────────────────────
    {
        ScriptedBridge b;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(t.state() == State::NoDevice, "USB-AND-001: nothing attached");
        b.devices << kCh340; b.permission = true;
        t.onDeviceAttached(1000);
        check(t.state() == State::Ready,
              "USB-AND-001: plugging in while running reaches Ready with no "
              "app restart");
    }

    // ── §12 hot detach ────────────────────────────────────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        const int closesBefore = b.closeCalls;
        t.onDeviceDetached(2000);
        check(t.state() == State::Disconnected && !t.isReady(),
              "USB-AND-001: detach drops Ready immediately");
        check(b.closeCalls > closesBefore,
              "USB-AND-001: and closes the transport");
        check(t.writeBytes(QByteArray("x")) < 0,
              "USB-AND-001: a write after detach FAILS rather than pretending "
              "to succeed");
        QByteArray out;
        check(t.readBytes(&out, 16, 0) < 0,
              "USB-AND-001: and so does a read");
        check(t.disconnectedAtMs() == 2000,
              "USB-AND-001: the disconnect time is recorded for support");
    }

    // ── a vanished device with no detach callback is still a detach ───────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        b.devices.clear();               // yanked; no callback arrives
        t.poll(50);
        check(t.state() == State::Disconnected,
              "USB-AND-001: a device that simply stops enumerating is treated "
              "as disconnected, not left showing Ready");
    }

    // ── §13 reconnect ─────────────────────────────────────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(t.reconnectCount() == 0,
              "USB-AND-001: the FIRST connection is not a reconnect");
        t.onDeviceDetached(1000);
        b.devices << kCh340;
        t.onDeviceAttached(2000);
        check(t.state() == State::Ready, "USB-AND-001: reconnect reaches Ready");
        check(t.reconnectCount() == 1,
              "USB-AND-001: and counts as exactly one reconnect",
              QString::number(t.reconnectCount()));
        check(t.connectedAtMs() == 2000,
              "USB-AND-001: with the reconnect time recorded");
    }

    // ── §17 read failure must NEVER become data ───────────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true; b.readResult = -1;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        QByteArray out("PREVIOUS");
        const int n = t.readBytes(&out, 32, 10);
        check(n < 0,
              "USB-AND-001: a failed read is reported as a FAILURE, never as "
              "zero bytes", QString::number(n));
        check(out.isEmpty(),
              "USB-AND-001: and the buffer is cleared - a stale buffer reaching "
              "Modbus becomes a plausible coordinate, and a plausible "
              "coordinate becomes a score",
              QString::fromLatin1(out));
    }

    // ── a zero-byte read is NOT an error ──────────────────────────────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true; b.readResult = 0;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        QByteArray out;
        check(t.readBytes(&out, 32, 0) == 0,
              "USB-AND-001: 'nothing available yet' is 0, distinct from -1");
    }

    // ── no bridge at all ──────────────────────────────────────────────────
    {
        AndroidUsbTransport t;
        t.poll(0);
        check(t.state() == State::Error,
              "USB-AND-001: with no bridge the transport reports Error rather "
              "than sitting silently in NoDevice");
        check(t.writeBytes(QByteArray("x")) < 0 && !t.isReady(),
              "USB-AND-001: and refuses I/O");
    }

    // ── the state machine names every state it can be in ──────────────────
    {
        ScriptedBridge b;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        check(!t.stateName().isEmpty() && t.stateName() != QStringLiteral("Unknown"),
              "USB-AND-001: every state has an operator-facing name",
              t.stateName());
    }
}
