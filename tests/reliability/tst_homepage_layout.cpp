// Homepage layout and selection-state regressions (UI-HOME-001..010).
//
// These are SOURCE checks. The harness is QtCore-only by design and there is
// no input path on this machine (endpoint security blocks injection), so they
// assert the structural properties that actually regress when someone edits
// the page — an anchor pair reappearing, a Flickable reverting to a ScrollView,
// a second derivation of the selected event creeping back in.
//
// What they CANNOT prove is recorded honestly in docs/ui/UI_Defect_Register.md
// §4 and requires human visual evidence.
#include "test_support.h"

#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace {

QString homepageSource(bool* ok)
{
    QDir d(QString::fromLatin1(RELIABILITY_FIXTURES_DIR));
    d.cdUp(); d.cdUp(); d.cdUp();          // -> repo root
    QFile f(d.absolutePath() + QStringLiteral("/LoginPage.qml"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { *ok = false; return QString(); }
    *ok = true;
    const QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s;
}

// Strip // comments so prose describing a defect cannot satisfy — or fail — a
// check about the code.
QString stripComments(const QString& src)
{
    QString out; out.reserve(src.size());
    const QStringList lines = src.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const int i = line.indexOf(QStringLiteral("//"));
        out += (i >= 0 ? line.left(i) : line);
        out += QLatin1Char('\n');
    }
    return out;
}

// Return the body of a QML block introduced by `marker`, up to `chars`.
QString blockAfter(const QString& src, const QString& marker, int chars = 900)
{
    const int i = src.indexOf(marker);
    if (i < 0) return QString();
    return src.mid(i, chars);
}

} // namespace

