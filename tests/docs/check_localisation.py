#!/usr/bin/env python3
"""Tech Aim / SETA — localisation gate (P0 WP1c).

Proves the German catalogue keeps up with the QML:

  LOC-GATE-001  the only untranslated catalogue entries are the vendored
                QModMaster forms, which are never shown;
  LOC-GATE-002  every QML file that uses a literal qsTr() is in the catalogue
                (a new file cannot ship without lupdate);
  LOC-GATE-003  every literal qsTr() source in the QML is in the catalogue,
                in that file's context, with a finished German translation
                (a changed or new string cannot ship without lupdate +
                translation);
  LOC-GATE-004  the report files carry no bare user-visible English literal
                (LOC-REPORT-001: reports translate at display time);
  LOC-GATE-005  no logic compares against translated text (QML-LANG-001);
  LOC-GATE-006  the compiled .qm is present and packaged in the qrc;
  LOC-GATE-007  report dates are formatted in the APP language
                (window.uiLocale()), never the Windows locale (Qt.locale("")).

A literal that logic compares stays English on purpose (QML-LANG-001); mark
such a line "// LOC-KEY" and translate it at display time.

Uses the existing Qt pipeline only (qsTr / lupdate / lrelease); it reads the
.ts and the QML as text and needs no Qt.

Run:  python tests/docs/check_localisation.py
"""
import html
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TS = "translations/techaim_de_DE.ts"
QM = "translations/techaim_de_DE.qm"

CHECKS = 0
FAILURES = 0


def check(ok, name, detail=""):
    global CHECKS, FAILURES
    CHECKS += 1
    if ok:
        print("PASS  %s" % name)
    else:
        FAILURES += 1
        print("FAIL  %s  %s" % (name, detail))
    sys.stdout.flush()


def read(rel):
    p = os.path.join(ROOT, rel)
    if not os.path.exists(p):
        return None
    return io.open(p, encoding="utf-8").read()


def read_qml(rel):
    s = read(rel)
    return None if s is None else strip_comments(s)


# Vendored QModMaster .ui forms: compiled in, never shown (mainWin->show() is
# commented out). They are deliberately left untranslated.
VENDORED_CONTEXTS = {"About", "BusMonitor", "MainWindow", "Settings",
                     "SettingsModbusRTU", "SettingsModbusTCP", "TachusWidget"}

# Report files converted under LOC-REPORT-001 - these must be fully clean.
REPORT_FILES = [
    "MatchReportView.qml", "SummaryReportView.qml", "MatchReportInfo.qml",
    "FinalsReportView.qml", "Finals10mReportView.qml",
    "CoachDashboardView.qml", "CoachDetailedView.qml", "CoachPrintView.qml",
    "TrainingReportView.qml", "CallDiagnoseReportView.qml",
    "PositionTransitionReportView.qml", "WindMapAnalysisView.qml",
    "RecoveryDialog.qml",
    # the shared Tech Aim report components
    "ReportHeader.qml", "ReportFooter.qml", "SectionTitle.qml", "MetricCard.qml",
]

# ── .ts parsing ──────────────────────────────────────────────────────────
def parse_ts(text):
    """{context: {source: (status, translation)}} plus {file basenames}."""
    out, files = {}, set()
    for cm in re.finditer(r"<context>\s*<name>([^<]+)</name>(.*?)</context>", text, re.S):
        ctx, body = cm.group(1), cm.group(2)
        msgs = out.setdefault(ctx, {})
        for mm in re.finditer(r"<message[^>]*>(.*?)</message>", body, re.S):
            msg = mm.group(1)
            for loc in re.finditer(r'<location filename="([^"]+)"', msg):
                files.add(loc.group(1).replace("../", "", 1))
            src = html.unescape(re.search(r"<source>(.*?)</source>", msg, re.S).group(1))
            tm = re.search(r'<translation(?: type="(\w+)")?>(.*?)</translation>', msg, re.S)
            status = tm.group(1) or "finished"
            msgs[src] = (status, html.unescape(tm.group(2)))
    return out, files


