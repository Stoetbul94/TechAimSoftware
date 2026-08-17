# SETA product line — identity boundary and competition selection

Branch `product/seta`. This document records two decisions that a later phase
must not silently reverse.

---

## 1. Brand / product name is NOT the legal publisher

Three separate facts, deliberately kept in three separate fields:

| Fact | Field | Tech Aim | SETA |
|---|---|---|---|
| What the product is called | `displayName` / `fullProductName` | Tech Aim | **SETA** |
| Who publishes the software | `legalPublisher` | JAC SHOOTING SOLUTIONS (PTY) LTD | **JAC SHOOTING SOLUTIONS (PTY) LTD — unchanged** |
| Which colour/asset package | `BuildFlavour` | `TECH_AIM` | `TECH_AIM` |

Re-branding a product must never silently re-attribute who published the
software. The publisher is a legal fact, not a skin. It changes only when an
authoritative business or contractual requirement is supplied — none has been.

`BuildFlavour::SetaOem` is a **different question** and remains reserved and
unbuildable: it is the full OEM *colour system and asset package*, and no SETA
palette exists. `BRAND_SETA` is the narrower thing that ships today — name,
mark and data namespace on the approved Tech Aim colour system. The startup log
prints both (`brand SETA · flavour TECH_AIM`) so the two are never conflated.

### Brand assets

`images/logo/seta.png` already existed in the repository, is already in
`images.qrc` and already brands the printed report. **No logo was invented.**

SETA supplied one mark. There is no white-on-dark and no single-ink variant, so
`BrandPackage::missingAssets()` reports both as outstanding rather than falling
back to the Tech Aim logo — putting another company's mark in a SETA header or
on a SETA report would be worse than an imperfect one.

**SETA BRAND ASSETS REQUESTED:** a white/knockout variant for dark surfaces, a
single-ink variant for print, and a Windows `.ico`.

---

## 2. SETA user data is separate from Tech Aim user data

### The audit

Qt resolves everything mutable from the organisation/application pair set in
`main.cpp`:

```
QStandardPaths::AppLocalDataLocation
  = %LOCALAPPDATA%\<organisationName>\<applicationName>
QSettings()  (default ctor)
  = HKCU\Software\<organisationName>\<applicationName>
```

Before this change both came from `organisationName` alone, so **an installed
Tech Aim build and an installed SETA build would have read and written the same
locations** for all of:

| Data | Where |
|---|---|
| Sessions (current / archive / corrupt) | `…\Sessions\*` — `StoragePaths` |
| Recovery + replay state | same root |
| Reports, exports, backups, derived indexes | `…\Reports`, `…\Exports`, … |
| Logs, support bundles | `…\Logs`, `…\SupportBundles` |
| Athlete name / last session state | session store under the same root |
| Remembered target fingerprints | `QSettings` group `TargetDevice` |
| EULA acceptance, last-used folder | legacy `QSettings("Seta"/"Tachus", "shootingApp")` |
| `config.ini` | next to the executable — already separate per install |

**Answer: YES, they would have shared.** That is undesirable for a product
line, so SETA now takes its own namespace.

### The decision

| | Tech Aim | SETA |
|---|---|---|
| `organisationName` (vendor) | `TechAim` | `TechAim` — **shared, deliberately** |
| `applicationStorageName` (product) | `TechAim` | **`TechAimSETA`** |
| Data root | `%LOCALAPPDATA%\TechAim\TechAim` | `%LOCALAPPDATA%\TechAim\TechAimSETA` |
| Registry scope | `…\TechAim\TechAim` | `…\TechAim\TechAimSETA` |
| `brandSettingsScope` (legacy keys) | *(empty — historical scope kept)* | `TechAimSETA` |
| `applicationId` | `za.co.techaim.electronic-target-control` | `za.co.techaim.seta.electronic-target-control` |
| `executableBaseName` | `TechAim` | `TechAim` — **shared, deliberately** |

Two things are shared on purpose:

- **The vendor folder.** The vendor really is the same. Everything mutable
  *inside* it is not.
- **`executableBaseName`**, because it also names the single-instance lock.
  One machine drives one target, so a SETA build and a Tech Aim build must
  still refuse to run at the same time.

`applicationStorageName` defaults to `organisationName`, so Tech Aim's existing
data root does not move by one byte.

### No silent migration

There is **no** automatic copy of Tech Aim data into the SETA namespace. A
fresh SETA install starts clean. Session and journal **file formats stay
compatible**, so a deliberate import feature remains possible later — but it
would be a feature, with its own approval, not a side effect of branding.

One visible consequence, and it is correct: the first SETA launch shows the
EULA again, because acceptance is recorded in the SETA scope and that scope is
new. Nothing else is re-prompted.

---

## 3. Hierarchical competition selection

`SetaCompetitionSelector.qml` is the SETA production selection path:
**RULE SET → DISCIPLINE → PROGRAMME**, with a breadcrumb and Back at every
level. Step 3 is skipped when a discipline has exactly one programme, so the
ISSF path is no longer than it was.

### One authoritative result

The only committed value is `programmeId`. `CompetitionCatalogue.runtimeConfig()`
turns it into the **existing** `gameRange` / `gameMode` / `gameEvent` state.
There is no second set of match-configuration rules: the event index it returns
is the same index the legacy controls already set.

Nothing is written while browsing. Rule set → discipline → back → programme
touches no shooting configuration; only a commit does.

### Equivalence (SETA-INT-001)

All 48 catalogue programmes are compared. For each one, the new path must
produce the same range, weapon, event index, shot count, target standard and
paper variant as the legacy path — and, decisively, the legacy index
arithmetic applied to the new result must land on **the same ListModel row**,
because that row is what sets the shot count and the displayed programme.

### Paper mode is part of the question

The catalogue holds both the standard and the 15-shot-paper variant of every
preset; the running application uses exactly one. Every level of the hierarchy
is filtered by `APPSETTINGS.getIs15Shoot()`, so the same preset is never
offered twice and nothing is offered that cannot be run.

A real consequence: **in 15-shot paper mode there is no 60-shot entry**, so no
ISSF course exists and the ISSF rule set correctly disappears.

### Legacy path preserved

The weapon / distance / event controls are **not deleted**. They are gated on
`!setaSelection` and remain the rollback and reference path until this has been
through an integrated approval; a later cleanup removes them. Only one selector
is ever on screen.

### Not owned by the hierarchy

3 Positions and the two Finals have **no catalogue entry** and stay on their
existing controls. Inventing entries for them would put programmes in the
hierarchy that the qualification engine cannot run.

### DSB

DSB is not offered, and a test asserts it. The navigation begins showing it
automatically the moment catalogue entries with `rulesetId: "dsb"` exist. No
empty DSB section is displayed. Nothing about German programmes is implemented:
even Rule 1.20 Luftgewehr 3-Stellung has only a reported name, number and
20/20/20 — preparation time, sighting policy, position-change rules, match
timing, scoring mode and applicable classes are unknown.

### Language

`programmeId` is identical under a German translator, and the selector compares
no translated string (QML-LANG-001 preserved).

**Known gap:** the selector's new UI strings are not present in any
`translations/*.ts`, so a German UI currently renders the English source text
for them. That is missing translation *content*, not a logic defect — and it is
harmless precisely because no logic depends on the label.

---

## Outstanding for SETA

- White/knockout logo, single-ink logo, Windows `.ico`.
- Legal publisher, if SETA is to be named as publisher rather than JAC.
- German translation of the selector strings.
- DSB Sportordnung rule detail before any German programme is added.
