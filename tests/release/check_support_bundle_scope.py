#!/usr/bin/env python3
"""SUP-002 - the support collector collects THIS product's data and no other.

Two faults have to stay dead at once, and they pull in opposite directions:

  1. Searching one directory too high (the vendor folder) found no Sessions
     directory and reported 0 journals every time, silently.
  2. Fixing that by searching EVERY product folder under the vendor root put
     Tech Aim's session journals into a SETA support bundle.

So this test asserts both: the SETA journal IS collected, the Tech Aim journal
is NOT, and - because "0 journals" must never again be indistinguishable from
"I looked in the wrong place" - that the bundle reports a missing data root in
different words from an empty one.

Runs the real script against a fixture vendor root. Nothing here touches
%LOCALAPPDATA%.
"""

import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCRIPT = os.path.join(ROOT, "tools", "release", "Make-SupportBundle.ps1")

checks = 0
failures = 0


def check(ok, name, detail=""):
    global checks, failures
    checks += 1
    if ok:
        print("PASS  " + name)
    else:
        failures += 1
        print("FAIL  " + name + ("  " + str(detail) if detail else ""))
    sys.stdout.flush()


def journal(path, session_id, age_hours=48):
    """A minimally plausible journal line - the collector only copies files.

    Backdated on purpose. Case C needs a window that legitimately finds
    nothing, and a file written a second ago falls inside every window. Without
    a stale file, "no recent journals" could not be distinguished from "wrong
    data root" - which is the whole point of this test.
    """
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with io.open(path, "w", encoding="utf-8", newline="") as f:
        f.write(json.dumps({"type": "SessionStarted", "sessionId": session_id}) + "\n")
    stale = time.time() - age_hours * 3600
    os.utime(path, (stale, stale))


def build_fixture(base):
    """Vendor/
         TechAim/Sessions/TechAim-session.jsonl
         TechAimSETA/Sessions/SETA-session.jsonl
    """
    vendor = os.path.join(base, "Vendor")
    journal(os.path.join(vendor, "TechAim", "Sessions", "TechAim-session.jsonl"), "techaim")
    journal(os.path.join(vendor, "TechAimSETA", "Sessions", "SETA-session.jsonl"), "seta")
    os.makedirs(os.path.join(vendor, "TechAim", "Logs"), exist_ok=True)
    os.makedirs(os.path.join(vendor, "TechAimSETA", "Logs"), exist_ok=True)
    return vendor


def run_collector(vendor, out_dir, storage=None, hours=8760, all_products=False):
    cmd = ["powershell", "-ExecutionPolicy", "Bypass", "-NoProfile", "-File", SCRIPT,
           "-VendorRoot", vendor, "-OutDir", out_dir, "-RecentHours", str(hours)]
    if storage:
        cmd += ["-StorageName", storage]
    if all_products:
        cmd += ["-AllProducts"]
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p


def newest_bundle(out_dir):
    zips = [os.path.join(out_dir, f) for f in os.listdir(out_dir) if f.lower().endswith(".zip")]
    if not zips:
        return None
    return max(zips, key=os.path.getmtime)


def bundle_contents(zpath):
    with zipfile.ZipFile(zpath) as z:
        names = z.namelist()
        what = ""
        for n in names:
            if n.endswith("WHAT-WAS-COLLECTED.txt"):
                what = z.read(n).decode("utf-8", "replace")
        ident = ""
        for n in names:
            if n.endswith("release-identity.txt"):
                ident = z.read(n).decode("utf-8", "replace")
    return names, what, ident


