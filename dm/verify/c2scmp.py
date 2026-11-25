









import sys, json
def load(p): return [json.loads(l) for l in open(p) if l.strip()]
a, b = load(sys.argv[1]), load(sys.argv[2])
if '--ids' in sys.argv:
    fa = [x['pkt'][:2] for i, x in enumerate(a) if i == 0 or x['pkt'][:2] != a[i - 1]['pkt'][:2]]
    fb = [x['pkt'][:2] for i, x in enumerate(b) if i == 0 or x['pkt'][:2] != b[i - 1]['pkt'][:2]]
    print(f'包号序列（折叠）：A {len(fa)}，B {len(fb)}')
    n = next((i for i, (x, y) in enumerate(zip(fa, fb)) if x != y), None)
    if n is None and len(fa) != len(fb): n = min(len(fa), len(fb))
    if n is None: print('C2SCMP 包号序列一致'); sys.exit(0)
    print(f'C2SCMP 包号序列不一致：第 {n} 个起  A {fa[max(0, n - 2):n + 3]}  B {fb[max(0, n - 2):n + 3]}'); sys.exit(2)
print(f'射程：A {len(a)} 条，B {len(b)} 条；包号种类 {len({x["pkt"][:2] for x in a + b})}')
if not a or not b: print('C2SCMP 射程不足'); sys.exit(3)
ka = [(x['unit'], x['len'], x['pkt']) for x in a]; kb = [(x['unit'], x['len'], x['pkt']) for x in b]
n = next((i for i, (x, y) in enumerate(zip(ka, kb)) if x != y), None)
if n is None and len(ka) != len(kb): n = min(len(ka), len(kb))
if n is not None:
    print(f'C2SCMP 内容不一致：第 {n} 条起')
    for lab, s in (('A', a), ('B', b)):
        for i in range(max(0, n - 2), min(len(s), n + 3)): print(f'  {lab} {">>" if i == n else "  "} {i} 拍 {s[i]["tick"]} {s[i]["pkt"][:40]}')
    sys.exit(2)
drift = [(i, x['tick'], y['tick']) for i, (x, y) in enumerate(zip(a, b)) if x['tick'] != y['tick']]
if not drift: print('C2SCMP 一致（内容、顺序、落拍全同）'); sys.exit(0)
d = sorted({y - x for _, x, y in drift})
print(f'C2SCMP 内容一致、落拍漂移：{len(drift)}/{len(a)} 条拍号不同，差值种类 {d[:10]}；首个第 {drift[0][0]} 条（A 拍 {drift[0][1]} / B 拍 {drift[0][2]}）')
sys.exit(1)
