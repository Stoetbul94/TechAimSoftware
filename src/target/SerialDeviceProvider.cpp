#include "SerialDeviceProvider.h"

#include <QSerialPortInfo>

namespace ta {
namespace target {

QVector<SerialDeviceInfo> QtSerialDeviceProvider::availableDevices() const
{
    QVector<SerialDeviceInfo> out;
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
}

}} // namespace ta::target
