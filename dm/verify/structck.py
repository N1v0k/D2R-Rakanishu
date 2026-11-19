



































import sys, os, re, struct, subprocess, tempfile
sys.path.insert(0, os.path.dirname(__file__))
import nsoimg
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')

LLVM = os.environ.get('LLVM_BIN', '/opt/homebrew/opt/llvm@23/bin')
out_dir, mod, nso = sys.argv[1:4]
orig = nsoimg.load(nso)
h = nsoimg.header(out_dir, mod)
PO, TE = h['PATCH_OFF'], h['TEXT_END']
ANDROID = h.get('TARGET', 0) == 1
KEYREAD = 'tpidr_r' if ANDROID else 'tpidrro_r'
pat = open(f'{out_dir}/{mod}.patched.bin', 'rb').read()
W = lambda b, o: struct.unpack_from('<I', b, o)[0]
fails = []
def bad(msg):
    fails.append(msg)
    if len(fails) <= 30: print('FAIL', msg)

def llvm_dis(blob, base):

    with tempfile.TemporaryDirectory() as td:
        b, e = os.path.join(td, 'x.bin'), os.path.join(td, 'x.elf')
        open(b, 'wb').write(blob)
        subprocess.run([f'{LLVM}/llvm-objcopy', '-I', 'binary', '-O', 'elf64-littleaarch64',
                        '--rename-section', '.data=.text,code', b, e], check=True)


        od = subprocess.Popen([f'{LLVM}/llvm-objdump', '-d', '--no-show-raw-insn', e], stdout=subprocess.PIPE)
        r = subprocess.run(['/usr/bin/grep', '-iE', r'[xw]18|tpidr|svc|cnt[pv]ct'], stdin=od.stdout, capture_output=True, text=True)
        od.stdout.close(); assert od.wait() == 0
    out = {}
    for line in r.stdout.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+(.*)$', line)
        if m: out[base + int(m.group(1), 16)] = m.group(2).split('//')[0].split('<')[0].strip().lower()
    return out

RX18 = re.compile(r'\b[xw]18\b')
def sysk(t):
    if re.match(r'svc\s', t): return 'svc'
    if re.match(r'mrs\s', t) and 'tpidr_el0' in t: return 'tpidr_r'
    if re.match(r'msr\s', t) and 'tpidr_el0' in t: return 'tpidr_w'
    if re.match(r'mrs\s', t) and 'tpidrro_el0' in t: return 'tpidrro_r'
    if re.match(r'mrs\s', t) and 'cntpct_el0' in t: return 'cntpct'
    if re.match(r'mrs\s', t) and 'cntvct_el0' in t: return 'cntvct'
    return None


sites = {}; wins = []; jumps = 0
for l in open(f'{out_dir}/{mod}.sites.tsv'):
    f = l.split(); a = int(f[0], 16)
    if f[1] == 'J': jumps += 1; continue
    if f[1] == 'W': wins.append((a, int(f[2], 16))); sites[a] = 'W'
    else: sites[a] = f[1]
inwin = set(o for lo, hi in wins for o in range(lo, hi, 4))



cuts = []; bid = None
for l in open(f'{out_dir}/{mod}.cuts.tsv'):
    if l.startswith('# BUILD_ID'): bid = l.split()[-1]; continue
    f = l.rstrip('\n').split('\t')
    if len(f) == 4: f = f[:3] + ['全'] + f[3:]
    off, ob, nb, scope, why = f[:5]
    if scope not in ('全', '仅宿主'): bad(f'剪口 {off} 作用域不认识：{scope}')
    cuts.append((int(off, 16), bytes.fromhex(ob), bytes.fromhex(nb), why))
if bid != open(nso, 'rb').read(0x50)[0x40:0x50].hex(): bad(f'cuts.tsv 构建号 {bid} 与 NSO 不符')
cutb = set()
for off, ob, nb, why in cuts:
    if orig[off:off + len(ob)] != ob: bad(f'剪口 {off:#x} 原字节不符')
    for k in range(0, len(nb), 4):
        if int.from_bytes(nb[k:k + 4], 'little') & 0xFFE0001F == 0xD4000001: continue
        if pat[off + k:off + k + len(nb[k:k + 4])] != nb[k:k + 4]: bad(f'剪口 {off:#x}+{k} 改写映像不是新字节'); break
    if not why.strip() or len(ob) != len(nb): bad(f'剪口 {off:#x} 理由为空或长度不等')
    cutb |= set(range(off, off + len(ob)))


