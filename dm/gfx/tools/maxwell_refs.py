

















import struct

TEX_BASE, SAMP_BASE, UBO_FIRST, UBO_LAST = 0x168, 0x568, 3, 16
SSBO_BASE = {'vs': 0x110, 'cs': 0x310, 'ps': 0x510}

def instrs(code, graphics):
    off = 0x80 if graphics and code[:4] == bytes.fromhex('78563412') else 0
    out = []
    for k in range(off, len(code) - 7, 8):
        if ((k - off) // 8) % 4 == 0: continue
        out.append(struct.unpack_from('<Q', code, k)[0])
    return out

def match(w, pat):
    for i, ch in enumerate(pat):
        if ch != '-' and int(ch) != (w >> (63 - i)) & 1: return False
    return True

def cbuf_alu(ins):

    return {((w >> 34) & 0x1f, ((w >> 20) & 0x3fff) * 4) for w in ins if (w >> 60) == 0x4 and w}

def ldc(ins):

    return {((w >> 36) & 0x1f, (w >> 20) & 0xffff) for w in ins if match(w, '1110111110010---')}

def bindings(code, stage):
    ins = instrs(code, stage != 'cs')
    refs = cbuf_alu(ins) | ldc(ins)
    ubo = {i - UBO_FIRST for i, _ in refs if UBO_FIRST <= i <= UBO_LAST}
    c2 = {o for i, o in refs if i == 2}


    for w in ins:
        for pat in ('110000----111---', '110010----111---', '11011100--11----', '1101111100------', '1101-00---------', '1101-01---------'):
            if match(w, pat): c2.add(((w >> 36) & 0x1fff) * 4); break
    tex = {(o - TEX_BASE) // 8 for o in c2 if TEX_BASE <= o < SAMP_BASE and (o - TEX_BASE) % 8 == 0}
    samp = {(o - SAMP_BASE) // 8 for o in c2 if o >= SAMP_BASE and (o - SAMP_BASE) % 8 == 0}
    base = SSBO_BASE.get(stage)
    ssbo = {(o - base) // 16 for i, o in refs if i == 0 and base and o >= base and (o - base) % 16 == 0 and (o - base) // 16 < 16} if base else set()
    drv = sorted(o for o in c2 if o < TEX_BASE)
    return {'ubo': ubo, 'tex': tex, 'samp': samp, 'ssbo': ssbo, 'c2drv': drv}

def vs_inputs(code):

    ins = instrs(code, True)
    return {((w >> 20) & 0x3ff) - 0x80 >> 4 for w in ins if match(w, '1110111111011---') and 0x80 <= ((w >> 20) & 0x3ff) < 0x280}

def sph_stage(code):

    if code[:4] != bytes.fromhex('78563412'): return None
    return {1: 'vs', 5: 'ps'}.get((struct.unpack_from('<I', code, 0x30)[0] >> 10) & 0xf)
