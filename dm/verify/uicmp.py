





import sys, json
UNTIL = sys.argv[sys.argv.index('--until') + 1] if '--until' in sys.argv else None


EXEMPT = {('OpenPanel', 'SaveIndicatorPanel'), ('ClosePanel', 'SaveIndicatorPanel')}
EXEMPT_H = {('0x723d40e52f74e229', 'SaveIndicatorPanel')}
def exempt(r): return (r.get('act'), r.get('arg')) in EXEMPT or (r['act_h'], r.get('arg')) in EXEMPT_H
def load(p):
    raw = [json.loads(l) for l in open(p, encoding='utf-8') if l.strip()]
    seq = [r for r in raw if not exempt(r)]
    load.dropped = getattr(load, 'dropped', 0) + len(raw) - len(seq)
    if UNTIL:
        k = next((i for i, r in enumerate(seq) if r.get('arg') == UNTIL), None)
        if k is None: print(f'{p} 里没有 {UNTIL}'); sys.exit(2)
        seq = seq[:k + 1]
    keys = [(r['cat_h'], r['act_h'], r.get('arg')) for r in seq]
    names = {(r['cat_h'], r['act_h'], r.get('arg')): f"{r.get('cat', r['cat_h'])}:{r.get('act', r['act_h'])}:{r.get('arg')}" for r in seq}
    fold = [k for i, k in enumerate(keys) if i == 0 or k != keys[i - 1]]
    return keys, fold, names
a, fa, na = load(sys.argv[1]); b, fb, nb = load(sys.argv[2]); names = {**nb, **na}
n = next((i for i, (x, y) in enumerate(zip(fa, fb)) if x != y), None)
if n is None and len(fa) != len(fb): n = min(len(fa), len(fb))
print(f'射程：真值 {len(a)} 条（折叠后 {len(fa)}），宿主 {len(b)} 条（折叠后 {len(fb)}），类别 {len({k[0] for k in fa + fb})} 种；豁免剔除 {load.dropped} 条')
if n is None: print('UICMP 一致'); sys.exit(0)
print(f'UICMP 不一致：折叠后第 {n} 条起')
for lab, f in (('真值', fa), ('宿主', fb)):
    print(f'  {lab}:'); [print(f"   {'>>' if i == n else '  '} {i} {names.get(f[i], f[i])}") for i in range(max(0, n - 3), min(len(f), n + 4))]
sys.exit(1)
