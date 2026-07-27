# Tech Aim Manual — Review Findings

Document version 1.1 (P0-J refinement) · Application commit `84db7a2`

A usability review of the P0-J documentation from four reader perspectives.
Findings are recorded honestly, including where the documentation is currently
weak.

## Method

The manuals were read as each audience would read them, looking for the answer
to a specific question that audience would actually have. This is a
**desk review** — no reader was observed using the documents against a running
build.

---

## Reader 1 — Athlete with basic computer knowledge

| Question | Result |
|---|---|
| Can I start and finish one session without reading everything? | **Yes** — Quick Start §4–17 plus the first-session checklist. |
| Do I understand sighters vs counted shots? | **Yes** — stated in Quick Start §13 and repeated per programme. |
| Do I understand my training summary? | **Partly** — see F-01. |
| Can I export a PDF? | **Yes.** |

**F-01 (Medium) — metric names are defined, but not interpreted.**
The glossary defines *group diameter* and *MPI*, and Part 8 lists them, but an
athlete is not told what a *good* or *typical* value looks like. That is
partly deliberate — Tech Aim avoids unsupported claims — but the athlete is
left without a frame of reference.
*Recommendation:* add a short "what this number is for" line per metric that
describes **use** ("compare it against your own previous sessions") without
claiming a target value. Needs coach input.

**F-02 (Low) — "Shot 0 of N" is explained but never shown.**
The reader meets the phrase before seeing it in context. Resolved when SS-08
and SS-17 are captured.

---

## Reader 2 — Coach familiar with shooting, not the software

| Question | Result |
|---|---|
| Can I understand measured patterns without assuming cause? | **Yes** — stated in Part 7 and enforced throughout Part 10. |
| Do I understand call accuracy? | **Yes** — including the median-vs-average point and the explicit warning against reading call bias as a sight adjustment. |
| Do I understand shot rhythm? | **Yes** — thresholds are stated exactly, with the caveat that a label alone proves nothing. |
| Can I compare repeated position work? | **Yes** — the compare-against-itself rule is stated in Part 11. |

**F-03 (Medium) — no worked example anywhere.**
The manual explains every metric but never walks through one real summary
end-to-end. A coach's fastest route to understanding is a single annotated
example.
*Recommendation:* add a worked example to Part 11 using Demo data once SS-20
is captured.

**F-04 (Low) — Group Pattern descriptions are listed, not illustrated.**
Ten pattern names with no pictures. Each needs a small illustrative plot.
*Recommendation:* add DG-plots or Demo screenshots per pattern.

---

## Reader 3 — Range operator installing and operating the system

| Question | Result |
|---|---|
| Can I diagnose "no shot received"? | **Yes** — decision tree A, which correctly leads with mode and phase, the two most common non-faults. |
| Do I know what to send to support? | **Yes** — stated in Quick Start §20 and at the head of the troubleshooting guide. |
| Can I identify the installed version? | **Yes** — Settings ▸ ABOUT / BUILD, with the commit. |
| Can I install it? | **No** — see F-05. |
| Do I know where the data lives? | **Partly** — see F-06. |

**F-05 (High, expected) — there is no installation procedure.**
Installation, update, rollback, uninstall and every Windows security
interaction are `[WINDOWS RC1 DEPENDENT]`. An operator cannot deploy from this
documentation alone. This is a known consequence of the installer not
existing; it is **the single largest documentation gap**.

**F-06 (Medium) — data locations are described, not specified.**
Part 17 names the *kind* of location but not exact paths, pending RC1. An
operator planning backup needs the exact path.
*Recommendation:* fill in at RC1.

**F-07 (Medium) — target connection setup is not documented.**
Section 2 of the troubleshooting guide tells an operator how to react when the
connection fails, but no document explains how to *configure* it in the first
place (COM port, protocol settings). This is genuinely unverified —
`[PHYSICAL TARGET DEPENDENT]` — but it is a gap an operator will hit
immediately.
*Recommendation:* write it during hardware validation.

---

## Reader 4 — German-speaking evaluator using the beta translation

| Question | Result |
|---|---|
| Is the German status honest? | **Yes** — coverage, mixed-language reality and the unwrapped-strings limit are all stated plainly. |
| Can I find a control the manual names in German? | **Yes** — English UI labels are given in brackets throughout. |
| Is terminology consistent with the UI? | **Believed yes**, unverified — F-08. |
| Can I complete a session in German? | **Probably**, unverified — F-09. |

**F-08 (High) — German terminology has not been checked against the running
UI.** The German manuals follow the glossary and the catalogue, but nobody has
compared a German manual sentence against the German screen. A term could be
correct in the glossary and still not match what the UI shows.
*Recommendation:* do this as the first step of native review.

**F-09 (High) — German layout is unverified.** Long compounds
(`Streukreisdurchmesser`, `Positionswechsel`) are the most likely source of
clipping, and no German screen has been seen at any resolution.

**F-10 (Medium) — the German operator manual is partial by design.**
Parts 5–6 and 12, 15–20 are summarised and point to the English master. This
is disclosed at the top of the document rather than hidden, but a
German-only reader still cannot work solely from German.

---

## Cross-cutting findings

**F-11 (High) — no screenshots at all.** Thirty are specified; none captured.
Several procedures (main-screen regions, callouts, the "no timer / no red
`000`" evidence) depend on them.

**F-12 (Medium) — no diagrams.** Eleven are specified and described in text;
none rendered.

**F-13 (Medium) — HTML generates; PDF does not.** The Pandoc pipeline produces
all six documents as HTML cleanly. **PDF output requires a LaTeX engine that is
not installed**, so pagination, orphan headings, glyph coverage and grayscale
legibility remain untested.
*Recommendation:* install a PDF engine and run `build-manuals.ps1 -Format pdf`.

**F-14 (Low) — audience labels are only in the operator manual.** The Quick
Start and Troubleshooting guides do not carry 🎯/🧑‍🏫/🛠 markers.

**F-15 (High) — supplied brand artwork conflicts with the product.** Two SVG
files (`logo.svg`, `logo-mark.svg`) were supplied from outside the repository
using a slate/amber palette (`#1f2937` / `#f59e0b`). The Tech Aim application
uses red `#C40046` throughout its interface and on every exported report.
Adopting them would put a **third** visual identity in front of the reader,
and both are plain Arial-text placeholders rather than finished artwork. They
are recorded as CANDIDATE — BRAND APPROVAL REQUIRED in
`_shared/brand-assets.md` and are **not used**; the manuals continue to use
the wordmark the application itself ships.
*Recommendation:* decide the brand direction at product level and apply it to
the application first, then to the manuals — not the other way round.

**F-16 (High) — there is no approved Windows application icon.** No `.ico`
exists, so screenshot SS-31 is registered PENDING — BLOCKED. No icon has been
invented or mocked. Blocks a professional-looking installer at RC1.

---

## Summary

| Severity | Count | Nature |
|---|---|---|
| High | 7 | installer procedure (F-05), German terminology check (F-08), German layout (F-09), screenshots (F-11), brand-artwork conflict (F-15), no application icon (F-16) |
| Medium | 7 | interpretation guidance, worked example, pattern illustrations, data paths, connection setup, partial German manual, diagrams, PDF engine |
| Low | 2 | cosmetic/structural |

**The text is accurate and honest about its own limits. What it lacks is
visual material and everything downstream of the installer.** No finding
indicates that a documented procedure is *wrong* — the risk is concentrated in
what has not yet been seen or captured.
