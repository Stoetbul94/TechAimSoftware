#include "customprint.h"
#include "defines.h"

#include <QFileDialog>
#include <QPainter>
#include <QImage>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QPdfWriter>
#include <QVariant>
#include <QDebug>
#include <QDateTime>

#include <QMessageBox>
#include <QTableView>

CustomPrint::CustomPrint(TachusWidget *tachus, QObject *parent) : QObject(parent), m_tachus(tachus)
{

}

void CustomPrint::printPNG(QVariant data)
{
    return;
//    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
//                                                    "untitled.png",
//                                                    tr("Images (*.png *.xpm *.jpg)"));
    QString fileName = QString("test_pdf_%1.png").arg(QDateTime::currentDateTime().toString("ddMMyyyy-hhmmss"));
    QImage img = qvariant_cast<QImage>(data);
    if(img.isNull())
    {
        qDebug() << "In valid image" << endl;
    }
    img.save(fileName);
    emit saveComplete();
}

void CustomPrint::printTest()
{
    //    const QString fileName("test1.pdf");
    //    QPdfWriter pdfWriter(fileName);
    //    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    //    QPainter painter(&pdfWriter);

    //    painter.drawPixmap(QRect(0,0,pdfWriter.logicalDpiX()*8.3,pdfWriter.logicalDpiY()*11.7), QPixmap("test.png"));

    QPdfWriter pdfWriter("mytest.pdf");
    QPainter painter(&pdfWriter);
    quint32 iYPos = 10;

    QPixmap pxPic;
    pxPic.load("test.png", "PNG");
    //painter.drawPixmap(0, iYPos, pxPic.width(), pxPic.height(), pxPic);
    //iYPos += pxPic.height() + 250;

    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    qDebug() << "************************************************* pdf resolution" <<pdfWriter.resolution();
    QSize s(iWidth, iHeight);
    QPixmap pxScaledPic = pxPic.scaled(s, Qt::KeepAspectRatio, Qt::FastTransformation);
    painter.drawPixmap(0, iYPos, pxScaledPic.width(), pxScaledPic.height(), pxScaledPic);
    iYPos += pxScaledPic.height() + 250;

    pdfWriter.setResolution(2400);
    qDebug() << "************************************************* 2 pdf resolution" <<pdfWriter.resolution();
    painter.drawPixmap(0, iYPos, pxScaledPic.width(), pxScaledPic.height(), pxScaledPic);
    iYPos += pxScaledPic.height() + 250;

    pdfWriter.setResolution(600);
    qDebug() << "************************************************* 3 pdf resolution" <<pdfWriter.resolution();
    painter.drawPixmap(0, iYPos, pxScaledPic.width(), pxScaledPic.height(), pxScaledPic);
    iYPos += pxScaledPic.height() + 250;
}

void CustomPrint::clearImagesList()
{
    m_images.clear();
}

void CustomPrint::addImage(QVariant data)
{
    QImage img = qvariant_cast<QImage>(data);
    if(!img.isNull())
    {
        m_images.append(img);
    }
    printPNG(data);
}

void CustomPrint::createPdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "untitled.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    QSize s(iWidth, iHeight);
    //quint32 iYPos = 10;

    for (int i=0; i<m_images.count(); ++i)
    {
        if (i >= 1) {
            qDebug() << "new page added " <<pdfWriter.newPage();
        }

        QImage img = m_images.at(i);
        QImage img1 = m_images.at(i);
        if(!img.isNull())
        {
            img = img.scaledToWidth(iWidth);
            painter.drawImage(QRectF(0, 0, img.width(), img.height()), img1, img1.rect());
            //iYPos += img.height() + 250;
        }
    }
    painter.end();
    emit saveComplete();

}

