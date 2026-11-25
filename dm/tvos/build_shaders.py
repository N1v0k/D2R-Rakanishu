#!/usr/bin/env python3










import argparse, concurrent.futures, csv, hashlib, json, os, pathlib, re, subprocess, sys
here = pathlib.Path(__file__).resolve().parent; dm = here.parent
sys.path.insert(0, str(dm / 'gfx/tools')); import dmsh_fetch
ap = argparse.ArgumentParser(); ap.add_argument('src', type=pathlib.Path); ap.add_argument('out', type=pathlib.Path)
ap.add_argument('-j', type=int, default=os.cpu_count())
a = ap.parse_args()
chain = re.search(r'#define DMSH_CHAIN "([^"]+)"', (dm / 'gfx/nvn/dmsh_chain.h').read_text())[1]
src, out = a.src / ('b-' + chain), a.out / ('b-' + chain)
if not src.is_dir(): sys.exit(f'【红】Mac 着色器缓存里没有当前翻译链版本 {src}（先按 dm/README 出 Mac 着色器）')
out.mkdir(parents=True, exist_ok=True)
METAL = ['xcrun', '-sdk', 'appletvos', 'metal', '-c', '-mtvos-version-min=26.0']
METALLIB = ['xcrun', '-sdk', 'appletvos', 'metallib']
RECORD = ' '.join(METAL) + '; ' + ' '.join(METALLIB)
version = subprocess.check_output(['xcrun', '-sdk', 'appletvos', 'metal', '--version'], text=True)
want = {s.strip() for s in (dm / 'gfx/nvn/bf_want.tsv').read_text().splitlines() if s.strip() and not s.startswith('#')}
rows = csv.reader((l for l in (dm / 'gfx/nvn/shader_map.tsv').read_text().splitlines() if l and not l.startswith('#')), delimiter='\t')
keys = sorted({r[3] for r in rows if len(r) > 3 and re.fullmatch(r'[0-9a-f]{16}', r[3])})

def compile(msl_text, lib):
    air = lib.with_suffix('.air'); msl = lib.with_suffix('.metal'); tmp = lib.with_suffix('.tmp')
    msl.write_text(msl_text)
    try:
        for cmd in (METAL + [str(msl), '-o', str(air)], METALLIB + [str(air), '-o', str(tmp)]):
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode: raise RuntimeError(f'{lib.name}: {r.stderr.strip()[:400]}')
        os.replace(tmp, lib)
    finally:
        for p in (air, msl, tmp): p.unlink(missing_ok=True)

def build(k):
    msl, meta = (src / f'{k}_b.metal').read_text(), json.loads((src / f'{k}_b.json').read_text())
    fp = hashlib.sha256((msl + json.dumps(meta, sort_keys=True) + version + RECORD).encode() + (here / 'build_shaders.py').read_bytes()).hexdigest()
    stamp = out / f'{k}.tvos.sha256'
    fetch = meta.get('fetch')
    names = [f'{k}_b.metallib', f'{k}_b.json'] + ([f'{k}_bf.metallib'] if fetch else [])
    if stamp.exists() and stamp.read_text() == fp and all((out / n).exists() for n in names): return k, bool(fetch), 'cached'
    compile(msl, out / f'{k}_b.metallib')
    if fetch:
        fv, fmeta = dmsh_fetch.variant(msl, meta['bindings'], om_only_ok=k in want)
        if not fv or fmeta != fetch: raise RuntimeError(f'{k}: 取帧变体与反射里的 fetch 对不上')
        compile(fv, out / f'{k}_bf.metallib')
    else: (out / f'{k}_bf.metallib').unlink(missing_ok=True)
    (out / f'{k}_b.json').write_text(json.dumps(dict(meta, chain_metal=RECORD), ensure_ascii=False))
    stamp.write_text(fp)
    return k, bool(fetch), 'built'

missing = [k for k in keys if not (src / f'{k}_b.metal').exists() or not (src / f'{k}_b.json').exists()]
if missing: sys.exit(f'【红】Mac 着色器缓存缺 {len(missing)} 个键（如 {missing[:3]}），先补齐 Mac 着色器')
fails, n_fetch, n_built = [], 0, 0
with concurrent.futures.ThreadPoolExecutor(max_workers=a.j) as pool:
    for i, f in enumerate(concurrent.futures.as_completed([pool.submit(build, k) for k in keys]), 1):
        try: k, fe, how = f.result(); n_fetch += fe; n_built += how == 'built'
        except Exception as e: fails.append(str(e))
        if i % 1000 == 0: print(f'{i}/{len(keys)}', flush=True)
if fails: print('\n'.join(fails[:10])); sys.exit(f'【红】{len(fails)} 个着色器没编过')
print(f'tvOS 着色器 OK（链 {chain}）：基础库 {len(keys)}，取帧变体 {n_fetch}，本次新编 {n_built}，输出 {out}')
