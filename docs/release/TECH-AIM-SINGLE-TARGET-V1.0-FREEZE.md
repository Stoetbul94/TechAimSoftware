# Tech Aim Single Target — Windows v1.0 freeze record

**Do not rebuild this artefact. Do not modify it. Do not overwrite its
package.** It is the first candidate on the v1.0 line, and every later claim
about Windows v1.0 rests on it being exactly what it was when it was built.

## Identity

| | |
|---|---|
| Version | **1.0.0-RC1** |
| Branch | `feature/rc2e-latency-and-reset` |
| Commit | **`da03984`** |
| Build config | Release, Qt 6.5.3 MinGW x86_64 |
| Built | 2026-08-30 13:40:56 |
| Package | `dist/v1.0.0-rc1/TechAim-1.0.0-RC1-Windows-x64/` |
| ZIP | `dist/v1.0.0-rc1/TechAim-1.0.0-RC1-Windows-x64.zip` (36.65 MB) |
| **ZIP SHA-256** | `4759B87DAFEF3E9C275168519B773AF0D052252B566DE19C4514A6ECF1BEA5CE` |
| **EXE SHA-256** | `E29E5EB57B66FC30C8EB375C568B8606DEBF86F210634A265431618CA50BB0D7` |
| Shipped config | `app_mode=Live`, **`developer_mode=0`**, `is_single_decimal=1` |

The binary was verified to contain both `1.0.0-RC1` and `da03984` **before**
packaging, and the packager refuses to run if it does not — the stale
`APP_GIT_SHA` failure this project has already had once cannot recur silently.

Built from a **clean** tree: `release/` was deleted, `qmake` re-run, and the
whole application recompiled. Nothing incremental.

## Why it is a candidate and not a release

Every release blocker is closed and the full automated regression is green, but
**no live target test was performed for this close-out.** Acquisition, Modbus,
counter reconciliation, paper feed and the 50 m 3P state machines are unchanged
since RC3F and inherit its field evidence; this round's reporting and
persistence work is automated, replay and render validated.

A build that has not itself been fired at may not describe itself as released,
so the binary reports its channel as **"v1.0 Release Candidate - Evaluation"**
and keeps the field-test notice — *Evaluation Build — Not for Official
Competition Results* — on screen and in report footers.

## Gate at the freeze

| | |
|---|---|
| Open blockers | **0** |
| Automated checks | **6 941, 0 failures** across nine suites |
| TypeError / ReferenceError | **0 / 0** |
| New acquisition warnings | **0** |
| Pre-existing binding warnings | **7, unchanged since RC3B** |
| Portable launch, no Qt on `PATH` | **PASS** — 0 missing DLL, correct identity line |

Full detail:
[`TECH-AIM-SINGLE-TARGET-V1.0-RELEASE-GATE.md`](TECH-AIM-SINGLE-TARGET-V1.0-RELEASE-GATE.md).

The identity line the packaged binary printed on its own first run, with no Qt
on `PATH`:

```
Tech Aim 1.0.0-RC1 Release build · commit da03984 · built Aug 30 2026 13:31:21
· v1.0 Release Candidate - Evaluation · flavour TECH_AIM
```

## What this freeze covers

**Windows, single target. Nothing else.** SETA, Android and the RMS interface
are not started, not built, not tested, and not covered by any statement in
this document or in the release gate.

## Known noise in the shipped build

`developer_mode=0` silences the QML-side diagnostic traces. A set of C++
`qDebug` lines is still unconditional — `getGroup`, `getXMPI`, and similar —
because the C++ logging audit and `QLoggingCategory` channels are a documented
deferred item that the QML side completed in S4 and the C++ side has not. It
is console noise on a build that normally runs without a console attached. It
writes nothing to a report, changes no value, and is not a regression.

## If this artefact is ever superseded

The next candidate gets its own number, its own package and its own freeze
record. This one is not rebuilt, not renumbered and not overwritten — the same
rule that protects `RC3F-DIAG`, and for the same reason.
