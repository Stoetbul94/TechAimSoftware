#ifndef TECHAIM_ANDROIDUSBTRANSPORT_H
#define TECHAIM_ANDROIDUSBTRANSPORT_H

// Android USB target transport — the state machine, and only the state machine.
//
// WHAT THIS IS. The C++ half of the Android CH340 transport. It owns the
// connection lifecycle and nothing else. Java owns the USB device and the
// CH340 line coding; libmodbus owns Modbus framing and CRC; the existing
// acquisition authority owns every question about whether a coordinate is real.
// This class answers exactly five questions:
//
//     is a device present, is permission granted, is the port open,
//     did these bytes move, has the connection dropped
//
// It must never grow an opinion about shots, scores, counters or competition
// state. If a future change wants to put one here, that is the signal that it
// belongs in the shared core instead.
//
// WHY THE BRIDGE IS AN INTERFACE. No Android device is attached to the machine
// this was written on, and the physical gate is a separate exercise. An
// injectable IUsbDeviceBridge is what makes attach, detach, permission denial,
// read failure and reconnect testable at all - none of which can be provoked
// on demand with real hardware, and several of which are exactly the cases
// that matter. §28 requires these tests to run without USB hardware.
//
// READY MEANS READY. Android will hand out a UsbDevice object for a plugged-in
// CH340 long before anything can be read from it. READY here means the port is
// open AND the line coding has been applied, because a status that turns green
// on device presence teaches an operator to distrust the status.

#include <QObject>
#include <QString>
#include <QByteArray>

namespace ta {

// The field-authoritative line settings. One definition, so a second copy
// cannot drift: 19200/Even/8/1 with RTS deasserted, from the Windows physical
// qualification. DTR is deliberately absent - see the transport implementation
// record; the desktop application never sets it and this must not invent it.
struct SerialLineConfig {
    int  baud     = 19200;
    char parity   = 'E';     // 'E'ven — NOT the legacy 9600/None default
    int  dataBits = 8;
    int  stopBits = 1;
    bool rts      = false;   // "Disable" in the field log
};

// The CH340 identity, taken from the vendored driver rather than from memory.
// A device is a candidate ONLY if it matches one of these; exposing bulk
// endpoints is not sufficient to be treated as a Tech Aim target.
struct UsbDeviceId {
    int vendorId  = 0;
    int productId = 0;
    bool operator==(const UsbDeviceId& o) const
    { return vendorId == o.vendorId && productId == o.productId; }
};

bool isSupportedCh340(const UsbDeviceId& id);
QString describeUsbId(const UsbDeviceId& id);

// The JNI boundary, expressed as an interface. The production implementation
// calls into Java; the test implementation is a script of outcomes.
class IUsbDeviceBridge {
public:
    virtual ~IUsbDeviceBridge() = default;

    // Devices Android currently reports. May be empty.
    virtual QList<UsbDeviceId> enumerate() = 0;

    // Has the user already granted permission for this device?
    virtual bool hasPermission(const UsbDeviceId& id) = 0;

    // Ask Android to show the permission dialog. Returns false if the request
    // could not even be posted. The ANSWER arrives later, via
    // onPermissionResult() - this is not a blocking call, and treating it as
    // one is how permission flows deadlock.
    virtual bool requestPermission(const UsbDeviceId& id) = 0;

    // Open and apply the line coding. Both, or neither: a port that opened but
    // could not be configured is not usable and must not report READY.
    virtual bool open(const UsbDeviceId& id, const SerialLineConfig& cfg) = 0;
    virtual void close() = 0;

    // Byte movement. Negative means FAILURE, and failure must stay failure -
    // see the read-error note in the .cpp. Zero means "nothing available",
    // which is not an error.
    virtual int  write(const QByteArray& data) = 0;
    virtual int  read(QByteArray* out, int maxBytes, int timeoutMs) = 0;

    virtual QString lastError() const = 0;
};

class AndroidUsbTransport : public QObject
{
    Q_OBJECT
public:
    // §7's states, and no others. Each one is a state an operator message can
    // be written for; "Error" carries a reason rather than being a bucket.
    enum class State {
        NoDevice,
        DeviceFound,
        PermissionRequired,
        PermissionRequested,
        PermissionDenied,
        Opening,
        Ready,
        Disconnected,
        Reconnecting,
        Error
    };
    Q_ENUM(State)

    explicit AndroidUsbTransport(QObject* parent = nullptr);

    void setBridge(IUsbDeviceBridge* bridge);   // not owned

    State   state() const { return m_state; }
    bool    isReady() const { return m_state == State::Ready; }
    QString stateName() const;
    QString lastError() const { return m_lastError; }
    UsbDeviceId device() const { return m_device; }
    SerialLineConfig lineConfig() const { return m_config; }

    // Diagnostics for the support bundle (§22). Deliberately no serial number
    // or other unique device identifier: none of it helps a support case.
    int  reconnectCount() const { return m_reconnectCount; }
    qint64 connectedAtMs() const { return m_connectedAtMs; }
    qint64 disconnectedAtMs() const { return m_disconnectedAtMs; }

    // ── inputs ────────────────────────────────────────────────────────────
    // Scan for a device and advance as far as it can without user input. Safe
    // to call at startup with the target ALREADY attached, which is the case
    // §10 insists must work without unplugging anything.
    void poll(qint64 nowMs = 0);

    // Android told us the user answered the permission dialog.
    void onPermissionResult(bool granted, qint64 nowMs = 0);

    // Android told us a device arrived or left.
    void onDeviceAttached(qint64 nowMs = 0);
    void onDeviceDetached(qint64 nowMs = 0);

    void closeTransport();

    // ── byte movement, for the libmodbus backend ──────────────────────────
    // Both refuse unless READY, so a half-open transport cannot silently
    // return plausible-looking nothing.
    int  writeBytes(const QByteArray& data);
    int  readBytes(QByteArray* out, int maxBytes, int timeoutMs);

signals:
    void stateChanged();
    void readyChanged();

private:
    void setState(State s, const QString& reason = QString(), qint64 nowMs = 0);
    void tryOpen(qint64 nowMs);

    IUsbDeviceBridge* m_bridge = nullptr;
    State             m_state  = State::NoDevice;
    UsbDeviceId       m_device;
    SerialLineConfig  m_config;
    QString           m_lastError;
    int     m_reconnectCount   = 0;
    bool    m_hasConnectedOnce = false;
    qint64  m_connectedAtMs    = 0;
    qint64  m_disconnectedAtMs = 0;
};

} // namespace ta

#endif // TECHAIM_ANDROIDUSBTRANSPORT_H
