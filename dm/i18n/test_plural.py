#!/usr/bin/env python3



import os
import re
import sys
import tempfile
import unittest
from pathlib import Path

DM = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(DM / 'ios' / 'tools'))
import dm_i18n

HEAD = '键\t中文\tEnglish\tEspañol\tEspañol MX\tРусский\t한국어\tDeutsch\tPortuguês (Brasil)\t用在哪\n'


def table(*lines):

    with tempfile.NamedTemporaryFile('w', suffix='.tsv', delete=False, encoding='utf-8') as f:
        f.write(HEAD + ''.join('\t'.join(list(l) + ['']) + '\n' for l in lines))
    old, old_composed = dm_i18n.TABLE, dm_i18n.COMPOSED
    dm_i18n.TABLE, dm_i18n.COMPOSED = f.name, {}
    try:
        return dm_i18n.rows()
    finally:
        dm_i18n.TABLE, dm_i18n.COMPOSED = old, old_composed
        os.unlink(f.name)


GOOD = ('dmTestCount', '%d 件', 'one=%d item|other=%d items', 'one=%d objeto|other=%d objetos', '',
        'one=%d предмет|few=%d предмета|many=%d предметов|other=%d предмета', '%d개', 'one=%d Gegenstand|other=%d Gegenstände', '')


class Forms(unittest.TestCase):
    def test_parse(self):
        self.assertIsNone(dm_i18n.forms('%d items'))
        self.assertEqual(dm_i18n.forms('one=%d item|other=%d items'), {'one': '%d item', 'other': '%d items'})
        for bad in ('one=%d item', 'one=a|one=b|other=c', 'one=a|other=b|lots=c', 'a|b', 'other=a|'):
            with self.assertRaises(ValueError, msg=bad):
                dm_i18n.forms(bad)

    def test_stamp_and_plain(self):
        self.assertEqual(dm_i18n.stamp('one=a|other=b', 'de'), 'de|one=a|other=b')
        self.assertEqual(dm_i18n.stamp('plain', 'de'), 'plain')
        self.assertEqual(dm_i18n.plain('one=a|other=b'), 'b')

    def test_good_row(self):
        r = table(GOOD)[0]
        self.assertEqual(dm_i18n.out(r, 'en'), 'en|' + GOOD[2])
        self.assertEqual(dm_i18n.out(r, 'mx'), 'es|' + GOOD[3], 'empty Español MX: the Spanish forms with Spanish rules')
        self.assertEqual(dm_i18n.out(r, 'pt'), 'en|' + GOOD[2], 'empty Português: the English forms with English rules, never pt rules')
        self.assertEqual(dm_i18n.out(r, 'ru'), 'ru|' + GOOD[5])
        self.assertEqual(dm_i18n.out(r, 'ko'), GOOD[6])
        self.assertEqual(dm_i18n.out(r, 'zh'), GOOD[1])

    def test_table_rules(self):
        def row(**cells):
            r = list(GOOD)
            for k, v in cells.items(): r[('k', 'zh', 'en', 'es', 'mx', 'ru', 'ko', 'de', 'pt').index(k)] = v
            return r
        for why, bad in (('few in English', row(en='one=%d item|few=%d items|other=%d items')),
                         ('forms in Chinese', row(zh='one=%d 件|other=%d 件')),
                         ('forms in Korean', row(ko='one=%d개|other=%d개')),
                         ('a form without the placeholder', row(de='one=ein Gegenstand|other=%d Gegenstände')),
                         ('a form with another placeholder', row(es='one=%s objeto|other=%d objetos')),
                         ('no other form', row(pt='one=%d item')),
                         ('a plain text with the separator', row(de='%d Gegenstände | mehr')),
                         ('a form with a newline the English lacks', row(de='one=%d\\nGegenstand|other=%d Gegenstände'))):
            with self.assertRaises(SystemExit, msg=why):
                table(bad)

    def test_composed_limit(self):
        frame = ('dmTestFrame', '第 %d 层：%s、%s', 'Level %d: %s, %s', 'Nivel %d: %s, %s', '', 'Уровень %d: %s, %s', '%d층: %s, %s', 'Stufe %d: %s, %s', '')
        part = ('dmTestPart', '%d 点', 'one=+%d Point|other=+%d Points', 'one=+%d punto|other=+%d puntos', '', 'one=+%d очко|few=+%d очка|many=+%d очков|other=+%d очка',
                '+%d', 'one=+%d Punkt|other=+%d Punkte', '')
        long_part = part[:7] + ('one=+%d Punkt|other=+%d Punkte, die man irgendwo ganz lange beschreibt',) + part[8:]
        old, old_composed = dm_i18n.EN_LIMITS, dm_i18n.COMPOSED
        dm_i18n.EN_LIMITS = ((r'dmTestFrame', 40),)
        try:
            rows = table(frame, part, part[:0] + ('dmTestPart2',) + part[1:])
            dm_i18n.COMPOSED = {'dmTestFrame': ('dmTestPart', 'dmTestPart2')}
            dm_i18n.composed(rows)
            rows = table(frame, part, ('dmTestPart2',) + long_part[1:])
            with self.assertRaises(SystemExit):
                dm_i18n.composed(rows)
        finally:
            dm_i18n.EN_LIMITS, dm_i18n.COMPOSED = old, old_composed


