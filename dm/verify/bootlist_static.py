#!/usr/bin/env python3









import os, sys
GAME_VERSION = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../PRODUCT'), encoding='utf-8').readline().split('-')[0]
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
lst = sys.argv[1]
rom = sys.argv[2] if len(sys.argv) > 2 else os.path.join(DM_ROOT, 'data/switch', GAME_VERSION, 'Update/RomFS/data')
bad, seen, n, fhs = [], set(), 0, {}
holes = {}
def hole_info(p):
    if p not in holes:
        hp = p + '.holes'; holes[p] = None
        if os.path.isfile(hp):
            ls = open(hp).read().split('\n'); holes[p] = (int(ls[0].split()[1]), [(int(x.split()[0]), int(x.split()[0]) + int(x.split()[1])) for x in ls[1:] if x.strip()])
    return holes[p]
def phys(p, off, ln):
    h = hole_info(p)
    if h is None: return off
    cut = 0
    for a, b in h[1]:
        if b <= off: cut += b - a
        elif a < off + ln: return None
    return off - cut
for i, line in enumerate(open(lst), 1):
    if line.startswith('#') or not line.strip():
        continue
    n += 1
    f, off, clen, _dlen, head = line.split('\t')[:5]
    off, clen = int(off), int(clen)
    p = os.path.join(rom, f)
    if (f, off) in seen:
        bad.append(f'第 {i} 行 重复 {f}@{off}'); continue
    seen.add((f, off))
    if f not in fhs:
        fhs[f] = open(p, 'rb') if os.path.isfile(p) else None
    fh = fhs[f]
    if fh is None:
        bad.append(f'第 {i} 行 数据文件不存在 {p}'); continue
    h = hole_info(p)
    if off + clen > (h[0] if h else os.fstat(fh.fileno()).st_size):
        bad.append(f'第 {i} 行 越界 {f}@{off}+{clen}'); continue
    po = phys(p, off, clen)
    if po is None:
        bad.append(f'第 {i} 行 落在剔掉的零段里 {f}@{off}+{clen}'); continue
    fh.seek(po)
    if fh.read(len(head) // 2).hex() != head:
        bad.append(f'第 {i} 行 开头字节不符 {f}@{off}')
if n == 0:
    bad.append('清单 0 块')
for b in bad[:5]:
    print(b)
print(f'BOOTSTATIC {"OK" if not bad else "RED"} {n} 块，不符 {len(bad)} 处（{os.path.basename(lst)}，资源包 {rom}）')
sys.exit(1 if bad else 0)
