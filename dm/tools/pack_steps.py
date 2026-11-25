#!/usr/bin/env python3


































import ctypes
import dataclasses
import hashlib
import json
import os
import shutil
import signal
import subprocess
import sys
import threading
import time

POLL_S = 0.25
WINDOWS = os.name == 'nt'
SHADER_WORKERS = 4
MODES = ('lanes', 'serial')


class StepError(Exception):

    def __init__(self, step, detail):
        super().__init__('%s: %s' % (step, detail))
        self.step, self.detail, self.ident = step, detail, 'PB-STEP-' + step


@dataclasses.dataclass(frozen=True)
class Build:
    dm: str
    exefs: str
    romfs: str
    stage: str
    work: str
    python: str
    target: str = 'android'
    deswizzle: str | None = None
    xlate: str | None = None
    shader_lib: str | None = None
    queries: str | None = None
    qa_combat: bool = False
    monpic: str | None = None
    tokens: int | None = None
    shader_workers: int = SHADER_WORKERS
    picture_threads: int | None = None
    mode: str = 'lanes'

    def __post_init__(self):
        if self.target not in ('android', 'ohos'):
            raise ValueError('unknown target %r' % self.target)
        if self.mode not in MODES:
            raise ValueError('unknown mode %r' % self.mode)
        for f in ('tokens', 'shader_workers', 'picture_threads'):
            v = getattr(self, f)
            if v is not None and (not isinstance(v, int) or isinstance(v, bool) or v < 1):
                raise ValueError('%s must be a whole number of at least 1: %r' % (f, v))
        for f in ('dm', 'exefs', 'romfs', 'stage', 'work', 'python'):
            if not os.path.isabs(getattr(self, f)):
                raise ValueError('%s must be an absolute path: %r' % (f, getattr(self, f)))

    def to_json(self):
        return json.dumps(dataclasses.asdict(self), sort_keys=True)

    @staticmethod
    def from_json(text):
        d = json.loads(text)
        names = {f.name for f in dataclasses.fields(Build)}
        if not isinstance(d, dict) or set(d) - names:
            raise ValueError('build description has unknown fields: %s' % sorted(set(d) - names if isinstance(d, dict) else []))
        return Build(**d)

    def env(self, base):

        e = dict(base, DM_ROOT=self.work, DM_ROMFS_SRC=self.romfs, PYTHONUTF8='1',
                 PYTHONPATH=os.pathsep.join(filter(None, [os.path.join(self.dm, 'tools', 'winlf'), base.get('PYTHONPATH')])))
        for k, v in (('DM_DESWIZZLE_LIB', self.deswizzle), ('DM_MONPIC_LIB', self.monpic)):
            e.pop(k, None)
            if v:
                e[k] = v
        return e

    def gen_dir(self, env):

        return os.path.normpath(env.get('DM_GEN_ASSETS') or os.path.join(self.dm, 'ios', 'Gen', 'assets'))


def product(b):
    with open(os.path.join(b.dm, 'PRODUCT'), encoding='utf-8') as f:
        return f.readline().strip()


def _tool_paths(b):
    for p in (os.path.join(b.dm, 'tools'), os.path.join(b.dm, 'ios', 'tools'), os.path.join(b.dm, 'android', 'tools')):
        if p not in sys.path:
            sys.path.insert(0, p)


REWRITE_DONE = '.complete'


