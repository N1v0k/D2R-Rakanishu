#!/usr/bin/env python3




import pathlib
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
TOOL = HERE.parent / 'tools' / 'check_player_env.py'
REPO = HERE.parents[2]


def header(names):
    rows = ''.join(f'    X("{n}") \\\n' for n in names)
    return f'#pragma once\n#define DM_PLAYER_ENV_KEEP(X) \\\n{rows}\n'


class CheckPlayerEnv(unittest.TestCase):
    def tree(self, keep, jni='', java='', host=''):
        root = pathlib.Path(self.tmp.name)
        (root / 'dm/android/compat/include').mkdir(parents=True, exist_ok=True)
        (root / 'dm/android/app/src/main/java/x').mkdir(parents=True, exist_ok=True)
        (root / 'dm/host/tests').mkdir(parents=True, exist_ok=True)
        (root / 'dm/gfx').mkdir(parents=True, exist_ok=True)
        (root / 'dm/android/compat/include/dm_player_env.h').write_text(header(keep), encoding='utf-8')
        (root / 'dm/android/debug_env.h').write_text('static const char *const dm_android_debug_keys[]={"DM_3D_SCALE","DM_3D_SCALE_DYN"};\n', encoding='utf-8')
        (root / 'dm/android/jni_android.c').write_text('void f(void) { setenv("DM_SAVE", s, 0); setenv(line, v, 1); }\n' + jni, encoding='utf-8')
        (root / 'dm/android/app/src/main/java/x/A.java').write_text('class A { void f() { Os.setenv("DM_DERIVED", p, true); } }\n' + java, encoding='utf-8')
        (root / 'dm/host/a.c').write_text('void g(void) { setenv("TZ", "UTC", 1); setenv("DM_RIFT_TEST_DONE", "1", 1); }\n' + host, encoding='utf-8')
        (root / 'dm/host/tests/t.c').write_text('void t(void) { setenv("DM_KEYS", "A", 1); }\n', encoding='utf-8')
        return root

    def run_tool(self, root):
        return subprocess.run([sys.executable, str(TOOL), '--root', str(root)], capture_output=True, text=True)

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = ['DM_3D_SCALE', 'DM_3D_SCALE_DYN', 'DM_DERIVED', 'DM_SAVE']

    def tearDown(self):
        self.tmp.cleanup()

    def test_match(self):
        r = self.run_tool(self.tree(self.base))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('4 names', r.stdout)

    def test_app_sets_unlisted_name(self):
        r = self.run_tool(self.tree(self.base, jni='void h(void) { setenv("DM_GFX_NEW", "1", 0); }\n'))
        self.assertEqual(r.returncode, 1)
        self.assertIn('DM_GFX_NEW is set by the App', r.stdout)

    def test_java_sets_unlisted_name(self):
        r = self.run_tool(self.tree(self.base, java='class B { void f() { Os.setenv("DM_OTHER", p, true); } }\n'))
        self.assertEqual(r.returncode, 1)
        self.assertIn('DM_OTHER is set by the App', r.stdout)

    def test_listed_name_never_set(self):
        r = self.run_tool(self.tree(sorted(self.base + ['DM_TEST_GOLD'])))
        self.assertEqual(r.returncode, 1)
        self.assertIn('DM_TEST_GOLD is in dm_player_env.h but the App never sets it', r.stdout)

    def test_unsorted_list(self):
        r = self.run_tool(self.tree(list(reversed(self.base))))
        self.assertEqual(r.returncode, 1)
        self.assertIn('sorted', r.stdout)

    def test_host_sets_product_name(self):
        r = self.run_tool(self.tree(self.base, host='void k(void) { setenv("DM_HOST_FLAG", "1", 1); }\n'))
        self.assertEqual(r.returncode, 1)
        self.assertIn('DM_HOST_FLAG is set by dm/host/a.c', r.stdout)

    def test_repository_matches(self):
        r = self.run_tool(REPO)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == '__main__':
    unittest.main()
