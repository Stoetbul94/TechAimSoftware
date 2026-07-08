#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

// ============================================================================
//  PdfExporter — A4 PDF export of the Coach Report Print view (context: PDFEXPORT)
// ----------------------------------------------------------------------------
//  QML grabs each Print-view SECTION to an image (item.grabToImage ->
//  addSection(result.image)); this composes them onto A4 portrait pages,
//  packing sections and starting a new page only when the next section would
//  not fit — so page breaks never cut through a section. It touches no
//  analytics: it only lays out images the Print view already rendered from the
//  currently loaded CoachReportData / CoachDiary.
// ============================================================================

#include <QObject>
#include <QVariant>
#include <QString>
#include <QList>
#include <QImage>

class PdfExporter : public QObject
{
    Q_OBJECT
public:
    explicit PdfExporter(QObject* parent = nullptr);

    Q_INVOKABLE void clear();
    // Append one grabbed section image (result.image from grabToImage).
    Q_INVOKABLE void addSection(const QVariant& image);
    Q_INVOKABLE int  sectionCount() const { return m_sections.size(); }

    // TechAim_CoachReport_<Athlete>_<Discipline>_<Date>.pdf  (sanitised).
    Q_INVOKABLE QString suggestFileName(const QString& athlete,
                                        const QString& discipline,
                                        const QString& date) const;

    // Compose the accumulated sections into an A4 portrait PDF. Opens a save
    // dialog pre-filled with `suggestedName`; returns the written path, or an
    // empty string if cancelled / nothing to export.
    Q_INVOKABLE QString exportToFile(const QString& suggestedName);

    // Write the accumulated sections to `fileName` as an A4 portrait PDF (no
    // dialog). A4 packing with section-aware page breaks. Returns true on
    // success. Public so the composition can be unit-tested without a GUI.
    Q_INVOKABLE bool composeToFile(const QString& fileName);

signals:
    void exportFinished(const QString& path);   // path empty => cancelled/failed

private:
    QList<QImage> m_sections;
};

#endif // PDFEXPORTER_H
