# Tech Aim — installer options

**No installer framework exists in this repository.** Verified by searching for
NSIS (`.nsi`), WiX, Inno Setup (`.iss`), MSIX/AppX manifests, `signtool` steps
and installer build targets: no match. `Seta.pro` has no install target beyond
qmake defaults.

**No framework is chosen here.** Choosing one is a product decision with
procurement and support consequences. This document sets out the options and
what each would actually cost.

**The supported deployment method today is the portable ZIP**, and it should
stay that way until the physical qualification passes.

---

## Why the portable ZIP is the right answer for now

- It is **proven**: RC1, RC2 and RC2a were all deployed this way, and the RC1/RC2
  packages ran on the range.
- It makes rollback **trivial and safe** — two folders side by side, no shared
  state, no uninstall step. That matters more than convenience while a serial
  defect is still awaiting physical confirmation.
- It requires **no administrator rights**, so it cannot break a locked-down
  range laptop.
- Upgrade safety is **structural**, not conventional: user data lives outside
  the program folder, so replacing the folder cannot touch it.

The costs are real but small: the operator extracts a ZIP, there is no Start
Menu entry, and Windows warns about an unsigned download.

---

## Options, if an installer is later wanted

| | **Inno Setup** | **WiX (MSI)** | **NSIS** | **MSIX** |
|---|---|---|---|---|
| Licence | Free, open | Free, open | Free, open | Free tooling |
| Learning cost | Low | High | Medium | Medium |
| Per-user install, no admin | Yes | Awkward | Yes | Yes |
| Enterprise deployment (GPO/Intune) | No | **Yes** | No | **Yes** |
| Clean uninstall entry | Yes | Yes | Yes | Yes |
| Preserves AppData on uninstall | By default | By default | By default | **Container may be removed** |
| **Signing required to be usable** | Recommended | Recommended | Recommended | **Mandatory** |
| Suits this product | **Best fit today** | If a club/federation demands MSI | Works, no advantage over Inno | Not until signing exists |

**Recommendation.** If and when an installer is wanted, use **Inno Setup**:
per-user install into `%LOCALAPPDATA%\Programs\TechAim`, no elevation, a
straightforward uninstall entry, and it will not disturb the AppData layout.
Revisit WiX only if a federation requires MSI for managed deployment.

**MSIX is not viable yet** — it cannot be installed unsigned, and it may remove
the application's data container on uninstall, which conflicts with the rule
that removal never destroys an athlete's sessions.

---

## What is required before any installer is built

1. **A code-signing certificate.** This is the blocker. There is no certificate,
   no `signtool` step and no Authenticode metadata anywhere in the build.
   Obtaining an OV or EV certificate is a **procurement** action, not a build
   change. An unsigned installer is *worse* than the ZIP: it presents a
   trust-inviting dialog while offering no more assurance than the checksum.
2. **The physical qualification must pass.** Packaging convenience must not
   arrive before hardware confidence.
3. **A decision on install scope** — per-user (recommended) or per-machine.
4. **CFG-001 must be honoured.** `config.ini` is read by *relative* path, so any
   shortcut the installer creates **must** set its working directory to the
   install folder or the software silently starts with defaults.
5. **PKG-002 ideally resolved** — windeployqt currently fails and deployment
   relies on a hand-maintained allow-list. An installer would inherit that.

### Additional tooling that would be required

| Need | Tool |
|---|---|
| Build the installer | Inno Setup Compiler (`ISCC.exe`) — not currently installed |
| Sign the installer and the binary | `signtool.exe` (Windows SDK) + a certificate — **neither present** |
| Timestamp the signature | A public timestamp authority (so signatures outlive the certificate) |

---

## Statements that must not be made

- Do **not** describe any current Tech Aim artefact as code-signed. Nothing is.
- Do **not** claim Windows SmartScreen approval or reputation. Reputation is
  earned by a signed publisher over time and none exists.
- Do **not** describe an installer as available. None is built, and this
  preparation phase does not build one.

---

## The framework-neutral install manifest

`New-DeploymentPrep.ps1` writes
`dist\deployment-prep\installer-candidate\install-manifest.json`. It states what
an installer would have to do without committing to a framework:

| Field | Value |
|---|---|
| `defaultInstallPath` | `%LOCALAPPDATA%\Programs\TechAim` |
| `requiresElevation` | `false` — a per-user install needs no administrator rights |
| `userDataPath` | `%LOCALAPPDATA%\TechAim\TechAim` |
| `userDataOnUninstall` | **PRESERVE** |
| `userDataOnUpgrade` | **PRESERVE** — the path does not change between versions |
| `registryScope` | `HKCU\Software\TechAim\TechAim` |
| `publisherSigned` | `false` |
| `codeSigningStatus` | NOT CONFIGURED — any installer produced now is UNSIGNED |
| `services`, `firewallRules`, `fileAssociations` | none |
| `autoUpdate` | NONE |

Every one of those is verifiable against this repository today.