class RealTable(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = dm_i18n.rows()
        cls.plural = {r[0] for r in cls.rows if any(dm_i18n.forms(t) for t in r[1:] if t)}

    def test_plain_rows_unchanged(self):

        old = lambda l, zh, en, es, mx, ru, ko, de, pt: zh if l in ('zhCN', 'zhTW') else es if l == 'esES' else mx if l == 'esMX' else ru if l == 'ruRU' else ko if l == 'koKR' else de if l == 'deDE' else (pt or en) if l == 'ptBR' else en
        for r in self.rows:
            if r[0] in self.plural: continue
            for l in dm_i18n.LANGS:
                self.assertEqual(dm_i18n.out(r, dm_i18n.GAME_COLUMN.get(l, 'en')), old(l, *r[1:]), (r[0], l))

    def test_game_columns(self):
        r = next(r for r in self.rows if r[0] == 'dmCmpMore')
        self.assertTrue(dm_i18n.out(r, dm_i18n.GAME_COLUMN.get('ptBR')).startswith('en|'))
        self.assertTrue(dm_i18n.out(r, dm_i18n.GAME_COLUMN.get('frFR', 'en')).startswith('en|'))
        self.assertTrue(dm_i18n.out(r, dm_i18n.GAME_COLUMN.get('deDE')).startswith('de|'))
        r = next(r for r in self.rows if r[0] == 'dmAndroidBestUnrated')
        self.assertTrue(dm_i18n.out(r, dm_i18n.GAME_COLUMN.get('ptBR')).startswith('pt|'))

    def test_expected_rows_have_forms(self):
        want = {'dmAndroidBestUnrated', 'dmAndroidBestSkipped', 'dmAndroidBestPages', 'dmAndroidBestEquipDoneN', 'dmAndroidBestTakeOffDoneN',
                'dmAndroidBuildFreeSkill', 'dmAndroidBuildFreeStat', 'dmAndroidBuildOutside', 'dmRiftPtsSkill', 'dmRiftPtsStat', 'dmKbKilledTotal',
                'dmCmpMore', 'dmCmpSkillMore', 'dmImpOver', 'dmVaultSearch', 'dmStoreLeft', 'dmGearBelow', 'dmGearBelowSame', 'dmImpFrames',
                'dmImpKeep', 'dmImpKeepNext', 'dmAndroidWidgetFooter', 'dmAndroidWidgetPartial', 'dmAndroidFxPoisonLeft', 'dmAndroidBookPlayersShort'}
        self.assertEqual(self.plural, want)

    def test_read_only_with_a_count(self):


        cores = {('dm_itemcmp.c', 'dmImpOver'), ('dm_itemcmp.c', 'dmImpFrames'), ('dm_itemcmp.c', 'dmImpKeep'), ('dm_itemcmp.c', 'dmImpKeepNext'),
                 ('dm_itemcmp.c', 'dmCmpSkillMore'), ('dm_sim_live.inc', 'dmGearBelow'), ('dm_sim_live.inc', 'dmGearBelowSame'),
                 ('dm_gear_stock.inc', 'dmGearBelow'), ('dm_gear_stock.inc', 'dmGearBelowSame')}
        bad = []
        for path in list((DM / 'host').glob('*.c')) + list((DM / 'host').glob('*.inc')) + list((DM / 'ios/Sources').glob('*.m')) + list((DM / 'ios/Sources').glob('*.c')):
            for n, line in enumerate(path.read_text(encoding='utf-8', errors='replace').split('\n'), 1):
                for k in self.plural:
                    for m in re.finditer(r'"%s"' % k, line):
                        if 'dm_strn(' in line[:m.start()].rsplit(';', 1)[-1]: continue
                        if (path.name, k) in cores and 'dm_strn' not in line: continue
                        bad.append('%s:%d %s' % (path.name, n, k))

        java = DM / 'android/app/src/main/java/com/xiaoboluo/d2r'
        own = {('StatusEffects.java', 'dmAndroidFxPoisonLeft')}
        for path in java.rglob('*.java'):
            for n, line in enumerate(path.read_text(encoding='utf-8').split('\n'), 1):
                for k in self.plural:
                    for m in re.finditer(r'R\.string\.%s\b' % k, line):
                        if re.search(r'(?:\btext|\bs|AppText\.get)\($', line[:m.start()]) or (path.name, k) in own: continue
                        if 'Plural.format(' in line or 'Plural.format(' in path.read_text(encoding='utf-8').split('\n')[n - 2]: continue
                        bad.append('%s:%d %s' % (path.name, n, k))
        self.assertIn('Plural.pick(t.own("PoisonLeft")', (java / 'EffectDetail.java').read_text(encoding='utf-8'))
        self.assertEqual(bad, [])

    def test_cores_pick(self):

        for name, keys in (('dm_itemcmp_core.c', ('w->over', 'w->to', 'w->keep', 'w->keep_next', 'w->skill_more')), ('dm_gear_core.c', ('w->below', 'w->below_same'))):
            text = (DM / 'host' / name).read_text(encoding='utf-8')
            for k in keys:
                self.assertRegex(text, r'dm_plural\(%s,' % re.escape(k), (name, k))


if __name__ == '__main__':
    unittest.main()
