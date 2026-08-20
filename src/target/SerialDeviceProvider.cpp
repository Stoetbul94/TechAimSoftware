#include "SerialDeviceProvider.h"

#include "platform/PlatformService.h"

#include <QDebug>

#if !defined(Q_OS_ANDROID)
#  include <QSerialPortInfo>
#endif

namespace ta {
namespace target {

QVector<SerialDeviceInfo> QtSerialDeviceProvider::availableDevices() const
{
    QVector<SerialDeviceInfo> out;

#if defined(Q_OS_ANDROID)
    // A1/A2 — Android returns NOTHING, deliberately and explicitly.
    //
    // This is not a stub waiting to be filled with the desktop code. Android
    // does not expose USB serial adapters as /dev/tty* device nodes to an
    // unprivileged app; USB access goes through the Java UsbManager API with a
    // per-device runtime permission grant. QSerialPortInfo::availablePorts()
    // would therefore return an empty or useless list ANYWAY — the point of
    // writing it out here is that the emptiness becomes a stated fact with a
    // reason attached, instead of a silent accident of the Qt backend that a
    // future reader might mistake for a bug and "fix".
    //
    // libQt6SerialPort_arm64-v8a.so ships in the Qt Android kit and this code
    // links against it. That does not mean a USB target works. It does not.
    //
    // Downstream behaviour is intentionally unchanged: TargetDeviceFingerprint
    // ranks an empty candidate list, concludes there is no confident device,
    // and the caller declines to connect speculatively — which is exactly the
    // behaviour the Windows selector was already fixed to have when no target
    // is present. No selection logic is bypassed or duplicated.
    //
    // See docs/architecture/android-target-transport-options.md.
    static bool announced = false;
    if (!announced) {
        announced = true;               // once per process, not once per scan
        qInfo().noquote()
            << "Serial enumeration: no USB serial transport on"
            << ta::platform::shellName(ta::platform::currentShell())
            << "— USB target acquisition is NOT IMPLEMENTED. "
               "Use Demo mode, or Modbus TCP where a networked target exists.";
    }
    return out;                          // empty
#else
    const auto ports = QSerialPortInfo::availablePorts();
    out.reserve(ports.size());
    for (const QSerialPortInfo& p : ports) {
        SerialDeviceInfo d;
        d.portName       = p.portName();
        d.systemLocation = p.systemLocation();
        d.manufacturer   = p.manufacturer();
        d.description    = p.description();
        d.serialNumber   = p.serialNumber();
        // hasVendorIdentifier() is the only honest test: a Bluetooth link
        // reports 0/0, which is indistinguishable from a real VID of zero.
        d.hasVendorId    = p.hasVendorIdentifier();
        if (d.hasVendorId)  d.vendorId  = p.vendorIdentifier();
        d.hasProductId   = p.hasProductIdentifier();
        if (d.hasProductId) d.productId = p.productIdentifier();
        // Qt 6 removed QSerialPortInfo::isBusy(), and it was never reliable on
        // Windows anyway - several drivers always reported false. Busy state is
        // therefore left UNKNOWN rather than guessed; the selector treats it as
        // advisory and never disqualifies a port on it.
        d.busyKnown = false;
        d.busy = false;
        out.append(d);
    }
    return out;
#endif
}

}} // namespace ta::target