# ── QML scanning ─────────────────────────────────────────────────────────
ESC = {"n": "\n", "t": "\t", "r": "\r", '"': '"', "'": "'", "\\": "\\", "/": "/", "b": "\b", "f": "\f", "v": "\v", "0": "\0"}


def read_js_string(src, i):
    """src[i] is a quote. Returns (value, index after closing quote)."""
    q, i, out = src[i], i + 1, []
    while i < len(src):
        c = src[i]
        if c == "\\":
            n = src[i + 1]
            if n == "u":
                out.append(chr(int(src[i + 2:i + 6], 16))); i += 6; continue
            out.append(ESC.get(n, n)); i += 2; continue
        if c == q:
            return "".join(out), i + 1
        out.append(c); i += 1
    raise ValueError("unterminated string")


def strip_comments(src):
    """Blank // and /* */ comments (newlines kept) - lupdate ignores them,
    and commented-out code must not count. String-aware."""
    out, i, n = list(src), 0, len(src)
    while i < n:
        c = src[i]
        if c in "\"'`":
            q, i = c, i + 1
            while i < n and src[i] != q and src[i] != "\n":
                i += 2 if src[i] == "\\" else 1
            i += 1
        elif src.startswith("//", i):
            while i < n and src[i] != "\n":
                out[i] = " "; i += 1
        elif src.startswith("/*", i):
            end = src.find("*/", i + 2)
            end = n if end < 0 else end + 2
            for k in range(i, end):
                if out[k] != "\n":
                    out[k] = " "
            i = end
        else:
            i += 1
    return "".join(out)


def skip_ws(src, i):
    while i < len(src):
        if src[i] in " \t\r\n":
            i += 1
        elif src.startswith("//", i):
            i = src.index("\n", i) if "\n" in src[i:] else len(src)
        elif src.startswith("/*", i):
            i = src.index("*/", i) + 2
        else:
            break
    return i


def qstr_calls(src):
    """Yield (literal or None, span start, span end) for every qsTr( call.
    literal is the concatenation of a pure string-literal first argument;
    None when the argument is dynamic."""
    for m in re.finditer(r"\bqsTr\s*\(", src):
        start, i = m.start(), m.end()
        parts, ok = [], True
        i = skip_ws(src, i)
        while True:
            if i < len(src) and src[i] in "\"'":
                s, i = read_js_string(src, i); parts.append(s)
                i = skip_ws(src, i)
                if i < len(src) and src[i] == "+":
                    i = skip_ws(src, i + 1); continue
                if i < len(src) and src[i] in "),":
                    break
                ok = False; break
            ok = False; break
        # span to the matching close paren (string-aware)
        depth, j = 1, m.end()
        while j < len(src) and depth:
            c = src[j]
            if c in "\"'":
                _, j = read_js_string(src, j); continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        yield ("".join(parts) if ok and parts else None), start, j


DISPLAY = r"(?:text|title|label|subtitle|heading|caption|generatedText|footerText|headerText|placeholderText|tooltip|hint|note|unit|l|k|t|v|name|desc|description|sessionType|discipline|eventName|model|reportTitle|pageText|dateText|timeText|athlete|softwareVersion|extraLabel|extraValue)"
NEUTRAL = re.compile(r"^(?:[#\d.:%/\-+–—·• ×()\[\],|°²]*|mm|mm²|px|s|ms|min|X|Y|S\d*|R\d*|[A-Z]{1,2}\d*|Segoe UI|Consolas|Arial|Inter)$")
SKIP_LINE = re.compile(r"^\s*(?:import|//|\*)|console\.|appendToLogFile|QStringLiteral|case\s+\"|switch\s*\(|_mark\(")
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')

