# Tech Aim — UI Decision Log

Accepted UI decisions, newest ID last. **A decision is never edited away or
deleted.** To change one, add a new entry that supersedes it and set the old
entry's status to `SUPERSEDED BY UI-DEC-nnn`.

Status values: `ACCEPTED` · `SUPERSEDED BY UI-DEC-nnn` · `PROPOSED` · `DEFERRED`

---

## UI-DEC-001 — Version B is the approved Beta homepage design

| | |
|---|---|
| **Date** | 2026-07-28 |
| **Status** | ACCEPTED |
| **Commit** | `8022033` |

**Decision.** Version B ("Recommended Beta") is the approved homepage
direction for Tech Aim 0.9.0 Beta 1.

**Reasoning.** The UI-0 audit scored the pre-existing homepage 2.5/5 with one
functional defect (content clipped at the target resolution, hiding a whole
discipline). Version B fixes the structural problems — information grouping,
action clarity, clipping — without touching the workflow or the controllers,
which keeps the regression surface to layout only.

**Alternatives considered.**
- *Keep current UI, fix blockers only* — leaves the clipping and the invisible
  primary action, both of which read as unfinished to a tester.
- *Version A (safe polish)* — spacing and typography only; would not have moved
  the primary action out of the clipped column, so the worst defect survives.
- *Version C (stepped workflow)* — restructures the entry flow. Too large for
  Beta and needs its own design review. See UI-DEC-011.

**Affected areas.** `LoginPage.qml` only.

---

## UI-DEC-002 — Homepage structure

| | |
|---|---|
| **Date** | 2026-07-28 |
| **Status** | ACCEPTED |
| **Commit** | `8022033`, corrected in `41c09a3` |

**Decision.** Three regions: **session setup on the left**, **event selection
on the right** (the wider column), a **full-width fixed action bar** at the
bottom, with the **event panel scrolling independently** and the **primary
Start action always visible**.

**Reasoning.** The setup fields were one rigid anchor chain with the actions
pinned to the bottom of the same clipped panel; on a shorter window the chain
overflowed and `clip: true` swallowed the Start button. Moving the actions into
a bar outside every scroll container makes that failure structurally
impossible, rather than merely unlikely.

**Alternatives considered.** Keeping the actions inside the left panel (as in
the original Version B mockup) — rejected because the bar must not be able to
be clipped by panel content.

**Affected areas.** `LoginPage.qml` — `actionBar`, `setupScroll`, `eventScroll`.

---

## UI-DEC-003 — Authoritative Tech Aim palette

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `1bc6b80` (tokens), `f62d289` (homepage) |

**Decision.**

| Token | Value |
|---|---|
| `accentPrimary` | `#A80038` |
| `accentHover` | `#C40046` |
| `accentPressed` | `#80032A` |
| `accentSubtle` | `#2D0A18` |
| `#BF1919` | **logo artwork only** — never a UI accent |

**Reasoning.** The product shipped three competing brand reds (`#a80038` in the
report system, `#e8003d` in the live shooting UI, `#C40046` in Training Lab and
the homepage). The approved logo `images/logo/techaim_color.png` was sampled
directly — 710,403 opaque pixels, of which `#A80038` accounts for 276,718 and
`#BF1919` for 38,687. `#C40046` appears nowhere in it. `#A80038` therefore
matches the approved asset, the existing shared `Theme.qml`, and the largest
existing consumer (the whole report/PDF system).

**Alternatives considered.**
- `#C40046` — the value originally expected in the UI-1 brief; not present in
  the approved artwork, so adopting it would make the application accent
  deliberately differ from the logo.
- `#e8003d` — the live shooting UI's red; matches neither the logo nor the
  recent work.
- `#e6003c` — the UI-0 **concept** accent. Never shipped, explicitly not adopted.

**Affected areas.** `src/ui/theme/DesignTokens.qml`, `src/app/BrandPackage.cpp`,
`LoginPage.qml`.

---

## UI-DEC-004 — Token migration scope is the homepage only

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `1bc6b80`, `f62d289` |

