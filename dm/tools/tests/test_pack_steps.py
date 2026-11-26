#!/usr/bin/env python3











import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

TOOLS = Path(__file__).resolve().parents[1]
DM = TOOLS.parent
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(DM / 'android' / 'python'))
import pack_steps





TRACE = '''import os, time
def span(step, threads=1):
    trace = os.path.join(os.environ['DM_ROOT'], 'trace')
    if not os.path.exists(os.path.join(os.environ['DM_ROMFS_SRC'], 'data', 'TRACE')):
        return
    t0 = time.time(); time.sleep(0.4)
    with open(trace, 'a') as f: f.write('%s %d %.4f %.4f\\n' % (step, threads, t0, time.time()))
def busy(step, out):
    import threading
    if not os.path.exists(os.path.join(os.environ['DM_ROMFS_SRC'], 'data', 'BUSY')):
        return
    open(os.path.join(os.environ['DM_ROOT'], 'busy-' + step), 'w').write(str(os.getpid()))
    def spin():
        while True: pass
    for _ in range(2): threading.Thread(target=spin, daemon=True).start()
    os.makedirs(out, exist_ok=True)
    while True:
        open(os.path.join(out, 'alive'), 'w').write(str(time.time())); time.sleep(0.05)
'''
STUBS = {
    'tools/steptrace.py': TRACE,
    'tools/patch_nso.py': '''import hashlib, os, sys
exe, out, m = sys.argv[1:4]
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)))); import steptrace; steptrace.span('rewrite')
open(os.path.join(out, m + '.patched.bin'), 'w').write(open(exe).read() + os.environ['DM_TARGET'] + os.environ['DM_RULESET'])
open(os.path.join(out, m + '.gates.tsv'), 'w').write(m)
open(os.path.join(out, 'build_src.sha256'), 'w').write(hashlib.sha256(open(__file__, 'rb').read()).hexdigest() + '  patch_nso.py\\n')
open(os.path.join(os.environ['DM_ROOT'], 'patch-runs'), 'a').write(m + '\\n')
''',
    'ios/tools/gen_paths.py': '''import os
HERE = os.path.dirname(os.path.abspath(__file__))
def gen_dir(*sub):
    p = os.path.normpath(os.path.join(os.environ.get('DM_GEN_ASSETS') or os.path.join(HERE, '..', 'Gen', 'assets'), *sub)); os.makedirs(p, exist_ok=True); return p
''',
    'ios/tools/gen_assets.py': '''import os, sys, gen_paths
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools')); import steptrace; steptrace.span('art')
seed = open(os.path.join(sys.argv[1], 'data', 'seed')).read()
for i in range(3): open(os.path.join(gen_paths.gen_dir('NativeHUD', 'v4', 'hud_v4_ui'), 'p%d.png' % i), 'w').write(seed + os.environ['DM_ROOT'] + os.environ['DM_DESWIZZLE_LIB'])
''',
    'ios/tools/native_art/generate.py': '''import os, sys
out = sys.argv[3]; os.makedirs(os.path.join(out, 'TouchArt'), exist_ok=True)
open(os.path.join(out, 'TouchArt', 'skill.png'), 'w').write(open(os.path.join(sys.argv[1], 'data', 'seed')).read())
open(os.path.join(out, 'dm_touch_native_art.c'), 'w').write('index')
''',
    'ios/tools/gen_vault_table.py': 'import sys; print("vault " + open(sys.argv[1] + "/data/seed").read())\n',
    'ios/tools/gen_hud_exp.py': 'import sys; print("exp " + open(sys.argv[1] + "/data/seed").read())\n',
    'ios/tools/shell_stubs.py': '''import os
def data(_, derived, gen):
    open(derived, 'w').write(open(os.path.join(gen, 'dm_vault_tab.h')).read())
    return 1, 2, 3, 4, 5
''',
    'ios/tools/gen_romfs_overlay.py': '''import os, sys, gen_paths
romfs, out = sys.argv[1:3]
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools')); import steptrace; steptrace.span('overlay')
if os.path.exists(os.path.join(romfs, 'data', 'FAIL')): sys.exit('overlay: the game files lack a table')
for d in (out, out + '_variants'):
    os.makedirs(d, exist_ok=True)
    open(os.path.join(d, 'ctrl_touch'), 'w').write(open(os.path.join(romfs, 'data', 'seed')).read() + os.environ['DM_OVERLAY_NO_SHARE'] + os.environ.get('DM_OVERLAY_NO_ICLOUD', '-'))
''',
    'android/tools/stage_touch_assets.py': '''import os, shutil
def stage(native_art, hud, overlay):
    os.makedirs(os.path.join(overlay, 'touchart'), exist_ok=True)
    shutil.copyfile(os.path.join(native_art, 'skill.png'), os.path.join(overlay, 'touchart', 'skill.png'))
    shutil.copyfile(os.path.join(hud, 'p0.png'), os.path.join(overlay, 'touchart', 'p0.png'))
''',
    'android/tools/gen_app_art.py': '''import os, time
def stage(romfs, appart):
    import steptrace; steptrace.span('app')
    os.makedirs(appart, exist_ok=True)
    if os.path.exists(os.path.join(romfs, 'data', 'SLOW')):
        open(os.path.join(romfs, 'data', 'SLOW'), 'w').write(str(os.getpid())); time.sleep(120)
    for n in ('icon', 'font'): open(os.path.join(appart, n), 'w').write(open(os.path.join(romfs, 'data', 'seed')).read() + n)
def missing(appart): return [n for n in ('icon', 'font') if not os.path.isfile(os.path.join(appart, n))]
''',
    'android/tools/gen_mon_pics.py': '''import os, json
def stage(romfs, out, main, workers=None):
    import steptrace; steptrace.span('pictures', workers or 0); steptrace.busy('pictures', out)
    if os.path.exists(os.path.join(romfs, 'data', 'FAILPIC')): raise SystemExit('appart: act bosses without a picture: x')
    os.makedirs(os.path.join(out, 'mon', 's'), exist_ok=True)
    seed = open(os.path.join(romfs, 'data', 'seed')).read() + open(main).read() + os.environ.get('DM_MONPIC_LIB', '-')
    for rel in ('mon/a.webp', 'mon/s/a.webp'): open(os.path.join(out, rel), 'w').write(seed + rel)
    json.dump({'pictures': {'a': 'a'}, 'none': {}}, open(os.path.join(out, 'mon.json'), 'w'))
''',
    'gfx/tools/dmsh_native.py': '''import os, sys
def build(gfx, exefs, data, stage, queries, lib, xlate, flush, sync, workers):
    sys.path.insert(0, os.path.join(gfx, '..', '..', 'tools')); import steptrace; steptrace.span('shaders', workers)
    steptrace.busy('shaders', os.path.join(stage, 'shaders'))
    os.makedirs(os.path.join(stage, 'shaders'), exist_ok=True)
    open(os.path.join(stage, 'shaders', 'k.xgs'), 'w').write(open(os.path.join(exefs, 'main')).read() + lib + xlate)
''',
    'android/tools/gen_style.py': 'import os\ndef stage(romfs, appart): open(os.path.join(appart, "style.json"), "w").write("{}")\n',
}
for _n in ('gen_recipe_data', 'gen_effect_data', 'gen_build_names', 'gen_glossary_data', 'gen_book_data'):
    STUBS['android/tools/%s.py' % _n] = 'import os\ndef stage(romfs, appart): open(os.path.join(appart, "%s.json"), "w").write(open(os.path.join(romfs, "data", "seed")).read())\n' % _n


