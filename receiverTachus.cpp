/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#include <QtNetwork>
#include <QDebug>

#include "receiverTachus.h"

#define DATA_DELIMITER "&*&"

ReceiverTachus::ReceiverTachus(QWidget *parent)
    : QWidget(parent)
{
    statusLabel = new QLabel(tr("Listening for broadcasted messages"));
    statusLabel->setWordWrap(true);

    auto quitButton = new QPushButton(tr("&Quit"));

//! [0]
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(7756, QUdpSocket::ShareAddress);
//! [0]

//! [1]
    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));
//! [1]
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Broadcast Receiver"));
}

void ReceiverTachus::test()
{

}

void ReceiverTachus::processPendingDatagrams()
{
    qDebug() <<__FUNCTION__ << __LINE__;
    if (!m_tachus)
        return;

    QByteArray datagram;
//! [2]
    while (udpSocket->hasPendingDatagrams()) {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        statusLabel->setText(tr("Received datagram: \"%1\"")
                             .arg(datagram.constData()));
        QString dataString = datagram.constData();
        QStringList dataList = dataString.split(DATA_DELIMITER);
        qDebug() << dataList << "  " << dataList.count()  << dataList[0] << dataList[1] << __LINE__;
        QStringList temp;
        temp  <<"from TCMA "<< dataList;
        temp<< "  ";
        temp<< QString::number(dataList.count());
        temp<< dataList[0];
        temp<< dataList[1];
        temp<< QString::number(__LINE__);
        m_tachus->appendToLogFile(temp.join("**"));

        if (dataList.count() < 3)
        {
            int gametype =0 ;
            int matchModeIndex = 0 ;
            int sighterTime = 0;
            int matchTimer = 0;
            int sighterPf = 0;
            int matchPf = 0;
            QStringList dataSet = dataList[1].split(",");
            QStringList data = dataSet[0].split("=");
            if( data.count()>1)
            {
                gametype = data[1].toInt();
            }
            data = dataSet[1].split("=");
            if(data.count()>1)
            {
                QStringList matchList_ = {"10 shots match", "20 shots match", "30 shots match", "40 shots match", "60 shots match","Free pratice"};
                matchModeIndex = matchList_.indexOf(data[1]);
            }
            data = dataSet[2].split("=");
            if(data.count()>1)
            {
                sighterTime = data[1].toInt();
            }
            data = dataSet[3].split("=");
            if(data.count()>1)
            {
                matchTimer = data[1].toInt();
            }
            data = dataSet[4].split("=");
            if(data.count()>1)
            {
                sighterPf = data[1].toInt();
            }
            data = dataSet[5].split("=");
            if(data.count()>1)
            {
                matchPf = data[1].toInt();
            }
            emit m_tachus->matchDetails(gametype,matchModeIndex,sighterTime,matchTimer,sighterPf,matchPf);

            qDebug() << "Gametype is" << gametype << matchModeIndex << sighterTime << matchTimer << matchPf << sighterPf << matchPf << endl;
            return;

        }
//            return;
        // list of message data
        // TCMA (indication of string coming from TCMA)
        // dedicated/nondedicated (means a broadcast message or not)
        // <ip-address>/NULL (if dedicated check ip address)
        // data (stirng or list of string)
        if (dataList[0] == "TCMA") {
            qDebug() << __FUNCTION__ << __LINE__;
            if (dataList[1] == "dedicated" && m_tachus->getIpAddress() == dataList[2]) {
                qDebug() << __FUNCTION__ << __LINE__;
                if (dataList[3] == "connected") {
                    qDebug() <<__FUNCTION__ << __LINE__;
                    m_tachus->setIsMasterConnected(true);
                }
                //("TCMA", "gametype=0,matchmode=10 shots match,sightertime=15,matchtime=15,sighterpf=110,matchpf=110")
//                return;
            }
            else if (dataList[1] == "start" && m_tachus->getIpAddress() == dataList[2]) {
                qDebug() << __FUNCTION__ << __LINE__;
                if (dataList[3] == "connected") {
                    qDebug() <<__FUNCTION__ << __LINE__;
                    emit m_tachus->startMatchFromServer();
//                    m_tachus->setIsMasterConnected(true);
                }
                //("TCMA", "gametype=0,matchmode=10 shots match,sightertime=15,matchtime=15,sighterpf=110,matchpf=110")
                return;
            }
//            else if(dataList[1].contains("gametype"))
//            {
//                int gametype =0 ;
//                int matchModeIndex = 0 ;
//                int sighterTime = 0;
//                int matchTimer = 0;
//                int sighterPf = 0;
//                int matchPf = 0;
//                if(QStringList data = dataList[1].split("="); data.count()>1)
//                {
//                    gametype = data[1].toInt();
//                }
//                if(QStringList data = dataList[2].split("="); data.count()>1)
//                {
////                        gametype
//                }
//                if(QStringList data = dataList[3].split("="); data.count()>1)
//                {
//                    sighterTime = data[1].toInt();
//                }
//                if(QStringList data = dataList[4].split("="); data.count()>1)
//                {
//                    matchTimer = data[1].toInt();
//                }
//                if(QStringList data = dataList[5].split("="); data.count()>1)
//                {
//                    sighterPf = data[1].toInt();
//                }
//                if(QStringList data = dataList[6].split("="); data.count()>1)
//                {
//                    matchPf = data[1].toInt();
//                }
//                emit m_tachus->matchDetails(gametype,matchModeIndex,sighterTime,matchTimer,sighterPf,matchPf);

//                qDebug() << "Gametype is" << gametype << matchModeIndex << sighterTime << matchTimer << matchPf << sighterPf << matchPf << endl;
//                return;
//            }

        }
    }
//! [2]
    qDebug() <<__FUNCTION__ << __LINE__;
}
