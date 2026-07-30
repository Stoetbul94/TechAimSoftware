#!/usr/bin/env python3
"""Tech Aim - Training Lab evidence governance checks.

Proves the Training Lab cannot quietly acquire a coaching diagnosis its data
does not support: every athlete-facing claim is classified, every threshold has
a recorded origin, research support is only claimed where a source was actually
read, product rules are labelled honestly, coach approval cannot be invented,
and conclusions are composed centrally rather than re-authored in QML.

Policy: docs/research/training-lab-evidence-standard.md

Run:  python tests/docs/check_training_lab_evidence.py
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

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


def ascii_safe(s, limit=110):
    """Console-safe detail text - the register is full of dashes and symbols."""
    return s.encode("ascii", "replace").decode("ascii")[:limit]


def flat(s):
    """Collapse whitespace so a prose search is not defeated by line wrapping."""
    return re.sub(r"\s+", " ", s.replace("**", ""))


_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
_LINE_COMMENT = re.compile(r"//[^\n]*")
_LITERAL = re.compile(r'"(?:[^"\\\n]|\\.)*"')


def athlete_strings(src):
    """The text this source can actually SHOW someone.

    Comments are stripped first — a comment that names a prohibited claim in
    order to forbid it must not read as the claim itself. Adjacent string
    literals are then joined, because a caveat wrapped across three source
    lines is still one sentence to the reader.
    """
    if not src:
        return ""
    body = _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", src))
    out, pos = [], 0
    run = []
    for m in _LITERAL.finditer(body):
        between = body[pos:m.start()]
        if run and between.strip() != "":
            out.append("".join(run))
            run = []
        run.append(m.group(0)[1:-1].replace('\\"', '"').replace("\\n", " "))
        pos = m.end()
    if run:
        out.append("".join(run))
    return "\n".join(out)


print("--- Tech Aim Training Lab evidence governance ---")

STANDARD = "docs/research/training-lab-evidence-standard.md"
REGISTER = "docs/research/training-lab-evidence-register.md"

# Every IMPLEMENTED programme, and the evidence document it must have.
PROGRAMME_DOCS = {
    "Technical Blocks":     "docs/research/technical-blocks-evidence.md",
    "Call & Diagnose":      "docs/research/call-and-diagnose-evidence.md",
    "Group Pattern Coach":  "docs/research/group-pattern-coach-evidence.md",
    "Position Transition":  "docs/research/position-transition-evidence.md",
    "Wind Map":             "docs/research/wind-map-feedback-evidence.md",
}

CLASSIFICATIONS = ("RESEARCH-SUPPORTED", "REASONED PRODUCT RULE",
                   "COACH-APPROVED PRODUCT RULE", "FUTURE VALIDATION REQUIRED")

# ---- 1. every implemented programme has an evidence document --------------
standard = read(STANDARD)
register = read(REGISTER)
check(standard is not None, "the shared evidence standard exists", STANDARD)
check(register is not None, "the central evidence register exists", REGISTER)
if standard is None or register is None:
    print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
    sys.exit(1)

flat_standard = flat(standard)

progdocs = {}
for prog, rel in PROGRAMME_DOCS.items():
    body = read(rel)
    progdocs[prog] = body
    check(body is not None, "%s has an evidence document" % prog, rel)
    check(prog in standard, "the standard governs %s" % prog)

# ---- 2. the standard states the four classifications and the limitations --
for label in CLASSIFICATIONS:
    check(label in standard, "the standard defines %s" % label)

# The eight shared limitations, and the two that matter most, stated verbatim.
check("cannot determine the exact technical cause" in flat_standard,
      "the standard records that impact data cannot determine technical cause")
check("athlete and coach decisions" in flat_standard,
      "the standard reserves sight/technique changes to athlete and coach")
check("Sample size and data quality" in flat_standard,
      "the standard requires sample size to be visible")
check("never described as scientifically proven" in flat_standard,
      "the standard forbids calling a product rule scientifically proven")
check("association is never described as causation" in flat_standard,
      "the standard forbids describing association as causation")

# ---- 3. the shared feedback model ----------------------------------------
for field in ("WHAT HAPPENED", "EVIDENCE", "WHAT IT MAY MEAN",
              "NEXT TRAINING STEP", "COACH DECISION", "LIMITATIONS"):
    check(field in standard, "the shared feedback model defines %s" % field)

# ---- 4. parse the register's claim entries -------------------------------
# An entry is "#### ID - title" followed by a field table.
entries = {}
order = []
for m in re.finditer(r"^####\s+([A-Z]{2}-\d{2})\s+.*$", register, re.M):
    cid = m.group(1)
    start = m.end()
    nxt = register.find("\n####", start)
    nxt2 = register.find("\n### ", start)
    ends = [e for e in (nxt, nxt2) if e != -1]
    end = min(ends) if ends else len(register)
    body = register[start:end]
    fields = {}
    for row in re.finditer(r"^\|\s*\*\*(.+?)\*\*\s*\|(.*?)\|\s*$", body, re.M):
        fields[row.group(1).strip()] = row.group(2).strip()
    entries[cid] = fields
    order.append(cid)

check(len(entries) >= 30, "the register holds the full claim set",
      "found %d entries" % len(entries))

# Every field name named by the governance brief appears in the register.
FIELD_VOCAB = ["Programme", "Feature / metric", "Athlete-facing claim",
               "Classification", "Source", "Source verification",
               "Population studied", "Discipline studied", "Limitation",
               "Tech Aim implementation", "Prohibited interpretation",
               "Threshold source", "Coach review", "UI location",
               "Report / PDF", "Test reference", "Status"]
for f in FIELD_VOCAB:
    check(any(f in e for e in entries.values()),
          "the register uses the field '%s'" % f)

# Fields required on EVERY entry.
CORE = ["Programme", "Feature / metric", "Athlete-facing claim", "Classification",
        "Source", "Source verification", "Population studied", "Discipline studied",
        "Limitation", "Tech Aim implementation", "Prohibited interpretation",
        "Threshold source", "Coach review", "Status"]

EMPTY = ("", "-", "--", "?", "tbd", "TBD", "todo", "TODO")

for cid in order:
    f = entries[cid]
    missing = [c for c in CORE if c not in f]
    check(not missing, "%s carries every core register field" % cid,
          "missing %s" % ascii_safe(", ".join(missing)))
    for c in CORE:
        if c in f:
            check(f[c].strip() not in EMPTY, "%s has a value for '%s'" % (cid, c))

# ---- 5. every athlete-facing claim has a classification ------------------
for cid in order:
    cls = entries[cid].get("Classification", "")
    hit = [c for c in CLASSIFICATIONS if c in cls]
    check(len(hit) >= 1, "%s carries a recognised classification" % cid,
          ascii_safe(cls))

# ---- 6. every threshold has a recorded origin ----------------------------
for cid in order:
    ts = entries[cid].get("Threshold source", "")
    # Either the origin is named, or the entry states plainly that it has no
    # threshold. What is never acceptable is a bare value with no provenance.
    low = ts.lower()
    ok = any(k in low for k in ("decision", "no threshold", "convention", "n/a",
                                "none", "classified in", "inherited"))
    check(ok, "%s records where its threshold came from" % cid, ascii_safe(ts))

# ---- 7. RESEARCH-SUPPORTED requires a source that was actually READ ------
# Parse the source list: "### S5 - ..." block, find its verification status.
sources = {}
for m in re.finditer(r"^###\s+(S\d+|M\d+)\s+", register, re.M):
    sid = m.group(1)
    start = m.end()
    nxt = register.find("\n### ", start)
    nxt2 = register.find("\n---", start)
    ends = [e for e in (nxt, nxt2) if e != -1]
    end = min(ends) if ends else len(register)
    sources[sid] = register[start:end]

check(len(sources) >= 10, "the register lists the source set",
      "found %d" % len(sources))

read_sources = set(s for s, b in sources.items() if "VERIFIED — RECORD READ" in b)
index_only = set(s for s, b in sources.items() if "VERIFIED — INDEX ONLY" in b)
check(len(read_sources) >= 8, "most sources were read, not merely indexed",
      "record-read: %d" % len(read_sources))
check(len(read_sources & index_only) == 0,
      "no source claims two different verification statuses")

for cid in order:
    if "RESEARCH-SUPPORTED" not in entries[cid].get("Classification", ""):
        continue
    src = entries[cid].get("Source", "")
    cited = set(re.findall(r"\bS\d+\b", src))
    check(bool(cited), "%s (research-supported) names its sources" % cid,
          ascii_safe(src))
    for s in cited:
        check(s in read_sources,
              "%s rests on %s, whose record was actually read" % (cid, s),
              "%s is not VERIFIED - RECORD READ" % s)

# An INDEX-ONLY source must never carry a sample size or effect size claim.
for sid in index_only:
    body = sources[sid]
    check("no sample size" in body.lower() or "no effect size" in body.lower()
          or "existence" in body.lower(),
          "%s (index only) states that no figures are claimed from it" % sid)

# ---- 8. product rules are labelled honestly ------------------------------
OVERCLAIM = ("scientifically proven", "clinically proven", "research-validated",
             "evidence-based", "proven to improve", "ISSF rule requires")
for cid in order:
    cls = entries[cid].get("Classification", "")
    if "RESEARCH-SUPPORTED" in cls:
        continue
    claim = entries[cid].get("Athlete-facing claim", "").lower()
    bad = [w for w in OVERCLAIM if w in claim]
    check(not bad, "%s does not dress a product rule as research" % cid,
          ascii_safe(", ".join(bad)))

# ---- 9. coach approval cannot be invented --------------------------------
check("No coach review has taken place" in register,
      "the register states plainly that no coach review has happened")
check("zero COACH-APPROVED PRODUCT RULES" in register
      or "0 COACH-APPROVED" in register or "| **COACH-APPROVED PRODUCT RULES** | 0 |" in register
      or "COACH-APPROVED PRODUCT RULES | **0**" in register,
      "the register records zero coach-approved rules")
approved = [c for c in order
            if "COACH-APPROVED" in entries[c].get("Classification", "")]
check(not approved, "no claim is classified COACH-APPROVED without a review",
      ascii_safe(", ".join(approved)))

# The coach-review table must not contain a row that looks like an approval
# without a named reviewer and a date.
crs = register.split("## Coach review", 1)
check(len(crs) == 2, "the register has a coach-review section")
if len(crs) == 2:
    coach = crs[1]
    for row in re.finditer(r"^\|(.+)\|\s*$", coach, re.M):
        cells = [c.strip() for c in row.group(1).split("|")]
        if len(cells) < 8:
            continue
        if cells[0].startswith("Reviewer") or set(cells[0]) <= set("-: "):
            continue
        named = cells[0] not in ("*(none)*", "", "-", "—")
        dated = cells[2] not in ("", "-", "—")
        outcome = cells[6].upper() if len(cells) > 6 else ""
        if "ACCEPT" in outcome:
            check(named and dated,
                  "a recorded coach approval names a reviewer and a date",
                  ascii_safe(row.group(1)))
    check("Do not invent coach approval" in standard,
          "the standard forbids inventing coach approval")

# ---- 10. no impact-only pattern produces an unsupported diagnosis --------
# Scan the Training Lab sources and their QML for cause claims and sight advice.
TRAINING_SRC = []
srcdir = os.path.join(ROOT, "src", "training")
for fn in sorted(os.listdir(srcdir)):
    if fn.endswith((".cpp", ".h")):
        TRAINING_SRC.append(os.path.join("src", "training", fn))
TRAINING_QML = ["TrainingHud.qml", "TrainingReportView.qml", "TrainingRightPanel.qml",
                "TrainingTopBar.qml", "CallDiagnoseHud.qml", "CallDiagnoseReportView.qml",
                "CallDiagnoseRightPanel.qml", "PositionTransitionHud.qml",
                "PositionTransitionReportView.qml", "PositionTransitionRightPanel.qml",
                "WindMapHud.qml", "WindMapRightPanel.qml", "WindMapAnalysisView.qml",
                "WindMapTargetPlot.qml"]

PROHIBITED = [
    r"caused by", r"due to your", r"because you ", r"move your sights",
    r"adjust your sights", r"breathing error", r"trigger snatch", r"you flinch",
    r"shoulder pressure is", r"your technique is", r"poor technique",
    r"indicates a fault", r"proves that you",
]
for rel in TRAINING_SRC + TRAINING_QML:
    body = read(rel)
    if body is None:
        continue
    low = body.lower()
    bad = [p for p in PROHIBITED if re.search(p, low)]
    check(not bad, "%s makes no prohibited diagnosis" % rel,
          ascii_safe(", ".join(bad)))

# Fatigue may never be asserted from Training Lab data.
for rel in TRAINING_SRC + TRAINING_QML:
    body = read(rel)
    if body is None:
        continue
    check("fatigue" not in body.lower(), "%s asserts no fatigue claim" % rel)

# Evaluative judgements of the athlete's group are prohibited (Wind Map's
# 40 mm rule was explicitly written to avoid them).
JUDGEMENT = [r"technically poor", r"a poor group", r"bad group",
             r"weak group", r"inadequate group", r"unacceptable group"]
for rel in TRAINING_SRC + TRAINING_QML:
    body = read(rel)
    if body is None:
        continue
    low = body.lower()
    bad = [p for p in JUDGEMENT if re.search(p, low)]
    check(not bad, "%s passes no judgement on the athlete's group" % rel,
          ascii_safe(", ".join(bad)))

# ---- 11. conclusions are sourced centrally, not re-authored in QML -------
# The mandatory caveats live in C++. Where one is also present in QML it is a
# recorded defect (EVID-PT-001) and the two copies must remain identical.
CAVEATS = {
    "CD-04": ("This describes a difference in shot perception. It does not "
              "indicate that the sights or the shots should be moved.",
              "src/training/CallDiagnoseController.cpp"),
    "PT-05": ("Positions have different stability demands — compare each "
              "position to itself across repeats, not against another "
              "position.",
              "src/training/PositionTransitionController.h"),
    "PT-08": ("Measured setup and early-group data. It does not identify the "
              "technical cause.",
              "src/training/PositionTransitionController.h"),
    "GP-disclaimer": ("This describes the measured group shape. It does not "
                      "identify the technical cause.",
                      "src/training/TrainingProgramController.cpp"),
}
for cid, (text, owner) in CAVEATS.items():
    strs = athlete_strings(read(owner))
    check(text in strs,
          "%s caveat is composed in C++ (%s)" % (cid, owner))

# EVID-PT-001 is FIXED: no caveat may be re-authored in QML at all. PT-05 was
# the duplicated one; it now binds to POSTRANS.crossPositionCaveat.
for cid in CAVEATS:
    text = CAVEATS[cid][0]
    dupes = [q for q in TRAINING_QML if text in athlete_strings(read(q))]
    check(not dupes, "%s conclusion is not re-authored in QML" % cid,
          ascii_safe(", ".join(dupes)))

# ...and the report view must actually consume the central property, not just
# have dropped the sentence.
ptview = read("PositionTransitionReportView.qml") or ""
ptview_strings = athlete_strings(ptview)
check("POSTRANS.crossPositionCaveat" in ptview,
      "the Position Transition report binds the central cross-position caveat")

# ---- 12. future programmes require an evidence review -------------------
FUTURE = ["First Shot & Re-entry", "Consistency Chain", "aim-trace",
          "SCATT", "Shadow Shooting", "personalised diagnostics"]
for f in FUTURE:
    check(f in flat_standard, "the standard gates the future programme '%s'" % f)
check("evidence review" in flat_standard,
      "the standard requires an evidence review before implementation")
check("before its controller ships" in flat_standard,
      "the standard requires a programme document before code ships")

# ---- 13. per-programme documents are honest -----------------------------
for prog, rel in PROGRAMME_DOCS.items():
    body = progdocs[prog]
    if body is None:
        continue
    check("Threshold" in body or "threshold" in body,
          "%s document records its thresholds" % prog)
    check(any(c in body for c in CLASSIFICATIONS),
          "%s document classifies its claims" % prog)
    check("Coach" in body, "%s document records coach-review status" % prog)

# The four programmes audited in this phase carry a full audit output.
for prog in ("Technical Blocks", "Call & Diagnose", "Group Pattern Coach",
             "Position Transition"):
    body = progdocs[prog]
    for heading in ("Research verification", "Test gaps", "Summary"):
        check(heading in body, "%s document has a '%s' section" % (prog, heading))
    check("Product decision" in body or "product decision" in body,
          "%s document names its thresholds as product decisions" % prog)

# No programme document may claim a threshold is research-derived.
for prog, rel in PROGRAMME_DOCS.items():
    body = progdocs[prog] or ""
    check("research-derived" not in body.lower().replace(
              "none is research-derived", ""),
          "%s claims no research-derived threshold" % prog)

# ---- 13b. MANDATORY CAVEATS (Phase 1, Part 4) ---------------------------
# Each of these protects a boundary the user named explicitly. They assert
# against the SHIPPING SOURCE, not against documentation, so a future edit
# that reintroduces a claim fails the build rather than a review.

# The corpus is what the software can SHOW an athlete: string literals with
# comments stripped and wrapped literals rejoined. Scanning raw source instead
# would flag the comments that exist precisely to forbid these claims.
def src_all(paths):
    return "\n".join(athlete_strings(read(p)) for p in paths)

training_all = src_all(TRAINING_SRC)
qml_all = src_all(TRAINING_QML)
both = training_all + "\n" + qml_all

# 1. impact patterns never claim a specific technical cause
CAUSE_WORDS = ["breathing", "trigger snatch", "flinch", "shoulder pressure",
               "natural point of aim was", "follow-through error"]
for w in CAUSE_WORDS:
    check(w not in both.lower(),
          "caveat: no impact pattern names '%s' as a cause" % w)
check("It does not identify the technical cause" in training_all,
      "caveat: the group-shape disclaimer states the limit explicitly")

# 2. performance decline never becomes fatigue
check("fatigue" not in both.lower(),
      "caveat: no Training Lab surface asserts fatigue from score movement")

# 3/4. transition duration and first-shot speed are not graded
pt_src = athlete_strings(read("src/training/PositionTransitionController.cpp"))
check("not automatically better" in pt_src and "not automatically worse" in pt_src,
      "caveat: a shorter ready-to-first-shot time is not called better, nor a "
      "longer one worse")
check("fully settled" not in pt_src.lower(),
      "caveat: the settling wording that implied a cause is gone")
check("cannot show whether they are related" in pt_src,
      "caveat: co-occurring timing and dispersion facts are not linked")

# 5. call bias never produces sight-adjustment advice
cd_src = athlete_strings(read("src/training/CallDiagnoseController.cpp"))
check("does not indicate that the sights or the shots should be moved" in cd_src,
      "caveat: the call-bias statement carries its no-sight-change boundary")
for phrase in ("move your sights", "adjust your sights", "click left", "click right",
               "come up two", "sight correction"):
    check(phrase not in both.lower(),
          "caveat: no sight-adjustment instruction ('%s')" % phrase)

# 6. vertical stringing diagnoses nothing
gp_src = athlete_strings(read("src/training/GroupPatternAnalyzer.cpp"))
check("Vertical spread dominant" in gp_src,
      "caveat: vertical stringing is named as a measured spread")
vert_ctx = gp_src[max(0, gp_src.find("Vertical spread dominant") - 1200):
                  gp_src.find("Vertical spread dominant") + 1200].lower()
for w in ("breathing", "trigger", "elevation error"):
    check(w not in vert_ctx,
          "caveat: vertical stringing does not diagnose '%s'" % w)

# 7. position differences are never attributed to wind alone
wm_src = athlete_strings(read("src/training/WindMapVerdict.cpp"))
check("cannot be attributed to wind alone" in wm_src,
      "caveat: the 3P position difference is not blamed on wind")
check("does not establish a cause" in wm_src,
      "caveat: every Wind Map verdict carries the no-cause limitation")

# 8. product rules are never called research-validated
for w in ("research-validated", "scientifically proven", "clinically proven",
          "evidence-based rule", "proven to improve"):
    check(w not in both.lower(),
          "caveat: no Training Lab source calls a product rule '%s'" % w)
check("REASONED PRODUCT RULE" in (read("src/training/WindMapVerdict.h") or ""),
      "caveat: the Wind Map thresholds are labelled as reasoned product rules "
      "in the code that implements them")

# 9. no coach-approved classification without a recorded reviewer
#    (register-side checked above; source side asserts nothing claims approval)
for w in ("coach-approved", "approved by a coach", "coach approved"):
    check(w not in both.lower(),
          "caveat: no source claims coach approval ('%s')" % w)

# 10. QML duplicates no central caveat text — asserted at 11 above, restated
#     here as the named Part 4 requirement so a reader finds it.
check("POSTRANS.crossPositionCaveat" in ptview
      and CAVEATS["PT-05"][0] not in ptview_strings,
      "caveat: QML consumes the central caveat and holds no copy of its own")

# 11. the UI model and the future PDF model carry the same verdict identity.
#     Wind Map composes every verdict field in WindMapVerdict and the
#     controller projects them; NO field may be assembled in QML.
wm_ctl = read("src/training/WindMapController.cpp") or ""
for field in ("verdictId", "evidence", "headline", "observedPattern",
              "interpretation", "nextTrainingStep", "coachDecision", "limitations"):
    check(field in wm_ctl,
          "caveat: the analysis model projects the central verdict field '%s'" % field)
wm_view = read("WindMapAnalysisView.qml") or ""
check("verdictId" not in wm_view or "v.verdictId" in wm_view,
      "caveat: the view reads verdict ids rather than minting them")
for composed in ("headline =", "interpretation =", "nextTrainingStep ="):
    check(composed not in wm_view,
          "caveat: the Wind Map view assigns no verdict text of its own ('%s')"
          % composed)

# The analytics version must be projected, so a report can never be shown as
# corrected when it was produced by the superseded classification.
check("analyticsVersion" in wm_ctl and "windmap-analytics-v2" in
      (read("src/training/WindMapAnalytics.h") or ""),
      "caveat: every analysis is stamped with the method that produced it")

# ---- 14. the register's open defects are visible ------------------------
for did in ("EVID-WM-001", "EVID-GEN-001", "EVID-GEN-002", "EVID-PT-001"):
    check(did in register, "defect %s is recorded" % did)
check("OPEN" in register, "the register shows open defects rather than closing them")

# ---- 15. the coach review pack, and its empty-by-design decision rows ----
pack = read("docs/research/training-lab-coach-review-pack.md")
check(pack is not None, "the coach review pack exists")
if pack:
    for rule in ("TB-05", "GP-01", "PT-02", "PT-06", "CD-02", "Wind Map"):
        check(rule in pack, "the pack presents %s for review" % rule)
    for field in ("Current rule", "Athlete-facing wording", "Classification",
                  "Verified research support", "Limitation", "Proposed wording",
                  "Test examples", "Reviewer name",
                  "Qualification or relevant experience", "Review date", "Notes"):
        check(field in pack, "the pack records '%s' for each rule" % field)
    check("Accept / Accept provisionally / Reject / Change" in pack,
          "the pack offers the full decision vocabulary")
    check("No coach approval has been recorded. No name has been entered." in pack,
          "the pack states plainly that no approval exists yet")
    # No name may be pre-filled into a decision row. Every decision table row
    # of the form "| Reviewer name | X |" must have an EMPTY X.
    prefilled = [m.group(1).strip() for m in
                 re.finditer(r"^\|\s*Reviewer name\s*\|(.*?)\|\s*$", pack, re.M)]
    check(all(v == "" for v in prefilled),
          "no reviewer name is pre-filled in the pack",
          ascii_safe(", ".join(v for v in prefilled if v)))
    dates = [m.group(1).strip() for m in
             re.finditer(r"^\|\s*Review date\s*\|(.*?)\|\s*$", pack, re.M)]
    check(all(v == "" for v in dates), "no review date is pre-filled in the pack")
    # Named individuals must not appear as approvers anywhere in the pack.
    check("Laferla" not in pack,
          "no coach is named as having approved anything")

# ---- 16. the dispersion correction is specified and honest ---------------
disp = read("docs/training-lab-wind-map-dispersion.md")
check(disp is not None, "the dispersion specification exists")
if disp:
    check("radialRmsMm = sqrt( SUM( (x - meanX)^2 + (y - meanY)^2 ) / (n - 1) )"
          in disp, "the exact formula is documented")
    check("millimetres" in disp and "n - 1" in disp,
          "units and denominator are documented")
    check("kMinSamplesDispersion" in disp,
          "the minimum sample requirement is documented")
    check("windmap-analytics-v2" in disp,
          "the analytics version is documented")
    # The d2 overclaim must be corrected, not quietly deleted.
    check("misapplied" in disp.lower(),
          "the superseded d2 illustration is explicitly corrected, not dropped")
    check("one-dimensional" in disp and "two-dimensional" in disp,
          "the 1-D vs 2-D distinction is stated")
    check("display only" in disp.lower(),
          "extreme spread is recorded as display-only")
# ...and the register carries the same correction, so the two cannot diverge.
check("misapplied" in register.lower(),
      "the register records the d2 correction too")
check("windmap-analytics-v2" in register,
      "the register records the corrected analytics version")

print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
sys.exit(1 if FAILURES else 0)
