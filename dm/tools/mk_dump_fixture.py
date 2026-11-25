#!/usr/bin/env python3















import argparse
import hashlib
import os
import random
import shutil
import struct
import subprocess
import sys
import tempfile

try:
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
except ImportError:
    sys.exit('mk_dump_fixture.py needs the Python package cryptography (pip install cryptography), or a Python that has it')

HERE = os.path.dirname(os.path.abspath(__file__))
VECTORS = os.path.join(HERE, 'dump_fixture_vectors.tsv')
SEED = 105
TITLE = 0x0100C0DE00105000
ADDON = (TITLE & ~0xFFF) + 0x1000 + 1
KEY_GEN = 0x0B
ARCHIVES = ['00-00000001.data', '00-00000002.data', '00-00000003.data', '00-00000004.data', '00-00000005.data',
            '00-00000006.data', '01-00000001.data', '01-00000002.data', '02-00000001.data', '02-00000002.data',
            '03-00000001.data', '03-00000002.data', '04-00000001.data', '04-00000002.data', '05-00000001.data',
            '05-00000002.data', '17-00000001.data', '17-00000002.data']
META = ['.build.config', 'build_key.txt', 'archive_manifest.psv', 'base_manifest.psv', 'build_manifest.psv',
        'equivalent_manifest.psv']
EXEFS = ['main', 'sdk', 'subsdk0']
IVFC_LOG = 14


def ecb(key, data, decrypt=False):
    c = Cipher(algorithms.AES(key), modes.ECB())
    op = c.decryptor() if decrypt else c.encryptor()
    return op.update(data) + op.finalize()


def ctr(key, ctr_hi, abs_off, data):
    assert abs_off % 16 == 0
    iv = ctr_hi + struct.pack('>Q', abs_off >> 4)
    op = Cipher(algorithms.AES(key), modes.CTR(iv)).encryptor()
    return op.update(data) + op.finalize()


