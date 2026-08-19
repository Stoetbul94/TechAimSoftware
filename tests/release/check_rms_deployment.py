# -*- coding: utf-8 -*-
"""RMS-DEPLOY-001 - a deployed Tech Aim RMS folder is self-contained and clean.

The claim this refuses is "it launched on my machine". A developer machine has
Qt on PATH, so a folder that silently borrows a DLL from C:\\Qt launches there
and fails on a range laptop. So this walks the PE import table of every binary
in the folder and requires every non-system import to be resolvable INSIDE the
folder - the same question Windows asks, asked before the customer does.

It also refuses a folder carrying development material: source, tests, git
metadata, caches, logs, or session data from the machine that built it.

Run: python tests/release/check_rms_deployment.py <deployment-dir>
"""
import hashlib
import json
import os
import struct
import sys

checks = 0
failures = 0


def check(ok, label, detail=''):
    global checks, failures
    checks += 1
    if ok:
        print('PASS  ' + label)
    else:
        failures += 1
        print('FAIL  ' + label + ('  ' + detail if detail else ''))


# ── PE import reading ──────────────────────────────────────────────────────

def pe_imports(path):
    """Names of the DLLs a PE file imports. Empty list if it is not a PE."""
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except OSError:
        return []
    if len(data) < 0x40 or data[:2] != b'MZ':
        return []
    e_lfanew = struct.unpack('<I', data[0x3C:0x40])[0]
    if e_lfanew + 4 > len(data) or data[e_lfanew:e_lfanew + 4] != b'PE\0\0':
        return []
    coff = e_lfanew + 4
    num_sections = struct.unpack('<H', data[coff + 2:coff + 4])[0]
    opt_size = struct.unpack('<H', data[coff + 16:coff + 18])[0]
    opt = coff + 20
    magic = struct.unpack('<H', data[opt:opt + 2])[0]
    dd = opt + (112 if magic == 0x20B else 96)
    imp_rva, imp_size = struct.unpack('<II', data[dd + 8:dd + 16])
    if imp_rva == 0:
        return []

    sections = []
    sec = opt + opt_size
    for i in range(num_sections):
        s = sec + i * 40
        va = struct.unpack('<I', data[s + 12:s + 16])[0]
        vsize = struct.unpack('<I', data[s + 8:s + 12])[0]
        raw_size, raw_ptr = struct.unpack('<II', data[s + 16:s + 24])
        sections.append((va, max(vsize, raw_size), raw_ptr))

    def off(rva):
        for va, size, ptr in sections:
            if va <= rva < va + size:
                return ptr + (rva - va)
        return None

    names, p = [], off(imp_rva)
    if p is None:
        return []
    while p + 20 <= len(data):
        entry = data[p:p + 20]
        if entry == b'\0' * 20:
            break
        name_rva = struct.unpack('<I', entry[12:16])[0]
        n = off(name_rva)
        if n is not None:
            end = data.find(b'\0', n)
            names.append(data[n:end].decode('latin-1').lower())
        p += 20
    return names


# Windows ships these; a deployment must NOT carry them.
SYSTEM_PREFIXES = (
    'api-ms-win', 'ext-ms-win', 'kernel32', 'user32', 'gdi32', 'advapi32',
    'shell32', 'ole32', 'oleaut32', 'msvcrt', 'ws2_32', 'wsock32', 'comdlg32',
    'winmm', 'imm32', 'version', 'netapi32', 'userenv', 'uxtheme', 'dwmapi',
    'shlwapi', 'crypt32', 'secur32', 'bcrypt', 'ncrypt', 'iphlpapi', 'dnsapi',
    'setupapi', 'cfgmgr32', 'winspool', 'comctl32', 'opengl32', 'glu32',
    'd3d9', 'd3d11', 'd3d12', 'dxgi', 'dxva2', 'evr', 'mf', 'mfplat',
    'mfreadwrite', 'mfuuid', 'ntdll', 'powrprof', 'propsys', 'psapi', 'rpcrt4',
    'authz', 'dbghelp', 'wtsapi32', 'winhttp', 'mpr', 'normaliz', 'ktmw32',
    'msimg32', 'oleacc', 'runtimeobject', 'combase', 'd2d1', 'dwrite',
    'windowscodecs', 'usp10', 'wintrust', 'avicap32', 'msdmo', 'strmiids',
    'quartz', 'ksuser', 'mmdevapi', 'audioses', 'mscoree', 'sechost',
    'imagehlp',   # ships in System32; the software OpenGL fallback imports it
)

