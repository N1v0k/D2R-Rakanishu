#!/usr/bin/env python3


import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
import game_addrs

TABLE = '''// test table
GB(main, "9.9.1", "00112233445566778899aabbccddeeff")
GL(main, ADDR_LOW, 0x130000)
GL(main, TEXT_END, 0x800000)
GL(main, RO_START, 0x800000)
GL(main, IMAGE_END, 0x1000000)
GA(main, STAT_GET,      0x5b9290, func, "(unit, stat, layer) -> value")
GA(main, AT_LIFE_CALL,  0x7d6720, site, "call that takes the hit off the life")
GA(main, UI_ROOT,       0xf05458, bss,  "UI root pointer")
'''


def write(d, rel, text):
    p = Path(d) / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)
    return p


class TableTest(unittest.TestCase):
    def table(self, text):
        with tempfile.TemporaryDirectory() as d:
            return game_addrs.Table(str(write(d, 't.def', text)))

    def test_parse(self):
        t = self.table(TABLE)
        self.assertEqual(t.ga.STAT_GET, 0x5b9290)
        self.assertEqual(t.gl.TEXT_END, 0x800000)
        self.assertEqual(t.game.version, '9.9.1')
        self.assertEqual(t.game.build_id, '00112233445566778899aabbccddeeff')
        self.assertEqual(t.by_addr[0x7d6720], 'AT_LIFE_CALL')

    def test_fields(self):
        t = self.table(TABLE + 'GF(main, CAM_ZOOM, 0x1348, 0x5b9300, "camera zoom of the controller singleton")\n')
        self.assertEqual(t.gf.CAM_ZOOM, 0x1348)
        self.assertEqual(t.fields, [('main', 'CAM_ZOOM', 0x1348, 0x5b9300, 'camera zoom of the controller singleton')])
        for why, line in {'duplicate name': 'GF(main, STAT_GET, 0x10, 0x5b9300, "x")', 'unresolved': 'GF_UNRESOLVED(main, F, 0x10, 0x5b9300, "x")',
                          'decimal': 'GF(main, F, 16, 0x5b9300, "x")'}.items():
            with self.subTest(why), self.assertRaises(game_addrs.TableError):
                self.table(TABLE + line + '\n')

    def test_insn_imm(self):
        cases = {0xb94ca2e8: 0xca0,
                 0xf9409a68: 0x130,
                 0x3940a108: 0x28,
                 0x7940a108: 0x50,
                 0x3dc00660: 0x10,
                 0xa9417bfd: 0x10,
                 0x29410a61: 0x8,
                 0xa9bf7bfd: -0x10,
                 0xf85f8268: -0x8,
                 0x9100a000: 0x28,
                 0x91400508: 0x1000,
                 0xd10043ff: 0x10,
                 0x529d3109: 0xe988,
                 0x52a00048: 0x20000,
                 0xaa1303e0: None,
                 0x94000010: None}
        for w, want in cases.items():
            with self.subTest(hex(w)):
                self.assertEqual(game_addrs.insn_imm(w), want)
        self.assertEqual(game_addrs.insn_shape(0xb94ca2e8), game_addrs.insn_shape(0xb94d32e8))
        self.assertNotEqual(game_addrs.insn_shape(0xb94ca2e8), game_addrs.insn_shape(0xb94ca2e9))
        self.assertIsNone(game_addrs.insn_shape(0xaa1303e0))

    def test_errors(self):
        bad = {
            'duplicate name': TABLE + 'GA(main, STAT_GET, 0x5b9300, func, "x")\n',
            'duplicate address': TABLE + 'GA(main, OTHER, 0x5b9290, func, "x")\n',
            'unknown kind': TABLE + 'GA(main, OTHER, 0x5b9300, thing, "x")\n',
            'unknown module': TABLE + 'GA(rtld, OTHER, 0x5b9300, func, "x")\n',
            'stray line': TABLE + 'OTHER = 0x5b9300\n',
            'no build line': TABLE.replace('GB(', '// GB('),
            'quote in note': TABLE + 'GA(main, OTHER, 0x5b9300, func, "a "b"")\n',
        }
        for why, text in bad.items():
            with self.subTest(why), self.assertRaises(game_addrs.TableError):
                self.table(text)

    def test_header_matches_loader(self):

        cc = next((c for c in ('cc', 'gcc', 'clang') if subprocess.run(['sh', '-c', f'command -v {c}'], capture_output=True).returncode == 0), None)
        if not cc:
            self.skipTest('no C compiler')
        with tempfile.TemporaryDirectory() as d:
            write(d, 'game_addrs.def', TABLE)
            (Path(d) / 'game_addrs.h').write_bytes((TOOLS.parent / 'host' / 'game_addrs.h').read_bytes())
            write(d, 'game_addrs.def', TABLE + 'GF(main, CAM_ZOOM, 0x1348, 0x5b9300, "x")\n')
            write(d, 'm.c', '#include <stdio.h>\n#include "game_addrs.h"\n'
                  'int main(void) { printf("%x %x %x %x %x %s %s\\n", GA_STAT_GET, GA_AT_LIFE_CALL, GA_UI_ROOT, GL_TEXT_END, GF_CAM_ZOOM, '
                  'GAME_VERSION_main, GAME_BUILD_ID_main); return 0; }\n')
            subprocess.run([cc, '-std=c11', '-Wall', '-Werror', '-o', f'{d}/m', f'{d}/m.c'], check=True)
            out = subprocess.run([f'{d}/m'], check=True, capture_output=True, text=True).stdout
        self.assertEqual(out, '5b9290 7d6720 f05458 800000 1348 9.9.1 00112233445566778899aabbccddeeff\n')


