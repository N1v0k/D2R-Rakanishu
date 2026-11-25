#!/usr/bin/env python3
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from unittest import mock
import gen_sim_data as g


class MiniDb:
    def ident(self, owner, name):
        if name == 'lvl': return ['lvl']
        if name == 'par1': return [7]
        raise g.CalcError(name)

    def ref(self, owner, kind, ref, member):
        if kind == 'skill' and ref == 'Spark' and member == 'blvl': return ['@3:blvl']
        if kind == 'stat' and ref == 'strength' and member == 'accr': return ['#0']
        raise g.CalcError(ref)


class CompilerTests(unittest.TestCase):
    def setUp(self):
        self.c = g.Compiler(MiniDb())

    def test_precedence(self):
        self.assertEqual(self.c.compile(None, '2 + 3 * 4'), '2 3 4 * +')
        self.assertEqual(self.c.compile(None, '(2 + 3) * 4'), '2 3 + 4 *')

    def test_ternary(self):
        self.assertEqual(self.c.compile(None, 'lvl > 2 ? 7 : 5'), 'lvl 2 > 7 5 ?:')
        self.assertEqual(self.c.compile(None, '1 ? 2 ? 3 : 4 : 5'), '1 2 3 4 ?: 5 ?:')

    def test_references(self):
        self.assertEqual(self.c.compile(None, "skill('Spark'.blvl)*par1+stat('strength'.accr)"), '@3:blvl 7 * #0 +')

    def test_functions(self):
        self.assertEqual(self.c.compile(None, 'min(3,max(4,5))'), '3 4 5 max min')
        self.assertEqual(self.c.compile(None, '-3 % 2'), '3 neg 2 %')

    def test_unknown(self):
        for expr in ('foo(1)', 'unknown', '1 +', '1 @ 2', '1 2'):
            with self.subTest(expr=expr), self.assertRaises(g.CalcError): self.c.compile(None, expr)

    def test_table_quoting(self):
        self.assertEqual(self.c.compile(None, '"min(1,2)"'), '1 2 min')


