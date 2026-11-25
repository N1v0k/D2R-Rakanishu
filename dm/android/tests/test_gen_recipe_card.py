#!/usr/bin/env python3




import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import gen_recipe_data as g


class FakeCatalog:
    sockets = ('Sockelfassungen', 'Sockets')
    cat_names = dict(g.CAT_NAMES)
    used_items = {'r08': {'short': ('Ral',)}}

    def describe(self, raw):
        assert raw == 'r08'
        return {'id': 'r08', 'n': 1, 'k': 'i', 'img': 'r08', 'de': 'Ral-Rune', 'en': 'Ral Rune'}

    def cond(self, f):
        return g.OWN_CONDS[f] if f in g.OWN_CONDS else ('Gesockelt', 'Socketed')


CLEAR = {'row': 141, 'cat': 'improve', 'fam': 'unsocket', 'quest': 0, 'out': '', 'ord': 141, 'de': 'Sockelfassungen leeren', 'en': 'Empty sockets',
         'colour': 'white', 'img': 'amu', 'num': 3, 's': 'x', 'pv': [['a', 'b']],
         'in': [{'id': 'any', 'n': 1, 'k': 't', 'img': 'amu', 'de': 'Gegenstände', 'en': 'Items', 'f': ['sock'], 'cde': 'Gesockelt', 'cen': 'Socketed'},
                {'id': 'r15', 'n': 1, 'k': 'i', 'img': 'r15', 'de': 'Hel-Rune', 'en': 'Hel Rune'},
                {'id': 'tsc', 'n': 1, 'k': 'i', 'img': 'tsc', 'de': 'Rolle', 'en': 'Scroll'}]}


class KeepContents(unittest.TestCase):
    def test_card(self):
        e = g.keep_contents(FakeCatalog(), CLEAR)
        self.assertEqual(e['app'], 'unsocket')
        self.assertEqual((e['de'], e['en']), ('Sockelfassungen leeren, Einsätze behalten', 'Empty sockets, keep contents'))
        self.assertEqual([x['id'] for x in e['in']], ['any', 'r08', 'tsc'])
        self.assertEqual(e['in'][0]['f'], ['fil'])
        self.assertEqual((e['in'][0]['cde'], e['in'][0]['cen']), g.OWN_CONDS['fil'])
        self.assertEqual(e['pv'], [list(x) for x in g.KEEP_TEXT])
        self.assertIn('ral', e['s'])
        self.assertIn('behalten', e['s'])
        self.assertEqual((e['row'], e['cat'], e['num']), (141, 'improve', 3))
        self.assertEqual(CLEAR['in'][1]['id'], 'r15', 'the game card itself stays as it was')
        self.assertEqual(CLEAR['in'][0]['f'], ['sock'])

    def test_other_shape_refused(self):
        odd = dict(CLEAR, **{'in': CLEAR['in'][:2]})
        with self.assertRaises(g.Refuse):
            g.keep_contents(FakeCatalog(), odd)


if __name__ == '__main__':
    unittest.main()
