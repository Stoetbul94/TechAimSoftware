#ifndef TA_RMS_CONTROLSTATUSMODEL_H
#define TA_RMS_CONTROLSTATUSMODEL_H

// What the RMS window shows about the control channel. STATUS ONLY - there is
// no command button here and none may be added until the control transport is
// wired and qualified end to end.
//
// THE RULE THIS FILE EXISTS TO ENFORCE. A control panel that looks operable
// when nothing is connected is worse than no panel at all: an operator who
// believes they started a range that never heard them finds out when the
// athletes do. So when no transport is attached, this model says exactly that,
// in those words, and every lane reads NOT CONNECTED.
//
// It reads the coordinator and the monitor. It never sends anything, and it
// never derives a lane's state from another lane's.

#include "rms/RangeMonitor.h"
#include "rms/control/RangeControlCoordinator.h"

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace ta {
namespace rms {
namespace control {

class ControlStatusModel : public QAbstractListModel
{
    Q_OBJECT
    // True only when a control transport has actually been attached. It is
    // false in every build that has not wired one, which is the honest answer
    // and the one the banner shows.
    Q_PROPERTY(bool transportAttached READ transportAttached NOTIFY summaryChanged)
    Q_PROPERTY(int laneCount READ laneCount NOTIFY summaryChanged)
    Q_PROPERTY(int authenticatedCount READ authenticatedCount NOTIFY summaryChanged)
    Q_PROPERTY(int unobservedShots READ unobservedShots NOTIFY summaryChanged)
    Q_PROPERTY(int lanesBehind READ lanesBehind NOTIFY summaryChanged)
    Q_PROPERTY(int lanesRecovering READ lanesRecovering NOTIFY summaryChanged)
    // One line for the banner. Says what is true, including "not enabled".
    Q_PROPERTY(QString statusLine READ statusLine NOTIFY summaryChanged)
    Q_PROPERTY(QString tone READ tone NOTIFY summaryChanged)
    Q_PROPERTY(bool keyConfigured READ keyConfigured NOTIFY summaryChanged)

public:
    enum Roles {
        NodeIdRole = Qt::UserRole + 1,
        LaneLabelRole,
        ChannelRole,        // the control link state, by name
        SyncQualityRole,    // "GOOD" | "DEGRADED" | "UNUSABLE"
        UnobservedRole,
        ReconciledToRole,   // the persisted watermark, or -1
        RestartsRole,       // node restarts RMS has observed on this lane
        PendingRole,        // commands still awaiting an answer
        ToneRole
    };

    explicit ControlStatusModel(QObject* parent = nullptr);

    void setSources(RangeMonitor* monitor, RangeControlCoordinator* coordinator);

    // Declared by whoever wires a transport. Nothing infers it: a coordinator
    // with a link set in a test is not a range with a socket.
    void setTransportAttached(bool attached);
    bool transportAttached() const { return m_transportAttached; }

    // A transport can be wired and still unusable because no range key was
    // configured. Those are DIFFERENT operator problems - one is "this build
    // cannot", the other is "copy the key across" - so they are never
    // collapsed into one message.
    void setKeyConfigured(bool configured, const QString& detail = QString());
    bool keyConfigured() const { return m_keyConfigured; }

    int laneCount() const { return m_rows.size(); }
    int authenticatedCount() const;
    int unobservedShots() const;
    int lanesBehind() const;
    // Lanes passing through a restart: detected, reauthenticating or replaying.
    int lanesRecovering() const;
    QString statusLine() const;
    QString tone() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    // Rebuilds from the monitor and the coordinator. Called on the same timer
    // the rest of the dashboard uses.
    void refresh();

signals:
    void summaryChanged();

private:
    struct Row {
        QString nodeId;
        QString laneLabel;
        bool    authenticated = false;
        QString channel;
        int     restarts = 0;
        int     pendingCommands = 0;
        QString syncQuality;
        int     unobserved = 0;
        int     reconciledTo = -1;
    };

    RangeMonitor* m_monitor = nullptr;
    RangeControlCoordinator* m_coordinator = nullptr;
    bool m_transportAttached = false;
    bool m_keyConfigured = false;
    QString m_keyDetail;
    QVector<Row> m_rows;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_CONTROLSTATUSMODEL_H
