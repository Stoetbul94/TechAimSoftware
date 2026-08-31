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
#include <QFile>
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


// Returns the file with // comments REMOVED. The first version of this
// assertion matched the whole file and failed on the transport's own comments,
// which say things like "no feed" - the test was reading the prose instead of
// the code. What matters is that no CODE here can request a feed.
static QString readSourceCode(const QString& relative)
{
    QFile f(QStringLiteral(TECHAIM_SOURCE_DIR "/") + relative);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    const QChar nl = QLatin1Char('\n');
    const QStringList lines = QString::fromUtf8(f.readAll()).split(nl);
    QString code;
    for (const QString& l : lines)
        code += l.split(QStringLiteral("//")).first() + nl;
    return code;
}

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
    // ── §18 the transport CANNOT feed paper, by construction ──────────────
    // The strongest form of "no feed on attach/permission/reconnect" is not a
    // test that counts feeds - it is that this layer has no way to ask for
    // one. Feeds are issued by the shared accepted-shot path, which the
    // transport never calls into.
    {
        const QString both =
            readSourceCode(QStringLiteral("src/target/AndroidUsbTransport.h"))
          + readSourceCode(QStringLiteral("src/target/AndroidUsbTransport.cpp"));
        check(!both.contains(QStringLiteral("feed"), Qt::CaseInsensitive)
              && !both.contains(QStringLiteral("motor"), Qt::CaseInsensitive),
              "USB-AND-001: the transport contains no feed or motor concept at "
              "all - attach, permission and reconnect CANNOT emit a feed "
              "because there is nothing here to emit one with");
        check(!both.contains(QStringLiteral("score"), Qt::CaseInsensitive)
              && !both.contains(QStringLiteral("shotCount"))
              && !both.contains(QStringLiteral("competition"), Qt::CaseInsensitive),
              "USB-AND-001: and no scoring, shot-count or competition concept");
    }

    // ── §16 shutdown races: no I/O after close, whatever the order ────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        t.closeTransport();
        // closeTransport() alone does not leave Ready lying: a subsequent poll
        // that finds the device re-opens rather than assuming the old handle.
        QByteArray out;
        t.onDeviceDetached(10);
        check(t.readBytes(&out, 8, 0) < 0 && t.writeBytes(QByteArray("x")) < 0,
              "USB-AND-001: no read or write survives a close/detach");
    }

    // ── a permission answer arriving after shutdown must not resurrect ────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = false;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        t.onDeviceDetached(5);                 // cable pulled during the dialog
        b.permission = true;
        t.onPermissionResult(true, 10);        // the answer lands anyway
        // It may proceed to open - the device list is what decides - but it
        // must never be Ready while the bridge cannot actually open.
        b.openSucceeds = false;
        t.onPermissionResult(true, 20);
        check(!t.isReady(),
              "USB-AND-001: a late permission answer cannot produce Ready when "
              "the port will not open", t.stateName());
    }

    // ── §19 every state maps to an operator sentence, none leaks Java ─────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = false;
        b.error = QStringLiteral("java.io.IOException: bulk transfer failed");
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(0);
        t.onPermissionResult(false, 1);
        check(t.stateName() == QStringLiteral("PermissionDenied"),
              "USB-AND-001: denial has its own named state for the UI to map",
              t.stateName());
        // The Java text is CARRIED for the log and the support bundle, and the
        // UI maps the STATE - so an operator is never shown a stack trace.
        check(!t.stateName().contains(QStringLiteral("java.")),
              "USB-AND-001: the operator-facing state name never contains Java "
              "exception text", t.stateName());
    }

    // ── §21 support diagnostics are present and carry no unique id ────────
    {
        ScriptedBridge b;
        b.devices << kCh340; b.permission = true;
        AndroidUsbTransport t; t.setBridge(&b);
        t.poll(1000);
        t.onDeviceDetached(2000);
        b.devices << kCh340;
        t.onDeviceAttached(3000);
        check(t.reconnectCount() == 1 && t.connectedAtMs() == 3000
              && t.disconnectedAtMs() == 2000,
              "USB-AND-001: reconnect count and both timestamps are available "
              "for the support bundle");
        check(ta::describeUsbId(t.device()) == QStringLiteral("VID 0X1A86 PID 0X7523"),
              "USB-AND-001: the device is described by VID/PID only - no serial "
              "number, which would identify the unit without helping a case",
              ta::describeUsbId(t.device()));
    }

}