_hostcall = {}
_reg = {int(v, 16) for v in re.findall(r'SVC_HOST_\w+\s*=\s*(0x[0-9a-fA-F]+)', open(os.path.join(os.path.dirname(__file__), '..', 'host', 'hle.h')).read())}
for off, ob, nb, why in cuts:
    for k in range(0, len(nb), 4):
        w = int.from_bytes(nb[k:k + 4], 'little')
        if w & 0xFFE0001F == 0xD4000001:
            n = (w >> 5) & 0xFFFF; a = off + k
            if n < 0x80: bad(f'剪口 {off:#x} 引入 svc #{n:#x}：0x80 以下是 Horizon 号段，剪口不得占用')
            elif n not in _reg: bad(f'剪口 {off:#x} 引入 svc #{n:#x}：未在 hle.h 宿主调用号登记表')
            elif sites.get(a) != 'svc': bad(f'剪口 {off:#x} 引入的 svc #{n:#x} 没被改写成宿主门（sites.tsv 无 {a:#x} svc）')
            _hostcall[a] = n










_hreg = {}
for l in open(os.path.join(os.path.dirname(__file__), '..', 'host', 'hle.h'), encoding='utf-8'):
    m = re.match(r'//\s+(0x[0-9a-fA-F]+)\s.*?\s(0x[0-9a-fA-F]+)（([^）]*)', l)
    if m: _hreg[int(m[1], 16)] = (int(m[2], 16), m[3])
def _bl(w, o): return o + 4 * (((w & 0x3FFFFFF) ^ 0x2000000) - 0x2000000) if w & 0xFC000000 == 0x94000000 else None
for a, n in _hostcall.items():
    if n not in _hreg: bad(f'宿主调用 svc #{n:#x}：hle.h 登记表没有对应行'); continue
    roff, form = _hreg[n]
    if mod != 'main' or roff != a: bad(f'宿主调用 svc #{n:#x} 在 {mod} {a:#x}，登记表写 main {roff:#x}'); continue
    nbw = next(c[2] for c in cuts if c[0] <= a < c[0] + len(c[2]))
    mt = re.search(r'\bbl (0x[0-9a-fA-F]+)', form)
    if mt:
        t = _bl(W(orig, a), a)
        if t is None: bad(f'宿主调用 svc #{n:#x} {a:#x}：登记为调用点形，原指令 {W(orig, a):#010x} 不是 bl')
        elif t != int(mt[1], 16): bad(f'宿主调用 svc #{n:#x} {a:#x}：原 bl 目标 {t:#x} ≠ 登记 {mt[1]}')
        if len(nbw) != 4: bad(f'宿主调用 svc #{n:#x} {a:#x}：调用点形剪口只许换这一条指令')
    elif '入口' in form:
        if a not in set(nsoimg.func_starts(orig, TE)): bad(f'宿主调用 svc #{n:#x} {a:#x}：登记为入口形，不是 eh_frame 函数入口')
        if nbw[4:8] != (0xD65F03C0).to_bytes(4, 'little'): bad(f'宿主调用 svc #{n:#x} {a:#x}：入口形新字节不是 svc ; ret')
    else: bad(f'宿主调用 svc #{n:#x}：登记表形态认不出（须含「bl 0x目标」或「入口」）：{form}')
if cutb & {a + i for a in list(sites) + list(inwin) if a not in _hostcall for i in range(4)}: bad('剪口与改写点 / 窗口重叠')
ncut_code = sum(1 for c in cuts if c[0] < TE); ncut_data = len(cuts) - ncut_code
obs = []
obs_p = f'{out_dir}/{mod}.obs.tsv'
if os.path.exists(obs_p):
    for l in open(obs_p, encoding='utf-8'):
        if l.startswith('# BUILD_ID'):
            if l.split()[-1] != open(nso, 'rb').read(0x50)[0x40:0x50].hex(): bad('obs.tsv 构建号与 NSO 不符')
            continue
        off, oid, ow, why = l.rstrip('\n').split('\t', 3); obs.append((int(off, 16), int(oid), int(ow, 16), why))
obs_heads = {o for o, *_ in obs}
params = []
par_p = f'{out_dir}/{mod}.params.tsv'
if os.path.exists(par_p):
    for l in open(par_p, encoding='utf-8'):
        if l.startswith('# BUILD_ID'):
            if l.split()[-1] != open(nso, 'rb').read(0x50)[0x40:0x50].hex(): bad('params.tsv 构建号与 NSO 不符')
            continue
        a, b, r, k, why = l.rstrip('\n').split('\t', 4)
        if not why.strip(): bad(f'参数门 {a} 理由为空')
        params.append((int(a, 16), int(b, 16), int(r.lstrip('x')), k))
par_words = {o for a, b, r, k in params for o in (a, b)}



