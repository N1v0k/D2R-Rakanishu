#!/usr/bin/env python3






import json
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'ios', 'tools'))
import gen_book_data as g

checks = 0


def check(ok, what):
    global checks
    if not ok:
        raise SystemExit('test_book_data: FAIL ' + what)
    checks += 1


def raises(fn, what, text=None):
    try:
        fn()
    except g.Refuse as e:
        check(text is None or text in str(e), '%s: %s' % (what, e))
        return
    check(False, what + ': no refusal')


def tsv(rows):
    head = list(dict.fromkeys(h for r in rows for h in r))
    return ('\t'.join(head) + '\n' + ''.join('\t'.join(r.get(h, '') for h in head) + '\n' for r in rows)).encode()


def tc(name, picks, items, nodrop='', group='', level='', cond='', ladder='', u=''):
    r = {'Treasure Class': name, 'group': group, 'level': level, 'Picks': str(picks), 'Unique': u, 'Set': '', 'Rare': '', 'Magic': '',
         'NoDrop': nodrop, 'ConditionCalc': cond, 'firstLadderSeason': ladder, 'lastLadderSeason': ladder}
    for i in range(1, 11):
        r['Item%d' % i], r['Prob%d' % i] = items[i - 1] if i <= len(items) else ('', '')
    return r


def monster(id_, idx, tc_='', **kw):
    r = {'Id': id_, '*hcIdx': str(idx), 'NameStr': kw.pop('name', 'Name'), 'enabled': '1', 'killable': '1', 'Align': '', 'npc': '', 'inTown': '',
         'neverCount': '', 'minion1': '', 'minion2': '', 'spawn': '', 'Level': '5', 'Level(N)': '40', 'Level(H)': '70',
         'TreasureClass': tc_, 'TreasureClass(N)': tc_, 'TreasureClass(H)': tc_}
    r.update({'Skill%d' % k: '' for k in range(1, 9)})
    r.update(kw)
    return r


