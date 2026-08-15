#ifndef TA_TARGET_TARGETDEVICEFINGERPRINT_H
#define TA_TARGET_TARGETDEVICEFINGERPRINT_H

// Tech Aim 0.9.0-RC2 — stable identity for the physical target adapter, and
// the rules that decide which enumerated device is the target.
//
// WHY THIS EXISTS. RC1 remembered only "COM7". Windows reassigns COM numbers
// when an adapter moves to another USB socket, so the remembered value stopped
// matching the same physical adapter. Worse, the RC1 fallback opened ports in
// enumeration order and kept the first that OPENED — and a Bluetooth serial
// port opens successfully with nothing behind it, so the application could
// attach to COM5 and then wait forever for a Modbus reply that never comes.
//
// The fix is to identify the ADAPTER, not the port number, and to reject
// devices that cannot be a target before any port is opened.
//
// Pure QtCore. No Modbus, no QML, no GUI — this decides WHICH device, never
// how to talk to it.

#include <QString>
#include <QVariantMap>
#include <QVector>

#include "SerialDeviceProvider.h"

namespace ta {
namespace target {

// How strongly a device is identified. Higher is better; the ordering is the
// identity preference the brief specifies.
enum class FingerprintStrength : int {
    None = 0,
    PortNameOnly = 1,      // weakest: survives nothing but a stable machine
    DescriptionOnly = 2,   // confirmed description/manufacturer
    VidPidDescription = 3,
    VidPidManufacturerDescription = 4,
    SerialVidPid = 5,      // strongest: unique to the physical adapter
};

QString fingerprintStrengthName(FingerprintStrength s);

// The stored identity of a confirmed target adapter.
struct TargetDeviceFingerprint {
    bool    valid = false;
    FingerprintStrength strength = FingerprintStrength::None;

    bool    hasVendorId = false;
    quint16 vendorId = 0;
    bool    hasProductId = false;
    quint16 productId = 0;
    QString serialNumber;
    QString manufacturer;
    QString description;

    // Informational ONLY. Recorded so the operator can be told where the
    // target used to be; never used to decide identity unless it is the only
    // thing available (PortNameOnly).
    QString lastKnownPortName;

    static TargetDeviceFingerprint fromDevice(const SerialDeviceInfo& d);

    // Does this fingerprint describe that device? Compares on the strongest
    // shared evidence; a COM-number change alone never breaks a match.
    bool matches(const SerialDeviceInfo& d) const;

    // Round-trip for QSettings. Deliberately a flat string map so the stored
    // form is readable in the .ini and survives a settings-format change.
    QVariantMap toMap() const;
    static TargetDeviceFingerprint fromMap(const QVariantMap& m);
};

// Why a device was not offered as a target candidate. Logged, so a field
// problem can be diagnosed from the log alone.
enum class RejectReason : int {
    NotRejected = 0,
    EmptyPortName,
    BluetoothLink,
    VirtualOrModem,
    NoUsbIdentity,
};

QString rejectReasonName(RejectReason r);

struct CandidateDevice {
    SerialDeviceInfo device;
    int    score = 0;              // higher is a better target candidate
    bool   matchesRemembered = false;
    RejectReason reject = RejectReason::NotRejected;
    QString reason;                // human-readable, for the log and the dialog
};

// What the caller should do next.
enum class SelectionOutcome : int {
    NoCandidates = 0,      // show "Target not detected", keep Rescan + manual
    AutoConnect,           // exactly one confident candidate, or a remembered match
    NeedsUserChoice,       // several plausible adapters — ask, do not guess
};

struct SelectionResult {
    SelectionOutcome outcome = SelectionOutcome::NoCandidates;
    SerialDeviceInfo selected;         // meaningful when outcome == AutoConnect
    QVector<CandidateDevice> candidates;   // accepted, best first
    QVector<CandidateDevice> rejected;     // with reasons, for the log
    QString summary;                   // one line for the operator log
};

// ── the rules ──────────────────────────────────────────────────────────────

// A Bluetooth serial link is never a Tech Aim target. Matched case-insensitively
// on description AND manufacturer, because Windows puts the wording in either.
bool looksLikeBluetooth(const SerialDeviceInfo& d);

// Modems and known virtual ports are likewise never targets.
bool looksLikeVirtualOrModem(const SerialDeviceInfo& d);

// A USB-serial bridge of the kind the Tech Aim target uses. CH340 is what the
// field unit presents today; the others are the common bridges an OEM might
// substitute. This RAISES confidence — it is not a whitelist, because an
// unrecognised USB bridge is still a plausible candidate.
bool looksLikeKnownUsbBridge(const SerialDeviceInfo& d);

class TargetDeviceSelector
{
public:
    // remembered may be invalid (first run). Ranking is deterministic: the
    // same device list always yields the same answer.
    static SelectionResult select(const QVector<SerialDeviceInfo>& devices,
                                  const TargetDeviceFingerprint& remembered);

    static SelectionResult select(const ISerialDeviceProvider& provider,
                                  const TargetDeviceFingerprint& remembered)
    { return select(provider.availableDevices(), remembered); }
};

}} // namespace ta::target

#endif // TA_TARGET_TARGETDEVICEFINGERPRINT_H
