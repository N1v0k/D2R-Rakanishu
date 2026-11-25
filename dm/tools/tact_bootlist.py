#!/usr/bin/env python3
















import sys, os, mmap, struct, zlib, hashlib, collections
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
chk, out = sys.argv[1], sys.argv[2]
R = (sys.argv[3] if len(sys.argv) > 3 else os.path.join(DM_ROOT, 'data/switch/3.3.94012/Update/RomFS')) + '/data'
NSO = sys.argv[4] if len(sys.argv) > 4 else os.path.join(DM_ROOT, 'data/switch/3.3.94012/Update/ExeFS/main')
BUILD_ID = open(NSO, 'rb').read(0x50)[0x40:0x50].hex()
maps = {}
def mm(fn):
    if fn not in maps: fh = open(f'{R}/{fn}', 'rb'); maps[fn] = (fh, mmap.mmap(fh.fileno(), 0, access=mmap.ACCESS_READ))
    return maps[fn][1]
idx = collections.defaultdict(list)
for l in open(f'{R}/build_manifest.psv', encoding='utf-8'):
    if l.startswith(('FileId', '#')): continue
    f = l.rstrip('\n').split('|')
    if len(f) < 10: continue
    fn = f'{int(f[2]):02d}-{int(f[3]):08d}.data'; off = int(f[6][22:32], 16); es = int(f[7]); enc = f[8]; m = mm(fn)
    if enc == 'z': idx[(m[off:off + 16].hex(), es)].append((fn, off)); continue
    if enc.startswith('b') and m[off:off + 4] == b'BLTE':
        hs = struct.unpack_from('>I', m, off + 4)[0]
        if hs == 0: continue
        nc = struct.unpack_from('>I', m, off + 8)[0] & 0xffffff; p = off + hs
        for k in range(nc):
            ce, cd = struct.unpack_from('>II', m, off + 12 + k * 24)
            if m[p:p + 1] == b'Z': idx[(m[p + 1:p + 17].hex(), ce - 1)].append((fn, p + 1))
            p += ce
rows = []; seen = set(); miss = amb = 0
for l in open(chk):
    q = l.rstrip('\n').split('\t')
    if len(q) < 4: continue
    if len(q[0]) != 32 or len(q[3]) != 32: continue
    h, n, dl, md = q[0], int(q[1]), int(q[2]), q[3]; th = q[4] if len(q) > 4 and q[4].startswith('0x') else '0'
    cands = idx.get((h, n), [])
    if len(cands) > 1: amb += 1
    hit = None
    for fn, off in cands:
        if len(cands) == 1 or hashlib.md5(zlib.decompress(mm(fn)[off:off + n])).hexdigest() == md: hit = (fn, off); break
    if not hit: miss += 1; continue
    if hit in seen: continue
    seen.add(hit)
    rows.append(f'{hit[0]}\t{hit[1]}\t{n}\t{dl}\t{h}\t{th}')
with open(out, 'w') as f: f.write(f'# BUILD_ID\t{BUILD_ID}\n' + '\n'.join(rows) + '\n')
print(f'启动块清单（main 构建号 {BUILD_ID}）{len(rows)} 块（撞键消歧 {amb}，查不到 {miss}）：压缩 {sum(int(r.split()[2]) for r in rows)/1048576:.1f} MiB → 解码 {sum(int(r.split()[3]) for r in rows)/1048576:.1f} MiB → {out}')