class CodemodTest(unittest.TestCase):
    C_IN = ('#include <stdint.h>\n'
            '// stat total, main+0x5b9290\n'
            'static int f(uint64_t b) { return ((int (*)(void))(b + 0x5b9290))() + 0x7d6720u + 0x5b9300; }\n'
            'static const char *s = "main+0x7d6720";\n')
    C_CODE = ('#include <stdint.h>\n'
              '#include "game_addrs.h"\n'
              '// stat total, main+0x5b9290\n'
              'static int f(uint64_t b) { return ((int (*)(void))(b + GA_STAT_GET))() + (uint32_t)GA_AT_LIFE_CALL + 0x5b9300; }\n'
              'static const char *s = "main+0x7d6720";\n')
    PY_IN = '# root at 0xf05458\nROOT = 0xf05458\nTEXT = "0xf05458"\n'

    def run_codemod(self, files, comments):
        with tempfile.TemporaryDirectory() as d:
            tp = write(d, 'host/game_addrs.def', TABLE)
            t = game_addrs.Table(str(tp))
            out = {}
            for rel, text in files.items():
                p = write(d, rel, text)
                new, _ = game_addrs.rewrite(str(p), text, t, [], comments)
                again, n = game_addrs.rewrite(str(p), new, t, [], comments)
                self.assertEqual((again, n), (new, 0), 'codemod must be idempotent')
                out[rel] = new
            return out

    def test_code_only(self):
        out = self.run_codemod({'host/x.c': self.C_IN, 'tools/y.py': self.PY_IN}, comments=False)
        self.assertEqual(out['host/x.c'], self.C_CODE)
        self.assertEqual(out['tools/y.py'], '# root at 0xf05458\nROOT = GA.UI_ROOT\nTEXT = "0xf05458"\n')

    def test_comments(self):
        out = self.run_codemod({'host/x.c': self.C_IN, 'tools/y.py': self.PY_IN}, comments=True)
        self.assertIn('// stat total, main+GA_STAT_GET\n', out['host/x.c'])
        self.assertIn('"main+0x7d6720"', out['host/x.c'])
        self.assertEqual(out['tools/y.py'], '# root at GA.UI_ROOT\nROOT = GA.UI_ROOT\nTEXT = "0xf05458"\n')

    def test_include_path_is_relative(self):
        out = self.run_codemod({'host/tests/t.c': '#include <stdio.h>\nint x = 0x5b9290;\n'}, comments=False)
        self.assertEqual(out['host/tests/t.c'], '#include <stdio.h>\n#include "../game_addrs.h"\nint x = GA_STAT_GET;\n')


class CheckTest(unittest.TestCase):
    def check(self, files, allow='', table=TABLE):
        sys.path.insert(0, str(TOOLS))
        import check_game_addrs
        with tempfile.TemporaryDirectory() as d:
            write(d, 'dm/host/game_addrs.def', table)
            write(d, 'dm/tools/game_addrs_allow.tsv', allow)
            for rel, text in files.items():
                write(d, rel, text)
            subprocess.run(['git', 'init', '-q', d], check=True)
            subprocess.run(['git', '-C', d, 'add', '-A'], check=True)
            t = game_addrs.Table(os.path.join(d, 'dm/host/game_addrs.def'))
            return check_game_addrs.check(d, t)

    USES = 'a = GA.STAT_GET + GA.AT_LIFE_CALL + GA.UI_ROOT\n'

    def test_clean(self):
        self.assertEqual(self.check({'dm/a.py': self.USES + '# 0x5b9300 only documents the old version\n'}), [])

    def test_findings(self):
        f = self.check({'dm/a.py': self.USES + 'x = 0x5b9300\n# at 0x5b9290\ns = "0x5b9290"\n', 'docs/n.md': 'see 0x7d6720\n'})
        self.assertEqual(len(f), 3, f)
        self.assertTrue(f[0].startswith('dm/a.py:2: 0x5b9300 address-range literal'), f)
        self.assertTrue(f[1].startswith('dm/a.py:3: 0x5b9290 table address STAT_GET'), f)
        self.assertTrue(f[2].startswith('docs/n.md:1: 0x7d6720 table address AT_LIFE_CALL'), f)

    def test_allow_and_unused(self):
        f = self.check({'dm/a.py': 'x = 0x5b9300 + GA.STAT_GET\n'}, allow='0x5b9300\tdm/a.py\tflag\n')
        self.assertEqual(sorted(f), ['dm/host/game_addrs.def: AT_LIFE_CALL is used nowhere',
                                     'dm/host/game_addrs.def: UI_ROOT is used nowhere'])

    def test_string_counts_as_use(self):
        f = self.check({'dm/a.py': 'x = GA.STAT_GET\nlog = "at 0x7d6720"\n', 'dm/b.c': 'int y = GA_UI_ROOT;\n'})
        self.assertEqual(f, [])

    def test_unused_field(self):
        f = self.check({'dm/a.py': self.USES, 'dm/b.c': 'int z = GF_CAM_ZOOM;\n'}, table=TABLE + 'GF(main, CAM_ZOOM, 0x1348, 0x5b9300, "x")\n'
                       'GF(main, OTHER, 0x10, 0x5b9304, "x")\n')
        self.assertEqual(f, ['dm/host/game_addrs.def: field OTHER is used nowhere'])

    def test_unaligned_code_value_is_not_an_address(self):
        self.assertEqual(self.check({'dm/a.py': self.USES + 'x = 0x5b9302\n'}), [])


if __name__ == '__main__':
    unittest.main()