void CustomPrint::createTablePdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "summary_report.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;

    TestModel* pTableModel = new TestModel(this);
    QList<QString> contactNames;
    QList<QString> contactPhoneNums;

    // Create some data that is tabular in nature:
    contactNames.append("Thomas");
    contactNames.append("Richard");
    contactNames.append("Harrison");
    contactPhoneNums.append("123-456-7890");
    contactPhoneNums.append("222-333-4444");
    contactPhoneNums.append("333-444-5555");

    pTableModel->populateData(contactNames, contactPhoneNums);

    QTableView* pTableView = new QTableView;
    pTableView->setModel(pTableModel);

    int width = 0;
    int height = 0;
//    pTableView->setFont(QFont("Courier New", 24, QFont::Bold));
    int columns = pTableModel->columnCount();
    int rows = pTableModel->rowCount();

    pTableView->resizeColumnsToContents();

    for( int i = 0; i < columns; ++i ) {
        width += pTableView->columnWidth(i);
    }

    for( int i = 0; i < rows; ++i ) {
        height += pTableView->rowHeight(i);
    }

    pTableView->setFixedSize(width, height);
    pTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);

    //pTableView->render(&painter);
    QPixmap pixmap(pTableView->size());
    pTableView->render(&pixmap);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    pixmap = pixmap.scaledToWidth(iWidth);
//    painter.drawPixmap(100, 100, iWidth, iHeight/2, pixmap);
    painter.drawPixmap(QPointF(100, 100), pixmap, pixmap.rect());
    painter.end();
}

void CustomPrint::createSummryPdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "summary_report.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    QSize s(iWidth, iHeight);
    //quint32 iYPos = 10;

    int totalHeight = 0;
    int heightOffset = 80;
    int fontSize = 7;
    int heightOffsetFor8Font = 10*fontSize;
    for (int i=0; i<m_images.count(); ++i)
    {
        if (i >= 1) {
            qDebug() << "new page added " <<pdfWriter.newPage();
        }

        QImage img = m_images.at(i);
        QImage img1 = m_images.at(i);
        if(!img.isNull())
        {
            img = img.scaledToWidth(iWidth*0.9);
            painter.drawImage(QRectF(0, 0, img1.width()*3, img1.height()), img1, img1.rect());
            totalHeight += img1.height();
            //iYPos += img.height() + 250;
        }
    }

    painter.setPen(Qt::blue);
    painter.setFont(QFont("Times", 10));
#ifdef BRAND_TACHUS
    QImage bImage(":/images/logo/tachus_logo.png");
    int offset = 3;
    int calWidth = 578*offset;
    int calHeight = 176*offset;
    QImage setaBImage(":/images/logo/seta.png");
    painter.drawImage(QRectF(iWidth-calWidth-20, 0, calWidth, calHeight),
                      bImage, bImage.rect());

    painter.setPen(Qt::red);
    if (!m_tachus->getIsAppDemoMode())
        painter.drawText(QRectF(iWidth-calWidth-20, calHeight, calWidth, calHeight), Qt::AlignRight, "DEMO");
#else
    QImage bImage(":/images/logo/seta.png");
    painter.drawImage(QRectF(iWidth-bImage.width()-20, 0, bImage.width(), bImage.height()), bImage, bImage.rect());
    //painter.drawText(QRectF(0, 0, iWidth-100, 200), Qt::AlignRight, "SETA");
#endif
    painter.setPen(Qt::black);

//    QPen blackPen;
//    blackPen.setWidth(2);
//    blackPen.setColor(Qt::black);
//    blackPen.setStyle(Qt::DotLine);

