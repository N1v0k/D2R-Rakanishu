#!/usr/bin/env python3


























import os, re, sys, json, struct, hashlib, shutil, subprocess, tempfile, collections, concurrent.futures as cf

HERE = os.path.dirname(os.path.abspath(__file__))
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
OPT, VAL = (os.environ.get(k, d) for k, d in (('SPIRV_OPT', 'spirv-opt'), ('SPIRV_VAL', 'spirv-val')))
STAGE_NO = {'vs': 0, 'hs': 1, 'ds': 2, 'gs': 3, 'ps': 4, 'cs': 5}

TEX_BASE, SAMP_BASE, IMG_BASE = 0x168, 0x568, 0x120


def compare(ref, got):

    st = collections.Counter(); bad = []
    for f in sorted(os.listdir(ref)):
        if not f.endswith('.xgs'): continue
        p = os.path.join(got, f)
        if not os.path.exists(p): st['本次没有'] += 1; bad.append((f, '本次没有')); continue
        a, b = open(os.path.join(ref, f), 'rb').read(), open(p, 'rb').read()
        if a == b: st['一致'] += 1; continue
        def parts(d):
            m, v, s, nb, ni, nc, ns, _ = struct.unpack_from('<8I', d, 0); o = 32 + 32 * nb + 4 * ni + nc
            return d[:o], d[o:o + ns]
        (ha, sa), (hb, sb) = parts(a), parts(b)
        why = ('绑定表 / 头不同' if ha != hb else '') + ('，' if ha != hb and sa != sb else '') + ('SPIR-V 不同' if sa != sb else '')
        st['不一致'] += 1; bad.append((f, why))
    print(f'参考 {sum(st.values())} 个：' + '，'.join(f'{k} {v}' for k, v in st.items()))
    for why, n in collections.Counter(w for _, w in bad).most_common(): print(f'  {why}：{n}（例：{next(f for f, w in bad if w == why)}）')
    return st['一致'] == sum(st.values())


if len(sys.argv) > 1 and sys.argv[1] == '--compare':
    sys.exit(0 if compare(sys.argv[2], sys.argv[3]) else 1)

dm_dir, out = sys.argv[1:3]; want = set(sys.argv[3:])
sys.path.insert(0, HERE); import dmsh_facts, pack_game_shaders
XLATE = os.environ.get('XLATE', f'{DM_ROOT}/build/gfx/dmsh_xlate/dmsh_xlate')
MAIN = os.environ.get('MAIN', f'{DM_ROOT}/data/switch/3.3.94012/Update/ExeFS/main')
MAP = os.path.join(HERE, '../nvn/shader_map.tsv')
FACTS = dmsh_facts.load()
if not FACTS: sys.exit('【红】没有作答表 nvn/shader_facts.tsv')



def records(d):
    r = [m.start() for m in re.finditer(b'BTCDBTCD', d)]
    return [d[x:(r[k + 1] if k + 1 < len(r) else len(d))] for k, x in enumerate(r)]

loc, stage_of = {}, {}
for ln in open(MAP):
    c = ln.rstrip('\n').split('\t')
    if len(c) > 4 and c[0].endswith('.bin'): loc.setdefault(c[3], (c[0], int(c[1])))
    if len(c) > 4 and c[0].startswith('main.rodata@'):
        loc.setdefault(c[3], (c[0], int(c[0].split('@')[1], 16))); stage_of[c[3]] = {'顶点': 'vs', '像素': 'ps'}[c[2]]

def main_rec(mi, o):
    g = mi.rfind(bytes.fromhex('78563412'), o - 0x1000, o)
    if g < 0 or mi[o:o + 4] != bytes.fromhex('34127698'): sys.exit(f'【红】main 映像 @{o:#x} 不是内嵌着色器控制段（MAIN={MAIN}）')
    return bytes(0x38) + mi[g:o + 0x880]

