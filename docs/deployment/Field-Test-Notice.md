# Tech Aim — internal field-test notice

## THIS IS FIELD-TEST SOFTWARE

**Tech Aim 0.9.0-RC2a — Internal Field Test — Diagnostic**

Please read this before using it at a range.

---

### What it is for

Controlled internal testing on known hardware, by people who know it is under
test and who can report what happens.

### What it must not be used for

- **Official competition results.** Do not use any score, ranking or report from
  this build to decide a competition outcome.
- **Selection, classification or qualification decisions** about an athlete.
- **Public or third-party demonstration** as a finished product.

Training use is fine. Recording a real result from it is not.

---

### What has actually been tested

| | |
|---|---|
| **Physically tested** | One 10 m Air Rifle Training workflow, on one machine, with one target. |
| **Automatically tested** | 2293 reliability + 568 training + 235 3P finals + 143 10 m finals checks, plus governance and packaging audits — all passing. |
| **Not physically tested** | Every other discipline. 10 m Air Pistol, 50 m Rifle Prone, 50 m Rifle 3 Positions, all finals and all other Training Lab programmes have **not** been physically qualified. |

Automated tests passing is not the same as hardware working. RC2 proved that
exactly: every automatic COM-detection test passed while the physical detection
failed, because no test asserted the *order* in which the application called
the selector.

---

### Open items in this build

| Item | Status |
|---|---|
| **SERIAL-AUTO-001** — automatic COM detection | **Fixed in code, awaiting physical re-test.** Not closed. |
| **Shot display latency** | Under investigation. RC2a adds timestamps to measure it — it does not change it. |
| **Auto-zoom** | **Unverified.** It may have been suppressed correctly by the Training visibility mode. |
| **Clean-machine test** | **Blocked** — needs a second machine or VM. |
| **Code signing** | **Not configured.** Windows will warn about the download. |
| **Crash dumps** | **Not captured.** If it crashes, collect a support bundle straight away. |

Full list: [0.9.0-known-limitations.md](../release/0.9.0-known-limitations.md).

---

### Diagnostic mode

This build is a **diagnostic** build. Its release channel reads *Internal Field
Test — Diagnostic* so a support bundle from it can never be confused with RC1 or
RC2.

For the physical retest only, `developer_mode=1` is set so the correlated
shot-pipeline timestamps are recorded. **Set it back to `0` afterwards.** Any
future deployment build must ship `developer_mode=0`.

---

### If something goes wrong

1. Stop.
2. Note what you saw and what you were doing.
3. Collect a support bundle **promptly** — logs live in the Windows temporary
   folder and Windows can clear them:

   ```
   powershell -File Make-SupportBundle.ps1 -Diagnostic
   ```

4. Send the bundle with your notes.

Stop immediately and report if the paper moves with no shot fired, a shot
appears that nobody fired, the shot count is wrong, or the application connects
to a Bluetooth port.

---

### Your data is safe across versions

Sessions, journals, settings and reports live in
`%LOCALAPPDATA%\TechAim\TechAim`, outside the program folder. Upgrading, rolling
back or deleting the program folder does not touch them. This is tested — 35
checks, 0 failures.

To go back to RC2, just run it from its own folder. Nothing needs uninstalling.
