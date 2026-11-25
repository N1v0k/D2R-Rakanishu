#!/usr/bin/env python3






























import hashlib, os, shutil, struct, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__)); DM = os.path.dirname(HERE); IOS = os.path.join(DM, 'ios'); GT = os.path.join(DM, 'gfx', 'tools')
sys.path.insert(0, HERE); import pack_steps; from d2rm_pack import ROOT, PY, say, die, run, step, game_layout, place_xlate, xlate_ready


def check_env(game):
    bad = []
    for m in ('PIL', 'numpy', 'lz4', 'capstone'):
        try: __import__(m)
        except Exception: bad.append('Python 包 %s：%s -m pip install pillow numpy lz4 capstone' % (m, PY))

    if not os.environ.get('DM_MONPIC_LIB') and not any(shutil.which(c) for c in (os.environ.get('CXX') or 'clang++', 'c++')):
        bad.append('C/C++ 编译器：clang（或 gcc；环境变量 CC / CXX 可以指定别的）')
    dotnet = shutil.which('dotnet') or (os.path.expanduser('~/.dotnet/dotnet') if os.path.exists(os.path.expanduser('~/.dotnet/dotnet')) else None)
    if not dotnet: bad.append('.NET SDK（dotnet）：https://dotnet.microsoft.com/download')
    for t in ('spirv-opt', 'spirv-val'):
        if not shutil.which(t): bad.append('%s：Vulkan SDK（https://vulkan.lunarg.com/），Mac 上 brew install spirv-tools' % t)
    if shutil.which('spirv-opt') and b'--trim-capabilities' not in subprocess.run(['spirv-opt', '--help'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout:
        bad.append('spirv-opt 版本太旧（%s，没有 --trim-capabilities）：换新版 Vulkan SDK 的（spirv-opt --help 里要有这一项）；安卓 NDK 自带的那份也太旧，别让它排在 PATH 前面' % shutil.which('spirv-opt'))
    sys.path.insert(0, GT); import pack_builtin_queries
    try: pack_builtin_queries.find_glslc(None)
    except SystemExit: bad.append('glslc：Vulkan SDK 或安卓 NDK 里都有（Mac 上 brew install shaderc），也可以用环境变量 GLSLC 指过去')
    for f in ('ExeFS/main', 'ExeFS/sdk', 'RomFS/data/build_manifest.psv'):
        if not os.path.isfile(os.path.join(game, f)): bad.append('游戏文件 %s（在 %s 下没找到）' % (f, game))
    if bad: die('还缺 %d 项：\n  - %s' % (len(bad), '\n  - '.join(bad)))
    if not os.environ.get('DOTNET_ROOT'):
        import re
        r = subprocess.run([dotnet, '--list-runtimes'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        m = re.search(r'^Microsoft\.NETCore\.App \S+ \[(.*)[/\\]shared[/\\]Microsoft\.NETCore\.App\]', r.stdout.decode('utf-8', 'replace'), re.M)
        if m: os.environ['DOTNET_ROOT'] = m.group(1)
    return dotnet


def warm_seed_records(data):


    i, records, keys = 0, 0, set()
    while i < len(data):
        if i + 8 > len(data) or struct.unpack_from('<I', data, i)[0] != 0x57504c58: raise ValueError('第 %d 字节不是一条记录的开头' % i)
        stages, = struct.unpack_from('<I', data, i + 4); i += 8
        if stages > 5: raise ValueError('一条记录有 %d 个阶段' % stages)
        for _ in range(stages):
            if i + 4 > len(data): raise ValueError('记录不完整')
            n, = struct.unpack_from('<I', data, i); i += 4
            if n > 256 or i + n > len(data): raise ValueError('着色器键长 %d' % n)
            keys.add(data[i:i + n].decode('ascii', 'replace')); i += n
        if i + 4 > len(data): raise ValueError('记录不完整')
        count, = struct.unpack_from('<I', data, i); i += 4
        if count > 4096 or i + 4 * count > len(data): raise ValueError('管线键 %d 字' % count)
        i += 4 * count; records += 1
    return records, keys


def warm_seed_load(path):



    if not path: return None
    if not os.path.isfile(path): die('没有这个预热种子：%s' % path)
    data = open(path, 'rb').read()
    try: records, keys = warm_seed_records(data)
    except ValueError as e: die('预热种子 %s 读不了：%s' % (path, e))
    if not records: die('预热种子 %s 是空的' % path)
    return path, data, records, keys


def warm_seed(seed, shaders):




    if not seed: return None
    path, data, records, keys = seed
    have = {f[:-4] for f in os.listdir(shaders) if f.endswith('.xgs')}
    missing = sorted(keys - have)
    if len(missing) == len(keys): die('预热种子 %s 的 %d 个着色器键在这套包里一个都没有：它不是用这版着色器录的' % (path, len(keys)))
    if missing: say('预热种子里 %d 个着色器键不在这套包里（例如 %s）：那些管线预热时建不了，只是跳过' % (len(missing), missing[0]))
    shutil.copyfile(path, os.path.join(shaders, 'vk-pipeline-list.seed'))
    digest = hashlib.sha256(data).hexdigest()[:16]
    say('预热种子：%d 条管线、%d 个着色器（%s，来自 %s）' % (records, len(keys), digest, path))
    return digest


def main():
    args = sys.argv[1:]; T = 'android'; seed = None
    while args and args[0].startswith('--') and len(args) >= 2:
        if args[0] == '--target': T = args[1]
        elif args[0] == '--warm-seed': seed = os.path.abspath(args[1])
        else: break
        args = args[2:]
    if len(args) != 2 or T not in ('android', 'ohos'): sys.exit('用法：d2rm_android.py [--target ohos] [--warm-seed <预热种子>] <游戏文件目录> <输出目录>')
    seed = warm_seed_load(seed)
    game, dst = (os.path.abspath(x) for x in args)
    for st in (sys.stdout, sys.stderr):
        try: st.reconfigure(errors='replace')
        except Exception: pass
    if not os.path.isdir(game): die('没有这个游戏文件目录：%s' % game)
    game = game_layout(game)
    dotnet = check_env(game)
    P = open(os.path.join(DM, 'PRODUCT')).readline().strip()
    romfs = os.path.join(game, 'RomFS'); stage = os.path.join(ROOT, 'build', 'pack', T); shutil.rmtree(stage, ignore_errors=True); os.makedirs(stage)
    final = os.path.join(dst, 'android') if T == 'android' else os.path.join(dst, 'harmony', 'dm')
    say('产物代号 %s；做好的文件夹放在 %s' % (P, final))




    qa_combat = T == 'android' and os.environ.get('DM_QA_COMBAT') == '1'
    build = pack_steps.Build(dm=DM, exefs=os.path.join(game, 'ExeFS'), romfs=romfs, stage=stage, work=ROOT, python=PY, target=T,
                             deswizzle=os.environ.get('DM_DESWIZZLE_LIB') or None, qa_combat=qa_combat, monpic=os.environ.get('DM_MONPIC_LIB') or None,
                             tokens=os.cpu_count() or 1)
    t0 = time.time()
    def report(event, name, *values):
        if event in ('start', 'done'): say('%s：%s' % (name, '开始' if event == 'start' else '完成'))



    expect = pack_steps.load_expect(os.path.join(HERE, 'pack_steps.tsv')) if T == 'android' and not qa_combat else None
    try: pack_steps.run(build, pack_steps.steps_for(T), report=report, expect=expect)
    except pack_steps.StepError as e: die('%s (log %s)' % (e, os.path.join(ROOT, 'build', 'pack_steps', e.step + '.log')))
    say('改写、素材、覆盖层与 App 素材：%d 秒' % (time.time() - t0))
    hud = os.path.join(IOS, 'Gen', 'assets', 'NativeHUD', 'v4', 'hud_v4_ui')
    caps = []
    if T == 'android':

        caps.append('quickmenu1')
    if T == 'ohos':
        os.makedirs(os.path.join(stage, 'ui-art'))
        for n in ['lbl_main.png'] + ['lbl_f%d.png' % i for i in range(1, 9)] + ['sil_dm_native_skill_%d.png' % i for i in range(9)]:
            if not os.path.isfile(os.path.join(hud, n)): die('底栏素材里缺 %s（%s）' % (n, hud))
            shutil.copyfile(os.path.join(hud, n), os.path.join(stage, 'ui-art', n))



    packs = os.path.join(ROOT, 'build', 'cache', 'dm', 'dmsh')
    def get_packs():
        os.makedirs(packs, exist_ok=True); e = {'DMX_ROOT': os.path.join(romfs, 'data')}
        for line in run([PY, os.path.join(HERE, 'dmx.py'), 'list', 'data:data/hd/shaders/*.bin'], env=e, what='列着色器包').splitlines():
            p = line.split('\t')[0]
            if p: run([PY, os.path.join(HERE, 'dmx.py'), 'get', p, os.path.join(packs, os.path.basename(p))], env=e, what='取着色器包 ' + p)
    step('取着色器包（162 个）', lambda: os.path.isdir(packs) and len(os.listdir(packs)) == 162, get_packs)
    xl = os.path.join(ROOT, 'build', 'gfx', 'dmsh_xlate'); exe = os.path.join(xl, 'dmsh_xlate.exe' if os.name == 'nt' else 'dmsh_xlate')
    step('编译着色器翻译工具', lambda: os.path.isfile(exe) and xlate_ready(xl),
         lambda: place_xlate(xl) or run([dotnet, 'publish', os.path.join(GT, 'dmsh_xlate'), '-c', 'Release', '-o', xl, '-v', 'quiet'], what='编译着色器翻译工具'))
    sh = os.path.join(stage, 'shaders'); t0 = time.time(); say('翻译全部着色器（约 2 分钟）')
    run([PY, os.path.join(GT, 'dmsh_vk.py'), packs, sh], env={'XLATE': exe, 'MAIN': os.path.join(game, 'ExeFS', 'main'), 'WORK': stage}, log=os.path.join(stage, 'shaders.log'), what='翻译着色器')
    q = os.path.join(stage, 'queries'); run([PY, os.path.join(GT, 'pack_builtin_queries.py'), '--output', q], what='编查询核')
    for f in os.listdir(q):
        if f.endswith('.xgs'): shutil.move(os.path.join(q, f), os.path.join(sh, f))
    n = len([f for f in os.listdir(sh) if f.endswith('.xgs')]); say('着色器 %d 个（%d 秒）' % (n, time.time() - t0))


    seed_digest = warm_seed(seed, sh)



    with open(os.path.join(stage, 'pack_info.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('PRODUCT=%s\nTARGET=%s\nMADE=%s\n' % (P, T, time.strftime('%Y-%m-%dT%H:%M:%S')))
        if caps: f.write('CAPS=%s\n' % ','.join(caps))
        if seed_digest: f.write('WARM_SEED=%s\n' % seed_digest)
    part = final + '.part'
    shutil.rmtree(part, ignore_errors=True); os.makedirs(part)
    for x in ('out', 'shaders', 'overlay') + (('ui-art',) if T == 'ohos' else ('appart',)): shutil.copytree(os.path.join(stage, x), os.path.join(part, x))
    for x in ('derived.tsv', 'pack_info.txt'): shutil.copyfile(os.path.join(stage, x), os.path.join(part, x))

    if T == 'android':
        for rel, full in sorted(pack_steps.outputs(build, 'pictures').items()):
            dst = os.path.join(part, pack_steps.installed(rel))
            if os.path.lexists(dst): die('the enemy picture %s collides with an app art file' % rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True); shutil.copyfile(full, dst)




    if T == 'android' and os.path.isdir(os.path.join(stage, 'overlay_variants')): shutil.copytree(os.path.join(stage, 'overlay_variants'), os.path.join(part, 'overlay_variants'))
    shutil.rmtree(final, ignore_errors=True); os.replace(part, final)
    size = sum(os.path.getsize(os.path.join(d, f)) for d, _, fs in os.walk(final) for f in fs)
    if T == 'ohos': say('好了：%s（%.0f MB）。在手机上建一个「D2R Mobile」文件夹，把这个 dm 文件夹和游戏资源（%s 整个拷过去，叫 data）并排放进去，'
                        '打开应用按提示选「D2R Mobile」导入。这里面有游戏内容，请只自己用' % (final, size / 1048576, os.path.join(romfs, 'data')))
    else: say('Ready: %s (%.0f MB). Copy android/ into Download/S2X/D2R Mobile/r4k4n1shu/. '
              'Shared archives stay in Download/S2X/D2R Mobile/data/ (source: %s). Keep the pack private.'
              % (final, size / 1048576, os.path.join(romfs, 'data')))


if __name__ == '__main__': main()
