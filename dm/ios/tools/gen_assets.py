#!/usr/bin/env python3


















import hashlib, os, shutil, subprocess, sys, time
import gen_paths
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
HERE = os.path.dirname(os.path.abspath(__file__))
STEPS = [('加宽储藏箱', ['gen_widestash_art.py', '{R}']), ('编年史盖板', ['gen_vault_cover.py']), ('百宝囊框', ['gen_pouch_art.py', '{R}']),
         ('选角右侧面板', ['gen_fe_rightpanel.py', '{R}']), ('guide card back', ['gen_build_card_art.py', '{R}']), ('小地图终点', ['gen_map_goal_art.py', '{R}']), ('佣兵面板底图', ['gen_merc_art.py', '{R}']),
         ('技能图集与索引', ['native_art/generate.py', '{R}']), ('底部 UI v4', ['gen_hud_v4.py', '{R}'])]


def python_command(script, *arguments):


    return [sys.executable, '-X', 'utf8', os.path.join(HERE, '..', '..', 'tools', 'winlf', 'run.py'), script, *arguments]


def stamp(R, overlay_only=False):
    h = hashlib.sha256(open(os.path.join(R, 'data', 'build_manifest.psv'), 'rb').read())
    h.update(b'\0overlay-only\0' if overlay_only else b'\0full-assets\0')





    for d, ds, fs in os.walk(HERE):
        ds[:] = sorted(x for x in ds if x != '__pycache__')
        for f in sorted(x for x in fs if not x.endswith('.pyc')): h.update(os.path.relpath(os.path.join(d, f), HERE).encode()); h.update(open(os.path.join(d, f), 'rb').read())
    pc = os.environ.get('DM_PC_DATA') or os.path.join(DM_ROOT, 'data/assets-src/raw')
    h.update(('%s|%s|%s' % (pc, os.path.isdir(pc), os.environ.get('DM_REQUIRE_PC_ART', ''))).encode())
    return h.hexdigest()


def ensure(R, overlay_only=False):

    root = gen_paths.gen_dir(); st = os.path.join(root, '.stamp'); lock = os.path.join(root, '.gen_lock')
    v4 = os.path.join(root, 'NativeHUD', 'v4')
    outs = [os.path.join(v4, 'base.png'), os.path.join(v4, 'hud.json'), os.path.join(v4, 'hud_v4_ui', 'ui.json')]
    full_outs = outs + [os.path.join(HERE, '..', 'Sources', 'TouchArt', 'dm_native_skill_0.png'),
                        os.path.join(HERE, '..', 'Sources', 'dm_touch_native_art.c'),
                        os.path.join(v4, 'hud_v4_ui', 'sil_dm_native_skill_0.png')]
    if not overlay_only: outs = full_outs





    want = stamp(R, overlay_only=overlay_only); full_want = stamp(R) if overlay_only else want
    def fresh():
        if not os.path.isfile(st): return False
        have = open(st).read()
        if have == full_want: return all(map(os.path.isfile, full_outs))
        return have == want and all(map(os.path.isfile, outs))
    if fresh(): return False
    t0 = time.time()
    while True:
        try: os.mkdir(lock); break
        except FileExistsError:
            try: pid = int(open(os.path.join(lock, 'pid')).read())
            except (OSError, ValueError): pid = 0
            if pid:
                try: os.kill(pid, 0)
                except ProcessLookupError: shutil.rmtree(lock, ignore_errors=True); continue
            if time.time() - t0 > 600: sys.exit('🔴 gen_assets：等别的进程生成等了 10 分钟（锁 %s，pid %s）' % (lock, pid))
            time.sleep(0.5)
    try:
        open(os.path.join(lock, 'pid'), 'w').write(str(os.getpid()))
        if fresh(): return False
        if os.path.exists(st): os.remove(st)
        run_steps(R, overlay_only=overlay_only); open(st, 'w').write(want); return True
    finally: shutil.rmtree(lock, ignore_errors=True)


def run_steps(R, overlay_only=False):
    for name, cmd in STEPS:
        if overlay_only and cmd[0] == 'native_art/generate.py': continue
        argv = python_command(os.path.join(HERE, cmd[0]), *[c.replace('{R}', R) for c in cmd[1:]])
        if overlay_only and cmd[0] == 'gen_hud_v4.py': argv.append('--overlay-only')
        t0 = time.time(); r = subprocess.run(argv, capture_output=True, text=True)
        if r.returncode: sys.exit('🔴 生成「%s」失败：\n%s%s' % (name, r.stdout[-1500:], r.stderr[-1500:]))
        tip = [l for l in r.stdout.splitlines() if l.startswith(('没找到', '底栏素材'))]
        print('%s：%.1f 秒%s' % (name, time.time() - t0, ('（' + tip[0] + '）') if tip else ''))


def main():
    R = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('DM_ROMFS_SRC') or os.path.join(DM_ROOT, 'data/switch/3.3.94012/Update/RomFS')
    if not os.path.isfile(os.path.join(R, 'data', 'build_manifest.psv')):
        sys.exit('🔴 没找到游戏文件：%s/data/build_manifest.psv。把 Switch 版 3.3.94012 的 Update/RomFS 放到这里，或用环境变量 DM_ROMFS_SRC 指过去'
                 '（见 dm/README.md「从你的游戏文件生成资源」）' % R)
    if not ensure(R): print('原版衍生资源：输入没变，沿用 %s' % gen_paths.gen_dir())


if __name__ == '__main__':
    main()
