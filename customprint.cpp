#include "customprint.h"
#include "defines.h"

#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QImage>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QPdfWriter>
#include <QVariant>
#include <QDebug>
#include <QDateTime>


CustomPrint::CustomPrint(TachusWidget *tachus, QObject *parent) : QObject(parent), m_tachus(tachus)
{

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
}

void CustomPrint::createPdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "untitled.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
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

// 3P FINAL report (Phase D3). Same writer behaviour as the redesigned
// createSummryPdf — each grabbed A4 page image fitted within the printable
// page, aspect preserved, centred, one image per PDF page — with a finals
// default filename. No qualification path is touched.
void CustomPrint::createFinalsPdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "finals_report.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;
    if (fileName.isEmpty())
        return;
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    quint32 iWidth = pdfWriter.width();
    quint32 iHeight = pdfWriter.height();

    for (int i = 0; i < m_images.count(); ++i)
    {
        if (i >= 1)
            pdfWriter.newPage();
        QImage img1 = m_images.at(i);
        if (!img1.isNull())
        {
            QImage img = img1.scaled(iWidth, iHeight, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
            qreal xOff = (iWidth - img.width()) / 2.0;
            painter.drawImage(QRectF(xOff, 0, img.width(), img.height()),
                              img1, img1.rect());
        }
    }
    painter.end();
    emit saveComplete();
}

bool CustomPrint::createTrainingPdf(QString filePath)
{
    if (filePath.isEmpty() || m_images.isEmpty()) {
        emit printingNotice(tr("Training report could not be created (no pages)."), 5000);
        return false;
    }
    // Fail early + clearly if the target is not writable (surfaced to the UI).
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
    QFile probe(filePath);
    if (!probe.open(QIODevice::WriteOnly)) {
        emit printingNotice(tr("Could not write the training report to %1").arg(filePath), 6000);
        return false;
    }
    probe.close();

    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));
    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        emit printingNotice(tr("Could not create the training report PDF."), 6000);
        return false;
    }
    const quint32 iWidth = pdfWriter.width();
    const quint32 iHeight = pdfWriter.height();
    for (int i = 0; i < m_images.count(); ++i) {
        if (i >= 1)
            pdfWriter.newPage();
        const QImage img1 = m_images.at(i);
        if (!img1.isNull()) {
            const QImage img = img1.scaled(iWidth, iHeight, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
            const qreal xOff = (iWidth - img.width()) / 2.0;
            painter.drawImage(QRectF(xOff, 0, img.width(), img.height()),
                              img1, img1.rect());
        }
    }
    painter.end();
    emit printingNotice(tr("Training report saved to %1").arg(filePath), 5000);
    emit saveComplete();
    return true;
}

void CustomPrint::createSummryPdf()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
                                                    "summary_report.pdf",
                                                    tr("*.pdf"));
    qDebug() << __FUNCTION__ << fileName;
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
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
            // Fit the grabbed A4 page within the printable page, preserving the
            // aspect ratio and centring it horizontally (was drawn at 3x width,
            // which distorted and overflowed the page).
            img = img1.scaled(iWidth, iHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            qreal xOff = (iWidth - img.width()) / 2.0;
            painter.drawImage(QRectF(xOff, 0, img.width(), img.height()), img1, img1.rect());
            totalHeight += img.height();
        }
    }
    // The redesigned Match Summary page is a self-contained A4 report (header,
    // executive summary, target, footer), so the PDF is exactly that page image
    // (PDF == print view). The legacy logo overlay and raw text dump below are
    // intentionally skipped.
    painter.end();
    return;

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
                    QTransform matrix;
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
                        QTransform matrix;
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
    // TechAim dialog framework (C5): the legacy TimedMessageBox blocked this
    // thread for up to 5 s before the PDF was written; the QML notice is
    // non-blocking (auto-dismisses) and the export proceeds immediately.
    emit printingNotice(QString("Creating report PDF at %1").arg(filePath), 5000);


    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
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
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
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

// Create a method to populate the model with data:
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
///
