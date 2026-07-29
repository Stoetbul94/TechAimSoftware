# EULA Replacement Requirements

Document version 1.0 (P0.1) · Application commit `169eef9`

**STATUS: LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA.**

This is an **engineering audit** of how the end-user agreement is currently
shown, stored and accepted. **It does not contain, draft or approve legal
terms.** No binding wording has been written, and none may be committed or
activated without explicit approval from JAC SHOOTING SOLUTIONS (PTY) LTD.

---

## 1. The blocker, precisely

The application displays an end-user agreement as **two rendered images**. Those
images carry a **SETA-era agreement naming an entity other than JAC SHOOTING
SOLUTIONS (PTY) LTD** — the publisher of this product.

An operator who accepts it is therefore agreeing to terms attributed to the
wrong party. This must not be distributed.

**This audit does not reproduce the artwork or its wording.**

## 2. Reachable components

| Component | Location | Notes |
|---|---|---|
| Agreement page | `LoginPage.qml:2197` (`eulaPage`) | full-screen overlay on the login page |
| Scroll container | `LoginPage.qml:2204` (`eulaScroll`) | height derives from the image heights |
| Page 1 image | `LoginPage.qml:2210` (`eulaFirstImage`) | `images/loginPage/End User Agreement SETA-1.png` |
| Page 2 image | `LoginPage.qml:2217` (`eulaSecondImage`) | `images/loginPage/End User Agreement SETA-2.png` |
| Legacy variant | same binding | `End User Agreement Tachus-1.png`, selected when `isDefaultIcon` |
| Accept control | `LoginPage.qml:2227-2231` | calls `APPSETTINGS.eulaAccepted()`, hides the page |
| Acceptance read | `appsettings.cpp` `isEulaAccepted()` | registry-backed |
| Acceptance write | `appsettings.cpp` `eulaAccepted()` | registry-backed |

### Embedded resources

Registered in `images.qrc`:

```
images/loginPage/End User Agreement SETA-1.png
images/loginPage/End User Agreement SETA-2.png
images/loginPage/End User Agreement Tachus-1.png
```

All three are **compiled into the executable**. Replacing the agreement
therefore requires a rebuild — it cannot be swapped in the field.

## 3. Current entity named

The SETA-era artwork names an entity that is **not** JAC SHOOTING SOLUTIONS
(PTY) LTD. A `Tachus`-branded variant also exists, naming a third, older
entity. **Neither is correct for this product.**

## 4. Where it is shown, and whether acceptance is required

Visibility (`LoginPage.qml:2201`):

```
visible: !APPSETTINGS.isEulaAccepted() || !MODREADER.isValidLicence()
```

- Shown on the login page when acceptance has **not** been recorded, **or**
  when the licence check fails.
- **Acceptance is required on first launch** — the overlay covers the login
  page until Accept is pressed.
- The licence-expiry check is currently **disabled**, so in practice the
  acceptance flag alone controls it.

## 5. Acceptance storage behaviour

| Property | Current behaviour |
|---|---|
| Storage | Windows registry, via `QSettings`, under a **legacy** organisation name (`Seta` or `Tachus`, selected by `getBrandName()`) — **not** under the Tech Aim identity |
| Key | a single boolean, `isEulaAccepted` |
| **Versioned?** | **No.** There is no agreement version, revision, date or hash recorded |
| Per-user | yes (HKCU) |
| Re-acceptance on change | **Not possible today.** With only a boolean, a revised agreement cannot trigger re-acceptance — an existing `true` would silently suppress the new agreement |
| Audit trail | none — no timestamp, no accepted-version record |

### This is the second blocker

Even with correct artwork, **the acceptance record cannot express *which*
agreement was accepted.** Any user who has already accepted the SETA-era terms
would never be shown the replacement.

## 6. Requirements for the replacement

### Legal (user / publisher — not engineering)

1. Supply an approved agreement naming **JAC SHOOTING SOLUTIONS (PTY) LTD**.
2. Confirm whether the pre-beta evaluation needs different terms from the
   eventual release.
3. Confirm the governing jurisdiction and the effective date.
4. Confirm whether German-language terms are required for a German beta, and
   whether the English text remains authoritative.
5. Decide whether existing acceptances (SETA/Tachus era) are void and require
   re-acceptance. **Engineering recommendation: treat them as void** — they
   record agreement with a different entity.

### Engineering (prepared, pending the approved text)

1. **Version the acceptance record.** Store an agreement identifier and version
   plus an acceptance timestamp, not a bare boolean.
2. **Re-accept on version change.** Show the agreement whenever the stored
   accepted version differs from the shipped version.
3. **Move the record to the Tech Aim identity.** It currently writes under a
   legacy organisation name.
4. **Prefer text over images.** Rendered images cannot be searched, selected,
   translated, scaled for accessibility, or read out. Text also makes the
   German question tractable.

## 7. Insertion path — no application logic rewrite needed

The current structure already isolates the agreement well: **one visibility
binding, one accept action, one storage pair.** The approved replacement can be
inserted by:

1. replacing the resource(s) referenced by `eulaFirstImage` / `eulaSecondImage`
   — or swapping those Image elements for a text view;
2. extending `isEulaAccepted()` / `eulaAccepted()` to carry a version;
3. changing the `eulaPage.visible` condition to compare versions.

**No change to the login flow, session lifecycle, scoring or reporting is
required.** No such change is made in this phase.

## 8. Interim internal evaluation notice

For internal testing only, a neutral placeholder may be substituted. It must be
labelled, in the artefact itself:

```
DRAFT — LEGAL APPROVAL REQUIRED
Internal evaluation build. This is not a licence agreement and grants no
rights. Tech Aim Electronic Target Control, Pre-Beta Validation.
```

**No such notice has been created or activated in this phase**, because doing so
would mean editing a legal-facing surface without approval. It is recorded here
as the agreed shape should you ask for it.

## 9. Required user action

1. Provide the approved agreement text (English; German if a German beta is
   intended).
2. Confirm whether prior acceptances are void.
3. Approve versioning the acceptance record.

Until then this remains **the primary legal blocker to any external beta.**