# (file, literal) pairs reviewed as data, not display text: engine/controller
# keys, journal values, PDF file names and the like.
BARE_ALLOW = {
    ("Finals10mReportView.qml", "10m Final"),       # fallback KEY passed to trName()
    ("FinalsReportView.qml", "DNS"),                # ISSF abbreviation, same in German
    ("MatchReportInfo.qml", ":  X: "),              # coordinate notation, same in German
    ("MatchReportInfo.qml", " mm; Y: "),
    ("SummaryReportView.qml", ":  X: "),
    ("SummaryReportView.qml", "; Y: "),
    ("RecoveryDialog.qml", "DSB "),                 # the federation's name, prefixed to a translated label
    ("RecoveryDialog.qml", "Training"),             # sessionKind journal value compared, not shown
    ("WindMapAnalysisView.qml", "summary"),         # page keys (k:); the label is the translated l:
    ("WindMapAnalysisView.qml", "compare"),
    ("WindMapAnalysisView.qml", "details"),
    ("WindMapAnalysisView.qml", "page open: "),     # performance trace mark, never displayed
    ("WindMapAnalysisView.qml", " (repeat)"),
    ("WindMapAnalysisView.qml", " (first)"),
    ("RecoveryDialog.qml", "DSB120"),               # programme ids chosen by the dialog, not shown
    ("RecoveryDialog.qml", "CALLDIAG"),
    ("RecoveryDialog.qml", "POSTRANS"),
    ("RecoveryDialog.qml", "WINDMAP"),
    ("RecoveryDialog.qml", "TRAINING"),
}

# A line whose source carries this marker returns an English KEY on purpose
# (logic compares it); a display-time lookup translates it. QML-LANG-001.
KEY_MARKER = "LOC-KEY"


def bare_literals(rel, src, raw=None):
    """User-visible literals outside every qsTr() call, line by line."""
    raw_lines = (raw or "").split("\n")
    blank = list(src)
    for _lit, a, b in qstr_calls(src):
        for k in range(a, b):
            if blank[k] != "\n":
                blank[k] = " "
    text = "".join(blank)
    found = []
    for n, line in enumerate(text.split("\n"), 1):
        if SKIP_LINE.search(line):
            continue
        if n - 1 < len(raw_lines) and KEY_MARKER in raw_lines[n - 1]:
            continue
        if not re.search(r"\b" + DISPLAY + r'\s*:|"\s*\+|\+\s*"|\breturn\s|\?\s*"', line):
            continue
        for m in LIT.finditer(line):
            s = m.group(1)
            # the value of a non-display property on a shared one-line element
            if re.search(r"(?:color|family|source|objectName|\bid|font\.\w+)\s*:\s*$", line[:m.start()]): continue
            # compared, so a logic key (QML-LANG-001), not display text
            if re.search(r"[!=]==?\s*$", line[:m.start()]) or re.match(r"\s*[!=]==?", line[m.end():]): continue
            if not re.search(r"[A-Za-zÄÖÜäöüß]", re.sub(r"\\.", "", s)): continue      # only escapes, e.g. "\n"
            if NEUTRAL.match(s.strip()): continue
            if re.match(r"^#[0-9A-Fa-f]{3,8}$", s): continue                             # colour
            if re.match(r"^[dMyhHmsapAPzt:.\-/ ,]+$", s) and re.search(r"yy|MM|dd|hh|HH|mm", s): continue  # date format
            direct = re.search(r"\b" + DISPLAY + r"\s*:\s*$", line[:m.start()])
            if re.match(r"^[a-z][A-Za-z0-9]*$", s) and not direct: continue             # a key, e.g. val("mpiXmm")
            if re.match(r"^[A-Z0-9_]+$", s) and "_" in s: continue
            if "/" in s and "." in s: continue
            if s.endswith((".qml", ".png", ".svg", ".wav", ".pdf", ".jsonl")): continue
            if (rel, s) in BARE_ALLOW: continue
            found.append("%d:%r" % (n, s))
    return found


