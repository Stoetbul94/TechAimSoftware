#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMutex>
#include <QSettings>
#include <QLabel>
#include <QString>
#include <QProcess>
#include <QPlainTextEdit>

#include "../forms/about.h"
#include "../forms/settingsmodbusrtu.h"
#include "../forms/settingsmodbustcp.h"
#include "../forms/settings.h"
#include "../forms/busmonitor.h"
#include "modbuscommsettings.h"
#include "modbusadapter.h"
#include "infobar.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, ModbusAdapter *adapter = 0, ModbusCommSettings *settings = 0);
    ~MainWindow();
    void showUpInfoBar(QString message, InfoBar::InfoType type);
    void hideInfoBar();
    QPlainTextEdit* getTextEdit() {
        if (m_busMonitor)
            return m_busMonitor->getPlaintTextEdit();
    }
    void tachusReconfigurePortNumber();

private:
    // Serializes every libmodbus transaction; see modbusReadRegistry().
    QMutex m_modbusTransport;

    Ui::MainWindow *ui;
    //UI - Dialogs
    About *m_dlgAbout;
    SettingsModbusRTU *m_dlgModbusRTU;
    SettingsModbusTCP *m_dlgModbusTCP;
    Settings *m_dlgSettings;
    BusMonitor *m_busMonitor;

    ModbusCommSettings *m_modbusCommSettings;
    void updateStatusBar();
    QLabel *m_statusText;
    QLabel *m_statusInd;
    QLabel *m_baseAddr;
    QLabel *m_statusPackets;
    QLabel *m_statusErrors;
    ModbusAdapter *m_modbus;
    void modbusConnect(bool connect, QString portName = QString());

    void changeEvent(QEvent* event);

private slots:
    void showSettingsModbusRTU();
    void showSettingsModbusTCP();
    void showSettings();
    void showBusMonitor();
    void changedModbusMode(int currIndex);
    void changedFunctionCode(int currIndex);
    void changedBase(int currIndex);
    void changedStartAddrBase(int currIndex);
    void changedScanRate(int value);
    void changedNoOfRegs(int value);
    void changedSlaveID(int value);
    void addItems();
    void clearItems();
    void openLogFile();
    void scan(bool value);
    void refreshView();
    void changeLanguage();
    void openModbusManual();
    void loadSession();
    void saveSession();

public slots:
    bool isModBusConnected();
    void changedConnect(bool value, QString portName = QString());
    // SERIAL-AUTO-001: read the stored serial port WITHOUT connecting, so the
    // selector can treat it as a last-resort candidate instead of having
    // ModbusCommSettings substitute it silently inside modbusConnect().
    QString storedSerialPortName() const;

    // True when the configured transport is Modbus TCP rather than serial RTU.
    // The serial selector must not gate the TCP path: with no serial device
    // present it returns no candidate and the connection is abandoned before
    // TCP is ever attempted. Found while bringing up the target emulator.
    bool isModbusTcpMode() const;
    void request();
    void changedStartAddress(int value);
    void setSBStartAddValue(int value, int type);
    QStringList getData();
    // ── THE MODBUS TRANSPORT AUTHORITY (THREAD-MODBUS-006) ───────────────
    // Every direct register access in the product goes through these two
    // functions, and they are the ONLY place a libmodbus transaction is
    // started. Acquisition polls from the GUI thread; MotorThread drives the
    // paper feed from its own; a WorkerThread used to write the counter reset
    // from a third. One libmodbus context cannot serve two frames at once - a
    // reply read by the wrong caller is a corrupted coordinate, which is
    // indistinguishable from a real one.
    //
    // The lock is held for ONE transaction and released; callers that wait
    // (motor status polling) wait outside it.
    int modbusWriteSingleRegister(int startAdd, int value);
    int modbusReadRegistry(int startAdd, int noOfItem, uint16_t* dest);

signals:
    void resetCounters();

};

extern MainWindow *mainWin;

#endif // MAINWINDOW_H
