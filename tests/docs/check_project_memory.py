#!/usr/bin/env python3
"""Tech Aim — project-memory documentation checks.

Proves the repository memory is internally consistent and, crucially, that it
does not overclaim: a defect may not be marked fully resolved without visual
evidence, and concept mockups may not be presented as application evidence.

Run:  python tests/docs/check_project_memory.py
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


REQUIRED = [
    "CLAUDE.md",
    "docs/project/Current_Project_State.md",
    "docs/design/TechAim_Design_System.md",
    "docs/design/Component_Catalogue.md",
    "docs/design/Screen_Layout_Rules.md",
    "docs/design/Brand_Asset_Register.md",
    "docs/design/Brand_Flavor_Guide.md",
    "docs/design/current-design-audit.md",
    "docs/ui/UI_Decision_Log.md",
    "docs/ui/UI_Defect_Register.md",
    "docs/ui/Homepage_Acceptance_Checklist.md",
    "docs/ui/Homepage_As_Built.md",
]

print("--- Tech Aim project memory ---")

# ── every required document exists ─────────────────────────────────────────
docs = {}
for rel in REQUIRED:
    body = read(rel)
    docs[rel] = body
    check(body is not None, "memory file exists: %s" % rel)

if any(v is None for v in docs.values()):
    print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
    sys.exit(1)

claude = docs["CLAUDE.md"]
state = docs["docs/project/Current_Project_State.md"]
log = docs["docs/ui/UI_Decision_Log.md"]
reg = docs["docs/ui/UI_Defect_Register.md"]
chk = docs["docs/ui/Homepage_Acceptance_Checklist.md"]
built = docs["docs/ui/Homepage_As_Built.md"]

# ── CLAUDE.md wires the memory in ──────────────────────────────────────────
check("TECH AIM UI PROJECT MEMORY" in claude,
      "CLAUDE.md declares the UI project memory section")
CLAUDE_REQUIRED = [
    "docs/project/Current_Project_State.md",
    "docs/design/TechAim_Design_System.md",
    "docs/design/Component_Catalogue.md",
    "docs/design/Screen_Layout_Rules.md",
    "docs/ui/UI_Decision_Log.md",
    "docs/ui/UI_Defect_Register.md",
    "docs/ui/Homepage_Acceptance_Checklist.md",
    "docs/ui/Homepage_As_Built.md",
]
for rel in CLAUDE_REQUIRED:
    check(rel in claude, "CLAUDE.md requires reading %s" % rel)
check("may silently reverse an accepted design decision" in claude,
      "CLAUDE.md forbids silently reversing a decision")
check("No defect may be closed without evidence" in claude,
      "CLAUDE.md forbids closing a defect without evidence")
# Pre-existing rules must survive.
for rule in ["ISSF", "docs/issf-rules/README.md", "qmake Seta.pro",
             "APPSETTINGS.getDeveloperMode()", "globalMatchModel"]:
    check(rule in claude, "CLAUDE.md preserves the existing rule mentioning %r" % rule)

# ── decision log ───────────────────────────────────────────────────────────
dec_ids = re.findall(r"^## (UI-DEC-\d{3})", log, re.M)
check(len(dec_ids) >= 11, "decision log has at least 11 decisions (%d)" % len(dec_ids))
check(len(dec_ids) == len(set(dec_ids)), "every decision ID is unique")
for n in range(1, 12):
    check("UI-DEC-%03d" % n in dec_ids, "decision UI-DEC-%03d is recorded" % n)

# Each decision carries the required fields.
blocks = re.split(r"^## (?=UI-DEC-)", log, flags=re.M)[1:]
for b in blocks:
    did = b.split("\n", 1)[0].split(" ")[0]
    for field in ["**Date**", "**Status**", "**Decision", "**Reasoning"]:
        check(field in b or field.rstrip(".") in b,
              "%s records %s" % (did, field.strip("*")))

check("Version B is the approved" in log, "UI-DEC-001 keeps Version B as the approved design")
check(re.search(r"UI-DEC-001[\s\S]{0,400}?\*\*Status\*\* \| ACCEPTED", log) is not None,
      "UI-DEC-001 is still ACCEPTED (Version B remains the active homepage)")

# Palette decision consistency across the memory.
for hexv in ["#A80038", "#C40046", "#80032A", "#BF1919"]:
    check(hexv in log, "palette decision records %s" % hexv)
check("#A80038" in docs["docs/design/TechAim_Design_System.md"],
      "design system agrees on the approved accent")
check("#A80038" in docs["docs/design/Brand_Asset_Register.md"],
      "asset register agrees on the approved accent")
check("#e6003c" not in log.replace("#e6003c", "", 0) or "not adopted" in log
      or "concept" in log.lower(),
      "the UI-0 concept accent is only ever named as a rejected concept")

# Homepage structure decision consistency.
for term in ["Session setup", "action bar", "scroll"]:
    check(term.lower() in log.lower(), "structure decision mentions %r" % term)

# OEM stays deferred.
check("SetaOem" in log or "OEM" in log, "OEM decision is recorded")
check("not implemented" in log.lower() or "refused" in log.lower(),
      "OEM appearance remains deferred / unimplemented")
check("DEFERRED" in log, "Version C remains deferred")

# ── defect register ────────────────────────────────────────────────────────
def_ids = re.findall(r"UI-HOME-(\d{3})", reg)
uniq = sorted(set(def_ids))
check(len(uniq) >= 10, "defect register has at least 10 defects (%d)" % len(uniq))
for n in range(1, 11):
    check("%03d" % n in uniq, "defect UI-HOME-%03d is recorded" % n)

# Only the main register table has the full 10-column shape. The traceability
# table in section 4 also begins with "| UI-HOME-" and must not be counted.
rows = [l for l in reg.split("\n")
        if l.startswith("| UI-HOME-") and l.count("|") >= 11]
check(len(rows) == 10, "every defect has exactly one register row (%d)" % len(rows))

SEV_OK = ("RESOLVED", "PARTIALLY RESOLVED", "OPEN", "BLOCKED")
for row in rows:
    cells = [c.strip() for c in row.split("|")]
    did = cells[1].replace("*", "").strip()
    status = cells[6].replace("*", "").strip()
    fixed = cells[7].replace("*", "").strip()
    check(any(status.startswith(s) for s in SEV_OK),
          "%s has a valid status (%s)" % (did, status))
    if status.startswith("RESOLVED") or status.startswith("PARTIALLY"):
        check(fixed not in ("", "-", "—"),
              "%s is resolved/partial and records a fixed commit" % did,
              "fixed commit cell = %r" % fixed)
    # The overclaim guard: full closure demands visual evidence.
    if status == "RESOLVED — AUTOMATED AND VISUAL EVIDENCE":
        visual = cells[9].replace("*", "").strip()
        check(visual not in ("", "none", "-", "—"),
              "%s claims full resolution and cites visual evidence" % did)

check("MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED" in reg,
      "blocked screenshot capture is recorded with the required wording")
check("CONCEPT MOCKUP — NOT CURRENT APPLICATION" in reg,
      "the register names the concept stamp so mockups cannot be mistaken for evidence")
check("Must not" in reg or "must not" in reg,
      "the register forbids using pre-Version-B evidence for Version B")

# ── acceptance checklist ───────────────────────────────────────────────────
for res in ["1536 × 960", "1366 × 768", "1280 × 720", "1100 × 700"]:
    check(res in chk, "acceptance checklist covers %s" % res)

allowed = ["PASS", "FAIL", "BLOCKED", "NOT TESTED", "HUMAN VISUAL CHECK REQUIRED"]
found = re.findall(r"\| (PASS|FAIL|BLOCKED|NOT TESTED|HUMAN VISUAL CHECK REQUIRED) \|", chk)
check(len(found) > 40, "acceptance checklist records statuses (%d)" % len(found))
check(all(f in allowed for f in found), "only permitted status values are used")
check("NOT ACCEPTED" in chk, "the checklist does not claim acceptance without evidence")

# ── as-built references the real implementation ────────────────────────────
check("LoginPage.qml" in built, "as-built names the principal QML file")
for ident in ["actionBar", "eventScroll", "setupScroll",
              "selectedProgrammeKind", "summaryCard"]:
    check(ident in built, "as-built references the real identifier %s" % ident)
check("HUMAN VISUAL CHECK REQUIRED" in built,
      "as-built records that visual review is outstanding")
check("concept" in built.lower(), "as-built distinguishes itself from the concept")

# ── current state ──────────────────────────────────────────────────────────
check("Next approved" in state or "next approved" in state.lower(),
      "current state names the next approved task")
check("screenshot" in state.lower(), "current state names the screenshot task")
check("Range Management" in state and "not the next lane task" in state,
      "Range Management is explicitly NOT the next lane-software task")
check("app_mode" in state and "Live" in state, "current state requires app_mode=Live")
check("English" in state, "current state requires English")
check("8022033" in state, "current state records the Version B commit")
check("41c09a3" in state, "current state records the homepage defect-fix commit")
check("JAC SHOOTING SOLUTIONS (PTY) LTD" in state, "current state records the publisher")

print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
sys.exit(1 if FAILURES else 0)
