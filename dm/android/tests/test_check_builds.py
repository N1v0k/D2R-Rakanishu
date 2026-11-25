#!/usr/bin/env python3








import copy
import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import check_builds as cb


def cases():
    out = []
    with open(os.path.join(HERE, 'build_plan_cases.tsv'), encoding='utf-8') as fh: lines = fh.readlines()
    for line in lines:
        if line.startswith('#') or not line.strip(): continue
        name, level, points, steps, base, reasons, expect, *table = line.rstrip('\n').split('\t')
        st = []
        if steps != '-':
            for s in steps.split(','):
                f = s.split(':')
                st.append((int(f[0]), int(f[1]), int(f[2]), len(f) > 3 and f[3] == 's'))
        kv = lambda t: {} if t == '-' else {int(a): int(b) for a, b in (x.split('=') for x in t.split(','))}
        state, idx, *at = expect.split()
        out.append((name, int(level), int(points), st, kv(base), kv(reasons), (state, int(idx), int(at[0]) if at else 0), skill_table(table[0]) if table else None))
    return out


def skill_table(t):

    out = {}
    for x in t.split(','):
        f = x.split(':')
        out[int(f[0])] = {'req': int(f[1]), 'max': int(f[2]), 'pre': [int(p) for p in f[3].split('/')] if len(f) > 3 else []}
    return out


def guide():

    t = lambda s: {'de': s, 'en': s}
    return {
        'schema': 1, 'id': 'test-guide', 'version': 1, 'class': 'war', 'classIndex': 7, 'title': t('Test'), 'summary': t('Summary'),
        'overview': {'strengths': {'de': ['a'], 'en': ['a']}, 'weaknesses': {'de': ['b'], 'en': ['b']}}, 'tags': ['leveling'], 'difficulty': 1,
        'mode': {'core': 'softcore', 'ladder': 'nonladder'}, 'gameVersion': '3.3.94012', 'patch': '3.3',
        'keySkills': [10], 'trees': [3], 'profile': {'damage': ['fire']},
        'phases': [
            {'id': 'lvl', 'name': t('Leveln'), 'levels': [1, 9], 'plan': 'cumulative',
             'skills': [{'lvl': 1, 'skill': 10, 'key': 'Ten', 'to': 3}, {'lvl': 6, 'skill': 11, 'key': 'Eleven', 'to': 1, 'save': True},
                        {'lvl': 6, 'skill': 10, 'key': 'Ten', 'to': 6}],
             'stats': [{'stat': 'str', 'to': 30}, {'stat': 'vit', 'rest': True}]},
            {'id': 'end', 'name': t('Endgame'), 'levels': [10, 20], 'plan': 'cumulative',
             'skills': [{'lvl': 10, 'skill': 10, 'key': 'Ten', 'to': 12}],
             'stats': [{'stat': 'vit', 'rest': True}]}],
        'gear': [{'phase': 'lvl', 'slot': 'helm', 'tier': 'budget', 'ref': {'type': 'helm', 'quality': 'magic'}}],
        'merc': {'act': 2, 'difficulty': 2, 'aura': 'Aura', 'why': t('x'), 'gear': []},
        'sources': [{'site': 'A', 'title': 'a', 'url': 'https://a.example.com/x', 'seen': '2026-11-01', 'patch': '3.3', 'kind': 'guide'},
                    {'site': 'B', 'title': 'b', 'url': 'https://b.example.org/y', 'seen': '2026-11-01', 'patch': '3.3', 'kind': 'guide'}],
        'review': {'checks': [], 'played': False}}


SKILLS = {10: {'key': 'Ten', 'class': 'war', 'req': 1, 'pre': [], 'max': 20, 'page': 3},
          11: {'key': 'Eleven', 'class': 'war', 'req': 6, 'pre': [10], 'max': 20, 'page': 3},
          12: {'key': 'Twelve', 'class': 'war', 'req': 30, 'pre': [11], 'max': 20, 'page': 3}}


