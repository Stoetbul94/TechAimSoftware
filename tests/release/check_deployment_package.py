#!/usr/bin/env python3
"""Tech Aim 0.9.0-RC1 - portable deployment audit.

Fails if the package carries anything that must never ship: source, tests,
seeded athlete data, repository metadata, build artefacts, secrets, developer
configuration or absolute developer paths. Then verifies the runtime files a
clean Windows machine actually needs.

Run:  python tests/release/check_deployment_package.py [package_dir]
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_PKG = os.path.join(ROOT, "dist", "TechAim-0.9.0-RC2d-Diagnostic-Windows-x64")

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


pkg = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PKG
print("--- Tech Aim RC1 deployment audit ---")
print("package: %s" % pkg)

check(os.path.isdir(pkg), "the package directory exists", pkg)
if not os.path.isdir(pkg):
    print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
    sys.exit(1)

# Inventory once.
files = []
for base, dirs, names in os.walk(pkg):
    for n in names:
        full = os.path.join(base, n)
        files.append((os.path.relpath(full, pkg).replace("\\", "/"), full))
rels = [r for r, _ in files]
print("files: %d" % len(rels))

# ---- 1. forbidden content ------------------------------------------------
# Extensions that have no business in a runtime package. .qml is NOT here: the
# QML is compiled into the binary's resources, so any loose .qml would be an
# unused copy of source - which the next check catches explicitly.
FORBIDDEN_EXT = [".cpp", ".h", ".hpp", ".cc", ".cxx", ".o", ".obj", ".a", ".lib",
                 ".pdb", ".ilk", ".exp", ".pro", ".pri", ".user", ".qrc",
                 ".pem", ".key", ".pfx", ".p12", ".jsonl"]
for ext in FORBIDDEN_EXT:
    bad = [r for r in rels if r.lower().endswith(ext)]
    check(not bad, "no %s files are packaged" % ext, ", ".join(bad[:4]))

# Qt's own QML modules ship .qml by design - that is how a QML module works.
# What must never appear is TECH AIM QML: the application's interface is
# compiled into the binary's resources, so a loose copy would be source.
QT_QML_MODULE_ROOTS = ("QtQuick/", "QtQml/", "QtCharts/", "Qt5Compat/", "Qt/", "QtCore/",
                       "QtMultimedia/", "QtQuick.2/", "QtTest/")
app_qml = [r for r in rels
           if r.lower().endswith(".qml") and not r.startswith(QT_QML_MODULE_ROOTS)]
check(not app_qml, "no Tech Aim QML source is packaged (it is compiled into the binary)",
      ", ".join(app_qml[:4]))

# Directories that must not appear at any depth.
FORBIDDEN_DIRS = [".git", ".claude", "tests", "src", "dist", "manual-preview",
                  "__pycache__", "debug", "Sessions", "node_modules"]
for d in FORBIDDEN_DIRS:
    bad = [r for r in rels if ("/%s/" % d) in ("/" + r) or r.startswith(d + "/")]
    check(not bad, "no '%s' directory is packaged" % d, ", ".join(bad[:3]))

# Test executables and build scripts.
bad = [r for r in rels if re.search(r"(_tests?\.exe|qmake|mingw32-make|\.bat$|Makefile)", r, re.I)]
check(not bad, "no test executables or build scripts are packaged", ", ".join(bad[:4]))

# Seeded review data / athlete sessions.
# Athlete/seed data is journal- and session-shaped. Matching bare words like
# "review" anywhere in a path flags Qt's own CharacterPreviewBubble.qml, which
# is a false positive - scope it to the athlete names and data shapes we seed.
SEED_MARKERS = ["Fitzwilliam", "Short-Session", "Arnold Bailie", "windmap-review",
                "finals_session", "session.jsonl", "snapshot.json"]
bad = [r for r in rels if any(m.lower() in r.lower() for m in SEED_MARKERS)]
check(not bad, "no seeded review or athlete session data is packaged", ", ".join(bad[:4]))
bad = [r for r in rels if re.search(r"(?i)(seed_|--seed-|/Sessions/|Current/)", r)]
check(not bad, "no session store is packaged", ", ".join(bad[:4]))

# ---- 2. no secrets or developer paths inside shipped text ---------------
TEXT_EXT = (".ini", ".txt", ".md", ".json", ".conf", ".ps1")
SECRET_RE = re.compile(r"(?i)\b(password|passwd|secret|api[_-]?key|token|private[_-]?key)\b\s*[:=]\s*\S")
DEVPATH_RE = re.compile(r"(?i)([A-Z]:[\\/]Users[\\/][^\\/\s\"']+[\\/](Downloads|Documents|Desktop)[\\/]TechAimSoftware|[A-Z]:[\\/]Qt[\\/])")
for rel, full in files:
    if not rel.lower().endswith(TEXT_EXT):
        continue
    try:
        body = io.open(full, encoding="utf-8", errors="replace").read()
    except Exception:
        continue
    m = SECRET_RE.search(body)
    check(m is None, "%s carries no secret-looking assignment" % rel,
          m.group(0)[:60] if m else "")
    # The support-bundle script legitimately NAMES these words in order to
    # strip them; it is the one file allowed to mention them.
    if rel.lower().endswith("make-supportbundle.ps1"):
        continue
    d = DEVPATH_RE.search(body)
    check(d is None, "%s carries no absolute developer path" % rel,
          d.group(0)[:70] if d else "")

# ---- 3. the shipped configuration is a RELEASE configuration ------------
cfg = os.path.join(pkg, "config.ini")
check(os.path.isfile(cfg), "a configuration template is packaged")
if os.path.isfile(cfg):
    body = io.open(cfg, encoding="utf-8").read()
    check("app_mode=Live" in body, "packaged configuration is Live, not Demo")
    check("app_mode=Demo" not in body, "packaged configuration carries no Demo mode")
    check("developer_mode=0" in body, "developer mode is off in the packaged configuration")
    check(not body.startswith("﻿"), "packaged configuration has no UTF-8 BOM")

# ---- 4. required runtime ------------------------------------------------
REQUIRED = ["TechAim.exe", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Qml.dll", "Qt6Quick.dll",
            "Qt6Network.dll", "Qt6SerialPort.dll", "Qt6Widgets.dll"]
for r in REQUIRED:
    check(os.path.isfile(os.path.join(pkg, r)), "required runtime file present: %s" % r)

check(os.path.isfile(os.path.join(pkg, "platforms", "qwindows.dll")),
      "the Qt Windows platform plugin is present")
check(os.path.isdir(os.path.join(pkg, "imageformats")),
      "image-format plugins are present")
check(os.path.isdir(os.path.join(pkg, "QtQuick")) or os.path.isdir(os.path.join(pkg, "QtQuick.2")),
      "QtQuick QML modules are present")

# The MinGW C++ runtime, or the app will not start on a clean machine.
runtime = [r for r in rels if re.match(r"(?i)(libgcc|libstdc\+\+|libwinpthread)", os.path.basename(r))]
check(len(runtime) >= 3, "the compiler runtime libraries are present",
      ", ".join(os.path.basename(r) for r in runtime))

# ---- 5. field-test documents + operator tooling -------------------------
check(os.path.isfile(os.path.join(pkg, "Make-SupportBundle.ps1")),
      "the support-bundle tool is packaged")
for d in ("0.9.0-rc2a-diagnostic.md", "0.9.0-rc2-known-limitations.md",
          "0.9.0-rc2-field-test-checklist.md"):
    check(os.path.isfile(os.path.join(pkg, "docs", d)), "field-test document packaged: %s" % d)

# ---- 6. every top-level entry is accounted for --------------------------
# An unexplained top-level directory is how unwanted material arrives.
KNOWN_TOP_DIRS = {
    "platforms": "Qt Windows platform plugin (qwindows.dll) - the app cannot create a window without it",
    "imageformats": "PNG/JPEG/SVG plugins for logos, target graphics and report images",
    "iconengines": "SVG icon engine used by the theme",
    "styles": "Qt Quick Controls Windows style",
    "multimedia": "Qt Multimedia backend for the finals audio cues",
    "networkinformation": "Qt network-state backend pulled in by Qt6Network",
    "tls": "TLS backends pulled in by Qt6Network",
    "generic": "Qt generic input plugins",
    "QtQuick": "QML modules the interface imports",
    "QtQuick.2": "QML modules the interface imports",
    "QtQml": "QML engine modules",
    "Qt": "QML support module",
    "QtCore": "QML core module",
    "QtMultimedia": "QML multimedia module for the finals audio cues",
    "docs": "field-test plan, checklist, known limitations and rollback procedure",
    "translations": "approved translations",
    "sqldrivers": "Qt SQL drivers pulled in by the Qt runtime",
    "QtCharts": "QtCharts QML module - imported by CenterPane and the coach views",
    "Qt5Compat": "Qt5Compat.GraphicalEffects only - imported by TechAimDialog",
}
tops = sorted(d for d in os.listdir(pkg) if os.path.isdir(os.path.join(pkg, d)))
for t in tops:
    check(t in KNOWN_TOP_DIRS, "top-level directory '%s' is documented and required" % t,
          "undocumented - add it to KNOWN_TOP_DIRS with a reason, or stop shipping it")

print("\ntop-level directories shipped:")
for t in tops:
    print("  %-20s %s" % (t, KNOWN_TOP_DIRS.get(t, "UNDOCUMENTED")))

print("\n=== %d checks, %d failures ===" % (CHECKS, FAILURES))
sys.exit(1 if FAILURES else 0)
