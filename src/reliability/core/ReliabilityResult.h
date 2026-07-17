#ifndef TA_REL_RELIABILITYRESULT_H
#define TA_REL_RELIABILITYRESULT_H

// Session Reliability Layer — typed result/error carrier (M1).
// Same shape philosophy as M0's StorageResult, but code-first: the
// ReliabilityError enum is the identity, the strings are presentation.

#include "ReliabilityError.h"

#include <QString>

namespace ta {
namespace rel {

struct ErrorInfo {
    ReliabilityError code = ReliabilityError::None;
    ErrorSeverity severity = ErrorSeverity::Info;
    QString operatorMessage;      // safe to show an operator verbatim
    QString technicalDetail;      // for logs / support bundles
    QString affectedPath;         // file involved, when relevant
    qint64 sequence = -1;         // envelope seq involved, -1 = n/a
    bool recoverable = true;

    bool isError() const { return code != ReliabilityError::None; }
};

struct ReliabilityResult {
    bool ok = true;
    ErrorInfo error;

    static ReliabilityResult success()
    {
        return ReliabilityResult();
    }

    static ReliabilityResult failure(ReliabilityError code,
                                     const QString& operatorMessage,
                                     const QString& technicalDetail = QString(),
                                     const QString& affectedPath = QString(),
                                     qint64 sequence = -1)
    {
        ReliabilityResult r;
        r.ok = false;
        r.error.code = code;
        r.error.severity = defaultSeverity(code);
        r.error.operatorMessage = operatorMessage;
        r.error.technicalDetail = technicalDetail;
        r.error.affectedPath = affectedPath;
        r.error.sequence = sequence;
        r.error.recoverable = defaultRecoverable(code);
        return r;
    }

    static ReliabilityResult failureFrom(const ErrorInfo& info)
    {
        ReliabilityResult r;
        r.ok = false;
        r.error = info;
        return r;
    }
};

} // namespace rel
} // namespace ta

#endif // TA_REL_RELIABILITYRESULT_H