class PlanRule(unittest.TestCase):
    def test_shared_cases(self):
        cs = cases()
        self.assertGreaterEqual(len(cs), 10)
        for name, level, points, steps, base, reasons, want, table in cs:
            future = (lambda lv, b, t=table: cb.table_reason(t, lv, b)) if table else None
            got = cb.next_step(steps, level, base, lambda s: reasons.get(s, 0), points, future)
            self.assertEqual(got, want, name)

    def test_table_reason(self):
        r = cb.table_reason(SKILLS, 6, {10: 1})
        self.assertEqual(r(11), cb.OK)
        self.assertEqual(cb.table_reason(SKILLS, 5, {10: 1})(11), cb.R_LEVEL)
        self.assertEqual(cb.table_reason(SKILLS, 9, {})(11), cb.R_PREREQ)
        self.assertEqual(cb.table_reason(SKILLS, 9, {10: 20})(10), cb.R_MAX)
        self.assertEqual(cb.table_reason(SKILLS, 9, {})(99), cb.R_DATA)
        self.assertEqual(cb.table_reason(SKILLS, 3, {10: 3})(10), cb.R_LEVEL)


class Simulation(unittest.TestCase):
    def test_runs_through(self):
        g = guide()
        res = cb.simulate(g['phases'], SKILLS, True)
        self.assertIsNone(res[0]['dead'])
        self.assertEqual(res[0]['done_at'], [4, 6, 8])
        self.assertEqual(res[1]['done_at'], [13])

    def test_without_quests_is_slower(self):
        g = guide()
        with_q, without = cb.simulate(g['phases'], SKILLS, True), cb.simulate(g['phases'], SKILLS, False)
        self.assertLessEqual(with_q[0]['done_at'][-1], without[0]['done_at'][-1])

    def test_dead_end(self):
        g = guide()
        g['phases'][0]['skills'].append({'lvl': 9, 'skill': 10, 'key': 'Ten', 'to': 25})
        SK = copy.deepcopy(SKILLS); SK[10]['max'] = 5
        res = cb.simulate(g['phases'], SK, True)
        self.assertIsNotNone(res[0]['dead'])

    def test_respec_starts_over(self):
        g = guide()
        g['phases'][1]['plan'] = 'respec'
        g['phases'][1]['skills'] = [{'lvl': 10, 'skill': 10, 'key': 'Ten', 'to': 9}]
        res = cb.simulate(g['phases'], SKILLS, False)
        self.assertEqual(res[1]['done_at'], [10])
        self.assertEqual(res[1]['base'], {10: 9})


class Structure(unittest.TestCase):
    def test_valid(self):
        err, _ = cb.check_ci(guide(), 'test-guide.json')
        self.assertEqual(err, [])

    def bad(self, change, want):
        g = guide(); change(g)
        err, _ = cb.check_ci(g, 'test-guide.json')
        self.assertTrue(any(want in e for e in err), (want, err))

    def test_rejects(self):
        self.bad(lambda g: g['title'].pop('en'), 'en fehlt')
        self.bad(lambda g: g.update(classIndex=3), 'classIndex')
        self.bad(lambda g: g['phases'][1].update(levels=[9, 20]), 'überlappen')
        self.bad(lambda g: g['phases'][0]['skills'].reverse(), 'Stufe')
        self.bad(lambda g: g['phases'][0]['stats'].reverse(), 'rest')
        self.bad(lambda g: g['sources'][1].update(url='http://b.example.org/y'), 'Form')
        self.bad(lambda g: g['review'].update(checks=['two-sources']) or g['sources'][1].update(url='https://www.example.com/z'), 'two-sources')
        self.bad(lambda g: g['gear'].append({'phase': 'xx', 'slot': 'helm', 'tier': 'bis', 'ref': {'unique': 'X'}}), 'Phase xx')
        self.bad(lambda g: g['gear'].append({'phase': 'lvl', 'slot': 'helm', 'tier': 'bis', 'ref': {'runeword': 'Runeword1'}}), 'base.sockets')
        self.bad(lambda g: g.update(unknownField=1), 'unbekanntes Feld')
        self.bad(lambda g: g.update(keySkills=[99]), 'keySkills')
        self.bad(lambda g: g['phases'][0]['skills'][0].update(to=21), 'außerhalb')

    def test_private_metadata_is_rejected(self):
        self.bad(lambda g: g.update(updated='2026-01-01'), 'unknown field')
        self.bad(lambda g: g['review'].update(by='reviewer'), 'unknown field')
        self.bad(lambda g: g['review'].update(date='2026-01-01'), 'unknown field')

    def test_repo_guides(self):
        files = [f for f in os.listdir(cb.BUILDS) if f.endswith('.json') and f != 'schema.json']
        for f in files:
            with open(os.path.join(cb.BUILDS, f), encoding='utf-8') as fh: g = json.load(fh)
            err, _ = cb.check_ci(g, f)
            self.assertEqual(err, [], f)

    def test_schema_is_json(self):
        with open(cb.SCHEMA, encoding='utf-8') as fh: s = json.load(fh)
        self.assertEqual(s['properties']['gameVersion']['const'], cb.VERSION)