def main():
    if os.name != "nt":
        print("SKIP  SUP-002 needs Windows PowerShell")
        print("\n=== 0 checks, 0 failures ===")
        return 0

    check(os.path.isfile(SCRIPT), "SUP-002: the collector script exists", SCRIPT)
    if not os.path.isfile(SCRIPT):
        print("\n=== %d checks, %d failures ===" % (checks, failures))
        return 1

    base = tempfile.mkdtemp(prefix="sup002-")
    try:
        vendor = build_fixture(base)

        # ── A. scoped to SETA ────────────────────────────────────────────
        out = os.path.join(base, "out-seta")
        os.makedirs(out)
        p = run_collector(vendor, out, storage="TechAimSETA")
        check(p.returncode == 0, "SUP-002 A: the collector succeeds when scoped to SETA",
              (p.stderr or "")[-300:])
        z = newest_bundle(out)
        check(z is not None, "SUP-002 A: a bundle was produced")
        if z:
            names, what, ident = bundle_contents(z)
            jsonls = [n for n in names if n.endswith(".jsonl")]
            seta_hit = [n for n in jsonls if "SETA-session" in n]
            techaim_hit = [n for n in jsonls if "TechAim-session" in n]

            check(len(seta_hit) > 0,
                  "SUP-002 A: SETA-session.jsonl is COLLECTED", jsonls)
            check(all(os.path.basename(n).startswith("TechAimSETA-") for n in jsonls),
                  "SUP-002 A: each copy is prefixed with the PRODUCT it came from, "
                  "not with a folder counted upwards from the file", jsonls)
            check(len(techaim_hit) == 0,
                  "SUP-002 A: TechAim-session.jsonl is NOT collected", techaim_hit)
            check(len(jsonls) > 0,
                  "SUP-002 A: journal count is greater than zero", len(jsonls))
            check("TechAimSETA" in what and "data root status" in what,
                  "SUP-002 A: the searched root is explicitly reported in the bundle")
            check("OK - scoped to this product only" in what,
                  "SUP-002 A: and reported as correctly scoped")
            check("Application data root" in ident and "TechAimSETA" in ident,
                  "SUP-002 A: the identity file names the application data root")
            # The regression that started all of this: the vendor folder itself
            # must not be what was searched.
            m = re.search(r"product data roots searched:\s*(.+)", what)
            searched = m.group(1).strip() if m else ""
            check(searched.endswith("TechAimSETA"),
                  "SUP-002 A: the root is the product folder, NOT the vendor folder "
                  "one level above it", searched)

        # ── B. a declared root that does not exist ───────────────────────
        out = os.path.join(base, "out-missing")
        os.makedirs(out)
        p = run_collector(vendor, out, storage="TechAimNoSuchProduct")
        check(p.returncode == 0, "SUP-002 B: a missing data root does not crash the collector")
        z = newest_bundle(out)
        if z:
            names, what, ident = bundle_contents(z)
            jsonls = [n for n in names if n.endswith(".jsonl")]
            check(len(jsonls) == 0, "SUP-002 B: and collects nothing from another product",
                  jsonls)
            check("DATA ROOT MISSING" in what,
                  "SUP-002 B: a MISSING ROOT is reported in those words")
            check("NOT the same as having no recent sessions" in what,
                  "SUP-002 B: and is explicitly distinguished from an empty window")

        # ── C. correct root, nothing in the window ───────────────────────
        out = os.path.join(base, "out-empty")
        os.makedirs(out)
        p = run_collector(vendor, out, storage="TechAimSETA", hours=1)
        z = newest_bundle(out)
        if z:
            names, what, ident = bundle_contents(z)
            jsonls = [n for n in names if n.endswith(".jsonl")]
            check("OK - scoped to this product only" in what,
                  "SUP-002 C: a correct root with nothing recent still reports OK")
            check("DATA ROOT MISSING" not in what and "NOT DECLARED" not in what,
                  "SUP-002 C: and is NOT reported as a root problem")
            check("journals modified in the last 1 h: 0" in what,
                  "SUP-002 C: the empty result is stated as a count, not implied", what[:200])

        # ── C2. the reported SOURCE must be truthful ─────────────────────
        # PowerShell variable names are case-insensitive, so deriving the root
        # into $storageName silently overwrote the $StorageName parameter and
        # every bundle claimed its root came from the command line. The root was
        # still right; only the account of where it came from was wrong - which
        # is exactly the kind of quiet inaccuracy a support bundle must not have.
        out = os.path.join(base, "out-source")
        os.makedirs(out)
        manifest_dir = os.path.dirname(SCRIPT)
        p = run_collector(vendor, out, storage="TechAimSETA")
        z = newest_bundle(out)
        if z:
            names, what, ident = bundle_contents(z)
            check("data root source : command line (-StorageName)" in what,
                  "SUP-002 C2: an explicit -StorageName is reported as coming from "
                  "the command line")
            check("Data root source     : command line (-StorageName)" in ident,
                  "SUP-002 C2: and the identity file agrees")
        # A manifest-driven run cannot be staged here without a packaged
        # manifest beside the script, so the code path is asserted on the source:
        # the source is decided BEFORE the derived name is assigned.
        src0 = io.open(SCRIPT, encoding="utf-8", errors="replace").read()
        i_src = src0.index("$rootSource = if ($StorageName)")
        i_val = src0.index("$dataRootName = if ($StorageName)")
        check(i_src < i_val,
              "SUP-002 C2: the root SOURCE is decided before the derived name is "
              "assigned, so the parameter cannot be shadowed first")
        check("$storageName" not in src0,
              "SUP-002 C2: no lower-cased alias of the parameter survives")

        # ── D. cross-product only when explicitly asked ──────────────────
        out = os.path.join(base, "out-all")
        os.makedirs(out)
        p = run_collector(vendor, out, all_products=True)
        z = newest_bundle(out)
        if z:
            names, what, ident = bundle_contents(z)
            jsonls = [n for n in names if n.endswith(".jsonl")]
            check(any("SETA-session" in n for n in jsonls)
                  and any("TechAim-session" in n for n in jsonls),
                  "SUP-002 D: -AllProducts DOES collect both, for when support wants both",
                  jsonls)
            check("deliberate cross-product collection" in what,
                  "SUP-002 D: and the bundle says that is what happened")

        # ── E. the default is scoped, not everything ─────────────────────
        src = io.open(SCRIPT, encoding="utf-8", errors="replace").read()
        check("[switch]$AllProducts" in src,
              "SUP-002 E: cross-product collection is a switch")
        check(re.search(r"\[switch\]\$AllProducts\s*\)", src) is not None,
              "SUP-002 E: and it is OFF unless asked for - a switch defaults to false")
    finally:
        shutil.rmtree(base, ignore_errors=True)

    print("\n=== %d checks, %d failures ===" % (checks, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
