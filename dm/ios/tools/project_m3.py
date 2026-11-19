#!/usr/bin/env python3
















import sys
src, dst = sys.argv[1:3]; free = sys.argv[3:4] == ['free']; s = open(src).read()
def one(old, new):
    global s
    if s.count(old) != 1: sys.exit(f"🔴 project_m3：工程描述里「{old[:40]}」应恰好出现 1 次，实际 {s.count(old)} 次")
    s = s.replace(old, new)
one('    iOS: "26.0"', '    iOS: "18.0"')
one('OTHER_CFLAGS: $(inherited) -mcpu=apple-a18', 'OTHER_CFLAGS: $(inherited) -mcpu=apple-a14 -DDM_NVN_M3=1')
import re
L = s.split('\n'); out, n_ba, n_dep, n_tgt, i = [], 0, 0, 0, 0
while i < len(L):
    l = L[i]
    if re.match(r'^ *(BAAppGroupID|BAHasManagedAssetPacks|BAUsesAppleHosting):', l): n_ba += 1; i += 1; continue
    if re.match(r'^ *- target: RomfsDownloader\b', l): n_dep += 1; i += 1; continue
    if re.match(r'^  RomfsDownloader:\s*$', l):
        n_tgt += 1; i += 1
        while i < len(L) and not re.match(r'^  \S', L[i]) and not re.match(r'^\S', L[i]): i += 1
        continue
    out.append(l); i += 1
want = (0, 0, 0) if free else (3, 1, 1)
if (n_ba, n_dep, n_tgt) != want: sys.exit(f"🔴 project_m3：资源下载扩展没去干净（托管资源包键 {n_ba} / {want[0]}、依赖 {n_dep} / {want[1]}、扩展目标 {n_tgt} / {want[2]}{'；输入应已过免费档' if free else ''}）")
if any('RomfsDownloader' in l and not l.lstrip().startswith('#') for l in out): sys.exit("🔴 project_m3：去掉之后工程描述里还有 RomfsDownloader")
open(dst, 'w').write('\n'.join(out))
