# Tech Aim — SETA Evaluation

**Product:** Tech Aim 0.9.0-RC3 — SETA Evaluation
**Publisher:** JAC SHOOTING SOLUTIONS (PTY) LTD
**Purpose:** Evaluation of Tech Aim Single Target functionality.
**Status:** Evaluation Build — **Not for Official Competition Results.**

## Installing

Run `TechAim-0.9.0-RC3-SETA-Evaluation-Setup.exe` as administrator and accept
the default location. Verify the file first against `SHA256.txt`:

    Get-FileHash .\TechAim-0.9.0-RC3-SETA-Evaluation-Setup.exe -Algorithm SHA256

Operator manuals and the evaluation checklist are installed in the `docs`
folder inside the installation directory.

## What we are asking you to test

Mark each **PASS**, **FAIL** or **NOT TESTED**, and add a note for anything
that fails.

| # | Item | Result | Notes |
|---|---|---|---|
| 1 | Installation and first launch | | |
| 2 | Real target detection (USB device, COM port) | | |
| 3 | 10 m Air Rifle | | |
| 4 | 10 m Air Pistol | | |
| 5 | 50 m Rifle | | |
| 6 | 50 m Rifle 3 Positions | | |
| 7 | Sighters | | |
| 8 | Match / counted shots | | |
| 9 | Paper feed | | |
| 10 | Disconnect warning | | |
| 11 | Reconnect | | |
| 12 | COM re-enumeration after replug | | |
| 13 | First shot after reconnect | | |
| 14 | Restart and resume | | |
| 15 | First shot after recovery | | |
| 16 | Reports | | |
| 17 | Call & Diagnose | | |
| 18 | English / Deutsch | | |
| 19 | Support bundle generation | | |

**Tester:** ______   **Date:** ______   **Windows version:** ______
**Target hardware / firmware:** ______

## If something fails

Run `Make-SupportBundle.ps1` from the installation folder and return the ZIP it
produces, together with your notes. It collects the application's own logs, a
sanitized configuration and the build identity. Please review the archive
before sending it.

## Support

support@techaim.co.za
