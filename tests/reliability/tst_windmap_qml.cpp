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
    bool okBar = false;
    const QString panel = stripComments(qmlSource("WindMapRightPanel.qml", &okPanel));
    const QString hud   = stripComments(qmlSource("WindMapHud.qml", &okHud));
    const QString bar   = stripComments(qmlSource("TrainingTopBar.qml", &okBar));
    const QString login = stripComments(qmlSource("LoginPage.qml", &okLogin));
    const QString shoot = stripComments(qmlSource("ShootingPage.qml", &okShoot));
    const QString mainq = stripComments(qmlSource("main.qml", &okMain));
    check(okPanel && okHud && okBar && okLogin && okShoot && okMain,
          "0. every Wind Map QML file was read");
    if (!(okPanel && okHud && okBar && okLogin && okShoot && okMain))
        return;

    const QString windMapQml = panel + hud + bar;

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

    // ── 9. UI-WIND-001: no Finals shell anywhere in Wind Map ────────────
    {
        // 9a. The Wind Map screens name no Finals artefact at all.
        const char* finalsArtefacts[] = {
            "FINAL 35", "FINAL 24", "CEREMONY", "Ceremony", "ceremony",
            "FINALS3P", "FINALS10M", "Finals3P", "Finals10m",
            "isFinalsMatch", "isFinals10mMatch",
            "currentmatchDisplay", "currentGameDisplay", "matchShootCount",
            "globalMatchModel", "finalsSeriesIndex", "stageId", "skipCeremony",
        };
        bool clean = true;
        QString found;
        for (const char* a : finalsArtefacts)
            if (windMapQml.contains(QLatin1String(a))) { clean = false; found += QLatin1String(a) + QStringLiteral(" "); }
        check(clean, "9a. no Wind Map screen contains or binds to any Finals artefact", found);

        // 9b. The SHARED training bar reads no controller at all — every value
        //     is passed in by whichever programme owns the screen. That is what
        //     makes one bar safe for four programmes.
        const char* anyCtl[] = { "FINALS3P.", "FINALS10M.", "QUAL.", "WINDMAP.",
                                 "TRAINING.", "CALLDIAG.", "POSTRANS.", "MODREADER." };
        bool noCtl = true;
        QString ctlFound;
        for (const char* c : anyCtl)
            if (bar.contains(QLatin1String(c))) { noCtl = false; ctlFound += QLatin1String(c); }
        check(noCtl, "9b. the shared training bar reads NO controller directly", ctlFound);
        check(shoot.contains(QStringLiteral("TrainingTopBar {")),
              "9b. ShootingPage supplies its values");

        // 9c. Every competition row in statusStrip is gated OFF for Wind Map.
        //     These are the exact three rows that produced the defect.
        const int strip = shoot.indexOf(QStringLiteral("id: statusStrip"));
        check(strip > 0, "9c. statusStrip is addressable");
        if (strip > 0) {
            // The band ends where the training bar is declared.
            const int barDecl = shoot.indexOf(QStringLiteral("TrainingTopBar {"), strip);
            check(barDecl > strip, "9c. TrainingTopBar occupies the statusStrip band");
            const QString band = shoot.mid(strip, barDecl - strip);
            // UI-TRAIN-001..003: ONE boundary, asked once, for all four
            // programmes. Every competition row in the band must be gated on
            // isTrainingModeAny — not on a per-programme !isXMatch term, which
            // is exactly how the four gates drifted apart in the first place.
            const int gates = band.count(QStringLiteral("!isTrainingModeAny"));
            check(gates >= 6,
                  "9c. every competition row is gated on the SHARED boundary",
                  QStringLiteral("found %1 gates").arg(gates));
            check(!band.contains(QStringLiteral("!isWindMapMatch")),
                  "9c. no per-programme gate remains in the band to drift again");
            // The official counter must be inside a gated row.
            const int counter = band.indexOf(QStringLiteral("globalMatchModel.count"));
            check(counter > 0 && band.lastIndexOf(QStringLiteral("visible: !isTrainingModeAny"), counter) > 0,
                  "9c. the official 0/N shot counter is inside a Training-gated row");
        }

        // 9d. The training bar shows the required Training identity instead.
        check(bar.contains(QStringLiteral("TRAINING LAB")),
              "9d. the bar identifies the screen as Training Lab");
        check(bar.contains(QStringLiteral("programmeName")) && bar.contains(QStringLiteral("phaseLabel"))
              && bar.contains(QStringLiteral("progressValue")) && bar.contains(QStringLiteral("positionName")),
              "9d. programme, phase, position and progress are all supplied");
        // All four programmes name themselves through the shared bar.
        const char* programmes[] = { "Wind Map", "Position Transition",
                                     "Call & Diagnose", "Technical Blocks" };
        bool allNamed = true;
        QString missing;
        for (const char* pr : programmes)
            if (!shoot.contains(QLatin1String(pr))) { allNamed = false; missing += QLatin1String(pr); }
        check(allNamed, "9d. every Training Lab programme names itself in the bar", missing);
        check(shoot.contains(QStringLiteral("WINDMAP.countedShots"))
              && shoot.contains(QStringLiteral("POSTRANS.shotsCompleted")),
              "9d. progress comes from each programme's OWN controller");
        check(bar.contains(QStringLiteral("NOT AN OFFICIAL COMPETITION RESULT")),
              "9d. the capture screen states it is not an official result");

        // 9e. UI-TRAIN-001..003: every Training Lab programme is isolated from
        //     inherited competition presentation state.
        {
            check(shoot.contains(QStringLiteral("isTrainingModeAny: isTrainingMatch || isCallDiagnoseMatch")),
                  "9e. the shared boundary covers Technical Blocks and Call & Diagnose");
            check(shoot.contains(QStringLiteral("isPositionTransitionMatch || isWindMapMatch")),
                  "9e. and Position Transition and Wind Map");
            // The three artefacts the defect reported, each reachable ONLY
            // from a row the shared boundary now gates.
            const char* competitionArtefacts[] = {
                "currentGameDisplay1", "currentmatchDisplay", "matchShootCount",
                "globalMatchModel.count",
            };
            const int strip2 = shoot.indexOf(QStringLiteral("id: statusStrip"));
            const int barDecl2 = shoot.indexOf(QStringLiteral("TrainingTopBar {"), strip2);
            const QString band2 = (strip2 > 0 && barDecl2 > strip2)
                                  ? shoot.mid(strip2, barDecl2 - strip2) : QString();
            bool allGated = true;
            QString ungated;
            for (const char* art : competitionArtefacts) {
                const int at = band2.indexOf(QLatin1String(art));
                if (at < 0) continue;                 // not in the band at all
                if (band2.lastIndexOf(QStringLiteral("!isTrainingModeAny"), at) < 0) {
                    allGated = false; ungated += QLatin1String(art) + QStringLiteral(" ");
                }
            }
            check(allGated,
                  "9e. every competition artefact in the band sits behind the shared gate",
                  ungated);
            // And the bar itself carries none of them.
            bool barClean = true;
            for (const char* art : competitionArtefacts)
                if (bar.contains(QLatin1String(art))) barClean = false;
            check(barClean, "9e. the training bar contains no competition artefact");
            check(bar.contains(QStringLiteral("NOT AN OFFICIAL COMPETITION RESULT")),
                  "9e. every Training capture screen states it is not an official result");
        }
    }

    // ── 10. the Finals screens themselves are untouched ─────────────────
    {
        // The finals components keep their own gates; Stage 5.1 must not have
        // widened or weakened them.
        check(shoot.contains(QStringLiteral("visible: isFinalsMatch"))
              && shoot.contains(QStringLiteral("visible: isFinals10mMatch")),
              "10. FinalsHud / Finals10mHud keep their original gates");
        check(shoot.contains(QStringLiteral("!isFinals10mMatch && !isTrainingMatch")),
              "10. the existing Finals/Training gates are preserved, not replaced");
        bool okF3 = false, okF10 = false;
        const QString f3  = stripComments(qmlSource("FinalsHud.qml", &okF3));
        const QString f10 = stripComments(qmlSource("Finals10mHud.qml", &okF10));
        check(okF3 && okF10, "10. the Finals HUDs were read");
        if (okF3 && okF10)
            check(!f3.contains(QStringLiteral("WINDMAP")) && !f10.contains(QStringLiteral("WINDMAP")),
                  "10. no Wind Map binding leaked into a Finals screen");
    }
}
