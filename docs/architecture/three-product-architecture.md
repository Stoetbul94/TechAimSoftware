# Tech Aim — three-product architecture and workspace

Architecture and workspace only. **No German programme, no RMS feature and no
refactor has been implemented.** The frozen RC3a artefact is untouched.

## 1. Repository and release anchor

| | |
|---|---|
| Repository root | `C:\Users\User\Downloads\TechAimSoftware-repo\seta10` |
| Development branch | `feature/rc2e-latency-and-reset` |
| HEAD at survey | `b2a7560`, working tree **clean**, local == remote |
| Remotes | `origin` → `github.com/Stoetbul94/TechAimSoftware`, `upstream` → `raosrinu2004/Merging_app_modReader` |
| Tags before this task | **none** |

**Release anchor created:** annotated tag **`techaim-v0.9.0-rc3a-seta-eval`** on
**`488d506`**, pushed to `origin`. That commit was read from the release
manifest (`dist/rc3/…RC3a…manifest.json` → `sourceCommit: 488d506`), not
guessed. The tag records both hashes, the Qt/compiler versions, the
`app_mode=Live` / `developer_mode=0` configuration and the regression totals.

RC3a binaries and the handoff/send folders remain **immutable**. Any change is
a new development version (RC3b / RC4) with new hashes.

## 2. Worktrees

`git worktree` throughout — **no directory was copied**, so history is shared
and commits are transferable between all three.

| Workspace | Path | Branch | Base |
|---|---|---|---|
| Tech Aim main | `…\TechAimSoftware-repo\seta10` | `feature/rc2e-latency-and-reset` | — |
| SETA / OEM | `C:\Users\User\Downloads\TechAimSoftware-SETA` | **`product/seta`** | `b2a7560` |
| RMS | `C:\Users\User\Downloads\TechAimSoftware-RMS` | **`feature/rms`** | `b2a7560` |

`feature/rms` follows the existing convention (`feature/*`, `chore/*`,
`fix/*`, `release/*`). `product/seta` deliberately introduces a `product/`
prefix: SETA is a long-lived *product line*, not a feature, and naming it
`feature/…` would invite someone to merge it into main.

Three Claude Code sessions can now be opened, one per directory.

## 3. Structure

```
                    ┌──────────────────────────────┐
                    │   SHARED FOUNDATION (main)   │
                    │  transport · acquisition ·   │
                    │  scoring · SessionStore ·    │
                    │  recovery · reporting        │
                    └───────────────┬──────────────┘
                promote │           │           │ promote
        ┌───────────────┘           │           └───────────────┐
        ▼                           ▼                           ▼
┌───────────────┐        ┌────────────────────┐      ┌────────────────────┐
│  TECH AIM     │        │  SETA / OEM        │      │  RMS               │
│  SINGLE TARGET│        │  single target     │      │  range control     │
│  ISSF         │        │  + DSB programmes  │      │  multi-node        │
│  product/main │        │  product/seta      │      │  feature/rms       │
└───────────────┘        └────────────────────┘      └────────┬───────────┘
                                                              │ commands
        ▲                                                     ▼ events
        └──────────────── target node ⇄ RMS interface ────────┘
```

Changes flow **up** into the shared foundation and **down** into products.
They never flow sideways between SETA and RMS.

## 4. Component ownership matrix

**A = core/shared · B = single-target product · C = SETA-specific · D = RMS-specific**

