#!/usr/bin/env python3

import hashlib
import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_itemcmp_data as g


def tsv(head, rows):
    return ('\t'.join(head) + '\n' + ''.join('\t'.join(r.get(h, '') for h in head) + '\n' for r in rows)).encode()


ISC_HEAD = ['Stat', '*ID', 'op', 'op param', 'op base', 'op stat1', 'op stat2', 'op stat3', 'ValShift']
ISC = [dict(Stat='strength', **{'*ID': '0'}), dict(Stat='energy', **{'*ID': '1', 'op': '8', 'op stat1': 'maxmana'}),
       dict(Stat='vitality', **{'*ID': '3', 'op': '9', 'op stat1': 'maxhp'}), dict(Stat='maxhp', **{'*ID': '7', 'ValShift': '8'}),
       dict(Stat='maxmana', **{'*ID': '9', 'ValShift': '8'}), dict(Stat='item_maxdamage_percent', **{'*ID': '17', 'op': '13', 'op stat1': 'maxdamage'}),
       dict(Stat='item_mindamage_percent', **{'*ID': '18', 'op': '13', 'op stat1': 'mindamage', 'op stat2': 'secondary_mindamage'}),
       dict(Stat='mindamage', **{'*ID': '21'}), dict(Stat='maxdamage', **{'*ID': '22'}), dict(Stat='secondary_mindamage', **{'*ID': '23'}),
       dict(Stat='fireresist', **{'*ID': '39'}), dict(Stat='lightresist', **{'*ID': '41'}), dict(Stat='firemindam', **{'*ID': '48'}),
       dict(Stat='firemaxdam', **{'*ID': '49'}), dict(Stat='item_addclassskills', **{'*ID': '83'}), dict(Stat='item_aura', **{'*ID': '151'}),
       dict(Stat='item_addskill_tab', **{'*ID': '188'}),
       dict(Stat='item_armor_perlevel', **{'*ID': '214', 'op': '4', 'op param': '3', 'op base': 'level', 'op stat1': 'armorclass'}),
       dict(Stat='armorclass', **{'*ID': '31'})]
PROP_HEAD = ['code'] + ['%s%d' % (c, k) for k in range(1, 8) for c in ('func', 'stat', 'set', 'val')]
PROPS = [dict(code='str', func1='1', stat1='strength'), dict(code='hp', func1='1', stat1='maxhp'),
         dict(code='res2', func1='1', stat1='fireresist', func2='3', stat2='lightresist'),
         dict(code='fire', func1='15', stat1='firemindam', func2='16', stat2='firemaxdam'), dict(code='ac/lvl', func1='17', stat1='item_armor_perlevel'),
         dict(code='cls', func1='21', stat1='item_addclassskills', val1='7'), dict(code='tab', func1='10', stat1='item_addskill_tab'),
         dict(code='aura', func1='22', stat1='item_aura'), dict(code='dmg-min', func1='5')]
SETS_HEAD = ['index'] + ['%s%d%s' % (c, n, s) for n in range(2, 6) for s in 'ab' for c in ('PCode', 'PParam', 'PMin', 'PMax')] + \
            ['%s%d' % (c, n) for n in range(1, 9) for c in ('FCode', 'FParam', 'FMin', 'FMax')]
SETS = [dict(index='A', PCode2a='str', PMin2a='5', PMax2a='5', FCode1='fire', FMin1='3', FMax1='9', FCode2='aura', FParam2='99', FMin2='4', FMax2='4'),
        dict(index='Expansion'), dict(index='B', FCode1='dmg-min', FMin1='7', FMax1='7', FCode2='str', FParam2='2', FMin2='1', FMax2='1')]
ITEMS_HEAD = ['index', 'set', 'add func'] + ['%s%d%s' % (c, n, s) for n in range(1, 6) for s in 'ab' for c in ('aprop', 'apar', 'amin', 'amax')]
ITEMS = [dict(index='a1', set='A', **{'add func': '2', 'aprop1a': 'hp', 'amin1a': '20', 'amax1a': '20', 'aprop2a': 'res2', 'amin2a': '10', 'amax2a': '10'}),
         dict(index='Expansion'), dict(index='a2', set='A', **{'add func': '1', 'aprop1a': 'hp', 'amin1a': '20', 'amax1a': '22'}),
         dict(index='b1', set='B', **{'add func': '2', 'aprop1a': 'ac/lvl', 'apar1a': '8', 'aprop1b': 'cls', 'amin1b': '1', 'amax1b': '1', 'aprop2a': 'tab', 'apar2a': '57', 'amin2a': '2', 'amax2a': '2'})]
TABLES = {'global/excel/itemstatcost.txt': tsv(ISC_HEAD, ISC), 'global/excel/properties.txt': tsv(PROP_HEAD, PROPS),
          'global/excel/sets.txt': tsv(SETS_HEAD, SETS), 'global/excel/setitems.txt': tsv(ITEMS_HEAD, ITEMS)}


def fingerprint(tables):
    return hashlib.sha256(b''.join(hashlib.sha256(tables[p]).digest() for p in g.GEAR_TABLES)).hexdigest()