def rewrite(b):
    _tool_paths(b)
    from d2rm_pack import MODS, run, step
    out = os.path.join(b.work, 'build', 'cache', 'dm', 'out', product(b) + '-' + b.target + ('-qa-combat' if b.qa_combat else ''))
    tool = os.path.join(b.dm, 'tools', 'patch_nso.py')
    src = hashlib.sha256(open(tool, 'rb').read()).hexdigest()
    if b.qa_combat:
        src += ' ' + hashlib.sha256(open(os.path.join(b.dm, 'tools', 'patch_nso_qa.py'), 'rb').read()).hexdigest()
    marker = os.path.join(out, REWRITE_DONE)

    def patch():

        if os.path.isdir(out):
            shutil.rmtree(out)
        os.makedirs(out)
        for m in MODS:
            run([b.python, tool, os.path.join(b.exefs, m), out, m] + (['--qa-combat'] if b.qa_combat and m == 'main' else []),
                env={'DM_TARGET': b.target, 'DM_RULESET': 'touch-run-v4'}, log=os.path.join(out, m + '.patch.log'), what='rewrite ' + m)
        flush([os.path.join(out, f) for f in sorted(os.listdir(out))])
        with open(marker + '.part', 'w', encoding='utf-8', newline='\n') as f:
            f.write(src + '\n')
            f.flush()
            os.fsync(f.fileno())
        os.replace(marker + '.part', marker)
        _sync_dir(out)


    def patched():
        try:
            with open(marker, encoding='utf-8') as f:
                return f.read().strip() == src
        except OSError:
            return False
    step('rewrite the game program', patched, patch)
    os.makedirs(os.path.join(b.stage, 'out'))
    for f in sorted(os.listdir(out)):
        if not f.endswith('.log') and not f.startswith('.'):
            shutil.copyfile(os.path.join(out, f), os.path.join(b.stage, 'out', f))


def art(b):
    _tool_paths(b)
    from d2rm_pack import run, say
    ios = os.path.join(b.dm, 'ios')
    run([b.python, os.path.join(ios, 'tools', 'gen_assets.py'), b.romfs], cwd=ios, log=os.path.join(b.work, 'gen_assets.log'), what='UI art')
    gen = os.path.join(b.stage, 'gen')
    os.makedirs(gen)
    run([b.python, os.path.join(ios, 'tools', 'native_art', 'generate.py'), b.romfs, '--output-dir', gen], what='skill icons')
    for name, gen_py in (('dm_vault_tab.h', 'gen_vault_table.py'), ('dm_hud_exp.h', 'gen_hud_exp.py')):
        text = run([b.python, os.path.join(ios, 'tools', gen_py), b.romfs], what=name)
        with open(os.path.join(gen, name), 'w', encoding='utf-8') as f:
            f.write(text)
    import shell_stubs
    say('tables: experience %d, slots %d, vault entries %d, sets %d, skill index %d' % shell_stubs.data(None, os.path.join(b.stage, 'derived.tsv'), gen))


def overlay(b):
    _tool_paths(b)
    from d2rm_pack import run
    ios = os.path.join(b.dm, 'ios')

    run([b.python, os.path.join(ios, 'tools', 'gen_romfs_overlay.py'), b.romfs, os.path.join(b.stage, 'overlay')], cwd=ios,
        env=dict({'DM_OVERLAY_NO_SHARE': '1'}, **({'DM_OVERLAY_NO_ICLOUD': '1'} if b.target == 'android' else {})),
        log=os.path.join(b.work, 'gen_overlay.log'), what='overlay')
    import stage_touch_assets
    stage_touch_assets.stage(os.path.join(b.stage, 'gen', 'TouchArt'), os.path.join(ios, 'Gen', 'assets', 'NativeHUD', 'v4', 'hud_v4_ui'),
                             os.path.join(b.stage, 'overlay'))


def app(b):
    if b.target != 'android':
        raise StepError('app', 'the app art is Android only')
    _tool_paths(b)
    appart = os.path.join(b.stage, 'appart')
    import gen_app_art
    gen_app_art.stage(b.romfs, appart)
    try:
        import gen_style
        gen_style.stage(b.romfs, appart)
    except Exception as e:
        print('appart: game style not staged (%r); the app uses its built-in style' % (e,))

    lack = gen_app_art.missing(appart)
    if lack:
        raise SystemExit('appart: missing %s (the Quick Menu needs them; check the game files)' % ', '.join(lack))
    import gen_recipe_data
    import gen_effect_data
    import gen_build_names
    import gen_glossary_data
    import gen_book_data
    for mod in (gen_recipe_data, gen_effect_data, gen_build_names, gen_glossary_data, gen_book_data):
        mod.stage(b.romfs, appart)


