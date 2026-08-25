# RC3B-DIAG — cross-discipline propagation audit

**Question:** does every live-target workflow actually use the corrected shared
acquisition engine, or do some still have their own path?

**Answer: they all use it — after two bypasses found by this audit were fixed.**
The audit was done against the source, not against the previous report.

**GLOBAL CODE FIX — PASS. GLOBAL PHYSICAL VALIDATION — PENDING.** Those are
different claims and this document never merges them.

## 1. Every live-target workflow in the codebase

From `CompetitionCatalogue.qml` (48 programmes, 4 discipline ids), the
discipline constants in the code, and the training and finals registries — not
from memory.

| Discipline id | Programmes | Source |
|---|---|---|
| `AR10` 10 m Air Rifle | free, match10/20/30/40, qualification60 (+ 6 `.p15` variants) | catalogue |
| `AP10` 10 m Air Pistol | free, match10/20/30/40, qualification60 (+ 6 `.p15`) | catalogue |
| `RIFLE50` 50 m Rifle | free, match10/20/30/40, qualification60 (+ 6 `.p15`) | catalogue |
| **`FREEPISTOL50` 50 m Free Pistol** | free, match10/20/30/40, qualification60 (+ 6 `.p15`) | catalogue |
| `PRONE50` 50 m Rifle Prone | qualification / training | controller |
| `3P50` 50 m Rifle 3 Positions | ISSF 3×20, DSB 1.40 3×20, DSB 1.60 3×40 | controller |
| `FINAL3P` 50 m 3P Final | 35-shot ISSF final | `Finals3PController` |
| 10 m Finals | `src/finals10m` | controller |
| Training Lab | `TRAINING` technical blocks, `CALLDIAG`, `WINDMAP`, `POSTRANS` | training registry |
| Sighter / Counted | a mode on all of the above, not a separate workflow | `isSighterMode` |

**`FREEPISTOL50` had never appeared in any earlier report.** It is in the
shipped catalogue with twelve programmes and it is a live-target discipline.
Its acquisition is the shared path like every other, but its omission is
exactly what this audit was for.

The `.p15` entries are 15-shot-per-page variants of the same programmes; they
change page geometry, not acquisition.

## 2. The acquisition path — one, for all of them

There is exactly **one** physical-shot pipeline:

```
TachusWidget::collectData            the single 100 ms poll
  -> MainWindow::modbusReadRegistry  mutex-guarded transport
  -> AcquisitionSequencer::poll      AcquisitionDecision rules
  -> coordinate read (rc checked)
  -> m_x/yCordList.append
  -> noteCoordinateCaptured()        the shot number IS the coordinate count
  -> emit shootCountChanged
  -> CenterPane.onShootCountChanged  the one scoring entry
  -> onPhysicalShotAccepted          the one feed authority
  -> journal / .tch / report
```

Every discipline reaches it identically; none supplies its own reader, counter,
decoder or reset. Proven by these facts, each independently checked:

- **No discipline controller touches acquisition at all.** `src/qualification`,
  `src/finals`, `src/finals10m`, `src/training`, `src/incident` and `src/mode`
  contain no reference to Modbus, `getXCord`, `checkForNewShots` or
  `TachusWidget`. The only `src/` files that do are the target layer itself and
  `src/bridge`, a downstream consumer.
- **`checkForNewShots`, `clearShootCount`, `resetShootinCount`,
  `FLUSH_SHOOT_COUNT`, `onPhysicalShotAccepted`** appear only in
  `tachuswidget.{cpp,h}`, `src/target/*`, the tests, and one QML call site
  (`LoginPage.qml:2582`, session start).
- **`m_currentShootsCount` and `m_oldResetCount` no longer exist as members.**
  They survive only in comments explaining what they used to do. No
  discipline-specific counter is left that could diverge.
- **Every assignment to an acquisition counter is inside
  `AcquisitionSequencer.h`.** The old `m_currentShootsCount = actual` — the
  Tablet-02 line — is commented out at `tachuswidget.cpp:1863`.
- **`shootCountChanged` has two emitters:** the guarded physical path, and
  `uxShoot` (Demo), which publishes `m_xCordList.count()` — the same number, so
  the two paths cannot disagree about what shot 11 is.

## 3. Bypass paths — two found, both fixed

| Bypass | Consequence | Fix |
|---|---|---|
| `src/bridge/coachreportfeeder.cpp` compared the first coordinate against **minus one** to decide whether a match had coordinates | A NaN compares equal to nothing, so after the sentinel change the probe said "we have coordinates" when there were none, and **NaN flowed into the analytics engine** — MPI, group size, every derived Coach Report figure | asks `coordinateHasValue()`, and **per shot**, not once per match |
| `appsettings.cpp` × 2 — the `.tch` **persistence** loops wrote `getXCord(i+1)` unguarded | Safe by construction today, but persistence is reopened, rescored, reported and fed to the coach engine long afterwards | stops rather than writing `nan` if that construction is ever broken |

No other bypass exists. All **11** coordinate consumers in the product are now
enumerated and guarded (§6 below).

