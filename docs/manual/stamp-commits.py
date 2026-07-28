# -*- coding: utf-8 -*-
"""Stamp the two commit identities into the manual front matter (P0.1).

They are DIFFERENT things and must be shown separately:

  * application baseline commit — the last commit that touched application
    source (the build the manual describes);
  * documentation source commit — the commit the manual text itself came from.

Documentation is edited far more often than the application, so a single
hardcoded "Application commit" line goes stale the moment a docs-only commit
lands — and it silently misreports which BUILD the manual describes. Before
this script the six manuals carried four different values.

Run after any commit that changes the manuals or the application:

    python docs/manual/stamp-commits.py

Then regenerate the published output:

    powershell -File docs/manual/build-manuals.ps1 -Format both
"""
import io
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAN = os.path.join(ROOT, "docs", "manual")

# Paths that constitute "the application". Documentation, tests and manual
# tooling are deliberately excluded.
APP_PATHS = ["*.cpp", "*.h", "*.qml", "*.pro", "*.pri", "*.rc", "*.qrc",
             "src/", "ModReader/"]

DOCS = [
    "TechAim_Quick_Start_EN.md",
    "TechAim_Operator_Manual_EN.md",
    "TechAim_Troubleshooting_EN.md",
    "TechAim_Quick_Start_DE.md",
    "TechAim_Operator_Manual_DE.md",
    "TechAim_Troubleshooting_DE.md",
    "TechAim_German_Translation_Status.md",
    "TechAim_Manual_Screenshot_Register.md",
    "TechAim_Manual_Validation_Checklist.md",
    "TechAim_Manual_Review_Findings.md",
    "manual-pdf-toolchain.md",
    "manual-pdf-validation.md",
    "target-connection-validation.md",
]


def git(*args):
    return subprocess.check_output(["git"] + list(args), cwd=ROOT).decode().strip()


def main():
    app = git("log", "-1", "--format=%h", "--", *APP_PATHS)
    doc = git("rev-parse", "--short", "HEAD")
    if not app:
        print("ERROR: could not resolve the application baseline commit")
        return 1

    stamp_en = ("Application baseline commit `%s` · "
                "Documentation source commit `%s`" % (app, doc))
    stamp_de = ("Anwendungs-Basis-Commit `%s` · "
                "Dokumentations-Commit `%s`" % (app, doc))

    changed = 0
    for name in DOCS:
        p = os.path.join(MAN, name)
        if not os.path.isfile(p):
            print("  skip (missing): %s" % name)
            continue
        s = io.open(p, encoding="utf-8").read()
        before = s
        stamp = stamp_de if name.endswith("_DE.md") else stamp_en

        # Already stamped -> refresh in place.
        s = re.sub(r"(?:Application baseline commit|Anwendungs-Basis-Commit) "
                   r"`[0-9a-f]+` · (?:Documentation source commit|"
                   r"Dokumentations-Commit) `[0-9a-f]+`", stamp, s)

        # Legacy single-value forms -> replace with the two-value stamp.
        s = re.sub(r"Anwendungs-Commit `[0-9a-f]+`", stamp_de, s)
        s = re.sub(r"Application commit `[0-9a-f]+`(?: \(doc v[\d.]+\))?",
                   stamp_en, s)

        if s != before:
            io.open(p, "w", encoding="utf-8", newline="").write(s)
            changed += 1
            print("  stamped: %s" % name)

    print("\napplication baseline commit : %s" % app)
    print("documentation source commit : %s" % doc)
    print("documents updated           : %d" % changed)
    return 0


if __name__ == "__main__":
    sys.exit(main())
