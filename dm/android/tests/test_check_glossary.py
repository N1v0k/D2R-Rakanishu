#!/usr/bin/env python3









import copy
import json
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import check_glossary as cg
import gen_glossary_data as gg


def repo():
    with open(cg.DEFAULT, encoding='utf-8') as fh: return json.load(fh)


class Checks(unittest.TestCase):
    def test_repo_passes(self):
        self.assertEqual(list(cg.check(repo())), [])

    def test_fifteen_terms_three_categories(self):
        d = repo()
        self.assertEqual(len(d['terms']), 15)
        self.assertEqual({t['cat'] for t in d['terms']}, set(cg.CATEGORIES))

    def broken(self, change):
        d = copy.deepcopy(repo()); change(d); return list(cg.check(d))

    def test_missing_language(self):
        self.assertTrue(self.broken(lambda d: d['terms'][0]['text']['what'].pop('en')))

    def test_one_site_only(self):
        self.assertTrue(self.broken(lambda d: d['terms'][0].__setitem__('sources', ['mx-hit', 'mx-dr'])))

    def test_unknown_source(self):
        self.assertTrue(self.broken(lambda d: d['terms'][0].__setitem__('sources', ['mx-hit', 'nowhere'])))

    def test_http_url(self):
        self.assertTrue(self.broken(lambda d: d['sources']['mx-hit'].__setitem__('url', 'http://maxroll.gg/d2')))

    def test_breakpoints_must_step(self):
        self.assertTrue(self.broken(lambda d: d['breakpoints']['fcr']['classes']['sor'].__setitem__(2, [5, 11])))
        self.assertTrue(self.broken(lambda d: d['breakpoints']['fhr']['classes'].pop('war')))

    def test_phrases(self):
        self.assertTrue(self.broken(lambda d: d['phrases'].pop('hitYou')))
        self.assertTrue(self.broken(lambda d: d['phrases']['def'].__setitem__('en', 'Defense: [{1}]')))
        self.assertTrue(self.broken(lambda d: d['phrases']['def'].__setitem__('de', '[a] [b]')))

    def test_limits_both_or_none(self):
        self.assertTrue(self.broken(lambda d: d['terms'][0]['limits'].__setitem__('en', '')))

    @unittest.skipUnless(os.environ.get('DM_GLOSSARY_TABLES'), 'DM_GLOSSARY_TABLES not set')
    def test_tables(self):
        self.assertEqual(list(cg.check_tables(repo(), os.environ['DM_GLOSSARY_TABLES'])), [])

    @unittest.skipUnless(os.environ.get('DM_GLOSSARY_TABLES'), 'DM_GLOSSARY_TABLES not set')
    def test_tables_amplify_and_diablo(self):

        src = os.environ['DM_GLOSSARY_TABLES']
        with tempfile.TemporaryDirectory() as d:
            for f in os.listdir(src):
                if f.endswith('.txt'): os.symlink(os.path.join(src, f), os.path.join(d, f))
            for f in ('skills.txt', 'monstats.txt'):
                os.unlink(os.path.join(d, f))
                with open(os.path.join(src, f), encoding='utf-8-sig') as fh: lines = fh.read().split('\n')
                head = lines[0].split('\t')
                col = head.index('Param5' if f == 'skills.txt' else 'ResDm(H)')
                for i, l in enumerate(lines):
                    c = l.split('\t')
                    if (f == 'skills.txt' and c[0] == 'Amplify Damage') or (f == 'monstats.txt' and c[0] == 'diablo'):
                        c[col] = '50'; lines[i] = '\t'.join(c)
                with open(os.path.join(d, f), 'w', encoding='utf-8') as fh: fh.write('\n'.join(lines))
            got = ' '.join(cg.check_tables(repo(), d))
            self.assertIn('Amplify Damage', got)
            self.assertIn('Diablo', got)


def monlvl(levels=111, bad=None):
    cols = ['Level'] + ['%s%s%s' % (p, c, d) for c in ('AC', 'TH') for p in ('', 'L-') for d in ('', '(N)', '(H)')]
    rows = ['\t'.join(cols)]
    for lv in range(levels):
        vals = [str(lv)] + [str(lv * (i + 1)) for i in range(len(cols) - 1)]
        if bad == lv: vals[3] = 'x'
        rows.append('\t'.join(vals))
    return ('\n'.join(rows) + '\n').encode('utf-8')


DIFF = ('Name\tLifeStealDivisor\tManaStealDivisor\nNormal\t1\t1\nNightmare\t2\t2\nHell\t3\t3\n').encode('utf-8')


class Pack(unittest.TestCase):
    def test_build(self):
        d = gg.build(monlvl(), DIFF)
        self.assertEqual(d['schema'], 1)
        self.assertEqual(len(d['expansion']['ar']), 3)
        self.assertEqual(len(d['expansion']['ar'][0]), 111)
        self.assertEqual(d['leech'], [[1, 1], [2, 2], [3, 3]])
        self.assertNotEqual(d['expansion']['ar'][0][10], d['classic']['ar'][0][10])

    def test_refuses(self):
        with self.assertRaises(gg.Refuse): gg.build(monlvl(100), DIFF)
        with self.assertRaises(gg.Refuse): gg.build(monlvl(bad=5), DIFF)
        with self.assertRaises(gg.Refuse): gg.build(monlvl(), b'Name\tLifeStealDivisor\tManaStealDivisor\nNormal\t1\t1\n')


if __name__ == '__main__':
    unittest.main()