**Decision.** UI-1 migrates the homepage and the components it needs. The
remaining ~20 screens keep their legacy styling; `Theme.qml` retains every
legacy property so they are unaffected.

**Reasoning.** Migrating 66 QML files and 972 colour literals in one pass would
put every screen into a single untestable change, with no automated visual
regression available on this machine.

**Alternatives considered.** Homepage + report system (the largest brand-red
consumer), and homepage + all shared components. Both rejected as scope for
this phase; the report system is already on `#A80038` so it does not block.

**Affected areas.** `Theme.qml`, `LoginPage.qml`. Not: shooting UI, HUDs,
dialogs, Settings, reports.

---

## UI-DEC-005 — One primary Tech Aim logo in the homepage shell

| | |
|---|---|
| **Date** | 2026-07-28 |
| **Status** | ACCEPTED |
| **Commit** | `8022033` |

**Decision.** Product branding lives in the application shell header
(`Header.qml`). The homepage does not repeat the wordmark.

**Reasoning.** `LoginPage` drew a second identity row (target icon, "TECH AIM",
"ELECTRONIC TARGET CONTROL") directly beneath the shell header that already
showed the same mark. Two stacked brand rows read as a rendering fault and cost
~20 px on a screen that was already clipping.

**Affected areas.** `LoginPage.qml` — `headerBar` (74 → 56 px).
`Header.qml` untouched, being shared with every other screen.

**Completed 2026-07-29.** `8022033` removed the duplicated identity row;
`d4674d0` removed the residual logo image. The homepage now renders no logo of
its own — the shell header carries the only Tech Aim mark (UI-HOME-006).

---

## UI-DEC-006 — One authoritative selected-event state

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `41c09a3` |

**Decision.** The selected event card, the Selected Profile summary, the shot
plan, the scoring mode, the Start-button wording and the action-bar recap all
derive from `selectedProgrammeKind()` / `selectedProgrammeName()` /
`selectedProgrammeLabel()` / `startButtonText()`.

**Reasoning.** Each of these previously derived the answer independently, and
the summary was hardcoded to `getDisciplineName() + " — ISSF"` regardless of
the event — so selecting Open Practice still displayed "10m Air Pistol — ISSF".
A practice session must not claim to be an ISSF match.

**Alternatives considered.** Patching the summary string alone — rejected: it
fixes the symptom and leaves four other derivations free to drift.

**Affected areas.** `LoginPage.qml`. **Controller dispatch is unchanged** — the
Start handler still keys off `ptConfirmed` / `cdConfirmed` /
`trainingConfirmed`. These functions are presentation only.

---

## UI-DEC-007 — Independent event-panel scrolling, fixed chrome

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `41c09a3` |

**Decision.** The right event panel scrolls independently. The page heading and
the bottom action bar are fixed and never scroll. No horizontal scrolling.

**Reasoning.** A `ScrollView` sizes itself from its content's implicit height;
with conditionally-visible cards and an Open Practice card that changes height
when selected, that measurement was unreliable and the list clipped instead of
scrolling. A `Flickable` with an explicitly bound `contentHeight` always knows
how tall its content is. Horizontal scrolling is prohibited because it is how a
clipped discipline would hide itself.

**Affected areas.** `LoginPage.qml` — `eventScroll`, `eventColumn`.

---

## UI-DEC-008 — Network Share may not show success without a folder

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `41c09a3` + `d4674d0` |

**Decision (proposed).** An enabled-success state must not be displayed when no
destination folder is selected.

**Reasoning.** The card showed "Share enabled" with an accent border while
nothing could be written anywhere — a success state for a non-working
configuration.

**Alternatives considered.**
- Default the toggle off until a folder exists, opening the picker on enable.
- Keep it on but render an amber "Share incomplete" warning state.
- Leave unchanged.

**Adopted 2026-07-29:** the first option. The toggle derives from whether a
destination folder exists, so an enabled-success state cannot occur without
one; turning sharing on with no folder opens the picker, and the on-but-
unconfigured case renders as an amber "Share incomplete" warning in both the
card and the footer.

