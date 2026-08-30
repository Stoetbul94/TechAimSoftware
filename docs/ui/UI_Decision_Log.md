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

---

## UI-DEC-013 — The reference image is the visual authority for the match-rifle silhouette

| | |
|---|---|
| **Date** | 2026-08-11 |
| **Status** | ACCEPTED — **amended in part by UI-DEC-014** (barrel thickness and the ventilation slots) |
| **Commit** | this change |
| **Approved by** | **ARNOLD BAILIE, 2026-08-11**, on the 132 px production tile |

**Decision.** `docs/ui/issf-match-rifle-reference.png` is the visual authority
for the rifle silhouette in `DisciplineArt.qml`. The silhouette is traced from
that image by measurement, not drawn from description, and three properties of
it are binding on any future revision:

1. **Cubic segments for every organic edge.** The butt underside, grip,
   buttplate hook and forend taper are curves. Only machined parts — receiver,
   barrel, sights — are straight lines, because on the real rifle they are.
   An all-straight-segment silhouette is what made the previous two attempts
   read as angular and chunky, and it is not an acceptable starting point.
2. **The cheek piece and buttplate are separate subpaths with visible gaps.**
   Absorbed into the stock outline they stop identifying the rifle at all.
   The cheek piece floats above the comb on two posts; the buttplate stands
   off the butt on two prongs and hooks below the stock line.
3. **A slender barrel.** Exposed barrel (forend nose → muzzle) measured on the
   rendered path is **34.5 of 126 grid units — 27.4% of overall length** — at
   **1.39 units across the plain section and 2.55 at the muzzle sleeve**, i.e.
   **13.5:1 measured on the thickest point** and 24.8:1 on the plain section.
   A short, thick barrel is the strongest military cue there is.

**Reasoning.** Two previous silhouettes were rejected for reading as a service
weapon on a sport-shooting product. Both failed for reasons that are describable
and therefore checkable, so they are written down as constraints rather than
left to taste. Naming a single reference image also stops the silhouette from
drifting one revision at a time.

**Measured fidelity.** Intersection-over-union against the reference silhouette
at matched scale is **0.819**. The residual is deliberate and is listed in
UI-ART-001: the sight rail and the forend vents are thickened for legibility at
the ~92 px width the left pane actually draws, and the forend nose is 1.6 units
short of the reference to hold the barrel proportion above 27%.

**Brief tolerance, recorded honestly.** The brief asked for ~29% at ~12:1.
The delivered silhouette is 27.4% at 13.5:1 — marginally shorter and
marginally slimmer than asked, and within 1.6 grid units (about 1.2 px at tile
size) of the reference's own proportion. The reference was treated as the
authority where the two disagreed, because the brief named it as such.

**Affected areas.** `DisciplineArt.qml` only. The plate composition — ring
motif, position glyphs, palette, tile — is unchanged apart from the rifle's
placement box, which was re-proportioned from 96×28 to 126×30 to match the
reference's 4.2:1 aspect.

**Affected decisions.** UI-DEC-001 through UI-DEC-012 are all **preserved**.
UI-DEC-003 (the authoritative palette) is honoured: the artwork introduces no
colour and continues to take `accent` / `ink` / `muted` from its caller.

**Supersedes.** Nothing.

---

## UI-DEC-014 — The production tile, not the reference or the score, is the acceptance gate for discipline artwork

| | |
|---|---|
| **Date** | 2026-08-11 |
| **Status** | ACCEPTED |
| **Commit** | this change |
| **Approved by** | **ARNOLD BAILIE, 2026-08-11** |

**Decision.** Discipline artwork is accepted or rejected on a render at the
size the application actually draws it — for the left pane that is a **132 px
plate, a 92 px rifle**. Three rules follow, and they bind future revisions:

1. **No feature below one device pixel at production size.** At 92 px the
   authoring grid scales by 0.73, so anything under ~1.4 grid units cannot
   resolve and renders as grey. Such a feature is either thickened until it
   resolves or removed. It is never left in on the grounds that it is in the
   reference: the reference is a 1212 px illustration and can afford detail the
   tile cannot.
2. **Silhouette before internal detail.** Where the two compete at tile size,
   the outer shape and the identifying sub-shapes — buttplate, cheek piece,
   grip, diopter, front tunnel, barrel — win. Internal texture is the first
   thing to go.
3. **Similarity metrics are diagnostic, never approval.** IoU against a
   reference mask is useful for catching drift and proving a trace is faithful.
   It cannot see mush. A silhouette can score 0.82 and still read wrong at
   92 px, which is exactly what happened here. Only a human decision on a
   production-size render closes an artwork defect.

