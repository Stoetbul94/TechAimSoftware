# Tech Aim Manual — Review Findings

Document version 1.0 (P0-J) · Application commit `3741980`

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
`[PHYSICAL HARDWARE DEPENDENT]` — but it is a gap an operator will hit
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

**F-13 (Medium) — no PDF output yet.** The manuals are Markdown only. No PDF
pipeline has been run, so pagination, orphan headings, glyph coverage and
grayscale legibility are untested.

**F-14 (Low) — audience labels are only in the operator manual.** The Quick
Start and Troubleshooting guides do not carry 🎯/🧑‍🏫/🛠 markers.

---

## Summary

| Severity | Count | Nature |
|---|---|---|
| High | 5 | installer procedure, German terminology check, German layout, screenshots, (F-08/F-09 blocking German beta) |
| Medium | 7 | interpretation guidance, worked example, pattern illustrations, data paths, connection setup, partial German manual, diagrams, PDF |
| Low | 2 | cosmetic/structural |

**The text is accurate and honest about its own limits. What it lacks is
visual material and everything downstream of the installer.** No finding
indicates that a documented procedure is *wrong* — the risk is concentrated in
what has not yet been seen or captured.
