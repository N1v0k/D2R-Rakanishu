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

    def variant(self, variant, release=False, markers=None):
        bt = self.root / 'sdk/build-tools/1'
        debug = variant == 'qa' and not release
        version = '2.3.1-player' if variant == 'player' else '2.3.1'
        self.script(bt / 'aapt2', f'''if [ "$2" = badging ]; then
  echo "package: name='io.r4k4n1shu.d2r' versionCode='20301' versionName='{version}'"
else echo 'android:debuggable={str(debug).lower()}'; fi
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
            z.writestr('classes.dex', 'fixture')
        flags = ['--release'] if release else []
        return self.run_shell(TOOLS / 'check_variant.sh', apk, variant, '--cert-sha256', 'abc', *flags)

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


if __name__ == '__main__':
    unittest.main()
