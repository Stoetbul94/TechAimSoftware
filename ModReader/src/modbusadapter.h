#ifndef MODBUSADAPTER_H
#define MODBUSADAPTER_H

#include <QObject>
#include "../3rdparty/libmodbus/modbus.h"
#include "registersmodel.h"
#include "rawdatamodel.h"
#include <QTimer>
#include <QRecursiveMutex>
#include "eutils.h"

class ModbusAdapter : public QObject
{
    Q_OBJECT
public:
    explicit ModbusAdapter(QObject *parent = 0);

     void busMonitorRequestData(uint8_t * data,uint8_t dataLen);
     void busMonitorResponseData(uint8_t * data,uint8_t dataLen);

     void modbusConnectRTU(QString port, int baud, QChar parity, int dataBits, int stopBits, int RTS, int timeOut=1);
     void modbusConnectTCP(QString ip, int port, int timeOut=1);
     void modbusDisConnect();
     RegistersModel *regModel;
     RawDataModel *rawModel;
     bool isConnected();

     void setSlave(int slave);
     void setFunctionCode(int functionCode);
     void setStartAddr(int addr);
     void setNumOfRegs(int num);
     void addItems();

     void setScanRate(int scanRate);
     void setTimeOut(int timeOut);
     void startPollTimer();
     void stopPollTimer();
     int packets();
     int errors();
     int directModbusWriteSingleRegister(int startAdd, int value);
     int directModbusReadRegistry(int startAdd, int noOfItems, uint16_t *dest);

private:
     void modbusReadData(int slave, int functionCode, int startAddress, int noOfItems);
     void modbusWriteData(int slave, int functionCode, int startAddress, int noOfItems);
     QString stripIP(QString ip);
     modbus_t * m_modbus;
     // Guards every access point to m_modbus (the shared libmodbus context).
     // libmodbus contexts are not safe for concurrent use - this app has at
     // least three independent callers that can reach m_modbus: the GUI-thread
     // poll timer (modbusTransaction -> modbusReadData/modbusWriteData), and
     // TachusWidget's MotorThread/WorkerThread (separate QThreads) calling
     // directModbusWriteSingleRegister/directModbusReadRegistry. Recursive
     // because modbusConnectRTU/modbusConnectTCP call modbusDisConnect()
     // internally, on the same thread, while already holding the lock.
     QRecursiveMutex m_modbusMutex;
     bool m_connected;
     int m_ModBusMode;
     int m_slave;
     int m_functionCode;
     int m_startAddr;
     int m_numOfRegs;
     int m_scanRate;
     QTimer *m_pollTimer;
     int m_packets;
     int m_errors;
     int m_timeOut;
     bool m_transactionIsPending;

signals:
    void refreshView();
    void modbusConnected(QString);
    void modbusDisconnected();

public slots:
    void modbusTransaction();
    void resetCounters();

};

#endif // MODBUSADAPTER_H
