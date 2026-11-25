











import os, shlex, subprocess, sys
host, gen = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
cmd = subprocess.run(['make', '-s', '-n', '-B', '-C', host, 'dmhost'], capture_output=True, text=True, check=True).stdout
toks = shlex.split(next(l for l in cmd.splitlines() if l.lstrip().startswith('clang')))
srcs, incs, defs, fws = [], [], [], []
i = 0
while i < len(toks):
    t = toks[i]
    if t == '-framework': fws.append(toks[i + 1]); i += 2; continue
    if t.startswith('-I'): incs.append(os.path.normpath(os.path.join(host, t[2:])))
    elif t.startswith('-D'): defs.append(t)
    elif t.endswith(('.c', '.m', '.S')): srcs.append(os.path.normpath(os.path.join(host, t)))
    i += 1
assert srcs, '编译命令里没有源文件'
dup = sorted({s for s in srcs if srcs.count(s) > 1})
if dup: sys.exit('编译命令里源文件重复（Makefile 显式列与通配重叠？）：' + ' '.join(dup))
os.makedirs(f'{gen}/src'); os.makedirs(f'{gen}/main')
dirs = []
for s in srcs:
    d = os.path.dirname(s)
    if d not in dirs: dirs.append(d)
    b = os.path.basename(s)
    if os.path.splitext(b)[0].endswith('_mac'): continue
    if os.path.dirname(s) == os.path.normpath(os.path.join(host, '../ios/Sources')): continue
    if s == os.path.join(host, 'main.c'): os.symlink(s, f'{gen}/main/main.c'); continue
    flat = os.path.relpath(s, os.path.dirname(host)).replace('/', '__')
    os.symlink(s, f'{gen}/src/{flat}')
print('HEADER_SEARCH_PATHS = $(inherited) ' + ' '.join(f'"{d}"' for d in incs + dirs))
print('OTHER_CFLAGS = $(inherited) ' + ' '.join(defs))
print('DM_FRAMEWORKS = ' + ' '.join(f'-framework {f}' for f in fws if f != 'Cocoa'))
