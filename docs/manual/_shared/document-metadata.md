# Shared Document Metadata — Tech Aim Manuals

**This file is the single source for the metadata block that appears on every
Tech Aim manual.** Do not retype these values into individual documents; when
something changes, change it here and re-check the documents that embed it.

## Controlled values

| Field | Value |
|---|---|
| Product | Tech Aim Electronic Target Control |
| Brand (prose) | Tech Aim |
| Brand (file/executable form) | TechAim |
| Executable | `TechAim.exe` |
| Product version | 0.9.0 |
| Release channel | Pre-Beta Validation |
| Publisher | JAC SHOOTING SOLUTIONS (PTY) LTD |
| Document status | Pre-Beta Documentation |
| Document version | 1.0 |
| Revision identifier | P0-J |
| Master language | English (controlled master edition) |
| Publication date | 2026-07-27 |
| Application commit | `3741980` |
| Confidentiality | Internal evaluation — not for public distribution |

## Naming rules

- **"Tech Aim"** (spaced) in all prose.
- **"TechAim"** (unspaced) only for the executable, file prefixes, package
  identifiers and the AppData folder.
- Never **Seta**, **Seeds**, or **Seta Electronic Target Control** as the
  product name.
- **"SETA" may appear** where it names the electronics supplier or the lane
  hardware/file-share integration — that is a different thing from the
  software product. See `docs/product-identity-audit.md`.

## Standard front-matter block

Every manual opens with this block, with `<Title>` and `<Language>`
substituted:

```
<Title>
Tech Aim Electronic Target Control

Product version 0.9.0 · Release channel: Pre-Beta Validation
Document version 1.0 (P0-J) · Language: <Language>
Published 2026-07-27 · Application commit 3741980
Publisher: JAC SHOOTING SOLUTIONS (PTY) LTD

Status: Pre-Beta Documentation.
Internal evaluation — not for public distribution.
```

The Tech Aim logo (`images/logo/techaim_color.png`) is placed above this block
in rendered (PDF/HTML) output. Page numbers and the document version go in the
page footer.

## Status vocabulary

Procedures in these manuals carry exactly one status:

| Status | Meaning |
|---|---|
| **VERIFIED AUTOMATICALLY** | Covered by a passing test in a harness. |
| **VERIFIED MANUALLY** | A person performed it against a running build and confirmed it. |
| **MANUAL VALIDATION REQUIRED** | Written from source, but **not** performed against a running build. Treat as unconfirmed. |
| **WINDOWS RC1 DEPENDENT** | Cannot be finalised until installer/signing exists. |
| **PHYSICAL HARDWARE DEPENDENT** | Needs a real electronic target. |
| **GERMAN REVIEW REQUIRED** | Needs a native German technical reviewer. |

A procedure is **never** marked verified because it looks plausible.
