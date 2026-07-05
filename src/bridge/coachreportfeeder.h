#ifndef COACHREPORTFEEDER_H
#define COACHREPORTFEEDER_H

// ============================================================================
//  CoachReportFeeder — real shot-data wiring (context property COACHFEED)
// ----------------------------------------------------------------------------
//  Reads the TachusWidget game-mode match history via its public accessors,
//  converts it with the pure tachusshotbuilder, and hands the shots to the
//  CoachReportBridge to run the offline engine. This is the ONLY place that
//  knows about both TachusWidget and the report bridge; it keeps the bridge
//  generic (it just takes shots) and the builder pure (it just takes arrays).
//
//  QML triggers a report after a match with:
//      COACHFEED.analyzeCurrentMatch(gameSubMode)
//  and then reads COACHREPORT.report (or the section accessors).
// ============================================================================

#include <QObject>
#include "tachusshotbuilder.h"

class TachusWidget;
class CoachReportBridge;

class CoachReportFeeder : public QObject
{
    Q_OBJECT
    // If the hardware uses +y = down, set this so the builder flips y to the
    // engine's +y = high convention. Default false (assume +y = high).
    Q_PROPERTY(bool coordinatesFlipY READ coordinatesFlipY WRITE setCoordinatesFlipY NOTIFY flipYChanged)

public:
    CoachReportFeeder(TachusWidget* widget, CoachReportBridge* bridge, QObject* parent = nullptr);

    // Build ShotAnalyticsData from the just-finished game-mode match and run the
    // report through the bridge. gameSubMode: 0 = prone/air, 1 = 3 positions.
    // Returns true when a valid report was produced.
    Q_INVOKABLE bool analyzeCurrentMatch(int gameSubMode = 0);

    // Number of game-mode shots the feeder would read (for QML guards).
    Q_INVOKABLE int matchShotCount() const;

    bool coordinatesFlipY() const { return m_flipY; }
    void setCoordinatesFlipY(bool f);

signals:
    void flipYChanged();

private:
    techaim::bridge::MatchArrays readGameMode(int gameSubMode) const;

    TachusWidget*      m_widget;
    CoachReportBridge* m_bridge;
    bool               m_flipY = false;
};

#endif // COACHREPORTFEEDER_H
