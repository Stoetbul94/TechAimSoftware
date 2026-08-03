# Tech Aim — operator guide

For the person running the software at the range. It uses no internal
terminology. Anything technical is in
[Diagnostics-Appendix.md](Diagnostics-Appendix.md).

> **This is field-test software.** Do not use it for official competition
> results. See [Known limitations](../release/0.9.0-known-limitations.md).

**Contents**

1. [Installation and extraction](#1-installation-and-extraction)
2. [First startup](#2-first-startup)
3. [Connecting the target](#3-connecting-the-target)
4. [Automatic target detection](#4-automatic-target-detection)
5. [Choosing the port yourself](#5-choosing-the-port-yourself)
6. [Paper feed settings](#6-paper-feed-settings)
7. [Exporting reports](#7-exporting-reports)
8. [Logs and support bundles](#8-logs-and-support-bundles)
9. [Upgrading](#9-upgrading)
10. [Going back to a previous version](#10-going-back-to-a-previous-version)
11. [Removing the software](#11-removing-the-software)

Sections 12–14 are separate documents:
[Known limitations](../release/0.9.0-known-limitations.md) ·
[Field-test notice](Field-Test-Notice.md) ·
[Physical qualification checklist](Physical-Qualification-Checklist.md)

---

## 1. Installation and extraction

There is **no installer**. The software runs from a folder.

1. Copy the ZIP to the computer that will run the range.
2. **Check it arrived intact.** Open PowerShell in the folder holding the ZIP:

   ```
   Get-FileHash .\TechAim-0.9.0-RC2a-Diagnostic-Windows-x64.zip -Algorithm SHA256
   ```

   Compare the result with the checksum supplied with the download. If they
   differ, stop — do not extract it, and report the mismatch.
3. Right-click the ZIP → **Extract All**, and extract to a **short path** such
   as `C:\TechAim\RC2a`. Avoid Desktop, Downloads and OneDrive folders: they
   can be synced or cleaned up underneath you.
4. Keep each version in its own folder. Never extract a new version on top of
   an old one.

Windows may warn that the file came from another computer. That is expected:
this software is **not code-signed**. Choose to keep the file only if the
checksum matched in step 2.

You do **not** need administrator rights, and the software does not ask for
them. If something asks for administrator access, stop and report it.

## 2. First startup

1. Open the extracted folder.
2. Double-click **TechAim.exe**.

   Always start it from inside its own folder. If you make a shortcut later,
   set the shortcut's **Start in** field to the folder containing
   `TechAim.exe` — otherwise the software will not find its settings.
3. Check the version shown in the application matches what you were sent.

On first start the software creates its data folder. Nothing is written into
Windows system areas.

**Your data lives here, not in the program folder:**

```
C:\Users\<your name>\AppData\Local\TechAim\TechAim
```

That separation is deliberate: you can delete, replace or roll back the program
folder and your sessions stay where they are.

## 3. Connecting the target

1. Connect the target's USB cable **before** starting the software.
2. Wait a few seconds for Windows to finish recognising it.
3. Start the software.

Connecting the target first lets automatic detection do its job. If you connect
it afterwards, use **Rescan** or pick the port yourself (§5).

## 4. Automatic target detection

The software looks at the serial ports on the computer, works out which one is
the target, and connects to it. You should not have to choose anything.

You will see one of these:

| Message | What it means | What to do |
|---|---|---|
| **SCANNING** | Looking at the available ports. | Wait a moment. |
| **TARGET DETECTED** | It found the target and is connecting. | Nothing. |
| **TARGET CONNECTED** | Connected and communicating. | Start shooting. |
| **MANUAL SELECTION REQUIRED** | Several devices could be the target. | Choose the port yourself — §5. |
| **TARGET NOT DETECTED** | No target found. | Check the cable, then **Rescan**. If it still fails, §5. |

Two things worth knowing:

- **Bluetooth ports are ignored on purpose.** Many laptops present Bluetooth as
  serial ports. The software rules them out from their description and never
  opens them, so it cannot get stuck on one.
- **It remembers your target.** After a successful connection, the software
  remembers *that adapter* — not just the port number. If Windows gives it a
  different port number next time, it still finds it. The memory is stored for
  your Windows user on that computer, so it is not carried around in the
  program folder or shared with anyone else.

If nothing is found, the software does **not** guess. It tells you, and leaves
manual selection available.

## 5. Choosing the port yourself

Manual selection always works and always overrides automatic detection.

1. Open the connection controls.
2. Choose the COM port for the target.
3. Connect.

To find the right port: open Windows **Device Manager**, expand **Ports (COM &
LPT)**, unplug the target, and see which entry disappears. That is the one.

Ignore anything described as *Bluetooth*, *standard serial over Bluetooth link*
or a modem.

## 6. Paper feed settings

The paper advances automatically after each shot the target accepts. You do not
press anything.

Two durations are set in `config.ini`, in seconds:

| Setting | Applies to |
|---|---|
| `motor_movement_time` | Counted match shots |
| `motor_movement_time_sighter` | Sighter shots |

Guidance:

- These are **seconds of motor movement**, not a distance. Adjust in small
  steps and watch the paper.
- Setting a value to `0` turns automatic feeding **off** for that shot type.
- Values over 30 seconds are refused and treated as 30, so a typing mistake
  cannot run the motor indefinitely.
- Demo and practice clicks never move the motor. Only a real shot accepted from
  the target does.
- The paper never moves while the software is starting up.

Manual feed remains available and is unchanged.

## 7. Exporting reports

Reports are exported as PDF. Unless you choose somewhere else, they are saved
to your **Documents** folder — not to the program folder, and not to the data
folder.

That means your exported PDFs survive an upgrade, a rollback and removing the
software. Keep them somewhere backed up if they matter.

## 8. Logs and support bundles

If something goes wrong, send a **support bundle**. It is one command and it
gathers what a diagnosis needs.

Open PowerShell in the software's folder and run:

```
powershell -File Make-SupportBundle.ps1
```

The bundle is written to your Desktop as a ZIP. It contains the version
details, recent logs, a summary of target communication, a settings file with
anything password-like removed, and a count of how many sessions you have.

**It does not include any athlete's session data unless you ask for it.** To
include one specific session:

```
powershell -File Make-SupportBundle.ps1 -SessionId 3f9c1a22
```

Please open the ZIP and look at it before sending it on.

**Collect the bundle soon after the problem.** Logs are written to the Windows
temporary folder, which Windows Disk Cleanup and Storage Sense can empty.

## 9. Upgrading

1. Close the software.
2. Extract the new version into a **new** folder — do not overwrite the old one.
3. Start the new version.
4. Check the version shown on screen.

Your sessions, settings and remembered target survive the upgrade. This is
tested: the upgrade drill checks that nothing in the data folder is removed,
that archived sessions are not reduced, and that a finished session is not
reopened as if it were still running.

Keep the previous folder until you are satisfied with the new one. That is your
rollback.

## 10. Going back to a previous version

1. Close the software.
2. Start `TechAim.exe` from the previous version's folder.

That is all. Your data is not deleted, not downgraded and not converted — going
back does not lose sessions, journals or reports.

If you no longer have the previous folder, use the supplied rollback package
and check its checksum first, exactly as in §1.

## 11. Removing the software

**To remove the program:** close it and delete its folder. There is nothing
else to uninstall — no registry cleanup, no service, no Windows entry.

**Your data is not removed with it.** Deliberately. If you also want the data
gone, delete these yourself, after saving anything you need:

| What | Where |
|---|---|
| Sessions, journals, logs, settings | `C:\Users\<you>\AppData\Local\TechAim\TechAim` |
| Remembered target and preferences | Registry: `HKEY_CURRENT_USER\Software\TechAim\TechAim` |
| Exported PDF reports | Your **Documents** folder — these are yours, check before deleting |

On a machine that ran earlier versions you may also see
`TechAim Electronic Target`, `TechAimLaneSimulator` or `TechAimRangeManager`
beside the data folder. Those are from earlier or different applications and
are not created by this version.
