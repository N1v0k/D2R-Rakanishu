#!/usr/bin/env python3












import csv, io, json, os, re, sys, collections
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ios', 'tools'))
import touch_layout
if len(sys.argv) != 2: sys.exit('用法：gen_bright_levels.py <原版 RomFS（Update）>')
romfs = sys.argv[1]


IDX = {}
for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
    f = line.rstrip('\n').split('|')
    if len(f) >= 11: IDX.setdefault(f[9].lower().split(':', 1)[-1], f)
def rd(path):
    f = IDX.get(path.lower())
    if f is None: raise SystemExit('gen_bright_levels：游戏文件里没有 %s（RomFS 版本不对？要 3.3.94012）' % path)
    return touch_layout._read_entry(romfs, f)
ENV = 'data/hd/env/'
def ls(prefix):
    out = []
    for k, f in IDX.items():
        if k.startswith(prefix) and '/' not in k[len(prefix):]: out.append(f[9].split(':', 1)[-1][len(prefix):])
    return out
def tsv(name):
    r = list(csv.reader(io.StringIO(rd('data/global/excel/' + name).decode('utf-8', 'replace')), delimiter='\t')); return r[0], r[1:]
def biome_of_json(path):
    try: return os.path.basename(json.loads(rd(path)).get('biomeFilename', '')).replace('.json', '')
    except (SystemExit, Exception): return ''
def norm(path):
    def strip(o):
        if isinstance(o, dict): return {k: strip(v) for k, v in o.items() if k not in ('name', 'id')}
        if isinstance(o, list): return [strip(v) for v in o]
        return o
    return json.dumps(strip(json.loads(rd(path))), sort_keys=True)
varies = {}
for f in ls(ENV + 'biome/'):
    s = rd(ENV + 'biome/' + f).decode('utf-8', 'replace'); vis = re.findall(r'"visualDataFilename"\s*:\s*"data/hd/env/(vis/[^"]+)"', s)
    d = [v for v in vis if v.endswith('_day.json')]; n = [v for v in vis if v.endswith('_night.json')]
    if d and n and (ENV + d[0]).lower() in IDX and (ENV + n[0]).lower() in IDX: varies[f[:-5]] = norm(ENV + d[0]) != norm(ENV + n[0])
def preset_json(ds1):
    parts = ds1.replace('\\', '/').split('/'); p = ENV + 'preset/' + '/'.join(x.lower() for x in parts[:-1]) + '/' + parts[-1].rsplit('.', 1)[0].lower() + '.json'
    return p if p.lower() in IDX else None
def dir_biome(rel):
    d = ENV + 'preset/' + '/'.join(x.lower() for x in rel.replace('\\', '/').split('/')) + '/'
    c = collections.Counter(biome_of_json(d + f) for f in ls(d) if f.lower().endswith('.json'))
    return c.most_common(1)[0][0] if c else ''
lh, lv = tsv('Levels.txt'); ph, pv = tsv('LvlPrest.txt'); th, tv = tsv('LvlTypes.txt')
iid, iname, idt, ilt, iact = lh.index('Id'), lh.index('Name'), lh.index('DrlgType'), lh.index('LevelType'), lh.index('Act')
pl, pf = ph.index('LevelId'), [ph.index(f'File{k}') for k in range(1, 7)]
types = {r[th.index('Id')]: r for r in tv if len(r) > 1}
out = []
for r in lv:
    if len(r) <= ilt or not r[iid].strip().isdigit(): continue
    lid, dt = int(r[iid]), r[idt].strip()
    if not dt.isdigit(): dt = '0'
    c = collections.Counter()
    if dt == '2':
        for p in pv:
            if len(p) > pf[-1] and p[pl].strip() == str(lid):
                for k in pf:
                    j = preset_json(p[k]) if p[k] not in ('', '0') else None
                    if j: c[biome_of_json(j)] += 1
    elif dt == '3':
        t = types.get(r[ilt].strip())
        if t:
            for k in range(2, len(t)):
                m = re.match(r'(Act\d|Expansion)[/\\]([A-Za-z]+)[/\\]', t[k] or '')
                if m: c[dir_biome(m.group(1) + '/' + m.group(2))] += 1
    c.pop('', None)
    b = c.most_common(1)[0][0] if c else ''
    out.append((lid, r[iname], dt, int(r[iact] or 0), b, int(varies[b]) if b in varies else -1))
print('// 由 dm/tools/gen_bright_levels.py 在构建时从玩家自己的游戏文件（Levels / LvlPrest / LvlTypes 与 hd/env 画面配置）生成，勿手改，不进仓库。')
print('// 每行：关卡号、幕号（Levels.txt Act，0 起）、DrlgType（1 迷宫 2 预设 3 野外）、原版画面随不随昼夜（1 随 / 0 不随 / -1 未找到 biome）——biome、关卡名')
for lid, name, dt, act, b, v in out: print(f'{{ {lid}, {act}, {dt or 0}, {v} }},   // {b or "（无）"}，{name}')