def pictures(b):
    if b.target != 'android':
        raise StepError('pictures', 'the enemy pictures are Android only')
    _tool_paths(b)
    import gen_mon_pics

    gen_mon_pics.stage(b.romfs, os.path.join(b.stage, 'monpics'), os.path.join(b.exefs, 'main'), workers=b.picture_threads)


def shaders(b):
    if not (b.xlate and b.shader_lib and b.queries):
        raise SystemExit('shaders: this build names no translator, worker library or query kernels')
    sys.path.insert(0, os.path.join(b.dm, 'gfx', 'tools'))
    import dmsh_native
    dmsh_native.build(os.path.join(b.dm, 'gfx', 'tools'), b.exefs, os.path.join(b.romfs, 'data'), b.stage, b.queries, b.shader_lib, b.xlate,
                      flush, _sync_dir, b.shader_workers)


@dataclasses.dataclass(frozen=True)
class Step:
    run: object
    outputs: tuple
    watch_gen: bool
    resumable: bool = False


STEPS = {
    'rewrite': Step(rewrite, ('out',), False),
    'art': Step(art, ('gen', 'derived.tsv'), True),
    'overlay': Step(overlay, ('overlay', 'overlay_variants'), False),
    'app': Step(app, ('appart',), False),
    'pictures': Step(pictures, ('monpics',), False),
    'shaders': Step(shaders, ('shaders',), False, True),
}
LANES = (('rewrite',), ('art', 'overlay'), ('app',), ('pictures',), ('shaders',))
PRIORITY = ('shaders', 'pictures', 'art', 'overlay', 'app', 'rewrite')
SERIAL = ('shaders', 'pictures', 'rewrite', 'art', 'overlay', 'app')
UNITS = '.units'

INSTALLED = (('monpics/', 'appart/'), ('gen/', None))


def installed(rel):

    for prefix, to in INSTALLED:
        if rel.startswith(prefix):
            return None if to is None else to + rel[len(prefix):]
    return rel


def steps_for(target):
    return ['rewrite', 'art', 'overlay'] + (['app', 'pictures'] if target == 'android' else [])


def lanes(names, serial=False):

    unknown = set(names) - set(STEPS)
    if unknown:
        raise ValueError('unknown steps %s' % sorted(unknown))
    if len(set(names)) != len(names):
        raise ValueError('a step is named twice')
    if serial:
        return [[s for s in SERIAL if s in names]]
    out = [[s for s in lane if s in names] for lane in LANES]
    return [lane for lane in out if lane]


def tokens(b):
    return b.tokens or os.cpu_count() or 1


def cost(b, name):

    if name == 'shaders':
        c = b.shader_workers + 1
    elif name == 'pictures':
        c = b.picture_threads or min(os.cpu_count() or 1, 8)
    else:
        c = 1
    return min(c, tokens(b))


def _files(root):

    out = {}
    if os.path.islink(root):
        raise ValueError('link in the outputs: %s' % os.path.basename(root))
    if os.path.isfile(root):
        return {os.path.basename(root): root}
    for top, dirs, files in os.walk(root):
        dirs.sort()
        for f in files:
            p = os.path.join(top, f)
            if os.path.islink(p):
                raise ValueError('link in the outputs: %s' % f)
            out[os.path.relpath(p, root).replace(os.sep, '/')] = p
    return out


def outputs(b, name):

    out = {}
    for o in STEPS[name].outputs:
        p = os.path.join(b.stage, o)
        if not os.path.lexists(p):
            continue
        for rel, full in _files(p).items():
            out[o if os.path.isfile(p) else o + '/' + rel] = full
    return out


def watched(b, name, env):

    paths = list(outputs(b, name).values())
    if STEPS[name].watch_gen:
        g = b.gen_dir(env)
        if os.path.isdir(g):
            paths += list(_files(g).values())
    return paths


