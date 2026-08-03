# Tech Aim — AppData structure and upgrade safety

**Result: 35 checks, 0 failures.** Data root reconciled at exactly **142 files
before and after**. Run on 2026-08-03 with the accepted RC2 and RC2a packages.

No storage format was changed. This documents and tests what exists.

---

## 1. The structure

Root: `QStandardPaths::AppLocalDataLocation`. Organisation and application are
both `TechAim`, so on Windows:

```
C:\Users\<user>\AppData\Local\TechAim\TechAim
```

`StoragePaths` (`src/reliability/storage/StoragePaths.h`) is the sole owner of
these paths — never the executable directory, never the process working
directory.

| Directory | Holds | Survives upgrade | Survives removal |
|---|---|---|---|
| `Settings` | Application settings written by the storage layer | Yes | Yes |
| `Sessions\Current` | Live session journals (append-only JSONL, hash-chained) | Yes | Yes |
| `Sessions\Archive` | Completed sessions, month-partitioned `<yyyy>\<MM>\` | Yes | Yes |
| `Sessions\Corrupt` | Journals that failed validation — **preserved, never deleted** | Yes | Yes |
| `Backups` | Journal backups | Yes | Yes |
| `Reports` | Report data written by the storage layer | Yes | Yes |
| `Exports` | Export staging | Yes | Yes |
| `Logs` | **Empty — see LOG-001** | Yes | Yes |
| `SupportBundles` | Crash/support artefacts — **empty, nothing writes here** | Yes | Yes |
| `DerivedIndexes` | Rebuildable indexes | Yes | Yes |
| `cache` | Qt QML and pipeline caches — rebuildable | Yes | Yes |

Two things live **outside** that tree:

| What | Where | Why it matters |
|---|---|---|
| **Remembered target fingerprint** | `HKCU\Software\TechAim\TechAim`, group `TargetDevice` | Per-user, per-machine. Never packaged; never copied between machines. |
| **Exported PDF reports** | The user's **Documents** folder | Operator-owned. Not deleted by removing the software. |

**Recovery snapshots** are not a separate directory — recovery replays the
append-only journal in `Sessions\Current` and seeds from the hash chain.
`Sessions\Corrupt` is where a journal that fails validation is preserved so it
can be examined rather than lost.

---

## 2. Why upgrade safety is structural, not conventional

The data root is derived from the Windows shell, not from the program folder.
Replacing, moving or deleting the program folder therefore **cannot reach it**.

Two rules keep it that way, and both are enforced:

- No package may contain a `.jsonl`. `Verify-Deployment.ps1` fails if one
  appears; `check_deployment_package.py` does the same.
- The drill asserts the data root is not inside the program folder.

---

## 3. The drill

```bash
powershell -File tools\deployment\Test-AppDataUpgrade.ps1 -OldPackage <RC2 folder> -NewPackage <RC2a folder>
```

### Isolation — what is and is not possible

Qt resolves `AppLocalDataLocation` through the Windows shell
(`SHGetKnownFolderPath` / `FOLDERID_LocalAppData`), **not** through the
`LOCALAPPDATA` environment variable. Overriding that variable for a child
process was attempted and **verified to have no effect** — the application still
used the real root and the sandbox stayed empty.

Genuinely redirecting AppData needs a second Windows user account, a VM, or a
change to the User Shell Folders registry value. The first two are unavailable
here; the third is a system settings change and is out of scope.

So the drill runs against the real per-user root and is made **safe by
construction** instead:

- a full before-inventory of every file under the data root, with sizes;
- it creates only files named `UPGRADE-DRILL-MARKER*`;
- it deletes exactly those markers and nothing else;
- an after-inventory that **fails** if any pre-existing file was removed.

It never deletes a session, journal, report or setting. This satisfies "do not
perform destructive migration testing on real AppData": the drill is not
destructive at all.

### What it covers

| Scenario | Checks |
|---|---|
| **First installation** | Data root exists; all ten storage directories present |
| **Restart** | Archived sessions and settings preserved |
| **Upgrade over older** | Sessions, settings and reports preserved; archived count not reduced; **completed sessions not promoted into `Current`**; per-user settings key survives; new program folder holds no journals |
| **RC2a and rollback separately** | Each package launches from its own folder with no Qt, no MinGW and no repository on `PATH` |
| **Rollback** | Journals and archived sessions not deleted; settings preserved |
| **Removal** | No user data inside the program folder; data root outside it, so folder deletion cannot reach it |
| **Reconciliation** | No pre-existing file removed; no drill marker left behind |

### Result (2026-08-03)

```
files before: 142
=== 35 passed, 0 failed ===
files after:  142
```

Every launch was made with `PATH` reduced to `%WINDIR%\System32;%WINDIR%` — no
Qt, no MinGW, no repository — and both packages launched and stayed running.

---

## 4. What this does *not* prove

- **Not a clean-machine test.** The development machine has had Qt installed;
  scrubbing `PATH` is not the same as a machine that never had a compiler. See
  [Clean-Machine-Test.md](Clean-Machine-Test.md) — that test is **BLOCKED**.
- **No hardware.** No target was connected and no shot was fired. Nothing here
  qualifies any discipline.
- **Not an upgrade across a format change.** RC2 → RC2a share a storage format.
  A future release that changes the format needs a migration test of its own.
- **Session count only.** The drill counts and preserves files; it does not
  replay a journal to prove semantic integrity. The reliability harness
  (2293 checks) covers replay separately.