class DataTests(unittest.TestCase):
    def test_parser(self):
        head, rows = g.table('b\ta\r\n2\t1\r\n\t\r\n')
        self.assertEqual(head, ['b', 'a'])
        self.assertEqual(rows, [{'a': '1', 'b': '2'}])
        with self.assertRaises(g.Refuse): g.need(head, ('missing',), 'mini')

    def test_families(self):
        self.assertEqual(len(g.FAMILY), 241)
        self.assertEqual(g.fam_parts('W3+DoT'), ['W3', 'DoT'])
        self.assertEqual(g.fam_parts('A+'), ['A+'])
        for value in ('W1+', 'W6', 'A', ''): self.assertIsNone(g.fam_parts(value))
        self.assertEqual(set(p for f in g.FAMILY.values() for p in g.fam_parts(f)), g.FAMILIES)

    def test_missile_closure_cycles(self):
        class Mini:
            desc = {}
            miss_by_name = {'a': {'SubMissile1': 'b'}, 'b': {'HitSubMissile1': 'c'}, 'c': {'ExplosionMissile': 'a'}}
        self.assertEqual(set(g.reach_missiles(Mini(), {'srvmissile': 'a'})), {'a', 'b', 'c'})

    def test_area_reasons(self):
        class Mini:
            desc = {}
            miss_by_name = {'blast': {'Missile': 'blast', 'SubMissile1': 'spark', '*server hit param1 desc': 'the explosion radius of this blast'},
                            'spark': {'Missile': 'spark', '*client hit param1 desc': 'explosion radius, drawn only'},
                            'cloud': {'Missile': 'cloud', 'Radius': '4'}}
        def row(**k): return dict({'summon': '', 'aura': '', 'aurarangecalc': '', 'srvdofunc': ''}, **k)
        self.assertEqual(g.area_reasons(Mini(), row(srvmissile='spark')), [])
        self.assertEqual(g.area_reasons(Mini(), row(srvmissile='blast')), ['radius'])
        self.assertEqual(g.area_reasons(Mini(), row(srvmissile='cloud')), ['radius'])
        self.assertEqual(g.area_reasons(Mini(), row(aura='1', aurarangecalc='ln12')), ['aura'])
        self.assertEqual(g.area_reasons(Mini(), row(aura='1')), [])
        for func in ('22', '63', '68'):
            self.assertEqual(g.area_reasons(Mini(), row(srvdofunc=func)), ['ring'], func)
        for func in ('20', '55', '66', '160', '164'):
            self.assertEqual(g.area_reasons(Mini(), row(srvdofunc=func)), ['field'], func)
        self.assertEqual(g.area_reasons(Mini(), row(srvdofunc='2')), [])
        self.assertEqual(g.area_reasons(Mini(), row(calc1='par1', **{'*calc1 desc': 'count of target jump hits'})), ['chain'])
        self.assertEqual(g.area_reasons(Mini(), row(**{'*calc1 desc': 'count of target jump hits'})), [], 'a description of an empty calc does not count')
        self.assertEqual(g.area_reasons(Mini(), row(Param3='5', **{'*Param3 Description': 'number of missiles at level one'})), ['missiles'])
        self.assertEqual(g.area_reasons(Mini(), row(summon='golem', srvmissile='blast', srvdofunc='22')), [], 'a summon is single target')

    def test_anim(self):
        events = bytearray(144); events[4] = 1
        record = b'ZZSCHTH\0' + struct.pack('<II', 12, 256) + events
        data = struct.pack('<I', 1) + record + b'\0' * (255 * 4)
        self.assertEqual(g.animdata(data)['ZZSCHTH'], (12, 256, 4))
        for invalid in (data[:-1], data + b'x', b''):
            with self.assertRaises(g.Refuse): g.animdata(invalid)

    def test_scaling(self):
        row = {'noRatio': '0', 'hp': '150'}
        levels = {1: {'L-HP': '103'}}
        self.assertEqual(g.scaled(row, 0, 1, levels, 'hp', 'L-HP'), 154)
        row['noRatio'] = '1'
        self.assertEqual(g.scaled(row, 0, 1, levels, 'hp', 'L-HP'), 150)

    def test_anim_action_flags(self):
        for marks, action in (({2: 3, 7: 2}, 7), ({1: 4, 6: 1}, 6),
                              ({3: 2, 8: 1}, 3), ({2: 3, 5: 4}, -1)):
            events = bytearray(144)
            for frame, flag in marks.items(): events[frame] = flag
            record = b'ZZSCHTH\0' + struct.pack('<II', 12, 256) + events
            data = struct.pack('<I', 1) + record + b'\0' * (255 * 4)
            self.assertEqual(g.animdata(data)['ZZSCHTH'], (12, 256, action))

    def test_monster_separator_does_not_consume_runtime_index(self):
        rows = [{'Id': 'first', '*hcIdx': '0'}, {'Id': 'Expansion', '*hcIdx': ''},
                {'Id': 'second', '*hcIdx': '1'}]
        self.assertEqual(g.monster_index({'monstats': (['Id', '*hcIdx'], rows)}), {'first': 0, 'second': 1})

    def test_runtime_index_rejects_misaligned_or_duplicate_rows(self):
        for rows in ([{'Id': 'first', '*hcIdx': '1'}],
                     [{'Id': 'first', '*hcIdx': '0'}, {'Id': 'first', '*hcIdx': '1'}],
                     [{'Id': '', '*hcIdx': '0'}]):
            with self.assertRaises(g.Refuse):
                g.monster_index({'monstats': (['Id', '*hcIdx'], rows)})

    def test_unique_separator_uses_the_same_runtime_index_rule(self):
        rows = [{'Superunique': 'first', 'hcIdx': '0'}, {'Superunique': 'Expansion', 'hcIdx': ''},
                {'Superunique': 'second', 'hcIdx': '1'}]
        self.assertEqual(g.runtime_indices((['Superunique', 'hcIdx'], rows), 'Superunique', 'hcIdx'),
                         {'first': 0, 'second': 1})

    def test_hand_types(self):
        head = ['ItemType', 'Code', 'Equiv1', 'Equiv2']
        rows = [dict(zip(head, r)) for r in (('', 'weap', '', ''), ('', 'mele', 'weap', ''), ('', 'h2h', 'mele', 'clas'), ('', 'h2h2', 'h2h', ''),
                                              ('', 'thro', 'weap', ''), ('', 'comb', 'mele', 'thro'), ('', 'tkni', 'comb', 'mele'), ('', 'clas', '', ''),
                                              ('', 'armo', '', ''), ('', 'shld', 'armo', ''), ('', 'shie', 'shld', ''), ('', 'misc', '', ''))]
        types = g.hand_types((head, rows))
        codes = [code for code, _ in types]
        self.assertEqual(set(codes), {'weap', 'mele', 'h2h', 'h2h2', 'thro', 'comb', 'tkni', 'shld', 'shie'})
        supers = dict(types)
        self.assertEqual(supers['h2h2'], ['h2h', 'mele', 'weap'])
        self.assertEqual(supers['tkni'], ['comb', 'mele', 'thro', 'weap'])
        self.assertEqual(supers['shie'], ['shld'])
        for code, sup in types:
            self.assertTrue(all(codes.index(s) < codes.index(code) for s in sup), code)
        with self.assertRaises(g.Refuse): g.hand_types((head, rows + [dict(zip(head, ('', 'axe', 'mele', 'nope')))]))
        with self.assertRaises(g.Refuse): g.hand_types((head, [dict(zip(head, ('', 'a', 'b', ''))), dict(zip(head, ('', 'b', 'a', '')))]))
        with self.assertRaises(g.Refuse): g.hand_types((head[:3], rows))
        known = set(codes)
        self.assertEqual(g.hand_requirement({'skill': 'x', 'itypea1': 'h2h', 'itypea2': 'mele', 'itypea3': ''}, ('itypea1', 'itypea2', 'itypea3'), known), 'h2h,mele')
        self.assertEqual(g.hand_requirement({'skill': 'x'}, ('itypeb1', 'itypeb2', 'itypeb3'), known), '-')
        with self.assertRaises(g.Refuse): g.hand_requirement({'skill': 'x', 'etypea1': 'misc'}, ('etypea1', 'etypea2'), known)

    def test_fingerprint_rejects(self):
        with self.assertRaises(g.Refuse): g.build(lambda _: b'wrong')

    def test_stale_file_removed(self):
        with tempfile.TemporaryDirectory() as path:
            out = pathlib.Path(path) / 'sim.tsv'; out.write_text('old')
            with mock.patch('romfs_art.read', side_effect=OSError('missing')):
                self.assertFalse(g.write('/missing', path))
            self.assertFalse(out.exists())

    def test_level_curve_order(self):
        root = os.environ.get('DM_SIM_TABLES')
        if not root: self.skipTest('DM_SIM_TABLES not set')
        read = g.read_dir(root)
        tables = {n: g.table(read('global/excel/' + n + '.txt').decode()) for n in g.NAMES}
        stats = {r['Stat']: int(r['*ID']) for r in tables['itemstatcost'][1] if r.get('*ID', '').isdigit()}
        db = g.Db('rotw', tables, {}, stats)
        row = next(r for r in db.skills if r.get('*Id') == '122')
        self.assertEqual(db.comp.compile(('skill', row), 'dm34'), '10 40 lvl DM')
        self.assertIn('claygolem', g.monster_index(tables))

    def test_darwin_stat_visibility(self):
        with tempfile.TemporaryDirectory() as path:
            root = pathlib.Path(path)
            (root / 'sys').mkdir()
            (root / 'sys/stat.h').write_text('#include <sys/types.h>\n#include <time.h>\nstruct stat {\n    off_t st_size;\n    ino_t st_ino;\n    time_t st_mtime;\n#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)\n    struct timespec st_mtimespec;\n#else\n    long st_mtimensec;\n#endif\n};\nint stat(const char *, struct stat *);\n')
            source = pathlib.Path(__file__).resolve().parents[2] / 'host/dm_sim.c'
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-D__APPLE__', '-I' + path, '-fsyntax-only', str(source)], check=True)

    def potion_rows(self):
        rows = []
        for code, kind in g.POTIONS:
            if kind == 'hpot': rows.append({'code': code, 'type': kind, 'stat1': 'hpregen', 'calc1': '30', 'stat2': '', 'calc2': '', 'len': '192'})
            elif kind == 'mpot': rows.append({'code': code, 'type': kind, 'stat1': 'manarecovery', 'calc1': '20', 'stat2': '', 'calc2': '', 'len': '128'})
            else: rows.append({'code': code, 'type': kind, 'stat1': 'hitpoints', 'calc1': '35', 'stat2': 'mana', 'calc2': '35', 'len': ''})
        return ['code', 'type', 'stat1', 'calc1', 'stat2', 'calc2', 'len'], rows

    def test_potions(self):
        head, rows = self.potion_rows()
        out = []; g.write_potions(out, (head, rows))
        self.assertEqual(out[0], 'cols\tpotion\tcode\tkind\tlife\tmana\tlen')
        self.assertEqual(out[1:3], ['potion\thp1\t0\t30\t0\t192', 'potion\thp2\t0\t30\t0\t192'])
        self.assertIn('potion\tmp5\t1\t0\t20\t128', out); self.assertEqual(out[-1], 'potion\trvl\t2\t35\t35\t0')
        for change in ({'code': 'hp9'}, {'type': 'mpot'}, {'stat1': 'hitpoints'}, {'len': '0'}, {'calc1': '-5'}, {'stat2': 'mana'}):
            broken = [dict(r) for r in rows]; broken[0].update(change)
            with self.subTest(change=change), self.assertRaises(g.Refuse): g.write_potions([], (head, broken))
        broken = [dict(r) for r in rows]; broken[-1]['len'] = '100'
        with self.assertRaises(g.Refuse): g.write_potions([], (head, broken))
        with self.assertRaises(g.Refuse): g.write_potions([], (head[:-1], rows))

    def test_real_potions(self):
        romfs = os.environ.get('DM_ROMFS_SRC')
        if not romfs: self.skipTest('DM_ROMFS_SRC not set')
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__))); import romfs_art
        text, _ = g.build(lambda path: romfs_art.read(romfs, path))
        sets, current = {}, None
        for line in text.splitlines():
            f = line.split('\t')
            if f[0] == 'set': current = f[1]
            elif f[0] == 'potion': sets.setdefault(current, {})[f[1]] = f[2:]
            elif f[0] == 'class' and f[2] == 'war': sets.setdefault(current + ':war', f[-2:])
        for mode in ('rotw', 'lod'):
            self.assertEqual(sorted(sets[mode]), sorted(code for code, _ in g.POTIONS))
            self.assertEqual(sets[mode]['hp1'], ['0', '30', '0', '192']); self.assertEqual(sets[mode]['mp2'], ['1', '0', '40', '128'])
            self.assertEqual(sets[mode]['rvs'], ['2', '35', '35', '0']); self.assertEqual(sets[mode]['rvl'], ['2', '100', '100', '0'])
        self.assertEqual(sets['rotw:war'], ['150', '200'])

    def test_real_export(self):
        root = os.environ.get('DM_SIM_TABLES')
        if not root: self.skipTest('DM_SIM_TABLES not set')
        read = g.read_dir(root); text, counts = g.build(read)
        self.assertEqual(counts['rotw'][0], 240); self.assertEqual(counts['lod'][0], 210)
        lines = text.splitlines(True)
        self.assertEqual(int(lines[-1].split('\t')[1]), len(lines))
        self.assertEqual(int(lines[0].split('\t')[3], 16), zlib.crc32(''.join(lines[1:-1]).encode()))
        standard = [line.split('\t') for line in lines if line.startswith('std\t2\t0\t')]
        self.assertTrue(all(row[4:6] == ['81', '5357'] for row in standard))
        headers, records = {}, {}
        for line in text.splitlines():
            fields = line.split('\t')
            if fields[0] == 'set': headers = {}; current_set = fields[1]
            elif fields[0] == 'cols': headers[fields[1]] = fields[2:]
            elif fields[0] in headers:
                row = dict(zip(headers[fields[0]], fields[1:])); row['set'] = current_set
                records.setdefault(fields[0], []).append(row)
        skills = records['skill']
        for sid in ('64', '256'):
            self.assertTrue(all(r['frameburn'] == '0' for r in skills if r['id'] == sid))
        for row in (r for r in skills if r['id'] == '111'):
            self.assertEqual(set(row['roles'].split(',')), {'firepct:1', 'coldpct:2', 'ltngpct:3'})
        area = {r['id']: r['area'] for r in skills if r['set'] == 'rotw'}
        expect = {'0': '0', '36': '0', '45': '0', '106': '0', '70': '0', '126': '0', '47': '1', '48': '1', '53': '1', '12': '1', '102': '1', '402': '1',
                  '83': '1', '154': '1', '380': '1', '391': '1'}
        self.assertEqual({sid: area[sid] for sid in expect}, expect)
        itypes = {(r['set'], r['code']): r['supers'] for r in records['itype']}
        for mode, _ in g.SETS:
            self.assertEqual(itypes[(mode, 'h2h2')], 'h2h,mele,weap')
            self.assertEqual(itypes[(mode, 'ashd')], 'shld')
            for sid, want in (('260', ('h2h', '-', 'h2h', '-')), ('133', ('mele', '-', 'mele', '-')), ('385' if mode == 'rotw' else '117', ('weap', 'h2h', '-', '-') if mode == 'rotw' else ('shld', '-', '-', '-')),
                              ('0', ('weap', 'tpot', '-', '-'))):
                row = next(r for r in skills if r['set'] == mode and r['id'] == sid)
                self.assertEqual(tuple(row[c] for c in ('itypea', 'etypea', 'itypeb', 'etypeb')), want, sid)
        for row in (r for r in skills if r['id'] == '66'):
            self.assertEqual(row['auratargetstate'], 'amplifydamage')
        for token, mode, frames, speed in [('40', 'S3', '10', '240'), ('TG', 'S3', '10', '208'), ('TG', 'A1', '12', '224')]:
            self.assertTrue(any(r['token'] == token and r['mode'] == mode and r['frames'] == frames and r['speed'] == speed for r in records['anim']))
        for row in (r for r in records['miss'] if r['name'] == 'holybolt'):
            self.assertEqual((row['typefilter'], row['sHitPar2'], row['dParam1']), ('1', '3', '0'))
        self.assertTrue(all(r['drain'] == '66' for r in records['std'] if r['diff'] == '2' and r['act'] == '0'))
        for mode, prefix in g.SETS:
            areas = {int(r['id']): int(r['act']) for r in records['area'] if r['set'] == mode}
            for act, town in enumerate((1, 40, 75, 103, 109), 1):
                self.assertEqual(areas[town], act)
            tables = {name: g.table(read(prefix + name + '.txt').decode()) for name in g.NAMES}
            stats = {r['Stat']: int(r['*ID']) for r in tables['itemstatcost'][1] if r.get('*ID', '').isdigit()}
            db = g.Db(mode, tables, {}, stats)
            key = {r['*Id']: r['skill'] for r in db.skills}
            for sid in ('158', '167', '176', '161', '162', '163', '184', '179', '291'):
                self.assertFalse(g.damaging_skill(db, key[sid]), sid)
            for sid in ('351', '160', '318', '156'):
                self.assertTrue(g.damaging_skill(db, key[sid]), sid)
            exported = {r['name']: r for r in records['miss'] if r['set'] == mode}
            for name, row in exported.items():
                source = db.miss_by_name[name]
                self.assertEqual(int(row['HitShift']), g.num(source['HitShift']))
                for field, source_field in [('EMinLev', 'MinELev'), ('EMaxLev', 'MaxELev')]:
                    self.assertEqual(row[field], ','.join(str(g.num(source[source_field + str(i)])) for i in range(1, 6)))
            spell = next(r for r in skills if r['set'] == mode and r['id'] == '338')
            self.assertEqual(spell['variants'].split(','), [exported['necromage' + str(i)]['id'] for i in range(1, 5)])
            mage = next(r for r in skills if r['set'] == mode and r['id'] == '80')
            self.assertEqual(mage['unavailable'], '0')
            for row in (r for r in records['mon'] if r['set'] == mode and r['id'] in ('skeleton1', 'fetish1', 'sandmaggot1')):
                self.assertNotIn('x', row['flags'])
            levels = {int(r['Id']): (r, [], []) for r in tables['levels'][1] if r.get('Id', '').isdigit()}
            self.assertEqual(set(levels) - g.typical_areas(levels), {39, 133, 134, 135, 136})
            standard = next(r for r in records['std'] if r['set'] == mode and r['diff'] == '2' and r['act'] == '0')
            self.assertEqual((standard['elavg'], standard['elpct'], standard['elshare']), ('46', '40', '56'))
        original = read(g.TABLES[0])
        with self.assertRaises(g.Refuse): g.build(lambda p: original + b' ' if p == g.TABLES[0] else read(p))
        self.assertEqual(hashlib.sha256(b''.join(hashlib.sha256(read(p)).digest() for p in g.TABLES)).hexdigest(), g.TABLES_SHA)