def manifest(b, name):

    lines = []
    for rel, full in sorted(outputs(b, name).items()):
        h = hashlib.sha256()
        with open(full, 'rb') as f:
            for chunk in iter(lambda: f.read(1 << 20), b''):
                h.update(chunk)
        lines.append('%s\t%d\t%s\n' % (rel, os.path.getsize(full), h.hexdigest()))
    return ''.join(lines)


def path_digest(paths):
    return hashlib.sha256(''.join(p + '\n' for p in sorted(paths)).encode('utf-8')).hexdigest()


def load_expect(path):

    out = {}
    with open(path, encoding='utf-8') as f:
        for line in f:
            if not line.strip() or line.startswith('#'):
                continue
            p = line.rstrip('\n').split('\t')
            if len(p) != 4 or p[0] not in STEPS or p[0] in out:
                raise ValueError('bad line in %s: %r' % (os.path.basename(path), line))
            out[p[0]] = (int(p[1]), int(p[2]), p[3])
    return out


def _sync_dir(d, windows=WINDOWS):

    if windows:
        return
    fd = os.open(d, os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def _fsync_file(path):
    fd = os.open(path, os.O_RDWR | getattr(os, 'O_BINARY', 0))
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def flush(paths, sync=getattr(os, 'sync', None), fsync=_fsync_file):

    if sync is not None:
        sync()
        return
    for p in paths:
        fsync(p)


def spawn_options(windows=WINDOWS):

    if windows:
        return {'creationflags': getattr(subprocess, 'CREATE_NEW_PROCESS_GROUP', 0x200)}
    return {'start_new_session': True}


def kill_tree(pid, windows=WINDOWS, run=subprocess.run):

    if windows:
        run(['taskkill', '/T', '/F', '/PID', str(pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return
    try:
        os.killpg(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def finish(b, name, expect=None):


    text = manifest(b, name)
    paths = [line.split('\t', 1)[0] for line in text.splitlines()]
    if expect is not None:
        if name not in expect:
            raise StepError(name, 'no expected file set')
        _, count, digest = expect[name]
        if len(paths) != count or path_digest(paths) != digest:
            raise StepError(name, 'wrote %d files, expected %d (or other names)' % (len(paths), count))
    flush(list(outputs(b, name).values()))
    units = os.path.join(b.stage, UNITS)
    os.makedirs(units, exist_ok=True)
    target = os.path.join(units, name + '.tsv')
    with open(target + '.part', 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)
        f.flush()
        os.fsync(f.fileno())
    os.replace(target + '.part', target)
    _sync_dir(units)
    return hashlib.sha256(text.encode('utf-8')).hexdigest()


def clear(b, name):


    for o in () if STEPS[name].resumable else STEPS[name].outputs:
        p = os.path.join(b.stage, o)
        if os.path.isdir(p) and not os.path.islink(p):
            shutil.rmtree(p)
        elif os.path.lexists(p):
            os.remove(p)
    m = os.path.join(b.stage, UNITS, name + '.tsv')
    for p in (m, m + '.part'):
        if os.path.lexists(p):
            os.remove(p)


class _Tokens:



    def __init__(self, total, log):
        self.total = self.free = total
        self.log = log
        self.cond = threading.Condition()
        self.waiting, self.admitted = {}, {}
        self.opened = False

    def ask(self, name, cost):
        with self.cond:
            if name not in self.admitted:
                self.waiting[name] = cost
            self._admit()

    def open(self):
        with self.cond:
            self.opened = True
            self._admit()

    def take(self, name, cost, stopped):

        with self.cond:
            if name not in self.admitted and name not in self.waiting:
                self.waiting[name] = cost
                self._admit()
            while name not in self.admitted:
                if stopped():
                    self.waiting.pop(name, None)
                    return False
                self.cond.wait(POLL_S)
            return True

    def give(self, name, then=None, then_cost=0):

        with self.cond:
            self.free += self.admitted.pop(name, 0)
            if then is not None:
                self.waiting[then] = then_cost
            self._admit()

    def _admit(self):
        if not self.opened:
            return
        for name in sorted(self.waiting, key=PRIORITY.index):
            c = self.waiting[name]
            if c <= self.free:
                self.free -= c
                self.admitted[name] = c
                del self.waiting[name]
                self.log('admitted %s with %d of %d tokens (%d left)' % (name, c, self.total, self.free))
        self.cond.notify_all()


class _Lanes:



    def __init__(self, b, names, report, expect, serial, base_env, log=None):
        self.b, self.report, self.expect = b, report, expect
        self.plan = lanes(names, serial or b.mode == 'serial')
        self.env = dict(b.env(base_env), DM_PACK_RUNNER=str(os.getpid()))
        self.lock = threading.Lock()
        self.children = {}
        self.failure = None
        self.stopped = False
        self.tokens = _Tokens(tokens(b), log or (lambda line: None))
        self.logs = os.path.join(b.work, 'build', 'pack_steps')
        os.makedirs(self.logs, exist_ok=True)
        os.makedirs(b.stage, exist_ok=True)

    def stop(self):
        with self.lock:
            self.stopped = True
            kids = list(self.children.values())
        for p in kids:
            kill_tree(p.pid)

    def _halted(self):
        with self.lock:
            return self.stopped or self.failure is not None

    def run(self):
        for lane in self.plan:
            self.tokens.ask(lane[0], cost(self.b, lane[0]))
        self.tokens.open()
        threads = [threading.Thread(target=self._lane, args=(lane,), daemon=True) for lane in self.plan]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        if self.failure:
            raise self.failure
        if self.stopped:
            raise StepError('stopped', 'the run was stopped')

    def _lane(self, lane):
        for i, name in enumerate(lane):
            if self._halted() or not self.tokens.take(name, cost(self.b, name), self._halted):
                self.tokens.give(name)
                return
            nxt = lane[i + 1] if i + 1 < len(lane) else None
            try:
                self._step(name)
            except StepError as e:
                self.tokens.give(name)
                self._fail(e)
                return
            except Exception as e:
                self.tokens.give(name)
                self._fail(StepError(name, '%s: %s' % (type(e).__name__, e)))
                return
            self.tokens.give(name, nxt, cost(self.b, nxt) if nxt else 0)

    def _fail(self, e):
        with self.lock:
            first = self.failure is None and not self.stopped
            if first:
                self.failure = e
        if first:
            self.report('error', e.step, e.ident, e.detail)
            self.stop()

    def _step(self, name):
        clear(self.b, name)

        parts = _parts(self.b, name) if STEPS[name].resumable else None
        log = os.path.join(self.logs, name + '.log')
        total = self.expect[name][0] if self.expect else None
        with open(log, 'wb') as out:
            p = subprocess.Popen([self.b.python, '-X', 'utf8', os.path.join(self.b.dm, 'tools', 'pack_steps.py'), 'step', name, self.b.to_json()],
                                 env=self.env, cwd=self.b.work, stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT,
                                 **spawn_options())
            with self.lock:
                self.children[name] = p
                if self.stopped:
                    kill_tree(p.pid)
            self.report('start', name, str(p.pid))
            seen, count = set(), -1
            while True:
                done = p.poll() is not None
                if parts is not None:
                    now_parts = _parts(self.b, name)
                    for part, ident in sorted(now_parts.items()):
                        if parts.get(part) != ident:
                            digest = _part_digest(self.b, name, part)
                            if digest:
                                self.report('part', name, part, digest)
                    parts = now_parts
                now = set(watched(self.b, name, self.env))
                new = now - seen
                seen = now
                n = len(now) if total is None else min(len(now), total)
                shown = [x for x in new if not os.path.basename(x).startswith('.') and x.rsplit(os.sep, 2)[-2:-1] != ['.gen_lock']]
                if n != count and shown:
                    count = n
                    self.report('file', name, str(n), os.path.basename(max(shown, key=_mtime)))
                if done:
                    break
                time.sleep(POLL_S)
            with self.lock:
                del self.children[name]
        if p.returncode:
            with self.lock:
                if self.stopped:
                    raise StepError(name, 'stopped')
            raise StepError(name, 'exit %d: %s' % (p.returncode, _last_line(log)))
        self.report('done', name, finish(self.b, name, self.expect))


def _parts(b, name):

    out = {}
    d = os.path.join(b.stage, UNITS, name)
    try:
        names = os.listdir(d)
    except FileNotFoundError:
        return out
    for f in names:
        if f.endswith('.tsv'):
            try:
                st = os.stat(os.path.join(d, f))
            except FileNotFoundError:
                continue
            out[f[:-4]] = (st.st_ino, st.st_mtime_ns)
    return out


def _part_digest(b, name, part):
    try:
        with open(os.path.join(b.stage, UNITS, name, part + '.tsv'), 'rb') as f:
            return hashlib.sha256(f.read()).hexdigest()
    except FileNotFoundError:
        return None


def _mtime(p):
    try:
        return os.stat(p).st_mtime_ns, p
    except FileNotFoundError:
        return 0, p


def _last_line(log):
    with open(log, 'rb') as f:
        lines = [x.strip() for x in f.read()[-4000:].decode('utf-8', 'replace').splitlines() if x.strip()]
    return (lines[-1] if lines else 'no output')[:300].replace('\t', ' ')


def run(b, names, report=None, expect=None, serial=False, base_env=None):


    lanes_ = _Lanes(b, names, report or (lambda *a: None), expect, serial, dict(os.environ) if base_env is None else base_env)
    try:
        lanes_.run()
    finally:
        lanes_.stop()


def _cli_run(argv):
    b = Build.from_json(argv[0])
    out_lock = threading.Lock()

    def report(*fields):
        with out_lock:
            sys.stdout.write('\t'.join(f.replace('\t', ' ').replace('\n', ' ') for f in fields) + '\n')
            sys.stdout.flush()
    if not WINDOWS:
        try:
            os.setsid()
        except OSError:
            pass
    runner = _Lanes(b, argv[1:], report, load_expect(os.path.join(b.dm, 'tools', 'pack_steps.tsv')), False, dict(os.environ),
                    log=lambda line: print('runner: ' + line, file=sys.stderr, flush=True))

    def watch_stdin():
        while os.read(0, 1 << 12):
            pass
        runner.stop()
    threading.Thread(target=watch_stdin, daemon=True).start()
    try:
        runner.run()
    except StepError:
        return 1
    finally:
        runner.stop()
    return 0


def _cli_expect(argv):
    b = Build.from_json(argv[0])
    env = b.env(dict(os.environ))
    print('# step\tprogress files\toutput files\toutput path digest')
    for name in STEPS:
        if not os.path.isfile(os.path.join(b.stage, UNITS, name + '.tsv')):
            continue
        paths = list(outputs(b, name))
        print('%s\t%d\t%d\t%s' % (name, len(watched(b, name, env)), len(paths), path_digest(paths)))
    return 0


def _end_with_runner(environ=os.environ):


    runner = environ.get('DM_PACK_RUNNER')
    if not runner or WINDOWS:
        return
    try:
        prctl = ctypes.CDLL(None, use_errno=True).prctl
    except (OSError, AttributeError):
        return
    PR_SET_PDEATHSIG = 1
    prctl(PR_SET_PDEATHSIG, signal.SIGKILL, 0, 0, 0)
    if os.getppid() != int(runner):
        os._exit(1)


def main(argv):
    if len(argv) >= 2 and argv[0] == 'run':
        return _cli_run(argv[1:])
    if len(argv) == 3 and argv[0] == 'step':
        _end_with_runner()
        if argv[1] not in STEPS:
            sys.exit('unknown step %s' % argv[1])
        STEPS[argv[1]].run(Build.from_json(argv[2]))
        return 0
    if len(argv) == 2 and argv[0] == 'expect':
        return _cli_expect(argv[1:])
    sys.exit(__doc__)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