//    painter.setPen(blackPen);
//    painter.setBrush(Qt::NoBrush);
//    totalHeight += 50;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
    totalHeight += 20;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));

    QStringList dataList = m_tachus->getPDFString();
    qDebug() <<"ppppppppppppppppppppp"<<dataList;
    bool earlierText = false;
    for(int i=0; i<dataList.count(); ++i) {
        QString data = dataList.at(i);
        if (data.startsWith("--")) {
            totalHeight += 2*heightOffset;
            painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
            earlierText = false;
        } else if (data.startsWith("Series No")) {
            painter.setPen(Qt::blue);
            painter.setFont(QFont("Times", 8));
            QStringList fragmentedDatList = data.split("###");
            int eachColWid = (iWidth-10)/fragmentedDatList.count();
            if (earlierText)
                totalHeight += 2* heightOffset;
            else
                totalHeight += 20;
            for (int j=0; j<fragmentedDatList.count();++j) {
                QString curData = fragmentedDatList.at(j);
                QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffset);
                painter.drawText(rectf, Qt::AlignLeft, fragmentedDatList.at(j));
            }
            earlierText = true;
            painter.setPen(Qt::black);
        } else if (data.startsWith("newpage")) {
            pdfWriter.newPage();
            totalHeight = 0;
            painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
            totalHeight += 20;
            painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
        }else {
            painter.setFont(QFont("Times", fontSize));
            QStringList fragmentedDatList = data.split("###");
            int eachColWid = (iWidth-10)/(fragmentedDatList.count());
            if (earlierText)
                totalHeight += 2* heightOffsetFor8Font;
            else
                totalHeight += 20;
            for (int j=0; j<fragmentedDatList.count();++j) {
                QString curData = fragmentedDatList.at(j);
                if (curData.contains("direction")) {
                    QStringList scoreDirectionDataList = curData.split("direction");
                    QString score = scoreDirectionDataList.at(0);
                    int direction = scoreDirectionDataList.at(1).toInt();
                    QImage image = QImage(":/images/rightPanel/up_arrow.png");
//                    image.scaledToHeight(2*heightOffsetFor8Font - 10);

                    QPoint center = image.rect().center();
                    QMatrix matrix;
                    matrix.translate(center.x(), center.y());
                    matrix.rotate(direction);
                    QImage dstImg = image.transformed(matrix);

//                    QRectF rectf((j-1)*eachColWid+ 30, totalHeight, 10, 2*heightOffset);
                    QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                    QRectF rectf1(j*eachColWid + 250, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                    QRectF rectf2(j*eachColWid + 250, totalHeight, 2*heightOffsetFor8Font, 2*heightOffsetFor8Font);
                    painter.drawText(rectf, Qt::AlignLeft, score);
//                    painter.rotate(direction);
//                    painter.drawText(rectf1, Qt::AlignLeft, "->");
                    painter.drawImage(rectf2, dstImg);
//                    painter.rotate(-direction);
                } else {
                    QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                    painter.drawText(rectf, Qt::AlignLeft, fragmentedDatList.at(j));
                }
            }
            earlierText = true;
        }

//        if (i==3)
//            painter.setFont(QFont("Times", 8));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////
    /// decision for anlytic
    ///
    /// /////////////////////////////////////////////////////////////////////////////////////////

    qDebug() << __LINE__ << m_tachus->getCurrentMatchTotalShotsCount();
    if (m_tachus->getCurrentMatchTotalShotsCount() == -1 ||
        m_tachus->getCurrentMatchTotalShotsCount() == 10 ||
        m_tachus->getCurrentMatchTotalShotsCount() == 20) {
        painter.end();
        return;
    }

#ifndef BRAND_TACHUS
    return;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// tachus analytic
    ///
    /// ///////////////////////////////////////////////////////////////////////////////////////////

    pdfWriter.newPage();
    totalHeight = 0;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
    totalHeight += 20;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
    totalHeight += 40;

    // for text -  Tachus Analytics
    painter.setFont(QFont("Times", 16));
    QRectF rectf(10, totalHeight, iWidth, 4*heightOffsetFor8Font);
    painter.drawText(rectf, Qt::AlignCenter,  "Tachus Analytics" );
    totalHeight += rectf.height()+40;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
    totalHeight += 20;
    painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
    totalHeight += 300;

    // for text - Series comparison table
    QRectF rectf1(10, totalHeight, iWidth, 3*heightOffsetFor8Font);
    painter.setFont(QFont("Times", 9));
    painter.drawText(rectf1, Qt::AlignCenter,  "Series Comparison Table" );
    totalHeight += rectf1.height()+120;

    // draw Series comparison table
    {
        QStringList sctableData = m_tachus->getSeriesComparisionData();
        earlierText = false;
        for(int i=0; i<sctableData.count(); ++i) {
            QString data = sctableData.at(i);
            if (data.startsWith("--")) {
                totalHeight += 2*heightOffset;
                painter.drawLine(QLineF(10, totalHeight, iWidth-10, totalHeight));
                earlierText = false;
            } else if (data.startsWith("Shot No")) {
                painter.setPen(Qt::blue);
                painter.setFont(QFont("Times", 8));
                QStringList fragmentedDatList = data.split("###");
                int eachColWid = (iWidth-10)/fragmentedDatList.count();
                if (earlierText)
                    totalHeight += 2* heightOffset;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffset);
                    painter.drawRect(rectf);
                    painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                }
                earlierText = true;
                painter.setPen(Qt::black);
            } else {
                painter.setFont(QFont("Times", fontSize));
                QStringList fragmentedDatList = data.split("###");
                int eachColWid = (iWidth-10)/7/*(fragmentedDatList.count())*/;
                if (earlierText)
                    totalHeight += 2* heightOffsetFor8Font;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    if (curData.contains("direction")) {
                        QStringList scoreDirectionDataList = curData.split("direction");
                        QString score = scoreDirectionDataList.at(0);
                        int direction = scoreDirectionDataList.at(1).toInt();
                        QImage image = QImage(":/images/rightPanel/up_arrow.png");
                        //                    image.scaledToHeight(2*heightOffsetFor8Font - 10);

                        QPoint center = image.rect().center();
                        QMatrix matrix;
                        matrix.translate(center.x(), center.y());
                        matrix.rotate(direction);
                        QImage dstImg = image.transformed(matrix);

                        //                    QRectF rectf((j-1)*eachColWid+ 30, totalHeight, 10, 2*heightOffset);
                        QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        QRectF rectf1(j*eachColWid + 250, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        QRectF rectf2(j*eachColWid + eachColWid/2 + 150, totalHeight, 2*heightOffsetFor8Font, 2*heightOffsetFor8Font);
                        painter.drawRect(rectf);
                        painter.drawText(rectf, Qt::AlignCenter, score);
                        //                    painter.rotate(direction);
                        //                    painter.drawText(rectf1, Qt::AlignLeft, "->");
                        painter.drawImage(rectf2, dstImg);
                        //                    painter.rotate(-direction);
                    } else {
                        QRectF rectf(j*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        painter.drawRect(rectf);
                        painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                    }
                }
                earlierText = true;
            }
        }
        totalHeight += 300;
    }
    // for text Shot Interval Table
    {
        QRectF rectf2(10, totalHeight, iWidth, 2*heightOffsetFor8Font);
        painter.setFont(QFont("Times", 10));
        painter.drawText(rectf2, Qt::AlignCenter,  "Shot Interval Table" );
        totalHeight += rectf2.height()+80;

        // draw Shot Interval table
        QStringList shootIntervaltableData = m_tachus->getShotIntervalData();
        int startPoint  = iWidth/5;
        int endPoint = 4*iWidth/5;
        earlierText = false;
        for(int i=0; i<shootIntervaltableData.count(); ++i) {
            QString data = shootIntervaltableData.at(i);
            QStringList fragmentedDatList = data.split("###");
            int eachColWid = (iWidth-10)/(fragmentedDatList.count()+2);
            if (data.startsWith("--")) {
                totalHeight += 2*heightOffset;
                painter.drawLine(QLineF(startPoint, totalHeight, endPoint, totalHeight));
                earlierText = false;
            } else if (data.startsWith("Interval (In Seconds")) {
                painter.setPen(Qt::blue);
                painter.setFont(QFont("Times", 8));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffset;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    QRectF rectf((j+1)*eachColWid, totalHeight, eachColWid, 2*heightOffset);
                    painter.drawRect(rectf);
                    painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                }
                earlierText = true;
                painter.setPen(Qt::black);
            } else {
                painter.setFont(QFont("Times", fontSize));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffsetFor8Font;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    {
                        QRectF rectf((j+1)*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        painter.drawRect(rectf);
                        painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                    }
                }
                earlierText = true;
            }
        }
        totalHeight += 300;
    }
    // for text Time Series Table
    {
        QRectF rectf3(10, totalHeight, iWidth, 2*heightOffsetFor8Font);
        painter.setFont(QFont("Times", 10));
        painter.drawText(rectf3, Qt::AlignCenter,  " Time Series Table " );
        totalHeight += rectf3.height()+80;

        // draw Time Series Table
        QStringList tstableData = m_tachus->getTimeSeriesData();
        //    int startPoint  = iWidth/5;
        //    int endPoint = 4*iWidth/5;
        earlierText = false;
        for(int i=0; i<tstableData.count(); ++i) {
            QString data = tstableData.at(i);
            QStringList fragmentedDatList = data.split("###");
            int eachColWid = (iWidth-10)/(fragmentedDatList.count()+2);
            if (data.startsWith("--")) {
//                totalHeight += 2*heightOffset;
//                painter.drawLine(QLineF(startPoint, totalHeight, endPoint, totalHeight));
//                earlierText = false;
            } else if (data.startsWith("Series Number")) {
                painter.setPen(Qt::blue);
                painter.setFont(QFont("Times", 8));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffset;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    QRectF rectf((j+1)*eachColWid, totalHeight, eachColWid, 2*heightOffset);
                    painter.drawRect(rectf);
                    painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                }
                earlierText = true;
                painter.setPen(Qt::black);
            } else {
                painter.setFont(QFont("Times", fontSize));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffsetFor8Font;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    {
                        QRectF rectf((j+1)*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        painter.drawRect(rectf);
                        painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                    }
                }
                earlierText = true;
            }
        }
        totalHeight += 300;
    }

    // Zone Table
    {
        QRectF rectf3(10, totalHeight, iWidth, 2*heightOffsetFor8Font);
        painter.setFont(QFont("Times", 10));
        painter.drawText(rectf3, Qt::AlignCenter,  "Zone Table" );
        totalHeight += rectf3.height()+80;

        int topHeaderWidth = (iWidth-20)/3;
        QRectF rectth1(10, totalHeight, topHeaderWidth, 2*heightOffset);
        painter.drawRect(rectth1);
        painter.fillRect(rectth1, Qt::green);
        painter.drawText(rectth1, Qt::AlignCenter, "Green Zone (10 - 10.9");

        QRectF rectth2(10+topHeaderWidth, totalHeight, topHeaderWidth, 2*heightOffset);
        painter.drawRect(rectth2);
        painter.fillRect(rectth2, Qt::yellow);
        painter.drawText(rectth2, Qt::AlignCenter, "Yellow Zone (9 - 9.9");

        QRectF rectth3(10+topHeaderWidth+topHeaderWidth, totalHeight, topHeaderWidth, 2*heightOffset);
        painter.drawRect(rectth3);
        painter.fillRect(rectth3, Qt::red);
        painter.drawText(rectth3, Qt::AlignCenter, "Red Zone (below 9");

        totalHeight += 2* heightOffset;

        QStringList tstableData = m_tachus->getZoneTableData();
        //    int startPoint  = iWidth/5;
        //    int endPoint = 4*iWidth/5;
        earlierText = false;
        for(int i=0; i<tstableData.count(); ++i) {
            QString data = tstableData.at(i);
            QStringList fragmentedDatList = data.split("###");
            int eachColWid = (iWidth-20)/(fragmentedDatList.count());
            if (data.startsWith("--")) {
//                totalHeight += 2*heightOffset;
//                painter.drawLine(QLineF(startPoint, totalHeight, endPoint, totalHeight));
//                earlierText = false;
            } else if (data.startsWith("Shot")) {
//                painter.setPen(Qt::blue);
//                painter.setFont(QFont("Times", 8));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffset;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    QRectF rectf((j)*eachColWid, totalHeight, eachColWid, 2*heightOffset);
                    painter.drawRect(rectf);
                    painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                }
                earlierText = true;
                painter.setPen(Qt::black);
            } else {
                painter.setFont(QFont("Times", fontSize));
                QStringList fragmentedDatList = data.split("###");
                if (earlierText)
                    totalHeight += 2* heightOffsetFor8Font;
                else
                    totalHeight += 20;
                for (int j=0; j<fragmentedDatList.count();++j) {
                    QString curData = fragmentedDatList.at(j);
                    {
                        QRectF rectf((j)*eachColWid, totalHeight, eachColWid, 2*heightOffsetFor8Font);
                        painter.drawRect(rectf);
                        painter.drawText(rectf, Qt::AlignCenter, fragmentedDatList.at(j));
                    }
                }
                earlierText = true;
            }
        }
        totalHeight += 300;
    }

    //////////

    painter.end();
}

