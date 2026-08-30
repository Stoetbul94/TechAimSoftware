# Single Target v1.0 — platform matrix

Updated 2026-08-30, at the close of Phase B.

| Platform | Status | Version | Automated | Physical |
|---|---|---|---|---|
| **Windows — Tech Aim** | **FROZEN RC** | 1.0.0-RC1 (`da03984`) | 6 948 / 0 | **PASS** — RC3F: 385 live shots, 0 faults |
| **Windows — SETA** | **AUTOMATED PARITY / PHYSICAL PENDING** | 1.0.0-EVAL1 (`47c5645`) | 6 878 / 0 (+8 DSB deferred) | **PENDING** |
| **Android** | **NOT STARTED** | — | — | — |
| **RMS** | **NOT STARTED** | — | — | — |

Every workspace is a git worktree of one repository
(`TechAimSoftware-repo/seta10/.git`), each on its own branch:
Tech Aim `feature/rc2e-latency-and-reset`, SETA `product/seta`,
Android `feature/android-tablet`, RMS `feature/rms`.

## What each status means

**FROZEN RC** — blockers 0, gate green, artefact hashed.
[TECH-AIM-SINGLE-TARGET-V1.0-FREEZE.md](TECH-AIM-SINGLE-TARGET-V1.0-FREEZE.md).

**AUTOMATED PARITY / PHYSICAL PENDING** — the SETA build carries the Tech Aim
v1.0 competition engine, so it inherits RC3F's evidence *for that engine*. It
does **not** inherit a physical pass for itself: no SETA-branded binary has ever
been fired at. [SETA-V1.0-EVALUATION-GATE.md](../seta/SETA-V1.0-EVALUATION-GATE.md).

**NOT STARTED** — no work has begun and no parity is claimed.

## Not claimed anywhere

- ~~"SETA is physically qualified."~~ PENDING.
- ~~"DSB 2026 is supported."~~ Deferred — [SETA-DSB-PORT-001](../seta/SETA-DSB-PORT-001.md).
- ~~"Android or RMS parity."~~ Neither has been started.