**Amends UI-DEC-013.** Point 3 of UI-DEC-013 fixed the barrel at 1.39 units on
the plain section (13.5:1 at the thickest point) and the header comment
described a *ventilated* forend. Both were measured off the reference and both
failed at tile size. The barrel is now **1.95 units, 17:1 over the exposed
length**, and the two ventilation slots are **removed**. UI-DEC-013's binding
intent — cubic organic edges, cheek piece and buttplate as separate subpaths
with real air, and a barrel that is unmistakably slender — is **preserved in
full**; only the two numbers that could not survive production rendering are
amended. UI-DEC-013 stays in the log with its status updated.

**Reasoning.** Two silhouettes were rejected for looking military, a third
measured well and still read soft, and the cause turned out to be partly a
shared-renderer defect (UI-ICON-001) and partly detail below the resolution of
the tile. Writing the gate down stops the next revision from being judged on a
2× view or a similarity number again.

**Affected areas.** `DisciplineArt.qml`. The plate composition — ring motif,
position glyphs, palette, tile geometry — is unchanged.

**Affected decisions.** UI-DEC-001 through UI-DEC-012 are **preserved**.
UI-DEC-013 is **amended in part**, as set out above.

**Supersedes.** Nothing outright; amends UI-DEC-013 points 3 and its forend
description.

---

## UI-DEC-015 — Translations are presentation-only and never participate in product decisions

| | |
|---|---|
| **Date** | 2026-08-11 |
| **Status** | ACCEPTED |
| **Commit** | this change |
| **Approved by** | **ARNOLD BAILIE, 2026-08-11** |

**Decision.** A translation may change presentation strings and nothing else.
Translated text must never participate in a decision about **discipline,
target, scoring, acquisition, match or session state**. Every such decision
reads a stable authoritative value — an enum, an id, or a numeric setting.

Concretely, and binding on future languages: no comparison against `qsTr(...)`
or `tr(...)`; no branching on a visible label, a `gameDisplay*` string or an
English literal such as `"PISTOL"`; no index derived from a translated model.
The discipline selectors are `loginPage.gameMode` (0 = pistol, 1 = rifle),
`APPSETTINGS.get10or50mRange()` (10 or 50) and `loginPage.gameSubMode`
(Prone / 3 Positions).

**Reasoning.** QML-LANG-001 was a Severity 1, release-blocking defect: because
the Pistol/Rifle selector compared stored display text against a live
`qsTr()` value, selecting German moved a 10 m Air Pistol session into the rifle
target and scoring branch. A defect that changes a score is not a translation
bug, and the language layer is exactly the wrong place to discover that. The
rule is written down so adding a language can never again be a scoring change,
and `tests/qml` enforces it file-wide rather than at the one site that failed.

**Affected areas.** `CenterPane.qml`, `LeftPanel.qml`, `tests/qml`.

**Affected decisions.** UI-DEC-001 through UI-DEC-014 are all **preserved**.

**Supersedes.** Nothing.


## UI-DEC-016 — Appearance is a token-layer decision, never a per-screen one

| | |
|---|---|
| **Date** | 2026-08-26 |
| **Status** | ACCEPTED |
| **Commit** | `9624eed` |
| **Directed by** | the round brief of 2026-08-26, which required a shared theme authority and expressly ruled out editing colours screen by screen |
| **Visual approval** | **NOT GIVEN.** The architecture was directed; the light appearance beyond the start page has not been seen by the operator. See `Homepage_Acceptance_Checklist.md` A12–A14. |

**Decision.** Light/Dark/System is implemented by resolving every semantic
token in `src/ui/theme/DesignTokens.qml` through a single `isLight`, and by
nothing else. A screen never asks what theme is active in order to pick a
colour; it reads a token by meaning and the token layer answers. The two
permitted exceptions are non-colour choices that a token cannot express — the
current one is the header logo asset, which must be the colour mark on light
chrome and the white mark on dark.

The preference is persisted **per user** (`QSettings` organisation scope,
`ui/appearance`) and never written into the deployed `config.ini`, which
carries range and deployment configuration and ships read-only inside the
release package. Default is **dark** when nothing is stored, so existing
operators keep the appearance they have.

Appearance is **presentation only**. It may not influence acquisition,
scoring, timing, competition rules, session state, COM settings, reports or
paper feed. `tests/qml` asserts the appearance API is absent from the
acquisition, 3P-finals and 10m-finals sources.

**Reasoning.** The product previously carried 240 distinct colour literals
across 66 QML files. The token layer (UI-1) exists precisely so a colour
decision is made once; implementing a second theme by editing screens would
have rebuilt the problem the token layer was created to remove, and would have
left every unmigrated screen silently wrong. Resolving inside the tokens also
makes the switch live for free, because consumers already read them as
bindings.

