







import re, sys, collections
log, t0, t1, wth, wpt, hth = sys.argv[1], float(sys.argv[2]), float(sys.argv[3]), sys.argv[4], sys.argv[5], sys.argv[6]
L = open(log, errors='replace').read().splitlines(); S = []
for i, l in enumerate(L):
    m = re.match(r'\[dm\] SAMPLE #\d+ \[([\d.]+)s\] thr=(0x[0-9a-f]+) pc=(\S+) lr=(\S+)', l)
    if m and i + 1 < len(L) and L[i + 1].startswith('[dm] bt:'): S.append((float(m.group(1)), m.group(2), m.group(3), L[i + 1][9:].split()))
W = [s for s in S if s[1] == wth and t0 <= s[0] <= t1 and any(f.startswith(wpt) for f in [s[2]] + s[3][:12])]
H = [s for s in S if s[1] == hth]
agg = collections.Counter()
print(f'{wth} 在 {wpt}* 上等的样本 {len(W)}')
for t, *_ in W:
    n = min(H, key=lambda s: abs(s[0] - t)); chain = []
    for f in [n[2]] + n[3]:
        if f.startswith(('main+', 'sdk+')): break
        chain.append(f.split('+')[0])
    game = [f for f in n[3] if f.startswith('main+')][:4]
    print(f'{t:.3f} 持锁线程@{n[0]:.3f} 宿主链：{" ← ".join(chain[:8])} | 游戏帧 {" ← ".join(game)}')
    for f in set(chain): agg[f] += 1
print('宿主函数出现次数：', agg.most_common(15))