nd = []
for blk in range(TE, len(orig), 4096):
    e = min(blk + 4096, len(orig))
    if pat[blk:e] != orig[blk:e]: nd += [o for o in range(blk, e) if pat[o] != orig[o] and o not in cutb]
if nd: bad(f'非 text 段与原映像不同（剪口以外）{len(nd)} 字节 例 {[hex(x) for x in nd[:5]]}')
if any(pat[len(orig):PO]): bad('映像尾到补丁区之间非 0')



diff = set()
for blk in range(0, TE & ~3, 4096):
    e = min(blk + 4096, TE & ~3)
    if orig[blk:e] != pat[blk:e]: diff |= {o for o in range(blk, e, 4) if W(orig, o) != W(pat, o)}
heads = set(sites)
cutw = {o & ~3 for o in cutb if o < TE}
_exp = (heads | obs_heads | par_words) - cutw; _got = diff - cutw
if _got != _exp: bad(f'text 改动与改写点不一致：多改 {len(_got - _exp)} 漏改 {len(_exp - _got)} 例 {[hex(x) for x in sorted(_got ^ _exp)[:5]]}')
def btarget(o):
    w = W(pat, o)
    if w & 0xFC000000 != 0x14000000: return None
    imm = w & 0x3FFFFFF
    if imm & 0x2000000: imm -= 0x4000000
    return o + 4 * imm
nb = [a for a in heads | obs_heads if not (btarget(a) is not None and PO <= btarget(a) < len(pat))]
if nb: bad(f'改写点头不是跳进补丁区的 B：{len(nb)} 例 {[hex(x) for x in nb[:5]]}')



def cached_dis(blob):

    import hashlib, json
    cd = os.environ.get('DM_VCACHE', os.path.join(DM_ROOT, 'build/cache/dm/verify'))
    ver = os.path.basename(os.path.dirname(LLVM.rstrip('/')))
    p = os.path.join(cd, f'{mod}.{hashlib.sha256(blob).hexdigest()[:16]}.{ver}.json')
    if os.path.exists(p): return {int(k): v for k, v in json.load(open(p)).items()}
    d = llvm_dis(blob, 0)
    os.makedirs(cd, exist_ok=True); tmp = f'{p}.new.{os.getpid()}'; json.dump(d, open(tmp, 'w')); os.replace(tmp, p)
    return d
D = cached_dis(orig[:TE])
ind = {}
for o, t in D.items():
    if RX18.search(t): ind[o] = 'x18'
    k = sysk(t)
    if k: ind[o] = k
miss = sorted(o for o in ind if o not in sites and o not in inwin)
if miss: bad(f'独立反汇编有、改写点没覆盖：{len(miss)} 例 {[(hex(x), D[x]) for x in miss[:5]]}')
extra = sorted(a for a, k in sites.items() if k != 'W' and a not in ind and a not in _hostcall)
if extra: bad(f'改写点不在独立集合里：{len(extra)} 例 {[(hex(x), D.get(x)) for x in extra[:5]]}')
kmis = sorted(a for a, k in sites.items() if k != 'W' and a in ind and ind[a] != k)
if kmis: bad(f'改写点种类与独立判读不同：{len(kmis)} 例 {[(hex(x), sites[x], D[x]) for x in kmis[:5]]}')
nwin_ref = sum(1 for o in ind if o in inwin)



P = llvm_dis(pat[PO:], PO)
left = sorted(o for o, t in P.items() if RX18.search(t) or sysk(t) not in (None, KEYREAD))
if left: bad(f'补丁区残留 x18/系统寄存器/svc：{len(left)} 例 {[(hex(x), P[x]) for x in left[:5]]}')



KEY = {k[4:-4]: v for k, v in h.items() if k.startswith('KEY_') and k.endswith('_IDX')}
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
_md = Cs(CS_ARCH_ARM64, CS_MODE_ARM); _md.detail = True
def tpseq(q):

    if not q or q[0].mnemonic != 'mrs': return None
    if not ANDROID: return q if 'tpidrro_el0' in q[0].op_str else None
    if 'tpidr_el0' not in q[0].op_str or len(q) < 2 or q[1].mnemonic != 'ldur' or q[1].operands[0].reg != q[0].operands[0].reg \
            or q[1].operands[1].mem.base != q[0].operands[0].reg or q[1].operands[1].mem.disp != -8: return None
    return q[:1] + q[2:]