## 4. Reset paths

| Path | Implementation | Verdict |
|---|---|---|
| 10-shot series flush | `decideCounterReset` → `ResettingCounter` → target-confirmed `ResetComplete` | PASS |
| Session start (`LoginPage`) | `resetShootinCount()` — writes the register, reads it back, and **does not assign a baseline**; leaves the sequencer SYNCHRONIZING so the next poll adopts what the target really reports. One adoption path, not two | PASS |
| Sighter → Counted (`changeSighterMode`) | calls the same `resetShootinCount()` | PASS |
| 3P position transition | no acquisition code of its own | PASS |
| Finals phase / position | no acquisition code of its own | PASS |
| Discard / restart / new match | same central reset; the feed coordinator is notified from inside it | PASS |

No path sets a baseline before the hardware reset is authoritative.

## 5. Reconnect paths

One: `attemptTargetReconnect()` → `planBaselineAdoption()`, which proves
`priorTotal + baseline == capturedShots` and faults `AdoptionWouldDesync`
rather than resuming. No discipline has its own adoption. Training,
Qualification, Finals, 3P transitions, sighters, counted, restart/recovery and
Home → Start Session all reach the same function.

## 6. Coordinate consumers — all 11, categorised

| Consumer | Category | Enforcement |
|---|---|---|
| `CenterPane.onShootCountChanged` | SCORING CRITICAL | refuse before fetch |
| `CenterPane.readDataFromBAckEnd` | SCORING CRITICAL | refuse before fetch |
| `broadCastNewShoot` (UDP to the range) | SCORING CRITICAL | refuse + fault |
| `updateShootData` (server lane file) | SCORING CRITICAL | refuse + fault |
| `updateSetaShootData` (SETA lane CSV) | SCORING CRITICAL | refuse + fault |
| `appsettings.cpp` `.tch` save ×2 | PERSISTENCE | stop, do not fabricate |
| `CoachReportFeeder` | ANALYTICS | ask, per shot |
| `getPDFString` | REPORTING | dash |
| `getXMPIForShoot` / `getYMPIForShoot` | REPORTING | NaN → em dash |
| `SeriesComponent` ×3, `MatchReportView` | DISPLAY ONLY | do not draw; no false fault |

Scoring for 10 m Rifle, 10 m Pistol, 50 m Rifle, 50 m Free Pistol, 3P, 3P
Final, Training and Qualification all runs through
`CenterPane.calculateShootingSocre`, reached only via `onShootCountChanged` —
which refuses first. There is no second scoring entry.

## 7. Paper feed

One authority, `onPhysicalShotAccepted`, with one call site, positioned after
the coordinate capture; the read-failure branch returns before either. Recovery
and reconnect publish no shot, so they can request no feed. Sighter → counted
numbering restarts are handled by the coordinator's `(identity, kind)` key.

## 8. Finals-specific

`Finals3PController` and `src/finals10m` own commands, stages, windows, timing
and shot *bookkeeping* — and no acquisition. They contain no Modbus, no
coordinate accessor, no counter and no reset. Pausing for a command pauses the
competition clock, not the poll. `EstIncidentController` is asserted by test to
contain no reference to Modbus, `MODREADER`, `TachusWidget`, either counter or
either coordinate accessor. The 50 m 3P Final — where the repeated 10.8 was
seen — has no acquisition code of its own to fix.

## 9. Training-specific

Training Lab programmes (`TRAINING` technical blocks, `CALLDIAG`, `WINDMAP`,
`POSTRANS`) define block size, series advancement and sighter policy, and none
of them touches acquisition. The flush boundary is fixed at
`FLUSH_SHOOT_COUNT = 10` in the shared layer and is not a discipline setting,
so an unlimited-series training exercise crosses exactly the same boundary as a
60-shot qualification.

## 10. Propagation matrix

| Workflow | Shared acquisition | Flush fix | Reconnect fix | Coord guard | Read guard | Feed guard | Automated |
|---|---|---|---|---|---|---|---|
| 10 m Air Rifle — training/free | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 10 m Air Rifle — match/qualification | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 10 m Air Pistol — training/free | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 10 m Air Pistol — match/qualification | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 50 m Rifle Prone — training | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 50 m Rifle Prone — qualification | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| **50 m Free Pistol — training/match** | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 50 m 3P — training | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 50 m 3P — qualification (ISSF, DSB 1.40, DSB 1.60) | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 50 m 3P Final | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 10 m Finals | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Training Lab (blocks, call & diagnose, wind map, position transition) | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Sighter mode | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Counted mode | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Reconnect | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Restart / recovery | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Demo / simulated (`uxShoot`) | N/A — no target | N/A | N/A | PASS | N/A | PASS | PASS |

`PASS` is written only where the source path was traced to the shared engine.
**No `OPEN` and no `BYPASS FOUND` remain** — the two that were found are fixed
and covered by regression.

## 11. Physical status

Every cell above is a **code** result. `Physical` is a separate column and it is
**PENDING for every row**; nothing in this table has been fired at. See
`rc3b-diag-physical-qualification-plan.md`.