def tree(root):

    dm = Path(root) / 'dm'
    for rel in ('tools/pack_steps.py', 'tools/d2rm_pack.py', 'tools/winlf/sitecustomize.py', 'tools/winlf/run.py', 'host/game_addrs.def', 'PRODUCT'):
        (dm / rel).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(DM / rel, dm / rel)
    for rel, text in STUBS.items():
        (dm / rel).parent.mkdir(parents=True, exist_ok=True)
        (dm / rel).write_text(text)
    return dm


def game(root, seed):
    g = Path(root)
    (g / 'ExeFS').mkdir(parents=True)
    (g / 'RomFS' / 'data').mkdir(parents=True)
    for m in ('main', 'sdk'):
        (g / 'ExeFS' / m).write_text(seed + m)
    (g / 'RomFS' / 'data' / 'seed').write_text(seed)
    return g


def build(root, seed, own_tree=True, shared_tree=None, **more):
    root = Path(root)
    dm = tree(root / 'tree') if own_tree else shared_tree
    g = game(root / 'game', seed)
    return pack_steps.Build(dm=str(dm), exefs=str(g / 'ExeFS'), romfs=str(g / 'RomFS'), stage=str(root / 'stage'), work=str(root / 'work'),
                            python=sys.executable, deswizzle=str(root / 'libhelper.so'), monpic=str(root / 'libmonpic.so'), **more)


