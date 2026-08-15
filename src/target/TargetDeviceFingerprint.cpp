#include "TargetDeviceFingerprint.h"

#include <QVariantMap>
#include <algorithm>

namespace ta {
namespace target {

QString fingerprintStrengthName(FingerprintStrength s)
{
    switch (s) {
    case FingerprintStrength::None:                          return QStringLiteral("none");
    case FingerprintStrength::PortNameOnly:                  return QStringLiteral("port-name-only");
    case FingerprintStrength::DescriptionOnly:               return QStringLiteral("description");
    case FingerprintStrength::VidPidDescription:             return QStringLiteral("vid+pid+description");
    case FingerprintStrength::VidPidManufacturerDescription: return QStringLiteral("vid+pid+manufacturer+description");
    case FingerprintStrength::SerialVidPid:                  return QStringLiteral("serial+vid+pid");
    }
    return QStringLiteral("none");
}

QString rejectReasonName(RejectReason r)
{
    switch (r) {
    case RejectReason::NotRejected:    return QStringLiteral("accepted");
    case RejectReason::EmptyPortName:  return QStringLiteral("empty port name");
    case RejectReason::BluetoothLink:  return QStringLiteral("Bluetooth serial link");
    case RejectReason::VirtualOrModem: return QStringLiteral("modem or virtual port");
    case RejectReason::NoUsbIdentity:  return QStringLiteral("no USB identity and no recognised description");
    }
    return QStringLiteral("accepted");
}

namespace {

bool containsAnyCi(const QString& haystack, std::initializer_list<const char*> needles)
{
    if (haystack.isEmpty()) return false;
    for (const char* n : needles)
        if (haystack.contains(QLatin1String(n), Qt::CaseInsensitive)) return true;
    return false;
}

} // namespace

bool looksLikeBluetooth(const SerialDeviceInfo& d)
{
    // Windows writes the Bluetooth wording into the description on some
    // machines and the manufacturer on others, so both are checked. RFCOMM is
    // the Bluetooth serial profile and appears on several stacks.
    return containsAnyCi(d.description,  { "bluetooth", "rfcomm", "bthenum" })
        || containsAnyCi(d.manufacturer, { "bluetooth", "rfcomm", "bthenum" });
}

bool looksLikeVirtualOrModem(const SerialDeviceInfo& d)
{
    return containsAnyCi(d.description,  { "modem", "virtual", "com0com", "null modem" })
        || containsAnyCi(d.manufacturer, { "com0com" });
}

bool looksLikeKnownUsbBridge(const SerialDeviceInfo& d)
{
    // CH340 is the bridge the current field target presents. The rest are the
    // common alternatives an OEM might fit. Matching one RAISES confidence;
    // failing to match does not disqualify a device that has a USB identity.
    return containsAnyCi(d.description,  { "ch340", "ch341", "cp210", "ft232", "ftdi",
                                           "pl2303", "usb-serial", "usb serial" })
        || containsAnyCi(d.manufacturer, { "wch", "silicon labs", "ftdi", "prolific" });
}

// ── fingerprint ────────────────────────────────────────────────────────────

TargetDeviceFingerprint TargetDeviceFingerprint::fromDevice(const SerialDeviceInfo& d)
{
    TargetDeviceFingerprint f;
    if (!d.isValid()) return f;

    f.valid = true;
    f.hasVendorId = d.hasVendorId;   f.vendorId = d.vendorId;
    f.hasProductId = d.hasProductId; f.productId = d.productId;
    f.serialNumber = d.serialNumber.trimmed();
    f.manufacturer = d.manufacturer.trimmed();
    f.description  = d.description.trimmed();
    f.lastKnownPortName = d.portName;

    // Strongest available identity wins — the order the brief specifies.
    const bool ids = f.hasVendorId && f.hasProductId;
    if (ids && !f.serialNumber.isEmpty())              f.strength = FingerprintStrength::SerialVidPid;
    else if (ids && !f.manufacturer.isEmpty()
                 && !f.description.isEmpty())         f.strength = FingerprintStrength::VidPidManufacturerDescription;
    else if (ids && !f.description.isEmpty())         f.strength = FingerprintStrength::VidPidDescription;
    else if (!f.description.isEmpty()
             || !f.manufacturer.isEmpty())            f.strength = FingerprintStrength::DescriptionOnly;
    else                                              f.strength = FingerprintStrength::PortNameOnly;
    return f;
}

bool TargetDeviceFingerprint::matches(const SerialDeviceInfo& d) const
{
    if (!valid || !d.isValid()) return false;

    const bool bothIds = hasVendorId && hasProductId && d.hasVendorId && d.hasProductId;
    const bool idsAgree = bothIds && vendorId == d.vendorId && productId == d.productId;

    switch (strength) {
    case FingerprintStrength::SerialVidPid:
        // The serial number is unique to the adapter, so this survives ANY
        // COM-number change and any number of identical adapters.
        return idsAgree
            && !serialNumber.isEmpty()
            && serialNumber.compare(d.serialNumber.trimmed(), Qt::CaseInsensitive) == 0;

    case FingerprintStrength::VidPidManufacturerDescription:
        return idsAgree
            && manufacturer.compare(d.manufacturer.trimmed(), Qt::CaseInsensitive) == 0
            && description.compare(d.description.trimmed(), Qt::CaseInsensitive) == 0;

    case FingerprintStrength::VidPidDescription:
        return idsAgree
            && description.compare(d.description.trimmed(), Qt::CaseInsensitive) == 0;

    case FingerprintStrength::DescriptionOnly:
        // No USB identity to lean on. Require BOTH strings to agree where both
        // were recorded, so two different no-VID adapters cannot collide.
        if (description.isEmpty() && manufacturer.isEmpty()) return false;
        if (!description.isEmpty()
            && description.compare(d.description.trimmed(), Qt::CaseInsensitive) != 0) return false;
        if (!manufacturer.isEmpty()
            && manufacturer.compare(d.manufacturer.trimmed(), Qt::CaseInsensitive) != 0) return false;
        return true;

    case FingerprintStrength::PortNameOnly:
        // The weak fallback, and the reason RC1 failed. Kept only so a machine
        // with no identifying information at all is not left unusable.
        return !lastKnownPortName.isEmpty()
            && lastKnownPortName.compare(d.portName, Qt::CaseInsensitive) == 0;

    case FingerprintStrength::None:
        return false;
    }
    return false;
}

QVariantMap TargetDeviceFingerprint::toMap() const
{
    QVariantMap m;
    m[QStringLiteral("valid")] = valid;
    m[QStringLiteral("strength")] = static_cast<int>(strength);
    m[QStringLiteral("hasVendorId")] = hasVendorId;
    m[QStringLiteral("vendorId")] = vendorId;
    m[QStringLiteral("hasProductId")] = hasProductId;
    m[QStringLiteral("productId")] = productId;
    m[QStringLiteral("serialNumber")] = serialNumber;
    m[QStringLiteral("manufacturer")] = manufacturer;
    m[QStringLiteral("description")] = description;
    m[QStringLiteral("lastKnownPortName")] = lastKnownPortName;
    return m;
}

TargetDeviceFingerprint TargetDeviceFingerprint::fromMap(const QVariantMap& m)
{
    TargetDeviceFingerprint f;
    f.valid = m.value(QStringLiteral("valid")).toBool();
    f.strength = static_cast<FingerprintStrength>(
        m.value(QStringLiteral("strength"), 0).toInt());
    f.hasVendorId = m.value(QStringLiteral("hasVendorId")).toBool();
    f.vendorId = static_cast<quint16>(m.value(QStringLiteral("vendorId")).toUInt());
    f.hasProductId = m.value(QStringLiteral("hasProductId")).toBool();
    f.productId = static_cast<quint16>(m.value(QStringLiteral("productId")).toUInt());
    f.serialNumber = m.value(QStringLiteral("serialNumber")).toString();
    f.manufacturer = m.value(QStringLiteral("manufacturer")).toString();
    f.description = m.value(QStringLiteral("description")).toString();
    f.lastKnownPortName = m.value(QStringLiteral("lastKnownPortName")).toString();
    // A stored strength outside the enum means a corrupt or newer settings
    // file: refuse it rather than matching unpredictably.
    if (static_cast<int>(f.strength) < 0 || static_cast<int>(f.strength) > 5) f.valid = false;
    return f;
}

// ── selection ──────────────────────────────────────────────────────────────

SelectionResult TargetDeviceSelector::select(const QVector<SerialDeviceInfo>& devices,
                                             const TargetDeviceFingerprint& remembered)
{
    SelectionResult r;

    for (const SerialDeviceInfo& d : devices) {
        CandidateDevice c;
        c.device = d;

        if (!d.isValid()) {
            c.reject = RejectReason::EmptyPortName;
            c.reason = rejectReasonName(c.reject);
            r.rejected.append(c);
            continue;
        }
        // REJECT BEFORE OPENING. A Bluetooth port opens happily with nothing
        // behind it, so it must never reach a connection attempt.
        if (looksLikeBluetooth(d)) {
            c.reject = RejectReason::BluetoothLink;
            c.reason = QStringLiteral("%1 (%2) rejected: %3")
                           .arg(d.portName, d.description, rejectReasonName(c.reject));
            r.rejected.append(c);
            continue;
        }
        if (looksLikeVirtualOrModem(d)) {
            c.reject = RejectReason::VirtualOrModem;
            c.reason = QStringLiteral("%1 (%2) rejected: %3")
                           .arg(d.portName, d.description, rejectReasonName(c.reject));
            r.rejected.append(c);
            continue;
        }
        // Nothing identifying at all: no USB ids and no description. Such a
        // port could be anything, so it is not offered automatically — manual
        // selection still reaches it.
        if (!d.hasVendorId && !d.hasProductId
            && d.description.trimmed().isEmpty() && d.manufacturer.trimmed().isEmpty()) {
            c.reject = RejectReason::NoUsbIdentity;
            c.reason = QStringLiteral("%1 rejected: %2")
                           .arg(d.portName, rejectReasonName(c.reject));
            r.rejected.append(c);
            continue;
        }

        // ── scoring ────────────────────────────────────────────────────────
        c.matchesRemembered = remembered.valid && remembered.matches(d);
        if (c.matchesRemembered) c.score += 1000;          // a confirmed target outranks everything
        if (looksLikeKnownUsbBridge(d)) c.score += 100;    // CH340 and friends
        if (d.hasVendorId && d.hasProductId) c.score += 50;
        if (!d.serialNumber.trimmed().isEmpty()) c.score += 10;
        if (d.busyKnown && d.busy) c.score -= 25;          // advisory only, never disqualifying
        c.reason = QStringLiteral("%1 (%2) score %3%4")
                       .arg(d.portName,
                            d.description.isEmpty() ? QStringLiteral("no description") : d.description)
                       .arg(c.score)
                       .arg(c.matchesRemembered ? QStringLiteral(" [remembered target]") : QString());
        r.candidates.append(c);
    }

    // Deterministic order: score, then port name, so an identical device list
    // always produces an identical answer.
    std::stable_sort(r.candidates.begin(), r.candidates.end(),
                     [](const CandidateDevice& a, const CandidateDevice& b) {
                         if (a.score != b.score) return a.score > b.score;
                         return a.device.portName < b.device.portName;
                     });

    if (r.candidates.isEmpty()) {
        r.outcome = SelectionOutcome::NoCandidates;
        r.summary = QStringLiteral("no target candidate among %1 enumerated port(s); %2 rejected")
                        .arg(devices.size()).arg(r.rejected.size());
        return r;
    }

    // A remembered match is decisive even when other adapters are present —
    // that is the whole point of remembering it.
    if (r.candidates.first().matchesRemembered) {
        r.outcome = SelectionOutcome::AutoConnect;
        r.selected = r.candidates.first().device;
        r.summary = QStringLiteral("remembered target found on %1 (%2)")
                        .arg(r.selected.portName, r.selected.description);
        return r;
    }

    if (r.candidates.size() == 1) {
        r.outcome = SelectionOutcome::AutoConnect;
        r.selected = r.candidates.first().device;
        r.summary = QStringLiteral("single target candidate %1 (%2)")
                        .arg(r.selected.portName, r.selected.description);
        return r;
    }

    // Several plausible adapters and nothing remembered. Guessing here is what
    // RC1 did wrong, so ask instead.
    r.outcome = SelectionOutcome::NeedsUserChoice;
    r.summary = QStringLiteral("%1 plausible target candidates - operator must confirm")
                    .arg(r.candidates.size());
    return r;
}

}} // namespace ta::target
