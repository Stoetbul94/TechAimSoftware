# Tech Aim — deployment configuration reference

Every setting a deployment may carry, its default, and what must never be
packaged.

`config.ini` sits **beside `TechAim.exe`** and is read by relative path. See
CFG-001 in [Deployment-Audit.md](Deployment-Audit.md): the working directory
must be the install folder.

Written UTF-8 **without BOM**. A BOM makes the first section header unreadable
to `QSettings`, so both the deployment audit and `Verify-Deployment.ps1` check
for it.

---

## The production template

This is the configuration a **final deployment** ships. It is prepared here and
deliberately **not** packaged yet — RC3 will carry it after the physical test
passes.

```ini
[shot_count_and_timer]
timer=yes

[App_Settings]
app_mode=Live
developer_mode=0
is_single_decimal=1
motor_movement_time=1
motor_movement_time_sighter=1
```

## Settings

### `[App_Settings]`

| Key | Values | Default | Notes |
|---|---|---|---|
| `app_mode` | `Live`, `Demo` | **`Live`** | `Live` reads real target hardware. `Demo` drives the interface from simulated input and **must never** be packaged for deployment. |
| `developer_mode` | `0`, `1` | **`0`** | `1` enables diagnostic logging including the shot-pipeline stamps. Production is `0`. |
| `is_single_decimal` | `0`, `1` | `1` | Coordinate scale from the target: `1` = tenths of a millimetre. Must match the hardware. |
| `motor_movement_time` | seconds | `1` | Automatic paper feed for **counted** shots. `0` disables. Values above 30 are clamped to 30. |
| `motor_movement_time_sighter` | seconds | `1` | Automatic paper feed for **sighter** shots, passed per command. |

### `[shot_count_and_timer]`

| Key | Values | Default | Notes |
|---|---|---|---|
| `timer` | `yes`, `no` | `yes` | Countdown clock. On by default; the file can turn it off. |

---

## developer_mode by release

| Release | Value | Reason |
|---|---|---|
| RC1, RC2 packaged default | `0` | Field test, near-silent |
| **RC2a packaged default** | **`0`** | The package ships `0` |
| **RC2a diagnostic physical test** | **`1`** | Set by the operator, for that test only. This is the only sanctioned use of `1`. |
| **RC3 and any deployment** | **`0`** | **Required.** `Verify-Deployment.ps1` fails a production package that carries `1`. |

`Verify-Deployment.ps1 -ExpectDeveloperMode <0|1>` makes the expectation
explicit at verification time, so a diagnostic package can never quietly pass a
production check.

---

## What must never be packaged

Each item is enforced by `Verify-Deployment.ps1` and by
`tests/release/check_deployment_package.py`.

| Must not ship | Why | Where it belongs |
|---|---|---|
| **A COM port number** | Machine-specific. Baking one in defeats automatic detection and breaks on every other machine. | Chosen at runtime; the adapter is remembered per-user. |
| **The remembered target identity** | Per-user, per-machine hardware fingerprint. | `HKCU\Software\TechAim\TechAim`, group `TargetDevice` |
| **A developer profile** | `developer_mode=1` in a deployment turns on diagnostic logging nobody asked for. | Set deliberately for a diagnostic test only |
| **A demo profile** | `app_mode=Demo` means scores are simulated. Shipping it risks simulated results being read as real. | Development only |
| **Test athlete names** | Fitzwilliam, Short-Session, "Arnold Bailie", windmap-review fixtures. | Test fixtures in the repository |
| **Session journals / snapshots** | `.jsonl` files are athlete data. | `%LOCALAPPDATA%\TechAim\TechAim\Sessions` |
| **Logs** | May contain session detail and machine identifiers. | `%TEMP%` (LOG-001) and AppData `Logs` |
| **Absolute repository paths** | Leaks the build machine's layout. | — |
| **Personal Windows usernames** | Privacy, not tidiness. A path like `C:\Users\<real name>\…` in a shipped text file exposes a person. | — |
| **Source, tests, build scripts** | Not runtime. | The repository |

The configuration written by the package builder is a **fresh literal**, never
a copy of the developer's `config.ini`, so no local value can leak by accident.
`config.ini` is untracked in the repository for the same reason.

---

## Verifying a configuration

```bash
powershell -File tools\deployment\Verify-Deployment.ps1 -PackageDir C:\TechAim\RC2a -ExpectDeveloperMode 0
```

Checks: `app_mode=Live`, no `Demo`, `developer_mode` matches the stated
expectation, no BOM, and no COM port baked in. Any failure exits non-zero with
the reason.
