# Saturday 2026-08-29 — 50 m 3P readiness

Only the two modes the operator will actually use. Written to be useful, not
reassuring.

---

## 50 m Rifle 3 Positions — FINAL

| | |
|---|---|
| **Rule authority verified?** | **YES.** ISSF Rule Book 2026, Edition 2025 (Second Print 07/2026), effective 1 July 2026, Rule 6.17.3, read section by section from the official PDF on 2026-08-27 |
| **Software flow matches?** | **YES.** Every duration, shot count, elimination point and CRO command phrase checked against the verified text. No mismatch |
| **Automated coverage** | 379 checks in `tests/finals`, including the full 35-shot course driven through the real panel by the real controller, early-completion and two late-transition cases, target-mode sequence, and the standing-transition UI |
| **Known open defects** | Tie-breaking (6.17.3 i) not implemented — **cross-lane, cannot be done on a single target**. 10 m-style report/PDF for a 3P Final exists (`FinalsReportView`); reload consistency not exercised |
| **Physical test status** | **NONE.** The 3P Final has never been fired at a target with this software. All evidence is DEMO / controller-driven |
| **Saturday risk** | **LOW for the rule engine. MEDIUM overall**, entirely because it has never run against live hardware |
| **Code change required before Saturday?** | **NO** |

The reported "extra 5-minute Standing sighting block" was investigated twice and
does not exist in the engine. What did exist was a presentation gap — the
30-second interval after the 22:00 STOP showed a fresh countdown with nothing
saying what it was for. That is now labelled `NEXT · STANDING SERIES 1 — 5 MATCH
SHOTS — WAIT FOR LOAD`. **That correction is in the working tree, not in RC3E.**

---

## 50 m Rifle 3 Positions — QUALIFICATION

| | |
|---|---|
| **Rule authority verified?** | **PARTIAL.** Course of fire **3 × 20 = 60 shots** confirmed from the ISSF events programme; **15 minutes** preparation and sighting confirmed from 6.11.1.1 |
| **Software flow matches?** | **AMBIGUOUS.** The two items verified above match (`matchShootCount === 60` gates `is3PMatch`; `m_prepTimeMinutes = 15`). **Per-position shooting time, position-transition timing and the qualification command sequence were NOT extracted from the rulebook this round** and are therefore not confirmed |
| **Automated coverage** | The qualification path is covered by the reliability and QML suites and by the mode-isolation tests. Those prove the software is self-consistent — **they do not prove it matches the rule** |
| **Known open defects** | None specific to 3P qualification. `FINALS-TCH-SIGHTER-001` and `UI-LAYOUT-001` remain open but are not qualification-specific |
| **Physical test status** | RC3B and RC3C fired 10 m Open Training and a 10 m Final on live hardware with a clean acquisition record. **50 m 3P qualification has not been fired with this software** |
| **Saturday risk** | **MEDIUM** — not because anything is known to be wrong, but because the per-position timing has not been checked against the rule and the discipline has no physical record |
| **Code change required before Saturday?** | **NO** — and none should be made without first extracting the qualification timing from the rulebook |

### What would reduce that risk, in order

1. **Extract the 50 m 3P qualification timing from the rulebook** (per-position
   shooting time and whether the clock is continuous or per position) and diff it
   against `QualificationController`. Perhaps an hour of work; it converts
   AMBIGUOUS into PASS or MISMATCH.
2. **A short live 3P qualification on the range before the match** — even 3 × 5
   shots — to give the discipline its first physical evidence.
3. Run `Collect-Logs.cmd` after both, so whatever happens is diagnosable.

---

## Blunt summary

The **Final** is the mode that was worried about, and it is the one now proven
against the official rule. The **qualification** is the mode nobody has been
worried about, and it is the one carrying the larger unverified surface.

Neither has ever been fired at 50 m with this software.
