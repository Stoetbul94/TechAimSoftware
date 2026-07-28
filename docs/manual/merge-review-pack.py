# -*- coding: utf-8 -*-
"""Merge the Tech Aim review pack into one bookmarked PDF, and validate it.

Called by build-review-pack.ps1 with a JSON plan. Merging is lossless: pypdf
copies page objects, so embedded fonts and images keep their original quality.

Also writes:
    TechAim_Manual_Review_Manifest.json
and performs the automated page-render checks (never a substitute for the
human visual review).
"""
import hashlib
import io
import json
import os
import subprocess
import sys

try:
    from pypdf import PdfReader, PdfWriter
except ImportError:
    sys.stderr.write("pypdf is required: pip install pypdf\n")
    sys.exit(2)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
COMBINED = "TechAim_All_Manuals_Review.pdf"


def sha256(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def git(*args):
    return subprocess.check_output(["git"] + list(args), cwd=ROOT).decode().strip()


def pdf_facts(path):
    """Structural facts about a PDF. Never a visual judgement."""
    r = PdfReader(path)
    pages = len(r.pages)
    fonts, text_chars, empty_pages, bad_pages = set(), 0, [], []
    for i, pg in enumerate(r.pages, 1):
        try:
            res = pg.get("/Resources") or {}
            fdict = res.get("/Font") or {}
            try:
                fdict = fdict.get_object()
            except Exception:
                pass
            for fk in (fdict or {}):
                try:
                    fo = fdict[fk].get_object()
                    fonts.add(str(fo.get("/BaseFont", fk)))
                except Exception:
                    fonts.add(str(fk))
            t = pg.extract_text() or ""
            text_chars += len(t.strip())
            if not t.strip() and "/XObject" not in str(res):
                empty_pages.append(i)          # no text AND no image
        except Exception as exc:
            bad_pages.append("%d:%s" % (i, exc))
    try:
        outlines = len(r.outline or [])
    except Exception:
        outlines = 0
    links = 0
    for pg in r.pages:
        try:
            for a in (pg.get("/Annots") or []):
                if str(a.get_object().get("/Subtype")) == "/Link":
                    links += 1
        except Exception:
            pass
    return {
        "pageCount": pages,
        "fileSizeBytes": os.path.getsize(path),
        "sha256": sha256(path),
        "fontsEmbedded": len(fonts) > 0,
        "fontCount": len(fonts),
        "textSelectable": text_chars > 200,
        "extractedTextChars": text_chars,
        "bookmarkCount": outlines,
        "internalLinkCount": links,
        "emptyPages": empty_pages,
        "unreadablePages": bad_pages,
    }


def main():
    plan = json.load(io.open(sys.argv[1], encoding="utf-8-sig"))
    review = plan["reviewDir"]
    items = plan["items"]

    writer = PdfWriter()

    def append(path):
        start = len(writer.pages)
        for pg in PdfReader(path).pages:
            writer.add_page(pg)          # object copy: no re-encoding
        return start

    append(plan["cover"])
    writer.add_outline_item("Review Pack — Cover", 0)

    diagram_total = 0
    for idx, it in enumerate(items, 1):
        pdf = os.path.join(review, it["file"])
        if not os.path.isfile(pdf):
            sys.stderr.write("MISSING: %s\n" % pdf)
            return 1
        d_at = append(it["divider"])
        parent = writer.add_outline_item(
            "%d. %s (%s)" % (idx, it["title"], it["lang"]), d_at)
        m_at = append(pdf)
        writer.add_outline_item("Document start", m_at, parent=parent)

        # Chapter bookmarks from the manual's own outline, offset into the
        # combined document.
        try:
            src = PdfReader(pdf)
            pagemap = {src.pages[i].indirect_reference.idnum: i
                       for i in range(len(src.pages))}
            for o in (src.outline or []):
                if isinstance(o, list):
                    continue
                try:
                    num = o.page.idnum if hasattr(o, "page") else None
                    p = pagemap.get(num)
                    if p is not None:
                        writer.add_outline_item(str(o.title), m_at + p, parent=parent)
                except Exception:
                    continue
        except Exception:
            pass

    out = os.path.join(review, COMBINED)
    with open(out, "wb") as fh:
        writer.write(fh)

    # ── manifest ────────────────────────────────────────────────────────
    entries = []
    for it in items:
        p = os.path.join(review, it["file"])
        f = pdf_facts(p)
        f.update({
            "filename": it["file"],
            "documentTitle": it["title"],
            "language": it["lang"],
            "germanBeta": it["lang"].startswith("Deutsch"),
            "applicationBaselineCommit": plan["appCommit"],
            "documentationSourceCommit": plan["docCommit"],
            "buildTimestamp": None,
            "generationTool": "pandoc + headless Chromium (--print-to-pdf)",
            "visualValidationStatus": "GENERATED - HUMAN VISUAL CHECK REQUIRED",
            "absolutePath": os.path.abspath(p),
        })
        entries.append(f)

    cf = pdf_facts(out)
    cf.update({
        "filename": COMBINED,
        "documentTitle": "Tech Aim Manuals Review Pack (combined)",
        "language": "English + Deutsch (Beta)",
        "germanBeta": True,
        "applicationBaselineCommit": plan["appCommit"],
        "documentationSourceCommit": plan["docCommit"],
        "buildTimestamp": None,
        "generationTool": "pypdf merge of pandoc + headless Chromium output",
        "visualValidationStatus": "GENERATED - HUMAN VISUAL CHECK REQUIRED",
        "absolutePath": os.path.abspath(out),
    })
    entries.append(cf)

    # Diagram presence: the operator manual is the only document carrying them.
    build_manifest = os.path.join(
        ROOT, "docs", "manual", "output", "TechAim_Manual_Build_Manifest.json")
    if os.path.isfile(build_manifest):
        bm = json.load(io.open(build_manifest, encoding="utf-8-sig"))
        diagram_total = bm.get("diagramsEmbedded", 0)

    stamp = None
    if os.path.isfile(build_manifest):
        stamp = json.load(io.open(build_manifest, encoding="utf-8-sig")).get("buildTimestamp")
    for e in entries:
        e["buildTimestamp"] = stamp

    manifest = {
        "schema": "techaim.manual-review-manifest/1",
        "product": "Tech Aim Electronic Target Control",
        "productVersion": "0.9.0",
        "releaseChannel": "Pre-Beta Validation",
        "applicationBaselineCommit": plan["appCommit"],
        "documentationSourceCommit": plan["docCommit"],
        "repositoryHead": git("rev-parse", "HEAD"),
        "buildTimestamp": stamp,
        "reviewFolder": os.path.abspath(review),
        "combinedPdf": os.path.abspath(out),
        "diagramsEmbedded": diagram_total,
        "screenshotsEmbedded": 0,
        "screenshotNote": ("No screenshots exist. None are embedded and no "
                           "placeholder image is presented as approved artwork."),
        "overallStatus": "GENERATED - HUMAN VISUAL CHECK REQUIRED",
        "documents": entries,
    }
    mpath = os.path.join(review, "TechAim_Manual_Review_Manifest.json")
    io.open(mpath, "w", encoding="utf-8", newline="\n").write(
        json.dumps(manifest, indent=2, ensure_ascii=False))

    # ── automated render checks ────────────────────────────────────────
    print("\n=== automated page checks (NOT a visual review) ===")
    problems = 0
    for e in entries:
        flags = []
        if e["unreadablePages"]:
            flags.append("UNREADABLE %s" % e["unreadablePages"]); problems += 1
        if e["emptyPages"]:
            flags.append("EMPTY %s" % e["emptyPages"]); problems += 1
        if not e["fontsEmbedded"]:
            flags.append("NO FONTS"); problems += 1
        if not e["textSelectable"]:
            flags.append("TEXT NOT SELECTABLE"); problems += 1
        print("%-40s %3dp  fonts=%-2d text=%-6d bmk=%-3d links=%-4d %s"
              % (e["filename"], e["pageCount"], e["fontCount"],
                 e["extractedTextChars"], e["bookmarkCount"],
                 e["internalLinkCount"], ("OK" if not flags else " | ".join(flags))))
    print("\ncombined: %s (%d pages)" % (out, cf["pageCount"]))
    print("manifest: %s" % mpath)
    print("problems: %d" % problems)
    return 0


if __name__ == "__main__":
    sys.exit(main())
