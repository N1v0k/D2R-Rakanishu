#!/usr/bin/env python3


















import argparse
import bisect
import collections
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
DM = os.path.normpath(os.path.join(HERE, '..'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(DM, 'verify'))
import game_addrs

MODULES = ('main', 'sdk', 'subsdk0')
R_AARCH64_RELATIVE = 1027



class Program:


    def __init__(self, path):
        import nsoimg
        d = open(path, 'rb').read()
        if d[:4] != b'NSO0':
            raise SystemExit(f'gameport: {path} is not an NSO module')
        segs = [struct.unpack_from('<III', d, 0x10 + i * 0x10) for i in range(3)]
        bss = struct.unpack_from('<I', d, 0x3c)[0]
        self.path = path
        self.sha256 = hashlib.sha256(d).hexdigest()
        self.build_id = d[0x40:0x50].hex()
        self.text_end = segs[0][2]
        self.ro_start = segs[1][1]
        self.data_start = segs[2][1]
        self.image_end = segs[2][1] + segs[2][2] + bss
        self.img = nsoimg.load(path)

    def words(self):
        return np.frombuffer(self.img, dtype='<u4', count=self.text_end // 4)

    def relocs(self):

        import nsoimg
        return {o: a for o, t, _, _, a in nsoimg.relocs(self.img) if t == R_AARCH64_RELATIVE}

    def func_starts(self):
        import nsoimg
        return nsoimg.func_starts(self.img, self.text_end)



def masks(w):


    m = np.full(w.shape, 0xffffffff, dtype=np.uint32)
    is_b = ((w >> 26) & 0x1f) == 0b00101
    is_adrp = (w & 0x9f000000) == 0x90000000
    is_adr = (w & 0x9f000000) == 0x10000000
    is_bcond = (w >> 24) == 0x54
    is_cbz = ((w >> 25) & 0x3f) == 0x1a
    is_tbz = ((w >> 25) & 0x3f) == 0x1b
    is_ldrlit = ((w >> 24) & 0x3b) == 0x18
    m[is_b] = 0xfc000000
    m[is_adrp | is_adr] = 0x9f00001f
    m[is_bcond | is_cbz | is_ldrlit] = 0xff00001f
    m[is_tbz] = 0xfff8001f
    is_addimm = (w & 0x7f800000) == 0x11000000
    is_ldst = (w & 0x3b000000) == 0x39000000
    rn = ((w >> 5) & 31).astype(np.uint8)
    cand = is_addimm | is_ldst
    rd_adrp = np.where(is_adrp, w & 31, 99).astype(np.uint8)
    is_call = ((w & 0xfc000000) == 0x94000000) | ((w & 0xfffffc1f) == 0xd63f0000) | (w == 0xd65f03c0)
    low5 = (w & 31).astype(np.uint8)
    pair = np.zeros(w.shape, dtype=bool)
    killed = np.zeros(w.shape, dtype=bool)
    for k in range(1, 25):
        prev = np.full_like(rd_adrp, 99)
        prev[k:] = rd_adrp[:-k]
        pair |= cand & (prev == rn) & ~killed
        pl = np.full_like(low5, 99)
        pl[k:] = low5[:-k]
        pc = np.zeros(w.shape, dtype=bool)
        pc[k:] = is_call[:-k]
        killed |= (pl == rn) | (pc & (rn <= 18))
    m[pair] &= np.uint32(0xffc003ff)
    return m, pair, is_adrp


def _rolling(tok, k):
    h = np.zeros(len(tok) - k + 1, dtype=np.uint64)
    for j in range(k):
        h = h * np.uint64(0x9E3779B97F4A7C15) + tok[j:len(tok) - k + 1 + j].astype(np.uint64) + np.uint64(j + 1)
    return h


def _unique_pairs(ha, hb):
    ua, ia, ca = np.unique(ha, return_index=True, return_counts=True)
    ub, ib, cb = np.unique(hb, return_index=True, return_counts=True)
    ua, ia = ua[ca == 1], ia[ca == 1]
    ub, ib = ub[cb == 1], ib[cb == 1]
    _, xa, xb = np.intersect1d(ua, ub, return_indices=True)
    pa, pb = ia[xa], ib[xb]
    o = np.argsort(pa)
    return pa[o], pb[o]


def _lis(seq):
    tails, tails_idx, prev = [], [], [-1] * len(seq)
    for i, v in enumerate(seq):
        j = bisect.bisect_left(tails, v)
        if j == len(tails):
            tails.append(v)
            tails_idx.append(i)
        else:
            tails[j] = v
            tails_idx[j] = i
        prev[i] = tails_idx[j - 1] if j else -1
    out, i = [], tails_idx[-1] if tails_idx else -1
    while i >= 0:
        out.append(i)
        i = prev[i]
    return out[::-1]


def _align(ta, tb, a0, a1, b0, b1, k, runs, depth=0):


    if a1 - a0 < 1 or b1 - b0 < 1:
        return
    n = min(a1 - a0, b1 - b0)
    eq = ta[a0:a0 + n] == tb[b0:b0 + n]
    pre = n if eq.all() else int(np.argmin(eq))
    if pre:
        runs.append((a0, b0, pre))
        a0 += pre
        b0 += pre
    if a1 - a0 < 1 or b1 - b0 < 1:
        return
    n = min(a1 - a0, b1 - b0)
    eq = ta[a1 - n:a1][::-1] == tb[b1 - n:b1][::-1]
    suf = n if eq.all() else int(np.argmin(eq))
    if suf:
        runs.append((a1 - suf, b1 - suf, suf))
        a1 -= suf
        b1 -= suf
    if a1 - a0 < k or b1 - b0 < k:
        if k > 2:
            _align(ta, tb, a0, a1, b0, b1, max(2, k // 2), runs, depth + 1)
        return
    pa, pb = _unique_pairs(_rolling(ta[a0:a1], k), _rolling(tb[b0:b1], k))
    if len(pa) == 0:
        if k > 2:
            _align(ta, tb, a0, a1, b0, b1, max(2, k // 2), runs, depth + 1)
        return
    keep = _lis(pb.tolist())
    pa, pb = pa[keep] + a0, pb[keep] + b0
    ca, cb, i, count = a0, b0, 0, len(pa)
    while i < count:
        a, b = int(pa[i]), int(pb[i])
        if a < ca or b < cb:
            i += 1
            continue
        s = 0
        while a - s - 1 >= ca and b - s - 1 >= cb and ta[a - s - 1] == tb[b - s - 1]:
            s += 1
        a -= s
        b -= s
        n = min(a1 - a, b1 - b)
        eq = ta[a:a + n] == tb[b:b + n]
        ln = n if eq.all() else int(np.argmin(eq))
        if (ca < a or cb < b) and depth < 40:
            _align(ta, tb, ca, a, cb, b, max(2, k // 2) if (a - ca) < 4096 else k, runs, depth + 1)
        runs.append((a, b, ln))
        ca, cb = a + ln, b + ln
        while i < count and pa[i] < ca:
            i += 1
    if ca < a1 and cb < b1 and depth < 40 and (ca, cb) != (a0, b0):
        _align(ta, tb, ca, a1, cb, b1, max(2, k // 2) if (a1 - ca) < 4096 else k, runs, depth + 1)


def align_code(wa, wb, window=12):

    ma, _, _ = masks(wa)
    mb, _, _ = masks(wb)
    ta, tb = wa & ma, wb & mb
    old_limit = sys.getrecursionlimit()
    sys.setrecursionlimit(max(old_limit, 10000))
    runs = []
    try:
        _align(ta, tb, 0, len(ta), 0, len(tb), window, runs)
    finally:
        sys.setrecursionlimit(old_limit)
    clean = []
    for a, b, n in sorted(runs):
        if n <= 0 or (clean and (a < clean[-1][0] + clean[-1][2] or b < clean[-1][1] + clean[-1][2])):
            continue
        clean.append((a, b, n))
    return clean


def adrp_page(w, pc):
    imm = ((w >> 29) & 3) | (((w >> 5) & 0x7ffff) << 2)
    if imm & (1 << 20):
        imm -= 1 << 21
    return (pc & ~0xfff) + (imm << 12)


def pair_target(w, page):
    if (w & 0x7f800000) == 0x11000000:
        imm = (w >> 10) & 0xfff
        return page + (imm << 12 if (w >> 22) & 1 else imm)
    size = (w >> 30) & 3
    if (w >> 26) & 1 and (w >> 23) & 1:
        size = 4
    return page + (((w >> 10) & 0xfff) << size)


def data_votes(wa, wb, runs):

    _, paira, adrpa = masks(wa)
    votes = collections.defaultdict(collections.Counter)
    for a, b, n in runs:
        for i in np.nonzero(adrpa[a:a + n])[0].tolist():
            ia, ib = a + i, b + i
            pga, pgb = adrp_page(int(wa[ia]), ia * 4), adrp_page(int(wb[ib]), ib * 4)
            rd = int(wa[ia]) & 31
            for j in range(1, 25):
                if i + j >= n:
                    break
                x, y = int(wa[ia + j]), int(wb[ib + j])
                if paira[ia + j] and ((x >> 5) & 31) == rd:
                    votes[pair_target(x, pga)][pair_target(y, pgb)] += 1
                if (x & 31) == rd or (x & 0xfc000000) == 0x94000000 or (x & 0xfffffc1f) == 0xd63f0000 or x == 0xd65f03c0:
                    break
    return votes



class Map:


    def __init__(self, runs, votes, old_text_end, old_build='', new_build='', sdk_same=False):
        self.runs = list(runs)
        self.starts = [r[0] for r in self.runs]
        self.votes = {o: collections.Counter(v) for o, v in votes.items()}
        self.old_text_end = old_text_end
        self.old_build, self.new_build = old_build, new_build
        self.sdk_same = sdk_same
        self.anchors = sorted(o for o, v in votes.items() if len(v) == 1)

    @classmethod
    def from_alignment(cls, runs, votes, old_text_end, old_build, new_build, sdk_same=False):
        return cls([(a * 4, b * 4, n) for a, b, n in runs], votes, old_text_end, old_build, new_build, sdk_same)

    def code(self, addr):

        i = bisect.bisect_right(self.starts, addr) - 1
        if i < 0:
            return None, 'none'
        o, n, count = self.runs[i]
        if addr < o + 4 * count:
            return n + (addr - o), 'aligned'
        if i + 1 < len(self.runs):
            no, nn, _ = self.runs[i + 1]
            if no - (o + 4 * count) == nn - (n + 4 * count):
                return n + (addr - o), 'position'
        return None, 'none'

    def data(self, addr):
        v = self.votes.get(addr)
        if v:
            best = v.most_common(2)
            if len(best) == 1:
                return best[0][0], 'xref'
            if best[0][1] >= 4 * best[1][1]:
                return best[0][0], 'xref-majority'
            return None, 'ambiguous'
        i = bisect.bisect_right(self.anchors, addr)
        lo = self.anchors[i - 1] if i else None
        hi = self.anchors[i] if i < len(self.anchors) else None
        if lo is not None and hi is not None:
            dlo = next(iter(self.votes[lo])) - lo
            dhi = next(iter(self.votes[hi])) - hi
            if dlo == dhi:
                return addr + dlo, 'between' if hi - lo <= 0x2000 else 'between-far'
        return None, 'none'

    def translate(self, addr, module='main'):
        if module == 'sdk':
            return (addr, 'module unchanged') if self.sdk_same else (None, 'module changed')
        return self.code(addr) if addr < self.old_text_end else self.data(addr)

    def write(self, path):
        with open(path, 'w', encoding='utf-8') as f:
            f.write(f'# gameport map\t{self.old_build}\t{self.new_build}\told_text_end\t{self.old_text_end:#x}'
                    f'\tsdk\t{"same" if self.sdk_same else "changed"}\n')
            for o, n, count in self.runs:
                f.write(f'code\t{o:#x}\t{n:#x}\t{count}\n')
            for o in sorted(self.votes):
                for n, c in sorted(self.votes[o].items()):
                    f.write(f'data\t{o:#x}\t{n:#x}\t{c}\n')

    @classmethod
    def read(cls, path):
        runs, votes = [], collections.defaultdict(collections.Counter)
        with open(path, encoding='utf-8') as f:
            head = f.readline().rstrip('\n').split('\t')
            if head[0] != '# gameport map' or head[3] != 'old_text_end':
                raise SystemExit(f'gameport: {path} is not a map')
            for line in f:
                k, a, b, c = line.rstrip('\n').split('\t')
                if k == 'code':
                    runs.append((int(a, 16), int(b, 16), int(c)))
                else:
                    votes[int(a, 16)][int(b, 16)] = int(c)
        return cls(runs, votes, int(head[4], 16), head[1], head[2], head[5:7] == ['sdk', 'same'])



def verify_data(old, new, a, n, kind, m, ro, rn):

    if kind == 'rodata':
        end = old.img.find(b'\0', a, a + 64)
        size = 16 if end < 0 else max(1, min(16, end - a + 1))
        return old.img[a:a + size] == new.img[n:n + size]
    if kind == 'vtable':
        ok = 0
        for k in range(0, 64, 8):
            x, y = ro.get(a + k), rn.get(n + k)
            if x is None or y is None:
                break
            if x >= old.text_end:
                continue
            mapped, _ = m.code(x)
            if mapped is None:
                continue
            if mapped != y:
                return False
            ok += 1
        return True if ok else None
    return None


def place(table, old, new, old_sdk, new_sdk, m):

    ro, rn = old.relocs(), new.relocs()
    out = []
    for e in table.entries:
        mod, name, a, kind, note = e
        if mod == 'sdk':
            same = old_sdk.sha256 == new_sdk.sha256
            out.append((e, a if same else None, 'module unchanged' if same else 'module changed', None))
            continue
        if a < old.text_end:
            n, how = m.code(a)
            out.append((e, n, how, None))
            continue
        n, how = m.data(a)
        ver = verify_data(old, new, a, n, kind, m, ro, rn) if n is not None else None
        out.append((e, n, how, ver))
    return out


def place_fields(table, old, new, m):


    out = []
    ow, nw = old.words(), new.words()
    for f in table.fields:
        mod, name, off, wit, note = f
        if mod != 'main':
            out.append((f, off, wit, 'module')); continue
        n, how = m.code(wit)
        if n is None:
            out.append((f, None, None, 'witness gone')); continue
        a, b = int(ow[wit // 4]), int(nw[n // 4])
        if game_addrs.insn_imm(a) != off:
            out.append((f, None, None, 'old witness does not encode the offset')); continue
        if game_addrs.insn_shape(a) is None or game_addrs.insn_shape(a) != game_addrs.insn_shape(b):
            out.append((f, None, n, 'witness changed shape')); continue
        out.append((f, game_addrs.insn_imm(b), n, how))
    return out


def status(how, ver):
    if how in ('aligned', 'xref', 'xref-majority', 'between', 'module unchanged') and ver is not False:
        return 'placed'
    if how in ('position', 'between-far') or ver is False:
        return 'review'
    return 'unresolved'


def write_table(path, table, placed, new, new_sdk, version, check_copy=False, fields=()):


    with open(table.path, encoding='utf-8') as f:
        lines = f.read().split('\n')
    head = [x for x in lines if x.startswith('//')]
    width = max(len(e[1]) for e in table.entries)
    lay = dict(table.layout)
    lay.update(TEXT_END=new.text_end, RO_START=new.ro_start, DATA_START=new.data_start, IMAGE_END=new.image_end)
    if 'SDK_TEXT_END' in lay:
        lay['SDK_TEXT_END'] = new_sdk.text_end
    mods = {k: 'sdk' if k.startswith('SDK_') else 'main' for k in lay}
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(head) + '\n')
        f.write(f'GB(main, "{version}", "{new.build_id}")\n')
        for k, v in lay.items():
            f.write(f'GL({mods[k]}, {k}, {v:#x})\n')
        for (mod, name, a, kind, note), n, how, ver in placed:
            st = status(how, ver)
            if (n is None or st != 'placed') and not check_copy:
                f.write(f'GA_UNRESOLVED({mod}, {name}, {a:#x}, {kind}, "{note}")\n')
                continue
            f.write(f'GA({mod}, {name + ",":<{width + 1}} {f"{(a if n is None else n):#x},":<10} {kind + ",":<7} "{note}")\n')
        for (mod, name, off, wit, note), noff, nwit, how in fields:
            if noff is None:
                f.write(f'GF_UNRESOLVED({mod}, {name}, {off:#x}, {wit:#x}, "{note}")\n' if not check_copy else
                        f'GF({mod}, {name}, {off:#x}, {wit:#x}, "{note}")\n')
                continue
            f.write(f'GF({mod}, {name}, {noff:#x}, {nwit:#x}, "{note}")\n')



def disasm(img, a):
    try:
        from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
    except ImportError:
        return struct.unpack_from('<I', img, a)[0].to_bytes(4, 'little').hex()
    for i in Cs(CS_ARCH_ARM64, CS_MODE_ARM).disasm(img[a:a + 4], a):
        return f'{i.mnemonic} {i.op_str}'
    return '?'


def changed_functions(table, old, new, m, limit=8):


    starts = old.func_starts()
    n_old = old.text_end // 4
    matched = np.zeros(n_old + 1, dtype=bool)
    inserted = np.zeros(n_old + 1, dtype=np.int64)
    for i, (o, n, count) in enumerate(m.runs):
        matched[o // 4:o // 4 + count] = True
        if i + 1 < len(m.runs):
            ga = (m.runs[i + 1][0] - o) // 4 - count
            gb = (m.runs[i + 1][1] - n) // 4 - count
            if gb > ga:
                inserted[o // 4 + count] += gb - ga
    cum_un = np.concatenate([[0], np.cumsum(~matched[:n_old])])
    cum_in = np.concatenate([[0], np.cumsum(inserted[:n_old])])
    funcs = collections.defaultdict(list)
    for mod, name, a, kind, note in table.entries:
        if mod != 'main' or a >= old.text_end:
            continue
        i = bisect.bisect_right(starts, a) - 1
        if i < 0:
            continue
        s = starts[i]
        e = starts[i + 1] if i + 1 < len(starts) else old.text_end
        funcs[(s, e)].append(name)
    out = []
    for (s, e), names in sorted(funcs.items()):
        un = int(cum_un[e // 4] - cum_un[s // 4])
        ins = int(cum_in[e // 4] - cum_in[s // 4])
        if not un and not ins:
            continue
        rows = []
        for x in range(s, e, 4):
            if not matched[x // 4] and len(rows) < limit:
                y, how = m.code(x)
                rows.append((x, disasm(old.img, x), disasm(new.img, y) if y is not None else '(no counterpart)'))
        out.append((s, e, un, ins, names, rows))
    return out


def field_moves(old, new, m):


    try:
        from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
        from capstone.arm64 import ARM64_OP_IMM, ARM64_OP_MEM
    except ImportError:
        return {}
    md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
    md.detail = True
    _, pair, _ = masks(old.words())

    def ops(img, a):
        for i in md.disasm(img[a:a + 4], a):
            vals, shape = [], []
            for op in i.operands:
                if op.type == ARM64_OP_IMM:
                    vals.append(op.imm)
                    shape.append('i')
                elif op.type == ARM64_OP_MEM:
                    vals.append(op.mem.disp)
                    shape.append(f'm{op.mem.base}.{op.mem.index}')
                else:
                    shape.append(f'r{op.reg}')
            return i.mnemonic, tuple(shape), vals
        return None, None, None

    moves = collections.defaultdict(list)
    runs = m.runs
    for i in range(len(runs) - 1):
        ea, eb = runs[i][0] + 4 * runs[i][2], runs[i][1] + 4 * runs[i][2]
        ga, gb = runs[i + 1][0] - ea, runs[i + 1][1] - eb
        if ga != gb or ga <= 0 or ga > 4 * 64:
            continue
        for k in range(0, ga, 4):
            a, b = ea + k, eb + k
            if old.img[a:a + 4] == new.img[b:b + 4] or pair[a // 4]:
                continue
            mo, so, vo = ops(old.img, a)
            mn, sn, vn = ops(new.img, b)
            if mo is None or mo != mn or so != sn or vo == vn or mo in ('adrp', 'adr', 'b', 'bl') or mo.startswith(('b.', 'cb', 'tb')):
                continue
            for x, y in zip(vo, vn):
                if x != y:
                    moves[(x, y)].append(a)
    return moves


def host_offsets(values):

    import re
    hits = collections.defaultdict(list)
    host = os.path.join(DM, 'host')
    rx = re.compile(r'\+\s*(0x[0-9a-fA-F]+)\b')
    for fn in sorted(os.listdir(host)):
        if not fn.endswith(('.c', '.h', '.inc')):
            continue
        for ln, line in enumerate(open(os.path.join(host, fn), encoding='utf-8', errors='replace'), 1):
            for x in rx.findall(line):
                v = int(x, 16)
                if v in values and f'dm/host/{fn}:{ln}' not in hits[v]:
                    hits[v].append(f'dm/host/{fn}:{ln}')
    return hits


SHADER_MAP = os.path.join(DM, 'gfx', 'nvn', 'shader_map.tsv')


def shader_keys(old, new, m, out_tsv):



    with open(SHADER_MAP, encoding='utf-8') as f:
        lines = f.read().split('\n')
    found, size = {}, 64
    for line in lines:
        if not line.startswith('main.rodata@'):
            continue
        a = int(line.split('\t', 1)[0].split('@')[1], 16)
        if a in found:
            continue
        block = old.img[a:a + size]
        n, how = m.data(a)
        if n is None or new.img[n:n + size] != block:
            hit = new.img.find(block, new.ro_start, new.data_start)
            again = new.img.find(block, hit + 1, new.data_start) if hit >= 0 else -1
            n, how = (hit, 'bytes') if hit >= 0 and again < 0 else (None, 'not found')
        found[a] = (n, how)
    with open(out_tsv, 'w', encoding='utf-8') as f:
        for line in lines[:-1] if lines and lines[-1] == '' else lines:
            if line.startswith('main.rodata@'):
                key, rest = line.split('\t', 1)
                n = found[int(key.split('@')[1], 16)][0]
                line = (f'main.rodata@{n:x}' if n is not None else 'UNRESOLVED-' + key) + '\t' + rest
            f.write(line + '\n')
    return [(a, n, how) for a, (n, how) in sorted(found.items())]


def rule_check(new_def, new_exefs, work):

    tree = os.path.join(work, 'tree')
    shutil.copytree(HERE, os.path.join(tree, 'dm', 'tools'), ignore=shutil.ignore_patterns('__pycache__', 'tests'))
    os.makedirs(os.path.join(tree, 'dm', 'host'), exist_ok=True)
    shutil.copyfile(new_def, os.path.join(tree, 'dm', 'host', 'game_addrs.def'))
    out = []
    for env, extra in (({'DM_TARGET': 'android', 'DM_RULESET': 'touch-run-v4'}, ['--qa-combat']),
                       ({'DM_TARGET': 'apple', 'DM_RULESET': 'touch-run-v4'}, []),
                       ({'DM_TARGET': 'android', 'DM_RULESET': 'native'}, [])):
        r = subprocess.run([sys.executable, os.path.join(tree, 'dm', 'tools', 'patch_nso.py'), os.path.join(new_exefs, 'main'),
                            os.path.join(work, 'rw'), 'main', '--check'] + extra, env=dict(os.environ, **env),
                           capture_output=True, text=True)
        label = ' '.join(f'{k}={v}' for k, v in env.items()) + (' ' + ' '.join(extra) if extra else '')
        out.append((label, r.returncode, (r.stdout + r.stderr).strip().split('\n')))
    return out


def graphics_slots(old, new, new_def, work):

    res = []
    for prog, table in ((old, None), (new, new_def)):
        tree = os.path.join(work, 'slots-' + ('new' if table else 'old'))
        shutil.copytree(os.path.join(DM, 'gfx', 'tools'), os.path.join(tree, 'dm', 'gfx', 'tools'), ignore=shutil.ignore_patterns('__pycache__'))
        shutil.copytree(HERE, os.path.join(tree, 'dm', 'tools'), ignore=shutil.ignore_patterns('__pycache__', 'tests'))
        os.makedirs(os.path.join(tree, 'dm', 'host'))
        shutil.copyfile(table or game_addrs.DEF, os.path.join(tree, 'dm', 'host', 'game_addrs.def'))
        img = os.path.join(tree, 'main.img')
        with open(img, 'wb') as f:
            f.write(prog.img)
        tsv = os.path.join(tree, 'slots.tsv')
        r = subprocess.run([sys.executable, os.path.join(tree, 'dm', 'gfx', 'tools', 'nvn_slots.py'), img, tsv], capture_output=True, text=True)
        os.unlink(img)
        if r.returncode:
            return None
        rows = [x.split('\t') for x in open(tsv, encoding='utf-8').read().split('\n')[2:] if x]
        res.append(({x[0] for x in rows}, {x[0] for x in rows if x[2] != '0'}, len(rows)))
    (no, uo, co), (nn, un, cn) = res
    return no == nn, uo == un, co, cn, sorted(no ^ nn), sorted(uo ^ un)


def manifest_diff(old_romfs, new_romfs):

    def man(root):
        d = {}
        with open(os.path.join(root, 'data', 'build_manifest.psv'), encoding='utf-8', errors='replace') as f:
            next(f)
            for line in f:
                if line.startswith('#'):
                    continue
                c = line.rstrip('\n').split('|')
                if len(c) >= 11:
                    d[c[9]] = c[4]
        return d
    o, n = man(old_romfs), man(new_romfs)
    out = collections.defaultdict(lambda: [0, 0, 0])

    def cat(k):
        p = k.split(':', 1)[-1].split('/')
        return '/'.join(p[:4]) if p[1:2] == ['hd'] else '/'.join(p[:3]), os.path.splitext(k)[1].lower()
    for k in set(n) - set(o):
        out[cat(k)][0] += 1
    for k in set(o) - set(n):
        out[cat(k)][1] += 1
    for k in set(o) & set(n):
        if o[k] != n[k]:
            out[cat(k)][2] += 1
    return dict(out), len(o), len(n)



def port(args):
    table = game_addrs.Table(args.table)
    progs = {}
    for side, exefs in (('old', args.old), ('new', args.new)):
        for mod in MODULES:
            p = os.path.join(exefs, mod)
            if not os.path.isfile(p):
                raise SystemExit(f'gameport: {p} missing')
            progs[side, mod] = Program(p)
    old, new = progs['old', 'main'], progs['new', 'main']
    if old.build_id != table.game.build_id:
        raise SystemExit(f'gameport: the old program has build id {old.build_id}, the table belongs to {table.game.build_id}')
    os.makedirs(args.out, exist_ok=True)
    wa, wb = old.words(), new.words()
    runs = align_code(wa, wb)
    m = Map.from_alignment(runs, data_votes(wa, wb, runs), old.text_end, old.build_id, new.build_id,
                           progs['old', 'sdk'].sha256 == progs['new', 'sdk'].sha256)
    m.write(os.path.join(args.out, 'map.tsv'))
    placed = place(table, old, new, progs['old', 'sdk'], progs['new', 'sdk'], m)
    fields = place_fields(table, old, new, m)
    write_table(os.path.join(args.out, 'game_addrs.def'), table, placed, new, progs['new', 'sdk'], args.version, fields=fields)
    work = tempfile.mkdtemp(prefix='gameport-')
    try:
        check_def = os.path.join(work, 'check.def')
        write_table(check_def, table, placed, new, progs['new', 'sdk'], args.version, check_copy=True, fields=fields)
        rules = rule_check(check_def, args.new, work)
        slots = graphics_slots(old, new, check_def, work)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    shaders = shader_keys(old, new, m, os.path.join(args.out, 'shader_map.tsv'))
    funcs = changed_functions(table, old, new, m)
    moves = field_moves(old, new, m)
    files = manifest_diff(args.romfs[0], args.romfs[1]) if args.romfs else None
    report(args, table, progs, runs, m, placed, rules, funcs, moves, slots, files, shaders, fields)
    n = collections.Counter(status(how, ver) for _, _, how, ver in placed)
    print(f'gameport: {len(table.entries)} entries: {n["placed"]} placed, {n["review"]} to review, {n["unresolved"]} unresolved (both written as GA_UNRESOLVED); '
          f'report {os.path.join(args.out, "report.md")}')
    return 0


def report(args, table, progs, runs, m, placed, rules, funcs, moves, slots, files, shaders, fields=()):
    old, new = progs['old', 'main'], progs['new', 'main']
    matched = sum(n for _, _, n in runs)
    L = [f'# Game address port {table.game.version} -> {args.version}', '',
         '## Programs', '', '| Module | Old build id | New build id | Same file |', '|---|---|---|---|']
    for mod in MODULES:
        a, b = progs['old', mod], progs['new', mod]
        L.append(f'| {mod} | {a.build_id} | {b.build_id} | {"yes" if a.sha256 == b.sha256 else "no"} |')
    L += ['', f'Code alignment: {len(runs)} runs, {matched} of {len(old.words())} old instructions matched '
          f'({100 * matched / max(1, len(old.words())):.2f} %); {len(m.votes)} data addresses seen through instruction pairs.', '']
    by = collections.Counter((status(how, ver), how) for _, _, how, ver in placed)
    L += ['## Table entries', '', '| Result | How | Entries |', '|---|---|---|']
    for (st, how), c in sorted(by.items()):
        L.append(f'| {st} | {how} | {c} |')
    todo = [(e, n, how, ver) for e, n, how, ver in placed if status(how, ver) != 'placed']
    if todo:
        L += ['', 'Entries that need a look:', '', '| Name | Old | New | How | Kind | Note |', '|---|---|---|---|---|---|']
        for (mod, name, a, kind, note), n, how, ver in todo:
            L.append(f'| {name} | {a:#x} | {"-" if n is None else hex(n)} | {how}{" (bytes differ)" if ver is False else ""} | {kind} | {note} |')
    deltas = collections.Counter((n - e[2]) for e, n, how, ver in placed if n is not None)
    L += ['', f'Deltas: {len(deltas)} distinct, most common ' + ', '.join(f'{d:+#x} ({c})' for d, c in deltas.most_common(5)), '']
    if fields:
        L += ['## Fields (GF, read from their witness instruction)', '', '| Name | Old offset | New offset | Old witness | New witness | How |',
              '|---|---|---|---|---|---|']
        for (mod, name, off, wit, note), noff, nwit, how in fields:
            L.append(f'| {name} | {off:#x} | {"UNRESOLVED" if noff is None else hex(noff)} | {wit:#x} | {"-" if nwit is None else hex(nwit)} | {how} |')
        L.append('')
    L += ['## Rule sites (rewriter --check with the new table)', '',
          'Unresolved entries keep their old address for this check only. The two displaced instructions that dm/host/entry.S replays',
          'are the original words of the entry cuts, so a passing check covers them.', '']
    for label, rc, lines in rules:
        L.append(f'- `{label}`: exit {rc}; ' + (lines[-1] if lines else ''))
        for x in lines[:-1][:40]:
            L.append(f'  - {x}')
    L += ['', '## Changed functions around table code addresses', '',
          f'{len(funcs)} functions changed. Gates and cuts in them that read registers: check register roles.', '']
    for s, e, un, ins, names, rows in funcs:
        L.append(f'- `{s:#x}`..`{e:#x}`: {un} changed, {ins} inserted instructions; holds {", ".join(names)}')
        for x, a_txt, b_txt in rows:
            L.append(f'  - `{x:#x}` `{a_txt}` -> `{b_txt}`')
    L += ['', '## Field offsets', '',
          'Instruction pairs in changed blocks of equal length that differ only in an immediate or a memory offset. A neighbour of a',
          'moved field is often read through the moved base plus a small constant, so this list names the objects whose layout',
          'changed (by the functions the moves occur in); it does not clear single values. Audit every host access to each object.', '',
          '| Old | New | Places | First places |', '|---|---|---|---|']
    big = sorted(((k, v) for k, v in moves.items() if abs(k[0]) >= 0x10 or abs(k[1]) >= 0x10), key=lambda kv: -len(kv[1]))
    starts = old.func_starts()
    with open(os.path.join(args.out, 'field-moves.tsv'), 'w', encoding='utf-8') as f:
        f.write('old\tnew\tplaces\tfunctions\tinstructions\n')
        for (x, y), places in sorted(moves.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            fs = sorted({starts[bisect.bisect_right(starts, p) - 1] for p in places if starts and p >= starts[0]})
            f.write(f'{x:#x}\t{y:#x}\t{len(places)}\t{" ".join(hex(q) for q in fs)}\t{" ".join(hex(p) for p in places)}\n')
    L.insert(len(L) - 2, f'All {len(moves)} moves with their functions: field-moves.tsv. The 120 most frequent from 0x10:')
    L.insert(len(L) - 2, '')
    for (x, y), places in big[:120]:
        L.append(f'| {x:#x} | {y:#x} | {len(places)} | {", ".join(hex(p) for p in places[:4])} |')
    hits = host_offsets({x for (x, y) in moves if x >= 0x100})
    L += ['', f'First screen: host offset literals (from 0x100) whose value occurs as an old moved value: {len(hits)} values.', '']
    for v, where in sorted(hits.items()):
        L.append(f'- `{v:#x}`: {len(where)} places, e.g. {", ".join(where[:3])}')
    L += ['', '## Embedded shader blocks (dm/gfx/nvn/shader_map.tsv keys, rewritten into shader_map.tsv here)', '']
    for a, n, how in shaders:
        L.append(f'- `{a:#x}` -> `{"-" if n is None else hex(n)}` ({how}{"" if n is None else f", {n - a:+#x}"})')
    L += ['', '## Graphics slots (nvn_slots.py)', '']
    if slots is None:
        L.append('nvn_slots.py failed on one of the programs.')
    else:
        same_n, same_u, co, cn, dn, du = slots
        L.append(f'Slot names {"identical" if same_n else "differ"} ({co} / {cn}); used set {"identical" if same_u else "differs"}.'
                 + (f' Names: {dn[:20]}; used: {du[:20]}' if dn or du else ''))
    if files is not None:
        cats, no, nn = files
        L += ['', '## Game files (build manifests)', '', f'{no} / {nn} files.', '', '| Category | Ext | Added | Removed | Changed |', '|---|---|---|---|---|']
        for (c, ext), (a, r, ch) in sorted(cats.items()):
            L.append(f'| {c} | {ext} | {a} | {r} | {ch} |')
    with open(os.path.join(args.out, 'report.md'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(L) + '\n')


def gaddr(args):
    m = Map.read(args.map)
    for q in args.address:
        a = int(q, 16)
        n, how = m.translate(a, args.module)
        print(f'{a:#x}\t{"-" if n is None else hex(n)}\t{how}')
    return 0


def main(argv):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    a = sub.add_parser('port')
    a.add_argument('old')
    a.add_argument('new')
    a.add_argument('out')
    a.add_argument('--version', required=True)
    a.add_argument('--romfs', nargs=2, metavar=('OLD', 'NEW'))
    a.add_argument('--table', default=game_addrs.DEF)
    g = sub.add_parser('gaddr')
    g.add_argument('map')
    g.add_argument('--module', choices=('main', 'sdk'), default='main')
    g.add_argument('address', nargs='+')
    args = p.parse_args(argv)
    return port(args) if args.cmd == 'port' else gaddr(args)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