class GameNameTests(unittest.TestCase):


    def test_untag(self):
        cases = (
            ('[ms]Abc Def', 'Abc Def'),
            ('[fs]Ghi', 'Ghi'),
            ('[mp]Jkl Mno', 'Jkl Mno'),
            ('[ns]Pqr', 'Pqr'),
            ('[nl]Stu', 'Stu'),
            ('[fs]uvw[ms]uvx[fp]uvy[mp]uvz', 'uvw'),
            ('[ms]aa[fs]ab[ns]ac[pl]ad', 'aa'),
            ('[ms]ba[fs]bb[ns]bc[p]bd', 'ba'),
            ('[fs][fp]Cde', 'Cde'),
            ('[ms]Efg [fs]Efh', 'Efg'),
            ('[ms]', ''),
        )
        for raw, shown in cases:
            with self.subTest(raw=raw): self.assertEqual(g.untag(raw), shown)

    def test_untag_keeps_unmarked_text_exactly(self):
        for raw in ('Abc Def', ' Abc  Def ', '[PH]Abc: %d', '[%s] Abc', 'Abc [ms]Def', '[Ab] Cd', '[MS]Abc', '', '[\u4e00\u4e8c]\u4e09'):
            with self.subTest(raw=raw): self.assertIs(g.untag(raw), raw)

    def test_game_names_drop_markers_in_every_language(self):
        row = {language: 'Name %s' % language for language in g.GAME_LANGUAGES}
        row.update(ptBR='[ms]Nome', esES='[fs]Nombre', plPL='[fs]Na[ms]Nb[nl]Nc', frFR='')
        names = dict(zip(g.GAME_LANGUAGES, g.game_names(row, 'key')))
        self.assertEqual((names['ptBR'], names['esES'], names['plPL']), ('Nome', 'Nombre', 'Na'))
        self.assertEqual(names['frFR'], 'Name enUS')
        self.assertEqual(names['deDE'], 'Name deDE')
        with self.assertRaises(ValueError): g.game_names(dict(row, ptBR='[ms]'), 'key', True)


if __name__ == '__main__':
    unittest.main()