| Component | Where it lives now | Class | Note |
|---|---|---|---|
| libmodbus transport | `3rdparty/libmodbus`, `ModReader/` | **A** | Vendored fork; never duplicate |
| Serial / FTDI discovery | `ModReader/`, `src/target/` | **A** | |
| Reconnect state machine | `src/reliability/` | **A** | |
| Target polling / acquisition | `ModReader/forms/tachuswidget.*` | **A** | |
| Shot coordinate handling | `tachuswidget` → `CenterPane.qml` | **A** | |
| **Scoring** (`calculateShootingSocre`) | `CenterPane.qml` | **A** | Single authority. RMS must never re-implement |
| Projectile / range definitions | `AppSettings::projectileDiameterMm` | **A** | SCORING-CAL-001 |
| Paper feed / motor timing | `AppSettings`, `MotorThread` | **A** | |
| SessionStore | `src/reliability/` (38 files) | **A** | |
| Recovery / replay | `src/reliability/` | **A** | |
| EventBus / EventRegistry | `src/reliability/events/` | **A** | Natural seam for RMS events |
| Match / session logic | `src/qualification/`, `src/mode/` | **A** | |
| Finals (3P, 10 m) | `src/finals/`, `src/finals10m/` | **B** | ISSF finals; SETA may add its own |
| Training Lab | `src/training/` (23 files) | **A** | Generic; programmes are content |
| Call & Diagnose | `src/training/` | **A** | |
| Coach analytics | `src/analytics/` | **A** | Frozen engine |
| Reports / PDF | `src/bridge/`, report QML | **A** | Layout may be branded per product |
| Product identity / branding | `src/app/ProductIdentity.*` | **A mechanism / C+D values** | Already the right seam |
| Translations | `translations/*.ts` | **A mechanism / C content** | UI-DEC-015: presentation only |
| **Discipline / programme definitions** | `main.qml` ListModels, `LoginPage.qml` | **A mechanism, B+C content** | **The seam that must be built first** |
| UI navigation / selection | `LoginPage.qml` | **B**, C variant | |
| Settings drawer | `SettingsPage.qml` | **A** | |
| Support bundle | `tools/deployment/` | **A** | |
| Installer / release tooling | `tools/release/` | **A mechanism, per-product config** | |
| Range / lane / rankings / TV | *does not exist* | **D** | |

Only one item is genuinely mis-placed today: **discipline and programme
definitions are hardcoded in QML ListModels**. Everything else already sits
behind a reasonable boundary.

## 4.1 Correction (2026-08-20) — target discovery is platform-specific

*Raised by the Android tablet milestone A1/A2 on `feature/android-tablet`. The
matrix row above is left intact deliberately; this entry supersedes its
classification rather than editing it away.*

**The matrix classifies "Serial / FTDI discovery" as A (core/shared). That
classification is too broad and is corrected here.**

Two different things were collapsed into one row:

| Concern | Correct class | Why |
|---|---|---|
| Target **connection interface** — what a target is, how one is chosen, ranked, remembered and reconnected (`ISerialDeviceProvider`, `TargetDeviceFingerprint`, `PaperFeedCoordinator`) | **A — genuinely shared** | Pure logic over a `SerialDeviceInfo` struct. It does not care where the list came from and compiles unchanged on every platform. |
| **Windows FTDI/COM enumeration** — `QSerialPortInfo::availablePorts()`, `CreateFileA("COMxx:")`, CH340/CP210x/FT232 device-node access (`QtSerialDeviceProvider`, libmodbus RTU backend) | **Platform shell, not core** | Has no Android implementation and cannot have one without the Java `UsbManager` API. On an unrooted tablet `availablePorts()` returns nothing usable. |

**The rationale.** The generic core owns the *interface*; it does not own one
platform's enumeration mechanism. The existing code already had this right —
`ISerialDeviceProvider` was introduced so the selection logic could be tested
without hardware — but the architecture document described the whole row as
shared, which would license a future product to assume COM-port enumeration is
universally available. It is not.

Nothing in the codebase moves as a result of this correction. It changes what
may be *assumed*, not what is *built*:

- `src/target/TargetDeviceFingerprint.*` and `PaperFeedCoordinator.*` stay
  shared and are used unmodified by the Android build.
- `QtSerialDeviceProvider` is Windows shell. Android supplies its own provider
  behind the same interface.
- libmodbus stays a vendored shared dependency, but its **RTU backend** is
  Windows/desktop-only in practice while its **TCP backend** is portable.

Detail and the transport decision: `android-target-transport-options.md` and
`android-product-architecture.md` §4.

## 5. SETA competition-profile design (design only)

