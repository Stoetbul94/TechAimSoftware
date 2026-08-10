# `m_hardwareCheckDisabled` — audit

Requested after LOGIN-LINK-001, where a liveness probe guarded by this flag
silently did nothing. Audited offline on 2026-08-10. **No behaviour was
changed by this audit.**

---

## The finding, first

```cpp
// ModReader/forms/tachuswidget.h:609
bool m_hardwareCheckDisabled = true;
```

**It is initialised to `true` and is never assigned anywhere in the
repository.** There is no setter, no config key, no UI control, no command-line
switch. Every site it guards is therefore permanently disabled, and has been
for the whole life of the flag.

Verified exhaustively:

```
grep -rn "m_hardwareCheckDisabled" --include=*.cpp --include=*.h .
  (excluding release/, debug/, worktrees)
→ 1 declaration, 5 guard sites, 2 comments. Zero assignments.
```

It is not a runtime mode. It is a constant `true` wearing the costume of a
switch, which is precisely why it is dangerous: every guard site reads as
"unless the operator disabled hardware checks", and none of them mean that.

## Why it defaults true

No commit message, comment or document states a reason. The two comments at the
guard sites say *"no physical target attached — skip motor setup writes"*,
which reads as a **development convenience** for running the application on a
machine with no target — the same intent as demo mode, added before demo mode
covered it. That is inference from the code, not a recorded decision.

RECONNECT-001 already recorded its effect (tachuswidget.cpp:1815): the
disconnect check *"is additionally gated on `m_hardwareCheckDisabled`, which
DEFAULTS TO TRUE, so it never ran."* That fix routed around the flag by making
the read return value authoritative rather than removing it.

## Every reference

| # | Site | What it disables | Live impact |
|---|---|---|---|
| 1 | `tachuswidget.cpp:521` | Re-entry guard: `m_hardwareDisconnected && !isHardwareConnected()` | Dead. Superseded by the RECONNECT-001 link-state machine, which does not consult the flag. |
| 2 | `intiateAutoMovementSetup()` :1453 | Motor **setup** writes: 8196 = auto-paper mode, 8197 = timer, 8198 = radius | **Never executed.** The target runs on its own retained/default settings. |
| 3 | `intiateAutoMovementSighterSetup()` :1479 | Same three registers for sighter mode | **Never executed.** |
| 4 | `checkForNewShots()` :1875 | `newShotsCount == 0` → probe `isHardwareConnected()` → emit `hardwareDisconnected` | Dead. Superseded by RECONNECT-001's authoritative read result. |
| 5 | `clearShootCount()` :2114 | Hardware counter reset write (8193 = 0), 3 attempts | **Never executed** — see below; the counter is still reset, by a different path. |

Sites 1 and 4 being dead is harmless: both were disconnect detection, and both
have been replaced by a mechanism that does not consult the flag.

## Why the shot counter is still reset

Site 5 looks alarming and is not. `clearShootCount()` is **not** the numbering
authority. `resetShootinCount()` (tachuswidget.h:373) is, and it gates on
`isAppDemoMode` — the inverted-name flag meaning *"is LIVE"* — **not** on
`m_hardwareCheckDisabled`:

```cpp
if (isAppDemoMode) {                       // "is live"
    for (int attempt = 0; attempt < 3; ++attempt)
        if (m_mainWindow->modbusWriteSingleRegister(8193, 0) != -1) break;
}
```

Confirmed in the 50 m field log, 2026-08-10:

```
ACQDIAG resetShootinCount ENTER  baselineBefore=0 liveFlag=1 modbusConnected=1
ACQDIAG resetShootinCount READBACK rc=2 hwCounter=0
```

The write happened and the readback confirmed it. `clearShootCount()`'s guarded
write is redundant dead code.

## Why paper feed still works

This was the sharpest question, and the answer is clean.

The flag disables motor **setup** (registers 8196/8197/8198 — mode, timer,
radius). It does **not** touch the per-shot feed command, which is issued by
`PaperFeedCoordinator` from the accepted-shot site:

```cpp
req.kind = isSighter ? ShotKind::Sighter : ShotKind::Counted;
m_feed.onShotAccepted(req);          // no flag test, no range test
```

So the sequence at 50 m on 2026-08-10 was: setup writes skipped → target used
its own retained configuration → every accepted shot still issued a feed
command → **paper fed correctly after every accepted shot** (operator-observed).

The practical conclusion is uncomfortable but important: **the application has
never configured the motor, and the hardware has been working on its own
settings the whole time.** That has been true of every field test to date.

## Decision taken

**The flag was left exactly as it is at all five sites.**

LOGIN-LINK-001 is solved without touching it: the new liveness probe simply
does not consult it, and that is documented at the call site. This satisfies
the instruction to prefer a solution that does not change broader motor/setup
behaviour.

Flipping it to `false` would, on the next connection, begin writing three motor
registers that this application has **never** written on any target. That is a
hardware behaviour change with no physical evidence behind it, and it must not
be made offline.

## Recommended follow-up (needs hardware, do not do offline)

1. Decide whether the application *should* configure the motor at all, or
   whether relying on the target's retained settings is the intended design.
   `motor_movement_time` in `config.ini` implies the former was intended.
2. If it should: enable sites 2 and 3 **on a bench target**, observe the paper
   behaviour before and after, and confirm 8197 matches `motor_movement_time`.
3. Delete sites 1, 4 and 5 — dead code superseded by RECONNECT-001 and
   `resetShootinCount()`.
4. Then remove the flag entirely. A switch that is never switched should not
   survive as a permanent `true`.

Until 1–3 are done on hardware, **the flag stays.**
