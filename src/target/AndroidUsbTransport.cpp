#include "target/AndroidUsbTransport.h"

namespace ta {

// From the vendored usb-serial-for-android Ch34xSerialDriver, read out of the
// driver's own getSupportedDevices() rather than recalled: VID 0x1A86 (WCH /
// QinHeng), PIDs 0x7523 (CH340) and 0x5523 (CH341).
//
// ONE list, because §8 asks for one. A field device reporting a pair that is
// not here is a one-line addition, and the physical gate records what the real
// target actually reports.
namespace {
constexpr int kCh340VendorId  = 0x1A86;
constexpr int kCh340ProductId = 0x7523;
constexpr int kCh341ProductId = 0x5523;
}

bool isSupportedCh340(const UsbDeviceId& id)
{
    // Deliberately a whitelist. Accepting anything that merely exposes bulk
    // endpoints would let a phone, a printer or a debug adapter be adopted as
    // a scoring target.
    return id.vendorId == kCh340VendorId
        && (id.productId == kCh340ProductId || id.productId == kCh341ProductId);
}

QString describeUsbId(const UsbDeviceId& id)
{
    return QStringLiteral("VID 0x%1 PID 0x%2")
        .arg(id.vendorId,  4, 16, QLatin1Char('0'))
        .arg(id.productId, 4, 16, QLatin1Char('0'))
        .toUpper();
}

AndroidUsbTransport::AndroidUsbTransport(QObject* parent)
    : QObject(parent)
{
}

void AndroidUsbTransport::setBridge(IUsbDeviceBridge* bridge)
{
    m_bridge = bridge;
}

QString AndroidUsbTransport::stateName() const
{
    switch (m_state) {
    case State::NoDevice:            return QStringLiteral("NoDevice");
    case State::DeviceFound:         return QStringLiteral("DeviceFound");
    case State::PermissionRequired:  return QStringLiteral("PermissionRequired");
    case State::PermissionRequested: return QStringLiteral("PermissionRequested");
    case State::PermissionDenied:    return QStringLiteral("PermissionDenied");
    case State::Opening:             return QStringLiteral("Opening");
    case State::Ready:               return QStringLiteral("Ready");
    case State::Disconnected:        return QStringLiteral("Disconnected");
    case State::Reconnecting:        return QStringLiteral("Reconnecting");
    case State::Error:               return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

void AndroidUsbTransport::setState(State s, const QString& reason, qint64 nowMs)
{
    if (!reason.isEmpty())
        m_lastError = reason;
    if (m_state == s)
        return;

    const bool wasReady = (m_state == State::Ready);
    m_state = s;

    if (s == State::Ready) {
        m_connectedAtMs = nowMs;
        // Only a connection that FOLLOWS a previous one is a reconnect. The
        // first connect of a session is not, or every session would report one.
        if (m_hasConnectedOnce)
            ++m_reconnectCount;
        m_hasConnectedOnce = true;
    }
    if (wasReady && s != State::Ready)
        m_disconnectedAtMs = nowMs;

    emit stateChanged();
    if (wasReady != (s == State::Ready))
        emit readyChanged();
}

void AndroidUsbTransport::poll(qint64 nowMs)
{
    if (!m_bridge) {
        setState(State::Error, QStringLiteral("no USB bridge available"), nowMs);
        return;
    }
    // A dialog is open and the user has not answered. Polling again here would
    // post a second request and start the dialog loop §9 forbids.
    if (m_state == State::PermissionRequested)
        return;
    // Denied is a decision, not a transient. It is cleared by the device being
    // re-attached or by an explicit retry, never by another poll - otherwise a
    // 1 Hz poll becomes an infinite prompt.
    if (m_state == State::PermissionDenied)
        return;

    const QList<UsbDeviceId> devices = m_bridge->enumerate();
    UsbDeviceId found;
    bool haveCandidate = false;
    for (const UsbDeviceId& d : devices) {
        if (isSupportedCh340(d)) { found = d; haveCandidate = true; break; }
    }

    if (!haveCandidate) {
        if (m_state == State::Ready) {
            // The device vanished without a detach callback - a yanked cable
            // often arrives this way. Treat it exactly like a detach.
            closeTransport();
            setState(State::Disconnected,
                     QStringLiteral("target no longer enumerated"), nowMs);
        } else if (m_state != State::Disconnected) {
            setState(State::NoDevice, QString(), nowMs);
        }
        return;
    }

    if (m_state == State::Ready)
        return;                       // already up on this device

    m_device = found;

    if (!m_bridge->hasPermission(found)) {
        setState(State::PermissionRequired, QString(), nowMs);
        if (m_bridge->requestPermission(found)) {
            setState(State::PermissionRequested, QString(), nowMs);
        } else {
            setState(State::Error,
                     QStringLiteral("could not ask for USB permission"), nowMs);
        }
        return;
    }

    setState(State::DeviceFound, QString(), nowMs);
    tryOpen(nowMs);
}

void AndroidUsbTransport::tryOpen(qint64 nowMs)
{
    setState(State::Opening, QString(), nowMs);
    // open() applies the line coding as well as opening the port. A port that
    // opened but could not be configured at 19200/Even/8/1 is NOT usable: it
    // would return bytes, and they would be wrong. READY is set only on the
    // combined success.
    if (m_bridge->open(m_device, m_config)) {
        setState(State::Ready, QString(), nowMs);
    } else {
        const QString why = m_bridge->lastError();
        setState(State::Error,
                 why.isEmpty() ? QStringLiteral("could not open the target port")
                               : why,
                 nowMs);
    }
}

void AndroidUsbTransport::onPermissionResult(bool granted, qint64 nowMs)
{
    if (!granted) {
        // Stated, and terminal until something changes. The UI offers a retry;
        // this class does not silently re-ask.
        setState(State::PermissionDenied,
                 QStringLiteral("USB permission was denied"), nowMs);
        return;
    }
    if (!m_bridge) {
        setState(State::Error, QStringLiteral("no USB bridge available"), nowMs);
        return;
    }
    setState(State::DeviceFound, QString(), nowMs);
    tryOpen(nowMs);
}

void AndroidUsbTransport::onDeviceAttached(qint64 nowMs)
{
    // A physical re-attach clears a previous denial: the user has done
    // something new, so asking again is reasonable rather than nagging.
    if (m_state == State::PermissionDenied || m_state == State::Error)
        setState(State::NoDevice, QString(), nowMs);
    if (m_state == State::Disconnected)
        setState(State::Reconnecting, QString(), nowMs);
    poll(nowMs);
}

void AndroidUsbTransport::onDeviceDetached(qint64 nowMs)
{
    closeTransport();
    setState(State::Disconnected, QStringLiteral("target disconnected"), nowMs);
    // NOTHING else happens here. No feed command, no synthetic shot, no
    // competition change. Detach is a transport event; the session above is
    // untouched and the shared core decides what a reconnect means.
}

void AndroidUsbTransport::closeTransport()
{
    if (m_bridge)
        m_bridge->close();
}

int AndroidUsbTransport::writeBytes(const QByteArray& data)
{
    if (!m_bridge || m_state != State::Ready)
        return -1;                     // not open: a failure, never a silent 0
    return m_bridge->write(data);
}

int AndroidUsbTransport::readBytes(QByteArray* out, int maxBytes, int timeoutMs)
{
    if (!m_bridge || m_state != State::Ready)
        return -1;
    const int n = m_bridge->read(out, maxBytes, timeoutMs);
    // ACQ-READ-004 / ACQ-SENTINEL-003, at the transport edge.
    //
    // A failed read stays a failure. It is NOT turned into 0 ("nothing to
    // read"), and the caller's buffer is NOT left holding whatever it held
    // before, because a stale buffer that reaches the Modbus layer becomes a
    // plausible coordinate and a plausible coordinate becomes a score. The
    // whole acquisition-hardening programme exists because of that chain.
    if (n < 0) {
        if (out)
            out->clear();
        return -1;
    }
    return n;
}

} // namespace ta
