# Android generic-branding audit — GENERIC BRANDING DEFECTS

**Milestone:** A2.5 (emulator + real-tablet UI/runtime qualification)
**Branch:** `feature/android-tablet`
**Audited build:** `0.9.0-ANDROID-A2.5`, package `za.co.techaim.target`

The first Android product line is **generic Tech Aim**. It must carry no SETA,
DSB, customer-specific or German OEM material. This audit covers everything
visible from startup, plus what the build actually ships.

Two of the five findings are **legal content and were deliberately not
changed** — those are Arnold's decision, not an engineering one.

---

## Summary

| # | Finding | Class | Status |
|---|---|---|---|
| 1 | First-run licence agreement is a **German "Seta"** document | **LEGAL** | **NOT CHANGED — decision required** |
| 2 | Match Summary **PDF is stamped with the SETA logo** | Branding (non-legal) | **NOT CHANGED — reported, see §2** |
| 3 | Release channel read "SETA Evaluation" | Branding (non-legal) | **FIXED on Android** |
| 4 | Settings stored under a `Seta/` namespace on disk | Internal, not visible | **NOT CHANGED — reported** |
| 5 | Application logo / window title | — | **PASS — already generic Tech Aim** |

---

## 1. LEGAL — the first-run licence agreement is a German SETA document

**Where:** `LoginPage.qml` renders
`qrc:/images/loginPage/End User Agreement SETA-1.png` and `-2.png` in the
first-run gate (`eulaPage`).

**What the user sees on an Android tablet, before anything else:** a licence
agreement headed *"Endbenutzer-Lizenzvereinbarung (\"Vereinbarung\")"*, in
German, naming **Seta** as the licensor and granting the licence on Seta's
terms.

Three separate problems, any one of which is disqualifying for a generic
Tech Aim product:

1. It names the wrong entity. The publisher is **JAC SHOOTING SOLUTIONS
   (PTY) LTD** and the product is Tech Aim.
2. It is in German only. There is no English text, and the app's default
   language is English.
3. It is a **bitmap**, so it cannot be translated, searched, copied, or
   accessibility-read, and its terms cannot be revised without new artwork.

**Deliberately NOT changed.** Replacing licence wording is a legal decision.
Nothing here invents, edits or substitutes agreement text. See
`docs/legal/eula-replacement-requirements.md`.

**Decision needed from Arnold**, choosing one of:
 - supply a Tech Aim end-user agreement (ideally as text, not an image); or
 - suppress the licence gate on Android development builds until (a) exists; or
 - confirm the SETA agreement is genuinely intended to apply to this product.

Note the related A2.5 fix: this dialog was previously **impossible to dismiss**
on Android (UI-AND-009). It can now be accepted — which makes the wrong-content
problem *more* pressing, not less, because a user can now actually accept it.

---

## 2. The Match Summary PDF is stamped with the SETA logo

**Where:** `customprint.cpp`, `CustomPrint::createSummryPdf()` — in the
non-`BRAND_TACHUS` branch (which is the branch this product builds):

```cpp
QImage bImage(":/images/logo/seta.png");
painter.drawImage(QRectF(iWidth-bImage.width()-20, 0, ...), bImage, ...);
```

`CUSTOMPRINT.createSummryPdf` is called from QML, so this is a **live,
reachable** path: a Match Summary exported from the generic Tech Aim product
carries a SETA logo on the page.

Generic Tech Aim artwork already exists and is shipped —
`images/logo/techaim_color.png`, `techaim_white.png` (used by `Theme.logoWhite`
for the on-screen header) — so the replacement asset is available.

**NOT changed in A2.5, deliberately**, for two reasons:
 - it alters **generated report output on Windows too**, and report appearance
   in this project is evidence-governed (an accepted report layout is not
   changed without a decision entry and fresh visual evidence);
 - A2.5 is an Android shell qualification, and this is not an Android-specific
   defect.

Recommended as a small, self-contained follow-up.

---

## 3. FIXED — release channel read "SETA Evaluation" on Android

**Where:** `src/app/ProductIdentity.cpp`, `p.releaseChannel`.

The channel is shown in **Settings > ABOUT / BUILD** and written to the log on
**every startup**. On the generic Tech Aim Android product it read
`SETA Evaluation`, which is simply the wrong customer's name on the tin: this
APK is not a SETA evaluation of anything.

This is ordinary, non-legal branding and generic wording was available, so it
was corrected under the §5 rule.

```
Android : "Android Development"
Windows : "SETA Evaluation"   (unchanged - that build IS the SETA candidate)
```

Scoped with `Q_OS_ANDROID` so the Windows product, its About screen and its
release identity are untouched.

---

## 4. Settings are stored under a `Seta/` namespace on disk

**Where:** `appsettings.cpp` — `QSettings regSettings("Seta", "shootingApp")`
(four call sites, including the EULA-accepted flag).

Confirmed on device:

```
/data/user/0/za.co.techaim.target/files/settings/Seta/shootingApp.conf
```

**Not user-visible.** It is an on-disk organisation directory inside
app-private storage; no screen displays it, and Android gives no file browser
that would reveal it.

**NOT changed.** Renaming the namespace would orphan the stored
`isEulaAccepted` flag and any other value written under it, re-prompting the
licence gate and losing settings — a migration, not a rename. Recorded so the
inconsistency is known and can be handled deliberately if the namespace is ever
migrated.

---

## 5. PASS — logo, window title and application label are generic

- Header logo: `theme.logoWhite` -> `images/logo/techaim_white.png` — Tech Aim.
- Android application label: **"Tech Aim"** (verified with `aapt2 dump
  badging`).
- Package id: `za.co.techaim.target` — Tech Aim namespace.
- Window title derives from `PRODUCT.fullProductName`; the legacy
  `isDefaultIcon ? "TACHUS" : "SETA"` title was already removed before this
  milestone.
- Publisher shown in About: **JAC SHOOTING SOLUTIONS (PTY) LTD** — correct.
- The shipped German translation catalogue (`techaim_de_DE`) is a legitimate
  language option, not OEM material, and is inactive by default (default
  language is English).

No defect.

---

## Scope note

This audit covers what is **visible from startup and what the APK ships**. It
does not cover screens not opened during A2.5 — the Coach Report, Training Lab,
Finals and Incident surfaces were not all reached, and no claim is made about
branding inside them.
