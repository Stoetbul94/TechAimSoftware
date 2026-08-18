# -*- coding: utf-8 -*-
"""Build images/logo/seta.ico from the approved SETA logo.

The source is images/logo/seta.png and NOTHING else. No symbol is invented and
no artwork is redrawn: the logo is 401x175, and squashing that whole rectangle
into 16x16 would turn the wordmark into three grey smears. It already contains
a square emblem - the crosshair target with its signal arcs - so the icon is
that emblem, cropped from the artwork by its own alpha channel rather than by
hand-guessed pixel coordinates.

Pure standard library: this must run on a build machine with nothing installed.
Downscaling is area-averaged (box filter), which is what a photo editor's
"bicubic sharper" approximates for large reductions and is the right choice
here - a nearest-neighbour reduction of a thin crosshair loses the crosshair.

Run: python tools/icon/make_seta_ico.py
"""
import struct
import zlib

SRC = 'images/logo/seta.png'
OUT = 'images/logo/seta.ico'
SIZES = [16, 24, 32, 48, 64, 128, 256]
# Windows draws icons up to 64 from the BMP entries; 128 and 256 are PNG, which
# is what every shell since Vista reads and what keeps the file small.
PNG_FROM = 128


def read_png_rgba(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'not a PNG'
    pos, idat, w, h, bitdepth, colour = 8, b'', 0, 0, 0, 0
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos + 4])[0]
        ctype = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]
        if ctype == b'IHDR':
            w, h, bitdepth, colour = struct.unpack('>IIBB', chunk[:10])
        elif ctype == b'IDAT':
            idat += chunk
        elif ctype == b'IEND':
            break
        pos += 12 + length
    assert bitdepth == 8 and colour == 6, 'expected 8-bit RGBA'
    raw = zlib.decompress(idat)
    stride = w * 4
    out = bytearray(h * stride)
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = raw[p]
        p += 1
        line = bytearray(raw[p:p + stride])
        p += stride
        if f == 1:      # Sub
            for i in range(4, stride):
                line[i] = (line[i] + line[i - 4]) & 0xFF
        elif f == 2:    # Up
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif f == 3:    # Average
            for i in range(stride):
                a = line[i - 4] if i >= 4 else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif f == 4:    # Paeth
            for i in range(stride):
                a = line[i - 4] if i >= 4 else 0
                b = prev[i]
                c = prev[i - 4] if i >= 4 else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return w, h, out


def alpha_bbox(w, h, px, threshold=8):
    """Bounding box of everything that is not effectively transparent."""
    x0, y0, x1, y1 = w, h, -1, -1
    for y in range(h):
        row = y * w * 4
        for x in range(w):
            if px[row + x * 4 + 3] > threshold:
                if x < x0: x0 = x
                if x > x1: x1 = x
                if y < y0: y0 = y
                if y > y1: y1 = y
    return x0, y0, x1, y1


def emblem_bbox(w, h, px, threshold=8):
    """The square mark at the left of the logo.

    Found by the artwork's own geometry: scan the alpha column profile from the
    left and stop at the first fully transparent gutter. That gutter is the
    space between the emblem and the wordmark, so the crop follows the logo
    rather than a number typed in by hand.
    """
    cols = []
    for x in range(w):
        ink = 0
        for y in range(h):
            if px[(y * w + x) * 4 + 3] > threshold:
                ink += 1
        cols.append(ink)
    x = 0
    while x < w and cols[x] == 0:
        x += 1
    start = x
    while x < w and cols[x] > 0:
        x += 1
    end = x - 1                      # last inked column of the first shape
    # Vertical extent of that column range only.
    y0, y1 = h, -1
    for yy in range(h):
        row = yy * w * 4
        for xx in range(start, end + 1):
            if px[row + xx * 4 + 3] > threshold:
                if yy < y0: y0 = yy
                if yy > y1: y1 = yy
                break
    return start, y0, end, y1


