# Tech Aim — shared-engine consumer audit and validation matrix

Audited offline at commit `97c76df` on `feature/rc2e-latency-and-reset`,
2026-08-10, with no physical target available.

**Implementation coverage and physical validation are different claims and are
never merged in this document.** A fix in a shared layer *applies* to every
consumer; it is *validated* in a workflow only if that workflow was actually
exercised.

---

## Status vocabulary

**APPLIES** · **DOES NOT APPLY** · **AUTOMATED PASS** · **EMULATOR PASS** ·
**UI VISUAL PASS** · **PHYSICAL PASS** · **PHYSICAL PENDING**

---

## 1. Consumer audit — is the engine actually shared?

**The decisive check: which files touch the acquisition path at all?**

```
grep -l "modbusReadRegistry|checkForNewShots"  (excluding tests/build output)
  → ModReader/forms/tachuswidget.cpp
  → ModReader/src/mainwindow.cpp
```

**No discipline, screen or controller has its own shot poll, its own Modbus
read, or its own acceptance rule.** Every workflow that receives a physical
shot goes through `TachusWidget`.

| Shared authority | Owner | Bypasses found |
|---|---|---|
| Target discovery / fingerprint | `TargetDeviceSelector` via `chooseStartupPort()` | **none** |
| Modbus transport | `ModbusAdapter` (mutex-serialised) | **none** |
| Acquisition decision | `ta::target::decidePoll()` | **none** |
| SYNCHRONIZING / ACQUIRING / FAULT | `TachusWidget::m_acqState` | **none** |
| Reconnect | `TachusWidget::onTargetLinkLost()` / `attemptTargetReconnect()` | **none** |
| Shot-numbering reset | `resetShootinCount()` | **none** |
| Paper feed | `PaperFeedCoordinator` | **none** |
| Session persistence / recovery | `SessionStore`, `SessionReducer`, `RecoveryCoordinator` | **none** |
| Operator target status | `TargetStatusPanel` → `MODREADER` properties | **none** |

The 50 m references in `CenterPane.qml` (`gameRange == 50`) are **display
geometry only** — a 500 mm target face instead of 155.5 mm, and decimal
visibility. They do not touch acquisition.

`decidePoll()` was extracted specifically so the automated gates call the same
function production calls, rather than a re-implementation that can drift.

---

## 2. 50 m Prone — SHARED ENGINE VERIFIED

Today's physical session is .22 / 50 m, so this was audited first.

| Shared behaviour | Result |
|---|---|
| Target discovery | **APPLIES** — no 50 m-specific path |
| Modbus transport | **APPLIES** |
| Acquisition result model | **APPLIES** — `decidePoll()` |
| SYNCHRONIZING before READY | **APPLIES** |
| ACQUISITION FAULT | **APPLIES** |
| Reconnect | **APPLIES** |
| Shot acceptance | **APPLIES** |
| SessionStore / recovery | **APPLIES** |

**Discipline-specific layer** (not shared, and not yet physically exercised):

- Sighters / match classification — `changeSighterMode()`, routed through the
  central `resetShootinCount()`, so PAPER-FEED-002's notification covers it
- Series handling and 50 m scoring configuration — `calculateShootingSocre()`
  with `gameRange == 50`
- **`radOf10Ring = 5.2` for 50 m Rifle remains unconfirmed** against the
  official rulebook. This is a pre-existing open item, not new, but it affects
  scoring accuracy in today's session and should be treated as provisional.
- Session completion and reports

**Status: SHARED ENGINE VERIFIED · PHYSICAL PENDING**

## 3. 50 m 3 Positions — SHARED ENGINE VERIFIED

All shared rows identical to 50 m Prone. Discipline-specific concerns:

| Item | Notes |
|---|---|
| Kneeling / Prone / Standing sighters + match | Gated on `is3PMatch` |
| Position transitions | `positionWatch`, a 500 ms declarative poll; rollover at 20/40 match shots gives a sighting break |
| Shot-numbering reset per position | Routes through `resetShootinCount()` → **enters SYNCHRONIZING and notifies the feed coordinator**, so each position change re-reads the target baseline rather than assuming it |
| Current-position recovery | `globalMatchModel` carries a `position` role (0=K 1=P 2=S) |
| Completion / reports | Discipline-specific |

**One risk worth stating plainly.** Each 3P position change now triggers a
SYNCHRONIZING cycle. That is correct and safer than the old behaviour, but it
means the target baseline is re-read three times in a 3P match instead of once.
It has **never been exercised on hardware**, and 3P is not in today's planned
session.

**Status: SHARED ENGINE VERIFIED · PHYSICAL PENDING**

---

## 4. Paper feed at 50 m — applicability

`checkAutoFeedMode()` returns `true` unconditionally, and the feed hook at the
accepted-shot site carries **no range or discipline condition**:

```cpp
req.kind = isSighter ? ShotKind::Sighter : ShotKind::Counted;
m_feed.onShotAccepted(req);          // no gameRange test anywhere
```

