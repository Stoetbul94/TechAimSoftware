# Rule authority index

**Rules must be rechecked:**

- before a major release
- after a new ISSF / DSB rulebook print
- after a formal amendment or interpretation
- before implementing a new Finals format
- before a SETA external release

A discipline whose row says `NOT AUDITED` or `AUTHORITY NOT VERIFIED` has **not**
been cleared. Green automated tests prove implementation consistency, not rule
correctness.

| DISCIPLINE | AUTHORITY | VERSION | EFFECTIVE | LAST CHECKED | STATE FLOW | AUDIT STATUS | NEXT REVIEW TRIGGER |
|---|---|---|---|---|---|---|---|
| 50 m Rifle 3P **Final** | ISSF 6.17.3 | Edition 2025, Second Print 07/2026 | 2026-07-01 | **2026-08-27** | [50M-3P-FINAL-STATE-FLOW-CURRENT.md](50M-3P-FINAL-STATE-FLOW-CURRENT.md) | ✅ **PASS** (engine + presentation) | new print / Finals format change |
| 10 m AR **Final** | ISSF 6.17.2 | Edition 2025, Second Print 07/2026 | 2026-07-01 | **2026-08-27** | ⏳ to write | ✅ PASS on course, times, eliminations, scoring | new print |
| 10 m AP **Final** | ISSF 6.17.2 | Edition 2025, Second Print 07/2026 | 2026-07-01 | **2026-08-27** | ⏳ to write | ✅ PASS on course, times, eliminations, scoring | new print |
| 50 m Rifle 3P **Qualification** | ISSF events programme + 6.11.1.1 | Edition 2025, Second Print 07/2026 | 2026-07-01 | **2026-08-27** | ⏳ to write | **PARTIAL** — course (3×20 = 60) and 15 min preparation verified; per-position timing not extracted | before a match where timing matters |
| 10 m Air Rifle Qualification | ISSF events programme | Edition 2025, Second Print 07/2026 | 2026-07-01 | 2026-08-27 | ⏳ | **PARTIAL** — 60-shot course only | next release |
| 10 m Air Pistol Qualification | ISSF events programme | Edition 2025, Second Print 07/2026 | 2026-07-01 | 2026-08-27 | ⏳ | **PARTIAL** — 60-shot course only | next release |
| 50 m Rifle Prone | ISSF events programme | Edition 2025, Second Print 07/2026 | 2026-07-01 | 2026-08-27 | ⏳ | **PARTIAL** — 60-shot course only | next release |
| 50 m Free Pistol | — | — | — | — | ⏳ | **AUTHORITY NOT VERIFIED** | before any competition use |
| Open Practice | none | — | — | — | n/a | **NON-OFFICIAL TRAINING MODE** | — |
| Training Lab (4 programmes) | none | — | — | — | n/a | **NON-OFFICIAL TRAINING MODE** | — |
| DSB 1.20 / 1.40 / 1.60 | DSB Sportordnung | — | — | — | — | **NOT ON THIS BRANCH** (`product/seta`) | before SETA release |

## Source of record

- Manifest: [ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md](ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md)
- Which rule governs, and why: [ISSF-50M-3P-FINAL-CURRENT-RULE-DECISION.md](ISSF-50M-3P-FINAL-CURRENT-RULE-DECISION.md)
- Full audit: [ALL-DISCIPLINES-CURRENT-RULE-AUDIT.md](ALL-DISCIPLINES-CURRENT-RULE-AUDIT.md)
- Machine-readable: [ALL-DISCIPLINES-CURRENT-RULES.json](ALL-DISCIPLINES-CURRENT-RULES.json)
- Portability: [../architecture/CROSS-PLATFORM-FIX-REGISTER.md](../architecture/CROSS-PLATFORM-FIX-REGISTER.md)

Windows, Android and SETA must use **these** documents. A platform branch may
not retain an older competition format.
