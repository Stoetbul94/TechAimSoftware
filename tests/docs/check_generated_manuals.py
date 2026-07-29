# -*- coding: utf-8 -*-
"""Checks on the GENERATED manuals and their build manifest (P0.1).

Separate from check_manuals.py, which validates tracked SOURCE. This script
validates the published OUTPUT, so it requires a build first:

    powershell -File docs/manual/build-manuals.ps1 -Format both
    python tests/docs/check_generated_manuals.py

Exit 0 = all checks passed. Exit 3 = no output present (build not run).
"""
import hashlib
import io
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAN = os.path.join(ROOT, "docs", "manual")
OUT = os.path.join(MAN, "output")
MANIFEST = os.path.join(OUT, "TechAim_Manual_Build_Manifest.json")

EXPECTED = [
    ("TechAim_Quick_Start_EN", "en"),
    ("TechAim_Operator_Manual_EN", "en"),
    ("TechAim_Troubleshooting_EN", "en"),
    ("TechAim_Quick_Start_DE_Beta", "de-DE"),
    ("TechAim_Operator_Manual_DE_Beta", "de-DE"),
    ("TechAim_Troubleshooting_DE_Beta", "de-DE"),
]

checks = 0
failures = []


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        failures.append(label)
    print(("PASS  " if ok else "FAIL  ") + label)


def git(*args):
    return subprocess.check_output(["git"] + list(args), cwd=ROOT).decode().strip()


def main():
    print("=== Tech Aim generated-manual checks ===")
    if not os.path.isfile(MANIFEST):
        print("\nNo build manifest at %s" % MANIFEST)
        print("Run: powershell -File docs/manual/build-manuals.ps1 -Format both")
        return 3

    man = json.load(io.open(MANIFEST, encoding="utf-8-sig"))

    # ── provenance matches the repository, not a stale carry-forward ────
    head = git("rev-parse", "HEAD")
    check(man["documentationSourceCommit"] == head,
          "manifest documentation source commit == current HEAD (%s)" % head[:7])

    app_paths = ["*.cpp", "*.h", "*.qml", "*.pro", "*.pri", "*.rc", "*.qrc",
                 "src/", "ModReader/", "translations/", "images/", "*.ini"]
    baseline = git("log", "-1", "--format=%H", "--", *app_paths)
    check(man["applicationBaselineCommit"] == baseline,
          "manifest application baseline == calculated baseline (%s)" % baseline[:7])
    check(man["applicationBaselineCommit"] != man["documentationSourceCommit"]
          or baseline == head,
          "baseline and documentation commits are computed independently")

    # ── the manifest describes all six manuals ─────────────────────────
    for base, lang in EXPECTED:
        for fmt in ("html", "pdf"):
            hit = [e for e in man["documents"]
                   if e["filename"] == "%s.%s" % (base, fmt)]
            check(len(hit) == 1, "manifest has one entry for %s.%s" % (base, fmt))
            if hit:
                check(hit[0]["language"] == lang,
                      "language recorded for %s.%s" % (base, fmt))

    # ── every manifest value matches the file on disk ──────────────────
    for e in man["documents"]:
        p = os.path.join(OUT, e["filename"])
        check(os.path.isfile(p), "output exists: %s" % e["filename"])
        if not os.path.isfile(p):
            continue
        blob = open(p, "rb").read()
        check(len(blob) == e["fileSizeBytes"],
              "size matches manifest: %s" % e["filename"])
        check(hashlib.sha256(blob).hexdigest() == e["sha256"],
              "SHA-256 matches manifest: %s" % e["filename"])
        check(e["fileSizeBytes"] > 0, "non-empty: %s" % e["filename"])
        for field in ("product", "productVersion", "documentVersion",
                      "buildTimestamp", "generationTool"):
            check(bool(e.get(field)), "manifest field '%s' set: %s"
                  % (field, e["filename"]))
        # Never claim visual approval.
        check(e["visualValidationStatus"] == "HUMAN VISUAL CHECK REQUIRED",
              "visual status not falsely approved: %s" % e["filename"])

    check("GENERATED" in man["overallStatus"]
          and "HUMAN VISUAL CHECK REQUIRED" in man["overallStatus"],
          "overall status is GENERATED - HUMAN VISUAL CHECK REQUIRED")
    check(man.get("screenshotsEmbedded") is False,
          "manifest records that no screenshots are embedded")

    # ── the generated documents carry the resolved values ──────────────
    head_short = git("rev-parse", "--short", "HEAD")
    base_short = git("log", "-1", "--format=%h", "--", *app_paths)
    for base, _lang in EXPECTED:
        p = os.path.join(OUT, base + ".html")
        if not os.path.isfile(p):
            continue
        t = io.open(p, encoding="utf-8").read()
        flat = re.sub(r"\s+", " ", t)   # HTML wraps; match on normalised text
        check(not re.search(r"\{\{[A-Z_]+\}\}", t),
              "no unresolved placeholder in output: %s.html" % base)
        check(head_short in t,
              "output shows the current documentation source commit: %s.html" % base)
        check(base_short in t,
              "output shows the calculated application baseline: %s.html" % base)
        if base.endswith("_DE_Beta"):
            check("GERMAN BETA TRANSLATION" in flat,
                  "German edition retains the Beta warning: %s.html" % base)
            check("NATIVE TECHNICAL REVIEW REQUIRED" in flat,
                  "German edition retains the native-review warning: %s.html" % base)
            # UTF-8 survived the staging round trip.
            check(any(c in t for c in "äöüßÄÖÜ"),
                  "German characters intact: %s.html" % base)

    # ── stamping must not mutate tracked source ────────────────────────
    # Proven directly rather than by inspecting the working tree, which may
    # legitimately carry unrelated edits: snapshot the tracked Markdown, run
    # the stamper into a throwaway directory, and require byte equality.
    import shutil, tempfile
    def snapshot():
        out = {}
        for f in sorted(os.listdir(MAN)):
            if f.endswith(".md"):
                out[f] = hashlib.sha256(
                    open(os.path.join(MAN, f), "rb").read()).hexdigest()
        return out

    before = snapshot()
    tmp = tempfile.mkdtemp(prefix="techaim_stamp_probe_")
    try:
        rc = subprocess.call([sys.executable,
                              os.path.join(MAN, "stamp-commits.py"),
                              "--out", os.path.join(tmp, "staged")],
                             cwd=ROOT, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        check(rc == 0, "stamper runs successfully")
        after = snapshot()
        changed = [f for f in before if before[f] != after.get(f)]
        check(not changed,
              "stamping did not modify tracked source (%s)" % (changed or "none"))
        staged = os.path.join(tmp, "staged")
        check(os.path.isdir(staged), "stamper produced a staging directory")
        if os.path.isdir(staged):
            sample = os.path.join(staged, "TechAim_Operator_Manual_EN.md")
            if os.path.isfile(sample):
                st = io.open(sample, encoding="utf-8").read()
                check(not re.search(r"\{\{[A-Z_]+\}\}", st),
                      "staged copy has no unresolved placeholder")
                check(head_short in st,
                      "staged copy carries the current HEAD")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("\n=== %d checks, %d failures ===" % (checks, len(failures)))
    for f in failures:
        print("  FAILED: %s" % f)
    sys.stdout.flush()
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
