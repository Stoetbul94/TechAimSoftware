#include "pdfexporter.h"

#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QFileDialog>
#include <QStandardPaths>
#include <QRegularExpression>

PdfExporter::PdfExporter(QObject* parent) : QObject(parent) {}

void PdfExporter::clear()
{
    m_sections.clear();
}

void PdfExporter::addSection(const QVariant& image)
{
    QImage img = qvariant_cast<QImage>(image);
    if (!img.isNull()) m_sections.append(img);
}

static QString sanitisePart(const QString& s, const QString& fallback)
{
    QString out = s;
    out.replace(QRegularExpression("[^A-Za-z0-9-]+"), "");   // filename-safe; keep hyphens (dates)
    return out.isEmpty() ? fallback : out;
}

QString PdfExporter::suggestFileName(const QString& athlete,
                                     const QString& discipline,
                                     const QString& date) const
{
    return "TechAim_CoachReport_"
         + sanitisePart(athlete, "Athlete") + "_"
         + sanitisePart(discipline, "Discipline") + "_"
         + sanitisePart(date, "Date") + ".pdf";
}

QString PdfExporter::exportToFile(const QString& suggestedName)
{
    if (m_sections.isEmpty()) { emit exportFinished(QString()); return QString(); }

    const QString startDir =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + suggestedName;
    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Export Coach Report PDF"),
                                                    startDir, tr("PDF Document (*.pdf)"));
    if (fileName.isEmpty()) { emit exportFinished(QString()); return QString(); }
    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) fileName += ".pdf";

    const bool ok = composeToFile(fileName);
    const QString result = ok ? fileName : QString();
    emit exportFinished(result);
    return result;
}

bool PdfExporter::composeToFile(const QString& fileName)
{
    if (m_sections.isEmpty() || fileName.isEmpty()) return false;

    QPdfWriter writer(fileName);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Portrait);
    writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
    // 300 dpi: the default 1200 dpi made the content width ~8800 device px, a
    // 6x upscale of the ~1500 px section grabs (blurry and slow to compose).
    // At 300 dpi the grabs land close to 1:1.
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) return false;
    const int W = writer.width();     // content width  (device px within margins)
    const int H = writer.height();    // content height
    const int gap = qMax(1, H / 60);  // small gap between sections

    int y = 0;
    for (const QImage& src : m_sections) {
        if (src.isNull()) continue;
        QImage img = src.scaledToWidth(W, Qt::SmoothTransformation);
        // Keep a section intact: if it is taller than a full page, shrink it to
        // fit one page rather than splitting it across a page break.
        if (img.height() > H) img = img.scaledToHeight(H, Qt::SmoothTransformation);
        // Start a new page when this section would not fit in the remaining space.
        if (y > 0 && y + img.height() > H) { writer.newPage(); y = 0; }
        painter.drawImage(QPointF((W - img.width()) / 2.0, y), img);
        y += img.height() + gap;
    }
    painter.end();
    return true;
}
