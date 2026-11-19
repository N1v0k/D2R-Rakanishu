#!/usr/bin/env python3

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')


class AcceptTests(unittest.TestCase):
    def setUp(self):
        root = Path(DM_ROOT) / 'run/verify/dm/accept_tests'
        root.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=root)
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.verify = self.root / 'dm/verify'
        (self.verify / 'shotgate_pages').mkdir(parents=True)
        (self.root / 'dm-work/run/verify/dm').mkdir(parents=True)
        self.trace = self.root / 'trace'
        self.env = dict(os.environ, HOME=str(self.root), DM_ROOT=str(self.root / 'dm-work'), LC_ALL='C', TRACE=str(self.trace))
        src = Path(os.environ.get('ACCEPT_SOURCE', Path(__file__).with_name('accept.sh')))
        shutil.copyfile(src, self.verify / 'accept.sh')
        self.write('dm-work/build/cache/dm/venv/bin/python', 'echo "替身 plan test OK"\n')
        self.write('dm/verify/bootchk.sh', 'echo BOOTLIST OK\n')
        self.write('dm/verify/regress.sh', 'echo REGRESS OK\n')
        self.write('dm/verify/test_accept.py', 'import unittest\nclass T(unittest.TestCase):\n    def test(self): pass\n')
        self.write('dm/ios/run.sh', 'echo "替身 iOS 构建 $*"\n')
        self.write('dm/verify/opmode.sh', 'echo "OPMODE ${OPMODE_RES:-OK} 替身"; [ "${OPMODE_RES:-OK}" = OK ]\n')
        self.write('dm/verify/touchchain.sh', 'echo "TOUCHCHAIN $1 OK 用时 0 秒"\n')
        gen = self.root / 'dm/ios/tools/gen_romfs_overlay.py'; gen.parent.mkdir(parents=True, exist_ok=True)
        gen.write_text('import os, sys\nif os.environ.get("GEN_RC"): sys.exit("替身：覆盖层生成失败")\nos.makedirs(sys.argv[2], exist_ok=True)\n')
        for case in ['move', 'inv_tap', 'inv_long_press']:
            case_dir = self.verify / 'touchcases' / case
            case_dir.mkdir(parents=True)
            (case_dir / 'script.txt').write_text('# fixture\n')
            (case_dir / 'assert.py').write_text('# fixture\n')
        self.write('dm/verify/shotgate.sh', 'echo "shot:$*" >> "$TRACE"; echo SHOTGATE_ALL OK\n')
        self.write('dm/verify/logred.sh', 'printf "0\\t\\n"\n')
        self.write('dm/tools/host_savecmp.sh',
                   'mkdir -p "$HOME/host"; printf "DM_TACT_PREFETCH：命中 %s\\n下一拍时限 0 → %s\\n" "${PF_HIT:-4000}" "${LAG_INC:-250}" > "$HOME/host/host.log"; echo "局目录 $HOME/host"; echo SAVECMP 一致\n')
        self.write('dm/tools/save_txn_regress.sh',
                   'echo txn >> "$TRACE"; echo "结论：存档事务回归 4/4 绿"; exit "${TXN_RC:-0}"\n')
        for tree in ['host', 'gfx']:
            p = self.root / f'dm/{tree}/tests'; p.mkdir(parents=True)
            (p / 'Makefile').write_text('run:\n\t@echo OK\nrun-m3:\n\t@echo OK\n')
        p = self.root / 'dm/ios/tests'; p.mkdir(parents=True)
        (p / 'Makefile').write_text('test:\n\t@echo OK\n')
        (self.root / 'dm/host/Makefile').write_text(
            'all:\n\t@test "$${HOST_BUILD_RC:-0}" = 0\n'
            '\t@echo built >> "$$HOME/build-count"\n'
            '\t@touch "$$HOME/host-built"\n'
            'dmhost_m3:\n\t@true\n')
        for p in ['camp_e', 'stash', 'wild', 'charselect', 'skilltree', 'npc']:
            (self.verify / f'shotgate_pages/{p}.json').write_text('{}')

    def write(self, rel, body):
        p = self.root / rel; p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#!/bin/bash\n' + body); p.chmod(0o755)

    def run_accept(self, *args, **env):
        self.env.update(env)
        return subprocess.run(['bash', str(self.verify / 'accept.sh'), *args],
                              env=self.env, capture_output=True, text=True, errors='replace', timeout=10)

    def test_m3_shotgate_runs_only_when_graphics_touched(self):


        for ch, want in (('dm/gfx/nvn/nvn_mtl.m', '旧接口版画面闸门 跑'), ('dm/host/front_mac.m dm/host/dm_rift.c', '旧接口版画面闸门 跑'),
                         ('dm/host/Makefile', '旧接口版画面闸门 跑'), ('dm/host/dm_rift.c dm/host/hle_fs.c', '旧接口版画面闸门 本次改动不涉及，跳过'),
                         ('dm/ios/Sources/main.m docs/x.md', '旧接口版画面闸门 本次改动不涉及，跳过')):
            r = self.run_accept('/fixture/out', ACCEPT_SCOPE_CH=ch, ACCEPT_SCOPE_ONLY='1')
            self.assertIn(want, r.stdout, ch)

    def test_m3_shotgate_row_present(self):


        r = self.run_accept('/fixture/out')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('⑪ 旧接口版画面闸门', r.stdout)
        self.assertEqual(self.trace.read_text().count('shot:'), 2, self.trace.read_text())

    def test_fast_has_only_five_pages_and_no_transaction(self):
        r = self.run_accept('--fast', '/fixture/out')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(self.trace.read_text().strip(), '\n'.join(['shot:/fixture/out camp_e stash wild charselect skilltree'] * 2))

    def test_full_runs_transaction_and_propagates_its_failure(self):
        r = self.run_accept('--full', '/fixture/out', TXN_RC='1')
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertIn('txn', self.trace.read_text())
        self.assertTrue(list((self.root / 'dm-work/run/verify/dm/accept').glob('*/KEEP')))

    def test_default_keeps_full_coverage(self):
        r = self.run_accept('/fixture/out')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('txn', self.trace.read_text())
        self.assertIn('npc', r.stdout)

    def test_pending_case_skipped_but_listed(self):


        d = self.verify / 'touchcases' / 'slow_prefix'
        d.mkdir(parents=True)
        for n in ('script.txt', 'assert.py'): (d / n).write_text('# fixture\n')
        (d / 'pending').write_text('等前缀不限速\n')
        r = self.run_accept('--fast', '/fixture/out')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('FUNC 3/3', r.stdout)
        self.assertIn('FUNCPENDING 待接入 1 例：slow_prefix：等前缀不限速', r.stdout)
        self.assertNotIn('FUNCCASE slow_prefix', r.stdout)

    def test_prefetch_hit_is_report_only(self):


        r = self.run_accept('--fast', '/fixture/out', PF_HIT='2999')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('预取命中 2999 块（只报不判）', r.stdout)

    def test_lag_below_range_is_red(self):
        r = self.run_accept('--fast', '/fixture/out', LAG_INC='199')
        self.assertEqual(r.returncode, 1)
        self.assertIn('LAGRANGE 空跑', r.stdout)

    def test_overlay_generation_failure_is_red(self):
        r = self.run_accept('--fast', '/fixture/out', GEN_RC='1')
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertIn('界面覆盖层生成失败', r.stdout)

    def test_opmode_failure_is_red(self):
        r = self.run_accept('--fast', '/fixture/out', OPMODE_RES='RED')
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertIn('⑩ 局内切运行模式', r.stdout)
        self.assertIn('OPMODE RED', r.stdout)

    def test_host_built_once_before_parallel_runtime_checks(self):
        self.write('dm/verify/shotgate.sh',
                   'test -f "$HOME/host-built" || exit 27; echo SHOTGATE_ALL OK\n')
        self.write('dm/tools/host_savecmp.sh',
                   'test -f "$HOME/host-built" || exit 27; mkdir -p "$HOME/host"; '
                   'printf "DM_TACT_PREFETCH：命中 4000\\n下一拍时限 0 → 250\\n" > "$HOME/host/host.log"; '
                   'echo "局目录 $HOME/host"; echo SAVECMP 一致\n')
        r = self.run_accept('--fast', '/fixture/out')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual((self.root / 'build-count').read_text().splitlines(), ['built'])

    def test_host_build_failure_stops_all_runtime_children(self):
        r = self.run_accept('/fixture/out', HOST_BUILD_RC='1')
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertFalse(self.trace.exists(), '宿主构建失败后不得启动画面或事务子项')
        self.assertTrue(list((self.root / 'dm-work/run/verify/dm/accept').glob('*/KEEP')))


if __name__ == '__main__': unittest.main(verbosity=2)
