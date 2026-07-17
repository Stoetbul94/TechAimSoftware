# Session Reliability Layer — M1 implementation note (reliability core)

Blueprint: docs/session-reliability-implementation-spec.md (§4–§8, §12,
§13, §24, §25, §28 M1). Scope: **library and tests only** — no production
controller uses any of this yet; Finals still writes its legacy M0-located
journal unchanged.

## Implemented components (all QtCore-only, `ta::rel`, via Reliability.pri)

```
src/reliability/
  core/    ReliabilityError.h  ReliabilityResult.h  FixedPoint.h
  events/  EventTypes.h  DomainEvent.h  EventEnvelope.h
           EventSerializer.{h,cpp}  EventRegistry.{h,cpp}
  journal/ HashChain.{h,cpp}  JournalWriter.{h,cpp}
           JournalReader.{h,cpp}  JournalValidator.{h,cpp}
  reducer/ SessionState.{h,cpp}  SessionReducer.{h,cpp}
```

The `QT = core` test binary compiling proves the no-QML/GUI rule; layering
follows spec §29 (events depend on nothing; journal on events(+storage
sync); reducer on events; nothing depends back).

## Typed error model (core/)

`ReliabilityError` (26 codes: argument/event, serialization, file I/O,
JSON structure, sequencing, integrity, reduction, whole-journal) +
`ErrorSeverity`. `ErrorInfo` carries code, severity, operator-safe
message, technical detail, affected path, sequence, recoverability;
`ReliabilityResult` is the carrier. The enum is the identity — QString is
presentation only.

## Fixed-point authoritative values (core/FixedPoint.h)

`ScoreTenths` (i16, 0.1), `CoordinateHundredthMm` (i32, 0.01 mm),
`CentiDegrees` (i32, 0.01°), `ElapsedMilliseconds` (i64),
`SplitMilliseconds` (i32). Explicit construction only; no implicit double
anything; `fromDouble` is the single sanctioned conversion boundary with
**half-away-from-zero** rounding for decimal quantities (deterministic
representation-error compensation: −1.235 mm → −124) and truncation for
native milliseconds (spec §6); overflow and NaN/Inf are typed statuses;
serialization is the raw integer; `toDisplayDouble()` is display-only.

## Event catalogue (events/)

`DomainEvent` = `std::variant` of 27 typed payload structs — 18 active
(SessionStarted/Configured, AthleteAssigned, Preparation/Sighting/
OfficialMatchStarted, Shot/SighterAccepted, StageCompleted,
PositionChanged, TimerStarted/Paused/Resumed/Expired, MatchCompleted,
SessionSuspended/Resumed, StateSnapshot) and 9 reserved (ShotInvalidated,
ShotRescored, SeriesAdjusted, CrossShotRecorded,
EquipmentMalfunctionRecorded, PenaltyIssued, RecoveryStarted,
RecoveryCompleted, SessionClosed). Every event: stable `kType` string,
`kVersion` (all pv 1), typed `validate()`. No QVariantMap anywhere.
`EventRegistry` maps type string → version, variant index, durability
(S/F/A per spec §12 incl. D2 sighter=flush), broadcast and reducer
classifications; unknown types / newer pv are typed failures, never
guessed.

## Envelope + file format (spec §5)

One compact UTF-8 JSON envelope per LF-terminated line:
`sv, pv, sid, lane, seq, tw, tm, t, p, (av, dev on seq 0), ph, h`.
`tw` is UTC `yyyy-MM-ddTHH:mm:ss.zzz`; `tm` monotonic ms; `seq` 0 must be
SessionStarted (enforced). `replayed` exists in memory only.

**Deterministic serialization**: `OrderedJsonWriter` emits fields in the
frozen order above (payload field orders equally frozen; QJsonObject
ordering is never relied on); integers hand-formatted (locale-free);
non-ASCII passes through as raw UTF-8; only mandatory escapes. The same
envelope always serializes to the same bytes on every platform.

## Hash chain (journal/HashChain)

Chained SHA-256, stored **truncated to 128 bits / 32 lowercase hex** —
truncation is explicitly mandated by spec decision D8 (documented choice).
Uniform procedure: `h = hex(SHA256(ph_ascii + core_bytes))[0..31]` where
`core_bytes` is the deterministic serialization WITHOUT `h` (it contains
`ph`). Genesis defined precisely: seq 0 uses `ph` = 32×'0'; its input is
that genesis material + the core (session-derived via the embedded sid).
**Verification is structured**: deserialize, deterministically
re-serialize the core, recompute, compare — no `,"h":"` string surgery
(supersedes the spec's suffix-scan wording per the M1 instruction; sound
because serialization is deterministic). Consequence (documented): a
journal whose line endings were mangled to CRLF by transfer tools still
verifies — validation is structural, not byte-superstitious.

## Journal writer (synchronous, DI)

`JournalWriter(identity, IJournalFile*)` assigns sequences, chains hashes,
appends one complete line, applies the registry durability policy
(append / +flush / +fsync via the M0 `StorageSync` boundary) with an
optional override, and returns the completed authoritative envelope.
Timestamps are caller-supplied (no clock reads — deterministic tests).
Exact append semantics: `ok` = fully appended AND durability achieved;
`!ok && !lineAppended` = nothing written, writer state unchanged,
retryable; `!ok && lineAppended` = bytes exist but flush/fsync failed
(seq/chain advanced). A **partial physical write poisons the writer**
(torn tail on disk; owner must reopen/recover). A failed append is never
reported successful. Latency metrics per write (serialize/append/flush/
fsync µs, totals, maxima). Measured on target hardware (Release,
2026-07-17): serialize 17 µs, append <1 µs, flush 29 µs, fsync 618 µs —
within spec §26 budgets (10 ms typical / 100 ms graduation ceiling D7,
asserted in the harness).

