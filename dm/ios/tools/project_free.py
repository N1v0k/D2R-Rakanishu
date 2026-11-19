#!/usr/bin/env python3














import re, sys
src, dst = sys.argv[1], sys.argv[2]
L = open(src, encoding='utf-8').read().split('\n')
ENT = re.compile(r'^ *com\.apple\.developer\.(kernel\.increased-memory-limit|kernel\.extended-virtual-addressing|sustained-execution|icloud-container-identifiers|icloud-services|ubiquity-container-identifiers):')
BA = re.compile(r'^ *(BAAppGroupID|BAHasManagedAssetPacks|BAUsesAppleHosting):')
out, n_ent, n_ba, n_dep, n_tgt, i = [], 0, 0, 0, 0, 0
while i < len(L):
    l = L[i]
    if ENT.match(l): n_ent += 1; i += 1; continue
    if BA.match(l): n_ba += 1; i += 1; continue
    if re.match(r'^ *- target: RomfsDownloader\b', l): n_dep += 1; i += 1; continue
    if re.match(r'^  RomfsDownloader:\s*$', l):
        n_tgt += 1; i += 1
        while i < len(L) and not re.match(r'^  \S', L[i]) and not re.match(r'^\S', L[i]): i += 1
        continue
    out.append(l); i += 1
if (n_ent, n_ba, n_dep, n_tgt) != (6, 3, 1, 1): sys.exit('project_free.py：该去掉的东西数目不对（权限 %d / 6、托管资源包键 %d / 3、依赖 %d / 1、扩展目标 %d / 1）——工程描述的写法变了，先改这个脚本' % (n_ent, n_ba, n_dep, n_tgt))
if any('RomfsDownloader' in l and not l.lstrip().startswith('#') for l in out): sys.exit('project_free.py：去掉之后工程描述里还有 RomfsDownloader')
open(dst, 'w', encoding='utf-8').write('\n'.join(out))