def xts_header(header_key, plain):
    out = b''
    for unit in range(len(plain) // 0x200):
        op = Cipher(algorithms.AES(header_key), modes.XTS(unit.to_bytes(16, 'big'))).encryptor()
        out += op.update(plain[unit * 0x200:(unit + 1) * 0x200]) + op.finalize()
    return out


def align(n, a):
    return (n + a - 1) // a * a


def pad(b, a):
    return b + bytes(align(len(b), a) - len(b))


def sha(b):
    return hashlib.sha256(b).digest()


def pfs0(files):

    strs = b''
    offs = []
    for name, _ in files:
        offs.append(len(strs))
        strs += name.encode() + b'\0'
    strs = pad(strs, 0x20)
    head = b'PFS0' + struct.pack('<III', len(files), len(strs), 0)
    data = b''
    for (name, body), so in zip(files, offs):
        head += struct.pack('<QQII', len(data), len(body), so, 0)
        data += body
    return head + strs + data




def path_hash(parent, name):
    h = parent ^ 123456789
    for c in name:
        h = ((h >> 5) | (h << 27)) & 0xFFFFFFFF
        h ^= c
    return h


def bucket_count(n):
    if n < 3:
        return 3
    if n < 19:
        return n | 1
    while any(n % p == 0 for p in (2, 3, 5, 7, 11, 13, 17)):
        n += 1
    return n


def romfs(tree):

    dirs, files = [], []

    def walk(node, name, parent):
        idx = len(dirs)
        dirs.append({'name': name.encode(), 'parent': parent, 'node': node, 'dirs': [], 'files': []})
        for fname, body in node.get('files', []):
            dirs[idx]['files'].append(len(files))
            files.append({'name': fname.encode(), 'parent': idx, 'body': body, 'path': (name, fname)})
        for dname, sub in node.get('dirs', []):
            dirs[idx]['dirs'].append(walk(sub, dname, idx))
        return idx

    walk(tree, '', 0)
    for d in dirs:
        d['size'] = 0x18 + align(len(d['name']), 4)
    off = 0
    for d in dirs:
        d['off'] = off
        off += d['size']
    dir_meta_size = off
    off = 0
    for f in files:
        f['off'] = off
        off += 0x20 + align(len(f['name']), 4)
    file_meta_size = off
    data = b''
    for f in files:
        data = pad(data, 0x10)
        f['data_off'] = len(data)
        data += f['body']
    none = 0xFFFFFFFF
    dir_buckets, file_buckets = [none] * bucket_count(len(dirs)), [none] * bucket_count(len(files))
    dnext, fnext = {}, {}
    for i, d in enumerate(dirs):
        h = path_hash(dirs[d['parent']]['off'], d['name']) % len(dir_buckets)
        dnext[i], dir_buckets[h] = dir_buckets[h], d['off']
    for i, f in enumerate(files):
        h = path_hash(dirs[f['parent']]['off'], f['name']) % len(file_buckets)
        fnext[i], file_buckets[h] = file_buckets[h], f['off']
    dmeta = b''
    for i, d in enumerate(dirs):
        par = dirs[d['parent']]
        sib = none
        siblings = par['dirs'] if i else []
        if i and siblings.index(i) + 1 < len(siblings):
            sib = dirs[siblings[siblings.index(i) + 1]]['off']
        child = dirs[d['dirs'][0]]['off'] if d['dirs'] else none
        cfile = files[d['files'][0]]['off'] if d['files'] else none
        dmeta += struct.pack('<IIIIII', par['off'], sib, child, cfile, dnext[i], len(d['name'])) + pad(d['name'], 4)
    fmeta = b''
    for i, f in enumerate(files):
        sibl = dirs[f['parent']]['files']
        sib = files[sibl[sibl.index(i) + 1]]['off'] if sibl.index(i) + 1 < len(sibl) else none
        fmeta += struct.pack('<IIQQII', dirs[f['parent']]['off'], sib, f['data_off'], len(f['body']), fnext[i],
                             len(f['name'])) + pad(f['name'], 4)
    data_off = 0x200
    tail = align(data_off + len(data), 4)
    dh_off = tail
    dh = b''.join(struct.pack('<I', x) for x in dir_buckets)
    dm_off = dh_off + len(dh)
    fh_off = dm_off + len(dmeta)
    fh = b''.join(struct.pack('<I', x) for x in file_buckets)
    fm_off = fh_off + len(fh)
    head = struct.pack('<QQQQQQQQQQ', 0x50, dh_off, len(dh), dm_off, dir_meta_size, fh_off, len(fh), fm_off, file_meta_size, data_off)
    img = pad(head, data_off) + data
    img = pad(img, 4)
    assert len(img) == tail
    img += dh + dmeta + fh + fmeta
    where = {f['path']: data_off + f['data_off'] for f in files}
    return img, where


def ivfc(data):

    bs = 1 << IVFC_LOG
    levels = [data]
    for _ in range(5):
        below = levels[0]
        levels.insert(0, b''.join(sha(pad(below[i:i + bs], bs)) for i in range(0, len(below), bs)))
    assert len(levels[0]) <= bs
    master = sha(pad(levels[0], bs))
    img, table = b'', []
    for lv in levels:
        img = pad(img, bs)
        table.append((len(img), len(lv)))
        img += lv
    return pad(img, 0x200), table, master


def ivfc_header(table, master):
    h = b'IVFC' + struct.pack('<III', 0x20000, 0x20, 7)
    for off, size in table:
        h += struct.pack('<QQII', off, size, IVFC_LOG, 0)
    h += bytes(0x20) + master
    assert len(h) == 0xE0
    return h




def fs_header(fs_kind, hash_kind, crypt, superblock, generation, secure, bktr=b''):
    h = struct.pack('<HBBB', 2, fs_kind, hash_kind, crypt) + bytes(3) + superblock
    h = h + bytes(0x100 - len(h))
    h += bktr + bytes(0x40 - len(bktr))
    h += struct.pack('<II', generation, secure)
    return h + bytes(0x200 - len(h))


def exefs_section(files, bs=0x1000):
    p = pfs0(files)
    table = b''.join(sha(p[i:i + bs]) for i in range(0, len(p), bs))
    po = align(len(table), 0x200)
    plain = pad(pad(table, 0x200) + p, 0x200)
    sb = sha(table) + struct.pack('<IIQQQQ', bs, 2, 0, len(table), po, len(p))
    return plain, sb


class Nca:


    def __init__(self, rnd, keys, ctype, rights_id, title=TITLE, key_area=None):
        self.rnd, self.keys, self.ctype, self.rights_id, self.title, self.key_area = rnd, keys, ctype, rights_id, title, key_area
        self.sections = []

    def add(self, fsh, size, encrypt):
        self.sections.append((fsh, size, encrypt))

    def build(self, mutate_header=None):
        body = b''
        entries, fshs, hashes = b'', b'', b''
        cur = 0xC00
        parts = []
        for fsh, size, encrypt in self.sections:
            start = align(cur, 0x200)
            parts.append((start, encrypt(start)))
            assert len(parts[-1][1]) == size and size % 0x200 == 0
            entries += struct.pack('<IIII', start // 0x200, (start + size) // 0x200, 1, 0)
            fshs += fsh
            cur = start + size
        total = cur
        entries += bytes(0x40 - len(entries))
        fshs += bytes(0x800 - len(fshs))
        if mutate_header:
            fshs = mutate_header('fsh', fshs)
        for i in range(4):
            hashes += sha(fshs[i * 0x200:(i + 1) * 0x200]) if i < len(self.sections) else bytes(32)
        key_area = self.key_area if self.key_area is not None else self.rnd.randbytes(0x40)
        h = self.rnd.randbytes(0x200) + b'NCA3' + struct.pack('<BBBBQQII', 0, self.ctype, 2, 0, total, self.title, 0, 0x000C1100)
        h += struct.pack('<BB', KEY_GEN, 0) + bytes(0xE) + self.rights_id + entries + hashes + key_area + bytes(0xC0) + fshs
        assert len(h) == 0xC00
        if mutate_header:
            h = mutate_header('header', h)
        out = bytearray(xts_header(self.keys['header_key'], h) + bytes(total - 0xC00))
        for start, data in parts:
            out[start:start + len(data)] = data
        return bytes(out)


def ticket(rnd, rights_id, enc_key, kind=0):
    body = b'Root-CA00000003-XS00000020'.ljust(0x40, b'\0') + enc_key + bytes(0xF0)
    body += struct.pack('<BBHBBH', 2, kind, 0, 0, KEY_GEN, 0) + bytes(8) + rnd.randbytes(8) + bytes(8) + rights_id
    body += struct.pack('<IIIHH', 0, 0, 0x2C0, 0, 0)
    t = struct.pack('<I', 0x10004) + rnd.randbytes(0x100) + bytes(0x3C) + body
    assert len(t) == 0x2C0
    return t


def rights(title):
    return struct.pack('>Q', title) + bytes(7) + bytes([KEY_GEN])


def bucket_tree(entries, per_bucket, entry_size, end, pack):

    buckets = [entries[i:i + per_bucket] for i in range(0, len(entries), per_bucket)]
    head = struct.pack('<IIQ', 0, len(buckets), end) + b''.join(struct.pack('<Q', b[0][0]) for b in buckets)
    out = pad(head, 0x4000)
    for i, b in enumerate(buckets):
        bend = buckets[i + 1][0][0] if i + 1 < len(buckets) else end
        bk = struct.pack('<IIQ', 0, len(b), bend) + b''.join(pack(e) for e in b)
        assert len(bk) <= 0x4000 and entry_size
        out += pad(bk, 0x4000)
    return out




def payloads(rnd):

    new_data, base_data = {}, {}
    for i, name in enumerate(ARCHIVES):
        new_data[name] = rnd.randbytes(rnd.randrange(24_000, 160_000))
        if name == '17-00000002.data':
            continue
        base_data[name] = new_data[name] if i % 3 != 1 else rnd.randbytes(rnd.randrange(24_000, 160_000))
    for name in META:
        size = {'.build.config': 857, 'build_key.txt': 32}.get(name, rnd.randrange(40_000, 120_000))
        new_data[name] = rnd.randbytes(size)
        base_data[name] = rnd.randbytes(size)
    extra = {n: rnd.randbytes(rnd.randrange(5_000, 30_000)) for n in ('06-00000001.data', '16-00000002.data', 'zz-unused.bin')}
    new_exefs = [(n, rnd.randbytes(s)) for n, s in (('main', 301_337), ('sdk', 97_331), ('subsdk0', 55_559), ('rtld', 4_201))]
    base_exefs = [(n, rnd.randbytes(len(b))) for n, b in new_exefs]
    return base_data, new_data, extra, base_exefs, new_exefs


def tree_of(data, extra, order):
    return {'dirs': [('other', {'files': [('readme.bin', extra['zz-unused.bin'])]}),
                     ('data', {'files': [(n, data[n]) for n in order if n in data] +
                               [(n, extra[n]) for n in ('06-00000001.data', '16-00000002.data')],
                               'dirs': [('sub', {'files': [('x.bin', extra['zz-unused.bin'][:999])]})]})],
            'files': [('root.bin', extra['06-00000001.data'][:123])]}


def build(rnd, mut=None):

    keys = {'header_key': rnd.randbytes(32), 'titlekek': rnd.randbytes(16), 'kak': rnd.randbytes(16)}
    tk_base, tk_upd = rnd.randbytes(16), rnd.randbytes(16)
    base_data, new_data, extra, base_exefs, new_exefs = payloads(rnd)
    names = ARCHIVES + META

    base_order = [n for n in names if n in base_data]
    new_order = ['17-00000002.data'] + [n for n in names if n != '17-00000002.data']
    base_img, base_where = romfs(tree_of(base_data, extra, base_order))
    new_img, new_where = romfs(tree_of(new_data, extra, new_order))
    base_sec, base_tab, base_master = ivfc(base_img)
    v_sec, v_tab, v_master = ivfc(new_img)

    def mutate(stage, b):
        fn = VARIANTS.get(mut) if mut else None
        return fn(stage, bytearray(b)) if fn else b


    v_data0, b_data0 = v_tab[5][0], base_tab[5][0]
    reuse, reuse_names = [], []
    for n in names:
        if n in base_data and base_data[n] == new_data[n]:
            reuse_names.append(n)
            v0 = v_data0 + new_where[('data', n)]
            b0 = b_data0 + base_where[('data', n)]
            ln = len(new_data[n]) & ~15
            if ln:
                reuse.append((v0, b0, ln))
    reuse.sort()
    relocs, patch = [], b''
    v = 0
    for v0, b0, ln in reuse + [(len(v_sec), None, 0)]:
        if v0 > v:
            relocs.append((v, len(patch), 1))
            patch += v_sec[v:v0]
            patch = pad(patch, 16)
        if b0 is not None:
            relocs.append((v0, b0, 0))
        v = v0 + ln
    patch = pad(patch, 0x200)
    pend = len(patch)
    if mut == 'upd-patch-flip':
        v = v_data0 + new_where[('data', '00-00000002.data')] + 5
        ev, p0, _ = [r for r in relocs if r[0] <= v][-1]
        patch = bytearray(patch)
        patch[p0 + v - ev] ^= 1
        patch = bytes(patch)
    reloc_tab = bucket_tree(relocs, 5, 0x14, len(v_sec), lambda e: struct.pack('<QQI', *e))
    subs = [(o, 0x10000 + 7 * i + rnd.randrange(1000)) for i, o in enumerate(range(0, pend, 0x3000))]
    sub_tab = bucket_tree(subs, 4, 0x10, pend + len(reloc_tab), lambda e: struct.pack('<QII', e[0], 0, e[1]))
    reloc_tab, sub_tab = mutate('reloc', reloc_tab), mutate('sub', sub_tab)
    generation, secure = 0x10000 + rnd.randrange(1000), rnd.randrange(1 << 32)
    hi = struct.pack('>II', secure, generation)

    def upd_romfs_encrypt(start):
        p = patch
        out = b''
        for i, (o, cv) in enumerate(subs):
            end = subs[i + 1][0] if i + 1 < len(subs) else pend
            out += ctr(tk_upd, hi[:4] + struct.pack('>I', cv), start + o, p[o:end])
        return out + ctr(tk_upd, hi, start + pend, reloc_tab + sub_tab)

    upd_size = pend + len(reloc_tab) + len(sub_tab)
    bktr = (struct.pack('<QQ', pend, len(reloc_tab)) + b'BKTR' + struct.pack('<III', 1, len(relocs), 0) +
            struct.pack('<QQ', pend + len(reloc_tab), len(sub_tab)) + b'BKTR' + struct.pack('<III', 1, len(subs), 0))

    def archive(tk, exefs_files, romfs_fsh, romfs_size, romfs_encrypt, rid, label, key_area=None):
        n = Nca(rnd, keys, 0, rid, key_area=key_area)
        plain, sb = exefs_section(exefs_files)
        g0, s0 = 0x20 + rnd.randrange(1000), rnd.randrange(1 << 32)
        h0 = struct.pack('>II', s0, g0)
        exefs_plain = mutate(label + '-exefs', plain)
        n.add(fs_header(1, 2, 3, sb, g0, s0), len(plain), lambda st: ctr(tk, h0, st, exefs_plain))
        n.add(romfs_fsh, romfs_size, romfs_encrypt)
        return n.build(mutate_header=(lambda stage, b: mutate(label + '-' + stage, b)))

    bg, bs_ = 0x30 + rnd.randrange(1000), rnd.randrange(1 << 32)
    bhi = struct.pack('>II', bs_, bg)
    base_fsh = fs_header(0, 3, 3, ivfc_header(base_tab, base_master), bg, bs_)
    base_sec_m = base_sec
    if mut == 'base-data-flip':
        o = b_data0 + base_where[('data', reuse_names[0])] + 7
        base_sec_m = base_sec[:o] + bytes([base_sec[o] ^ 1]) + base_sec[o + 1:]
    base_nca = archive(tk_base, base_exefs, base_fsh, len(base_sec), lambda st: ctr(tk_base, bhi, st, base_sec_m), rights(TITLE), 'base')
    upd_fsh = fs_header(0, 3, 4, ivfc_header(v_tab, v_master), generation, secure, bktr)
    upd_nca = archive(tk_upd, new_exefs, upd_fsh, upd_size, upd_romfs_encrypt, rights(TITLE | 0x800), 'upd')

    def meta_nca():
        n = Nca(rnd, keys, 1, bytes(16))
        plain, sb = exefs_section([('Application_%016x.cnmt' % TITLE, rnd.randbytes(0x1A0))])
        n.add(fs_header(1, 2, 3, sb, 0, 0), len(plain), lambda st: rnd.randbytes(len(plain)))
        return n.build()

    def package(nca, title, tk, kind):
        rid = rights(title)
        files = [(sha(nca)[:16].hex() + '.nca', nca), (sha(b'meta' + nca)[:16].hex() + '.cnmt.nca', meta_nca())]
        if mut != 'upd-no-ticket' or kind != 'upd':
            files.append((rid.hex() + '.tik', mutate(kind + '-ticket', ticket(rnd, rid, ecb(keys['titlekek'], tk)))))
        files.append((rid.hex() + '.cert', rnd.randbytes(0x700)))
        return mutate(kind + '-package', pfs0(files))

    out = {'base.nsp': package(base_nca, TITLE, tk_base, 'base'), 'update.nsp': package(upd_nca, TITLE | 0x800, tk_upd, 'upd')}
    out['keys.txt'] = ('header_key = %s\ntitlekek_%02x = %s\nkey_area_key_application_%02x = %s\n' % (
        keys['header_key'].hex(), KEY_GEN - 1, keys['titlekek'].hex(), KEY_GEN - 1, keys['kak'].hex())).encode()



    def addon_files():
        rid, tk = rights(ADDON), rnd.randbytes(16)
        files = []
        for ctype, r in ((5, rid), (1, bytes(16))):
            n = Nca(rnd, keys, ctype, r, ADDON)
            plain, sb = exefs_section([('AddOnContent_%016x.cnmt' % ADDON if ctype == 1 else 'data', rnd.randbytes(0x400))])
            n.add(fs_header(1, 2, 3, sb, 0, 0), len(plain), lambda st, size=len(plain): rnd.randbytes(size))
            nca = n.build()
            files.append((sha(nca)[:16].hex() + ('.cnmt.nca' if ctype == 1 else '.nca'), nca))
        files.append((rid.hex() + '.tik', ticket(rnd, rid, ecb(keys['titlekek'], tk))))
        files.append((rid.hex() + '.cert', rnd.randbytes(0x700)))
        return files
    out['addon.nsp'] = pfs0(addon_files())

    key_area = rnd.randbytes(0x20) + ecb(keys['kak'], tk_base) + rnd.randbytes(0x10)
    bare = archive(tk_base, base_exefs, base_fsh, len(base_sec), lambda st: ctr(tk_base, bhi, st, base_sec_m), bytes(16), 'bundle', key_area)
    out['base-addon.nsp'] = pfs0([(sha(bare)[:16].hex() + '.nca', bare)] + addon_files())
    return out




def at(off, value):
    def f(stage, b):
        b[off:off + len(value)] = value
        return bytes(b)
    return f


def fsh_field(section, off, value):
    def f(stage, b):
        if stage == 'upd-fsh':
            b[section * 0x200 + off:section * 0x200 + off + len(value)] = value
        return bytes(b)
    return f


def only(stage_name, fn):
    return lambda stage, b: fn(stage, b) if stage == stage_name else bytes(b)


def flip(off):
    def f(stage, b):
        b[off] ^= 0x01
        return bytes(b)
    return f


VARIANTS = {
    'upd-reloc-count': fsh_field(1, 0x118, struct.pack('<I', 999)),
    'upd-reloc-buckets': only('reloc', at(4, struct.pack('<I', 0xFFFF))),
    'upd-reloc-offset': only('reloc', at(0x4000 + 0x10 + 0x14 + 8, struct.pack('<Q', (1 << 64) - 0x100))),
    'upd-reloc-order': only('reloc', at(0x4000 + 0x10 + 0x14, struct.pack('<Q', 0))),
    'upd-sub-order': only('sub', at(0x4000 + 0x10 + 0x10, struct.pack('<Q', 0))),
    'upd-sub-end': only('sub', at(8, struct.pack('<Q', 0x10))),
    'upd-table-offset': fsh_field(1, 0x100, struct.pack('<Q', (1 << 64) - 0x10)),
    'upd-ivfc-size': fsh_field(1, 8 + 0x10 + 0x18 * 2 + 8, struct.pack('<Q', 1 << 40)),
    'upd-ivfc-block': fsh_field(1, 8 + 0x10 + 0x18 * 3 + 16, struct.pack('<I', 30)),
    'upd-feature': fsh_field(1, 0x150, b'\x01'),
    'upd-nca2': only('upd-header', at(0x200, b'NCA2')),
    'upd-ticket-personal': only('upd-ticket', at(0x140 + 0x141, b'\x01')),
    'upd-no-ticket': lambda stage, b: bytes(b),
    'upd-patch-flip': None,
    'base-data-flip': None,
    'upd-exefs-flip': only('upd-exefs', flip(0x3000)),
    'pfs-count': only('upd-package', at(4, struct.pack('<I', 0xFFFF))),
    'pfs-offset': only('upd-package', at(16, struct.pack('<Q', (1 << 64) - 0x100))),
    'pfs-strtab': only('upd-package', at(8, struct.pack('<I', 0x80000000))),
}


def generate(seed, mut=None):
    return build(random.Random(seed), mut)




def read_vectors():
    with open(VECTORS, encoding='utf-8') as f:
        rows = [l.rstrip('\n').split('\t') for l in f if not l.startswith('#')]
    head = {r[0]: r[1:] for r in rows if r[0] in ('fixture', 'title')}
    files = [r for r in rows if r[0] in ('exefs', 'data')]
    return head, files


def contract_text(title, files):
    lines = ['title\t%016x' % title]
    for target, name, size, digest in files:
        unit = 'exefs' if target == 'exefs' else 'meta' if name in META else name
        lines.append('file\t%s\t%s\t%s\t%s\t%s' % (unit, target, name, size, digest))
    return '\n'.join(lines) + '\n'


def fixture_id(out):
    return sha(out['base.nsp']).hex() + '\t' + sha(out['update.nsp']).hex()


def hactool_vectors(out, workdir):
    ht = os.environ.get('HACTOOL') or shutil.which('hactool')
    if not ht:
        sys.exit('--vectors needs hactool: set HACTOOL=<path> or put it on PATH')
    home = os.path.join(workdir, 'home')
    os.makedirs(os.path.join(home, '.switch'))
    keyfile = os.path.join(workdir, 'keys.txt')
    open(keyfile, 'wb').write(out['keys.txt'])
    env = dict(os.environ, HOME=home)

    def run(*args):
        r = subprocess.run([ht, '-k', keyfile, '--disablekeywarns'] + list(args), env=env, capture_output=True, text=True)
        if r.returncode:
            sys.exit('hactool failed: %s\n%s' % (args, r.stdout + r.stderr))
        return r.stdout

    programs, titlekeys = {}, []
    for pkg in ('base.nsp', 'update.nsp'):
        d = os.path.join(workdir, pkg + '.d')
        open(os.path.join(workdir, pkg), 'wb').write(out[pkg])
        run('-t', 'pfs0', '--outdir=' + d, os.path.join(workdir, pkg))
        for name in sorted(os.listdir(d)):
            p = os.path.join(d, name)
            if name.endswith('.tik'):
                t = open(p, 'rb').read()
                titlekeys.append('%s = %s' % (t[0x140 + 0x160:0x140 + 0x170].hex(), t[0x140 + 0x40:0x140 + 0x50].hex()))
            elif name.endswith('.nca') and not name.endswith('.cnmt.nca'):
                programs[pkg] = p
    open(os.path.join(home, '.switch', 'title.keys'), 'w').write('\n'.join(titlekeys) + '\n')
    info = run('-y', '-i', programs['base.nsp'])
    if 'Content Type:                       Program' not in info:
        sys.exit('hactool does not see a program content in the base package')
    ex, rf = os.path.join(workdir, 'exefs'), os.path.join(workdir, 'romfs')
    log = run('-y', '--basenca=' + programs['base.nsp'], '--section0dir=' + ex, '--romfsdir=' + rf, programs['update.nsp'])
    for line in log.splitlines():
        if 'FAIL' in line and ('Hash' in line or 'IVFC' in line):
            sys.exit('hactool reports a failed hash check: ' + line.strip())
    files = []
    for name in EXEFS:
        b = open(os.path.join(ex, name), 'rb').read()
        files.append(('exefs', name, str(len(b)), sha(b).hex()))
    for name in META + ARCHIVES:
        b = open(os.path.join(rf, 'data', name), 'rb').read()
        files.append(('data', name, str(len(b)), sha(b).hex()))
    return files


def write_outputs(outdir, out, contract):
    os.makedirs(outdir, exist_ok=True)
    for name, data in out.items():
        open(os.path.join(outdir, name), 'wb').write(data)
    open(os.path.join(outdir, 'contract.tsv'), 'w').write(contract)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', required=True)
    ap.add_argument('--vectors', action='store_true', help='extract with hactool and rewrite ' + os.path.basename(VECTORS))
    ap.add_argument('--no-variants', action='store_true', help='skip the malformed variants (screens only)')
    a = ap.parse_args()
    out = generate(SEED)
    if a.vectors:
        with tempfile.TemporaryDirectory() as w:
            files = hactool_vectors(out, w)
        with open(VECTORS, 'w', encoding='utf-8') as f:
            f.write('# Expected files of the synthetic dump fixture, extracted by hactool from mk_dump_fixture.py output (seed %d).\n' % SEED)
            f.write('# Regenerate with: python3 dm/tools/mk_dump_fixture.py --out <dir> --vectors\n')
            f.write('fixture\t%s\ntitle\t%016x\n' % (fixture_id(out), TITLE))
            for r in files:
                f.write('\t'.join(r) + '\n')
    head, files = read_vectors()
    if '\t'.join(head['fixture']) != fixture_id(out):
        sys.exit('the fixture no longer matches %s: rerun with --vectors' % os.path.basename(VECTORS))
    write_outputs(a.out, out, contract_text(int(head['title'][0], 16), files))
    if not a.no_variants:
        vdir = os.path.join(a.out, 'variants')
        os.makedirs(vdir, exist_ok=True)
        for v in VARIANTS:
            o = generate(SEED, v)
            for pkg in ('base.nsp', 'update.nsp'):
                if o[pkg] != out[pkg]:
                    open(os.path.join(vdir, '%s.%s' % (v, pkg)), 'wb').write(o[pkg])
    print('fixture written to %s' % a.out)


if __name__ == '__main__':
    main()
