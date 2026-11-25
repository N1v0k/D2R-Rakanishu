#!/usr/bin/env python3


















import os, sys, hashlib, subprocess
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
HERE = os.path.dirname(os.path.abspath(__file__)); GFX = os.path.dirname(HERE); HOME = os.path.expanduser('~')
FILES = ['tools/dmsh_chain.py', 'tools/dmsh_build.py', 'tools/dmsh_fetch.py', 'tools/dmsh_facts.py', 'nvn/shader_facts.tsv', 'tools/dmsh_xlate/Program.cs', 'tools/dmsh_xlate/dmsh_xlate.csproj',
         'nvn/shader_map.tsv', 'nvn/shader_map.tsv.conflicts.tsv', '../ios/tools/ios_metal.py', 'tools/dmsh_chain.lock']
LOCK = os.path.join(HERE, 'dmsh_chain.lock'); HEADER = os.path.join(GFX, 'nvn/dmsh_chain.h')
XLATE_DIR = os.path.dirname(os.environ.get('XLATE', f'{DM_ROOT}/build/gfx/dmsh_xlate/dmsh_xlate'))

def options():
    return 'debug=%s reduced=%s' % (os.environ.get('XLATE_DEBUG', '1') != '0', os.environ.get('XLATE_REDUCED', '1') == '1')

def version():
    h = hashlib.sha256()
    for f in FILES:
        h.update(f.encode() + b'\0' + open(os.path.join(GFX, f), 'rb').read() + b'\0')
    h.update(options().encode())
    return h.hexdigest()[:12]

def sha16(p): return hashlib.sha256(open(p, 'rb').read()).hexdigest()[:16]

def tools_now():

    t = {}
    for dll in ('dmsh_xlate.dll', 'Ryujinx.Graphics.Shader.dll', 'Ryujinx.Common.dll'): t[dll] = sha16(os.path.join(XLATE_DIR, dll))



    try:
        r = subprocess.run([os.path.join(XLATE_DIR, 'dmsh_xlate'), '--source-sha'], capture_output=True, text=True)
        t['xlate_source'] = r.stdout.strip() or '?退出码 %d：%s' % (r.returncode, (r.stderr.strip().splitlines() or ['没有输出'])[0][:160])
    except OSError as e:
        t['xlate_source'] = '?起不来：%s' % e
    sc = subprocess.run(['which', 'spirv-cross'], capture_output=True, text=True).stdout.strip()
    t['spirv-cross'] = sha16(sc) if sc else 'missing'
    for sdk in ('macosx', 'iphoneos'):
        t['metal_' + sdk] = subprocess.run(['xcrun', '-sdk', sdk, 'metal', '--version'], capture_output=True, text=True).stdout.splitlines()[0:1] or ['missing']
        t['metal_' + sdk] = t['metal_' + sdk][0]
    return t

def xlate_source_sha():
    return hashlib.sha256(b''.join(open(os.path.join(HERE, 'dmsh_xlate', f), 'rb').read() for f in ('Program.cs', 'dmsh_xlate.csproj'))).hexdigest()[:16]

def xlate_problem(src):



    if src.startswith('?'):
        return f'翻译器没跑起来（{src[1:]}）：多半是旧壳留下的框架依赖版找不到 .NET；删掉 {XLATE_DIR} 再做包（会换成壳里预编的），或重编 dmsh_xlate'
    return f'翻译器不是用仓内源编的（嵌入 {src} / 仓内 {xlate_source_sha()}）：先重编 dmsh_xlate'

def check_tools():
    want = dict(l.rstrip('\n').split('\t', 1) for l in open(LOCK) if '\t' in l and not l.startswith('#'))
    now = tools_now(); bad = [f'{k}：锁 {want.get(k)} / 实际 {v}' for k, v in now.items() if want.get(k) != v]
    if now['xlate_source'] != xlate_source_sha(): bad.append(xlate_problem(now['xlate_source']))
    return bad

if __name__ == '__main__':
    a = sys.argv[1:]
    if a == ['--update-lock']:
        t = tools_now()
        if t['xlate_source'] != xlate_source_sha(): sys.exit('【红】' + xlate_problem(t['xlate_source']))
        open(LOCK, 'w').write('# 翻译链工具指纹（dmsh_chain.py --update-lock 生成；换工具后重生成并随提交入仓）\n' + ''.join(f'{k}\t{v}\n' for k, v in t.items()))
        print('已写', LOCK)
    elif a == ['--check-tools']:
        bad = check_tools()
        for b in bad: print('【红】翻译链工具与锁文件不符：' + b)
        sys.exit(1 if bad else 0)
    elif a == ['--header']:
        open(HEADER, 'w').write('// 生成文件（dm/gfx/tools/dmsh_chain.py --header）：本源码树期望的 B 着色器库翻译链版本（缺省选项）；宿主只加载 <库目录>/b-<V>/\n'
                                f'#define DMSH_CHAIN "{version()}"\n')
        print(version())
    elif not a: print(version())
    else: sys.exit(__doc__ if __doc__ else '用法见文件头')