The dark palette is unchanged value-for-value. A theme feature that also
restyles the existing product is two changes wearing one commit message.

**Affected areas.** `src/ui/theme/DesignTokens.qml`, `Theme.qml`,
`LoginPage.qml`, `Header.qml`, `appsettings.{h,cpp}`, `tests/qml`.

**Affected decisions.** UI-DEC-001 through UI-DEC-015 are all **preserved**.
UI-DEC-005 (one primary Tech Aim logo, in the application shell) is preserved
and refined: it remains one logo in one place; only which variant of that one
mark is drawn now depends on the chrome behind it.

**Supersedes.** Nothing.


---

## UI-DEC-017 — A report destination is chosen by named discipline family, never by one "finals" flag

| | |
|---|---|
| **Date** | 2026-08-30 |
| **Status** | ACCEPTED |
| **Directed by** | the Phase A final reporting round of 2026-08-30, §2 and §3 |
| **Visual approval** | **NOT GIVEN.** The rendered report pages are filed as automated evidence; an operator has not seen them on the machine. |

**Decision.** `ReportWindow` names each report family and selects the view from
the family, not from a generic boolean:

| Session | Report |
|---|---|
| 10 m AR / AP Final (`isFinals10mMatch`) | `Finals10mReportView` — tab 3 |
| 50 m 3P Final (`isFinalsMatch`) | `FinalsReportView` — tab 2 |
| Qualification / Open Practice | Summary + Match — tabs 0 and 1 |
| Training Lab programmes | their own off-screen report renderers, never this window |

`finalsMode` is the union of the two finals families and exists only to hide
the qualification tabs. Which report is shown is decided by `finalsTab`, and
`prepare()` pins the window to that tab whatever the caller asked for — so no
entry point, present or future, can put a Final in front of the wrong model.

**Reasoning.** There are two finals families and they share nothing: a 50 m 3P
Final is 35 shots across kneeling, prone and standing; a 10 m Final is 24 shots
as two 5-shot series and then singles. Before this round `finalsMode` tested
only `isFinalsMatch`, so a 10 m Final fell through to the Summary and Match
tabs — whose own comment says they "carry qualification assumptions that must
never be fed finals data". One generic flag is what made that possible, and
adding a third value to it would have made it possible again.

**Affected areas.** `ReportWindow.qml`, `WindowManager.qml`, `ShootingPage.qml`,
`Finals10mReportView.qml`, `tests/qml`.

**Affected decisions.** UI-DEC-001 through UI-DEC-016 are all **preserved.**

**Supersedes.** Nothing.

---

## UI-DEC-018 — Tech Aim does not present Teiler; the measurement stays

| | |
|---|---|
| **Date** | 2026-08-30 |
| **Status** | ACCEPTED |
| **Directed by** | the Phase A final reporting round of 2026-08-30, §13–§17 |
| **Visual approval** | **NOT GIVEN.** Automated evidence only; see the acceptance checklist. |

**Decision.** No Tech Aim output an operator or athlete can see shows "Teiler".
It was removed outright from the four places that displayed it — the Summary
report metrics, the Match report metrics, the match-report series header, and
the per-shot column of the match shot table — and the shot table's remaining
five columns were re-proportioned to fill the width, so nothing is left blank,
renamed or spaced for an absent value.

`TachusWidget::getTeiler()`, `getTeilerForShoot()` and
`getTeilerForShootOfMatch()` are **unchanged**. Teiler is the mean radial
distance of a shot from centre — a property of the coordinates. Deleting the
measurement because the label is German would have moved a number, which this
decision explicitly does not do.

The decision itself lives in the brand package as
`BrandPackage::showTeilerMetric`, false for Tech Aim. See
`docs/design/Teiler_Presentation_Decision.md` for what a future German-market
edition would have to decide before it could be true.

**Reasoning.** "Teiler" is a German-market figure carried over from the origin
of this codebase. It is not an ISSF concept, it appears in no ISSF result, and
Tech Aim's reports are ISSF-shaped. Hiding it behind a runtime flag would have
left the column widths, the grid cell and the accessor call in place for a
value nobody would ever see; removing it and recording the decision where brand
decisions live keeps the report honest and the re-enablement path explicit.

**Affected areas.** `SummaryReportView.qml`, `MatchReportView.qml`,
`MatchReportInfo.qml`, `src/app/BrandPackage.{h,cpp}`, `tests/qml`,
`tests/reliability/tst_brandpackage.cpp`.

**Affected decisions.** UI-DEC-010 (OEM branding via BrandPackage, one
codebase) is **preserved and applied** — this is the first product decision
recorded in a brand package rather than in a screen. All others preserved.

**Supersedes.** Nothing. No previous entry accepted Teiler as a Tech Aim
metric; it was inherited, never decided.
