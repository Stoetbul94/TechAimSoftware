# -*- coding: utf-8 -*-
"""Build-time provenance stamping for the Tech Aim manuals (P0.1).

THE TRACKED MARKDOWN IS NEVER MODIFIED. Source documents carry stable
placeholders; this script copies them to a temporary build directory and
substitutes there. That is the only way the documentation source commit can
equal the commit that CONTAINS the documents: stamping concrete hashes into
tracked source is self-defeating, because committing the stamp changes HEAD
and the stamp is immediately stale.

Placeholders:
    {{APPLICATION_BASELINE_COMMIT}}   last commit affecting the application
    {{DOCUMENTATION_SOURCE_COMMIT}}   git rev-parse HEAD at generation time
    {{DOCUMENT_BUILD_TIMESTAMP}}      UTC timestamp of this generation
    {{DOCUMENT_VERSION}}              manual-set release version

Usage (normally called by build-manuals.ps1):

    python docs/manual/stamp-commits.py --out <staging-dir>
    python docs/manual/stamp-commits.py --print-json     # values only

Exit codes: 0 ok, non-zero on ANY failure. Missing Git information is a hard
failure — never an "unknown" or a stale carry-forward.
"""
import argparse
import datetime
import io
import json
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAN = os.path.join(ROOT, "docs", "manual")

# Manual-set release version. Per-document editorial versions ("Document
# version 1.2") stay literal in each file; this is the version of the SET.
MANUAL_SET_VERSION = "P0.1"

# ── Application baseline definition ──────────────────────────────────────
# The latest commit affecting APPLICATION BEHAVIOUR OR BUILD IDENTITY.
# Documentation-only commits are excluded by construction: no docs/ path and
# no test path appears here.
#
#   *.cpp *.h        C++ sources and headers
#   *.qml            QML UI
#   *.pro *.pri      qmake project + shared includes (build identity)
#   *.rc             Windows version resource (build identity)
#   *.qrc            resource manifests — what is compiled INTO the exe
#   src/ ModReader/  application + vendored Modbus source trees
#   translations/    .ts/.qm catalogues, embedded via techaim_translations.qrc
#   images/          application assets, embedded via images.qrc
#   *.ini            runtime configuration templates, if ever tracked
APP_PATHS = [
    "*.cpp", "*.h", "*.qml", "*.pro", "*.pri", "*.rc", "*.qrc",
    "src/", "ModReader/", "translations/", "images/", "*.ini",
]

PLACEHOLDER_RE = re.compile(r"\{\{([A-Z_]+)\}\}")


class StampError(RuntimeError):
    pass


def git(*args):
    try:
        out = subprocess.check_output(["git"] + list(args), cwd=ROOT,
                                      stderr=subprocess.PIPE)
    except (subprocess.CalledProcessError, OSError) as exc:
        raise StampError("git %s failed: %s" % (" ".join(args), exc))
    return out.decode("utf-8").strip()


def provenance():
    """Resolve every stamped value. Any gap is a hard failure."""
    head = git("rev-parse", "HEAD")
    if not re.fullmatch(r"[0-9a-f]{40}", head):
        raise StampError("could not resolve HEAD (got %r)" % head)
    head_short = git("rev-parse", "--short", "HEAD")

    baseline = git("log", "-1", "--format=%H", "--", *APP_PATHS)
    if not re.fullmatch(r"[0-9a-f]{40}", baseline or ""):
        raise StampError(
            "could not resolve the application baseline commit from paths: %s"
            % " ".join(APP_PATHS))
    baseline_short = git("log", "-1", "--format=%h", "--", *APP_PATHS)

    return {
        "APPLICATION_BASELINE_COMMIT": baseline_short,
        "APPLICATION_BASELINE_COMMIT_FULL": baseline,
        "DOCUMENTATION_SOURCE_COMMIT": head_short,
        "DOCUMENTATION_SOURCE_COMMIT_FULL": head,
        "DOCUMENT_BUILD_TIMESTAMP": datetime.datetime.now(
            datetime.timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        "DOCUMENT_VERSION": MANUAL_SET_VERSION,
    }


def stamp_to(out_dir):
    """Copy docs/manual/*.md into out_dir with placeholders substituted.

    The source tree is only ever READ.
    """
    values = provenance()
    if os.path.isdir(out_dir):
        shutil.rmtree(out_dir)
    os.makedirs(out_dir)

    # Diagrams are referenced relatively from the Markdown, so they must sit
    # beside the staged copies.
    src_diagrams = os.path.join(MAN, "diagrams")
    if os.path.isdir(src_diagrams):
        shutil.copytree(src_diagrams, os.path.join(out_dir, "diagrams"))
    src_images = os.path.join(MAN, "images")
    if os.path.isdir(src_images):
        shutil.copytree(src_images, os.path.join(out_dir, "images"))
    src_shared = os.path.join(MAN, "_shared")
    if os.path.isdir(src_shared):
        shutil.copytree(src_shared, os.path.join(out_dir, "_shared"))

    stamped = []
    for name in sorted(os.listdir(MAN)):
        if not name.endswith(".md"):
            continue
        # Read and write as UTF-8 explicitly so German characters survive.
        text = io.open(os.path.join(MAN, name), encoding="utf-8").read()

        def repl(m):
            key = m.group(1)
            if key not in values:
                raise StampError("%s: unknown placeholder {{%s}}" % (name, key))
            return values[key]

        text = PLACEHOLDER_RE.sub(repl, text)

        leftover = PLACEHOLDER_RE.findall(text)
        if leftover:
            raise StampError("%s: unresolved placeholders %s" % (name, leftover))

        io.open(os.path.join(out_dir, name), "w",
                encoding="utf-8", newline="\n").write(text)
        stamped.append(name)

    if not stamped:
        raise StampError("no Markdown sources found in %s" % MAN)
    return values, stamped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="staging directory to write stamped copies to")
    ap.add_argument("--print-json", action="store_true",
                    help="print the resolved provenance values and exit")
    args = ap.parse_args()

    try:
        if args.print_json:
            print(json.dumps(provenance(), indent=2))
            return 0
        if not args.out:
            ap.error("--out is required unless --print-json is given")
        values, stamped = stamp_to(args.out)
    except StampError as exc:
        sys.stderr.write("STAMP FAILED: %s\n" % exc)
        return 2

    print("application baseline commit : %s" % values["APPLICATION_BASELINE_COMMIT"])
    print("documentation source commit : %s" % values["DOCUMENTATION_SOURCE_COMMIT"])
    print("build timestamp             : %s" % values["DOCUMENT_BUILD_TIMESTAMP"])
    print("manual set version          : %s" % values["DOCUMENT_VERSION"])
    print("documents stamped           : %d -> %s" % (len(stamped), args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