A profile is **data**, not code. One record per programme, loaded from a
versioned catalogue, keyed by a stable ID.

```
CompetitionProfile
  rulesetId          ISSF | DSB                 (enum, never translated)
  federation         "ISSF" | "DSB"
  ruleVersion        e.g. "2026 Ed.2025 2nd print" / Sportordnung edition
  ruleSource         document + rule number
  authorityStatus    CONFIRMED | CUSTOMER_SOURCE_REQUIRED | PENDING
  disciplineId       AR10 | AP10 | RIFLE50 | ...  (enum)
  distanceM          10 | 50
  weaponFamily       AIR_RIFLE | AIR_PISTOL | SMALLBORE_RIFLE
  targetFaceId       -> the existing authoritative target definition
  programmeId        e.g. DSB_AR10_3X20        (enum/stable string)
  displayName        { en, de }                 presentation ONLY
  positions          ordered [KNEELING, PRONE, STANDING]
  shotsPerPosition   [20, 20, 20]
  totalShots         60
  scoringMode        INTEGER | DECIMAL
  sighterPolicy      unlimited-in-window | fixed | none
  preparationTimeS / sightingTimeS / matchTimeS / changeoverTimeS
  positionTransition  behaviour id
  hasFinal           bool
  athleteClasses     [SENIOR, JUNIOR, SCHUELER, PARA…] if restricted
  validFrom / validTo
```

Binding rules:

1. **`programmeId` drives every decision.** `displayName` is never compared,
   switched on, or used as a key — **QML-LANG-001** proved what that costs:
   a translated string in a logic path put a 10 m Air Pistol session into the
   rifle scoring branch.
2. A profile **selects** an existing target definition and scoring path; it
   never carries its own geometry or formula.
3. A profile whose `authorityStatus` is not `CONFIRMED` may be listed as
   *unofficial/practice* but must not be presented as an official competition.
4. Catalogue changes ship with tests, exactly as the ISSF constants do.

## 6. DSB / German rule-authority matrix

**Nothing here is confirmed.** No DSB Sportordnung text has been obtained or
read; the entries below record what has been *reported*, and by whom.

| Programme | Federation | Official source | Rule no. | Timing | Sighting | Position order | Scoring | Class | Status |
|---|---|---|---|---|---|---|---|---|---|
| LG 3-Stellung 20/20/20 | DSB | Sportordnung — **not obtained** | reported **1.20** | unknown | unknown | reported Kniend/Liegend/Stehend | unknown | unknown | **PENDING** — name, rule number and 20/20/20 from the DSB competition table as reported; nothing verified |
| LG 3×10 (Schüler) | DSB | regional material — **not obtained** | unknown | unknown | unknown | unknown | unknown | Schüler? | **CUSTOMER SOURCE REQUIRED** |
| LG 3×20 (Schüler) | DSB | regional material — **not obtained** | unknown | unknown | unknown | unknown | unknown | Schüler? | **CUSTOMER SOURCE REQUIRED** |
| LG 3×15 | — | **none identified** | — | — | — | — | — | — | **CUSTOMER SOURCE REQUIRED** — mentioned by Harald/SETA only; no rule source exists |
| KK 50 m 3×10 | DSB | **not obtained** | unknown | unknown | unknown | unknown | unknown | unknown | **CUSTOMER SOURCE REQUIRED** |
| KK 50 m 3×20 | DSB | **not obtained** | unknown | unknown | unknown | unknown | unknown | unknown | **CUSTOMER SOURCE REQUIRED** |
| KK 50 m 3×40 | DSB | **not obtained** | unknown | unknown | unknown | unknown | unknown | unknown | **CUSTOMER SOURCE REQUIRED** |

**Gate:** no programme enters official competition logic until its rule text is
supplied and recorded, the way the ISSF constants were. The DSB position order
*Kniend/Liegend/Stehend* differs from the ISSF 3P order the code already
implements — that alone must be verified before any code assumes it.

