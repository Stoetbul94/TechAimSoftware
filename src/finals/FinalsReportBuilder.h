#pragma once
// 3P FINAL — report assembler (Phase D1). Consumes the stored session state
// (accepted official shots, MissingShot records, incidents, stage statuses,
// subtotals, cumulative total, command timeline, metadata) and produces an
// immutable FinalsReportData + a QVariant form for QML. QtCore only.

#include <QVariantList>
#include <QVariantMap>

#include "FinalsReportData.h"

namespace techaim {
namespace finals {

class FinalsReportBuilder
{
public:
    // officialShots: accepted official finals records (schema maps, any order).
    // stageStatuses/stageSubtotals: keyed by stageName() (controller getters).
    // meta: athlete / eventName / dateTime / sighterCount.
    static FinalsReportData build(const QVariantList& officialShots,
                                  const QVariantList& missingShots,
                                  const QVariantList& incidents,
                                  const QVariantMap& stageStatuses,
                                  const QVariantMap& stageSubtotals,
                                  double cumulativeTotal,
                                  const QVariantList& commandEvents,
                                  const QVariantMap& meta);

    static QVariantMap toVariant(const FinalsReportData& d);
};

} // namespace finals
} // namespace techaim
