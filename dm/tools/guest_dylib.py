#!/usr/bin/env python3





























import hashlib, os, struct, sys

PG = 0x4000
MIN_IOS = 26 << 16


def r16(x): return (x + PG - 1) & ~(PG - 1)


def s16(s):
    b = s.encode(); assert len(b) <= 16, s; return b.ljust(16, b'\0')


def segment(name, addr, size, fileoff, filesize, prot, sects):
    b = struct.pack('<II16sQQQQiiII', 0x19, 72 + 80 * len(sects), s16(name), addr, size, fileoff, filesize, prot, prot, len(sects), 0)
    for sn, a, sz, flags in sects:
        b += struct.pack('<16s16sQQIIIIIIII', s16(sn), s16(name), a, sz, a, 14, 0, 0, flags, 0, 0, 0)
    return b


def dylib_cmd(cmd, name):
    name += b'\0' * (8 - len(name) % 8)
    return struct.pack('<IIIIII', cmd, 24 + len(name), 24, 2, 0x10000, 0x10000) + name


def adhoc_sig(data, text_size):


    n = (len(data) + 4095) // 4096; ident = b'Guest\0'; hdr = struct.calcsize('>IIIIIIIIIBBBBIIIIQQQQ'); assert hdr == 88
    cd = struct.pack('>IIIIIIIIIBBBBIIIIQQQQ', 0xfade0c02, hdr + len(ident) + 32 * n, 0x20400, 0x20002, hdr + len(ident), hdr, 0, n, len(data), 32, 2, 0, 12, 0, 0, 0, 0, 0, 0, text_size, 0)
    cd += ident + b''.join(hashlib.sha256(data[i:i + 4096]).digest() for i in range(0, len(data), 4096))
    return struct.pack('>IIIII', 0xfade0cc0, 20 + len(cd), 1, 0, 20) + cd


def build(out, names):
    text, wsegs = [], []
    for n in names:
        hdr = dict(l[2:].split() for l in open(os.path.join(out, n + '.fixups.tsv')) if l.startswith('# '))
        te, po = int(hdr['TEXT_END'], 16), int(hdr['PATCH_OFF'], 16)
        img = open(os.path.join(out, n + '.patched.bin'), 'rb').read(); sz = len(img); a, end = r16(te), r16(sz)
        assert po % PG == 0 and a <= po <= end, (n, hex(te), hex(po), hex(sz))
        cut = lambda lo, hi: img[lo:min(hi, sz)].ljust(hi - lo, b'\0')
        text += [('__dm%sx0' % n, cut(0, a)), ('__dm%sx1' % n, cut(po, end))]
        wsegs.append(('__DM%sW' % n, cut(a, po)))
    off = PG; tsects = []; body = b''
    for sn, b in text:
        tsects.append((sn, off, len(b), 0x80000400)); body += b; off += len(b)
    cmds = [segment('__TEXT', 0, off, 0, off, 5, tsects)]; tsz = off
    for sn, b in wsegs:
        cmds.append(segment(sn, off, len(b), off, len(b), 3, [('__img', off, len(b), 0)])); body += b; off += len(b)
    strtab = b'\0' * 8
    cs_off = (off + len(strtab) + 15) & ~15; cs_size = (((cs_off // 4096 + 1) * 52 + 4095) & ~4095) + 0x40000
    uuid = hashlib.sha256(body).digest()[:16]
    cmds += [segment('__LINKEDIT', off, r16(cs_off + cs_size - off), off, cs_off + cs_size - off, 1, []),
             dylib_cmd(0xd, b'@rpath/Guest.framework/Guest'),
             struct.pack('<II10I', 0x80000022, 48, *([0] * 10)),
             struct.pack('<IIIIII', 0x2, 24, off, 0, off, len(strtab)),
             struct.pack('<II18I', 0xb, 80, *([0] * 18)),
             struct.pack('<II16s', 0x1b, 24, uuid),
             struct.pack('<IIIIII', 0x32, 24, 2, MIN_IOS, MIN_IOS, 0),
             dylib_cmd(0xc, b'/usr/lib/libSystem.B.dylib'),
             struct.pack('<IIII', 0x1d, 16, cs_off, cs_size)]
    lc = b''.join(cmds); assert 32 + len(lc) <= PG
    mh = struct.pack('<IiiIIIII', 0xfeedfacf, 0x0100000c, 0, 6, len(cmds), len(lc), 0x00100085, 0)
    data = ((mh + lc).ljust(PG, b'\0') + body + strtab).ljust(cs_off, b'\0')
    return (data + adhoc_sig(data, tsz)).ljust(cs_off + cs_size, b'\0')


INFO_PLIST = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>Guest</string>
<key>CFBundleIdentifier</key><string>%s</string>
<key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
<key>CFBundleName</key><string>Guest</string>
<key>CFBundlePackageType</key><string>FMWK</string>
<key>CFBundleShortVersionString</key><string>1.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>MinimumOSVersion</key><string>26.0</string>
<key>CFBundleSupportedPlatforms</key><array><string>iPhoneOS</string></array>
</dict></plist>
'''


def write(out, dst, names, bundle_id='local.guest'):
    fw = os.path.join(dst, 'Guest.framework'); os.makedirs(fw, exist_ok=True)
    data = build(out, names)
    with open(os.path.join(fw, 'Guest'), 'wb') as f: f.write(data)
    with open(os.path.join(fw, 'Info.plist'), 'w', encoding='utf-8', newline='\n') as f: f.write(INFO_PLIST % bundle_id)
    return fw, len(data)


if __name__ == '__main__':
    if len(sys.argv) < 4: sys.exit(__doc__ or '用法：guest_dylib.py <产物目录> <输出目录> <模块名...>')
    fw, n = write(sys.argv[1], sys.argv[2], sys.argv[3:])
    print('%s：%d 字节' % (fw, n))
