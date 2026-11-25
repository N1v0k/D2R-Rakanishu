#!/usr/bin/env python3






















import filecmp, hashlib, json, os, re, runpy, shutil, subprocess, sys, time, zipfile

HERE = os.path.dirname(os.path.abspath(__file__)); DM = os.path.dirname(HERE); IOS = os.path.join(DM, 'ios'); GT = os.path.join(DM, 'gfx', 'tools')
ROOT = os.path.abspath(os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work'))
PY = sys.executable
WINLF = os.path.join(HERE, 'winlf'); runpy.run_path(os.path.join(WINLF, 'sitecustomize.py'))
MODS = ['main', 'sdk']


def say(s): print('[%s] %s' % (time.strftime('%H:%M:%S'), s), flush=True)


def die(s): sys.exit('[x] ' + s)


def run(cmd, env=None, cwd=None, log=None, what=None):
    e = dict(os.environ, DM_ROOT=ROOT, PYTHONUTF8='1', PYTHONPATH=os.pathsep.join(filter(None, [WINLF, os.environ.get('PYTHONPATH')]))); e.update(env or {})
    r = subprocess.run(cmd, env=e, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if log: open(log, 'wb').write(r.stdout)
    if r.returncode: die('%s 没成功（退出码 %d）：%s%s' % (what or cmd[0], r.returncode, r.stdout.decode('utf-8', 'replace').strip().splitlines()[-1:] or '', '；完整输出 ' + log if log else ''))
    return r.stdout.decode('utf-8', 'replace')


def step(name, done, fn):
    if done(): say('%s：已有，跳过' % name); return
    t0 = time.time(); say(name); fn()
    if not done(): die('%s 做完了但没看到产物' % name)
    say('%s 完成（%d 秒）' % (name, time.time() - t0))


def read_shell(ipa):
    try: z = zipfile.ZipFile(ipa)
    except zipfile.BadZipFile: die('这个文件不是安装包（解不开）：%s' % ipa)
    apps = sorted({n.split('/')[1] for n in z.namelist() if n.startswith('Payload/') and n.count('/') >= 2 and n.split('/')[1].endswith('.app')})
    if len(apps) != 1: die('壳里的 App 不是恰好一个：%s' % apps)
    base = 'Payload/%s/' % apps[0]
    try:
        manifest = [l.strip() for l in z.read(base + 'dm/shell_manifest.txt').decode().splitlines() if l.strip()]
        info = dict(l.strip().split('=', 1) for l in z.read(base + 'dm/shell_info.txt').decode().splitlines() if '=' in l)
    except KeyError: die('这不是壳：里面没有 dm/shell_manifest.txt / dm/shell_info.txt（%s）' % ipa)
    exe = next((n for n in z.namelist() if n == base + apps[0][:-4]), None)
    return z, base, manifest, info, exe


def plat():
    import platform
    m = platform.machine().lower()
    return ('win-' if os.name == 'nt' else 'macos-' if sys.platform == 'darwin' else 'linux-') + ('arm64' if m in ('arm64', 'aarch64') else 'x64')


def place_deswizzle(z):




    if os.environ.get('DM_DESWIZZLE_LIB'): return True
    pre = 'Tools/deswizzle/%s/' % plat(); names = [n for n in z.namelist() if n.startswith(pre) and not n.endswith('/')]
    if not names: return False
    d = os.path.join(ROOT, 'build', 'tools', 'deswizzle', plat()); os.makedirs(d, exist_ok=True)
    for n in names:
        with open(os.path.join(d, os.path.basename(n)), 'wb') as o: o.write(z.read(n))
    lib = os.path.join(d, 'deswizzle.dll' if os.name == 'nt' else 'deswizzle.dylib' if sys.platform == 'darwin' else 'deswizzle.so')
    if not os.path.isfile(lib): return False
    os.environ['DM_DESWIZZLE_LIB'] = lib; say('贴图解排布库：用壳里预编的（%s）' % plat())
    return True


def find_spirv_cross(z=None):






    exe = 'spirv-cross.exe' if os.name == 'nt' else 'spirv-cross'
    hit = None
    if z is not None and 'Tools/spirv-cross/%s/%s' % (plat(), exe) in z.namelist():
        d = os.path.join(ROOT, 'build', 'tools', 'spirv-cross', plat()); os.makedirs(d, exist_ok=True)
        hit = os.path.join(d, exe)
        with open(hit, 'wb') as o: o.write(z.read('Tools/spirv-cross/%s/%s' % (plat(), exe)))
        os.chmod(hit, 0o755); say('spirv-cross：用壳里带的（%s）' % plat())
    hit = hit or shutil.which('spirv-cross')
    if hit and os.path.dirname(hit) not in os.environ.get('PATH', '').split(os.pathsep):
        os.environ['PATH'] = os.path.dirname(hit) + os.pathsep + os.environ.get('PATH', '')
    return hit


def check_env(game, z=None):




    bad = []
    for m in ('PIL', 'numpy', 'lz4', 'capstone'):
        try: __import__(m)
        except Exception: bad.append('Python 包 %s：%s -m pip install pillow numpy lz4 capstone' % (m, PY))
    have_dsw = place_deswizzle(z) if z else bool(os.environ.get('DM_DESWIZZLE_LIB'))
    if not have_dsw and not (shutil.which('clang') or shutil.which('gcc') or shutil.which('cc')): bad.append('贴图解排布库：壳里没有 %s 的预编版，这台机器也没有 C 编译器（clang 或 gcc）现编' % plat())


    dotnet = None
    if not xlate_prebuilt(z):
        dotnet = shutil.which('dotnet') or next((p for p in (os.path.expanduser('~/.dotnet/dotnet'),) if os.path.isfile(p)), None)
        if not dotnet or not subprocess.run([dotnet, '--list-sdks'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout.strip():
            bad.append('着色器翻译工具：壳里没有 %s 的预编版（壳太旧，请换新版壳），这台机器也没有 .NET SDK 现编' % plat())
        elif not os.environ.get('DOTNET_ROOT'):
            r = subprocess.run([dotnet, '--list-runtimes'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            m = re.search(r'^Microsoft\.NETCore\.App \S+ \[(.*)[/\\]shared[/\\]Microsoft\.NETCore\.App\]', r.stdout.decode('utf-8', 'replace'), re.M)
            if m: os.environ['DOTNET_ROOT'] = m.group(1)
    if not find_spirv_cross(z): bad.append('spirv-cross：壳里没带这个平台的、PATH 里也没有（版本要和壳 dm/shell_info.txt 的 SPIRV_CROSS 一致）：https://github.com/KhronosGroup/SPIRV-Cross')
    for f in ('ExeFS/main', 'ExeFS/sdk', 'RomFS/data/build_manifest.psv'):
        if not os.path.isfile(os.path.join(game, f)): bad.append('游戏文件 %s（在 %s 下没找到）' % (f, game))
    if bad: die('还缺 %d 项：\n  - %s' % (len(bad), '\n  - '.join(bad)))
    return dotnet


def game_layout(game):


    if os.path.isdir(os.path.join(game, 'ExeFS')) or not (os.path.isdir(os.path.join(game, 'exefs')) and os.path.isdir(os.path.join(game, 'romfs'))):
        return game
    d = os.path.join(ROOT, 'build', 'pack', 'game'); shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    for a, b in (('ExeFS', 'exefs'), ('RomFS', 'romfs')): os.symlink(os.path.join(game, b), os.path.join(d, a), target_is_directory=True)
    return d


def xlate_prebuilt(z):

    return bool(os.environ.get('DM_XLATE_PREBUILT', '').strip()) or bool(z and any(n.startswith(XLATE_IN_SHELL + plat() + '/') for n in z.namelist()))


def place_xlate(xl, z=None):




    src = os.environ.get('DM_XLATE_PREBUILT', '').strip(); pre = XLATE_IN_SHELL + plat() + '/'
    names = [n for n in (z.namelist() if z else []) if n.startswith(pre) and not n.endswith('/')]
    if not src and not names: return False
    shutil.rmtree(xl, ignore_errors=True); os.makedirs(xl)
    if src: shutil.copytree(src, xl, dirs_exist_ok=True)
    else:
        for n in names:
            f = os.path.join(xl, n[len(pre):]); os.makedirs(os.path.dirname(f), exist_ok=True)
            with open(f, 'wb') as o: o.write(z.read(n))
    for e in ('dmsh_xlate', 'dmsh_xlate.exe'):
        if os.path.isfile(os.path.join(xl, e)): os.chmod(os.path.join(xl, e), 0o755)
    say('着色器翻译工具：用预编的（%s）' % (src or '壳里带的 ' + plat()))
    return True


XLATE_IN_SHELL = 'Tools/dmsh_xlate/'


def xlate_ready(xl, z=None):







    if not any(os.path.isfile(os.path.join(xl, e)) for e in ('dmsh_xlate', 'dmsh_xlate.exe')): return False
    src = os.environ.get('DM_XLATE_PREBUILT', '').strip()
    if src:
        for root, _, files in os.walk(src):
            for f in files:
                a = os.path.join(root, f); b = os.path.join(xl, os.path.relpath(a, src))
                if not os.path.isfile(b) or not filecmp.cmp(a, b, shallow=False): return False
        return True
    pre = XLATE_IN_SHELL + plat() + '/'
    for n in (z.namelist() if z else []):
        if not n.startswith(pre) or n.endswith('/'): continue
        b = os.path.join(xl, n[len(pre):])
        if not os.path.isfile(b) or os.path.getsize(b) != z.getinfo(n).file_size: return False
        with open(b, 'rb') as f:
            if f.read() != z.read(n): return False
    return True


def patched_ready(out):







    src = hashlib.sha256(open(os.path.join(HERE, 'patch_nso.py'), 'rb').read()).hexdigest()
    try: rec = open(os.path.join(out, 'build_src.sha256')).read().split()[0]
    except (OSError, IndexError): return False
    return rec == src and all(os.path.isfile(os.path.join(out, m + '.patched.bin')) and os.path.isfile(os.path.join(out, m + '.gates.tsv')) for m in MODS)




PHONE_ARCHIVES = ['00-00000001.data', '00-00000002.data', '00-00000003.data', '00-00000004.data', '00-00000005.data', '00-00000006.data',
                  '01-00000001.data', '01-00000002.data', '02-00000001.data', '02-00000002.data', '03-00000001.data', '03-00000002.data',
                  '04-00000001.data', '04-00000002.data', '05-00000001.data', '05-00000002.data', '17-00000001.data', '17-00000002.data']


def s2arm_pack(romfs, dst):




    os.makedirs(os.path.join(dst, 'data'), exist_ok=True)
    m = os.path.join(dst, 'manifest.json')
    if os.path.exists(m): os.remove(m)
    rows, total = [], 0
    for n in PHONE_ARCHIVES:
        src, out = os.path.join(romfs, 'data', n), os.path.join(dst, 'data', n)
        if not os.path.isfile(src): die('游戏资源里缺 %s（%s）' % (n, src))
        if not (os.path.isfile(out) and os.path.getsize(out) == os.path.getsize(src)):
            if os.path.exists(out): os.remove(out)
            if not (sys.platform == 'darwin' and subprocess.run(['cp', '-c', src, out], stderr=subprocess.DEVNULL).returncode == 0): shutil.copyfile(src, out)
        h = hashlib.sha256()
        with open(out, 'rb') as f:
            for b in iter(lambda: f.read(8 << 20), b''): h.update(b)
        size = os.path.getsize(out); total += size; rows.append('data/%s\t%d\t%s\n' % (n, size, h.hexdigest()))
    index = ''.join(rows)
    with open(os.path.join(dst, 'index.tsv'), 'w', encoding='utf-8', newline='\n') as f: f.write(index)
    with open(m + '.tmp', 'w', encoding='utf-8') as f: json.dump({'version': 2, 'files': len(rows), 'total': total, 'index_sha256': hashlib.sha256(index.encode()).hexdigest()}, f, indent=1)
    os.replace(m + '.tmp', m)
    return len(rows), total


def have_metal():
    if sys.platform != 'darwin': return False
    try: return subprocess.run(['xcrun', '-sdk', 'iphoneos', 'metal', '--version'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    except FileNotFoundError: return False


def src_digest(d):





    h = hashlib.sha256()
    for f in sorted(x for x in os.listdir(d) if x.endswith('_b.metal')):
        h.update(f.encode()); h.update(hashlib.sha256(open(os.path.join(d, f), 'rb').read().replace(b'\r\n', b'\n')).digest())
    return h.hexdigest()[:16]


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--src-digest': print(src_digest(sys.argv[2])); return
    if len(sys.argv) != 4: sys.exit(__doc__ or '用法：d2rm_pack.py <壳.ipa> <游戏文件目录> <输出.ipa>')
    shell, game, out_ipa = (os.path.abspath(x) for x in sys.argv[1:])
    for st in (sys.stdout, sys.stderr):
        try: st.reconfigure(errors='replace')
        except Exception: pass
    if not os.path.isfile(shell): die('没有这个壳文件：%s' % shell)
    if not os.path.isdir(game): die('没有这个游戏文件目录：%s' % game)
    game = game_layout(game)
    z, base, manifest, info, exe = read_shell(shell)
    P = open(os.path.join(DM, 'PRODUCT')).readline().strip()
    if info.get('PRODUCT') != P: die('壳与这份源码不是同一版：壳要的产物代号是 %s，这份源码是 %s。请用与壳同一个发布版本的源码' % (info.get('PRODUCT'), P))
    chain = info.get('CHAIN') or die('壳的版本信息里没有着色器链版本')
    dotnet = check_env(game, z)




    nometal = os.environ.get('D2RM_NO_METAL') == '1' or not have_metal()


    gt = (lambda script, *a: [PY, os.path.join(GT, 'aot_run.py'), *([os.path.join(GT, 'nometal_run.py')] if nometal else []), os.path.join(GT, script), *a])
    if nometal: say('这台机器不调苹果的着色器编译器：着色器以源码放进安装包，首次启动时在手机上编（第一次进游戏会多等一会儿）')
    romfs = os.path.join(game, 'RomFS'); stage = os.path.join(ROOT, 'build', 'pack', 'stage'); shutil.rmtree(stage, ignore_errors=True); os.makedirs(os.path.join(stage, 'dm'))
    say('壳 %s：产物代号 %s，着色器链 %s，要补 %d 项' % (os.path.basename(shell), P, chain, len(manifest)))



    out = os.path.join(ROOT, 'build', 'cache', 'dm', 'out', P + '-touch'); os.makedirs(out, exist_ok=True)
    def patch():
        for m in MODS: run([PY, os.path.join(HERE, 'patch_nso.py'), os.path.join(game, 'ExeFS', m), out, m], env={'DM_RULESET': 'touch-run-v4'}, log=os.path.join(out, m + '.patch.log'), what='改写 ' + m)
    step('改写游戏程序', lambda: patched_ready(out), patch)



    say('从游戏文件生成界面素材（输入没变会自己跳过）')
    run([PY, os.path.join(IOS, 'tools', 'gen_assets.py'), romfs], cwd=IOS, env={'DM_ROMFS_SRC': romfs}, log=os.path.join(stage, 'gen_assets.log'), what='生成界面素材')
    for name, gen in (('dm_vault_tab.h', 'gen_vault_table.py'), ('dm_hud_exp.h', 'gen_hud_exp.py')):
        txt = run([PY, os.path.join(IOS, 'tools', gen), romfs], what='生成 ' + name)
        with open(os.path.join(DM, 'host', name), 'w', encoding='utf-8') as f: f.write(txt)
    sys.path.insert(0, os.path.join(IOS, 'tools')); import shell_stubs
    say('数据表：经验 %d、部位 %d、仓库条目 %d、套装 %d、技能索引 %d' % shell_stubs.data(DM, os.path.join(stage, 'dm', 'derived.tsv')))
    hud = os.path.join(IOS, 'Gen', 'assets', 'NativeHUD')
    for d, src in (('hud_v4_ui', os.path.join(hud, 'v4', 'hud_v4_ui')), ('mapgoal', os.path.join(hud, 'mapgoal')), ('mercslots', os.path.join(hud, 'mercslots'))):
        if d + '/' in manifest: shutil.copytree(src, os.path.join(stage, d))
    ta = os.path.join(IOS, 'Sources', 'TouchArt')
    for f in sorted(os.listdir(ta)):
        if f in manifest: shutil.copyfile(os.path.join(ta, f), os.path.join(stage, f))



    say('生成覆盖层')






    astc = os.environ.get('D2RM_ASTC_PACK', '').strip(); astc = os.path.abspath(astc) if astc else ''
    if astc and not all(os.path.isfile(os.path.join(astc, f)) for f in ('pack_index.tsv', 'pack.json')): die('D2RM_ASTC_PACK 指的目录里没有 pack_index.tsv / pack.json：%s（要的是贴图包三步做完后的那个 pack 目录）' % astc)
    run([PY, os.path.join(IOS, 'tools', 'gen_romfs_overlay.py'), romfs, os.path.join(stage, 'romfs_overlay')], cwd=IOS, env={'DM_ROMFS_SRC': romfs, 'DM_ASTC_PACK': astc}, log=os.path.join(stage, 'gen_overlay.log'), what='生成覆盖层')
    extra = []
    if astc:
        if not os.path.isfile(os.path.join(stage, 'romfs_overlay_astc', 'pack.json')): die('老机型的覆盖层没生成出来（见 %s）' % os.path.join(stage, 'gen_overlay.log'))
        extra = ['romfs_overlay_astc/', 'romfs_overlay_astc_variants/']; pj = json.load(open(os.path.join(astc, 'pack.json'), encoding='utf-8'))
        say('带老机型支持：贴图包 %s（%.2f GB，SHA-256 %s…）；装机后把它里面给手机用的那个文件夹（收尾那一步生成的）拷进手机' % (astc, pj['size'] / 1e9, pj['sha256'][:12]))
    else:
        for d in ('romfs_overlay_astc', 'romfs_overlay_astc_variants'): shutil.rmtree(os.path.join(stage, d), ignore_errors=True)



    packs = os.path.join(ROOT, 'build', 'cache', 'dm', 'dmsh')
    def get_packs():
        os.makedirs(packs, exist_ok=True); e = {'DMX_ROOT': os.path.join(romfs, 'data')}
        for line in run([PY, os.path.join(HERE, 'dmx.py'), 'list', 'data:data/hd/shaders/*.bin'], env=e, what='列着色器包').splitlines():
            p = line.split('\t')[0]
            if p: run([PY, os.path.join(HERE, 'dmx.py'), 'get', p, os.path.join(packs, os.path.basename(p))], env=e, what='取着色器包 ' + p)
    step('取着色器包（162 个）', lambda: os.path.isdir(packs) and len(os.listdir(packs)) == 162, get_packs)
    xl = os.path.join(ROOT, 'build', 'gfx', 'dmsh_xlate')
    step('编译着色器翻译工具', lambda: xlate_ready(xl, z),
         lambda: place_xlate(xl, z) or run([dotnet, 'publish', os.path.join(GT, 'dmsh_xlate'), '-c', 'Release', '-o', xl, '-v', 'quiet'], what='编译着色器翻译工具'))
    chk = lambda: subprocess.run(gt('dmsh_chain.py', '--check-tools'), env=dict(os.environ, DM_ROOT=ROOT, PYTHONUTF8='1'), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not chk():
        say('本机工具与仓库登记的不同：按本机工具重新登记翻译链')
        run(gt('dmsh_chain.py', '--update-lock'), what='登记翻译链'); run(gt('dmsh_chain.py', '--header'), what='写翻译链版本')
    local = run(gt('dmsh_chain.py'), what='取翻译链版本').strip().splitlines()[-1]
    ios_out = os.path.join(ROOT, 'build', 'ios', 'dmsh_b'); mac_cache = os.path.join(ROOT, 'build', 'cache', 'metal_shader_cache')




    def shaders_done(tell=False):
        try: res = json.load(open(os.path.join(mac_cache, 'b-' + local, '_b_build.json'), encoding='utf-8'))
        except (OSError, ValueError): return False
        d = os.path.join(ios_out, 'b-' + local); have = set(os.listdir(d)) if os.path.isdir(d) else set()
        bad = [r for r in res if r['st'] != 'ok']; miss = [r['key'] for r in res if r['st'] == 'ok' and r['key'] + '_b.metallib' not in have]
        if tell and (bad or miss or not res): say('着色器 %d 个：没翻译成功 %d 个，库目录里缺 %d 个（完整输出见 %s）' % (len(res), len(bad), len(miss), os.path.join(stage, 'shaders.log')))
        return bool(res) and not bad and not miss
    step('翻译全部着色器（首次约 8 分钟）', shaders_done,
         lambda: (run(gt('dmsh_build.py', packs, mac_cache), env={'IOSOUT': ios_out, 'J': os.environ.get('J', '6'), 'MAIN': os.path.join(game, 'ExeFS', 'main')}, log=os.path.join(stage, 'shaders.log'), what='翻译着色器'), shaders_done(tell=True)))
    lib = os.path.join(stage, 'msl', 'b-' + chain); os.makedirs(os.path.dirname(lib)); shutil.copytree(os.path.join(ios_out, 'b-' + local), lib)
    if local != chain:
        old, new, n = ('"chain": "%s"' % local).encode(), ('"chain": "%s"' % chain).encode(), 0
        for f in os.listdir(lib):
            if f.endswith('.json'):
                b = open(os.path.join(lib, f), 'rb').read()
                if old in b: open(os.path.join(lib, f), 'wb').write(b.replace(old, new)); n += 1
        if not n: die('着色器说明里没找到链版本 %s（格式变了？）' % local)
    got = src_digest(os.path.join(mac_cache, 'b-' + local))
    if info.get('SHADER_SRC') and got != info['SHADER_SRC']:
        die('这台机器翻译出的着色器与壳要的不一样（摘要 %s，壳要 %s）。最常见的原因是 spirv-cross 版本不同：请换成 %s（见 docs 里的说明），'
            '然后删掉 %s 重跑。不一样的着色器装到手机上可能画错，所以不往下做' % (got, info['SHADER_SRC'], info.get('SPIRV_CROSS', '说明里写的那个版本'), os.path.join(mac_cache, 'b-' + local)))
    c1 = os.path.join(stage, 'dm', 'c1_full.tsv')
    run([PY, os.path.join(GT, 'c1_guard.py'), os.path.join(mac_cache, 'b-' + local), c1], what='出着色器下标清单')
    if local != chain:
        t = open(c1, encoding='utf-8').read(); open(c1, 'w', encoding='utf-8').write(t.replace('b-' + local, 'b-' + chain, 1))
        say('着色器库按本机工具出（链版本 %s），放进壳认的目录名 b-%s' % (local, chain))



    import guest_dylib
    fw, n = guest_dylib.write(out, os.path.join(stage, 'Frameworks'), MODS); say('游戏程序动态库：%d 字节' % n)
    for f in [m + s for m in MODS for s in ('.fixups.tsv', '.gates.tsv')] + ['bootlist.tsv', 'main.cuts.tsv']:
        if os.path.isfile(os.path.join(out, f)): shutil.copyfile(os.path.join(out, f), os.path.join(stage, 'dm', f))



    js = [f for f in os.listdir(lib) if f.endswith('.json')]; want = ('"chain": "%s"' % chain).encode()
    wrong = [f for f in js if want not in open(os.path.join(lib, f), 'rb').read()]
    if not js or wrong: die('着色器说明里的链版本与壳要的 %s 不一致：%d / %d 个不对%s' % (chain, len(wrong), len(js), '，如 ' + wrong[0] if wrong else ''))
    if ('b-' + chain) not in open(c1, encoding='utf-8').readline(): die('着色器下标清单首行的链版本不是壳要的 %s' % chain)
    say('自检：%d 个着色器说明、下标清单、目录名的链版本都是壳要的 %s；翻译出的源码摘要 %s%s' % (len(js), chain, got, '与壳一致' if info.get('SHADER_SRC') else '（这个壳没带摘要，没比）'))



    with open(os.path.join(stage, 'dm', 'pack_info.txt'), 'w', encoding='utf-8') as f: f.write('PRODUCT=%s\n' % P)
    extra = extra + ['dm/pack_info.txt']



    miss = [m for m in manifest if not (os.path.isdir(os.path.join(stage, m)) if m.endswith('/') else os.path.isfile(os.path.join(stage, m)))]
    if miss: die('有 %d 项没做出来：%s。壳的清单里要、而这份工具没有生成——壳与工具不是同一版。请用与这份仓库同一版本的壳（发布页最新的 D2RMobile-shell-build-input.zip），或把仓库更新到与壳对应的版本' % (len(miss), '、'.join(miss)))
    tmp = out_ipa + '.part'; added = 0
    with zipfile.ZipFile(tmp, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as o:
        for i in z.infolist():
            if not i.filename.startswith('Tools/'): o.writestr(i, z.read(i.filename))
        for m in manifest + extra:
            src = os.path.join(stage, m)
            files = [os.path.join(d, f) for d, _, fs in sorted(os.walk(src)) for f in sorted(fs)] if m.endswith('/') else [src]
            for f in files:
                zi = zipfile.ZipInfo(base + os.path.relpath(f, stage).replace(os.sep, '/'), date_time=(2026, 1, 1, 0, 0, 0)); zi.compress_type = zipfile.ZIP_DEFLATED
                zi.external_attr = (0o755 if os.path.basename(f) == 'Guest' else 0o644) << 16; o.writestr(zi, open(f, 'rb').read()); added += 1
    os.replace(tmp, out_ipa)
    say('好了：%s（%.0f MB，放进去 %d 个文件）。下一步：用自签工具给它签名装机（见 docs 里的说明）；这个安装包里有游戏内容，请只自己用' % (out_ipa, os.path.getsize(out_ipa) / 1048576, added))

    sp = os.environ.get('D2RM_S2ARM_PACK', '').strip()
    if sp:
        n, total = s2arm_pack(romfs, os.path.abspath(sp)); say('S2Arm 资源包：%s（%d 个档案，%.2f GB）' % (sp, n, total / 1e9))


if __name__ == '__main__': main()
