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

    bool okAnalysis = false, okPlot = false;
    const QString analysis = stripComments(qmlSource("WindMapAnalysisView.qml", &okAnalysis));
    const QString plot     = stripComments(qmlSource("WindMapTargetPlot.qml", &okPlot));
    check(okAnalysis && okPlot, "0. the analysis view and target plot were read");
    const QString windMapQml = panel + hud + bar + analysis + plot;

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

    // ── 8. no metric is computed outside the engine ─────────────────────
    {
        // Stage 6.1 adds an Export PDF ACTION; the PDF itself is Stage 6.2.
        // What must hold now and after it lands: QML never calculates a report
        // value and never reaches a second analytics engine.
        check(!windMapQml.contains(QStringLiteral("COACHREPORT")),
              "8. the Wind Map UI wires in no other analytics engine");
        check(!windMapQml.contains(QStringLiteral("CUSTOMPRINT.create")),
              "8. no PDF is generated from QML-side calculations");
        check(!hud.contains(QStringLiteral("exportPdf")),
              "8. the capture HUD offers no export — the analysis owns it");
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
        // This asserted the literal "!isFinals10mMatch && !isTrainingMatch".
        // The Tech Aim 1.0 convergence renamed the training half of that gate
        // to isTrainingModeAny, an UMBRELLA covering every Training Lab
        // programme rather than the single one isTrainingMatch named. The
        // assertion was therefore pinning an implementation detail - a
        // variable name - and not the rule it exists to defend.
        //
        // The rule is that competition chrome stays hidden in Finals and in
        // Training, and it is not merely preserved but STRENGTHENED: more
        // training modes are now excluded, not fewer. Either spelling
        // satisfies it; a gate that names neither does not.
        check(shoot.contains(QStringLiteral("!isFinals10mMatch && !isTrainingModeAny"))
              || shoot.contains(QStringLiteral("!isFinals10mMatch && !isTrainingMatch")),
              "10. the existing Finals/Training gates are preserved, not replaced");
        bool okF3 = false, okF10 = false;
        const QString f3  = stripComments(qmlSource("FinalsHud.qml", &okF3));
        const QString f10 = stripComments(qmlSource("Finals10mHud.qml", &okF10));
        check(okF3 && okF10, "10. the Finals HUDs were read");
        if (okF3 && okF10)
            check(!f3.contains(QStringLiteral("WINDMAP")) && !f10.contains(QStringLiteral("WINDMAP")),
                  "10. no Wind Map binding leaked into a Finals screen");
    }

    // ── 11. Stage 6.1: the analysis view RECALCULATES NOTHING ───────────
    {
        check(analysis.contains(QStringLiteral("analysisModel()")),
              "11. the view reads the engine-backed model");
        // Arithmetic that would mean a metric was recomputed in QML. Formatting
        // (toFixed, the plot's own pixel mapping) is fine; deriving a statistic
        // is not.
        // COMPUTATION patterns only. The plain-language definitions the
        // redesign added legitimately contain the word "average" ("Average
        // distance of each shot from the group centre") — banning the word
        // would fail on a definition while missing an actual calculation.
        const char* recompute[] = {
            "Math.sqrt", "Math.pow", "sum +=", "total +=", "mean =", "stdDev =",
            "variance =", "/ count", "/ n)",
        };
        bool clean = true; QString found;
        for (const char* r : recompute)
            if (analysis.contains(QLatin1String(r))) { clean = false; found += QLatin1String(r) + QStringLiteral(" "); }
        check(clean, "11. no metric is recalculated in the analysis view", found);

        // A withheld metric must be gated on its has* flag, never printed raw.
        check(analysis.contains(QStringLiteral("function metric(")),
              "11. metrics go through one flag-checked formatter");
        check(analysis.contains(QStringLiteral("hasDispersion")) && analysis.contains(QStringLiteral("hasMpi")),
              "11. the view honours the engine's sample-threshold flags");

        // Stage 6.1.1: THREE primary pages, not five.
        const char* pages[] = { "SUMMARY", "COMPARE CONDITIONS", "SHOT DETAILS" };
        bool all = true; QString missing;
        for (const char* sec : pages)
            if (!analysis.contains(QLatin1String(sec))) { all = false; missing += QLatin1String(sec); }
        check(all, "11. the three primary pages are present", missing);
        // The five-tab set the redesign replaced must be gone.
        check(!analysis.contains(QStringLiteral("Target Plot"))
              && !analysis.contains(QStringLiteral("2 · CONDITION-COLOURED TARGET PLOT")),
              "11. UI-WIND-004: the old five-pill navigation is gone");
        check(analysis.contains(QStringLiteral("Session Overview"))
              && analysis.contains(QStringLiteral("filters the page below")),
              "11. the position control is labelled as a FILTER, not a second page row");
        check(analysis.contains(QStringLiteral("New Wind Map session"))
              && analysis.contains(QStringLiteral("Home")),
              "11. New session and Home are offered");
        check(shoot.contains(QStringLiteral("WINDMAP.phase === 6")),
              "11. the analysis replaces the capture summary once complete");

        // Wording: descriptive, never an instruction. The Stage 6.1.1 redesign
        // states this in plain language rather than as a section heading.
        check(analysis.contains(QStringLiteral("It is a record, not an aiming instruction"))
              || analysis.contains(QStringLiteral("observations, not instructions")),
              "11. the plot is described as a record, not an instruction");
        const char* banned[] = { "correction", "hold off", "aim off", "sight adjustment",
                                 "sight click", "aim at", "hold left", "hold right" };
        bool wordsOk = true; QString bad;
        for (const char* b : banned)
            if (analysis.contains(QLatin1String(b), Qt::CaseInsensitive)) { wordsOk = false; bad += QLatin1String(b); }
        check(wordsOk, "11. no correction / hold / aim-off / sight-adjustment wording", bad);
    }

    // ── 12. UI-WIND-002: every used type must be IMPORTED ───────────────
    {
        // The defect this exists to stop recurring: WindMapAnalysisView.qml
        // used ScrollBar while importing only QtQuick. The type never
        // resolved, the whole component failed to instantiate, and a completed
        // session showed only the capture HUD's basic review.
        //
        // Every §1-§11 check still PASSED, because a file that never loads
        // still contains all the right strings. A static string check cannot
        // see this; an import-coverage check can.
        struct File { const char* name; const QString* src; };
        const File files[] = {
            { "WindMapAnalysisView.qml", &analysis },
            { "WindMapHud.qml",          &hud },
            { "WindMapRightPanel.qml",   &panel },
            { "TrainingTopBar.qml",      &bar },
        };
        // Types that live in QtQuick.Controls, not QtQuick.
        const char* controlsTypes[] = {
            "ScrollBar", "ScrollView", "Button", "CheckBox", "ComboBox", "Slider",
            "TextField", "TextArea", "SpinBox", "TabBar", "TabButton", "Switch",
            "RadioButton", "ProgressBar", "ToolTip", "Menu", "Popup", "Dialog",
        };
        bool allCovered = true;
        QString gaps;
        for (const File& f : files) {
            const bool importsControls = f.src->contains(QStringLiteral("import QtQuick.Controls"));
            for (const char* t : controlsTypes) {
                // Match a type USE: the name followed by '{' or '.'.
                const QString name = QLatin1String(t);
                const bool used = f.src->contains(name + QStringLiteral(" {"))
                               || f.src->contains(name + QStringLiteral("."));
                if (used && !importsControls) {
                    allCovered = false;
                    gaps += QStringLiteral("%1 uses %2 without importing QtQuick.Controls; ")
                                .arg(QLatin1String(f.name), name);
                }
            }
        }
        check(allCovered,
              "12. every QtQuick.Controls type used is backed by its import", gaps);

        // The specific regression, named so a failure is unmistakable.
        check(!analysis.contains(QStringLiteral("ScrollBar"))
              || analysis.contains(QStringLiteral("import QtQuick.Controls")),
              "12. UI-WIND-002: the analysis view imports QtQuick.Controls for ScrollBar");

        // Stage 6.1.1 replaced five section pills with THREE pages and one
        // position filter (UI-WIND-004).
        const char* navPages[] = { "SUMMARY", "COMPARE CONDITIONS", "SHOT DETAILS" };
        bool nav = true;
        QString missingNav;
        for (const char* n : navPages)
            if (!analysis.contains(QLatin1String(n))) { nav = false; missingNav += QLatin1String(n); }
        check(nav, "12. the three navigation pages are present", missingNav);
        check(analysis.contains(QStringLiteral("Session Overview")),
              "12. 3P offers Session Overview inside the position filter");
        check(analysis.contains(QStringLiteral("sampleNote")),
              "12. an insufficient sample explains itself in words");
        check(analysis.contains(QStringLiteral("Not enough shots to report a centre")),
              "12. a group below threshold says so instead of showing a number");
    }

    // ── 13. Stage 6.1.1: the UX redesign ────────────────────────────────
    {
        // UI-WIND-008: the PDF control must not be an enabled primary action.
        check(analysis.contains(QStringLiteral("PDF — COMING NEXT")),
              "13. UI-WIND-008: the PDF control says it is not implemented yet");
        check(!analysis.contains(QStringLiteral("exportPdfRequested")),
              "13. UI-WIND-008: there is no export signal to fire");
        // Scoped to the Wind Map block only: Position Transition legitimately
        // handles its OWN onExportPdfRequested, and that PDF is implemented.
        {
            const int wmAt = shoot.indexOf(QStringLiteral("WindMapAnalysisView {"));
            check(wmAt > 0, "13. the Wind Map analysis view is instantiated");
            if (wmAt > 0) {
                const int close = shoot.indexOf(QStringLiteral("\n    }"), wmAt);
                const QString block = shoot.mid(wmAt, (close > wmAt ? close - wmAt : 500));
                check(!block.contains(QStringLiteral("onExportPdfRequested")),
                      "13. UI-WIND-008: the Wind Map view handles no export signal");
            }
        }
        // It must carry no MouseArea — a disabled control that still responds
        // is the same lie in a different colour.
        const int pdfAt = analysis.indexOf(QStringLiteral("PDF — COMING NEXT"));
        check(pdfAt > 0, "13. the PDF control is addressable");
        if (pdfAt > 0) {
            const QString after = analysis.mid(pdfAt, 320);
            check(!after.contains(QStringLiteral("MouseArea")),
                  "13. the disabled PDF control is not clickable");
        }

        // UI-WIND-003: a loading state, and lazy pages.
        check(analysis.contains(QStringLiteral("Preparing your Wind Map analysis")),
              "13. UI-WIND-003: an explained loading state exists");
        check(analysis.contains(QStringLiteral("Loader")),
              "13. UI-WIND-003: pages are behind a Loader, not all built at once");
        check(analysis.contains(QStringLiteral("ListView")),
              "13. UI-WIND-003: shot details use a virtualised ListView");
        check(analysis.contains(QStringLiteral("model: {"))
              && analysis.indexOf(QStringLiteral("ListView")) < analysis.indexOf(QStringLiteral("delegate: Column")),
              "13. the shot list is a ListView with a delegate, not a flat Repeater");

        // UI-WIND-005: the target graphic.
        check(plot.contains(QStringLiteral("HIGH")) && plot.contains(QStringLiteral("LOW"))
              && plot.contains(QStringLiteral("LEFT")) && plot.contains(QStringLiteral("RIGHT")),
              "13. UI-WIND-005: the plot labels its orientation");
        check(plot.contains(QStringLiteral("mm")),
              "13. UI-WIND-005: the plot carries a millimetre scale marker");
        check(plot.contains(QStringLiteral("recomputeSpan")),
              "13. UI-WIND-005: the plot span is computed on data change, not per binding");
        check(analysis.contains(QStringLiteral("LEGEND"))
              && analysis.contains(QStringLiteral("Hollow ring — sighter"))
              && analysis.contains(QStringLiteral("Cross — that condition's group centre"))
              && analysis.contains(QStringLiteral("White circle — reference centre"))
              && analysis.contains(QStringLiteral("Faint circle — average distance")),
              "13. UI-WIND-005: every marker style is defined in the legend");
        check(!plot.contains(QStringLiteral("arrow")) && !plot.contains(QStringLiteral("Arrow")),
              "13. UI-WIND-005: there is no aiming arrow");

        // UI-WIND-007: plain language before technical values.
        check(analysis.contains(QStringLiteral("WHAT HAPPENED"))
              && analysis.contains(QStringLiteral("WHAT THIS MEANS"))
              && analysis.contains(QStringLiteral("NEXT TRAINING STEP"))
              && analysis.contains(QStringLiteral("EVIDENCE")),
              "13. UI-WIND-007: the summary answers what/meaning/next/evidence");
        check(analysis.contains(QStringLiteral("SHOW TECHNICAL MEASUREMENTS")),
              "13. UI-WIND-007: technical values are behind an expander");
        check(analysis.contains(QStringLiteral("Average centre of the recorded shot group"))
              && analysis.contains(QStringLiteral("Average distance of each shot from the group centre"))
              && analysis.contains(QStringLiteral("Distance between the two widest shots"))
              && analysis.contains(QStringLiteral("Total left-to-right width"))
              && analysis.contains(QStringLiteral("Total high-to-low height")),
              "13. UI-WIND-007: every technical label has a plain definition");
        check(analysis.contains(QStringLiteral("function acrossWords"))
              && analysis.contains(QStringLiteral("function upWords")),
              "13. UI-WIND-007: coordinates are translated to right/left and high/low");
        // The words themselves, not a signed coordinate. Built from parts so
        // the quoting stays readable.
        const QString rightWord = QStringLiteral("qsTr(") + QLatin1Char('"')
                                + QStringLiteral("right") + QLatin1Char('"') + QStringLiteral(")");
        const QString leftWord  = QStringLiteral("qsTr(") + QLatin1Char('"')
                                + QStringLiteral("left")  + QLatin1Char('"') + QStringLiteral(")");
        check(analysis.contains(rightWord) && analysis.contains(leftWord),
              "13. UI-WIND-007: the words right and left are used, not signed numbers");

        // UI-WIND-006: scoped VERDICTS (Stage 6.1.3 replaced findings).
        check(analysis.contains(QStringLiteral("scopeIsSession")),
              "13. UI-WIND-006: the view reads each verdict's scope");
        check(analysis.contains(QStringLiteral("function verdictsForScope")),
              "13. UI-WIND-006: verdicts are filtered by the selected position");
        check(analysis.contains(QStringLiteral("scopeLabel")),
              "13. UI-WIND-006: the scope is displayed");

        // Stage 6.1.3: the view renders the VERDICT model and composes no
        // verdict text of its own.
        check(analysis.contains(QStringLiteral("model.verdicts")),
              "13. the view reads the verdict model");
        check(analysis.contains(QStringLiteral("function primaryVerdict"))
              && analysis.contains(QStringLiteral("function secondaryVerdicts")),
              "13. one prioritised primary verdict, the rest secondary");
        check(analysis.contains(QStringLiteral("function verdictForCondition")),
              "13. each condition card can carry its OWN verdict");
        // Every athlete-facing sentence comes from the engine.
        const char* verdictFields[] = { ".headline", ".interpretation",
                                        ".nextTrainingStep", ".coachDecision",
                                        ".evidenceExplanation" };
        bool fromEngine = true;
        QString missingField;
        for (const char* f : verdictFields)
            if (!analysis.contains(QLatin1String(f))) {
                fromEngine = false; missingField += QLatin1String(f);
            }
        check(fromEngine, "13. what happened / meaning / next step / coach / evidence "
                          "all come from the verdict record", missingField);
        check(analysis.contains(QStringLiteral("COACH DECISION")),
              "13. the coach-decision block exists");

        // Relative wind direction, derived and optional.
        check(analysis.contains(QStringLiteral("hasRelativeWind"))
              && analysis.contains(QStringLiteral("relativeWind")),
              "13. the relative wind description is shown where available");
        check(analysis.contains(QStringLiteral("relativeWindNote")),
              "13. and the unavailable fallback is shown where it is not");

        // Overview tiles reduced to four.
        check(analysis.contains(QStringLiteral("SESSION DETAILS")),
              "13. the secondary counts moved into an expandable details panel");
        check(analysis.contains(QStringLiteral("COUNTED SHOTS"))
              && analysis.contains(QStringLiteral("CONDITIONS USED"))
              && analysis.contains(QStringLiteral("DATA QUALITY")),
              "13. no more than four prominent evidence values");

        // Still no metric computed in QML, and still no prescriptive wording.
        const char* recompute[] = { "Math.sqrt", "Math.pow", "stdDev =", "variance" };
        bool clean = true; QString found;
        for (const char* r : recompute)
            if (analysis.contains(QLatin1String(r)) || plot.contains(QLatin1String(r))) {
                clean = false; found += QLatin1String(r);
            }
        check(clean, "13. the redesign still recalculates no metric", found);
        const char* banned[] = { "correction", "hold off", "aim off", "sight adjustment",
                                 "sight click", "aim at", "hold left", "hold right" };
        bool wordsOk = true; QString bad;
        for (const char* b : banned)
            if (analysis.contains(QLatin1String(b), Qt::CaseInsensitive)
                || plot.contains(QLatin1String(b), Qt::CaseInsensitive)) {
                wordsOk = false; bad += QLatin1String(b);
            }
        check(wordsOk, "13. no prescriptive wording survived the redesign", bad);
    }
}
