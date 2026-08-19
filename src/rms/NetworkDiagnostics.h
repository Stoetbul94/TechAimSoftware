#ifndef TA_RMS_NETWORKDIAGNOSTICS_H
#define TA_RMS_NETWORKDIAGNOSTICS_H

// ─────────────────────────────────────────────────────────────────────────────
// NETWORK DIAGNOSTICS — why RMS can or cannot hear the range.
//
// The failure this exists to prevent: RMS shows an empty range, the operator
// concludes the tablets are broken, and the actual cause is a firewall prompt
// nobody answered. A listener that could not bind must SAY SO, loudly, instead
// of showing zero stations and letting the range draw its own conclusions.
//
// Everything here is observation of the LOCAL machine. It opens no sockets of
// its own, contacts nothing, and changes no system setting — in particular it
// does not touch Windows Firewall, which is a decision for a person.
// ─────────────────────────────────────────────────────────────────────────────

#include <QObject>
#include <QString>
#include <QVariantList>

namespace ta {
namespace rms {

class NetworkDiagnostics : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool listening READ listening NOTIFY changed)
    Q_PROPERTY(int port READ port NOTIFY changed)
    Q_PROPERTY(QString listenerError READ listenerError NOTIFY changed)
    Q_PROPERTY(QString mode READ mode NOTIFY changed)
    Q_PROPERTY(bool live READ live NOTIFY changed)
    Q_PROPERTY(QString hostName READ hostName CONSTANT)

public:
    explicit NetworkDiagnostics(QObject* parent = nullptr);

    // Told by the application what actually happened when it tried to listen.
    // Not inferred — a guess about the socket is worse than no answer.
    void setListenerState(bool listening, int port, const QString& error);
    void setMode(const QString& mode, bool isLive);

    bool listening() const { return m_listening; }
    int port() const { return m_port; }
    QString listenerError() const { return m_error; }
    QString mode() const { return m_mode; }
    bool live() const { return m_live; }
    QString hostName() const;

    // Plausible LAN interfaces: IPv4, up, running, not loopback. Loopback and
    // link-local noise are filtered because an operator hunting a range fault
    // does not need to read twelve virtual adapters.
    // [{ name, address, netmask, hardware, isWireless }]
    Q_INVOKABLE QVariantList interfaces() const;
    // Every interface, including the ones normally filtered out.
    Q_INVOKABLE QVariantList allInterfaces() const;

    // A one-line answer for the preflight: "no usable network interface" is a
    // different problem from "no stations yet".
    Q_INVOKABLE bool hasUsableInterface() const;

signals:
    void changed();

private:
    QVariantList collect(bool onlyPlausible) const;

    bool m_listening = false;
    int m_port = 0;
    QString m_error;
    QString m_mode = QStringLiteral("LIVE");
    bool m_live = true;
};

}  // namespace rms
}  // namespace ta

#endif
