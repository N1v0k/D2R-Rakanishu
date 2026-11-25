#!/usr/bin/env python3











import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
import unittest

TOOLS = Path(__file__).resolve().parents[1]
DM = TOOLS.parent
GFX = DM / 'gfx' / 'tools'
SHADERS = DM / 'android' / 'shaders'
for p in (TOOLS, GFX):
    sys.path.insert(0, str(p))
import pack_steps
import dmsh_native
import dmsh_vk
import pack_builtin_queries


SPV = bytes.fromhex(
    '03022307000001000b000d001c0000000000000011000200010000000b00060001000000474c534c2e7374642e343530000000000e00030000000000'
    '010000000f00060005000000040000006d61696e000000000f0000001000060004000000110000000800000001000000010000000300030002000000'
    'c201000004000a00474c5f474f4f474c455f6370705f7374796c655f6c696e655f646972656374697665000004000800474c5f474f4f474c455f696e'
    '636c7564655f6469726563746976650005000400040000006d61696e0000000005000300080000004200000006000400080000000000000076000000'
    '050003000a00000062000000050008000f000000676c5f476c6f62616c496e766f636174696f6e494400000047000400070000000600000004000000'
    '4700030008000000030000004800050008000000000000002300000000000000470004000a0000002100000000000000470004000a00000022000000'
    '00000000470004000f0000000b0000001c000000470004001b0000000b00000019000000130002000200000021000300030000000200000015000400'
    '0600000020000000000000001d00030007000000060000001e0003000800000007000000200004000900000002000000080000003b00040009000000'
    '0a00000002000000150004000b00000020000000010000002b0004000b0000000c00000000000000170004000d000000060000000300000020000400'
    '0e000000010000000d0000003b0004000e0000000f000000010000002b00040006000000100000000000000020000400110000000100000006000000'
    '2b000400060000001400000001000000200004001500000002000000060000002b000400060000001a000000080000002c0006000d0000001b000000'
    '1a00000014000000140000003600050002000000040000000000000003000000f8000200050000004100050011000000120000000f00000010000000'
    '3d0004000600000013000000120000004100060015000000160000000a0000000c000000130000003d00040006000000170000001600000080000500'
    '060000001800000017000000140000004100060015000000190000000a0000000c000000130000003e0003001900000018000000fd00010038000100')

