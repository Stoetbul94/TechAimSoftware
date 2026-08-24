# RC3B-DIAG — cross-discipline impact audit

The 2026-08-23 defects are not discipline defects. They live in `TachusWidget`,
the one acquisition path every discipline polls, and in the two coordinate
accessors every score is built from. Nothing was fixed in a discipline, a
workflow or a screen.

**A central fix is globally applicable the moment it is central. It is not
globally *proven* until a tablet says so.** Those are different columns below
and they are never merged.

| Surface | Shared fix | Automated | Emulator | Physical |
|---|---|---|---|---|
| 10 m Air Rifle | APPLIES | PASS | PENDING | PENDING |
| 10 m Air Pistol | APPLIES | PASS | PENDING | PENDING |
| 50 m Prone | APPLIES | PASS | PENDING | PENDING |
| 50 m 3P | APPLIES | PASS | PENDING | PENDING |
| Training | APPLIES | PASS | PENDING | PENDING |
| Qualification | APPLIES | PASS | PENDING | PENDING |
| Finals (3P, 10 m) | APPLIES | PASS | PENDING | PENDING |
| Sighter shots | APPLIES | PASS | PENDING | PENDING |
| Counted shots | APPLIES | PASS | PENDING | PENDING |
| Reconnect | APPLIES | PASS | PARTIAL | PENDING |
| Recovery / restart | APPLIES | PASS | PENDING | PENDING |

**Shared fix = APPLIES everywhere** because the changed code is reached by every
one of them:

- `TachusWidget::collectData` / `checkForNewShots` — the single poll. Every
  discipline's shots arrive here; none has its own reader.
- `ta::target::AcquisitionSequencer` — the counter-reset and reconnect
  decisions. No discipline supplies its own.
- `getXCord` / `getYCord` / `getShootCount` — every score, marker and journalled
  coordinate in the product is built from these three.
- `getXMPIForShoot` / `getYMPIForShoot` — the per-shot columns of the match
  report, for every discipline that prints one.
- `MainWindow::modbusReadRegistry` / `modbusWriteSingleRegister` — the only two
  functions that start a libmodbus transaction.
- `CenterPane.qml::coordinatesUsable` — the refusal sits on the shared scoring
  entry points, not on a discipline branch.

**Automated = PASS** means the reliability and QML harnesses exercise the shared
decisions and the shared guards: 2 438 and 153 checks, 0 failures, including the
reconstructed Tablet-02 sequence, 500 series boundaries and seven reset
latencies. It does **not** mean each discipline was run end to end.

**Emulator = PARTIAL for Reconnect** and PENDING elsewhere. The application has
been shown to connect to the emulator over Modbus TCP and hold one connection,
with the serial selector correctly standing down; the acquisition poll only runs
once a match is open, which needs an operator at the keyboard. Scenarios F and G
exist for exactly that step.

**Physical = PENDING everywhere.** Nothing in this table has been fired at.