**Deliberately NOT gated.** An incomplete share never blocks Start. Sharing is
a convenience; refusing to let an athlete shoot because a results folder is
unset would be a worse failure than not sharing the results.

---

## UI-DEC-009 — One consistent event-card selected-state pattern

| | |
|---|---|
| **Date** | 2026-07-28 |
| **Status** | ACCEPTED |
| **Commit** | `8022033` (grouping) + `d4674d0` (indicator) |

**Decision.** Event cards use one pattern: the whole card is clickable, the
selected card takes an accent border plus tint, the selection indicator sits in
a consistent position, and an arrow means *navigation into another page* — never
selection.

**Reasoning.** Cards mixed a radio indicator (`EventCard`), no indicator at all
(Open Practice), and an arrow (Training Lab, which genuinely navigates).

**Implemented.** Four labelled groups; `EventCard` carries a radio indicator;
the Training Lab arrow is genuine navigation (`practiceView = 1`).

**Completed 2026-07-29.** Open Practice now carries the same radio indicator
in the same position (`d4674d0`), so one pattern covers every card in the list.

---

## UI-DEC-010 — OEM branding via BrandPackage, one codebase

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `1bc6b80` |

**Decision.** A future company/OEM edition selects a different
`BrandPackage` + `BuildFlavour` in the same source tree. No copied application,
no permanent branding branch, no `if (company == X) colour = Y`.

**Reasoning.** A fork diverges; a conditional multiplies. A brand package is a
reviewable unit that carries presentation and identity only, and structurally
cannot reach scoring, rules, analytics, recovery or target communication.

**Affected areas.** `src/app/BrandPackage.{h,cpp}`, `assets/brands/`,
`docs/design/Brand_Flavor_Guide.md`.

**Constraint.** `BuildFlavour::SetaOem` is registered, empty, and refused by
`isFlavourBuildable()`. No OEM appearance is implemented or approved.

---

## UI-DEC-011 — Version C remains a post-Beta concept

| | |
|---|---|
| **Date** | 2026-07-28 |
| **Status** | DEFERRED |
| **Commit** | — |

**Decision.** The Version C stepped workflow (event → athlete → readiness →
shoot) is **not** the current homepage and is not scheduled.

**Reasoning.** It restructures the entry flow, so it needs its own design
review, new manual screenshots throughout and a full regression pass. Version B
solves the Beta problems without that cost.

**Affected areas.** None in the application. The concept lives only in
`manual-preview/ui-audit/concepts/home-screen-A-B-C.html`, which is git-ignored
and stamped **CONCEPT MOCKUP — NOT CURRENT APPLICATION**.

---

## UI-DEC-012 — Version B accepted as the Tech Aim Beta homepage

| | |
|---|---|
| **Date** | 2026-07-29 |
| **Status** | ACCEPTED |
| **Commit** | `d4674d0` (application) |
| **Approved by** | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE** |

**Decision.** The Version B homepage, as built at `d4674d0`, is accepted as
the Tech Aim Beta homepage. **No further homepage styling changes are to be
made unless a new defect is found.**

**Reasoning.** All ten reported defects (UI-HOME-001…010) are closed with a
fix, a passing automated check and human visual approval. The page was
reviewed on screen and approved. Continuing to adjust an approved screen
without a defect to point at is how a settled design drifts.

**Scope of the approval — recorded precisely.** Reviewed at **1536 × 960**,
English, executable SHA-256 `F40BA723…F4588E73`. It does **not** cover
1366 × 768, 1280 × 720 or 1100 × 700 (never opened), the German catalogue
(never run), or interaction stepped through as a test script. Those remain
NOT TESTED in `Homepage_Acceptance_Checklist.md` and must not be upgraded
without someone actually looking.

**Alternatives considered.** Withholding acceptance until all four window
sizes and German were reviewed — rejected: it would block the Beta on checks
that are not defects, and the unreviewed sizes are recorded rather than
assumed.

**Affected areas.** `LoginPage.qml` is closed to styling change. UI-DEC-001
through UI-DEC-011 are all **preserved**; none is superseded by this entry.

**Supersedes.** Nothing.