def tables():
    itemtypes = [{'ItemType': 'Weapon', 'Code': 'weap', 'Equiv1': '', 'Equiv2': '', 'Magic': '', 'Rare': '1', 'Normal': '', 'Class': '', 'TreasureClass': '1'},
                 {'ItemType': 'Axe', 'Code': 'axe', 'Equiv1': 'weap', 'Equiv2': '', 'Magic': '', 'Rare': '1', 'Normal': '', 'Class': '', 'TreasureClass': '', 'Rarity': '3'},
                 {'ItemType': 'Misc', 'Code': 'misc', 'Equiv1': '', 'Equiv2': '', 'Magic': '', 'Rare': '', 'Normal': '', 'Class': '', 'TreasureClass': ''},
                 {'ItemType': 'Rune', 'Code': 'rune', 'Equiv1': 'misc', 'Equiv2': '', 'Magic': '', 'Rare': '', 'Normal': '1', 'Class': '', 'TreasureClass': ''},
                 {'ItemType': 'Charm', 'Code': 'char', 'Equiv1': 'misc', 'Equiv2': '', 'Magic': '1', 'Rare': '', 'Normal': '', 'Class': '', 'TreasureClass': ''},
                 {'ItemType': 'Wand', 'Code': 'wand', 'Equiv1': 'weap', 'Equiv2': 'necr', 'Magic': '', 'Rare': '1', 'Normal': '', 'Class': '', 'TreasureClass': '', 'Rarity': '0'},
                 {'ItemType': 'Necro', 'Code': 'necr', 'Equiv1': '', 'Equiv2': '', 'Magic': '', 'Rare': '', 'Normal': '', 'Class': 'nec', 'TreasureClass': ''}]
    weapons = [{'code': 'axe', 'namestr': 'axe', 'level': '4', 'rarity': '3', 'spawnable': '1', 'type': 'axe', 'type2': '', 'normcode': 'axe'},
               {'code': '9ax', 'namestr': '9ax', 'level': '30', 'rarity': '2', 'spawnable': '1', 'type': 'axe', 'type2': '', 'normcode': 'axe'},
               {'code': 'wnd', 'namestr': 'wnd', 'level': '5', 'rarity': '1', 'spawnable': '1', 'type': 'wand', 'type2': '', 'normcode': 'wnd'},
               {'code': 'xxx', 'namestr': 'xxx', 'level': '5', 'rarity': '1', 'spawnable': '0', 'type': 'axe', 'type2': '', 'normcode': 'xxx'},
               {'code': 'qst', 'namestr': 'qst', 'level': '5', 'rarity': '1', 'spawnable': '1', 'type': 'axe', 'type2': '', 'normcode': 'qst', 'quest': '1'}]
    misc = [{'code': 'r01', 'namestr': 'r01', 'level': '1', 'rarity': '0', 'spawnable': '1', 'type': 'rune', 'type2': ''},
            {'code': 'cm1', 'namestr': 'cm1', 'level': '1', 'rarity': '0', 'spawnable': '1', 'type': 'char', 'type2': ''},
            {'code': 'gld', 'namestr': 'gld', 'level': '0', 'rarity': '0', 'spawnable': '1', 'type': 'misc', 'type2': ''}]
    tcs = [tc('Root', 2, [('Sub', '3'), ('gld,mul=1280', '2'), ('weap6', '1'), ('r01', '1')], nodrop='5', group='7', level='1', u='512'),
           tc('Root B', 1, [('r01', '1')], group='7', level='20'),
           tc('Sub', -2, [('Annihilus', '1'), ('cm1', '1')]),
           tc('Hell', 1, [('r01', '1')], cond="cond('Difficulty', hell)*(cond('Desecrated')==0)"),
           tc('Zone', 1, [('r01', '1')], cond="cond('Desecrated')"),
           tc('Season', 1, [('r01', '1')], ladder='2'),
           tc('Herald', 1, [('r01', '1')], cond="(stat('heraldtier'.accr) >4) ")]
    uniques = [{'index': 'Annihilus', '*ID': '1', 'disabled': '', 'spawnable': '0', 'DropConditionCalc': '', 'rarity': '1', 'lvl': '70', 'code': 'cm1'},
               {'index': 'Axe Unique', '*ID': '2', 'disabled': '', 'spawnable': '1', 'DropConditionCalc': "cond('Difficulty', hell)", 'rarity': '5', 'lvl': '3', 'code': 'axe'},
               {'index': 'Ladder Axe', '*ID': '3', 'disabled': '', 'spawnable': '1', 'DropConditionCalc': '', 'rarity': '5', 'lvl': '3', 'code': 'axe',
                'firstLadderSeason': '15', 'lastLadderSeason': '15'}]
    sets = [{'index': 'Set Wand', '*ID': '4', 'disabled': '', 'spawnable': '1', 'rarity': '7', 'lvl': '2', 'item': 'wnd', 'set': 'S0'},
            {'index': 'Off Set', '*ID': '5', 'disabled': '', 'spawnable': '', 'rarity': '7', 'lvl': '2', 'item': 'wnd', 'set': 'S0'},
            {'index': 'Bovine Wand', '*ID': '6', 'disabled': '', 'spawnable': '1', 'rarity': '7', 'lvl': '2', 'item': 'wnd', 'set': 'S29'}]
    set_table = [{'index': 'S%d' % i, 'name': 'S%d' % i} for i in range(16)] + [{'index': 'Expansion', 'name': ''}] + \
        [{'index': 'S%d' % i, 'name': 'S%d' % i} for i in range(16, 31)]
    ratio = [{'Version': v, 'Uber': u, 'Class Specific': c, 'Unique': '400', 'UniqueDivisor': '1', 'UniqueMin': '6400', 'Rare': '100', 'RareDivisor': '2',
              'RareMin': '3200', 'Set': '160', 'SetDivisor': '2', 'SetMin': '5600', 'Magic': '34', 'MagicDivisor': '3', 'MagicMin': '192'}
             for v in ('0', '1') for u in ('0', '1') for c in ('0', '1')]
    mons = [monster('walker', 1, 'Root', minion1='helper'), monster('helper', 2, 'Root B', Skill1='Call'), monster('called', 3, spawn='egg'),
            monster('egg', 4), monster('friend', 5, Align='1'), monster('ghost', 6, neverCount='1'), monster('boss', 7, 'Hell'),
            monster('lonely', 8, 'Root'), monster('noname', 9, name='Nobody'), monster('hellbovine', 10, 'Root', noRatio='1')]
    mons[1]['minion1'] = 'friend'
    mons[2]['minion2'] = 'ghost'
    mons[2]['minion1'] = 'noname'
    levels = [{'Id': '1', 'mon1': 'walker', 'nmon1': 'boss', 'mon2': 'hellbovine', 'umon1': ''}]
    mons[6]['boss'] = '1'
    su = [{'Superunique': 'Su', 'Class': 'walker', 'hcIdx': '0', 'TC': 'Root', 'TC(N)': 'Root', 'TC(H)': 'Hell'}]
    skills = [{'skill': 'Call', 'summon': 'called'}]
    t = {'treasureclassex': tcs, 'monstats': mons, 'superuniques': su, 'levels': levels, 'itemratio': ratio, 'itemtypes': itemtypes, 'weapons': weapons,
         'armor': [{'code': 'arm', 'namestr': 'arm', 'level': '7', 'rarity': '0', 'spawnable': '1', 'type': 'weap', 'type2': '', 'normcode': 'arm'}],
         'misc': misc, 'uniqueitems': uniques, 'setitems': sets, 'monpreset': [{'Act': '1', 'Place': ''}], 'skills': skills, 'sets': set_table}
    for k in ('weapons', 'armor', 'misc'):
        for r in t[k]:
            r.setdefault('quest', '')
    return {k: g.table(tsv(v), k) for k, v in t.items()}