ALL = pack_steps.steps_for('android') + ['shaders']


def with_shaders(b, **more):

    return pack_steps.Build(**dict(json.loads(b.to_json()), xlate=b.work + '/libx.so', shader_lib=b.work + '/libw.so', queries=b.work, **more))


def snapshot(stage):
    out = {}
    for d, _, fs in os.walk(stage):
        for f in fs:
            p = os.path.join(d, f)
            with open(p, 'rb') as fh:
                out[os.path.relpath(p, stage)] = hashlib.sha256(fh.read()).hexdigest()
    return out


def expect_rows(b):
    env = b.env(dict(os.environ))
    rows = ['# step\tprogress files\toutput files\toutput path digest']
    for name in pack_steps.STEPS:
        paths = list(pack_steps.outputs(b, name))
        rows.append('%s\t%d\t%d\t%s' % (name, len(pack_steps.watched(b, name, env)), len(paths), pack_steps.path_digest(paths)))
    return '\n'.join(rows) + '\n'


def cli(b, steps, env=None):

    err = open(os.path.join(b.work, 'runner.err'), 'w')
    p = subprocess.Popen([sys.executable, os.path.join(b.dm, 'tools', 'pack_steps.py'), 'run', b.to_json()] + steps,
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err, text=True, cwd=b.work, env=env)
    err.close()
    return p


def finish(p, b):

    out = p.stdout.read()
    p.wait(timeout=60)
    p.stdin.close()
    p.stdout.close()
    with open(os.path.join(b.work, 'runner.err')) as f:
        return out, f.read()


class PackStepsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        os.environ.pop('DM_GEN_ASSETS', None)

    def tearDown(self):
        self.tmp.cleanup()

    def test_lanes_and_serial_give_the_same_files(self):
        b = build(self.root / 'a', 'seed')
        pack_steps.run(b, pack_steps.steps_for('android'))
        lanes = snapshot(b.stage)
        shutil.rmtree(b.stage)
        shutil.rmtree(Path(b.dm) / 'ios' / 'Gen')
        pack_steps.run(b, pack_steps.steps_for('android'), serial=True)
        self.assertEqual(len(lanes), 5 + 4 + 1 + 3 + 1 + 8 + 3 + 5)
        self.assertEqual(snapshot(b.stage), lanes)

    def test_two_builds_in_one_interpreter_keep_apart(self):
        x, y = build(self.root / 'x', 'first'), build(self.root / 'y', 'second')
        pack_steps.run(x, pack_steps.steps_for('android'))
        before = snapshot(x.stage)
        pack_steps.run(y, pack_steps.steps_for('android'))
        self.assertEqual(snapshot(x.stage), before, 'the second build left the first one untouched')
        self.assertIn('second', (Path(y.stage) / 'derived.tsv').read_text())
        art = (Path(y.dm) / 'ios' / 'Gen' / 'assets' / 'NativeHUD' / 'v4' / 'hud_v4_ui' / 'p0.png').read_text()
        self.assertIn(y.work, art)
        self.assertIn(y.deswizzle, art)
        self.assertNotIn(x.work, art)
        pack_steps.run(x, pack_steps.steps_for('android'))
        self.assertEqual(snapshot(x.stage), before, 'the first build again gives the same files')

    def test_build_environment_comes_from_the_build(self):
        b = build(self.root / 'e', 's')
        env = b.env({'DM_ROOT': '/elsewhere', 'DM_DESWIZZLE_LIB': '/other.so', 'KEEP': '1'})
        self.assertEqual((env['DM_ROOT'], env['DM_DESWIZZLE_LIB'], env['DM_ROMFS_SRC'], env['KEEP']), (b.work, b.deswizzle, b.romfs, '1'))
        self.assertNotIn('DM_DESWIZZLE_LIB', pack_steps.Build(**dict(json.loads(b.to_json()), deswizzle=None)).env({'DM_DESWIZZLE_LIB': '/x'}))
        self.assertEqual(pack_steps.Build.from_json(b.to_json()), b)
        with self.assertRaises(ValueError):
            pack_steps.Build.from_json(json.dumps(dict(json.loads(b.to_json()), extra=1)))
        with self.assertRaises(ValueError):
            pack_steps.Build(**dict(json.loads(b.to_json()), stage='relative'))
        self.assertEqual(env['DM_MONPIC_LIB'], b.monpic)
        with self.assertRaises(ValueError):
            pack_steps.lanes(['rewrite', 'icons'])
        for bad in ({'mode': 'fast'}, {'tokens': 0}, {'shader_workers': 0}, {'picture_threads': True}):
            with self.assertRaises(ValueError):
                pack_steps.Build(**dict(json.loads(b.to_json()), **bad))
        self.assertEqual(pack_steps.lanes(['shaders', 'art', 'rewrite', 'pictures']), [['rewrite'], ['art'], ['pictures'], ['shaders']],
                         'the shader and the picture steps are lanes of their own')
        self.assertEqual(pack_steps.lanes(ALL, serial=True), [['shaders', 'pictures', 'rewrite', 'art', 'overlay', 'app']], 'serial: heaviest first')
        self.assertNotIn('shaders', pack_steps.steps_for('android'), 'the PC pack tool makes the shaders with its tools')
        self.assertIn('pictures', pack_steps.steps_for('android'))
        self.assertNotIn('pictures', pack_steps.steps_for('ohos'))

        self.assertEqual([pack_steps.installed(p) for p in ('monpics/mon/a.webp', 'monpics/mon.json', 'gen/dm_vault_tab.h', 'appart/x', 'derived.tsv')],
                         ['appart/mon/a.webp', 'appart/mon.json', None, 'appart/x', 'derived.tsv'])

    def test_failure_stops_the_run_and_a_retry_finishes(self):
        b = build(self.root / 'f', 'seed')
        fail = Path(b.romfs) / 'data' / 'FAIL'
        fail.write_text('')
        events = []
        with self.assertRaises(pack_steps.StepError) as e:
            pack_steps.run(b, pack_steps.steps_for('android'), report=lambda *a: events.append(a))
        self.assertEqual((e.exception.step, e.exception.ident), ('overlay', 'PB-STEP-overlay'))
        self.assertIn('lack a table', e.exception.detail)
        self.assertEqual([a for a in events if a[0] == 'error'], [('error', 'overlay', 'PB-STEP-overlay', e.exception.detail)])
        self.assertFalse((Path(b.stage) / '.units' / 'overlay.tsv').exists())
        fail.unlink()
        pack_steps.run(b, ['overlay'])
        clean = build(self.root / 'c', 'seed', own_tree=False, shared_tree=Path(b.dm))
        clean = pack_steps.Build(**dict(json.loads(clean.to_json()), work=b.work, deswizzle=b.deswizzle, monpic=b.monpic))
        done = snapshot(b.stage)
        pack_steps.run(clean, pack_steps.steps_for('android'))
        self.assertEqual({k: v for k, v in snapshot(clean.stage).items()}, done, 'the retry gives the files of a clean build')

    def test_command_line_protocol_and_file_sets(self):
        b = build(self.root / 'p', 'seed')
        pack_steps.run(b, pack_steps.steps_for('android'))
        (Path(b.dm) / 'tools' / 'pack_steps.tsv').write_text(expect_rows(b))
        p = cli(b, ['app', 'rewrite', 'art', 'overlay'])
        out, err = finish(p, b)
        self.assertEqual(p.returncode, 0, out + err)
        lines = [l.split('\t') for l in out.splitlines()]
        self.assertEqual(sorted(l[1] for l in lines if l[0] == 'start'), ['app', 'art', 'overlay', 'rewrite'])
        self.assertTrue(all(len(l) == 3 and l[2].isdigit() for l in lines if l[0] == 'start'))
        for l in lines:
            self.assertIn(l[0], ('start', 'file', 'done'))
            if l[0] == 'done':
                self.assertEqual(l[2], hashlib.sha256((Path(b.stage) / '.units' / (l[1] + '.tsv')).read_bytes()).hexdigest())
            if l[0] == 'file':
                self.assertLessEqual(int(l[2]), int(expect_rows(b).split(l[1] + '\t')[1].split('\t')[0]))
        order = [l[1] for l in lines if l[0] == 'done']
        self.assertLess(order.index('art'), order.index('overlay'))

        rows = [r.split('\t') for r in expect_rows(b).splitlines()]
        rows = [r[:2] + [str(int(r[2]) + 1)] + r[3:] if r[0] == 'app' else r for r in rows]
        (Path(b.dm) / 'tools' / 'pack_steps.tsv').write_text(''.join('\t'.join(r) + '\n' for r in rows))
        p = cli(b, ['app'])
        out, _ = finish(p, b)
        self.assertEqual(p.returncode, 1)
        self.assertEqual(out.splitlines()[-1].split('\t')[:3], ['error', 'app', 'PB-STEP-app'])

    def test_windows_branches(self):
        self.assertEqual(pack_steps.spawn_options(windows=True), {'creationflags': 0x200})
        self.assertEqual(pack_steps.spawn_options(windows=False), {'start_new_session': True})
        calls = []
        pack_steps.kill_tree(4242, windows=True, run=lambda cmd, **kw: calls.append(cmd))
        self.assertEqual(calls, [['taskkill', '/T', '/F', '/PID', '4242']])
        pack_steps._sync_dir(str(self.root / 'no such folder'), windows=True)
        synced, fsynced = [], []
        pack_steps.flush(['a', 'b'], sync=None, fsync=fsynced.append)
        self.assertEqual(fsynced, ['a', 'b'])
        pack_steps.flush(['a', 'b'], sync=lambda: synced.append(1), fsync=fsynced.append)
        self.assertEqual((synced, fsynced), ([1], ['a', 'b']))

        b = build(self.root / 'w', 'seed')
        seen = []
        real = pack_steps._fsync_file
        old = pack_steps.flush.__defaults__
        pack_steps.flush.__defaults__ = (None, lambda p: (seen.append(p), real(p)))
        try:
            pack_steps.run(b, pack_steps.steps_for('android'))
        finally:
            pack_steps.flush.__defaults__ = old
        outs = [p for n in pack_steps.STEPS for p in pack_steps.outputs(b, n).values()]
        self.assertEqual(sorted(set(seen) & set(outs)), sorted(outs))

    def test_interrupted_rewrite_cache_is_redone(self):
        b = build(self.root / 'r', 'seed')
        out = Path(b.work) / 'build' / 'cache' / 'dm' / 'out' / (pack_steps.product(b) + '-android')
        out.mkdir(parents=True)
        tool = Path(b.dm) / 'tools' / 'patch_nso.py'

        for m in ('main', 'sdk'):
            (out / (m + '.patched.bin')).write_text('half')
            (out / (m + '.gates.tsv')).write_text(m)
        (out / 'build_src.sha256').write_text(hashlib.sha256(tool.read_bytes()).hexdigest() + '  patch_nso.py\n')
        pack_steps.run(b, ['rewrite'])
        self.assertEqual((Path(b.stage) / 'out' / 'main.patched.bin').read_text(), 'seedmainandroidtouch-run-v4')
        self.assertFalse((Path(b.stage) / 'out' / pack_steps.REWRITE_DONE).exists())
        runs = (Path(b.work) / 'patch-runs').read_text().split()
        self.assertEqual(runs, ['main', 'sdk'])

        shutil.rmtree(b.stage)
        pack_steps.run(b, ['rewrite'])
        self.assertEqual((Path(b.work) / 'patch-runs').read_text().split(), ['main', 'sdk'])
        self.assertEqual((Path(b.stage) / 'out' / 'main.patched.bin').read_text(), 'seedmainandroidtouch-run-v4')

    def test_closing_the_input_kills_the_running_steps(self):
        b = build(self.root / 'k', 'seed')
        pack_steps.run(b, pack_steps.steps_for('android'))
        (Path(b.dm) / 'tools' / 'pack_steps.tsv').write_text(expect_rows(b))
        slow = Path(b.romfs) / 'data' / 'SLOW'
        slow.write_text('')
        p = cli(b, ['app'])
        deadline = time.time() + 60
        while not slow.read_text() and time.time() < deadline:
            time.sleep(0.05)
        pid = int(slow.read_text())
        t0 = time.time()
        p.stdin.close()
        p.wait(timeout=20)
        p.stdout.close()
        self.assertLess(time.time() - t0, 10)
        self.assertNotEqual(p.returncode, 0)
        deadline = time.time() + 10
        while time.time() < deadline:
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                break
            time.sleep(0.05)
        with self.assertRaises(ProcessLookupError):
            os.kill(pid, 0)


