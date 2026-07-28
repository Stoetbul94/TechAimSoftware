# Target Connection — Implementation Audit and Physical Test Script

Document version 1.0 ({{DOCUMENT_VERSION}}) · Application baseline commit `{{APPLICATION_BASELINE_COMMIT}}` · Documentation source commit `{{DOCUMENTATION_SOURCE_COMMIT}}`
Built {{DOCUMENT_BUILD_TIMESTAMP}}

Closes review finding **F-07** (target connection setup undocumented) as far as
the source permits. **Everything requiring real hardware is marked
PHYSICAL TARGET DEPENDENT and has not been performed.**

**No SETA protocol details are guessed.** Where the vendor protocol is not
evident from this repository, that is stated rather than inferred.

---

## 1. Physical interface (from source)

| Aspect | Finding | Evidence |
|---|---|---|
| Protocol family | **Modbus** | vendored QModMaster fork under `ModReader/` |
| Transports | **Modbus RTU (serial)** and **Modbus TCP** | `ModReader/src/modbus-rtu.*`, `modbus-tcp.*` |
| Bridge to the app | `TachusWidget` → QML as `MODREADER` | `ModReader/forms/tachuswidget.*` |
| Settings store | `ModbusCommSettings` | `ModReader/src/modbuscommsettings.h` |
| Settings file | `qModMaster.ini` in the Windows temp folder | `main.cpp` |

## 2. Settings fields (from `ModbusCommSettings`)

| Field | Accessors |
|---|---|
| Serial port | `serialPort()` / `setSerialPort()`, `serialPortName()` |
| Baud rate | `baud()` / `setBaud()` |
| Slave ID | `slaveID()` / `setSlaveID()` |

Additional parity/data-bit/stop-bit/timeout settings exist in the vendored
QModMaster settings dialogs, but **those dialogs are unreachable** — the
QModMaster main window is never shown. Their effective values therefore come
from the stored settings file, not from operator input.

**Operator-facing connection UI** is `ModConnectorDialog.qml`, whose labels are
`Connect`, `Cancel` and the validation message `Please Provide a Port Number`.

> **Gap:** the reachable dialog collects a **port number** only. Baud rate,
> slave ID and serial framing are **not** operator-editable through the
> reachable Tech Aim UI in this build. Recorded as a finding, not documented as
> a procedure.

## 3. Connection status

| Mechanism | Detail |
|---|---|
| Signal | `TachusWidget::masterConnectionChanged(bool isConnected)` |
| QML handler | `LoginPage.qml:139` `onMasterConnectionChanged` |
| Polled state | `LoginPage.qml:79` `MODREADER.isModBusConnected()` |
| QML property | `LoginPage.qml:30` `mod_connected` |

The login page reacts to connection changes and can raise a connector popup
when the connection is absent.

## 4. Live source gating

Independently of transport, the operating-mode gate decides whether a shot is
accepted:

- **Live** accepts physical-target input and **rejects simulated** input.
- **Demo** accepts generated input and **rejects physical-target** input.

**VERIFIED FROM CODE AND TESTS** — covered by the mode/source tests. This is
why "no shot received" so often turns out to be the wrong operating mode.

## 5. Expected hardware

Recorded from the project's working notes, **not** from vendor documentation:

- SETA electronic target electronics
- Modbus RTU over a serial/USB adapter — previously used at **COM7, 19200 baud**
- `config.ini` needs `app_mode=Live` and `is_single_decimal=1`
  (coordinates in tenths of a millimetre)

**PHYSICAL TARGET DEPENDENT** — none of this has been exercised in
this phase.

## 6. What is NOT documented, and will not be guessed

- The SETA register map (which registers carry coordinates, score, shot index)
- Coordinate units and axis orientation as the hardware actually reports them
- Vendor reconnect/retry semantics
- Any target-side configuration performed outside this application
- Whether a power-cycle requires a specific sequence

These need vendor documentation or physical measurement.

---

## 7. Physical test script (for the operator / SETA engineer)

Run in order. Record the result of every step. **Do not skip a failing step.**

### A — Preparation
1. Target powered, connected, seated cable.
2. Note the COM port in Windows Device Manager: `COM____`
3. `config.ini`: `app_mode=Live`, `is_single_decimal=1`.
4. Launch `TechAim.exe`; confirm the startup log reports
   `Operating mode: Live`.

### B — Connection
5. Open the connector dialog; enter the port number; select **Connect**.
6. **Expected:** the connection indicator shows connected.
   Result: ☐ pass ☐ fail — note: ____________________
7. Close and relaunch. Does it reconnect without re-entering the port?
   Result: ☐ yes ☐ no

### C — Shot acquisition (the critical test)
8. Fire **one deliberate shot high-left**, clearly off centre.
9. **Expected:** it appears **high and left** on the target face.
   Result: ☐ correct ☐ mirrored horizontally ☐ mirrored vertically ☐ rotated
10. Repeat low-right. Same check.
11. **Axis orientation must be confirmed by this test** — it is the single
    most important hardware check, and a mirrored axis invalidates every
    coordinate-derived metric (MPI, group, spread, group pattern).

### D — Scoring accuracy
12. Fire a shot as close to centre as possible; record physical measurement
    against displayed score and coordinates.
13. Fire near a known ring boundary; record both.
14. **50 m Rifle only:** the 10-ring radius awaits confirmation. Record
    measured vs displayed values for calibration.

### E — Source gating
15. Still in Live, confirm no simulated input can be injected.
16. Switch to **Demo**, restart, and confirm physical shots are **rejected**.
    Result: ☐ rejected as expected ☐ accepted (DEFECT — report immediately)

### F — Interruption behaviour
17. Mid-session, disconnect the cable. Record what the indicator does and how
    long it takes.
18. Reconnect. Does operation resume without restarting?
19. Mid-session, power-cycle the target. Record the recovery behaviour.
20. Mid-session, kill the application (Task Manager). Relaunch and confirm the
    **recovery** dialog offers the session with the correct shot count.

### G — Endurance
21. Complete a full match on live hardware. Confirm **no dropped, duplicated
    or late shots**, and that the count matches the shots actually fired.

### Recording
For each failure record: step number, operating mode, discipline, expected vs
observed, time, and the diagnostic log from the Windows temp folder.

---

## 8. Manual updates made from this audit

Only **verified** information has gone into the English manuals: the transport
family, the connection indicator mechanism, and the source gate. The
port-number-only limitation and the missing register-map detail are recorded
**here** and are deliberately **not** presented in the manual as a working
setup procedure.

**F-07 remains open** until section 7 has been executed on real hardware.
