import io
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import zipfile

ANDROID = Path(__file__).resolve().parents[1]
TOOLS = ANDROID / 'tools'
BASH = os.environ.get('DM_TEST_BASH', 'bash')


class ReleaseTools(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.env = dict(os.environ)

    def script(self, path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#!/bin/bash\n' + text)
        path.chmod(0o755)

    def run_shell(self, *args):
        return subprocess.run([BASH, *map(str, args)], env=self.env, text=True, capture_output=True)

    def variant(self, variant, release=False, markers=None, entries=(), flags=(), package='io.r4k4n1shu.d2r', shared='S2X/D2R Mobile',
                extract=True, dex='fixture'):
        bt = self.root / 'sdk/build-tools/1'
        debug = variant == 'qa' and not release
        version = '2.3.1-player' if variant == 'player' else '2.3.1'
        self.script(bt / 'aapt2', f'''if [ "$2" = badging ]; then
  echo "package: name='{package}' versionCode='20301' versionName='{version}'"
else echo 'A: http://schemas.android.com/apk/res/android:debuggable(0x0101000f)={str(debug).lower()}'
  echo 'A: http://schemas.android.com/apk/res/android:extractNativeLibs(0x010104ea)={str(extract).lower()}'
  echo '  E: meta-data (line=60)'
  echo '    A: http://schemas.android.com/apk/res/android:name(0x01010003)="dmSharedRoot" (Raw: "dmSharedRoot")'
  echo '    A: http://schemas.android.com/apk/res/android:value(0x01010024)="{shared}" (Raw: "{shared}")'; fi
''')
        self.script(bt / 'apksigner', "echo 'certificate SHA-256 digest: abc'\n")
        self.env['ANDROID_HOME'] = str(self.root / 'sdk')
        apk = self.root / 'test.apk'
        if markers is None:
            markers = 'boot: variant ' + variant
            if variant == 'qa':
                markers += '\nboot: 环境'
                if not release:
                    markers += '\nDM_TEST_RECIPE_STOCK\n[qa] '
        with zipfile.ZipFile(apk, 'w') as z:
            z.writestr('lib/arm64-v8a/libdm.so', markers)
            z.writestr('classes.dex', dex)
            for e in ('lib/arm64-v8a/libpackbuild.so', 'lib/arm64-v8a/libdeswizzle.so') + tuple(entries):
                z.writestr(e, self.tools_zip() if e == 'assets/py/tools.zip' else 'x')
        flags = (['--release'] if release else []) + list(flags)
        return self.run_shell(TOOLS / 'check_variant.sh', apk, variant, '--cert-sha256', 'abc', *flags)


    BUILDER = ['assets/py/VERSION', 'assets/py/runtime.zip', 'assets/py/tools.zip', 'assets/py/queries.zip', 'lib/arm64-v8a/libpylauncher.so',
               'lib/arm64-v8a/libpython3.14.so', 'lib/arm64-v8a/libdmsh_xlate.so', 'lib/arm64-v8a/libdmsh_shaders.so', 'lib/arm64-v8a/libmonpic.so',
               'lib/arm64-v8a/libfreetype.so', 'lib/arm64-v8a/libharfbuzz.so', 'lib/arm64-v8a/libfribidi.so', 'lib/arm64-v8a/libwebp.so',
               'lib/arm64-v8a/libwebpmux.so', 'lib/arm64-v8a/libwebpdemux.so', 'lib/arm64-v8a/libsharpyuv.so']

    def tools_zip(self, extra=None):
        import sys
        sys.path.insert(0, str(ANDROID / 'python'))
        import pack_assets
        out = io.BytesIO()
        with zipfile.ZipFile(out, 'w') as z:
            for rel, src in sorted(pack_assets.tool_files(str(ANDROID.parents[1]), variant='player').items()):
                z.write(src, rel)
            for rel, text in (extra or self.tools_extra).items():
                z.writestr(rel, text)
        return out.getvalue()

    tools_extra = {}

    def test_builder_complete_or_absent(self):
        cases = [('complete', self.BUILDER, [], 0), ('absent', [], [], 0), ('absent, public', [], ['--public'], 1),
                 ('complete, public', self.BUILDER, ['--public'], 0),
                 ('one library missing', [e for e in self.BUILDER if not e.endswith('libmonpic.so')], [], 1),
                 ('unknown asset', self.BUILDER + ['assets/py/extra.zip'], [], 1)]
        for name, entries, flags, rc in cases:
            with self.subTest(name):
                result = self.variant('player', True, entries=entries, flags=flags)
                self.assertEqual(result.returncode, rc, result.stdout + result.stderr)
        self.assertIn('libmonpic.so', self.variant('player', True, entries=cases[4][1]).stdout)

    def test_builder_needs_extracted_libraries(self):
        result = self.variant('player', True, entries=self.BUILDER, extract=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('extractNativeLibs', result.stdout)

    def test_public_needs_default_folder_and_release_id(self):
        for name, kw in [('own shared folder', {'shared': 'pb105s6'}), ('test application id', {'package': 'io.r4k4n1shu.pb105s6'})]:
            with self.subTest(name):
                self.assertEqual(self.variant('player', True, entries=self.BUILDER, **kw).returncode, 0)
                result = self.variant('player', True, entries=self.BUILDER, flags=['--public'], **kw)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn('public:', result.stdout)

    def test_qa_code_in_the_player(self):
        result = self.variant('player', True, dex='xx Lcom/xiaoboluo/d2r/StepsOnly; xx')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('StepsOnly', result.stdout)
        result = self.variant('player', True, dex='xx qa-stage-pack xx')
        self.assertNotEqual(result.returncode, 0)
        for name, extra in [('qa module', {'dm/tools/patch_nso_qa.py': 'x'}), ('test switch', {'dm/tools/x.py': 'DM_TEST_X'}),
                            ('test-only file', {'dm/ios/tools/uireg/uireg.py': 'x'})]:
            with self.subTest(name):
                self.tools_extra = extra
                result = self.variant('player', True, entries=self.BUILDER)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn('G7', result.stdout)
        self.tools_extra = {}

    def test_variant_matrix(self):
        for variant, release in [('qa', False), ('qa', True), ('player', True)]:
            with self.subTest(variant=variant, release=release):
                result = self.variant(variant, release)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_debug_qa_requires_recipe_marker(self):
        result = self.variant('qa', markers='boot: variant qa\nboot: 环境\n[qa] ')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lacks 'DM_TEST_RECIPE_STOCK'", result.stdout)

    def test_release_qa_requires_boot_markers(self):
        result = self.variant('qa', True, 'boot: variant qa')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lacks 'boot: 环境'", result.stdout)

    def test_signing_modes(self):
        mode = self.root / 'signing.mode'
        for value, kind, allowed, success in [
                ('release', 'release', '', True),
                ('debug-fallback', 'debug', '', True),
                ('debug-fallback', 'release', '', False),
                ('debug-fallback', 'release', 'false', False),
                ('debug-fallback', 'release', 'true', True),
                ('debug-fallback', 'release', '1', True),
                ('unknown', 'release', '1', False)]:
            with self.subTest(value=value, kind=kind, allowed=allowed):
                mode.write_text(value + '\n')
                result = self.run_shell(TOOLS / 'check_signing.sh', mode, kind, allowed)
                self.assertEqual(result.returncode == 0, success, result.stderr)
                if value == 'debug-fallback':
                    self.assertIn('WARNING:', result.stderr)
        mode.unlink()
        self.assertNotEqual(self.run_shell(TOOLS / 'check_signing.sh', mode, 'release', '1').returncode, 0)

    def test_shell_verifier_arguments(self):
        source = (TOOLS / 'shell_apk.sh').read_text()
        footer = source[source.index('CERT=$(cat'):source.index('\nt "好了')]
        build = self.root / 'dm/android/app/build'
        build.mkdir(parents=True)
        (build / 'signing.sha256').write_text('abc\n')
        self.script(self.root / 'dm/android/tools/check_variant.sh', 'printf "%s\\n" "$@" > "$CAPTURE"\n')
        self.env.update(W=str(self.root), DM=str(self.root / 'dm'), O=str(self.root),
                        A='test.apk', VARIANT='qa', SFX='', CAPTURE=str(self.root / 'args'))
        for kind in ['debug', 'release']:
            self.env['KIND'] = kind
            result = self.run_shell('-uc', 'die() { exit 1; };\n' + footer)
            self.assertEqual(result.returncode, 0, result.stderr)
            args = (self.root / 'args').read_text().splitlines()
            self.assertEqual('--release' in args, kind == 'release')
            self.assertEqual(args[:4], ['test.apk', 'qa', '--cert-sha256', 'abc'])

    def test_shell_stops_before_assembly(self):
        source = (TOOLS / 'shell_apk.sh').read_text()
        stage = source[source.index('rm -f "$W/dm/android/app/build/signing.mode"'):source.index('cp "$OUT"')]
        build = self.root / 'dm/android/app/build'
        build.mkdir(parents=True)
        self.env.update(W=str(self.root), DM=str(ANDROID.parent), O=str(self.root), G='tables', PY='python3',
                        SFX='', KIND='release', TASK=':app:assembleRelease', OUT=str(self.root / 'app.apk'))
        stub = '''gradle() {
  case " $* " in
    *writeSigningFingerprint*) printf 'abc\\n' > "$W/dm/android/app/build/signing.sha256"; printf 'debug-fallback\\n' > "$W/dm/android/app/build/signing.mode" ;;
    *) touch "$OUT" ;;
  esac
}
die() { echo "$*" >&2; exit 1; }
'''
        for allowed in ['', 'true']:
            self.env['ALLOW_DEBUG_SIGNING'] = allowed
            result = self.run_shell('-uc', stub + stage)
            self.assertEqual(result.returncode == 0, bool(allowed), result.stderr)
            self.assertEqual((self.root / 'app.apk').exists(), bool(allowed))
            self.assertIn('WARNING:', result.stderr)

    def test_notices_inventory(self):
        import sys
        sys.path.insert(0, str(TOOLS))
        import check_notices
        notices = ANDROID.parents[1] / 'THIRD-PARTY-NOTICES.md'
        apk = self.root / 'n.apk'

        def make(libs, tools=None):
            with zipfile.ZipFile(apk, 'w') as z:
                for lib in libs:
                    z.writestr('lib/arm64-v8a/' + lib, 'x')
                if tools is not None:
                    inner = io.BytesIO()
                    with zipfile.ZipFile(inner, 'w') as t:
                        for n in tools:
                            t.writestr(n, 'x')
                    z.writestr('assets/py/tools.zip', inner.getvalue())
            return check_notices.check(str(apk), str(notices))
        self.assertEqual(make(['libdm.so', 'libmonpic.so', 'libfribidi.so', 'libpython3.14.so'], ['dm/tools/patch_nso.py', 'dm/PRODUCT']), [])
        self.assertIn('libplanted.so', ' '.join(make(['libdm.so', 'libplanted.so'])))
        self.assertIn('third_party', ' '.join(make(['libdm.so'], ['dm/third_party/x/y.c'])))
        moved = self.root / 'moved.md'
        text = notices.read_text(encoding='utf-8')
        moved.write_text(text.replace('## libwebp\n', '## Components not inside the app\n\n## libwebp\n', 1), encoding='utf-8')
        self.assertEqual(make(['libmonpic.so']), [])
        self.assertTrue(any('not inside' in b for b in check_notices.check(str(apk), str(moved))))
        moved.write_text(text + '\nThe qa app.\n', encoding='utf-8')
        self.assertTrue(any('qa app' in b for b in check_notices.check(str(apk), str(moved))))


if __name__ == '__main__':
    unittest.main()
