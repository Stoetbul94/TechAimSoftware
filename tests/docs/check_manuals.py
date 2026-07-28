# -*- coding: utf-8 -*-
"""Documentation checks for the Tech Aim manuals (P0 Phase J).

Structural and factual checks only — deliberately NOT assertions about
paragraph wording, which would break on every edit. Run from anywhere:

    python tests/docs/check_manuals.py

Exit code 0 = all checks passed.
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAN = os.path.join(ROOT, "docs", "manual")

checks = 0
failures = []


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        failures.append(label)
    print(("PASS  " if ok else "FAIL  ") + label)


def read(name):
    with io.open(os.path.join(MAN, name), encoding="utf-8") as fh:
        return fh.read()


EN_DOCS = [
    "TechAim_Quick_Start_EN.md",
    "TechAim_Operator_Manual_EN.md",
    "TechAim_Troubleshooting_EN.md",
]
DE_DOCS = [
    "TechAim_Quick_Start_DE.md",
    "TechAim_Operator_Manual_DE.md",
    "TechAim_Troubleshooting_DE.md",
]
SUPPORT_DOCS = [
    "TechAim_German_Translation_Status.md",
    "TechAim_Manual_Screenshot_Register.md",
    "TechAim_Manual_Validation_Checklist.md",
    "TechAim_Manual_Review_Findings.md",
]
ALL_DOCS = EN_DOCS + DE_DOCS + SUPPORT_DOCS


def main():
    print("=== Tech Aim documentation checks ===")

    # --- every document exists -------------------------------------------
    for d in ALL_DOCS:
        check(os.path.isfile(os.path.join(MAN, d)), "exists: %s" % d)
    if failures:
        return report()

    docs = {d: read(d) for d in ALL_DOCS}

    # --- no legacy PRODUCT identity --------------------------------------
    # "SETA" may appear ONLY as the supplier/hardware reference or when
    # naming the legacy artwork/executable being replaced. A legacy PRODUCT
    # name is what must never appear.
    banned_product = [
        "Seta Electronic Target Control",
        "Seeds Electronic Target Control",
        "Seta.exe is the product",
    ]
    for d, text in docs.items():
        for bad in banned_product:
            check(bad not in text, "no legacy product name (%s): %s" % (bad, d))

    # --- product identity is consistent ----------------------------------
    for d in EN_DOCS + DE_DOCS:
        t = docs[d]
        check("Tech Aim Electronic Target Control" in t,
              "full product name present: %s" % d)
        check("TechAim.exe" in t, "executable named: %s" % d)
        check("JAC SHOOTING SOLUTIONS (PTY) LTD" in t, "publisher present: %s" % d)
        check("0.9.0" in t, "product version present: %s" % d)
        check("Pre-Beta Validation" in t or "Pre-Beta" in t,
              "release channel present: %s" % d)
        check(re.search(r"[Cc]ommit `[0-9a-f]{7,}`", t) is not None,
              "application commit present: %s" % d)
        # Never the unspaced brand in running prose as the product name.
        check("TechAim Electronic" not in t,
              "brand spelled 'Tech Aim' in prose: %s" % d)

    # --- German documents must be marked beta + review-required ----------
    for d in DE_DOCS:
        t = docs[d]
        check("GERMAN BETA TRANSLATION" in t, "marked GERMAN BETA: %s" % d)
        check("NATIVE TECHNICAL REVIEW REQUIRED" in t,
              "marked native-review-required: %s" % d)
        check("Master" in t or "master" in t,
              "points at the English master edition: %s" % d)

    # --- required manual parts -------------------------------------------
    manual = docs["TechAim_Operator_Manual_EN.md"]
    for n in range(1, 21):
        check(("Part %d " % n) in manual, "operator manual has Part %d" % n)

    # --- required quick-start sections -----------------------------------
    qs = docs["TechAim_Quick_Start_EN.md"]
    for n in range(1, 21):
        check(re.search(r"^## %d\. " % n, qs, re.M) is not None,
              "quick start has section %d" % n)
    check("FIRST SESSION CHECKLIST" in qs.upper(),
          "quick start has the first-session checklist")

    # --- every current Training programme is documented ------------------
    for prog in ["Technical Blocks", "Call & Diagnose", "Position Transition",
                 "Group Pattern"]:
        check(prog in manual, "training programme documented: %s" % prog)

    # --- every reachable report type is documented -----------------------
    for rep in ["Technical Blocks", "Call & Diagnose", "Position Transition",
                "10 m Final", "3P Final", "Coach report"]:
        check(rep in manual, "report documented: %s" % rep)

    # --- troubleshooting index completeness ------------------------------
    ts = docs["TechAim_Troubleshooting_EN.md"]
    for area in ["Application", "Target connection", "Scoring and coordinates",
                 "Training Lab", "Reports", "Recovery",
                 "Windows and security", "Decision trees"]:
        check(area in ts, "troubleshooting covers: %s" % area)
    for tree in ["No shot received", "Session will not resume",
                 "PDF will not export", "Application will not start"]:
        check(tree in ts, "decision tree present: %s" % tree)

    # --- security guidance must never tell users to disable protection ---
    for d, t in docs.items():
        low = t.lower()
        for bad in ["disable defender", "turn off defender",
                    "disable smartscreen", "turn off smartscreen",
                    "disable the firewall", "turn off the firewall",
                    "disable windows firewall"]:
            check(bad not in low, "no instruction to disable protection (%s): %s"
                  % (bad, d))

    # --- unsupported features must not be described as available ---------
    # They may be NAMED (as excluded); they must not be presented as usable.
    excluded = ["Wind Map", "SCATT", "Consistency Chain", "Shadow Shooting"]
    for d in EN_DOCS + DE_DOCS:
        t = docs[d]
        for feat in excluded:
            if feat in t:
                # must appear near an exclusion word
                idx = t.find(feat)
                window = t[max(0, idx - 400):idx + 400].lower()
                check(any(w in window for w in
                          ["not included", "excluded", "not started",
                           "nicht enthalten", "ausgeschlossen"]),
                      "excluded feature framed as unavailable (%s): %s" % (feat, d))

    # --- no development paths / personal data ----------------------------
    for d, t in docs.items():
        check("C:\\Users\\" not in t, "no developer user path: %s" % d)
        check("Downloads\\TechAimSoftware" not in t
              and "Downloads/TechAimSoftware" not in t,
              "no repository path: %s" % d)
        check("arnoldbailie" not in t.lower(), "no personal username: %s" % d)

    # --- screenshot references resolve -----------------------------------
    reg = docs["TechAim_Manual_Screenshot_Register.md"]
    ids = set(re.findall(r"\bSS-\d\d\b", reg))
    check(len(ids) >= 25, "screenshot register defines >= 25 screenshots")
    img_dir = os.path.join(MAN, "images")
    # Any image actually referenced from a manual must exist on disk.
    missing = []
    for d in EN_DOCS + DE_DOCS:
        for ref in re.findall(r"!\[[^\]]*\]\(([^)]+)\)", docs[d]):
            path = os.path.normpath(os.path.join(MAN, ref))
            if not os.path.isfile(path):
                missing.append("%s -> %s" % (d, ref))
    check(not missing, "all embedded image references resolve (%s)" % (missing or "none"))
    # Placeholders must not be committed as if they were approved captures.
    if os.path.isdir(img_dir):
        stray = [f for f in os.listdir(img_dir)
                 if f.lower().endswith((".png", ".jpg")) and "placeholder" in f.lower()]
        check(not stray, "no placeholder images committed as captures")
    else:
        check(True, "no placeholder images committed as captures")

    # --- validation checklist must not claim unearned verification -------
    chk = docs["TechAim_Manual_Validation_Checklist.md"]

    # Status vocabulary is repository-wide, so scan EVERY tracked .md under
    # docs/, not just the manual set.
    doc_corpus = {}
    for base, _dirs, files in os.walk(os.path.join(ROOT, "docs")):
        if "output" in base:
            continue
        for f in files:
            if f.endswith(".md"):
                fp = os.path.join(base, f)
                rel = os.path.relpath(fp, ROOT).replace("\\", "/")
                with io.open(fp, encoding="utf-8") as fh:
                    doc_corpus[rel] = fh.read()
    # STATUS_PHRASES are the ONLY user-facing status values. Abbreviations are
    # permitted here, as internal test constants; what is ASSERTED is always
    # the complete phrase, because that is what a rendered document must show.
    STATUS_PHRASES = {
        "CT":  "VERIFIED FROM CODE AND TESTS",
        "MT":  "VERIFIED BY EXISTING MANUAL TEST",
        "HV":  "HUMAN VISUAL CHECK REQUIRED",
        "RC1": "WINDOWS RC1 DEPENDENT",
        "PT":  "PHYSICAL TARGET DEPENDENT",
        "DE":  "GERMAN REVIEW REQUIRED",
    }
    for phrase in STATUS_PHRASES.values():
        check(phrase in chk, "validation checklist defines status: %s" % phrase)
    check("no VERIFIED BY EXISTING MANUAL TEST entries" in chk,
          "validation checklist states that nothing is manually verified yet")

    # Superseded vocabulary must not linger in ANY repository document.
    SUPERSEDED = ["VERIFIED AUTOMATICALLY", "VERIFIED MANUALLY",
                  "MANUAL VALIDATION REQUIRED", "PHYSICAL HARDWARE DEPENDENT",
                  "PHYSICAL HARDWARE VALIDATION REQUIRED"]
    for d, t in doc_corpus.items():
        for bad in SUPERSEDED:
            check(bad not in t, "superseded status label absent (%s): %s" % (bad, d))
        check("[VERIFIED]" not in t and "[HUMAN]" not in t,
              "superseded bracket markers absent: %s" % d)

    # No external-facing document may use a BARE abbreviation as a status.
    # Only the bold form is checked: "DE" also legitimately labels the German
    # LANGUAGE column of the screenshot register, and "PT" appears in prose.
    for d, t in doc_corpus.items():
        bare = re.findall(r"\*\*(?:CT|MT|HV|RC1|PT|DE)\*\*", t)
        check(not bare,
              "no abbreviated status in a rendered document (%d found): %s"
              % (len(bare), d))

    # The documents that actually carry VALIDATION statuses must spell at
    # least one out in full. Scoped explicitly rather than by heuristic: the
    # ISSF rules documents also have a "Status" column, but it records rule
    # confirmation, which is an unrelated vocabulary.
    VALIDATION_DOCS = [
        "docs/manual/TechAim_Manual_Validation_Checklist.md",
        "docs/manual/TechAim_Operator_Manual_EN.md",
        "docs/manual/TechAim_Quick_Start_EN.md",
        "docs/manual/TechAim_Troubleshooting_EN.md",
        "docs/manual/TechAim_Manual_Review_Findings.md",
        "docs/manual/manual-pdf-validation.md",
        "docs/manual/target-connection-validation.md",
        "docs/manual/_shared/document-metadata.md",
        "docs/pre-beta-manual-acceptance.md",
        "docs/german-beta-visual-review.md",
    ]
    for d in VALIDATION_DOCS:
        check(d in doc_corpus, "validation-status document present: %s" % d)
        if d in doc_corpus:
            check(any(ph in doc_corpus[d] for ph in STATUS_PHRASES.values()),
                  "uses a complete status phrase: %s" % d)

    # Where the checklist gives a legend, it must sit beside its table.
    legend_at = chk.find("| Status | Meaning |")
    table_at  = chk.find("| # | Procedure |")
    check(0 <= legend_at < table_at,
          "status legend appears immediately before the traceability table")

    # --- release blockers must be recorded, not softened ------------------
    check("LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA" in chk,
          "EULA artwork marked LEGAL REPLACEMENT REQUIRED (checklist)")
    manual_t = docs["TechAim_Operator_Manual_EN.md"]
    check("LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA" in manual_t,
          "EULA artwork marked LEGAL REPLACEMENT REQUIRED (manual)")
    check("not Live-hardware certified" in manual_t
          or "not** Live-hardware certified" in manual_t,
          "manual states the system is NOT Live-hardware certified")
    # No document may claim Live/hardware certification.
    for d, t in docs.items():
        low = t.lower()
        for bad in ["live hardware certified", "live-target certified",
                    "issf certified", "issf-certified software is",
                    "seta certified"]:
            if bad in low:
                idx = low.find(bad)
                window = low[max(0, idx - 200):idx + 200]
                check(any(n in window for n in ["not ", "nicht ", "no "]),
                      "certification claim is negated (%s): %s" % (bad, d))

    # --- the application icon must not be invented ------------------------
    reg_t = docs["TechAim_Manual_Screenshot_Register.md"]
    docs_extra_pdfval = io.open(os.path.join(MAN, "manual-pdf-validation.md"),
                                encoding="utf-8").read()
    check("no approved" in reg_t and "ico" in reg_t.lower(),
          "screenshot register records that no approved .ico exists")
    check("PENDING — BLOCKED" in reg_t,
          "application-icon screenshot is registered as blocked")

    # --- screenshot rejection rules --------------------------------------
    for rule in ["Seta / Seeds", "Hello World", "Seta.exe",
                 "end-user agreement artwork"]:
        check(rule in reg_t, "screenshot rejection rule present: %s" % rule)

    # --- German pending-localisation list must be precise ----------------
    gs_t = docs["TechAim_German_Translation_Status.md"]
    check("pending localisation" in gs_t.lower(),
          "German status lists sections pending localisation")
    check("Part 20" in gs_t and "Part 12" in gs_t,
          "German pending list names specific manual parts")

    # --- German status must state real numbers ---------------------------
    gs = docs["TechAim_German_Translation_Status.md"]
    check("583" in gs and "100" in gs and "483" in gs,
          "German status states the real catalogue numbers")

    # --- P0.1: diagrams -------------------------------------------------
    diag_dir = os.path.join(MAN, "diagrams")
    check(os.path.isfile(os.path.join(diag_dir, "make_diagrams.py")),
          "diagram source script is committed")
    svgs = sorted(f for f in os.listdir(diag_dir)) if os.path.isdir(diag_dir) else []
    svgs = [f for f in svgs if f.endswith(".svg")]
    check(len(svgs) == 11, "all 11 diagrams rendered (found %d)" % len(svgs))
    for f in svgs:
        p_svg = os.path.join(diag_dir, f)
        check(os.path.getsize(p_svg) > 500, "diagram non-trivial: %s" % f)
    # every diagram referenced by a manual must exist
    for d in EN_DOCS + DE_DOCS:
        for ref in re.findall(r"!\[[^\]]*\]\((diagrams/[^)]+)\)", docs[d]):
            check(os.path.isfile(os.path.join(MAN, ref)),
                  "referenced diagram exists: %s (%s)" % (ref, d))
    # Diagrams are rendered, NOT visually approved. The register and the PDF
    # validation record must both say so, so "COMPLETE" can never be read as
    # "somebody looked at it".
    check("HUMAN VISUAL CHECK REQUIRED" in reg_t,
          "screenshot register records diagram status as visual-check-required")
    check("RENDERED but not yet inspected" in reg_t,
          "screenshot register does not claim diagrams are visually approved")
    pdfv = docs_extra_pdfval
    check("HUMAN VISUAL CHECK REQUIRED" in pdfv and "nobody has *looked*" in pdfv,
          "PDF record states diagram legibility is unverified")

    # the update diagram must not present updates as available
    upd = io.open(os.path.join(diag_dir, "DG-11_update_flow.svg"), encoding="utf-8").read()
    check("NOT YET AVAILABLE" in upd and "RC1 DEPENDENT" in upd,
          "update diagram marks the process as not yet available")

    # --- P0.1: publication pipeline + records ---------------------------
    for f in ["manual-pdf-toolchain.md", "manual-pdf-validation.md",
              "target-connection-validation.md", "build-manuals.ps1",
              "_shared/brand-assets.md", "_shared/windows-icon-specification.md",
              "_shared/manual-print.css"]:
        check(os.path.isfile(os.path.join(MAN, f)), "P0.1 artefact exists: %s" % f)
    for f in ["docs/legal/eula-replacement-requirements.md",
              "docs/german-beta-visual-review.md"]:
        check(os.path.isfile(os.path.join(ROOT, f)), "P0.1 artefact exists: %s" % f)

    build = io.open(os.path.join(MAN, "build-manuals.ps1"), encoding="utf-8").read()
    check("LASTEXITCODE" in build, "build script checks exit codes")
    check("resource-path" in build, "build script resolves image paths")
    check("did not embed" in build, "build script fails on unembedded images")
    check("suspiciously small" in build, "build script rejects truncated output")

    pdfval = io.open(os.path.join(MAN, "manual-pdf-validation.md"), encoding="utf-8").read()
    check("NO PDF IS APPROVED" in pdfval,
          "PDF record does not claim approval without visual inspection")
    check("NOT PERFORMED" in pdfval, "PDF record marks visual checks not performed")

    # --- P0.1: no false visual-verification claims -----------------------
    reg_v = docs["TechAim_Manual_Screenshot_Register.md"]
    check("No screenshots have been captured" in reg_v,
          "screenshot register does not claim captures")
    check("Alex Example" in reg_v, "screenshot register names the synthetic athlete")
    ger = io.open(os.path.join(ROOT, "docs/german-beta-visual-review.md"),
                  encoding="utf-8").read()
    check("was NOT performed" in ger or "NOT performed" in ger,
          "German visual review does not claim to have been performed")
    check("mixed-language evaluation preview" in ger,
          "German review gives an explicit recommendation")
    eula = io.open(os.path.join(ROOT, "docs/legal/eula-replacement-requirements.md"),
                   encoding="utf-8").read()
    # normalise line wrapping before matching prose
    eula_flat = re.sub(r"\s+", " ", eula)
    check("does not contain, draft or approve legal terms" in eula_flat,
          "EULA audit does not draft legal wording")
    check("LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA" in eula,
          "EULA audit records the blocker")
    icon = io.open(os.path.join(MAN, "_shared/windows-icon-specification.md"),
                   encoding="utf-8").read()
    check("NO APPROVED ICON EXISTS" in icon, "icon status recorded accurately")
    rc = io.open(os.path.join(ROOT, "TechAim.rc"), encoding="utf-8").read()
    check("ICON" not in rc.upper().replace("ICONS", ""),
          "TechAim.rc has NOT been given an icon before one is approved")

    # --- internal document cross-references resolve ----------------------
    bad_links = []
    for d, t in docs.items():
        for ref in re.findall(r"`(TechAim_[A-Za-z_]+\.md)`", t):
            if not os.path.isfile(os.path.join(MAN, ref)):
                bad_links.append("%s -> %s" % (d, ref))
    check(not bad_links, "all manual cross-references resolve (%s)"
          % (bad_links or "none"))

    return report()


def report():
    print("\n=== %d checks, %d failures ===" % (checks, len(failures)))
    for f in failures:
        print("  FAILED: %s" % f)
    sys.stdout.flush()
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
