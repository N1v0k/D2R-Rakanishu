







import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import gen_effect_data as g

checks = fails = 0


def check(ok, what):
    global checks, fails
    checks += 1
    if not ok: fails += 1; print('FAIL', what)


def main():

    rows = [{'Name': 'Hell', 'ResistPenalty': '-100', 'ResistPenaltyNonExpansion': '-50'}, {'Name': 'Normal', 'ResistPenalty': '0', 'ResistPenaltyNonExpansion': '0'},
            {'Name': 'Nightmare', 'ResistPenalty': '-40', 'ResistPenaltyNonExpansion': '-20'}]
    check(g.penalties(rows) == [[0, 0], [-40, -20], [-100, -50]], 'penalty by difficulty name, expansion and classic column')
    check(g.penalties(rows[:2]) is None, 'a missing difficulty gives no penalty table')
    check(g.penalties([dict(rows[0], ResistPenalty='x')] + rows[1:]) is None, 'a malformed number gives no penalty table')
    check(g.penalties([{'Name': 'Normal'}, rows[0], rows[2]]) is None, 'a missing column raises nothing')


    head = {'Level': 'MaxLvl', **{c: '99' for c in g.CLASSES}}
    lv = [head] + [{'Level': str(n), **{c: str(n * 1000) for c in g.CLASSES}} for n in range(100)]
    t = g.thresholds(lv)
    check(len(t) == 8 and t['7'][0] == 0 and t['7'][8] == 8000 and len(t['0']) == 100, 'per class 100 thresholds, row N = the threshold for level N + 1')
    del lv[50]['Warlock']
    t = g.thresholds(lv)
    check('7' not in t and '6' in t, 'a class with a missing cell is left out, the others stay')


    names = {'short': ('Belegt Eure Waffe\nmit einem Fluch', 'Hexes your weapon\nto impair enemies'), 'dur': ('Dauer: %d Sekunden', 'Duration: %d seconds'),
             'one': ('Dauer: %d Sekunde', 'Duration: %d second'), 'def': ('%+d%% zu Verteidigung des Gegners', '%+d%% to Enemy Defense'), 'mana': ('Manakosten: %d', 'Mana Cost: %d')}
    skill = {'Param1': '', 'Param3': '3600', 'Param5': '30', 'Param6': '1', 'Param9': '5', 'calc1': "150+(((skill('Blast'.blvl)) *par9))", 'calc2': '',
             'aurastat1': 'damageresist', 'aurastatcalc1': '"min(ln56,50)*-1"', 'aurastat2': 'damagepercent', 'aurastatcalc2': '-25', 'aurastat3': 'nostat', 'aurastatcalc3': '-25',
             'EMin': '2', 'EMinLev1': '1', 'EMinLev2': '3', 'auralencalc': 'ln34'}
    stat_ids = {'damageresist': 36, 'damagepercent': 25, 'hitpoints': 6}
    desc = {'str short': 'short', 'dsc2line1': '36', 'dsc2texta1': 'one', 'dsc2textb1': 'dur', 'dsc2calca1': 'clc1', 'dsc2calcb1': '25',
            'dsc2line2': '74', 'dsc2texta2': 'def', 'dsc2calca2': 'ast1', 'dsc2line3': '40', 'dsc2texta3': 'def', 'dsc2calca3': 'ast2',
            'dsc2line4': '36', 'dsc2texta4': 'mana', 'dsc2calca4': 'usmc', 'dsc2calcb4': '256', 'dsc2line5': '74', 'dsc2texta5': 'missing', 'dsc2calca5': 'ast2'}
    c = g.skill_card(skill, desc, names, {'Blast': 384}, stat_ids)
    check(c['own'] == [{'st': 36, 'ca': 'min(ln56,50)*-1'}, {'st': 25, 'ca': '-25'}], 'the aura stats with their formulas, in column order; a stat the table does not know is left out')
    check(c['short'] == ['Belegt Eure Waffe mit einem Fluch', 'Hexes your weapon to impair enemies'], 'the short description joined into one line per language')
    check([l['ca'] for l in c['enemy']] == ['clc1', 'ast1'], 'enemy lines: 36 and 74 kept; line function 40, the mana cost and a missing text left out')
    check(c['enemy'][0]['a'] == list(names['one']) and c['enemy'][0]['b'] == list(names['dur']) and c['enemy'][0]['cb'] == '25' and c['enemy'][0]['f'] == 36, 'a 36 line: singular, plural, divisor')
    check('b' not in c['enemy'][1], 'a 74 line without a second text has no b')
    k = c['calc']
    check(k['par'][:9] == [0, 0, 3600, 0, 30, 1, 0, 0, 5] and len(k['par']) == g.PARAMS, 'Params 1-12, empty = 0')
    check(k['ast'][:2] == ['min(ln56,50)*-1', '-25'] and k['len'] == 'ln34' and k['calc'][0].startswith('150+'), 'aura stat formulas without quotes, length formula, calcs')
    check(k['emin'] == 2 and k['eminlev'] == [1, 3, 0, 0, 0], 'the row fields the variables edns / edmn read: EMin and its five level steps')
    check(k['refs'] == {'Blast': 384} and 'bad' not in k, 'a referred skill is resolved to its id')
    c = g.skill_card(skill, desc, names, {}, stat_ids)
    check(c['calc'].get('bad') == 1 and c['calc']['refs'] == {}, 'a reference to a skill missing from the table: cannot be evaluated')
    c = g.skill_card({'aurastat1': 'hitpoints', 'aurastatcalc1': "skill('Prayer'.edns)"}, {}, names, {'Prayer': 99}, stat_ids)
    check(c.get('own') is None and c.get('calc') is None, 'a stat whose aura formula reads another skill is not offered')
    check(set(g.CARD_VALUE_SKILLS) == {'Might', 'Prayer', 'Thorns', 'Defiance', 'Blessed Aim', 'Holy Freeze', 'Amplify Damage'},
          'card numbers only for the checked skills; Conviction, Fanaticism, Cleansing and shared states stay without a card')
    c = g.skill_card({}, {'str short': 'gone'}, names, {}, stat_ids)
    check(c == {}, 'no columns at all: nothing, and no exception')
    check(g.one_line('a\\nb') == 'a b' and g.one_line(' a \n\n b ') == 'a b', 'one line from escaped and real newlines')


    rows = [{'Stat': 'strength', 'op': ''}, {'Stat': 'vitality', 'op': '9', 'op stat1': 'maxhp', 'op stat2': 'maxstamina'}, {'Stat': 'maxhp', 'op': ''},
            {'Stat': 'maxstamina', 'op': ''}, {'Stat': 'skill_staminapercent', 'op': '1', 'op stat1': 'maxstamina'}, {'Stat': 'odd', 'op': '2', 'op stat1': 'gone'}]
    check(g.op_sources(rows) == {'2': [1], '3': [1, 4]}, 'op sources by target id; an unknown target is skipped')


    names = {'AllRes': ('Alle Widerstände %+d', 'All Resistances %+d'), 'AllAttr': ('%+d zu allen Attributen', '%+d to all Attributes')}
    res = {'dgrp': '2', 'dgrpfunc': '19', 'dgrpstrpos': 'AllRes', 'dgrpstrneg': 'AllRes', 'ValShift': ''}
    rows = [{'Stat': 'strength', 'dgrp': '1', 'dgrpfunc': '19', 'dgrpstrpos': 'AllAttr'}, dict(res, Stat='fireresist'), {'Stat': 'maxfire', 'dgrp': ''},
            dict(res, Stat='lightresist'), {'Stat': 'energy', 'dgrp': '1', 'dgrpfunc': '19', 'dgrpstrpos': 'AllAttr'}, dict(res, Stat='coldresist'),
            {'Stat': 'odd1', 'dgrp': '3', 'dgrpfunc': '41', 'dgrpstrpos': 'AllRes'}, {'Stat': 'odd2', 'dgrp': '3', 'dgrpfunc': '41', 'dgrpstrpos': 'AllRes'},
            {'Stat': 'odd3', 'dgrp': '4', 'dgrpfunc': '19', 'dgrpstrpos': 'Gone'}, {'Stat': 'odd4', 'dgrp': '4', 'dgrpfunc': '19', 'dgrpstrpos': 'Gone'}, {'Stat': 'lone', 'dgrp': '5', 'dgrpfunc': '19', 'dgrpstrpos': 'AllRes'}]
    gr = g.stat_groups(names, rows)
    check([x['ids'] for x in gr] == [[0, 4], [1, 3, 5]], 'groups by dgrp in id order; an unformattable function, a missing text and a group of one are left out')
    check(gr[1]['de'] == 'Alle Widerstände %+d' and gr[1]['nen'] == 'All Resistances %+d' and gr[1]['shift'] == 0, 'the group text, negative text falls back to positive')
    check(g.stat_groups(names, [{'Stat': 'x'}, {'Stat': 'y'}]) == [], 'no dgrp columns: no groups, no exception')

    print('test_effect_data: %d checks, %d failed' % (checks, fails))
    return fails == 0


if __name__ == '__main__':
    sys.exit(0 if main() else 1)
