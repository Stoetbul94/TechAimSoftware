# -*- coding: utf-8 -*-
"""WIN-ICON-001 — the executable wears its own product's icon.

The claim this file exists to refuse is "the .ico is in the repo, therefore the
build is branded". An icon file proves nothing: what ships is the PE resource
inside the executable. So this reads the RT_ICON resources OUT of the built
binary and compares them, byte for byte, with the images in the .ico the
flavour is supposed to use - and with the images in the OTHER flavour's icon,
which must not appear.

It also checks the two directions of leakage separately, because they are
different mistakes: a Tech Aim build wearing the SETA mark is a branding
error, and a SETA build still wearing the Tech Aim mark is the defect this
work was raised for.

Run: python tests/release/check_windows_icon.py [path-to-exe]
"""
import hashlib
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SETA_ICO = os.path.join(ROOT, 'images', 'logo', 'seta.ico')
TECHAIM_ICO = os.path.join(ROOT, 'images', 'logo', 'techaim.ico')
RC = os.path.join(ROOT, 'TechAim.rc')
PRO = os.path.join(ROOT, 'Seta.pro')
DEFAULT_EXE = os.path.join(ROOT, 'release', 'TechAim.exe')

REQUIRED_SIZES = [16, 24, 32, 48, 64, 128, 256]

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


# ── .ico reading ───────────────────────────────────────────────────────────

def read_ico(path):
    """[(width, height, image-bytes)] for every image in an .ico."""
    with open(path, 'rb') as f:
        data = f.read()
    reserved, kind, count = struct.unpack('<HHH', data[:6])
    assert reserved == 0 and kind == 1, 'not an .ico: ' + path
    out = []
    for i in range(count):
        entry = data[6 + i * 16:22 + i * 16]
        w, h, _, _, _, _, size, offset = struct.unpack('<BBBBHHII', entry)
        out.append((w or 256, h or 256, data[offset:offset + size]))
    return out


# ── PE resource reading ────────────────────────────────────────────────────

RT_ICON = 3


def pe_icon_resources(path):
    """Every RT_ICON payload in a PE file, as raw bytes."""
    with open(path, 'rb') as f:
        data = f.read()
    e_lfanew = struct.unpack('<I', data[0x3C:0x40])[0]
    assert data[e_lfanew:e_lfanew + 4] == b'PE\0\0', 'not a PE file'
    coff = e_lfanew + 4
    num_sections, = struct.unpack('<H', data[coff + 2:coff + 4])
    opt_size, = struct.unpack('<H', data[coff + 16:coff + 18])
    opt = coff + 20
    magic, = struct.unpack('<H', data[opt:opt + 2])
    dd = opt + (112 if magic == 0x20B else 96)          # data directories
    rsrc_rva, rsrc_size = struct.unpack('<II', data[dd + 16:dd + 24])
    sections = []
    sec = opt + opt_size
    for i in range(num_sections):
        s = sec + i * 40
        va, vsize = struct.unpack('<II', data[s + 12:s + 20])[1], struct.unpack('<I', data[s + 8:s + 12])[0]
        raw_size, raw_ptr = struct.unpack('<II', data[s + 16:s + 24])
        sections.append((struct.unpack('<I', data[s + 12:s + 16])[0], vsize, raw_size, raw_ptr))

    def rva_to_off(rva):
        for va, vsize, raw_size, raw_ptr in sections:
            if va <= rva < va + max(vsize, raw_size):
                return raw_ptr + (rva - va)
        raise AssertionError('rva 0x%x not in any section' % rva)

    base = rva_to_off(rsrc_rva)
    icons = []

    def walk(offset, level, type_id):
        n_named, n_id = struct.unpack('<HH', data[offset + 12:offset + 16])
        for i in range(n_named + n_id):
            e = offset + 16 + i * 8
            name, child = struct.unpack('<II', data[e:e + 8])
            this_type = type_id
            if level == 0:
                this_type = None if name & 0x80000000 else name
            if child & 0x80000000:
                walk(base + (child & 0x7FFFFFFF), level + 1, this_type)
            elif this_type == RT_ICON:
                d = base + child
                data_rva, size = struct.unpack('<II', data[d:d + 8])
                off = rva_to_off(data_rva)
                icons.append(data[off:off + size])

    walk(base, 0, None)
    return icons


