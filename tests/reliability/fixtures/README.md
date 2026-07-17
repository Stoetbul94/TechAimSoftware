# Reliability golden fixtures (M1)

Committed, human-readable journal fixtures verified BYTE-FOR-BYTE by
`tst_fixtures.cpp` on every harness run, plus by expected validation
classification. They are the cross-version regression anchors (spec §25.8):
future builds must keep reading these exact bytes.

| Fixture | Classification | Content |
|---|---|---|
| `fixture_qualification_clean.jsonl` | Clean | 50m Prone: prep → sighting → sighter → match → 3 shots → stage → completed → closed |
| `fixture_finals_clean.jsonl` | Clean | 3P Final: sighter, 3 officials, position change, StateSnapshot, completed, closed |
| `fixture_sighter_official.jsonl` | Clean | minimal sighter + official shot pair |
| `fixture_corrected_shot.jsonl` | Clean | ShotRescored + ShotInvalidated corrections |
| `fixture_tail_truncated.jsonl` | TailTruncated | power-loss signature: final line cut mid-bytes |
| `fixture_corrupt_internal.jsonl` | CorruptInternal | one digit flipped inside an interior payload |
| `fixture_unsupported_version.jsonl` | UnsupportedVersion | trailing line with `pv:99` |

## Regeneration (explicit, never automatic)

Normal test runs ONLY READ these files and fail if the deterministic
generators in `tst_fixtures.cpp` produce different bytes. To regenerate
after an intentional format change:

```
reliability_tests --write-fixtures
```

then review the diff in git — a fixture diff IS a journal-format change
and must be treated as such (schema version discipline, spec §5).

These files are marked `-text` in `.gitattributes`: byte-exact content,
no EOL conversion, ever.