Ask SETA for: the Sportordnung edition and effective date; the rule number per
programme; shots and position order; preparation, sighting, match and
changeover times; sighter policy; integer or decimal; and which athlete classes
each applies to.

## 7. SETA UI navigation

Three steps instead of an ever-growing wall of cards:

```
STEP 1  RULE SET        [ ISSF ]  [ DSB / German ]
STEP 2  DISCIPLINE      10 m Air Rifle · 10 m Air Rifle 3-Position ·
                        10 m Air Pistol · 50 m Rifle · 50 m Rifle 3-Position
STEP 3  PROGRAMME       only when the discipline has more than one
                        e.g. 3×10 · 3×20 · 3×40
```

- Step 3 is **skipped** when a discipline has exactly one programme, so the
  ISSF path stays as short as it is today.
- Selection state is one value — the chosen **`programmeId`** — resolved to a
  profile. The engine consumes the profile; it never learns how the user got
  there.
- Components: `RuleSetSelector`, `DisciplineSelector`, `ProgrammeSelector`,
  `SelectedProgrammeSummary`, over a `CompetitionCatalogue` model.
- Migration: keep `LoginPage` as-is, add the catalogue behind it, make the
  current cards resolve to profile IDs, then replace the card grid last. The
  scoring/target engine is untouched at every step.
- Unverified programmes are visibly marked and cannot claim official status.

## 8. RMS boundary

| Target node owns | RMS owns |
|---|---|
| Physical target connection, acquisition, sequence integrity | Range, lanes, athlete assignment |
| **Local scoring — the only scoring authority** | Match orchestration, central commands |
| SessionStore, recovery, paper feed, target health | Common timing, monitoring, results aggregation |
| The authoritative accepted-shot event | Rankings, incidents, display/TV feed, central archival |

**RMS never computes a score.** It aggregates the score the node already
computed and stored. A shoot must remain valid if RMS disappears mid-match.

## 9. Target Node ⇄ RMS interface

**Transport recommendation: reuse what already exists.** `sender.cpp` already
broadcasts UDP on **port 7755** and `receiverTachus.cpp` binds **7756** — a
node→server datagram path is present today. Recommended: **UDP datagrams for
telemetry/discovery on the existing ports, plus one TCP connection per node for
commands and acknowledged delivery.** No WebSocket, no broker, no new
dependency until a measured need appears.

`AcceptedShotEvent`: `protocolVersion · nodeId · laneId · sessionId ·
rulesetId · programmeId · position · shotSequence · rawX · rawY ·
authoritativeScore · innerTen · timestampUtc · targetStatus`.

Commands: `identify · assignLane · createSession · loadSession · preparation ·
startSighting · startMatch · changePosition · stop · resume · end ·
requestStatus · requestReplay`.

Reliability rules:

- **Sequence numbers per session**, monotonic; RMS detects gaps rather than
  trusting arrival order.
- **Idempotency:** every command carries a `commandId`; re-delivery is a no-op.
- **Acknowledgement:** node acks with the applied state, not merely "received".
- **Offline queue:** the node keeps emitting into a bounded local queue and
  replays on reconnect — the match continues without RMS.
- **Duplicate suppression** on `(sessionId, shotSequence)`.
- **Version compatibility:** `protocolVersion` first field; unknown fields
  ignored, unknown commands rejected explicitly.

## 10. Shared-change policy

| Change | Where it starts | How it moves |
|---|---|---|
| Generic defect / core improvement | **main** | validated on main → deliberately promoted to `product/seta`, then `feature/rms` if relevant |
| SETA-specific feature | `product/seta` | stays there |
| RMS-specific feature | `feature/rms` | stays there |
| Generic defect *found* in SETA/RMS | fix on **main** | then promote — never three divergent fixes |

No blind branch merging. No automatic cherry-picking of product-specific
commits. Promotion is an explicit, reviewed act, exactly like the staged
single-file commits used through the RC3 work.

## 11. Risks

1. **Rule authority.** Six of seven German programmes have no source. Encoding
   them from a customer conversation would repeat the mistake the ISSF audit
   was created to prevent.