# A curated list can only ever be as complete as the last thing that broke, so
# Windows itself is also consulted: a name that resolves in System32 is an OS
# DLL. Never for these families though - a Qt or toolchain DLL found outside
# the folder is exactly the defect this test exists to catch, and a stray copy
# in System32 must not excuse it.
NEVER_FROM_SYSTEM = ('qt6', 'qt5', 'libgcc', 'libstdc', 'libwinpthread',
                     'libssl', 'libcrypto', 'd3dcompiler', 'opengl32sw')
SYSTEM32 = os.path.join(os.environ.get('SystemRoot', r'C:\Windows'), 'System32')


def is_system(dll):
    d = dll.lower()
    if any(d.startswith(p) for p in NEVER_FROM_SYSTEM):
        return False
    if any(d.startswith(p) for p in SYSTEM_PREFIXES):
        return True
    return os.path.isfile(os.path.join(SYSTEM32, d))


LEAK_SUFFIXES = ('.cpp', '.h', '.hpp', '.pro', '.pri', '.py', '.pyc', '.md',
                 '.ts', '.tch', '.jsonl', '.log', '.o', '.obj', '.a', '.lib')
LEAK_DIRS = ('.git', '.claude', '__pycache__', 'tests', 'docs', 'src',
             'ModReader', 'scripts', 'rms')
# Deliberately shipped: the launchers and the two handoff documents. They are
# .cmd and .txt, so they do not collide with the source suffixes above; this
# list exists so the intent is written down rather than implied.
EXPECTED_HANDOFF = ('Launch-TechAimRMS-Demo.cmd', 'Launch-TechAimRMS-Live.cmd',
                    'Reset-Demo.cmd', 'README-FIELD-TEST.txt',
                    'FIELD-TEST-CHECKLIST.txt')
# Qt's own QML modules ship .qml/.qmltypes/qmldir - that is Qt runtime, not our
# source. Our application QML stays inside the executable's resources.
ALLOWED_QML_ROOTS = ('qml',)