void CustomPrint::setServerPath(QString path)
{
    qDebug() << __FUNCTION__ << __LINE__ << path;
    m_serverPath = path;
}

void CustomPrint::createPdf(QString filePath)
{
//    QMessageBox msgBox;
//    msgBox.setText(QString("Creating report pdf @ %1.").arg(filePath));
//    msgBox.exec();
    QString msg = QString("Creating report pdf @ %1.").arg(filePath);
    TimedMessageBox * tmb = new TimedMessageBox(5, tr("Printing"), msg, QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel, QMessageBox::NoButton,nullptr);
    int ret = tmb->exec();
    delete tmb;
    printf("ret=%i\n", ret);


    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    QSize s(iWidth, iHeight);
    //quint32 iYPos = 10;

    for (int i=0; i<m_images.count(); ++i)
    {
        if (i >= 1) {
            qDebug() << "new page added " <<pdfWriter.newPage();
        }

        QImage img = m_images.at(i);
        QImage img1 = m_images.at(i);
        if(!img.isNull())
        {
            img = img.scaledToWidth(iWidth);
            painter.drawImage(QRectF(0, 0, img.width(), img.height()), img1, img1.rect());
            //iYPos += img.height() + 250;
        }
    }
    painter.end();
    emit saveComplete();
}