2. **Scoring divergence.** Three branches touching one QML scoring function is
   the highest-consequence risk here. Scoring changes belong on main only.
3. **Translated strings in logic.** QML-LANG-001 is fixed and guarded
   file-wide; German programme names must never become keys.
4. **Branch drift.** Long-lived `product/seta` will diverge; promote from main
   regularly and in small steps.
5. **Position-order assumption.** DSB Kniend/Liegend/Stehend appears to differ
   from the implemented ISSF 3P order — an unverified assumption here silently
   corrupts a match.
6. **RMS scope creep into the node.** Central-control screens must not land in
   the single-target application because the code is nearby.
7. **Frozen artefact confusion.** RC3a is out with SETA; development continues
   past it. The tag is the defence.

## 12. Seams, not a refactor

The application is qualified and working. Recommended smallest steps, in order:

1. **Competition catalogue seam** — introduce `CompetitionProfile` + a
   catalogue that the *existing* ISSF cards resolve to. Behaviour identical;
   this is the enabling change for everything else.
2. **Product identity already is a seam** — SETA branding needs configuration,
   not code.
3. **Event seam for RMS** — publish the accepted-shot event through the
   existing `EventRegistry`; RMS subscribes. No change to acquisition.
4. **Only then** consider extracting a core library, if the three products
   actually demand it.

Explicitly **not** doing: repository restructure, mass renames, splitting every
component into libraries, replacing QML architecture, rewriting SessionStore,
scoring, Modbus or reports.

## 13. Recommended order

1. Obtain DSB rule sources (blocks all German work).
2. Build the competition-catalogue seam on **main**, ISSF-only, behaviour
   unchanged, with tests.
3. Promote to `product/seta`; add SETA branding via ProductIdentity.
4. Add **confirmed** DSB profiles only, one at a time, each with its rule
   reference and tests.
5. In parallel on `feature/rms`: publish the accepted-shot event, then a
   read-only monitor — no commands until the event path is proven.
6. RMS commands with acknowledgement and idempotency.
7. Revisit modularisation only if the three products demand it.

---

## Appendix A — foundation promotion (2026-08-15)

The competition catalogue foundation was promoted into both product lines.
Ancestry was verified first: `git merge-base --is-ancestor 2bec656 8d1d243`
returns true, so the two commits cannot arrive separately.

| Line | Branch | HEAD after promotion |
|---|---|---|
| Tech Aim foundation | `feature/rc2e-latency-and-reset` | `8d1d243` |
| SETA | `product/seta` | `4eb5aa2` |
| RMS | `feature/rms` | `5a4ffbf` |

Promoted range `b2a7560..8d1d243` — three commits: `9a8da80` (architecture,
worktrees, RC3a anchor), `2bec656` (catalogue seam), `8d1d243` (authority
correction). Merged `--no-ff` so each promotion is a recorded act rather than a
silent fast-forward.

**`2bec656` and `8d1d243` must never be separated.** The first labels all 48
entries ISSF; the second corrects that to 4 official courses and 44 Tech Aim
presets. RMS in particular will transmit `programmeId` on the wire, so the
uncorrected ids would put false ISSF authority claims into messages.

Verified in **both** product worktrees: `CompetitionCatalogue.qml` present ·
`targetStandardId` semantics present · 4 official / 44 preset preserved ·
**zero** `issf.*.match*` ids remaining · QML suite **139 checks / 0 failures** ·
working trees clean.

No SETA-specific change is in RMS and no RMS-specific change is in SETA. No DSB
programme, no UI redesign, no networking.

### Approved next lanes

**SETA next** — ProductIdentity/branding, the hierarchical Rule Set →
Discipline → Programme selector, and verified DSB programme profiles. DSB
profiles remain blocked on rule authority; they enter as catalogue entries with
`rulesetId=dsb`, never as new QML literals.

**RMS next** — read-only Range Monitor architecture and the Target Node
accepted-shot/status protocol. The node keeps the only scoring authority.
