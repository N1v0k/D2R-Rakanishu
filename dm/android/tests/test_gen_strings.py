#!/usr/bin/env python3







import os
import re
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
DM = HERE.parents[1]
sys.path.insert(0, str(DM / 'android' / 'tools'))
import gen_strings

SETS = {'values': 'en', 'values-zh': 'zh', 'values-es': 'es', 'values-b+es+419': 'mx', 'values-ru': 'ru', 'values-ko': 'ko', 'values-de': 'de', 'values-pt': 'pt'}


def load(folder):
    root = ET.parse(folder / 'strings.xml').getroot()
    return {e.get('name'): e for e in root.findall('string')}


class GenStrings(unittest.TestCase):
    def test_shader_recovery_folder(self):
        for folder in SETS:
            message = ''.join(load(Path(self.tmp.name) / folder)['dmAndroidMissingShaders'].itertext())
            self.assertIn('Download/S2X/D2R Mobile/r4k4n1shu/', message)
            self.assertNotIn('Download/S2X/D2R Mobile/ ', message)

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        out = Path(cls.tmp.name)
        gen_strings.generate(out)
        cls.sets = {f: load(out / f) for f in SETS}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_every_language_has_its_code(self):
        for folder, code in SETS.items():
            self.assertEqual(self.sets[folder]['dmAndroidStringsLang'].text, code, folder)
            self.assertEqual(self.sets[folder]['dmAndroidStringsLang'].get('translatable'), 'false', folder)

    def test_runtime_table_error_uses_pack_repair_message(self):
        activity = (DM / 'android/app/src/main/java/com/xiaoboluo/d2r/DmActivity.java').read_text()
        self.assertIn('case -11 -> R.string.dmAndroidImportArt;', activity)
        for folder in SETS:
            text = self.sets[folder]['dmAndroidImportArt'].text
            self.assertTrue(text, folder)
            self.assertNotIn('.build.config', text)
        self.assertIn('make it again', self.sets['values']['dmAndroidImportArt'].text)

    def test_same_keys_everywhere(self):
        keys = set(self.sets['values'])
        for folder in SETS:
            self.assertEqual(set(self.sets[folder]), keys, folder)

    def test_languages_are_their_own(self):
        life = {f: self.sets[f]['dmHudLife'].text for f in SETS}
        self.assertTrue(life['values'].startswith('Life'))
        self.assertTrue(life['values-de'].startswith('Leben'))
        self.assertRegex(life['values-ru'], '[А-я]')
        self.assertRegex(life['values-ko'], '[가-힣]')
        self.assertRegex(life['values-zh'], '[一-鿿]')
        self.assertTrue(life['values-es'].startswith('Vida'))
        self.assertTrue(life['values-pt'].startswith('Vida'))
        self.assertEqual(life['values-b+es+419'], life['values-es'])

    def test_multi_argument_formats_are_marked(self):
        for folder in SETS:
            for key in ('dmHudLife', 'dmHudMana', 'dmAndroidHudLevel'):
                self.assertEqual(self.sets[folder][key].get('formatted'), 'false', (folder, key))

    def test_group_mapping_mirrored(self):

        c = (DM / 'host' / 'dm_i18n.c').read_text(encoding='utf-8')
        self.assertIn('int dm_lang_group_of(int l) { return l == 8 || l == 9 ? DM_LG_ZH : l == 2 ? DM_LG_ES : l == 10 ? DM_LG_MX : l == 7 ? DM_LG_RU : '
                      'l == 5 ? DM_LG_KO : l == 1 ? DM_LG_DE : l == 12 ? DM_LG_PT : DM_LG_EN; }', c)
        j = (DM / 'android' / 'app' / 'src' / 'main' / 'java' / 'com' / 'xiaoboluo' / 'd2r' / 'GameLang.java').read_text(encoding='utf-8')
        body = j[j.index('static String groupOf'):j.index('static int fromTag')]
        got = dict(re.findall(r'case (\d+):(?: case \d+:)* return (\w+);', body))
        self.assertEqual(got, {'8': 'ZH', '2': 'ES', '10': 'MX', '7': 'RU', '5': 'KO', '1': 'DE', '12': 'PT'})


if __name__ == '__main__':
    unittest.main()