def sha(b):
    return hashlib.sha256(b).hexdigest()[:16]


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_EXE
    print('=== Windows executable icon checks (WIN-ICON-001) ===\n')

    # ── the .ico assets ────────────────────────────────────────────────────
    check(os.path.exists(SETA_ICO), 'the SETA icon asset exists', SETA_ICO)
    check(os.path.exists(TECHAIM_ICO), 'the Tech Aim icon asset exists', TECHAIM_ICO)
    if not (os.path.exists(SETA_ICO) and os.path.exists(TECHAIM_ICO)):
        return finish()

    seta = read_ico(SETA_ICO)
    techaim = read_ico(TECHAIM_ICO)
    sizes = sorted(w for w, h, _ in seta)
    check(sizes == REQUIRED_SIZES,
          'the SETA icon carries every size Windows asks for (16..256)',
          str(sizes))
    check(all(w == h for w, h, _ in seta),
          'every SETA icon image is square - the wide logo was not squashed')

    seta_blobs = set(sha(b) for _, _, b in seta)
    techaim_blobs = set(sha(b) for _, _, b in techaim)
    check(not (seta_blobs & techaim_blobs),
          'the two products share no icon image at all')

    # ── the resource script binds them per flavour ─────────────────────────
    rc = open(RC, encoding='utf-8').read()
    pro = open(PRO, encoding='utf-8').read()
    check('#ifdef BRAND_SETA' in rc and 'images/logo/seta.ico' in rc
          and 'images/logo/techaim.ico' in rc,
          'the resource script selects the icon by flavour, keeping both')
    check('RC_DEFINES += BRAND_SETA' in pro,
          'the flavour reaches the RESOURCE compiler, not only the C++ - '
          'otherwise the binary would say SETA and look like Tech Aim')

    # ── and the built executable actually carries it ───────────────────────
    if not os.path.exists(exe):
        print('\nNOTE: no built executable at %s - resource check skipped.' % exe)
        print('      Build it and re-run to verify the shipped binary.')
        return finish()

    in_exe = set(sha(b) for b in pe_icon_resources(exe))
    check(bool(in_exe), 'the executable carries icon resources at all',
          str(len(in_exe)))

    # Which flavour is this binary? Read it from the version resource rather
    # than assuming, so the check works on either build.
    raw = open(exe, 'rb').read()
    is_seta = b'S\x00E\x00T\x00A\x00 \x00E\x00l\x00e\x00c\x00t\x00r\x00o\x00n\x00i\x00c' in raw
    expected, forbidden, name = ((seta_blobs, techaim_blobs, 'SETA') if is_seta
                                 else (techaim_blobs, seta_blobs, 'Tech Aim'))
    print('\n  binary identifies as: %s\n' % name)

    missing = sorted(expected - in_exe)
    check(not missing,
          'the %s executable contains EVERY image from its own icon' % name,
          '%d missing' % len(missing))
    leaked = sorted(forbidden & in_exe)
    check(not leaked,
          'and not one image from the other product\'s icon',
          '%d leaked' % len(leaked))

    # ── the OTHER direction, without needing a second full build ──────────
    # Compiling the resource script both ways proves the mapping itself: the
    # .o is what the linker embeds verbatim, so what is in it is what a build
    # of that flavour would ship.
    import shutil
    import subprocess
    import tempfile
    windres = shutil.which('windres')
    if not windres:
        print('')
        print('NOTE: windres not on PATH - flavour cross-check skipped.')
        return finish()
    tmp = tempfile.mkdtemp()
    try:
        both = {}
        for flavour, args in (('SETA', ['-DBRAND_SETA']), ('Tech Aim', [])):
            obj = os.path.join(tmp, flavour.replace(' ', '') + '.o')
            subprocess.check_call([windres, '-i', RC, '-o', obj,
                                   '--include-dir=' + ROOT] + args,
                                  cwd=ROOT)
            blob = open(obj, 'rb').read()
            both[flavour] = blob
        # Each flavour's own 256px PNG must appear in its object and not in the
        # other one. The 256 image is used because it is the largest and least
        # likely to collide by chance.
        seta_png = [b for w, h, b in seta if w == 256][0]
        techaim_big = max((b for _, _, b in techaim), key=len)
        check(seta_png in both['SETA'] and seta_png not in both['Tech Aim'],
              'compiling with BRAND_SETA embeds the SETA icon, and compiling '
              'without it does not')
        check(techaim_big in both['Tech Aim'] and techaim_big not in both['SETA'],
              'compiling WITHOUT the flavour keeps the Tech Aim icon exactly, '
              'and it never reaches a SETA build')
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    return finish()


def finish():
    print('\n=== %d checks, %d failures ===' % (checks, failures))
    sys.stdout.flush()
    return 0 if failures == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