void run_homepage_layout_tests()
{
    std::printf("--- homepage layout + selection state (UI-HOME-001..010) ---\n");

    bool ok = false;
    const QString raw = homepageSource(&ok);
    check(ok && !raw.isEmpty(), "homepage: LoginPage.qml is readable");
    if (!ok) return;
    const QString src = stripComments(raw);

    // ── UI-HOME-001 / UI-HOME-010: action-bar region separation ─────────────
    {
        const QString bar = blockAfter(src, QStringLiteral("id: actionRow"), 400);
        check(!bar.isEmpty(), "UI-HOME-001: the action row exists");

        // The defect: the row carried BOTH horizontal anchors, so it spanned
        // the whole bar and drew over the readiness text.
        const bool hasLeft  = bar.contains(QStringLiteral("anchors.left:"));
        const bool hasRight = bar.contains(QStringLiteral("anchors.right:"));
        check(!(hasLeft && hasRight),
              "UI-HOME-001: the action row is not anchored to BOTH edges (the overlap cause)");
        check(hasRight, "UI-HOME-010: the action row is right-anchored");
        check(bar.contains(QStringLiteral("width: 210 + 12 + 268")),
              "UI-HOME-010: the action row has a fixed width so the left region can be derived");
        check(bar.contains(QStringLiteral("height: 56")),
              "UI-HOME-001: the action row is as tall as its 56 px children");

        // The readiness region must size itself from that fixed width, not
        // from a live anchor to the row.
        const QString readiness = blockAfter(src, QStringLiteral("id: readinessBlock"), 400);
        check(!readiness.isEmpty(), "UI-HOME-010: the readiness region exists");
        check(readiness.contains(QStringLiteral("actionBar.width - actionRow.width")),
              "UI-HOME-001: the readiness width is derived from the action row's fixed width");
        check(readiness.contains(QStringLiteral("Math.max(0,")),
              "UI-HOME-001: the readiness width cannot go negative");
        check(readiness.contains(QStringLiteral("elide: Text.ElideRight")),
              "UI-HOME-010: readiness text elides rather than overflowing");
    }

    // ── UI-HOME-002: event panel scrolling ──────────────────────────────────
    {
        const int idx = src.indexOf(QStringLiteral("id: eventScroll"));
        check(idx > 0, "UI-HOME-002: the event scroll container exists");
        const QString ev = src.mid(qMax(0, idx - 200), 1000);

        check(ev.contains(QStringLiteral("Flickable")),
              "UI-HOME-002: the event list is a Flickable (ScrollView mis-measured its content)");
        check(ev.contains(QStringLiteral("contentHeight: eventColumn.height")),
              "UI-HOME-002: contentHeight is bound explicitly to the content");
        check(ev.contains(QStringLiteral("contentWidth: width")),
              "UI-HOME-002: content width is pinned — no horizontal scrolling");
        check(ev.contains(QStringLiteral("clip: true")),
              "UI-HOME-002: content is clipped to the panel");
        check(ev.contains(QStringLiteral("ScrollBar.vertical")),
              "UI-HOME-002: a vertical scrollbar is provided");
        check(ev.contains(QStringLiteral("ScrollBar.AlwaysOn")),
              "UI-HOME-002: the scrollbar is shown while content overflows");
        check(src.contains(QStringLiteral("bottomPadding: 20")),
              "UI-HOME-002: the final card has bottom padding and is fully reachable");
        check(!ev.contains(QStringLiteral("ScrollBar.horizontal.policy")),
              "UI-HOME-002: no horizontal scrollbar is configured");
    }

    // ── UI-HOME-003: one authoritative selected-event state ─────────────────
    {
        for (const char* fn : { "function selectedProgrammeKind()",
                                "function selectedProgrammeName()",
                                "function selectedProgrammeLabel()",
                                "function startButtonText()" }) {
            check(src.contains(QString::fromLatin1(fn)),
                  QString(QStringLiteral("UI-HOME-003: %1 exists"))
                      .arg(QString::fromLatin1(fn).mid(9)));
        }

        // Every consumer must call the single source rather than re-derive.
        check(src.contains(QStringLiteral("text: selectedProgrammeName()")),
              "UI-HOME-003: the Selected Profile summary uses the single source");
        check(src.contains(QStringLiteral("text: selectedProgrammeLabel()")),
              "UI-HOME-003: the summary heading uses the single source");
        check(src.contains(QStringLiteral("text: startButtonText()")),
              "UI-HOME-003: the Start button wording uses the single source");
        check(src.contains(QStringLiteral("selectedProgrammeName()"))
              && src.contains(QStringLiteral("readinessSummary()")),
              "UI-HOME-003: the action-bar recap uses the single source");

        // The exact defect: the summary was hardcoded to "<discipline> — ISSF"
        // for every event, so Open Practice claimed to be an ISSF match.
        check(!src.contains(QStringLiteral("trainingConfirmed ? \"Technical Blocks\" : getDisciplineName() + \" — ISSF\"")),
              "UI-HOME-003: the hardcoded '<discipline> - ISSF' summary is gone");
        check(src.contains(QStringLiteral("Open Practice")),
              "UI-HOME-003: practice is labelled as practice, not as ISSF");

        // Every homepage event kind must be represented.
        for (const char* kind : { "POSTRANS", "CALLDIAG", "TRAINING",
                                  "FINAL", "OFFICIAL", "PRACTICE" }) {
            check(src.contains(QString::fromLatin1(kind)),
                  QString(QStringLiteral("UI-HOME-003: event kind %1 is handled"))
                      .arg(QString::fromLatin1(kind)));
        }

        // Presentation only — the controller dispatch must be untouched.
        for (const char* ctrl : { "POSTRANS.startPositionTransition",
                                  "CALLDIAG.startCallDiagnose",
                                  "TRAINING.startTraining" }) {
            check(src.contains(QString::fromLatin1(ctrl)),
                  QString(QStringLiteral("UI-HOME-003: controller dispatch %1 is unchanged"))
                      .arg(QString::fromLatin1(ctrl)));
        }
    }

    // ── UI-HOME-004: network share validity ─────────────────────────────────
    {
        check(src.contains(QStringLiteral("shareIncomplete"))
              && src.contains(QStringLiteral("shareConfigured")),
              "UI-HOME-004: share-validity state exists");
        // Enabled must require a folder — the toggle derives from one.
        check(src.contains(QStringLiteral("property bool netEnabled: netowrk_path_text.text !== \"\"")),
              "UI-HOME-004: sharing starts off unless a destination folder exists");
        // A success label may never be shown without a folder.
        check(src.contains(QStringLiteral("shareConfigured ? qsTr(\"Share enabled\")")),
              "UI-HOME-004: 'Share enabled' is gated on a configured share");
        check(src.contains(QStringLiteral("shareIncomplete ? qsTr(\"Share incomplete\")")),
              "UI-HOME-004: the on-but-unconfigured case reads as incomplete, not success");
        // The footer must agree with the card.
        check(src.contains(QStringLiteral("Share incomplete\"")),
              "UI-HOME-004: the footer reports an incomplete share");
        check(src.contains(QStringLiteral("readinessSummary")),
              "UI-HOME-004: an incomplete share is surfaced in the readiness line");
        // ...and must not block shooting.
        check(src.contains(QStringLiteral("readonly property bool readinessOk: !shareIncomplete")),
              "UI-HOME-004: an incomplete share is advisory and does not gate Start");
        check(src.contains(QStringLiteral("networkFolderDialog.open()")),
              "UI-HOME-004: an explicit choose-folder action exists");
    }

    // ── UI-HOME-005: status not needlessly repeated ─────────────────────────
    {
        const QString hdr = blockAfter(src, QStringLiteral("id: headerBar"), 900);
        check(!hdr.contains(QStringLiteral("_modeBadge")),
              "UI-HOME-005: the read-only LIVE/DEMO badge is gone from the page heading");
        // Two indicators must remain: the mode CONTROL and the footer strip.
        // Demo mode staying unmissable is a result-integrity requirement.
        check(src.contains(QStringLiteral("DEMO / SIMULATION")),
              "UI-HOME-005: the operating-mode control still states the mode");
        check(src.contains(QStringLiteral("text: appMode ? \"LIVE\" : \"DEMO\"")),
              "UI-HOME-005: the footer still states the mode");
    }

    // ── UI-HOME-006: one primary logo ───────────────────────────────────────
    {
        const QString hdr = blockAfter(src, QStringLiteral("id: headerBar"), 900);
        check(!hdr.contains(QStringLiteral("theme.logoWhite")),
              "UI-HOME-006: the duplicate logo image is gone from the page header");
        check(!src.contains(QStringLiteral("theme.logoWhite")),
              "UI-HOME-006: the homepage renders no logo of its own");
    }

    // ── UI-HOME-007: consistent selection indicator ─────────────────────────
    {
        // The card body runs to ~3.4k chars (badge, titles, preset repeater),
        // and the indicator is declared after it.
        const QString op = blockAfter(src, QStringLiteral("id: openPracticeCard"), 4200);
        check(!op.isEmpty(), "UI-HOME-007: the Open Practice card is addressable");
        check(op.contains(QStringLiteral("width: 18; height: 18; radius: 9")),
              "UI-HOME-007: Open Practice uses the same radio indicator as every EventCard");
        check(op.contains(QStringLiteral("openPracticeCard.selected")),
              "UI-HOME-007: the indicator reflects the card's selected state");
    }

    // ── UI-HOME-008: no text below the readable floor ───────────────────────
    {
        const QRegularExpression tiny(QStringLiteral("pixelSize: [1-9]\\b"));
        auto it = tiny.globalMatch(src);
        QStringList found;
        while (it.hasNext()) found << it.next().captured(0);
        check(found.isEmpty(),
              QString(QStringLiteral("UI-HOME-008: no sub-10px text remains (found %1)"))
                  .arg(found.join(QStringLiteral(", "))));
        check(src.contains(QStringLiteral("theme.type.helperText.size")),
              "UI-HOME-008: helper text uses the typography role");
    }

    // ── UI-HOME-009: Open Practice expansion is proportionate ───────────────
    {
        check(src.contains(QStringLiteral("height: selected ? 78 + 48 + 8 : 78")),
              "UI-HOME-009: collapsed matches other cards (78); expanded adds only the preset row");
        check(!src.contains(QStringLiteral("height: selected ? 148 : 78")),
              "UI-HOME-009: the oversized 148 px expansion is gone");
    }

    // ── UI-HOME-006: no duplicate identity row in the page header ───────────
    {
        const QString hdr = blockAfter(src, QStringLiteral("id: headerBar"), 700);
        check(!hdr.isEmpty(), "UI-HOME-006: the page header exists");
        check(!hdr.contains(QStringLiteral("ELECTRONIC TARGET CONTROL")),
              "UI-HOME-006: the duplicated identity row is gone from the page header");
        check(hdr.contains(QStringLiteral("height: 56")),
              "UI-HOME-006: the page header is the reduced 56 px height");
    }

    // ── UI-HOME-007: four labelled event groups ─────────────────────────────
    {
        for (const char* g : { "OFFICIAL ISSF MATCH", "FINALS", "TRAINING LAB", "PRACTICE" })
            check(src.contains(QString::fromLatin1(g)),
                  QString(QStringLiteral("UI-HOME-007: group %1 is labelled"))
                      .arg(QString::fromLatin1(g)));
        check(src.contains(QStringLiteral("practiceView = 1")),
              "UI-HOME-007: the Training Lab arrow is genuine navigation");
    }

    // ── UI-HOME-008: the action bar uses typography roles, not raw sizes ─────
    {
        const QString bar = blockAfter(src, QStringLiteral("id: readinessBlock"), 700);
        check(bar.contains(QStringLiteral("theme.type.")),
              "UI-HOME-008: the readiness region uses typography roles");
    }

    // ── layout invariants at the supported sizes ────────────────────────────
    {
        // The primary action must live OUTSIDE every scroll container, which is
        // what makes it un-clippable at any window height.
        const int setupIdx  = src.indexOf(QStringLiteral("id: setupScroll"));
        const int barIdx    = src.indexOf(QStringLiteral("id: actionBar"));
        const int startIdx  = src.indexOf(QStringLiteral("id: startSessionRect"));
        const int panelEnd  = src.indexOf(QStringLiteral("} // leftPanel"));
        check(setupIdx > 0 && barIdx > panelEnd,
              "layout: the action bar is declared outside the scrollable left panel");
        check(startIdx > barIdx,
              "layout: the primary Start action lives inside the action bar");

        // Panels stop at the action bar, so the bar can never be overlaid.
        check(src.contains(QStringLiteral("anchors.bottom: actionBar.top")),
              "layout: the panels stop at the action bar rather than under it");
        // Minimum widths are declared in the design system, not invented here.
        check(src.contains(QStringLiteral("Math.floor(parent.width * 0.44)")),
              "layout: the event panel keeps the greater share of the width");
    }
}