def spans(b):

    out = []
    for line in (Path(b.work) / 'trace').read_text().splitlines():
        step, threads, t0, t1 = line.split()
        out.append((step, int(threads), float(t0), float(t1)))
    return out


def cost(step, threads):
    return threads + 1 if step == 'shaders' else threads if step == 'pictures' else 1


def gone(pid):

    try:
        with open('/proc/%d/stat' % pid) as f:
            return f.read().rsplit(')', 1)[1].split()[0] == 'Z'
    except FileNotFoundError:
        return True


class TokensTest(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        os.environ.pop('DM_GEN_ASSETS', None)

    def tearDown(self):
        self.tmp.cleanup()

    def traced(self, name, **plan):
        b = with_shaders(build(self.root / name, 'seed'), **plan)
        pack_steps.run(b, ALL)
        (Path(b.dm) / 'tools' / 'pack_steps.tsv').write_text(expect_rows(b))
        shutil.rmtree(b.stage)
        shutil.rmtree(Path(b.work) / 'build' / 'cache')
        (Path(b.romfs) / 'data' / 'TRACE').write_text('')
        return b

    def test_running_steps_stay_within_the_tokens(self):

        for n, (workers, pics) in {8: (3, 2), 6: (2, 1), 4: (1, 1), 2: (1, 1)}.items():
            with self.subTest(tokens=n):
                b = self.traced('t%d' % n, tokens=n, shader_workers=workers, picture_threads=pics)
                p = cli(b, ALL)
                out, err = finish(p, b)
                self.assertEqual(p.returncode, 0, out + err)
                self.assertEqual(sorted(l.split('\t')[1] for l in out.splitlines() if l.startswith('done\t')), sorted(ALL))
                run = spans(b)
                self.assertEqual({(s, t) for s, t, _, _ in run if s in ('shaders', 'pictures')}, {('shaders', workers), ('pictures', pics)})
                for _, _, at, _ in run:
                    self.assertLessEqual(sum(min(cost(s, t), n) for s, t, a, z in run if a <= at < z), n, 'busy threads at %.3f' % at)
                start = {s: min(a for x, _, a, _ in run if x == s) for s in ALL}
                end = {s: max(z for x, _, _, z in run if x == s) for s in ALL}
                if n == 8:
                    self.assertGreaterEqual(start['rewrite'], min(end[s] for s in ('shaders', 'pictures', 'art', 'app')))
                if n == 2:
                    self.assertTrue(all(start[s] >= end['shaders'] for s in ALL if s != 'shaders'))
                self.assertIn('admitted shaders with %d of %d tokens' % (min(workers + 1, n), n), err)

    def test_serial_runs_one_step_at_a_time_heaviest_first(self):
        b = self.traced('s', tokens=8, shader_workers=4, picture_threads=7, mode='serial')
        p = cli(b, ALL)
        out, err = finish(p, b)
        self.assertEqual(p.returncode, 0, out + err)
        run = sorted(spans(b), key=lambda r: r[2])
        order = []
        for s, _, _, _ in run:
            if not order or order[-1] != s:
                order.append(s)
        self.assertEqual(order, ['shaders', 'pictures', 'rewrite', 'art', 'overlay', 'app'])
        for (_, _, _, z), (_, _, a, _) in zip(run, run[1:]):
            self.assertLessEqual(z, a + 1e-3, 'spans overlap')
        self.assertEqual({(s, t) for s, t, _, _ in run if s in ('shaders', 'pictures')}, {('shaders', 4), ('pictures', 7)})

    def test_failing_pictures_name_their_step_and_a_retry_finishes(self):
        b = with_shaders(build(self.root / 'f', 'seed'))
        (Path(b.romfs) / 'data' / 'FAILPIC').write_text('')
        with self.assertRaises(pack_steps.StepError) as e:
            pack_steps.run(b, ['pictures'])
        self.assertEqual((e.exception.step, e.exception.ident), ('pictures', 'PB-STEP-pictures'))
        self.assertIn('act bosses', e.exception.detail)
        self.assertFalse((Path(b.stage) / '.units' / 'pictures.tsv').exists())
        (Path(b.romfs) / 'data' / 'FAILPIC').unlink()
        pack_steps.run(b, ['pictures'])
        m = (Path(b.stage) / '.units' / 'pictures.tsv').read_text()
        self.assertEqual([l.split('\t')[0] for l in m.splitlines()], ['monpics/mon.json', 'monpics/mon/a.webp', 'monpics/mon/s/a.webp'])
        self.assertIn(b.monpic, (Path(b.stage) / 'monpics' / 'mon' / 'a.webp').read_text(), 'the step loads the library the build names')

    def test_runner_killed_alone_leaves_no_step(self):
        b = self.traced('k', tokens=8, shader_workers=2, picture_threads=2)
        (Path(b.romfs) / 'data' / 'TRACE').unlink()
        (Path(b.romfs) / 'data' / 'BUSY').write_text('')
        p = cli(b, ['pictures', 'shaders'])
        marks = [Path(b.work) / 'busy-pictures', Path(b.work) / 'busy-shaders']
        deadline = time.time() + 60
        while not all(m.exists() and m.read_text() for m in marks) and time.time() < deadline:
            time.sleep(0.05)
        pids = [int(m.read_text()) for m in marks]
        self.assertFalse(any(gone(x) for x in pids))
        p.kill()
        p.wait(timeout=20)
        p.stdin.close()
        p.stdout.close()
        deadline = time.time() + 10
        while not all(gone(x) for x in pids) and time.time() < deadline:
            time.sleep(0.05)
        self.assertTrue(all(gone(x) for x in pids), 'a step outlived its runner')
        before = snapshot(b.stage)
        time.sleep(0.5)
        self.assertEqual(snapshot(b.stage), before, 'nothing writes after the runner died')


STUB_LIB = '''#include <stddef.h>
__attribute__((visibility("default"))) int monpic_bitknit_decode(const unsigned char *s, size_t sl, unsigned char *d, size_t dl) { return 1; }
__attribute__((visibility("default"))) int monpic_render(const char *r, const char *j, const char *o, int t, const char *res, char *e, size_t el) { return 1; }
'''
SMOKE = '''import os, sys
dm, empty = sys.argv[1:3]
sys.path.insert(0, os.path.join(dm, 'tools'))
import pack_steps
pack_steps._tool_paths(pack_steps.Build(dm=dm, exefs=empty, romfs=empty, stage=empty, work=empty, python=sys.executable))
sys.path.insert(0, os.path.join(dm, 'gfx', 'tools'))
import d2rm_pack, shell_stubs, stage_touch_assets, gen_app_art, gen_style, gen_recipe_data, gen_effect_data, gen_build_names, gen_glossary_data
import gen_book_data
import dmsh_native, gen_mon_pics, touch_layout, gen_sim_data, nsoimg
assert gen_mon_pics.lib().monpic_render, 'no render entry'
print('imported', os.path.realpath(gen_mon_pics.__file__).startswith(os.path.realpath(dm)))
'''


class PackagedToolsTest(unittest.TestCase):


    def test_packaged_tree(self):
        import pack_assets
        import pack_zip
        import zipfile
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            pack_zip.write(pack_assets.tool_files(str(DM.parent)), str(t / 'tools.zip'))
            with zipfile.ZipFile(t / 'tools.zip') as z:
                z.extractall(t / 'tree')
            (t / 'stub.c').write_text(STUB_LIB)
            lib = t / 'libmonpic.so'
            subprocess.run([os.environ.get('CC', 'cc'), '-shared', '-fPIC', '-o', str(lib), str(t / 'stub.c')], check=True)
            empty = t / 'empty'
            empty.mkdir()
            env = {'PATH': os.environ.get('PATH', ''), 'DM_MONPIC_LIB': str(lib), 'DM_ROOT': str(t / 'work')}
            r = subprocess.run([sys.executable, '-I', '-c', SMOKE, str(t / 'tree' / 'dm'), str(empty)], cwd=empty, env=env, capture_output=True, text=True)
            self.assertEqual((r.returncode, r.stdout.strip()), (0, 'imported True'), r.stderr[-3000:])
            g = game(t / 'game', 'seed')
            (g / 'RomFS' / 'data' / 'build_manifest.psv').write_text('')
            b = pack_steps.Build(dm=str(t / 'tree' / 'dm'), exefs=str(g / 'ExeFS'), romfs=str(g / 'RomFS'), stage=str(t / 'stage'), work=str(t / 'work'),
                                 python=sys.executable, monpic=str(lib))
            r = subprocess.run([sys.executable, '-I', str(t / 'tree' / 'dm' / 'tools' / 'pack_steps.py'), 'step', 'pictures', b.to_json()], cwd=empty,
                               env=b.env(env), capture_output=True, text=True)
            self.assertNotEqual(r.returncode, 0)
            self.assertIn('not the verified 3.4.95035 build', r.stderr, r.stderr[-3000:])


class ToolListTest(unittest.TestCase):


    def test_variants(self):
        import pack_assets
        repo = str(DM.parent)
        qa, player = pack_assets.tool_files(repo, variant='qa'), pack_assets.tool_files(repo, variant='player')
        self.assertIn('dm/tools/patch_nso_qa.py', qa)
        self.assertNotIn('dm/tools/patch_nso_qa.py', player)
        self.assertIn('dm/ios/tools/uireg/uireg.py', qa)
        gone = sorted(set(qa) - set(player))
        self.assertTrue(all(p == 'dm/tools/patch_nso_qa.py' or p.startswith(('dm/ios/tools/uireg/', 'dm/ios/tools/e2e/')) or p in pack_assets.test_only(repo)
                            for p in gone), gone)
        self.assertTrue(set(player) <= set(qa))
        self.assertTrue(all(not p.startswith('dm/ios/tools/uireg/') for p in player))
        with self.assertRaises(SystemExit):
            pack_assets.tool_files(repo, variant='other')
        sys.path.insert(0, str(DM / 'ios' / 'tools'))
        import overlay_contract
        self.assertFalse(set(gone) & set(overlay_contract.SOURCE_INPUTS), 'a left-out file is an overlay input')


@unittest.skipUnless(os.environ.get('DM_PACK_GAME_DIR'), 'DM_PACK_GAME_DIR (ExeFS/ and RomFS/ of the game) not set')
class ChainTest(unittest.TestCase):

    def test_chain(self):
        import pack_assets
        sys.path.insert(0, str(DM / 'android' / 'tools'))
        import pic_tolerance
        game_dir = Path(os.environ['DM_PACK_GAME_DIR'])
        ref = os.environ.get('DM_PACK_REFERENCE')
        self.assertTrue(ref and os.path.isdir(ref), 'DM_PACK_REFERENCE (the android/ folder of a cold PC pack from the same source) is required')
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            for rel, src in pack_assets.tool_files(str(DM.parent)).items():
                (t / 'tree' / rel).parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(src, t / 'tree' / rel)
            (t / 'work').mkdir()
            lib = os.environ.get('DM_DESWIZZLE_LIB') or str(t / 'libdeswizzle.so')
            if not os.environ.get('DM_DESWIZZLE_LIB'):
                subprocess.run([os.environ.get('CC', 'cc'), '-O2', '-shared', '-fPIC', '-I', str(DM / 'gfx' / 'nvn'), '-I', str(DM / 'gfx' / 'tex'), '-o', lib,
                                str(DM / 'gfx' / 'tex' / 'nvn_deswizzle.c'), str(DM / 'gfx' / 'nvn' / 'nvn_tex.c')], check=True)
            b = pack_steps.Build(dm=str(t / 'tree' / 'dm'), exefs=str(game_dir / 'ExeFS'), romfs=str(game_dir / 'RomFS'), stage=str(t / 'stage'),
                                 work=str(t / 'work'), python=sys.executable, deswizzle=lib, monpic=os.environ.get('DM_MONPIC_LIB') or None)
            env = {k: v for k, v in os.environ.items() if k not in ('DM_PC_DATA', 'DM_REQUIRE_PC_ART', 'DM_GEN_ASSETS')}
            p = cli(b, pack_steps.steps_for('android'), env=env)
            out, err = finish(p, b)
            self.assertEqual(p.returncode, 0, out[-2000:] + err[-2000:])
            self.assertEqual(sorted(l.split('\t')[1] for l in out.splitlines() if l.startswith('done\t')), sorted(pack_steps.steps_for('android')))
            for item in ('out', 'derived.tsv', 'overlay', 'overlay_variants', 'appart'):
                got = snapshot(t / 'stage' / item) if (t / 'stage' / item).is_dir() else {item: hashlib.sha256((t / 'stage' / item).read_bytes()).hexdigest()}
                want = snapshot(Path(ref) / item) if (Path(ref) / item).is_dir() else {item: hashlib.sha256((Path(ref) / item).read_bytes()).hexdigest()}
                if item == 'appart':
                    want = {k: v for k, v in want.items() if not (k.startswith('mon/') or k == 'mon.json')}
                self.assertEqual(got, want, item)
            self.assertEqual(pictures_differ(pic_tolerance, Path(ref) / 'appart', t / 'stage' / 'monpics'), [], 'enemy pictures')
            phone = os.environ.get('DM_PACK_PHONE_STAGE')
            if phone:
                self.assertEqual((Path(phone) / 'appart' / 'book.json').read_bytes(), (t / 'stage' / 'appart' / 'book.json').read_bytes(),
                                 'the phone\'s book.json')
                self.assertEqual(pictures_differ(pic_tolerance, t / 'stage' / 'monpics', Path(phone) / 'monpics'), [], 'the phone\'s enemy pictures')


def pictures_differ(pic_tolerance, reference, candidate):

    return pic_tolerance.compare(str(reference), str(candidate))[0]


if __name__ == '__main__':
    unittest.main()
