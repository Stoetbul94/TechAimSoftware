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
        // Advisory: some Windows drivers always report false.
        d.busyKnown = true;
        d.busy = p.isBusy();
        out.append(d);
    }
    return out;
}

}} // namespace ta::target