def preview_names():

    labels = {k: ['%s %%d' % k, '%s %%d' % k] for k in cb.PREVIEW_LABELS}
    items = {'t:helm': {'name': ['Helm', 'Helm']}, 'u:Crest': {'name': ['K', 'C'], 'code': 'uap', 'row': 3}, 'i:uap': {'name': ['T', 'S']},
             's:Claws': {'name': ['K', 'C'], 'code': 'xmg', 'row': 0}, 'i:xmg': {'name': ['A', 'B']},
             'r:Runeword1': {'name': ['W', 'W'], 'runes': ['r01', 'r02'], 'row': 0}, 'i:r01': {'name': ['El', 'El'], 'short': ['El', 'El']},
             'i:r02': {'name': ['Eld', 'Eld'], 'short': ['Eld', 'Eld']}, 't:shie': {'name': ['S', 'S']}}
    return {'items': items, 'qualities': {'magic': ['Magisch', 'Magic']}, 'labels': labels}


class Preview(unittest.TestCase):

    def gear(self, *refs):
        g = guide()
        g['gear'] = [dict(phase='lvl', slot='helm', tier='budget', **r) for r in refs]
        return g

    def test_complete(self):
        g = self.gear({'ref': {'type': 'helm', 'quality': 'magic'}}, {'ref': {'unique': 'Crest'}}, {'ref': {'set': 'Claws'}},
                      {'ref': {'runeword': 'Runeword1'}, 'base': {'type': 'shie', 'sockets': 2}}, {'ref': {'item': 'uap'}})
        self.assertEqual(cb.preview_errors(g, preview_names()), [])

    def test_row_zero_is_a_row(self):
        g = self.gear({'ref': {'set': 'Claws'}})
        self.assertEqual(cb.preview_errors(g, preview_names()), [])

    def test_missing(self):
        n = preview_names()
        del n['items']['u:Crest']['row']; n['items']['s:Claws']['row'] = True; del n['items']['i:r02']['short']; del n['labels']['sock']
        g = self.gear({'ref': {'unique': 'Crest'}}, {'ref': {'set': 'Claws'}}, {'ref': {'runeword': 'Runeword1'}, 'base': {'item': 'zzz', 'sockets': 2}},
                      {'ref': {'type': 'boot', 'quality': 'rare'}}, {'ref': {'item': 'nope'}})
        err = '\n'.join(cb.preview_errors(g, n))
        for want in ('gear[0]', 'gear[1]', 'r02', 'Basis ohne Vorschau', 'Typ ohne Vorschau', 'Qualität ohne Namen', 'Gegenstand ohne Vorschau', 'sock'):
            self.assertIn(want, err)

    def test_merc_gear_counts(self):
        g = guide()
        g['merc']['gear'] = [{'slot': 'helm', 'tier': 'bis', 'ref': {'unique': 'Gone'}}]
        self.assertTrue(any('gear[1]' in e for e in cb.preview_errors(g, preview_names())))


class GearRows(unittest.TestCase):

    class T:
        uniques = {'Old Charm': {'spawnable': '', 'code': 'cm3', 'lvl req': '75'}, 'PreCrafted Old Charm': {'spawnable': '1', 'code': 'cm3', 'lvl req': '75'},
                   'Crafted Old Charm': {'spawnable': '', 'code': 'cs2', 'lvl req': '75'}}
        sets, items = {}, {'cm3': {'levelreq': ''}, 'cs2': {'levelreq': ''}}
        item_types = staticmethod(lambda code: {'char'})

    def facts(self, name):
        err = []
        cb.gear_facts({'slot': 'charm', 'ref': {'unique': name}}, self.T, err, 'gear[0]')
        return err

    def test_old_sunder_row(self):
        self.assertTrue(any('PreCrafted Old Charm' in e for e in self.facts('Old Charm')))

    def test_latent_and_renewed_rows(self):
        self.assertEqual(self.facts('PreCrafted Old Charm'), [])
        self.assertEqual(self.facts('Crafted Old Charm'), [])