def crop_square(w, h, px, box, pad_ratio=0.06):
    """Crop `box` and centre it on a transparent square canvas.

    A little padding keeps the mark off the edge, where Windows' own icon
    rounding and shadowing would clip it.
    """
    x0, y0, x1, y1 = box
    bw, bh = x1 - x0 + 1, y1 - y0 + 1
    side = int(max(bw, bh) * (1 + pad_ratio * 2))
    ox, oy = (side - bw) // 2, (side - bh) // 2
    out = bytearray(side * side * 4)
    for y in range(bh):
        src = ((y0 + y) * w + x0) * 4
        dst = ((oy + y) * side + ox) * 4
        out[dst:dst + bw * 4] = px[src:src + bw * 4]
    return side, out


def resize_box(sw, sh, px, dw, dh):
    """Area-averaged resize, premultiplying alpha so transparent pixels do not
    drag their colour into the edges (the classic dark-fringe artefact)."""
    out = bytearray(dw * dh * 4)
    for dy in range(dh):
        sy0, sy1 = dy * sh // dh, max(dy * sh // dh + 1, (dy + 1) * sh // dh)
        for dx in range(dw):
            sx0, sx1 = dx * sw // dw, max(dx * sw // dw + 1, (dx + 1) * sw // dw)
            r = g = b = a = n = 0
            for sy in range(sy0, sy1):
                base = sy * sw * 4
                for sx in range(sx0, sx1):
                    i = base + sx * 4
                    al = px[i + 3]
                    r += px[i] * al
                    g += px[i + 1] * al
                    b += px[i + 2] * al
                    a += al
                    n += 1
            o = (dy * dw + dx) * 4
            if a:
                out[o] = min(255, r // a)
                out[o + 1] = min(255, g // a)
                out[o + 2] = min(255, b // a)
            out[o + 3] = a // n if n else 0
    return out


def png_bytes(w, h, px):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += px[y * w * 4:(y + 1) * w * 4]

    def chunk(tag, payload):
        return (struct.pack('>I', len(payload)) + tag + payload
                + struct.pack('>I', zlib.crc32(tag + payload) & 0xFFFFFFFF))

    return (b'\x89PNG\r\n\x1a\n'
            + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(bytes(raw), 9))
            + chunk(b'IEND', b''))


def bmp_bytes(w, h, px):
    """32-bit BGRA DIB with the AND mask Windows still expects."""
    header = struct.pack('<IiiHHIIiiII', 40, w, h * 2, 1, 32, 0, w * h * 4,
                         0, 0, 0, 0)
    body = bytearray()
    for y in range(h - 1, -1, -1):          # bottom-up
        row = y * w * 4
        for x in range(w):
            i = row + x * 4
            body += bytes((px[i + 2], px[i + 1], px[i], px[i + 3]))
    mask_stride = ((w + 31) // 32) * 4
    mask = bytearray(mask_stride * h)       # all zero: alpha carries the shape
    return header + bytes(body) + bytes(mask)


def main():
    w, h, px = read_png_rgba(SRC)
    full = alpha_bbox(w, h, px)
    mark = emblem_bbox(w, h, px)
    mw, mh = mark[2] - mark[0] + 1, mark[3] - mark[1] + 1
    print('logo %dx%d, ink bbox %s' % (w, h, full))
    print('emblem bbox %s  (%dx%d, aspect %.2f)' % (mark, mw, mh, mw / float(mh)))
    assert 0.75 < mw / float(mh) < 1.35, \
        'the leading mark is not square enough to be the emblem'

    side, sq = crop_square(w, h, px, mark)
    images = []
    for s in SIZES:
        img = resize_box(side, side, sq, s, s)
        images.append((s, img))
        print('  rendered %dx%d' % (s, s))

    entries, blobs, offset = [], [], 6 + 16 * len(images)
    for s, img in images:
        blob = png_bytes(s, s, img) if s >= PNG_FROM else bmp_bytes(s, s, img)
        entries.append(struct.pack('<BBBBHHII', 0 if s == 256 else s,
                                   0 if s == 256 else s, 0, 0, 1, 32,
                                   len(blob), offset))
        offset += len(blob)
        blobs.append(blob)
    with open(OUT, 'wb') as f:
        f.write(struct.pack('<HHH', 0, 1, len(images)))
        for e in entries:
            f.write(e)
        for b in blobs:
            f.write(b)
    print('wrote %s (%d bytes, %d images)' % (OUT, offset, len(images)))


if __name__ == '__main__':
    main()