STUB = r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Stand-in translator: the machine code is the SPIR-V; code starting with FAIL fails; STUB_SLOW_MS slows every call */
int dmsh_xlate_one(const char *job, size_t job_len, const uint8_t *code, size_t code_len, uint8_t **spv, size_t *spv_len,
                   char **res, size_t *res_len, char **err) {
  const char *slow = getenv("STUB_SLOW_MS");
  (void)job; (void)job_len;
  if (slow) usleep(atoi(slow) * 1000);
  if (code_len >= 4 && !memcmp(code, "FAIL", 4)) { *err = strdup("InvalidOperationException: made to fail"); return 1; }
  *spv = malloc(code_len); memcpy(*spv, code, code_len); *spv_len = code_len;
  *res = strdup("{\"cbufs\": [], \"sbufs\": [], \"textures\": [], \"images\": []}"); *res_len = strlen(*res);
  return 0;
}
void dmsh_xlate_free(void *p) { free(p); }
'''

CACHE = Path(os.environ.get('DM_ROOT') or Path.home() / 'dm-work') / 'build' / 'cache' / 'dmsh-shaders-host'


def worker_lib():

    if os.environ.get('DM_SHADER_LIB'):
        return os.environ['DM_SHADER_LIB']
    h = hashlib.sha256()
    for f in ('dmsh_shaders.cpp', 'dmsh_shaders.cmake', 'CMakeLists.txt'):
        h.update((SHADERS / f).read_bytes())
    h.update((DM / 'third_party' / 'spirv-tools' / 'CMakeLists.txt').read_bytes())
    out = CACHE / h.hexdigest()[:16]
    lib = out / 'libdmsh_shaders.so'
    if not lib.is_file():
        if not (shutil.which('cmake') and (shutil.which('c++') or shutil.which('clang++'))):
            return None
        b = out / 'build'
        subprocess.run(['cmake', '-S', str(SHADERS), '-B', str(b), '-DCMAKE_BUILD_TYPE=Release'], check=True, stdout=subprocess.DEVNULL)
        subprocess.run(['cmake', '--build', str(b), '--target', 'dmsh_shaders', '-j', str(os.cpu_count() or 4)], check=True, stdout=subprocess.DEVNULL)
        shutil.copyfile(b / 'libdmsh_shaders.so', lib)
        shutil.rmtree(b)
    return str(lib)


def record(code, c1):

    ctl = bytearray(0x880)
    ctl[0:4] = bytes.fromhex('34127698')
    c1o = len(code) + 0x880
    struct.pack_into('<2I', ctl, 0x700, c1o, c1o + len(c1))
    return b'BTCDBTCD' + bytes(0x30) + code + bytes(ctl) + c1


def archives(data, files):

    os.makedirs(data, exist_ok=True)
    blob, rows = b'', ['FileId|KeyLayout|Chunk|Archive|CKey|CSize|EKey|ESize|Encoding|Path|Flags']
    for i, (path, content) in enumerate(sorted(files.items())):
        off = len(blob)
        ekey = bytes(9) + bytes([0, 0]) + off.to_bytes(5, 'big')
        rows.append('%d|0|0|0|%s|%d|%s|%d|n|data:data/%s|0' % (i, hashlib.md5(content).hexdigest(), len(content), ekey.hex(), len(content), path))
        blob += content
    Path(data, '00-00000000.data').write_bytes(blob)
    Path(data, 'build_manifest.psv').write_text('\n'.join(rows) + '\n', encoding='utf-8')


class Game:

    def __init__(self, root, n_a=7, n_b=5, fail=None):
        self.root = Path(root)
        self.data = self.root / 'game' / 'RomFS' / 'data'
        self.exefs = self.root / 'game' / 'ExeFS'
        self.exefs.mkdir(parents=True)
        rows, files, self.keys = [], {}, []
        for name, n in (('cs_a.bin', n_a), ('cs_b.bin', n_b)):
            recs = []
            for i in range(n):
                key = hashlib.sha256(('%s%d' % (name, i)).encode()).hexdigest()[:16]
                self.keys.append(key)
                recs.append(record(b'FAIL' if key == fail else SPV, struct.pack('<I', i)))
                rows.append('%s\t%d\t计算\t%s\t-\t1\t-1\t-1\t-\t-\t-\t-\t15' % (name, i, key))
            files['hd/shaders/' + name] = b''.join(recs)
        archives(self.data, files)
        self.map = self.root / 'shader_map.tsv'
        self.map.write_text('# made-up\npackage\tindex\tstage\tdm_ctl_sha16\n' + '\n'.join(rows) + '\n', encoding='utf-8')
        self.queries = self.root / 'queries'
        self.queries.mkdir()
        keys = pack_builtin_queries.kernel_keys()
        self.qkeys = sorted(keys[m] for m in pack_builtin_queries.MODES)
        for k in self.qkeys:
            (self.queries / (k + '.xgs')).write_bytes(b'query ' + k.encode())
        self.stage = self.root / 'stage'


def build(g, lib, stub, workers=2):
    dmsh_native.build(str(GFX), str(g.exefs), str(g.data), str(g.stage), str(g.queries), lib, stub, pack_steps.flush, pack_steps._sync_dir,
                      workers=workers, say=lambda *_: None, shader_map=str(g.map))


def tree(stage):

    out = {}
    for d in ('shaders', '.units/shaders'):
        p = Path(stage, d)
        for f in sorted(p.iterdir()) if p.is_dir() else ():
            out[d + '/' + f.name] = (f.read_bytes(), f.stat().st_mtime_ns)
    return out


LIB = worker_lib()


@unittest.skipUnless(LIB and (shutil.which('cc') or shutil.which('clang')), 'needs cmake and a C/C++ compiler (or DM_SHADER_LIB)')
class StepTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp(prefix='packshaders')
        src = Path(cls.tmp, 'stub.c')
        src.write_text(STUB)
        cls.stub = str(Path(cls.tmp, 'libstub.so'))
        subprocess.run([shutil.which('cc') or 'clang', '-shared', '-fPIC', '-O1', '-o', cls.stub, str(src)], check=True)
        cls.part_size = dmsh_native.PART_SIZE
        dmsh_native.PART_SIZE = 3

    @classmethod
    def tearDownClass(cls):
        dmsh_native.PART_SIZE = cls.part_size
        shutil.rmtree(cls.tmp)

    def game(self, **kw):
        return Game(tempfile.mkdtemp(dir=self.tmp), **kw)

    def test_parts_and_bundles(self):
        g = self.game()
        build(g, LIB, self.stub)
        names = sorted(f.name for f in (g.stage / 'shaders').iterdir())
        self.assertEqual(names, sorted(k + '.xgs' for k in g.keys + g.qkeys))
        parts = sorted(f.name for f in (g.stage / '.units' / 'shaders').iterdir())
        self.assertEqual(parts, ['cs_a.0.tsv', 'cs_a.1.tsv', 'cs_a.2.tsv', 'cs_b.0.tsv', 'cs_b.1.tsv', 'queries.tsv'])
        listed = []
        for p in parts:
            for line in (g.stage / '.units' / 'shaders' / p).read_text().splitlines():
                path, size, sha = line.split('\t')
                data = (g.stage / path).read_bytes()
                self.assertEqual((len(data), hashlib.sha256(data).hexdigest()), (int(size), sha))
                listed.append(path)
        self.assertEqual(sorted(listed), ['shaders/' + n for n in names])

        k = sorted(g.keys)[0]
        data = (g.stage / 'shaders' / (k + '.xgs')).read_bytes()
        m, v, stage_no, nb, ni, nc, ns, _ = struct.unpack_from('<8I', data, 0)
        self.assertEqual((m, stage_no, nc), (0x31534758, 5, 4))
        spv = data[32 + 32 * nb + 4 * ni + nc:]
        self.assertEqual(len(spv), ns)
        self.assertEqual(spv[:4], SPV[:4])

    def test_resume_keeps_verified_parts(self):
        g = self.game()
        build(g, LIB, self.stub)
        first = tree(g.stage)
        units = g.stage / '.units' / 'shaders'
        digest = {p.stem: hashlib.sha256(p.read_bytes()).hexdigest() for p in units.iterdir()}

        Path(g.stage, '.units', 'shaders.keep').write_text('cs_a.0\t%s\ncs_a.1\t%s\nqueries\t%s\n' % (digest['cs_a.0'], '0' * 64, digest['queries']))
        victim = next(p for p in (g.stage / 'shaders').iterdir() if p.name[:-4] in g.keys[7:])
        victim.write_bytes(b'x' * victim.stat().st_size)
        (g.stage / 'shaders' / 'stray.xgs').write_bytes(b'stray')
        time.sleep(0.01)
        build(g, LIB, self.stub)
        second = tree(g.stage)
        self.assertEqual({p: v[0] for p, v in second.items()}, {p: v[0] for p, v in first.items()}, 'the same bytes after the resume')
        kept = [l.split('\t')[0] for p in ('cs_a.0', 'queries') for l in (units / (p + '.tsv')).read_text().splitlines()]
        for p in kept + ['.units/shaders/cs_a.0.tsv', '.units/shaders/queries.tsv']:
            self.assertEqual(second[p][1], first[p][1], 'kept untouched: ' + p)
        remade = [p for p in second if p not in kept and not p.endswith(('cs_a.0.tsv', 'queries.tsv'))]
        self.assertTrue(remade and all(second[p][1] != first[p][1] for p in remade), 'everything else made again')

    def test_failing_shader(self):
        g0 = self.game()
        bad = g0.keys[8]
        g = self.game(fail=bad)
        with self.assertRaises(dmsh_native.ShaderError) as e:
            build(g, LIB, self.stub)
        self.assertIn(bad, str(e.exception))
        self.assertIn('translate failed', str(e.exception))
        self.assertIn('made to fail', str(e.exception))

    def runner_build(self, g):

        work = g.root / 'work'
        work.mkdir()
        tree_ = Path(g.root, 'tree')
        for f in ('tools/pack_steps.py', 'tools/dmx.py', 'gfx/tools/dmsh_native.py', 'gfx/tools/dmsh_vk.py', 'gfx/tools/dmsh_facts.py',
                  'gfx/tools/pack_builtin_queries.py', 'gfx/tools/pack_game_shaders.py', 'gfx/nvn/nvn_builtin.h', 'verify/nsoimg.py'):
            (tree_ / 'dm' / f).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(DM / f, tree_ / 'dm' / f)
        shutil.copyfile(g.map, tree_ / 'dm' / 'gfx' / 'nvn' / 'shader_map.tsv')
        (tree_ / 'dm' / 'gfx' / 'nvn' / 'shader_facts.tsv').write_text('0000000000000000\t{}\n')
        (tree_ / 'dm' / 'PRODUCT').write_text('test\n')
        keys = sorted(g.keys + g.qkeys)
        paths = ['shaders/%s.xgs' % k for k in keys]
        (tree_ / 'dm' / 'tools' / 'pack_steps.tsv').write_text('shaders\t%d\t%d\t%s\n' % (len(keys), len(keys), pack_steps.path_digest(paths)))
        return pack_steps.Build(dm=str(tree_ / 'dm'), exefs=str(g.exefs), romfs=str(g.data.parent), stage=str(g.stage), work=str(work),
                                python=sys.executable, xlate=self.stub, shader_lib=LIB, queries=str(g.queries)), keys

    def test_parts_made_before_the_runner_looks_are_reported(self):


        g = self.game(n_a=600, n_b=10)
        b, keys = self.runner_build(g)
        units = g.stage / '.units' / 'shaders'
        units.mkdir(parents=True)
        Path(g.stage, '.units', 'shaders.keep').write_text('fingerprint\t' + 'f' * 64 + '\n')
        real, popen, started, events = pack_steps._parts, subprocess.Popen, [], []

        def spawn(*a, **kw):
            p = popen(*a, **kw)
            started.append(p)
            return p

        def slow(*a):
            if len(started) == 1:
                started.append(None)
                time.sleep(2)
            return real(*a)
        pack_steps._parts, subprocess.Popen = slow, spawn
        try:
            pack_steps.run(b, ['shaders'], report=lambda *e: events.append(e), expect=pack_steps.load_expect(os.path.join(b.dm, 'tools', 'pack_steps.tsv')),
                           base_env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
        finally:
            pack_steps._parts, subprocess.Popen = real, popen
        self.assertEqual(len(started), 2, 'the slow look happened')
        made = sorted(f.stem for f in units.glob('*.tsv'))
        self.assertEqual(sorted(e[2] for e in events if e[0] == 'part'), made, 'every finished part is reported')
        self.assertEqual(len(made), 5)
        self.assertEqual((units / '.fingerprint').read_text().strip(), 'f' * 64)

    def test_cancel_kills_the_step_and_its_workers(self):
        g = self.game(n_a=600, n_b=10)
        b, keys = self.runner_build(g)
        env = dict(os.environ, STUB_SLOW_MS='20', PYTHONDONTWRITEBYTECODE='1')
        p = subprocess.Popen([sys.executable, os.path.join(b.dm, 'tools', 'pack_steps.py'), 'run', b.to_json(), 'shaders'], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, env=env, text=True)
        group, parts = None, []
        end = time.time() + 120
        while time.time() < end:
            line = p.stdout.readline()
            if not line:
                break
            f = line.rstrip('\n').split('\t')
            if f[0] == 'start':
                group = int(f[2])
            if f[0] == 'part':
                parts.append((f[2], f[3]))
                if len(parts) == 2:
                    break
        self.assertTrue(group and len(parts) == 2, 'the step starts and reports parts: %r %r' % (group, parts))
        p.stdin.close()
        self.assertEqual(p.wait(timeout=30), 1, 'a cancelled runner ends with a failure status')
        p.stdout.close()
        deadline = time.time() + 10
        while time.time() < deadline:
            try:
                os.killpg(group, 0)
            except ProcessLookupError:
                break
            time.sleep(0.05)
        else:
            self.fail('the step and its workers still run after the cancel')
        self.assertFalse((g.stage / '.units' / 'shaders.tsv').exists(), 'a cancelled step has no manifest')

        Path(g.stage, '.units', 'shaders.keep').write_text(''.join('%s\t%s\n' % x for x in parts))
        before = {x[0]: (g.stage / '.units' / 'shaders' / (x[0] + '.tsv')).stat().st_mtime_ns for x in parts}
        env.pop('STUB_SLOW_MS')
        p2 = subprocess.Popen([sys.executable, os.path.join(b.dm, 'tools', 'pack_steps.py'), 'run', b.to_json(), 'shaders'], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, env=env, text=True)
        lines = []
        while True:
            line = p2.stdout.readline()
            if not line:
                break
            lines.append(line.rstrip('\n').split('\t'))
            if line.startswith(('done', 'error')):
                break
        rc = p2.wait(timeout=30)
        p2.stdin.close()
        self.assertEqual(rc, 0, 'the resumed run finishes: %r' % lines[-1:])
        p2.stdout.close()
        reported = {f[2] for f in lines if f[0] == 'part'}
        self.assertFalse(reported & set(before), 'kept parts are not made or reported again')
        for name, t in before.items():
            self.assertEqual((g.stage / '.units' / 'shaders' / (name + '.tsv')).stat().st_mtime_ns, t)
        self.assertEqual(sorted(f.name for f in (g.stage / 'shaders').iterdir()), [k + '.xgs' for k in keys])


@unittest.skipUnless(LIB and all(shutil.which(t) or os.environ.get(e) for t, e in (('glslc', 'GLSLC'), ('spirv-opt', 'SPIRV_OPT'), ('spirv-val', 'SPIRV_VAL'))),
                     'needs glslc, spirv-opt and spirv-val')
class PostStepsTest(unittest.TestCase):

    SOURCES = {
        'comp': '#version 450\nlayout(local_size_x = 4) in;\nlayout(std430, binding = 1) buffer B { uint v[]; } b;\n'
                'layout(binding = 2) uniform U { uint n; } u;\nvoid main() { b.v[gl_GlobalInvocationID.x] = u.n; }\n',
        'img': '#version 450\n#extension GL_EXT_shader_image_load_formatted : require\nlayout(local_size_x = 1) in;\n'
               'layout(binding = 0) uniform image2D src;\nlayout(binding = 3) uniform writeonly image2D dst;\n'
               'void main() { imageStore(dst, ivec2(0), imageLoad(src, ivec2(1))); }\n',
        'frag': '#version 450\nlayout(location = 0) out vec4 o;\nlayout(binding = 4) uniform sampler2D t;\nlayout(location = 0) in vec2 uv;\n'
                'void main() { o = texture(t, uv) * 2.0; }\n',
    }
    STAGE = {'comp': 5, 'img': 5, 'frag': 4}

    def tool(self, name, env):
        return os.environ.get(env) or shutil.which(name)

    def test_post_steps_match(self):
        lib = ctypes.CDLL(LIB)
        lib.dmsh_post.restype = ctypes.c_int
        lib.dmsh_post.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_int, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_size_t),
                                  ctypes.POINTER(ctypes.c_void_p)]
        lib.dmsh_free.argtypes = [ctypes.c_void_p]
        with tempfile.TemporaryDirectory() as t:
            for name, src in self.SOURCES.items():
                stage = {4: 'frag', 5: 'comp'}[self.STAGE[name]]
                Path(t, name + '.glsl').write_text(src)
                spv_path = Path(t, name + '.spv')
                subprocess.run([self.tool('glslc', 'GLSLC'), '-fshader-stage=' + stage, '--target-env=vulkan1.1', str(Path(t, name + '.glsl')), '-o', str(spv_path)], check=True)
                spv = spv_path.read_bytes()
                if name == 'img':
                    w = list(struct.unpack('<%dI' % (len(spv) // 4), spv))
                    out, i = w[:5], 5
                    while i < len(w):
                        n = w[i] >> 16
                        if not ((w[i] & 0xffff) == 17 and w[i + 1] in (55, 56)):
                            out += w[i:i + n]
                        i += n
                    spv = struct.pack('<%dI' % len(out), *out)

                stage_name = {4: 'ps', 5: 'cs'}[self.STAGE[name]]
                Path(t, 'r.spv').write_bytes(dmsh_vk.rewrite(spv, stage_name))
                subprocess.run([self.tool('spirv-opt', 'SPIRV_OPT'), '--trim-capabilities', str(Path(t, 'r.spv')), '-o', str(Path(t, 't.spv'))], check=True)
                want = dmsh_vk.add_format_caps(Path(t, 't.spv').read_bytes())
                Path(t, 'f.spv').write_bytes(want)
                subprocess.run([self.tool('spirv-val', 'SPIRV_VAL'), '--target-env', 'vulkan1.1', str(Path(t, 'f.spv'))], check=True)
                out, n, err = ctypes.c_void_p(), ctypes.c_size_t(), ctypes.c_void_p()
                rc = lib.dmsh_post(spv, len(spv), self.STAGE[name], ctypes.byref(out), ctypes.byref(n), ctypes.byref(err))
                got = ctypes.string_at(out, n.value) if out.value else b''
                msg = ctypes.string_at(err).decode() if err.value else ''
                lib.dmsh_free(out)
                lib.dmsh_free(err)
                self.assertEqual(rc, 0, name + ': ' + msg)
                self.assertEqual(got, want, name + ': native post-steps differ from the tools')
                if name == 'img':
                    caps = [w for i, op, a in dmsh_vk.insts(struct.unpack('<%dI' % (len(got) // 4), got)) if op == 17 for w in a[:1]]
                    self.assertTrue({55, 56} <= set(caps), 'the format capabilities are added back')

        bad = SPV[:20] + struct.pack('<I', 0)
        out, n, err = ctypes.c_void_p(), ctypes.c_size_t(), ctypes.c_void_p()
        self.assertEqual(lib.dmsh_post(bad, len(bad), 5, ctypes.byref(out), ctypes.byref(n), ctypes.byref(err)), 2)
        lib.dmsh_free(err)


GAME = os.environ.get('DM_PACK_GAME_DIR')
XLATE = os.environ.get('DM_XLATE_LIB')


@unittest.skipUnless(LIB and GAME and XLATE, 'set DM_PACK_GAME_DIR and DM_XLATE_LIB for the real shaders')
class RealShadersTest(unittest.TestCase):
    def test_translator_library_equals_the_tool(self):
        tool = os.environ.get('DM_XLATE_TOOL')
        if not tool:
            self.skipTest('set DM_XLATE_TOOL (the published dmsh_xlate) to compare with the console tool')
        import dmsh_facts
        import dmx
        dmx.ROOT = os.path.join(GAME, 'RomFS', 'data')
        dmx._IDX = None
        loc, stage_of = dmsh_vk.shader_index()
        facts = dmsh_facts.load()
        sample = sorted(k for k in loc if not loc[k][0].startswith('main.'))[::200]
        lib = ctypes.CDLL(XLATE)
        lib.dmsh_xlate_one.restype = ctypes.c_int
        lib.dmsh_xlate_one.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t] + [ctypes.c_void_p] * 5
        lib.dmsh_xlate_free.argtypes = [ctypes.c_void_p]
        with tempfile.TemporaryDirectory() as t:
            jobs, codes, packs = [], {}, {}
            for k in sample:
                name, i = loc[k]
                if name not in packs:
                    packs[name] = dmsh_vk.records(dmx.get('hd/shaders/' + name))
                job, c1, code = dmsh_vk.job_for(k, name, packs[name][i], stage_of, facts, t)
                jobs.append(job)
                codes[k] = (dict(job), code)
            Path(t, 'jobs.json').write_text(json.dumps(jobs))
            subprocess.run([tool, str(Path(t, 'jobs.json')), t], check=True, capture_output=True)
            for k, (job, code) in codes.items():
                job.pop('code')
                j = json.dumps(job).encode()
                spv, sn, res, rn, err = ctypes.c_void_p(), ctypes.c_size_t(), ctypes.c_void_p(), ctypes.c_size_t(), ctypes.c_void_p()
                self.assertEqual(lib.dmsh_xlate_one(j, len(j), code, len(code), ctypes.byref(spv), ctypes.byref(sn), ctypes.byref(res), ctypes.byref(rn), ctypes.byref(err)), 0, k)
                self.assertEqual(ctypes.string_at(spv, sn.value), Path(t, k + '.spv').read_bytes(), k)
                self.assertEqual(ctypes.string_at(res, rn.value), Path(t, k + '.res.json').read_bytes(), k)
                for p in (spv, res):
                    lib.dmsh_xlate_free(p)
        print('test_pack_shaders: %d sample shaders equal through the library and the tool' % len(sample))

    def test_whole_step_against_reference(self):
        ref = os.environ.get('DM_SHADER_REFERENCE')
        if not ref:
            self.skipTest('set DM_SHADER_REFERENCE (shaders/ of a cold PC pack) to compare the whole step')
        with tempfile.TemporaryDirectory() as t:
            q = Path(t, 'queries')
            subprocess.run([sys.executable, str(GFX / 'pack_builtin_queries.py'), '--output', str(q)], check=True, capture_output=True)
            b = pack_steps.Build(dm=str(DM), exefs=os.path.join(GAME, 'ExeFS'), romfs=os.path.join(GAME, 'RomFS'), stage=os.path.join(t, 'stage'),
                                 work=os.path.join(t, 'work'), python=sys.executable, xlate=XLATE, shader_lib=LIB, queries=str(q))
            os.makedirs(b.work)
            p = subprocess.Popen([sys.executable, str(TOOLS / 'pack_steps.py'), 'run', b.to_json(), 'shaders'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
            lines = p.stdout.read().splitlines()
            p.stdin.close()
            self.assertEqual(p.wait(timeout=60), 0, lines[-1:])
            self.assertEqual(lines[-1].split('\t')[0], 'done')
            got = sorted(os.listdir(os.path.join(b.stage, 'shaders')))
            want = sorted(f for f in os.listdir(ref) if f.endswith('.xgs'))
            self.assertEqual(got, want)
            diff = [f for f in want if Path(ref, f).read_bytes() != Path(b.stage, 'shaders', f).read_bytes()]
            self.assertEqual(diff, [], '%d bundles differ' % len(diff))
            print('test_pack_shaders: %d/%d bundles equal to the reference' % (len(want) - len(diff), len(want)))


if __name__ == '__main__':
    unittest.main(verbosity=1)