def main():
    ts_text = read(TS)
    check(ts_text is not None, "LOC-GATE-000 German catalogue exists", TS)
    if ts_text is None:
        return
    cat, cat_files = parse_ts(ts_text)

    # 001 — open entries only in vendored contexts
    open_ctx = {}
    for ctx, msgs in cat.items():
        for src, (status, _tr) in msgs.items():
            if status == "unfinished":
                open_ctx.setdefault(ctx, []).append(src)
    stray = {c: v for c, v in open_ctx.items() if c not in VENDORED_CONTEXTS}
    check(not stray, "LOC-GATE-001 only vendored QModMaster forms are untranslated",
          "; ".join("%s: %s" % (c, v[:4]) for c, v in sorted(stray.items())))
    for ctx in sorted(VENDORED_CONTEXTS):
        check(ctx in cat, "LOC-GATE-001 vendored context %s is still catalogued (allowlist is not stale)" % ctx)

    # walk the product QML (repo root; tests and vendored code excluded)
    qml_files = sorted(f for f in os.listdir(ROOT) if f.endswith(".qml"))
    finished = lambda st: st not in ("unfinished", "vanished", "obsolete")
    for rel in qml_files:
        src = read_qml(rel)
        ctx = rel[:-4]
        literals = [l for l, _a, _b in qstr_calls(src) if l is not None]
        if not literals:
            continue
        # 002 — file covered
        check(rel in cat_files, "LOC-GATE-002 %s is in the catalogue" % rel, "run lupdate with this file")
        # 003 — every literal present + translated in this context
        msgs = cat.get(ctx, {})
        missing = sorted({l for l in literals if l not in msgs})
        untrans = sorted({l for l in literals if l in msgs and not finished(msgs[l][0])})
        check(not missing, "LOC-GATE-003 %s: every qsTr literal is in the catalogue" % rel,
              "missing %d: %s" % (len(missing), [m[:50] for m in missing[:5]]))
        check(not untrans, "LOC-GATE-003 %s: every qsTr literal has finished German" % rel,
              "untranslated %d: %s" % (len(untrans), [m[:50] for m in untrans[:5]]))

    # 004 — report files carry no bare user-visible literal
    for rel in REPORT_FILES:
        src = read_qml(rel)
        check(src is not None, "LOC-GATE-004 report file %s exists" % rel)
        if src is None:
            continue
        bare = bare_literals(rel, src, read(rel))
        check(not bare, "LOC-GATE-004 %s has no bare user-visible English literal" % rel,
              "%d: %s" % (len(bare), bare))
        # 007 — dates follow the APP language, not the Windows locale
        sys_loc = [n for n, line in enumerate(src.split("\n"), 1)
                   if re.search(r"Qt\.locale\(\s*(?:\"\")?\s*\)", line)]
        check(not sys_loc, "LOC-GATE-007 %s formats dates with window.uiLocale(), not the system locale" % rel,
              "lines %s" % sys_loc)

    # 005 — QML-LANG-001: no comparison against translated text
    cmp_hits = []
    for rel in qml_files:
        for n, line in enumerate(read(rel).split("\n"), 1):
            if re.search(r"[!=]==?\s*qsTr\s*\(|qsTr\s*\([^)]*\)\s*[!=]==?", line):
                cmp_hits.append("%s:%d" % (rel, n))
    check(not cmp_hits, "LOC-GATE-005 no logic compares against translated text (QML-LANG-001)", cmp_hits[:6])

    # 006 — compiled catalogue present and packaged
    check(os.path.exists(os.path.join(ROOT, QM)), "LOC-GATE-006 compiled catalogue %s exists" % QM)
    qrc = read("techaim_translations.qrc") or ""
    check("translations/techaim_de_DE.qm" in qrc, "LOC-GATE-006 the .qm is packaged in techaim_translations.qrc")


main()
print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
sys.exit(1 if FAILURES else 0)
