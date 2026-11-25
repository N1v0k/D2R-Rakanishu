













import os, sys
out, dst, names = sys.argv[1], sys.argv[2], sys.argv[3:]
PG = 0x4000
def r16(x): return (x + PG - 1) & ~(PG - 1)
asm = ['// 由 dm/ios/tools/gen_embed.py 生成，勿手改']
flags = []
for n in names:
    hdr = dict(l[2:].split() for l in open(f'{out}/{n}.fixups.tsv'))
    te, po = int(hdr['TEXT_END'], 16), int(hdr['PATCH_OFF'], 16)
    ko, ks = int(hdr['KEY_PAGE_OFF'], 16), int(hdr['KEY_PAGE_SIZE'], 16)
    bin_ = os.path.abspath(f'{out}/{n}.patched.bin'); sz = os.path.getsize(bin_)
    a, end = r16(te), r16(sz)
    assert po % PG == 0 and a <= ko and ko + ks <= po and ko % PG == 0 and ks == PG
    for sect, lo, hi in ((f'__TEXT,__dm{n}x0', 0, a), (f'__DM{n}W,__img', a, po), (f'__TEXT,__dm{n}x1', po, end)):
        assert all(len(x) <= 16 for x in sect.split(','))
        fill = min(hi, sz) - lo
        asm += [f'    .section {sect}', '    .p2align 14', f'    .incbin "{bin_}", {lo:#x}, {fill:#x}']
        if hi - lo > fill: asm.append(f'    .space {hi - lo - fill:#x}')
        asm.append('    .p2align 14')
    flags.append(f'-Wl,-segprot,__DM{n}W,rw,rw')
open(dst, 'w').write('\n'.join(asm) + '\n')
print(' '.join(flags))