def job_for(key, name, rec, w):
    ci = rec.find(bytes.fromhex('34127698')); code = rec[0x38:ci]; stage = stage_of.get(key, name[:2])
    c1o, c1t = struct.unpack_from('<2I', rec, ci + 0x700)
    c1 = rec[0x38 + c1o:0x38 + c1t]
    if stage != 'cs': code = code[0x30:]
    cp = os.path.join(w, key + '.bin'); open(cp, 'wb').write(code)
    j = {'key': key, 'stage': stage, 'code': cp, 'tex': {}, 'img': {}, 'attr': {}, 'fout': {}, 'c1': c1.hex(),
         'debug': os.environ.get('XLATE_DEBUG', '1') != '0', 'reduced': os.environ.get('XLATE_REDUCED', '1') == '1'}
    if key in FACTS: j.update(FACTS[key])
    return j, c1



def insts(words):
    i = 5
    while i < len(words):
        n = words[i] >> 16
        yield i, words[i] & 0xffff, words[i + 1:i + n]
        i += n

def rewrite(spv, stage):

    w = list(struct.unpack('<%dI' % (len(spv) // 4), spv))
    for i, op, a in insts(w):
        if op == 71 and a[1] == 33: w[i + 3] = a[2] * 6 + STAGE_NO[stage]
    return struct.pack('<%dI' % len(w), *w)

def add_format_caps(spv):




    w = list(struct.unpack('<%dI' % (len(spv) // 4), spv))
    fmt, ty, have, need = {}, {}, set(), []
    for i, op, a in insts(w):
        if op == 17: have.add(a[0])
        elif op == 25: fmt[a[0]] = a[7]
        elif op == 61: ty[a[1]] = a[0]
    for i, op, a in insts(w):
        if op in (98, 99):
            img = a[2] if op == 98 else a[0]
            cap = 55 if op == 98 else 56
            if fmt.get(ty.get(img)) == 0 and cap not in have and cap not in need: need.append(cap)
    for cap in need: w[5:5] = [(2 << 16) | 17, cap]
    return struct.pack('<%dI' % len(w), *w)

def spirv_info(spv):

    w = struct.unpack('<%dI' % (len(spv) // 4), spv)
    caps, exts, deco, types, ptr, var, names, consts = [], [], collections.defaultdict(dict), {}, {}, {}, {}, {}
    for i, op, a in insts(w):
        if op == 17: caps.append(a[0])
        elif op == 43 and len(a) >= 3: consts[a[1]] = a[2]
        elif op == 10: exts.append(bytes(struct.pack('<%dI' % len(a), *a)).split(b'\0')[0].decode())
        elif op == 5: names[a[0]] = bytes(struct.pack('<%dI' % (len(a) - 1), *a[1:])).split(b'\0')[0].decode()
        elif op == 71: deco[a[0]][a[1]] = a[2] if len(a) > 2 else 1
        elif op in (19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30): types[a[0]] = (op, a[1:])
        elif op == 32: ptr[a[0]] = (a[1], a[2])
        elif op == 59: var[a[1]] = (a[0], a[2])
    return caps, exts, deco, types, ptr, var, names, consts

def image_desc(types, tid):
    op, a = types[tid]
    while op in (27, 28, 29): tid = a[0]; op, a = types[tid]
    st, dim, depth, arrayed, ms, sampled, fmt = a[0], a[1], a[2], a[3], a[4], a[5], a[6]
    sop, sa = types[st]
    scalar = {'kind': 'float'} if sop == 22 else {'kind': 'int', 'signed': bool(sa[1])}
    return {'format': fmt, 'dim': dim, 'arrayed': arrayed, 'multisampled': ms, 'depth': depth, 'scalar': scalar}

def decode_tex(slots, h, image):
    tb = (slots & 0xffff) if slots >= 0 else 2; sb = ((slots >> 16) - 1) if slots >= 0 and (slots >> 16) else tb
    o0, o1, ty = h & 0x3fff, (h >> 14) & 0x3fff, h >> 28
    def slot(word, base, cb): return (word * 4 - base) // 8 if cb == 2 and word * 4 >= base and (word * 4 - base) % 8 == 0 else None
    words = [o0] + ([o1] if ty else [])
    if image: return next((slot(x, IMG_BASE, tb) for x in words if IMG_BASE <= x * 4 < TEX_BASE and slot(x, IMG_BASE, tb) is not None), None), None, ty
    t = next((slot(x, TEX_BASE, tb) for x in words if slot(x, TEX_BASE, tb) is not None and x * 4 < SAMP_BASE), None)
    s = next((slot(x, SAMP_BASE, sb) for x in words if x * 4 >= SAMP_BASE and slot(x, SAMP_BASE, sb) is not None), None)
    return t, s, ty

def export(key, stage, c1, spv, res):

    caps, exts, deco, types, ptr, var, names, consts = spirv_info(spv)
    if exts: raise ValueError('残留扩展 ' + ','.join(exts))
    by_bind = {}
    for vid, (pt, sc) in var.items():
        if 33 in deco[vid] and sc in (0, 2, 12): by_bind[deco[vid][33]] = (vid, ptr[pt][1], sc)
    base_no = STAGE_NO[stage]; descs = []
    def vbind(b): return b * 6 + base_no
    def block_bytes(tid):
        op, a = types[tid]
        if op == 30 and a and types[a[0]][0] == 28: return 16 * consts.get(types[a[0]][1][1], 0)
        return 0
    sb_sorted = sorted(res['sbufs'], key=lambda b: b['binding'])


    sup = by_bind.get(vbind(0))
    if sup and names.get(sup[0], '') == 'support_buffer':
        descs.append({'name': 'support_buffer', 'binding': vbind(0), 'type': 'uniform_buffer', 'count': 1, 'source': {'category': 'support_buffer'}})
    for b in res['cbufs']:
        vid, tid, sc = by_bind[vbind(b['binding'])]
        n = b['slot']
        nvn = {'kind': 'uniform', 'slot': n - 3} if 3 <= n <= 16 else ({'kind': 'c1', 'slot': 0} if n == 1 else {'kind': 'driver', 'bank': n, 'slot': 0})
        descs.append({'name': names.get(vid, ''), 'binding': vbind(b['binding']), 'type': 'uniform_buffer', 'count': 1,
                      'source': {'category': 'nvn', 'nvn': nvn}, 'block': {'bytes': block_bytes(tid)}})
    sbase = {'vs': 0x110, 'cs': 0x310, 'ps': 0x510}.get(stage)
    for b in sb_sorted:
        vid, tid, sc = by_bind[vbind(b['binding'])]
        off = b['cb_offset'] * 4 if b['cb_slot'] == 0 else None
        if off is None: raise ValueError(f'存储缓冲不在 c0：{b}')
        if stage == 'cs' and 0x210 <= off < 0x310 and (off - 0x210) % 16 == 0: nvn = {'kind': 'uniform_pointer', 'slot': (off - 0x210) // 16}
        elif sbase is not None and 0 <= off - sbase < 0x100 and (off - sbase) % 16 == 0: nvn = {'kind': 'storage_pointer', 'slot': (off - sbase) // 16}
        else: raise ValueError(f'存储缓冲槽解不出：{stage} off={off:#x}')
        descs.append({'name': names.get(vid, ''), 'binding': vbind(b['binding']), 'type': 'storage_buffer', 'count': 1,
                      'source': {'category': 'nvn', 'nvn': nvn}, 'block': {'bytes': block_bytes(tid)}})
    for kind, lst in (('texture', res['textures']), ('image', res['images'])):
        for t in lst:
            vid, tid, sc = by_bind[vbind(t['binding'])]
            img = image_desc(types, tid)
            slot, samp, ty = decode_tex(t['cbuf_slot'], t['handle'], kind == 'image')
            if slot is None: raise ValueError(f'{kind} 槽解不出：{t}')
            if kind == 'texture':
                typ = 'uniform_texel_buffer' if img['dim'] == 5 else 'combined_image_sampler'
                nvn = {'kind': 'texture', 'slot': slot, 'sampler_slot': samp if ty else None}
            else:
                typ = 'storage_texel_buffer' if img['dim'] == 5 else 'storage_image'
                nvn = {'kind': 'image', 'slot': slot}
            descs.append({'name': names.get(vid, ''), 'binding': vbind(t['binding']), 'type': typ, 'count': 1,
                          'source': {'category': 'nvn', 'nvn': nvn, 'handle_decode': {'type': ty}}, 'image': img})
    descs.sort(key=lambda d: d['binding'])
    inputs = []
    if stage == 'vs':
        for vid, (pt, sc) in var.items():
            if sc == 1 and 30 in deco[vid] and 11 not in deco[vid]: inputs.append({'storage': 'input', 'location': deco[vid][30]})
    return {'identity': {'key': key}, 'sha256': hashlib.sha256(spv).hexdigest(), 'stage': stage, 'capabilities': sorted(set(caps)),
            'descriptors': descs, 'interfaces': inputs, 'frozen_job': {'c1': c1.hex()}}



keys = sorted(want) if want else sorted(loc)
NJ = int(os.environ.get('J', '8'))
os.makedirs(out, exist_ok=True)
work = tempfile.mkdtemp(prefix='dmsh_vk_', dir=os.environ.get('WORK'))
bundle = os.path.join(work, 'bundle'); os.makedirs(bundle)
packs, jobs, c1s = {}, {}, {}
for key in keys:
    name, k = loc[key]
    if name.startswith('main.rodata@'):
        if 'main' not in packs:
            sys.path.insert(0, os.path.join(HERE, '../../verify')); import nsoimg
            packs['main'] = nsoimg.load(MAIN)
        rec = main_rec(packs['main'], k)
    else:
        if name not in packs: packs[name] = records(open(os.path.join(dm_dir, name), 'rb').read())
        rec = packs[name][k]
    jobs[key], c1s[key] = job_for(key, name, rec, work)
chunks = [keys[i::NJ] for i in range(NJ)]
def xl(ch):
    jp = os.path.join(work, f'job_{abs(hash(ch[0]))}.json'); json.dump([jobs[k] for k in ch], open(jp, 'w'))
    return subprocess.run([XLATE, jp, work], capture_output=True, text=True)
with cf.ThreadPoolExecutor(NJ) as ex: xr = list(ex.map(xl, [c for c in chunks if c]))
xerr = {}
for r in xr:
    for ln in r.stderr.splitlines():
        m = re.match(r'【红】(\w+)：(.*)', ln)
        if m: xerr[m.group(1)] = m.group(2)[:160]

def one(key):
    if key in xerr or not os.path.exists(os.path.join(work, key + '.spv')): return key, '翻译失败', xerr.get(key, '无 spv')
    stage = jobs[key]['stage']
    spv = rewrite(open(os.path.join(work, key + '.spv'), 'rb').read(), stage)
    f1, f2 = os.path.join(work, key + '.r.spv'), os.path.join(work, key + '.t.spv')
    open(f1, 'wb').write(spv)
    r = subprocess.run([OPT, '--trim-capabilities', f1, '-o', f2], capture_output=True, text=True)
    if r.returncode: return key, 'spirv-opt 失败', r.stderr.strip()[:160]
    spv = add_format_caps(open(f2, 'rb').read()); open(f2, 'wb').write(spv)
    r = subprocess.run([VAL, '--target-env', 'vulkan1.1', f2], capture_output=True, text=True)
    if r.returncode: return key, 'spirv-val 失败', r.stderr.strip().splitlines()[0][:160] if r.stderr.strip() else '?'
    spv = open(f2, 'rb').read()
    try: vk = export(key, stage, c1s[key], spv, json.load(open(os.path.join(work, key + '.res.json'))))
    except (ValueError, KeyError) as e: return key, '导出失败', str(e)[:160]
    open(os.path.join(bundle, key + '.spv'), 'wb').write(spv)
    json.dump(vk, open(os.path.join(bundle, key + '.vk.json'), 'w'), ensure_ascii=False)
    try: k2, data = pack_game_shaders.pack(__import__('pathlib').Path(os.path.join(bundle, key + '.vk.json')))
    except (ValueError, KeyError) as e: return key, '打包失败', str(e)[:160]
    open(os.path.join(out, k2 + '.xgs'), 'wb').write(data)
    return key, 'ok', ''

with cf.ThreadPoolExecutor(NJ) as ex: res = list(ex.map(one, keys))
if os.environ.get('DMSH_VK_KEEP'): print('中间产物留在', work)
else: shutil.rmtree(work, ignore_errors=True)
st = collections.Counter(r[1] for r in res)
print(f'着色器 {len(res)}：成功 {st["ok"]}，' + '，'.join(f'{k} {v}' for k, v in st.items() if k != 'ok'))
for lvl in [k for k in st if k != 'ok']:
    for e, n in collections.Counter(r[2] for r in res if r[1] == lvl).most_common(8): print(f'  [{lvl}] {n:5d}  {e}')
sys.exit(0 if st['ok'] == len(res) else 1)