| Workflow | Software applicability |
|---|---|
| 10 m Air Rifle / Air Pistol | **APPLIES** — physically confirmed |
| 50 m Prone | **APPLIES (software)** — hardware capability unverified |
| 50 m 3P | **APPLIES (software)** — hardware capability unverified |
| Finals / Training Lab | **APPLIES (software)** |

> **Open question for the operator, not answerable from the code.** The software
> will issue a motor command for every accepted 50 m shot. Whether the 50 m
> target Arnold uses has a paper-feed motor, and whether `motor_movement_time`
> suits 50 m paper, is a **hardware** question. If the 50 m target has no feed
> mechanism the command is harmless but pointless; if it has one with different
> travel, the duration may need adjusting. **This should be checked before the
> session rather than discovered during it.**

---

## 5. Validation matrix

Columns: 10AR = 10 m Air Rifle, 10AP = 10 m Air Pistol, 50P = 50 m Prone,
50-3P = 50 m 3 Positions, Prac = Practice/Training, Match, Fin = Finals,
TL = Training Lab live, Rec = Recovery.

| Feature / Defect | Shared layer | 10AR | 10AP | 50P | 50-3P | Prac | Match | Fin | TL | Rec | Automated | Emulator | UI visual | Physical |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Automatic target discovery | `TargetDeviceSelector` | ✅ | A | A | A | ✅ | A | A | A | A | **PASS** | Partial | — | **10AR PASS** |
| COM / device display | `TargetStatusPanel` | A | A | A | A | A | A | A | A | A | — | PASS | **PASS** | **PENDING** |
| Disconnect detection | `onTargetLinkLost()` | ✅ | A | A | A | ✅ | A | A | A | A | PASS | PASS | **PASS** | **10AR PASS** |
| Reconnect | `attemptTargetReconnect()` | ✅ | A | A | A | ✅ | A | A | A | A | PASS | PASS | **PASS** | **10AR PASS** |
| Synchronization | `decidePoll()` | A | A | A | A | A | A | A | A | A | **PASS** | **PASS** | **PASS** | **PENDING** |
| Normal shot acquisition | `decidePoll()` | ✅ | A | A | A | ✅ | A | A | A | A | **PASS** | **PASS** | — | **10AR PASS** |
| Forward counter anomaly | `decidePoll()` | A | A | A | A | A | A | A | A | A | **PASS** | **PASS** | **PASS** | **PENDING** |
| Backwards counter anomaly | `decidePoll()` | A | A | A | A | A | A | A | A | A | **PASS** | **PASS** | shared UI | **PENDING** |
| Sighter handling | `changeSighterMode()` | ✅ | ✅ | A | A | ✅ | A | A | A | A | PASS | PASS | — | **10AR/10AP PASS** |
| Counted / Match handling | discipline | ✅ | ✅ | A | A | ✅ | A | A | A | A | PASS | PASS | — | **10AR PASS** |
| Sighter → Counted feed reset | `resetShootinCount()` | ✅ | A | A | A | ✅ | A | A | A | A | **PASS** | — | — | **10AR PASS** |
| Position transition | 3P-specific | n/a | n/a | n/a | A | A | A | n/a | n/a | A | — | — | — | **PENDING** |
| Session recovery | `SessionStore` | ✅ | A | A | A | ✅ | A | A | A | ✅ | PASS | — | — | **10AR PASS** |
| Reports | discipline | ✅ | A | A | A | ✅ | A | A | A | A | PASS | — | — | **PENDING** |
| Paper feed | `PaperFeedCoordinator` | ✅ | A | A(sw) | A(sw) | ✅ | A | A | A | A | **PASS** | — | — | **10AR PASS** |

**Legend:** ✅ = physically passed in that workflow · A = APPLIES, physical
pending · n/a = DOES NOT APPLY · A(sw) = applies in software, hardware
capability unverified.

**Nothing is marked PHYSICAL PASS on the strength of shared code alone.**
10 m Air Pistol is **SHARED ENGINE CONSUMER CONFIRMED · EMULATOR / REAL-
APPLICATION PATH PASS · PHYSICAL PENDING** — it drove the emulator through the
production path but fired no pellets.

---

## 6. Physical-only checks remaining

Nothing below can be settled offline.

| Check | Why it needs hardware |
|---|---|
| Real USB device description | Emulator reports "Emulated / network target" |
| Real COM port, and re-enumeration to a different port | No serial device present |
| READY with a real adapter | — |
| Unplug → DISCONNECTED → RECONNECTING → SYNCHRONIZING → READY | Requires a physical cable |
| 50 m Prone end-to-end | Discipline never physically exercised |
| 50 m 3P position transitions | Never physically exercised |
| Paper feed at 50 m | Hardware capability unknown |
| 50 m scoring accuracy (`radOf10Ring`) | Needs calibration or rulebook |