class PreviewRows(unittest.TestCase):


    @classmethod
    def setUpClass(cls):
        import gen_build_names
        cls.g = gen_build_names

    def rec(self, size, records):
        import struct
        return struct.pack('<I', len(records)) + b''.join(r.ljust(size, b'\0') for r in records)

    def test_bin_rows(self):
        raw = self.rec(0x30, [b'a', b'b'])
        get = self.g.bin_rows(raw, 0x30, 2, 'x.bin')
        self.assertEqual(get(1)[:1], b'b')
        with self.assertRaises(self.g.Refuse): self.g.bin_rows(raw, 0x30, 3, 'x.bin')
        with self.assertRaises(self.g.Refuse): self.g.bin_rows(raw[:-1], 0x30, 2, 'x.bin')
        with self.assertRaises(self.g.Refuse): self.g.bin_rows(raw, 0x20, 2, 'x.bin')
        with self.assertRaises(self.g.Refuse): self.g.bin_rows(b'', 0x30, 0, 'x.bin')

    def test_item_row(self):
        rec = self.g.bin_rows(self.rec(0x30, [b'\0' * 0x28 + b'uap ', b'\0' * 0x28 + b'xmg ']), 0x30, 2, 'x.bin')
        self.assertEqual(self.g.item_row(rec, '1', 'xmg', 'x.bin'), 1)
        with self.assertRaises(self.g.Refuse): self.g.item_row(rec, '1', 'uap', 'x.bin')
        with self.assertRaises(self.g.Refuse): self.g.item_row(rec, '', 'uap', 'x.bin')
        with self.assertRaises(self.g.Refuse): self.g.item_row(rec, 'x1', 'uap', 'x.bin')

    def test_word_row(self):
        import struct
        b = bytearray(0xe8); b[0x44] = 1
        for k, v in enumerate((5, 6, -1, -1, -1, -1)): struct.pack_into('<i', b, 0x5c + 4 * k, v)
        rec = self.g.bin_rows(self.rec(0xe8, [bytes(b)]), 0xe8, 1, 'runes.bin')
        self.assertEqual(self.g.word_row(rec, 0, {'complete': '1', 'Rune1': 'r01', 'Rune2': 'r02'}), 0)
        with self.assertRaises(self.g.Refuse): self.g.word_row(rec, 0, {'complete': '0', 'Rune1': 'r01', 'Rune2': 'r02'})
        with self.assertRaises(self.g.Refuse): self.g.word_row(rec, 0, {'complete': '1', 'Rune1': 'r01'})

    def test_num_pair(self):
        self.assertEqual(self.g.num_pair({'minac': '3', 'maxac': '5'}, 'minac', 'maxac'), [3, 5])
        self.assertIsNone(self.g.num_pair({'minac': '', 'maxac': '5'}, 'minac', 'maxac'))
        self.assertIsNone(self.g.num_pair({'minac': '0', 'maxac': '0'}, 'minac', 'maxac'))


@unittest.skipUnless(os.environ.get('DM_BUILDS_TABLES'), 'DM_BUILDS_TABLES nicht gesetzt | not set')
class WithTables(unittest.TestCase):
    def test_repo_guides_against_tables(self):
        T = cb.Tables(cb.dir_reader(os.environ['DM_BUILDS_TABLES']))
        self.assertEqual(T.sha, cb.TABLES_SHA)
        for f in [f for f in os.listdir(cb.BUILDS) if f.endswith('.json') and f != 'schema.json']:
            with open(os.path.join(cb.BUILDS, f), encoding='utf-8') as fh: g = json.load(fh)
            err, _, _ = cb.check_tables(g, T)
            self.assertEqual(err, [], f)


if __name__ == '__main__':
    unittest.main(verbosity=1)