class GearTests(unittest.TestCase):
    def build(self, tables=TABLES):
        with mock.patch.object(g, 'GEAR_TABLES_SHA', fingerprint(tables)):
            return g.build_gear(lambda p: tables[p])

    def test_rows(self):
        text, nset, nitem, nb = self.build()
        lines = text.splitlines()
        self.assertEqual((nset, nitem), (2, 3))
        self.assertTrue(lines[0].startswith('DM_GEAR\t1\t'))
        self.assertEqual(lines[-1], 'end\t%d' % len(lines))
        rows = set(lines[1:-1])
        expect = {
            'op\t1\t8\t0\t0\t9', 'op\t3\t9\t0\t0\t7', 'op\t214\t4\t3\t1\t31', 'op\t17\t13\t0\t0\t22', 'op\t18\t13\t0\t0\t21,23', 'shift\t7\t8', 'shift\t9\t8',
            'set\t0\t2', 'set\t1\t1',
            'setb\t0\t2\t0\t0\t5', 'setb\t0\t0\t48\t0\t3', 'setb\t0\t0\t49\t0\t9', 'setx\t0\t0\t151',
            'setb\t1\t0\t21\t0\t7', 'setb\t1\t0\t23\t0\t7', 'setx\t1\t0\t0',
            'item\t0\t0\t2', 'item\t1\t0\t1', 'item\t2\t1\t2',
            'itemb\t0\t1\t7\t0\t5120', 'itemb\t0\t2\t39\t0\t10', 'itemb\t0\t2\t41\t0\t10',
            'itemx\t1\t1\t7',
            'itemb\t2\t1\t214\t0\t8', 'itemb\t2\t1\t83\t7\t1', 'itemb\t2\t2\t188\t57\t2',
        }
        self.assertEqual(expect - rows, set())
        self.assertEqual(rows - expect, set())

    def test_fingerprint(self):
        with self.assertRaises(g.Refuse): g.build_gear(lambda p: TABLES[p])

    def test_unknown_set(self):
        broken = dict(TABLES, **{'global/excel/setitems.txt': tsv(ITEMS_HEAD, [dict(index='x', set='C', **{'add func': '2'})])})
        with self.assertRaises(g.Refuse): self.build(broken)

    def test_missing_column(self):
        broken = dict(TABLES, **{'global/excel/sets.txt': tsv(['index', 'PCode2a'], [dict(index='A')])})
        with self.assertRaises(g.Refuse): self.build(broken)


DESC_HEAD = ['Stat', '*ID', 'op', 'op stat1', 'op stat2', 'op stat3'] + list(g.DESC_COLS)
DESC_ISC = [dict(Stat=n, **{'*ID': str(i)}) for i, n in enumerate(g.METRIC_STATS, 1000)]
DESC_ISC[g.METRIC_STATS.index('fireresist')].update(descfunc='19', descstrpos='ModStr1j', descstrneg='ModStr1j', dgrp='2', dgrpfunc='19',
                                                    dgrpstrpos='strModAllResistances', dgrpstrneg='strModAllResistances')
DESC_ISC[g.METRIC_STATS.index('maxhp')].update(descfunc='19', descstrpos='ModStr1u', descstrneg='ModStr1u', ValShift='8')
DESC_TABLES = {'global/excel/itemstatcost.txt': tsv(DESC_HEAD, DESC_ISC),
               'global/excel/charstats.txt': tsv(['class', 'LifePerVitality', 'ManaPerMagic', 'StaminaPerVitality'],
                                                 [{'class': 'Amazon', 'LifePerVitality': '12', 'ManaPerMagic': '6', 'StaminaPerVitality': '4'}]),
               'global/excel/difficultylevels.txt': tsv(['Name', 'ResistPenalty', 'ResistPenaltyNonExpansion'],
                                                        [{'Name': n, 'ResistPenalty': p, 'ResistPenaltyNonExpansion': p} for n, p in (('N', '0'), ('M', '-40'), ('H', '-100'))]),
               'global/excel/skills.txt': tsv(['*Id', 'charclass', 'skilldesc'] + ['passivestat%d' % k for k in range(1, 15)], [{'*Id': '0', 'skilldesc': 'attack'}]),
               'global/excel/skilldesc.txt': tsv(['skilldesc', 'SkillPage', 'str name'], [{'skilldesc': 'attack', 'SkillPage': '0', 'str name': 'skillname0'}]),
               'global/excel/playerclass.txt': tsv(['Player Class', 'Code'], [{'Player Class': 'Amazon', 'Code': 'ama'}])}


class DescTests(unittest.TestCase):
    def build(self, tables=DESC_TABLES):
        sha = hashlib.sha256(b''.join(hashlib.sha256(tables[p]).digest() for p in g.TABLES)).hexdigest()
        with mock.patch.object(g, 'TABLES_SHA', sha):
            return g.build(lambda p: tables[p])[0].splitlines()

    def test_desc_rows(self):
        rows = [r for r in self.build() if r.startswith('desc\t')]
        self.assertEqual(len(rows), len(g.DESC_STATS))
        fire, hp = 1000 + g.METRIC_STATS.index('fireresist'), 1000 + g.METRIC_STATS.index('maxhp')
        self.assertIn('desc\t%d\t19\tModStr1j\tModStr1j\t2\t19\tstrModAllResistances\tstrModAllResistances\t-' % fire, rows)
        self.assertIn('desc\t%d\t19\tModStr1u\tModStr1u\t-\t-\t-\t-\t8' % hp, rows)

    def test_desc_key_with_space(self):
        bad = [dict(r) for r in DESC_ISC]; bad[0]['descstrpos'] = 'Mod Str'
        with self.assertRaises(g.Refuse): self.build(dict(DESC_TABLES, **{'global/excel/itemstatcost.txt': tsv(DESC_HEAD, bad)}))


if __name__ == '__main__':
    unittest.main()
