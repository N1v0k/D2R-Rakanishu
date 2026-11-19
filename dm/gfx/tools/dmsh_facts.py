#!/usr/bin/env python3
















import json, os, re, subprocess, sys
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
HERE = os.path.dirname(os.path.abspath(__file__)); HOME = os.path.expanduser('~')
TABLE = os.path.join(HERE, '../nvn/shader_facts.tsv'); MAP = os.path.join(HERE, '../nvn/shader_map.tsv')
TT = {'texture1d': '1d', 'texture1darray': '1darray', 'texture2d': '2d', 'texture2darray': '2darray', 'texture2dms': '2dms',
      'texture2dmsarray': '2dmsarray', 'texture3d': '3d', 'texturecube': 'cube', 'texturecubearray': 'cubearray', 'buffer_typed': 'buffer'}
CT = {'float': 'float', 'int': 'sint', 'uint': 'uint'}

def load():

    if not os.path.exists(TABLE): return {}
    return {k: json.loads(v) for k, v in (l.rstrip('\n').split('\t', 1) for l in open(TABLE) if l.strip() and not l.startswith('#'))}

def row(key, facts): return key + '\t' + json.dumps(facts, sort_keys=True, separators=(',', ':'), ensure_ascii=False) + '\n'

def from_pc(pj, stage, dxbc_asm):

    f = {'tex': {}, 'img': {}, 'attr': {}, 'fout': {}}
    for b in pj['bindings']:
        if b['kind'] == 't' and b['type'] in TT: f['tex'][str(b['reg'])] = TT[b['type']]
        if b['kind'] == 'u' and b['type'] in TT:
            f['img'][str(b['reg'])] = {'type': TT[b['type']], 'fmt': 'R32Uint' if 'uint' in (b.get('msl_type') or '') else 'R16G16B16A16Float'}
    for v in pj.get('vertex_inputs') or []: f['attr'][str(v.get('location', v.get('index')))] = CT.get(v.get('component_type'), 'float')
    for o in pj.get('fragment_outputs') or []: f['fout'][str(o.get('index', 0))] = CT.get(o.get('component_type'), 'float')
    if pj.get('threadgroup_size'): f['local'] = pj['threadgroup_size']
    if stage == 'cs':
        asm = dxbc_asm or ''
        n = sum(int(m.group(1)) for m in re.finditer(r'dcl_tgsm_raw\s+g\d+,\s*(\d+)', asm)) + \
            sum(int(m.group(1)) * int(m.group(2)) for m in re.finditer(r'dcl_tgsm_structured\s+g\d+,\s*(\d+),\s*(\d+)', asm))
        f['shared'] = (n + 15) // 16 * 16
        if dxbc_asm is None: f['pc_missing'] = True
    return f

def pairs():

    pc_of, stage_of = {}, {}
    for ln in open(MAP):
        c = ln.rstrip('\n').split('\t')
        if len(c) > 4 and c[0].endswith('.bin'): pc_of.setdefault(c[3], c[4]); stage_of.setdefault(c[3], c[0][:2])
        if len(c) > 4 and c[0].startswith('main.rodata@'): pc_of.setdefault(c[3], c[4]); stage_of[c[3]] = {'顶点': 'vs', '像素': 'ps'}[c[2]]
    for ln in open(MAP + '.conflicts.tsv'):
        c = ln.rstrip('\n').split('\t')
        if len(c) > 1 and not ln.startswith('#') and not ln.startswith('dm_') and c[0] in pc_of: pc_of[c[0]] = c[1]
    return {k: (pc_of[k], stage_of[k]) for k in pc_of}

def recompute():
    pcc = os.environ.get('PCCACHE', f'{DM_ROOT}/build/cache/metal_shader_cache')
    dxdirs = os.environ.get('PCDXBC', f'{DM_ROOT}/build/ios/dxbc_all_0930:{DM_ROOT}/data/d3d12_study/shaders').split(':')
    vk = os.environ.get('VKD3D', f'{DM_ROOT}/build/vkd3d/inst/bin/vkd3d-compiler')
    if not os.path.isdir(pcc): sys.exit(f'【红】没有 PC 反射目录 {pcc}（PCCACHE=）')
    out, missing = {}, 0
    for key, (pc, stage) in sorted(pairs().items()):
        pp = os.path.join(pcc, pc + '.json')
        if not os.path.exists(pp): missing += 1; continue
        asm = None
        if stage == 'cs':
            dx = next((p for d in dxdirs for p in (os.path.join(d, pc + '.dxbc'), os.path.join(d, pc[:12] + '.dxbc')) if os.path.exists(p)), None)
            if dx: asm = subprocess.run([vk, '-x', 'dxbc-tpf', '-b', 'd3d-asm', dx], capture_output=True, text=True).stdout
        out[key] = from_pc(json.load(open(pp)), stage, asm)
    return out, missing

if __name__ == '__main__':
    a = sys.argv[1:]
    if a == ['--freeze']:
        t, missing = recompute()
        open(TABLE, 'w').write('# 翻译查询的作答表（dmsh_facts.py --freeze 生成，勿手改）：键 <TAB> 事实 JSON\n' + ''.join(row(k, t[k]) for k in sorted(t)))
        print(f'已写 {TABLE}：{len(t)} 键；无配对 PC 反射、未进表 {missing}')
    elif a == ['--check']:
        t, missing = recompute(); have = load()
        bad = sorted(k for k in set(t) | set(have) if t.get(k) != have.get(k))
        for k in bad[:10]: print(f'【红】{k}：表 {have.get(k)} / 重算 {t.get(k)}')
        print(f'逐键比对 {len(set(t) | set(have))}：不一致 {len(bad)}；无配对 PC 反射 {missing}'); sys.exit(1 if bad else 0)
    elif not a:
        t = load(); print(f'{len(t)} 键：' + '，'.join(f'{n} {sum(bool(f.get(n)) for f in t.values())}' for n in ('tex', 'img', 'attr', 'fout', 'local', 'shared', 'pc_missing')))
    else: sys.exit('用法见文件头')
