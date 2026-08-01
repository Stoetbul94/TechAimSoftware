#ifndef TA_TARGET_SERIALDEVICEPROVIDER_H
#define TA_TARGET_SERIALDEVICEPROVIDER_H

// Tech Aim 0.9.0-RC2 — serial device enumeration, behind an interface.
//
// The selection logic must be testable without hardware, so enumeration is a
// dependency rather than a direct QSerialPortInfo call. Production uses
// QSerialPortInfo; tests use a deterministic list.
//
// This header is QtCore + QtSerialPort only. No QML, no GUI, no Modbus: it
// decides WHICH device to talk to, never how to talk to it.

#include <QString>
#include <QVector>

namespace ta {
namespace target {

// One enumerated serial device. Windows does not always populate every field —
// Bluetooth links often have no VID/PID, and some USB bridges have no serial
// number — so every optional field carries its own `has` flag rather than
// relying on a sentinel value.
struct SerialDeviceInfo {
    QString portName;          // "COM4"
    QString systemLocation;    // "\\\\.\\COM4"
    QString manufacturer;      // "wch.cn"
    QString description;       // "USB-SERIAL CH340"
    QString serialNumber;      // often empty on CH340 clones

    bool    hasVendorId = false;
    quint16 vendorId = 0;
    bool    hasProductId = false;
    quint16 productId = 0;

    // QSerialPortInfo::isBusy() is not reliable on every Windows driver, so it
    // is advisory: a busy port is ranked lower, never silently discarded.
    bool    busyKnown = false;
    bool    busy = false;

    bool isValid() const { return !portName.isEmpty(); }
};

// Enumeration source. Kept abstract purely so the ranking can be tested.
class ISerialDeviceProvider
{
public:
    virtual ~ISerialDeviceProvider() = default;
    virtual QVector<SerialDeviceInfo> availableDevices() const = 0;
};

// Production: QSerialPortInfo::availablePorts(). Defined in the .cpp so this
// header stays usable in a QtCore-only test binary that supplies its own
// provider.
class QtSerialDeviceProvider : public ISerialDeviceProvider
{
public:
    QVector<SerialDeviceInfo> availableDevices() const override;
};

// Test double: returns exactly what it was given, in the given order.
class FixedSerialDeviceProvider : public ISerialDeviceProvider
{
public:
    explicit FixedSerialDeviceProvider(QVector<SerialDeviceInfo> devices = {})
        : m_devices(std::move(devices)) {}
    void setDevices(QVector<SerialDeviceInfo> devices) { m_devices = std::move(devices); }
    QVector<SerialDeviceInfo> availableDevices() const override { return m_devices; }
private:
    QVector<SerialDeviceInfo> m_devices;
};

}} // namespace ta::target

#endif // TA_TARGET_SERIALDEVICEPROVIDER_H
