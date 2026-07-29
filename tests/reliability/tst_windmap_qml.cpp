// Wind Map QML — static source guards (Stage 5).
//
// SOURCE checks, in the spirit of tst_homepage_layout.cpp. The harness is
// QtCore-only and there is no input path on this machine, so these assert the
// structural rules that actually regress when someone edits the workflow —
// they are NOT a substitute for the human visual check recorded in
// docs/ui/UI_Defect_Register.md.
//
// The rules being guarded are the ones the Stage 5 brief states outright:
//   · QML must not construct domain events.
//   · QML must not convert m/s into hundredths by hand.
//   · Raw hundredths must never reach the UI.
//   · Wind Map must not offer sight-click or aiming advice.
//   · The programme must be reachable for 50m rifle only.
#include "test_support.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

namespace {

QDir repoRoot()
{
    QDir d(QString::fromLatin1(RELIABILITY_FIXTURES_DIR));
    d.cdUp(); d.cdUp(); d.cdUp();
    return d;
}

QString qmlSource(const char* name, bool* ok)
{
    QFile f(repoRoot().absolutePath() + QLatin1Char('/') + QLatin1String(name));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { *ok = false; return QString(); }
    *ok = true;
    const QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s;
}

// Strip // comments so prose describing a rule cannot satisfy a check about
// the code — the same reason tst_homepage_layout.cpp does it.
QString stripComments(const QString& src)
{
    QString out; out.reserve(src.size());
    for (const QString& line : src.split(QLatin1Char('\n'))) {
        const int i = line.indexOf(QStringLiteral("//"));
        out += (i >= 0 ? line.left(i) : line);
        out += QLatin1Char('\n');
    }
    return out;
}

} // namespace

