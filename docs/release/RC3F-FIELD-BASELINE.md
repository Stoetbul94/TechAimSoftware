# RC3F — the last field-proven pre-v1.0 baseline

**Do not rebuild it. Do not modify it. Do not overwrite its package.**

RC3F is the build that ran a real competition. Every later claim about the
acquisition engine and the 3P state machines rests on this evidence, so the
artefact has to stay exactly as it was used.

## Identity

| | |
|---|---|
| Version | **0.9.0-RC3F-DIAG** |
| Commit | **39df782** |
| Branch | `feature/rc2e-latency-and-reset` |
| ZIP SHA-256 | `864A5F5711C1365D64CDB3C269BCD7F92CCFF2327244539F7611A0FB948617B3` |
| EXE SHA-256 | `F27A6743F9114B60B6BAD8B919A995E51E47A7DC86F726B4CDA2BBFB82796856` |
| Package | `dist/rc3f/TechAim-0.9.0-RC3F-DIAG.zip` |

Verified intact on 2026-08-30. The hashes above were re-checked against the
three tablets' own copies and against the repository package; all matched.

## What it proved — 2026-08-29, live

Three tablets, three athletes, live 50 m targets, ISSF 50 m Rifle 3 Positions
**Indoor Qualification** and **3P Final**.

| | |
|---|---|
| Accepted physical shots | **385** |
| Distinct coordinates | **385** — no repeats |
| Paper feed requested / started / completed | **385 / 385 / 385** |
| Motor commands | **385** — no unexplained movement |
| Manual feeds | **0** |
| `ACQUISITION_FAULT` | **0** |
| Read failures · UI refusals · counter jumps · desyncs | **0** |
| Qualifications completed | **3 × 60 official shots** |
| Finals completed | **3** |
| Mid-qualification reconnects | **1** (Tablet 3, 2.3 s, reconciled, session completed correctly) |
| Unexplained shots | **0** |

Full forensic reconstruction:
[`docs/field-tests/2026-08-29-RC3F-THREE-TABLET-50M-3P-LIVE-AUDIT.md`](../field-tests/2026-08-29-RC3F-THREE-TABLET-50M-3P-LIVE-AUDIT.md)
and its JSON companion.

## What it did NOT prove

- **The 3P Final's internal state flow.** The finals journals were not
  collected (`Collect-Logs.cmd` was never run), and the Final journals its
  stage transitions rather than logging them. The Finals *completed*; the
  22-minute block, warnings, STOP, interval, series and singles are **not
  verified in the field**.
- The 10- and 5-minute CRO match warnings — all three athletes finished before
  the 10-minute mark (Tablet 3 by 44 seconds). **NOT EXERCISED.**
- Anything on Android or SETA. Neither has ever been physically tested.

## Its role from here

RC3F is the **physical evidence baseline** that v1.0 inherits. Because the
v1.0 close-out does not touch acquisition, Modbus, scoring, counter
reconciliation or paper feed, this evidence carries forward. If any of those
are ever modified, **this evidence stops applying** and a new physical test is
required before the claim can be repeated.

Three defects were found by this event, all presentation-only and all fixed
after it: `UI-LASTSHOT-DWELL-001`, `CRO-ORDER-001`, `CRO-REPEAT-002`.
