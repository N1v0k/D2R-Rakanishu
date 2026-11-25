#!/usr/bin/env python3












import struct, sys
LOADER = 0x1739290
img, out = sys.argv[1], sys.argv[2]
d = open(img, 'rb').read()
TEXT_END = 0x1b786a0
N = TEXT_END // 4
w = struct.unpack_from('<%dI' % N, d, 0)

def adrp(x, pc):
    imm = ((x >> 29) & 3) | (((x >> 5) & 0x7ffff) << 2)
    if imm & (1 << 20): imm -= 1 << 21
    return (pc & ~0xfff) + (imm << 12)
def cstr(a): return d[a:d.index(b'\0', a)].decode()



i, reg, cur, slot = LOADER // 4, {}, None, {}
while w[i] != 0xd65f03c0:
    x, pc = w[i], i * 4
    if x & 0x9f000000 == 0x90000000: reg[x & 31] = adrp(x, pc)
    elif x & 0xffc00000 == 0x91000000 and (x & 31) == 1: cur = cstr(reg[(x >> 5) & 31] + ((x >> 10) & 0xfff))
    elif x & 0xffc00000 == 0xf9000000 and (x & 31) == 0: slot[reg[(x >> 5) & 31] + ((x >> 10) & 0xfff) * 8] = cur
    i += 1
offs = {a & 0xfff for a in slot}

def writes(x, r):

    op0 = (x >> 25) & 0xf
    if x & 0xfc000000 == 0x94000000 or x & 0xfffffc1f == 0xd63f0000: return r <= 18
    if op0 in (0b1000, 0b1001, 0b0101, 0b1101): return (x & 31) == r
    if op0 & 0b0101 == 0b0100:
        L = (x >> 22) & 1
        if (x >> 27) & 7 == 0b101:
            wb = (x >> 23) & 3 in (1, 3)
            return (L and ((x & 31) == r or ((x >> 10) & 31) == r)) or (wb and ((x >> 5) & 31) == r)
        if (x >> 27) & 7 == 0b111 and (x >> 24) & 3 == 0:
            wb = (x >> 21) & 1 == 0 and (x >> 10) & 3 in (1, 3)
            return (L and (x & 31) == r) or (wb and ((x >> 5) & 31) == r)
        if (x >> 27) & 7 == 0b111: return bool(L) and (x & 31) == r
        if (x >> 27) & 7 == 0b011: return (x & 31) == r
    return False



uses = {a: [] for a in slot}
for i in range(N):
    x = w[i]
    if x & 0xffc00000 != 0xf9400000: continue
    off = ((x >> 10) & 0xfff) * 8
    rn = (x >> 5) & 31
    if off not in offs or rn == 31: continue
    for j in range(i - 1, max(i - 4000, -1), -1):
        y = w[j]
        if y == 0xd65f03c0: break
        if writes(y, rn):
            if y & 0x9f000000 == 0x90000000 and (y & 31) == rn and adrp(y, j * 4) + off in uses:
                uses[adrp(y, j * 4) + off].append(i * 4)
            break
with open(out, 'w') as f:
    f.write('# kind: evidence  main NVN 取址槽与静态调用点（nvn_slots.py）\nname\tslot\tnuses\tfirst_use\n')
    for a in sorted(slot):
        u = uses[a]; f.write('%s\t%x\t%d\t%s\n' % (slot[a], a, len(u), '%x' % u[0] if u else '-'))
if len(sys.argv) > 3: open(sys.argv[3], 'w').write(''.join(slot[a] + '\n' for a in sorted(slot)))
used = sum(1 for a in slot if uses[a])
print('槽 %d，有调用点 %d，调用点 %d' % (len(slot), used, sum(len(v) for v in uses.values())))
