

















import re
import sys, os, struct

def files(d):
    d = os.path.join(d, '0')
    return {os.path.relpath(os.path.join(r, f), d) for r, _, fs in os.walk(d) for f in fs}

def d2s_sum(b):

    z = bytearray(b); z[0x0c:0x10] = b'\0\0\0\0'; s = 0
    for c in z: s = (((s << 1) | (s >> 31)) + c) & 0xFFFFFFFF
    return s

import json
A, B = sys.argv[1], sys.argv[2]
opt = dict(x.split('=', 1) for x in sys.argv[3:] if x.startswith('--'))
def tsrc(side):
    if f'--{side}-time' in opt: v = int(opt[f'--{side}-time']); return (v, v, f'固定值 {v}')
    if f'--{side}-meta' in opt: m = json.load(open(opt[f'--{side}-meta'])); return (m['wall_start'], m['wall_end'], f'墙钟区间 [{m["wall_start"]}, {m["wall_end"]}]')
    return None
TS = {'A': tsrc('a'), 'B': tsrc('b')}
def check_time(lab, f, v):
    t = TS[lab]
    if t is None: print(f'  {f} {lab} 时间 {v} 未核（缺 --{lab.lower()}-meta / --{lab.lower()}-time）'); return 1
    if not t[0] <= v <= t[1]: print(f'  {f} {lab} 时间 {v} 不在{t[2]}内'); return 1
    return 0
fa, fb = files(A), files(B)
print(f'射程：A {len(fa)} 个文件，B {len(fb)} 个；.d2s A {sum(f.endswith(".d2s") for f in fa)} / B {sum(f.endswith(".d2s") for f in fb)}')
if not any(f.endswith('.d2s') for f in fa) or not any(f.endswith('.d2s') for f in fb): print('SAVECMP 射程不足'); sys.exit(3)
bad = 0
for f in sorted(fa ^ fb): print(f'  只在 {"A" if f in fa else "B"}：{f}'); bad += 1
for f in sorted(fa & fb):
    a = open(os.path.join(A, '0', f), 'rb').read(); b = open(os.path.join(B, '0', f), 'rb').read()
    if f.endswith('.d2s'):
        for lab, x in (('A', a), ('B', b)):
            if len(x) >= 16 and struct.unpack_from('<I', x, 0x0c)[0] != d2s_sum(x): print(f'  {f} {lab} 校验和不自洽'); bad += 1
        a2, b2 = bytearray(a), bytearray(b); a2[0x0c:0x10] = b2[0x0c:0x10] = b'\0\0\0\0'
        if a[4] == 0x69 and b[4] == 0x69 and a[0x20:0x24] != b[0x20:0x24]:
            for lab, x in (('A', a), ('B', b)): bad += check_time(lab, f, struct.unpack_from('<I', x, 0x20)[0])
        if a[4] == 0x69 and b[4] == 0x69: a2[0x20:0x24] = b2[0x20:0x24] = b'\0\0\0\0'
        a, b = bytes(a2), bytes(b2)
    if f.endswith('.d2s.meta'):
        if a != b:
            for lab, x in (('A', a), ('B', b)):
                mt = re.search(rb'"last_modified_time":(\d+)', x)
                if mt: bad += check_time(lab, f, int(mt.group(1)) // 10**9)
        a, b = (re.sub(rb'"last_modified_time":\d+', b'"last_modified_time":0', x) for x in (a, b))
    if a != b:
        bad += 1
        offs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
        print(f'  不同：{f} 长 {len(a)}/{len(b)}，差 {len(offs) + abs(len(a) - len(b))} 字节，首处 {hex(offs[0]) if offs else hex(min(len(a), len(b)))}'
              + (f'（A {a[offs[0]:offs[0] + 8].hex()} / B {b[offs[0]:offs[0] + 8].hex()}）' if offs else ''))
print('SAVECMP 一致（校验和各自自洽、墙钟时间核过来源、其余逐字节全同）' if not bad else f'SAVECMP 不一致：{bad} 处'); sys.exit(1 if bad else 0)