def main():
    if len(sys.argv) < 2:
        print('usage: check_rms_deployment.py <deployment-dir>')
        return 2
    root = os.path.abspath(sys.argv[1])
    print('=== Tech Aim RMS deployment checks (RMS-DEPLOY-001) ===\n')
    print('  folder: %s\n' % root)

    check(os.path.isdir(root), 'the deployment folder exists', root)
    if not os.path.isdir(root):
        return finish()

    exe = os.path.join(root, 'TechAimRMS.exe')
    check(os.path.isfile(exe), 'TechAimRMS.exe is present')
    if not os.path.isfile(exe):
        return finish()

    # ── every binary's imports resolve inside the folder ──────────────────
    present, binaries = {}, []
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            p = os.path.join(dirpath, f)
            if f.lower().endswith(('.dll', '.exe')):
                present[f.lower()] = p
                binaries.append(p)
    missing, from_os = {}, set()
    for b in binaries:
        for dep in pe_imports(b):
            if dep in present:
                continue
            if is_system(dep):
                from_os.add(dep)
                continue
            missing.setdefault(dep, []).append(os.path.relpath(b, root))
    # Printed, not hidden: every import the folder does NOT carry is named here,
    # so "Windows provides it" is a visible claim and not a silent allowance.
    print('  satisfied by Windows itself: %s\n' % ', '.join(sorted(from_os)))
    check(not missing,
          'every imported DLL resolves INSIDE the folder - nothing is borrowed '
          'from a Qt or MinGW installation',
          '; '.join('%s (needed by %s)' % (d, v[0]) for d, v in sorted(missing.items())[:6]))
    check(len(binaries) > 20, 'the folder actually contains a runtime',
          str(len(binaries)))

    # ── the pieces Qt cannot start without ────────────────────────────────
    check(os.path.isfile(os.path.join(root, 'platforms', 'qwindows.dll')),
          'the Windows platform plugin is deployed - without it Qt aborts with '
          '"could not find or load the Qt platform plugin"')
    # RMS imports QtQuick and QtQuick.Window and nothing else. Anything more
    # in this list would be asserting the presence of weight the product does
    # not use.
    for mod in ('QtQuick', 'QtQml'):
        check(os.path.isdir(os.path.join(root, 'qml', *mod.split('/'))),
              'QML module deployed: %s' % mod)
    for plug in ('imageformats', 'tls'):
        check(os.path.isdir(os.path.join(root, plug)),
              'plugin directory deployed: %s' % plug)
    for rt in ('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll'):
        check(os.path.isfile(os.path.join(root, rt)),
              'MinGW runtime deployed: %s' % rt)

    # ── SETA identity, read out of the shipped binary ─────────────────────
    raw = open(exe, 'rb').read()

    def version_value(key):
        """The value of a VS_VERSIONINFO string key, read from the resource.

        Both product names exist as C++ literals in the binary (ProductIdentity
        compiles every flavour and returns one), so searching the whole file
        proves nothing. What Explorer shows is the version resource, and that
        is what is read here: the key, then the next UTF-16 string after it.
        """
        k = key.encode('utf-16-le')
        i = raw.find(k)
        if i < 0:
            return None
        j = i + len(k)
        while j + 1 < len(raw) and raw[j:j + 2] == bytes(2):
            j += 2
        end = raw.find(bytes(2), j)
        if end < 0:
            return None
        if (end - j) % 2:
            end += 1
        return raw[j:end].decode('utf-16-le', 'ignore')

    product = version_value('ProductName')
    check(product is not None and 'Range Management' in (product or ''),
          'the deployed executable identifies as Tech Aim RMS in its VERSION '
          'RESOURCE - which is what Explorer and Properties show', str(product))
    check(product is None or 'SETA' not in product,
          'and it is not a SETA build - RMS ships through its own path',
          str(product))
    check('0.9.0-M4.5-FIELDTEST'.encode('utf-16-le') in raw
          or b'0.9.0-M4.5-FIELDTEST' in raw,
          'the field-test version string is baked into the binary')

    # ── no development material ships ─────────────────────────────────────
    leaks = []
    for dirpath, dirs, files in os.walk(root):
        rel = os.path.relpath(dirpath, root).replace('\\', '/')
        top = rel.split('/')[0]
        for d in list(dirs):
            if d.lower() in [x.lower() for x in LEAK_DIRS]:
                leaks.append(rel + '/' + d + '/')
        for f in files:
            if top in ALLOWED_QML_ROOTS:
                continue          # Qt's own QML modules
            if f.lower().endswith(LEAK_SUFFIXES):
                leaks.append((rel + '/' + f).lstrip('./'))
    check(not leaks,
          'no source, test, documentation or development file is deployed',
          ', '.join(sorted(leaks)[:8]))

    # ── no state from the machine that built it ───────────────────────────
    state = [f for f in os.listdir(root)
             if f.lower() in ('range.json', 'athletes.json', 'plans.json',
                              'config.ini')]
    check(not state,
          'no range, athlete or plan data from the build machine is deployed - '
          'the first run starts clean and a demo run creates its own profile',
          ', '.join(sorted(state)[:6]))

    for f in EXPECTED_HANDOFF:
        check(os.path.isfile(os.path.join(root, f)),
              'field-test handoff file present: %s' % f)

    # ── the manifest describes what is actually here ──────────────────────
    man_path = os.path.join(root, 'deployment-manifest.json')
    check(os.path.isfile(man_path), 'a deployment manifest is present')
    if os.path.isfile(man_path):
        # PowerShell 5.1 writes UTF-8 with a BOM.
        man = json.load(open(man_path, encoding='utf-8-sig'))
        sha = hashlib.sha256(raw).hexdigest()
        check(man.get('executableSha256') == sha,
              'the manifest names the executable that is actually deployed',
              (man.get('executableSha256') or '')[:16] + ' vs ' + sha[:16])
        check(man.get('product') and man.get('qtVersion') and man.get('gitCommit'),
              'the manifest carries product, Qt version and commit')
        blob = json.dumps(man)
        check('C:\\\\Users' not in blob and 'C:/Users' not in blob,
              'and no build-machine path is written into a customer-facing file')

    return finish()


def finish():
    print('\n=== %d checks, %d failures ===' % (checks, failures))
    sys.stdout.flush()
    return 0 if failures == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
