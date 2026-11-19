#!/usr/bin/env python3
















import hashlib, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import touch_layout

GLOW = 'data/hd/vfx/particles/overlays/object/horadric_light/fx_horadric_light.particles'
SHAFT = 'data/hd/vfx/particles/missiles/diablo_light_shaft/fx_diablo_light_shaft.particles'
TIERS = {'1': GLOW, '2': SHAFT}
ROOTS = ('weapon', 'armor', 'misc')
MISC_LIT = ('rune/', 'ring/', 'amulet/', 'charm/', 'key/', 'quest/')

def add_light(text, fx, name):

    d = json.loads(text); assert d.get('type') == 'UnitDefinition', name
    deps = d['dependencies'].setdefault('particles', [])
    if not any(p.get('path', '').lower() == fx for p in deps): deps.append({'path': fx})
    eid = int.from_bytes(hashlib.md5(('droplight:' + name).encode()).digest()[:4], 'little') | 1
    d['entities'].append({'type': 'Entity', 'name': 'droplight', 'id': eid, 'components': [
        {'type': 'TransformDefinitionComponent', 'name': 'component_transform_droplight', 'position': {'x': 0.0, 'y': 0.0, 'z': 0.0},
         'orientation': {'x': 0.0, 'y': 0.0, 'z': 0.0, 'w': 1.0}, 'scale': {'x': 1.0, 'y': 1.0, 'z': 1.0}, 'inheritOnlyPosition': False},
        {'type': 'VfxDefinitionComponent', 'name': 'component_vfx_droplight', 'filename': fx, 'hardKillOnDestroy': False}]})
    return json.dumps(d, separators=(',', ':'))

def variant_path(root, asset, tier):

    return f'data/hd/items/{root}/zswg/{tier}__' + asset.lower().replace('/', '__') + '.json'

def build(romfs, outdir, repl):

    manifest = set()
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) >= 11 and not line.startswith('#'): manifest.add(f[9].split(':', 1)[-1].lower())
    assets = set()
    for table in ('items.json', 'uniques.json', 'sets.json'):
        for row in json.loads(touch_layout.read_layout(romfs, None, suffix='/hd/items/' + table)[1]):
            (_, v), = row.items()
            assets.update(x.lower() for x in ([v['asset']] if 'asset' in v else [v[k] for k in ('normal', 'uber', 'ultra')]))
    out, skipped = {}, 0
    for asset in sorted(assets):
        root = next((r for r in ROOTS if f'data/hd/items/{r}/{asset}.json' in manifest), None)
        if root is None: skipped += 1; continue
        if root == 'misc' and not asset.startswith(MISC_LIT): continue
        src = f'data/hd/items/{root}/{asset}.json'
        text = open(repl[src], encoding='utf-8').read() if src in repl else touch_layout.read_layout(romfs, None, suffix='/' + src.split('/', 1)[1])[1]
        for tier, fx in TIERS.items():
            dst = variant_path(root, asset, tier)
            o = os.path.join(outdir, 'droplight_' + dst.replace('/', '_')); open(o, 'w').write(add_light(text, fx, dst)); out['+' + dst] = o
    return out, f'掉落光变体：{len(out) // 2} 个模型 × 2 档（原表指向不存在模型、跳过 {skipped} 个）'

if __name__ == '__main__':
    romfs, out = sys.argv[1:3]; os.makedirs(out, exist_ok=True)
    r, stat = build(romfs, out, {})
    with open(os.path.join(out, 'repl.tsv'), 'w') as f:
        for k, v in r.items(): f.write(f'{k}\t{v}\n')
    print(stat)
