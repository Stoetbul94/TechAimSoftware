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
    for status in ["VERIFIED FROM CODE AND TESTS",
                   "VERIFIED BY EXISTING MANUAL TEST",
                   "HUMAN VISUAL CHECK REQUIRED",
                   "WINDOWS RC1 DEPENDENT",
                   "PHYSICAL TARGET DEPENDENT",
                   "GERMAN REVIEW REQUIRED"]:
        check(status in chk, "validation checklist defines status: %s" % status)
    check("no MT entries" in chk,
          "validation checklist states that nothing is manually verified yet")
    # The superseded vocabulary must not linger anywhere.
    for d, t in docs.items():
        check("MANUAL VALIDATION REQUIRED" not in t,
              "superseded status label removed: %s" % d)
        check("PHYSICAL HARDWARE DEPENDENT" not in t,
              "superseded hardware label removed: %s" % d)

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