def keyseq(at):
    q = tpseq(list(_md.disasm(pat[at:at + 24], at)))
    if not q or len(q) < 5 \
            or q[1].mnemonic != 'adrp' or q[1].operands[1].imm != h['KEY_PAGE_OFF'] \
            or q[2].mnemonic != 'ldr' or q[2].operands[1].mem.base != q[1].operands[0].reg \
            or q[3].mnemonic != 'add' or q[3].operands[0].reg != q[0].operands[0].reg \
            or q[3].operands[1].reg != q[0].operands[0].reg:
        return None
    d = q[2].operands[1].mem.disp
    return (q[3].address + 4, d // 8) if d % 8 == 0 and 0 <= d // 8 < 8 else None
def paramseq(at, r):
    q = list(_md.disasm(pat[at:at + 44], at))
    tmp = 17 if r == 16 else 16
    rn = lambda ins, i: ins.reg_name(ins.operands[i].reg)
    if ANDROID:
        t = tpseq(q[1:])
        if len(q) < 9 or not t or q[0].mnemonic != 'str' or rn(q[0], 0) != f'x{tmp}' or not q[0].writeback \
                or q[0].operands[1].mem.base != capstone.arm64_const.ARM64_REG_SP or q[0].operands[1].mem.disp != -16 \
                or rn(t[0], 0) != f'x{r}' or t[1].mnemonic != 'adrp' or t[1].operands[1].imm != h['KEY_PAGE_OFF'] \
                or t[2].mnemonic != 'ldr' or t[2].operands[1].mem.base != t[1].operands[0].reg or t[3].mnemonic != 'add' \
                or t[3].operands[0].reg != t[0].operands[0].reg or t[3].operands[1].reg != t[0].operands[0].reg \
                or t[4].mnemonic != 'ldr' or t[4].operands[0].reg != t[0].operands[0].reg or t[4].operands[1].mem.base != t[3].operands[0].reg \
                or t[5].mnemonic != 'ldr' or rn(t[5], 0) != f'x{tmp}' or not getattr(t[5], 'post_index', False) \
                or t[5].operands[1].mem.base != capstone.arm64_const.ARM64_REG_SP or t[6].mnemonic != 'b':
            return None
        return (t[5].address + 4, t[2].operands[1].mem.disp // 8)
    if len(q) < 8 or q[0].mnemonic not in ('stur', 'str') or rn(q[0], 0) != f'x{tmp}' \
            or q[0].operands[1].mem.base != capstone.arm64_const.ARM64_REG_SP or q[0].operands[1].mem.disp not in (-16, -24) \
            or q[1].mnemonic != 'mrs' or rn(q[1], 0) != f'x{r}' \
            or 'tpidrro_el0' not in q[1].op_str or q[2].mnemonic != 'adrp' \
            or q[2].operands[1].imm != h['KEY_PAGE_OFF'] or q[3].mnemonic != 'ldr' \
            or q[3].operands[1].mem.base != q[2].operands[0].reg or q[4].mnemonic != 'add' \
            or q[4].operands[0].reg != q[1].operands[0].reg or q[4].operands[1].reg != q[1].operands[0].reg \
            or q[5].mnemonic != 'ldr' or q[5].operands[0].reg != q[1].operands[0].reg \
            or q[5].operands[1].mem.base != q[4].operands[0].reg or q[6].mnemonic not in ('ldur', 'ldr') \
            or rn(q[6], 0) != f'x{tmp}' \
            or q[6].operands[1].mem.base != capstone.arm64_const.ARM64_REG_SP \
            or q[6].operands[1].mem.disp != q[0].operands[1].mem.disp or q[7].mnemonic != 'b':
        return None
    d = q[3].operands[1].mem.disp
    return (q[6].address + 4, d // 8)
gates = {}
for l in open(f'{out_dir}/{mod}.gates.tsv'):
    r, k, arg, oa = l.split(); gates[int(r, 16)] = (k, int(arg), int(oa, 16))
def B(frm, to): return 0x14000000 | (((to - frm) >> 2) & 0x3FFFFFF)


LDR_X29_SP = 0xF94003FD
gsites = sorted(a for a, k in sites.items() if k in ('svc', 'cntpct', 'cntvct'))
seen = set()
for a in gsites:
    s = btarget(a); ow = W(orig, a) if a not in _hostcall else (0xD4000001 | _hostcall[a] << 5)
    ks = keyseq(s + 4)
    if W(pat, s) != 0xA9BF7BFD or not ks or ks[1] != KEY.get('KGATE') \
            or W(pat, ks[0]) & 0xFFC00000 != 0xF9400000 or W(pat, ks[0] + 4) != LDR_X29_SP or W(pat, ks[0] + 8) != 0xD63F03C0 \
            or W(pat, ks[0] + 12) != 0xA8C17BFD or W(pat, ks[0] + 16) != B(ks[0] + 16, a + 4):
        bad(f'门块不符键页模板 {a:#x}'); continue
    arg = (ow >> 5) & 0xFFFF if sites[a] == 'svc' else ow & 31
    g = gates.get(ks[0] + 12)
    if g != (sites[a], arg, a): bad(f'gates.tsv 不符 {a:#x}：表 {g} 期望 {(sites[a], arg, a)}')
    seen.add(ks[0] + 12)


import bisect as _bs
_starts = set(nsoimg.func_starts(orig, TE))
_busy = {a + i for a in list(sites) + list(inwin) for i in range(4)} | cutb
for off, oid, ow, why in obs:
    if W(orig, off) != ow: bad(f'观测门 {off:#x} 原指令字不符：映像 {W(orig, off):08x} 登记 {ow:08x}'); continue
    ins = next(_md.disasm(struct.pack('<I', ow), off), None)
    m = ins.mnemonic if ins else '?'
    if ins is None or m in ('adr', 'adrp', 'b', 'bl', 'br', 'blr', 'ret', 'cbz', 'cbnz', 'tbz', 'tbnz', 'svc', 'mrs', 'msr', 'eret') or m.startswith('b.') \
       or (m.startswith('ldr') and '[' not in ins.op_str) or RX18.search(ins.op_str or ''):
        bad(f'观测门 {off:#x} 原指令不可搬：{m} {ins.op_str if ins else ""}')




    if set(range(off, off + 4)) & _busy: bad(f'观测门 {off:#x} 与改写点 / 窗口 / 剪口重叠')
    s_ = btarget(off)
    if s_ is None or not (PO <= s_ < len(pat)): bad(f'观测门头 {off:#x} 不是跳进补丁区的 B'); continue
    ks = keyseq(s_ + 4)
    if W(pat, s_) != 0xA9BF7BFD or not ks or ks[1] != KEY.get('KGATE') \
            or W(pat, ks[0]) & 0xFFC00000 != 0xF9400000 or W(pat, ks[0] + 4) != LDR_X29_SP or W(pat, ks[0] + 8) != 0xD63F03C0 \
            or W(pat, ks[0] + 12) != 0xA8C17BFD or W(pat, ks[0] + 16) != ow \
            or W(pat, ks[0] + 20) != B(ks[0] + 20, off + 4):
        bad(f'观测门块不符键页模板 {off:#x}'); continue
    if gates.get(ks[0] + 12) != ('obs', oid, off): bad(f'观测门行不符 {off:#x}：表 {gates.get(ks[0] + 12)}')
    seen.add(ks[0] + 12)






nbranch = 0
for n, (roff, form) in _hreg.items():
    if not form.startswith('分支门'): continue
    cut = next((c for c in cuts if c[0] == roff), None)
    if cut is None: continue
    mt, mb = re.search(r'跳 (0x[0-9a-fA-F]+)', form), re.search(r'回 (0x[0-9a-fA-F]+)', form)
    if not mt or not mb or int(mb[1], 16) != roff + 4: bad(f'分支门 svc #{n:#x}：登记表形态缺跳转目标或回点 ≠ 原处+4：{form}'); continue
    tgt = int(mt[1], 16); s_ = btarget(roff); ks = keyseq(s_ + 8) if s_ is not None else None
    if s_ is None or not (PO <= s_ < len(pat)) or W(pat, s_) != 0xA9BF7FE8 or W(pat, s_ + 4) != 0xA9BF7BFD or not ks or ks[1] != KEY.get('KGATE') \
            or W(pat, ks[0]) & 0xFFC00000 != 0xF9400000 or W(pat, ks[0] + 4) != LDR_X29_SP or W(pat, ks[0] + 8) != 0xD63F03C0 or W(pat, ks[0] + 12) != 0xA8C17BFD \
            or W(pat, ks[0] + 16) != 0x34000068 or W(pat, ks[0] + 20) != 0xA8C17FE8 or W(pat, ks[0] + 24) != B(ks[0] + 24, tgt) \
            or W(pat, ks[0] + 28) != 0xA8C17FE8 or W(pat, ks[0] + 32) != W(orig, roff) or W(pat, ks[0] + 36) != B(ks[0] + 36, roff + 4):
        bad(f'分支门 svc #{n:#x} {roff:#x}：块不符模板'); continue
    if gates.get(ks[0] + 12) != ('svc', n, roff): bad(f'分支门 svc #{n:#x} gates.tsv 行不符：{gates.get(ks[0] + 12)}')
    seen.add(ks[0] + 12); nbranch += 1
if set(gates) != seen: bad(f'gates.tsv 有多余行 {len(set(gates) - seen)}')



_base = {'K18', 'KTLS', 'KTLR', 'KGATE'}; _extra = set(KEY) - _base
if set(KEY) != {'K18', 'KTLS', 'KTLR', 'KGATE', 'KRES0', 'KRES1', 'KASPECT', 'KASPECTUI'} \
        or not {k for *_, k in params} <= set(KEY) or set(KEY.values()) != set(range(8)):
    bad(f'表头键页下标不合规：{KEY}（须含八个键且下标恰为 0..7）')
kv = {v: k for k, v in KEY.items()}; nk = {}
for o in range(PO, len(pat) - 4, 4):
    w = W(pat, o)
    if w & 0xFFFFFFE0 != (0xD53BD040 if ANDROID else 0xD53BD060): continue
    n = w & 31; ks = keyseq(o); k = kv.get(ks[1]) if ks else None
    if not ks or k is None:
        bad(f'补丁区 {o:#x} 读线程槽基址后不是键页间接槽序列'); continue
    nk[k] = nk.get(k, 0) + 1
expected = {}
for kind in sites.values():
    key = {'x18': 'K18', 'W': 'K18', 'tpidr_r': 'KTLS', 'tpidr_w': 'KTLS', 'tpidrro_r': 'KTLR'}.get(kind)
    if key: expected[key] = expected.get(key, 0) + 1
expected['KGATE'] = len(gsites) + len(obs) + nbranch
for _, _, _, key in params: expected[key] = expected.get(key, 0) + 1
if any(nk.get(k, 0) != v for k, v in expected.items()) or any(k not in expected and v for k, v in nk.items()):
    bad(f'键页槽访问计数不符：期望 {expected} 实际 {nk}')









par_const = {}
if params:
    import capstone
    _cs = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_ARM); _cs.detail = True
    _tg = set()
    for o in range(0, TE & ~3, 4):
        w = W(orig, o)
        if w & 0x7C000000 == 0x14000000: d = ((w & 0x3FFFFFF) ^ 0x2000000) - 0x2000000; _tg.add(o + 4 * d)
        elif w & 0xFF000010 == 0x54000000 or w & 0x7E000000 == 0x34000000: d = (((w >> 5) & 0x7FFFF) ^ 0x40000) - 0x40000; _tg.add(o + 4 * d)
        elif w & 0x7E000000 == 0x36000000: d = (((w >> 5) & 0x3FFF) ^ 0x2000) - 0x2000; _tg.add(o + 4 * d)
    _fs = set(nsoimg.func_starts(orig, TE))
    _busy8 = {a for a in sites} | set(inwin) | obs_heads | {o & ~3 for o in cutb}
    for a, b, r, k in params:
        wa, wb = W(orig, a), W(orig, b)




        w64 = wa & 0xFFE00000 == 0xD2800000 and wb & 0xFFE00000 == 0xF2C00000
        w32 = wa & 0xFFE00000 == 0x52800000 and wb & 0xFFE00000 == 0x72A00000
        if not (w64 or w32) or wa & 31 != r or wb & 31 != r:
            bad(f'参数门 {a:#x}/{b:#x}：原指令 {wa:#010x}/{wb:#010x} 不是 movz/movk 造常数对（64 位 lsl #32 或 32 位 lsl #16）写 x{r}')
        if not a < b: bad(f'参数门 {a:#x}/{b:#x} 顺序不对')
        if k not in KEY: bad(f'参数门 {a:#x} 键 {k} 未在表头登记'); continue
        s = btarget(a); ks = paramseq(s, r) if s is not None else None
        ok = ks and ks[1] == KEY[k] and W(pat, b) == 0xD503201F
        if not ok: bad(f'参数门 {a:#x}/{b:#x}：键页间接模板不符')
        for o in range(a + 4, b, 4):
            ins = next(_cs.disasm(orig[o:o + 4], o), None)
            if ins is None: bad(f'参数门 {a:#x}：中间 {o:#x} 解不出'); continue
            rr, ww = ins.regs_access()
            names = {ins.reg_name(x) for x in rr + ww}
            if f'x{r}' in names or f'w{r}' in names: bad(f'参数门 {a:#x}：中间 {o:#x} {ins.mnemonic} {ins.op_str} 读写 x{r}')
        bt = sorted(t for t in _tg if a < t <= b) + sorted(t for t in _fs if a < t <= b)
        if bt: bad(f'参数门 {a:#x}..{b:#x}：区间内有分支目标 / 函数入口 {[hex(x) for x in bt[:4]]}')
        if {a, b} & _busy8: bad(f'参数门 {a:#x}/{b:#x} 与改写点 / 窗口 / 观测门 / 剪口重叠')
        par_const[(a, k)] = ((wb >> 5) & 0xFFFF) << (16 if w32 else 32) | ((wa >> 5) & 0xFFFF)
    _kc = {}
    for (a, k), v in par_const.items(): _kc.setdefault(k, set()).add(v)
    for k, vs in _kc.items():
        if len(vs) != 1: bad(f'参数门键 {k} 对应多个原常数 {[hex(v) for v in vs]}（同一槽给不出唯一原值）')
    print('参数门 ' + ', '.join(f'{a:#x}={k}:原值 {v:#x}' for (a, k), v in par_const.items()))






    _fsl = sorted(_fs)
    def _rn(ins, x): return re.sub(r'^[wx]', '', ins.reg_name(x)) if re.match(r'^[wx]\d+$', ins.reg_name(x) or '') else None
    _donef = set()
    for a, b, r, k in params:
        import bisect as _bi
        i = _bi.bisect_right(_fsl, a); f0 = _fsl[i - 1]; f1 = _fsl[i] if i < len(_fsl) else TE
        if f0 in _donef: continue
        _donef.add(f0)
        seeds = {(pa, str(pr)) for pa, pb, pr, pk in params if f0 <= pa < f1}
        taint, const = set(), {}
        for o in range(f0, f1, 4):
            for pa, pr in seeds:
                if o == pa: taint.add(pr)
            if any(o == pa for pa, _ in seeds): continue
            ins = next(_cs.disasm(pat[o:o + 4], o), None)
            if ins is None: continue
            rr, ww = ins.regs_access(); rd = {x for x in (_rn(ins, q) for q in rr) if x}; wr = {x for x in (_rn(ins, q) for q in ww) if x}
            m = ins.mnemonic
            if m in ('and', 'ands', 'bic', 'bics') and rd & taint:
                imm = any(op.type == capstone.arm64.ARM64_OP_IMM for op in ins.operands)
                if imm or (rd - taint) & set(const): bad(f'参数门下游常数掩码 {o:#x}：{m} {ins.op_str}（函数 {f0:#x}，被参数寄存器污染的值再被常数掩码）')
            if m in ('mov', 'csel', 'csinc', 'csinv', 'csneg') and rd & taint: taint |= wr; [const.pop(x, None) for x in wr]
            else:
                taint -= wr
                isimm = m in ('mov', 'movz') and len(ins.operands) == 2 and ins.operands[1].type == capstone.arm64.ARM64_OP_IMM
                for x in wr:
                    if isimm: const[x] = ins.operands[1].imm
                    else: const.pop(x, None)



import capstone
def _writes(ins):
    _, ww = ins.regs_access()
    return {'x29' if ins.reg_name(r) == 'fp' else 'x30' if ins.reg_name(r) == 'lr' else re.sub(r'^[wx]', 'x', ins.reg_name(r))
            for r in ww if ins.reg_name(r) not in ('sp', 'pc')}
def _sp_regs(ins, cur):

    mem = next((op for op in ins.operands if op.type == capstone.arm64_const.ARM64_OP_MEM and ins.reg_name(op.mem.base) == 'sp'), None)
    if mem is None or not ins.mnemonic.startswith(('ld', 'st')): return None, cur
    if getattr(ins, 'post_index', False):
        addr = cur; cur += next((op.imm for op in ins.operands if op.type == capstone.arm64_const.ARM64_OP_IMM), 0)
    elif ins.writeback: cur += mem.mem.disp; addr = cur
    else: addr = cur + mem.mem.disp
    out = []
    for k, op in enumerate(x for x in ins.operands[:2] if x.type == capstone.arm64_const.ARM64_OP_REG):
        name = ins.reg_name(op.reg); size = 4 if name.startswith('w') else 8
        out.append(('x29' if name == 'fp' else 'x30' if name == 'lr' else re.sub(r'^[wx]', 'x', name), addr + k * size))
    return out, cur
def _block(start):
    out = []
    for ins in _md.disasm(pat[start:start + 256], start):
        out.append(ins)
        if ins.mnemonic == 'b':
            t = ins.operands[0].imm
            if not (PO <= t < len(pat)): break
    return out
static_entries = [(a, set()) for a in sites]
for a, b, r, _ in params:
    static_entries.append((a, {f'x{r}'}))
static_seen = 0
for a, allowed in static_entries:
    s = btarget(a)
    if s is None: continue
    block = _block(s); static_seen += 1
    if not allowed:
        end = next((hi for lo, hi in wins if lo == a), a + 4)
        for ins in _md.disasm(orig[a:end], a): allowed |= _writes(ins)
        if next(_md.disasm(orig[a:a + 4], a), None).mnemonic in ('stp', 'ldp'):
            allowed.update(('x0', 'x1'))
    saved = {}
    restored = {}
    loads = set(); cur = 0
    for i, ins in enumerate(block):
        regs, cur = _sp_regs(ins, cur)
        if regs is None: continue
        if ins.mnemonic.startswith('ld'): loads.add(i)
        for name, addr in regs: (restored if ins.mnemonic.startswith('ld') else saved).setdefault(name, []).append((addr, i))
    writes = set().union(*(_writes(ins) for ins in block)) if block else set()
    for reg in sorted(writes - allowed):
        wi = [i for i, ins in enumerate(block) if reg in _writes(ins) and not (i > 0 and i in loads)]
        dead = any(ins.mnemonic in ('br', 'blr') and ins.operands and ins.reg_name(ins.operands[0].reg) in (reg, reg[1:]) for ins in block)
        ok = dead or any(sd == rd and si < min(wi, default=10**9) and ri > max(wi, default=-1)
                 for sd, si in saved.get(reg, []) for rd, ri in restored.get(reg, []))
        if not ok: bad(f'临时寄存器 {a:#x}：块写 {reg}，原目的寄存器外且无同偏移存取配对')
print(f'STATIC {mod} scanned={static_seen} violations={sum(1 for x in fails if x.startswith("临时寄存器"))}')









_heads = {o for o in range(0, TE & ~3, 4) if W(pat, o) & 0xFC000000 == 0x14000000 and btarget(o) is not None and btarget(o) >= PO}
_wend = dict(wins)
def _bt(w, o):
    if w & 0x7C000000 == 0x14000000: return o + 4 * (((w & 0x3FFFFFF) ^ 0x2000000) - 0x2000000)
    if w & 0xFF000010 == 0x54000000 or w & 0x7E000000 == 0x34000000: return o + 4 * ((((w >> 5) & 0x7FFFF) ^ 0x40000) - 0x40000)
    if w & 0x7E000000 == 0x36000000: return o + 4 * ((((w >> 5) & 0x3FFF) ^ 0x2000) - 0x2000)
    return None
def _exits(start):
    ex, seen, st = set(), set(), [start]
    while st:
        o = st.pop()
        while o not in seen and o + 4 <= len(pat):
            seen.add(o); w = W(pat, o)
            if w & 0xFC000000 == 0x14000000:
                t = _bt(w, o)
                if t < PO: ex.add(t); break
                o = t; continue
            if w & 0xFC000000 == 0x94000000: o += 4; continue
            if w == 0xD65F03C0 or w & 0xFFFFFC1F == 0xD61F0000: break
            t = _bt(w, o)
            if t is not None:
                if t < PO: ex.add(t)
                else: st.append(t)
            o += 4
    return ex
nexit = 0; _cutoff = {c[0] for c in cuts}; _cutex = []
for a in sorted(_heads):
    if a in _wend:
        seg = range(a, _wend[a], 4); legal = {_wend[a]}
    else:
        e = a + 4
        while e < TE and W(pat, e) != W(orig, e) and e not in _heads: e += 4
        seg = range(a, e, 4); legal = {a + 4 * k for k in range(1, len(seg) + 1)}
    legal |= {t for o in seg for t in [_bt(W(orig, o), o)] if t is not None}
    for t in _exits(btarget(a)):
        nexit += 1
        if t not in legal and a in _cutoff: _cutex.append(f'{a:#x}→{t:#x}'); continue
        if t not in legal: bad(f'跳板出口 {a:#x} → {t:#x}：既不是 {a + 4:#x} / 吸收段尾 / 窗口尾，也不是段内原指令的分支目标（越过未改原指令或别的跳板头）')
print(f'EXITS {mod} 跳板头={len(_heads)} 出口={nexit} 已登记剪口改控制流出口={_cutex}')

cnt = {}
if _hostcall: print(f'剪口引入宿主调用门 {len(_hostcall)} 个：' + ', '.join(f"{a:#x}=svc#{n:#x}" for a, n in _hostcall.items()))
for k in sites.values(): cnt[k] = cnt.get(k, 0) + 1
print(f'STRUCT {mod} {"OK" if not fails else "FAIL"} 改写点={len(sites)} {cnt} 跳转表={jumps} 窗口内x18引用={nwin_ref} '
      f'已登记剪口={len(cuts)}(代码{ncut_code}/数据{ncut_data}) 观测门={len(obs)} 参数门={len(params)} '
      f'独立分母={len(ind)} text改动字={len(diff)} 门={len(gsites)} 槽访问={nk} 失败项={len(fails)}')
sys.exit(1 if fails else 0)