void CustomPrint::print(QVariant data)
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "untitled.pdf",
                                                    tr("*.pdf"));
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();
    QSize s(iWidth, iHeight);
    quint32 iYPos = 10;

    qDebug() <<iWidth << " height " << iHeight<< "************************************************* print pdf resolution" <<pdfWriter.resolution();
    QImage img = qvariant_cast<QImage>(data);
    qDebug() <<img.width() << " height " << img.height()<< "************************************************* ";
    if(!img.isNull())
    {
        img.scaled(s, Qt::KeepAspectRatio, Qt::FastTransformation);
        qDebug() <<img.width() << " height " << img.height()<< "************************************************* after";
        painter.drawImage(QRectF(0, iYPos, iWidth, iHeight), img, img.rect());
        iYPos += img.height() + 250;
        painter.end();
    }
    emit saveComplete();
}


///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
///

TestModel::TestModel(QObject *parent) : QAbstractTableModel(parent)
{
}

// Create a method to populate the model with data:
void TestModel::populateData(const QList<QString> &contactName,const QList<QString> &contactPhone)
{
    tm_contact_name.clear();
    tm_contact_name = contactName;
    tm_contact_phone.clear();
    tm_contact_phone = contactPhone;
    return;
}

int TestModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return tm_contact_name.length();
}

int TestModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    } else if(index.isValid() && role==Qt::FontRole)
        return QFont("Courier New", 80, QFont::Bold);

    if (index.column() == 0) {
        return tm_contact_name[index.row()];
    } else if (index.column() == 1) {
        return tm_contact_phone[index.row()];
    }
    return QVariant();
}

QVariant TestModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return QString("Name");
        } else if (section == 1) {
            return QString("Phone");
        }
    }
    return QVariant();
}
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
///