def main():
    t = tables()
    out, keys = g.build_set(t, {}, {'Name'}, {})
    check([r[0:9] for r in out['tc'][:2]] == [[2, 5, 512, 0, 0, 0, 7, 1, 7], [1, 0, 0, 0, 0, 0, 7, 20, 7]], 'class heads in table order: %s' % out['tc'][:2])
    items = {it[0]: k for k, it in enumerate(out['item'])}
    names = ['Root', 'Root B', 'Sub', 'Hell', 'Zone', 'Season', 'Herald', 'weap6']
    check(len(out['tc']) == len(names), 'one item-table class after the table\'s: %d' % len(out['tc']))
    check(out['tc'][0][9] == [0, 2, 3, 1, items['gld'], 2, 0, 7, 1, 1, items['r01'], 1], 'references: class, gold with its multiplier, item class, item')
    check(out['tc'][2][9] == [2, 0, 1, 1, items['cm1'], 1], 'a unique named by the class drops as itself: %s' % out['tc'][2][9])
    check(out['tc'][7][9] == [1, items['axe'], 3, 1, items['wnd'], 1], 'weap6: the spawnable non-quest weapons of levels 4 to 6, weighted by their own '
          'type\'s Rarity (at least 1): %s' % out['tc'][7])
    check([r[8] for r in out['tc'][3:7]] == [4, 0, 0, 0], 'conditions: hell outside a zone, a zone, a ladder season, a herald')
    flags = {it[0]: it[4] for it in out['item']}
    check(flags['axe'] == g.QUALITY and flags['9ax'] == g.QUALITY | g.UBER and flags['wnd'] == g.QUALITY | g.CLASS and flags['r01'] == 0
          and flags['cm1'] == g.QUALITY and flags['gld'] == 0, 'item flags %s' % flags)
    check({it[0]: it[3] for it in out['item']}['r01'] == 'r' and {it[0]: it[3] for it in out['item']}['wnd'] == 'w', 'categories')
    check([u[0] for u in out['unique']] == [1, 2, 3] and [u[3] for u in out['unique']] == [0, 5, 5] and out['unique'][1][5] == 4 and out['unique'][2][5] == 7,
          'uniques: only spawnable ones roll, a ladder season one too, with their difficulties: %s' % out['unique'])
    check([s[0] for s in out['set']] == [4, 6] and [s[5] for s in out['set']] == [0, 1], 'set items the game can roll; the bovine set (record 29)')
    check(len(out['ratio']) == 4 and all(len(r) == 14 for r in out['ratio']), 'the expansion\'s quality rows')
    check(sorted(out['extra']) == ['2', '3', '4'] and out['extra']['4'][3] == [3] and out['extra']['2'][3] == [1] and out['extra']['2'][1] == 'helper',
          'extra monsters: minion, summon and its spawn; not the ally, the uncounted, the nameless or the unreached: %s' % out['extra'])
    check(out['mon']['1'] == [0, 0, 0, g.UPGRADES] and out['mon']['2'] == [1, 1, 1, g.UPGRADES] and '8' not in out['mon']
          and out['su']['0'] == [0, 0, 3, g.UPGRADES] and out['mon']['7'][3] == 0 and out['mon']['10'][3] == g.BOVINE_DROP,
          'classes of the book\'s monsters and the superuniques')
    check({'axe', 'Axe Unique', 'Set Wand'} <= keys, 'name keys')

    def broken(fix):
        t2 = tables()
        fix(t2)
        return lambda: g.build_set(t2, {}, {'Name'}, {})
    raises(broken(lambda t2: t2['treasureclassex'].append(tc('Bad', 1, [('nothing', '1')]))), 'a reference to nothing', 'nothing')
    raises(broken(lambda t2: t2['treasureclassex'].extend([tc('A', 1, [('B', '1')]), tc('B', 1, [('A', '1')])])), 'a cycle', 'call each other')
    raises(broken(lambda t2: t2['treasureclassex'].append(tc('Odd', 1, [('r01', '1')], cond="cond('Moon', full)"))), 'an unknown condition')
    raises(broken(lambda t2: t2['itemratio'].pop()), 'a missing quality row', 'itemratio')
    raises(lambda: g.build(lambda p: b''), 'a wrong fingerprint', 'not those of')
    check(g.condition("(stat('heraldtier'.accr)>2)*(stat('heraldtier'.accr) <5) ", 'x') == 0 and g.condition('', 'x') == 7
          and g.condition("cond('Difficulty', nightmare)", 'x') == 2, 'conditions under the standard rules')

    game = os.environ.get('D2R_GAME')
    if not game:
        print('test_book_data: SKIPPED real data (set D2R_GAME to the Update directory with RomFS)')
    else:
        with tempfile.TemporaryDirectory() as d:
            g.stage(os.path.join(game, 'RomFS'), d)
            raw = open(os.path.join(d, 'book.json'), 'rb').read()
            data = json.loads(raw)
            check(len(raw) < 2 * 1024 * 1024, 'book.json %d bytes, under 2 MiB' % len(raw))
            for name, s in data['sets'].items():
                check(all(-1 <= c < len(s['tc']) for v in list(s['mon'].values()) + list(s['su'].values()) for c in v), name + ': monster classes resolve')
            g.stage(os.path.join(game, 'RomFS'), d)
            check(open(os.path.join(d, 'book.json'), 'rb').read() == raw, 'the same bytes twice')
            print('real data: book.json %d KiB, %s' % (len(raw) // 1024, ', '.join('%s %d classes, %d extra monsters' % (k, len(v['tc']), len(v['extra']))
                                                                                  for k, v in sorted(data['sets'].items()))))
    print('test_book_data: %d checks passed' % checks)


if __name__ == '__main__':
    main()