## Journal reader

Line-by-line, raw bytes preserved per line, strictly non-mutating.
Detects: missing final LF (torn-tail flag), blank lines (distinguished
from malformed), invalid UTF-8 (typed), per-line typed parse errors.
Sources: file path, QIODevice, QByteArray. A lone trailing `\r` per line
is tolerated (CRLF-mangled transfers; see hash-chain note).

## Journal validator

Classifications: `Clean, Empty, TailTruncated, CorruptInternal,
UnsupportedVersion, SessionMismatch`. Position-sensitive rule: any defect
confined to the FINAL line → TailTruncated (safely truncatable);
interior → CorruptInternal; newer sv/pv/unknown type → UnsupportedVersion
regardless of position; sid inconsistency → SessionMismatch. Checks:
schema/payload versions, required fields, type/payload match, sid
consistency (header payload vs envelope too), strict monotonic seq
(duplicate/gap/regression typed — the §9D degraded-gap allowance is
deliberately absent until M2's markers exist), chain linkage, hash
recompute, snapshot `lastAppliedSeq == seq−1`, completion detection.
Report: classification, valid prefix count + envelopes (replay input for
M3), first invalid line/seq, typed error, tail-truncatable and
internal-corruption flags. It never repairs or truncates (M3's call).

## SessionState + reducer

Typed value state (spec §7 reduced to the M1 catalogue): identity,
config, lifecycle (`None/Active/Suspended/Complete/Closed`) + match phase,
officials/sighters/crossShots records (corrections mark, never delete),
corrections/adjustments/incidents, reducer-owned totals, timer state,
`lastSeq`, and `std::variant<monostate, QualificationState, Finals3PState,
TrainingState>`. Deterministic byte-stable state serialization +
deserialization + exact equality.

`SessionReducer::apply(state, envelope)` is pure and deterministic — no
I/O, clocks, settings, signals, QML, controllers. Check order per spec §8:
payload validation → sequence defence (`seq == lastSeq+1`) → transition
legality → apply → post-invariants. **Totals are always re-derived from
the records** (Σ effective official tenths + adjustments; penalties are
match-level, series adjustments stage-level) — never incremented anywhere
else. Duplicate shot identity (shotNumber or non-zero externalId)
rejected. `MatchCompleted` totals must equal the reduced state.
Corrections fully reduced (invalidate / rescore keep original values in a
`CorrectionEntry` audit list). `RecoveryStarted` is an idempotent marker.

## StateSnapshot (spec §13, format only in M1)

Payload: `stateVersion`, `lastAppliedSeq`, integrity metadata (official/
sighter counts, totalTenths), and the full deterministic SessionState
serialization embedded as a JSON string (keeps events/ independent of
reducer/ per the layering table; byte-stable under re-serialization, so
snapshots participate in the hash chain like any line). The reducer
verifies metadata AND deep-equality of the embedded state against the
folded state (drift detector). The validator enforces
`lastAppliedSeq == seq−1`. Replay-from-snapshot optimization is M3.

## Golden fixtures (tests/reliability/fixtures/)

7 committed, human-readable journals (clean qualification, clean finals
incl. snapshot, sighter+official, corrected-shot, tail-truncated,
corrupt-internal, unsupported-version) — verified byte-for-byte against
their deterministic generators and by expected classification on every
run. Regeneration is explicit only: `reliability_tests --write-fixtures`
(fixtures/README.md); normal runs never write them. `.gitattributes`
marks them `-text` (no EOL conversion).

## Fault injection (test-only)

`MemoryJournalFile` (tests/reliability/test_support.h) injects open/write/
flush/sync failures and one-shot partial writes through the `IJournalFile`
seam. No production retry/recovery behaviour exists (M2).

## Test coverage

`tests/reliability` harness: **527 checks, 0 failures** (was 23 at M0;
the M0 checks are preserved inside the same binary). Modules: storage
(M0), fixed point (rounding/overflow/round-trip incl. the mandated
10.4→104 and −1.235→−124 vectors), events+registry, deterministic
serialization (golden layout bytes, Unicode, escaping, 27-type
round-trip), hash chain (genesis, tamper suite: payload/seq/ph edits,
line removal/insertion/reorder/duplication, wrong session, CRLF, empty),
writer (semantics, durability policy, metrics, all injected faults,
poisoning, real-file+fsync), reader, validator (all classifications, a
power-loss byte sweep over the final line), reducer (legal/illegal
transitions, duplicates, totals, sighter exclusion, corrections, timers,
completion, suspend/resume), snapshot, and golden fixtures incl. a full
fixture replay. Finals harness: **178 checks, 0 failures** (untouched).
Full app builds clean; **no app source file changed** (only
Reliability.pri gained the new library files).

## Known limitations / deliberately deferred (M2/M3+)

- No SessionStore, EventBus, retry queue, degraded mode, health model,
  emergency spill (M2). Writer flush/sync failures are reported typed;
  nobody retries yet.
- No recovery UI, startup replay, truncation/repair, RecoveryCoordinator,
  controller `primeFrom` (M3).
- Finals3PController still writes the legacy line format to the M0
  AppData location; nothing produces typed envelopes in production yet
  (M2 migration).
- Validator has no degraded-gap allowance (§9D) until M2 markers exist.
- SchemaMigrator not needed yet (every pv is 1); newer versions are
  classified, never migrated.
- Session UUID generation, filename scheme `session_<ts>_<uuid8>.jsonl`,
  rotation/segmenting (Training) arrive with M2's JournalManager.
- Qualification/RMS/spectator/report-reconstruction untouched (M4+).