void run_windmap_qml_tests()
{
    fputs("\n--- wind map QML source guards (stage 5) ---\n", stdout);

    bool okPanel = false, okHud = false, okLogin = false, okShoot = false, okMain = false;
    const QString panel = stripComments(qmlSource("WindMapRightPanel.qml", &okPanel));
    const QString hud   = stripComments(qmlSource("WindMapHud.qml", &okHud));
    const QString login = stripComments(qmlSource("LoginPage.qml", &okLogin));
    const QString shoot = stripComments(qmlSource("ShootingPage.qml", &okShoot));
    const QString mainq = stripComments(qmlSource("main.qml", &okMain));
    check(okPanel && okHud && okLogin && okShoot && okMain, "0. every Wind Map QML file was read");
    if (!(okPanel && okHud && okLogin && okShoot && okMain))
        return;

    const QString windMapQml = panel + hud;

    // ── 1. no domain events are constructed in QML ──────────────────────
    {
        const char* eventNames[] = {
            "WindMapSessionStarted", "WindConditionChanged", "WindMapSighterAccepted",
            "WindMapShotAccepted", "WindMapPositionChanged", "WindMapSessionCompleted",
            "WindMapPhaseChanged", "DomainEvent", "submit(",
        };
        bool clean = true;
        QString found;
        for (const char* n : eventNames)
            if (windMapQml.contains(QLatin1String(n))) { clean = false; found = QLatin1String(n); }
        check(clean, "1. the Wind Map QML constructs no domain event", found);
    }

    // ── 2. no hand-rolled unit conversion, no raw hundredths ────────────
    {
        check(!windMapQml.contains(QStringLiteral("hundredth"), Qt::CaseInsensitive),
              "2. hundredths never appear in the UI layer");
        // The two literals a hand-rolled m/s conversion would use.
        check(!windMapQml.contains(QStringLiteral("* 100"))
              && !windMapQml.contains(QStringLiteral("*100"))
              && !windMapQml.contains(QStringLiteral("/ 100"))
              && !windMapQml.contains(QStringLiteral("/100")),
              "2. QML performs no m/s <-> hundredths scaling of its own");
        check(windMapQml.contains(QStringLiteral("setMeasuredCondition"))
              && windMapQml.contains(QStringLiteral("m/s")),
              "2. speed is handed to the controller in m/s, the operator's unit");
    }

    // ── 3. the three condition states are all reachable ─────────────────
    {
        check(panel.contains(QStringLiteral("setMeasuredCondition"))
              && panel.contains(QStringLiteral("setCalmCondition"))
              && panel.contains(QStringLiteral("setNoReadingCondition")),
              "3. measured, calm and no-reading each have their own control");
        check(panel.contains(QStringLiteral("NO READING")),
              "3. 'no reading' is offered as itself, not as a blank measured entry");
    }

    // ── 4. no advice, correction or sight-click language ────────────────
    {
        // PHRASES, not single words — "Tech Aim" contains "aim", and an
        // athlete legitimately adjusts a note. These are the causal and
        // prescriptive constructions the wording rule prohibits.
        const char* prohibited[] = {
            "sight click", "sight clicks", "clicks left", "clicks right",
            "clicks up", "clicks down", "aim off", "hold off", "hold into",
            "you should", "we recommend", "recommended correction",
            "because the wind", "caused by the wind", "compensate for",
            "correct for the wind", "adjust your sights",
        };
        bool clean = true;
        QString found;
        for (const char* p : prohibited)
            if (windMapQml.contains(QLatin1String(p), Qt::CaseInsensitive)) {
                clean = false; found = QLatin1String(p);
            }
        check(clean, "4. no prescriptive or causal wording in the Wind Map UI", found);
        check(hud.contains(QStringLiteral("disclaimer")),
              "4. the review carries the training-only disclaimer");
    }

    // ── 5. reachability is 50m rifle only ───────────────────────────────
    {
        const int card = login.indexOf(QStringLiteral("WIND MAP"));
        check(card > 0, "5. the Training Lab catalogue offers Wind Map");
        if (card > 0) {
            // The card's `visible:` guard is the first line of its Rectangle,
            // above the title — scan back to that Rectangle, not a fixed span.
            const int rect = login.lastIndexOf(QStringLiteral("Rectangle {"), card);
            const QString block = rect >= 0 ? login.mid(rect, card - rect + 200) : QString();
            check(block.contains(QStringLiteral("gameMode === 1"))
                  && block.contains(QStringLiteral("gameRange === 50")),
                  "5. the card is shown for 50m RIFLE only");
        }
        check(login.contains(QStringLiteral("unsupportedDisciplineMessage")),
              "5. an unsupported discipline is reported, never coerced");
        check(login.contains(QStringLiteral("WINDMAP.configureSession")),
              "5. the controller decides availability — QML asks it");
    }

    // ── 6. it is a Training programme, never a competition one ──────────
    {
        check(login.contains(QStringLiteral("WINDMAP.startWindMap")),
              "6. Start routes Wind Map to its own controller");
        check(!windMapQml.contains(QStringLiteral("QUAL."))
              && !windMapQml.contains(QStringLiteral("FINALS3P."))
              && !windMapQml.contains(QStringLiteral("FINALS10M.")),
              "6. the Wind Map UI touches no competition controller");
        check(shoot.contains(QStringLiteral("WINDMAP.registerShot")),
              "6. shots route to WINDMAP while a Wind Map session is active");
        check(shoot.contains(QStringLiteral("isWindMapMatch = true"))
              && shoot.contains(QStringLiteral("isFinalsMatch = false")),
              "6. entering Wind Map mode clears every competition mode flag");
    }

    // ── 7. recovery is dispatched to the Wind Map owner ─────────────────
    {
        check(mainq.contains(QStringLiteral("\"WINDMAP\""))
              && mainq.contains(QStringLiteral("restoreWindMapSession")),
              "7. the recovery dispatcher routes WINDMAP to its own restorer");
        check(shoot.contains(QStringLiteral("WINDMAP.resumeFromRecovery")),
              "7. the restorer resumes through the controller");
        check(shoot.contains(QStringLiteral("recoveredMaxExternalId")),
              "7. the resume carries the duplicate guard past recovered shots");
    }

    // ── 8. no PDF or analytics were smuggled into this stage ────────────
    {
        check(!windMapQml.contains(QStringLiteral("exportPdf"))
              && !windMapQml.contains(QStringLiteral("CUSTOMPRINT"))
              && !windMapQml.contains(QStringLiteral("PDFEXPORT")),
              "8. Stage 5 produces no PDF export");
        check(!windMapQml.contains(QStringLiteral("COACHREPORT")),
              "8. Stage 5 wires in no analytics engine");
    }
}
