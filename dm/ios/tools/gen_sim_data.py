#!/usr/bin/env python3






import csv
import hashlib
import io
import json
import os
import pathlib
import re
import statistics
import struct
import sys
import tempfile
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from dm_i18n import untag
FORMAT = 5
NAMES = ('skills', 'skilldesc', 'skillcalc', 'missiles', 'monstats', 'monstats2', 'monlvl', 'monumod', 'superuniques', 'levels',
         'difficultylevels', 'charstats', 'states', 'itemstatcost', 'playerclass', 'plrtype', 'monpet', 'misc', 'itemtypes')
SETS = (('rotw', 'global/excel/'), ('lod', 'global/excel/base/'))
ANIMDATA = 'global/AnimData.D2'
TABLES = tuple(p + n + '.txt' for _, p in SETS for n in NAMES) + (ANIMDATA,)
TABLES_SHA = 'bbdfd99a90ca1d6ee50ea96de99f5c91e7fe841f1a6c91192eb7c13dd90076f3'
DIFF_SUFFIX = ('', '(N)', '(H)')
ELEMENTS = ('phys', 'fire', 'ltng', 'cold', 'pois', 'mag')

PLAYER_MODES = ('A1', 'A2', 'S1', 'S2', 'S3', 'S4', 'SC', 'TH', 'KK', 'GH', 'BL')



FAMILY = {
    0: 'W1',

    6: 'W3', 7: 'W3', 11: 'W3', 21: 'W3', 16: 'W3', 22: 'W3', 27: 'W3+Z3', 31: 'W3', 12: 'W3', 15: 'W3+DoT', 25: 'W3+DoT', 20: 'W3', 35: 'W3',
    24: 'W1+Z1', 26: 'W2+W3', 14: 'W1', 34: 'W1+Z1', 19: 'W1', 10: 'W2', 30: 'W2', 8: 'K', 17: 'K', 9: 'Pa', 13: 'Pa', 18: 'Pa', 29: 'Pa', 23: 'Pa',
    33: 'Pa', 28: 'P', 32: 'P',

    36: 'Z1', 39: 'Z1', 45: 'Z1', 47: 'Z1', 55: 'Z1', 49: 'Z1', 53: 'Z1', 38: 'Z1', 43: 'Z1', 44: 'Z2', 48: 'Z2', 41: 'Z3', 46: 'Z3', 51: 'Z3',
    56: 'Z3', 57: 'Z3', 59: 'Z3', 64: 'Z3', 42: 'L', 40: 'B', 50: 'B', 60: 'B', 58: 'B', 52: 'B', 54: 'U', 62: 'F', 37: 'Pa', 61: 'Pa', 63: 'Pa',
    65: 'Pa',

    67: 'Z1', 84: 'Z1', 93: 'Z1', 73: 'W1+DoT', 74: 'L', 83: 'L+DoT', 92: 'Z2+DoT', 66: 'K', 87: 'K', 91: 'K', 72: 'K', 76: 'K', 82: 'K', 71: 'K',
    77: 'K', 81: 'K', 86: 'K', 78: 'U', 88: 'U', 68: 'B', 70: 'P', 80: 'P', 75: 'P', 85: 'P', 94: 'P', 90: 'P', 95: 'P', 69: 'Pa', 79: 'Pa', 89: 'Pa',

    96: 'W1', 97: 'W1', 111: 'W1', 116: 'W1+K', 107: 'W1', 106: 'W2', 101: 'Z1', 112: 'Z1', 121: 'Z1', 98: 'A+', 108: 'A+', 113: 'A+', 122: 'A+',
    123: 'A+', 102: 'A~', 118: 'A~', 114: 'A~', 119: 'A~', 103: 'A~', 99: 'A-', 100: 'A-', 105: 'A-', 110: 'A-', 125: 'A-', 104: 'A-', 109: 'A-',
    115: 'A-', 120: 'A-', 124: 'A-', 117: 'B',

    126: 'W1', 139: 'W1', 144: 'W1', 143: 'W1+Z2', 152: 'W1', 133: 'W2', 140: 'W2+W3', 147: 'W2', 151: 'W2', 154: 'Z2', 146: 'K', 137: 'K', 130: 'K',
    150: 'K', 138: 'B', 149: 'B', 155: 'B', 131: 'U', 142: 'U', 132: 'U', 127: 'Pa', 128: 'Pa', 129: 'Pa', 134: 'Pa', 135: 'Pa', 136: 'Pa', 141: 'Pa',
    145: 'Pa', 148: 'Pa', 153: 'Pa',

    221: 'P', 227: 'P', 237: 'P', 247: 'P', 222: 'P+DoT', 231: 'P', 241: 'P', 226: 'P', 236: 'P', 246: 'P', 223: 'B', 228: 'B', 235: 'B', 224: 'Pa',
    232: 'W1', 233: 'W1', 238: 'W1+DoT', 239: 'W1', 242: 'W1', 248: 'W2', 229: 'Z1', 240: 'Z1', 243: 'Z2', 230: 'Z3', 225: 'Z3', 234: 'Z3', 244: 'Z3',
    245: 'Z3', 249: 'Z3', 250: 'Z3',

    251: 'Z2', 253: 'Z1', 256: 'Z3', 277: 'Z3', 266: 'W3', 254: 'W4', 265: 'W4', 259: 'W4', 269: 'W4', 274: 'W4', 280: 'W4', 255: 'W5', 260: 'W2',
    270: 'W5', 275: 'W5', 273: 'Z2+K', 261: 'F', 262: 'F', 271: 'F', 272: 'F', 276: 'F+L', 257: 'F', 268: 'P', 279: 'P', 264: 'K', 278: 'B+DoT',
    258: 'B', 267: 'B', 252: 'Pa', 263: 'Pa',

    373: 'P', 376: 'P', 377: 'P', 382: 'P', 375: 'K+B', 379: 'B', 381: 'B', 387: 'B', 380: 'Z2', 384: 'Z3', 394: 'Z2', 390: 'Z1', 395: 'Z1+Z3',
    398: 'Z3', 399: 'Z3', 401: 'Z3', 402: 'Z3', 400: 'L', 393: 'K', 396: 'K', 385: 'K', 386: 'K', 389: 'K', 388: 'W3', 391: 'W2', 392: 'W2',
    374: 'Pa', 378: 'Pa', 383: 'Pa', 397: 'Pa',
}
FAMILIES = frozenset(('W1', 'W2', 'W3', 'W4', 'W5', 'Z1', 'Z2', 'Z3', 'DoT', 'L', 'P', 'F', 'A+', 'A~', 'A-', 'K', 'B', 'Pa', 'U'))


SU_AREA = {
    0: 3, 1: 18, 2: 9, 3: 4, 4: 5, 6: 25, 7: 30, 9: 33, 11: 60, 12: 61, 13: 43, 14: 59, 15: 64, 16: 54, 17: 44, 19: 66, 20: 28, 21: 85, 22: 91,
    23: 79, 24: 94, 25: 92, 26: 83, 27: 83, 28: 102, 29: 83, 30: 102, 31: 102, 36: 108, 37: 108, 38: 108, 39: 39, 40: 8, 41: 107, 42: 110, 43: 120,
    44: 120, 45: 120, 47: 115, 48: 110, 49: 111, 51: 112, 52: 121, 53: 119, 59: 114, 61: 131, 62: 131, 63: 131, 64: 131, 65: 131,
}

BOSSES = {
    156: (1, 'actboss', 37), 211: (2, 'actboss', 73), 242: (3, 'actboss', 102), 243: (4, 'actboss', 108), 544: (5, 'actboss', 132),
    267: (1, 'quest', 17), 365: (1, 'quest', 38), 229: (2, 'quest', 49), 250: (2, 'quest', 74), 256: (4, 'quest', 105), 526: (5, 'quest', 124),
    546: (5, 'quest', 125), 707: (5, 'uber', 133), 708: (5, 'uber', 134), 706: (5, 'uber', 135), 704: (5, 'uber', 136), 705: (5, 'uber', 136),
    709: (5, 'uber', 136), 745: (5, 'ancient', 137), 746: (5, 'ancient', 137), 747: (5, 'ancient', 137), 333: (0, 'clone', 0),
}
CALC_FIELDS = (['calc%d' % i for i in range(1, 11)] + ['ToHitCalc', 'DmgSymPerCalc', 'EDmgSymPerCalc', 'ELenSymPerCalc', 'auralencalc',
               'aurarangecalc', 'petmax', 'prgcalc1', 'prgcalc2', 'prgcalc3'] + ['aurastatcalc%d' % i for i in range(1, 7)]
               + ['passivecalc%d' % i for i in range(1, 15)] + ['sumsk%dcalc' % i for i in range(1, 6)] + ['perdelay'])

DAMAGE_FIELDS = ('EDmgSymPerCalc', 'DmgSymPerCalc', 'ddam1', 'ddam2')
MISS_CALC_FIELDS = ('DmgCalc1', 'Range', 'DmgSymPerCalc', 'EDmgSymPerCalc', 'SrvCalc1')

VARS = frozenset(['lvl', 'blvl', 'ulvl', 'edmn', 'edmx', 'edln', 'edns', 'edxs', 'enma', 'exma', 'enms', 'exms', 'edma', 'toht', 'mana', 'len',
                  'rng', 'pets', 'mael', 'madm', 'math', 'macr', 'pnma', 'pxma', 'pnms', 'pxms', 'skpt', 'usmc', 'mps', 'fnum']
                 + ['clc%d' % i for i in range(1, 11)] + ['ast%d' % i for i in range(1, 7)] + ['pst%d' % i for i in range(1, 15)])


def fam_parts(fam):

    parts = re.findall(r'A[+~-]|[WZ]\d|DoT|Pa|[LPFKBU]', fam)
    return parts if parts and '+'.join(parts) == fam and set(parts) <= FAMILIES and re.sub(r'A[+~-]|[WZ]\d|DoT|Pa|[LPFKBU]|\+', '', fam) == '' else None



ROLES = (('dmgpct', r'^damage %$'), ('flat', r'flat damage( \(melee only\)| added)?$'), ('elpct', r'% damage dealt as elemental'),
         ('sias', r'attack speed %( \(from .*\))?$'), ('hits', r'# of (hits|kicks)$'), ('attacks', r'amount of attacks$'),
         ('after1pct', r'total damage % applied to attacks after the first'), ('nmiss', r'# of (bolt )?missiles created$'),
         ('jumps', r'# of target jump hits$'), ('petlvl', r'summon pet level'), ('pethp', r'^hp %( modifier)?$'), ('ll', r'^lifesteal %$'),
         ('firepct', r'^fire damage %$'), ('coldpct', r'^cold damage %$'), ('ltngpct', r'^lightning damage %$'),
         ('dur', r'^(debuff )?duration$'), ('charges', r'# of max charges$'))
PARAM_ROLES = (('rollback', r'% anim frame rollback'),)




HIT_FACTORS = {64: 300, 256: 100, 59: 300, 56: 100, 234: 300, 402: 300, 38: 300, 67: 100, 249: 25, 250: 100, 277: 100, 57: 100}
FRAME_BURNS = {51: 1, 46: 1, 225: 1, 41: 2, 230: 2}
TIMED_DAMAGE = {249: 'par4', 250: 'par4', 277: 'par3', 57: '(100-dm56) * par4/100 + par3'}
BURNING_IMPACT = 56


def roles(row):

    out = []
    for col, val in row.items():
        m = re.match(r'\*calc(\d+) ?desc$', col)
        if m and row.get('calc' + m.group(1), '').strip():
            for role, pat in ROLES:
                if re.search(pat, val.strip().lower()): out.append('%s:%s' % (role, m.group(1)))
        m = re.match(r'\*Param(\d+) ?Description', col)
        if m:
            for role, pat in PARAM_ROLES:
                if re.search(pat, val.strip().lower()): out.append('%s:P%s' % (role, m.group(1)))
    return ','.join(sorted(set(out))) or '-'






AREA_RING_FUNCS = {22, 63, 68}
AREA_FIELD_FUNCS = {20, 38, 50, 53, 54, 55, 66, 142, 145, 160, 163, 164, 165}
AREA_SKILL_TEXT = (('radius', r'radius|area of effect'), ('missiles', r'# of (bolt )?missiles|number of missiles|# missiles|missile spawn rate'),
                   ('chain', r'target jump hits'))
AREA_MISSILE_TEXT = (('radius', r'explosion radius|damage radius|radius to create|disc radius'),
                     ('missiles', r'# of hitsubmissile|missile spawn rate|how many missiles|ring density'))


def area_reasons(db, row):

    if row.get('summon', '').strip(): return []
    found = set()
    if num(row.get('aura')) and row.get('aurarangecalc', '').strip(): found.add('aura')
    func = num(row.get('srvdofunc'))
    if func in AREA_RING_FUNCS: found.add('ring')
    if func in AREA_FIELD_FUNCS: found.add('field')
    for col, text in row.items():
        m = re.match(r'\*(calc(\d+) ?desc|Param(\d+) ?Description)', col)
        if not m or not text: continue
        used = row.get('calc' + m.group(2)) if m.group(2) else row.get('Param' + m.group(3))
        if not (used or '').strip(): continue
        found |= {reason for reason, pattern in AREA_SKILL_TEXT if re.search(pattern, text.strip().lower())}
    for name in reach_missiles(db, row):
        missile = db.miss_by_name[name]
        if missile.get('Radius', '').strip(): found.add('radius')
        for col, text in missile.items():
            if not col.startswith('*') or 'client' in col.lower() or not text: continue
            found |= {reason for reason, pattern in AREA_MISSILE_TEXT if re.search(pattern, text.strip().lower())}
    return [reason for reason in ('aura', 'ring', 'field', 'chain', 'radius', 'missiles') if reason in found]


class Refuse(Exception):
    """This RomFS gets no data."""


def table(text):
    rows = list(csv.reader(io.StringIO(text), delimiter='\t', quoting=csv.QUOTE_NONE))
    if not rows: raise Refuse('empty table')
    head = [h.strip() for h in rows[0]]
    return head, [dict(zip(head, (c.strip() for c in r))) for r in rows[1:] if r and any(c.strip() for c in r)]


def need(head, cols, name):
    miss = [c for c in cols if c not in head]
    if miss: raise Refuse('%s lacks %s' % (name, miss))


def num(s, default=0):
    s = (s or '').strip()
    try: return int(s)
    except ValueError:
        try: return int(float(s))
        except ValueError: return default


def animdata(raw):


    out, o = {}, 0
    for _ in range(256):
        if o + 4 > len(raw): raise Refuse('AnimData truncated')
        n, = struct.unpack_from('<I', raw, o); o += 4
        for _ in range(n):
            if o + 160 > len(raw): raise Refuse('AnimData truncated')
            name = raw[o:o + 8].split(b'\0')[0].decode('latin-1').upper()
            fpd, spd = struct.unpack_from('<II', raw, o + 8)
            ev = raw[o + 16:o + 160]
            out[name] = (fpd, spd, next((k for k, e in enumerate(ev) if e in (1, 2)), -1)); o += 160
    if o != len(raw): raise Refuse('AnimData has %d trailing bytes' % (len(raw) - o))
    return out



_TOK = re.compile(r"\s*(?:(\d+)|('[^']*')|([A-Za-z_]\w*)|(<=|>=|==|!=|&&|\|\||[-+*/%()?:,.<>!]))")
_BIN = {'||': 1, '&&': 2, '==': 3, '!=': 3, '<': 4, '>': 4, '<=': 4, '>=': 4, '+': 5, '-': 5, '*': 6, '/': 6, '%': 6}


class CalcError(Exception):
    pass


def lex(expr):
    out, pos = [], 0
    while pos < len(expr):
        if expr[pos:].strip() == '': break
        m = _TOK.match(expr, pos)
        if not m: raise CalcError('bad character at %d in %r' % (pos, expr))
        pos = m.end()
        n, s, i, op = m.groups()
        out.append(('n', int(n)) if n is not None else ('s', s[1:-1]) if s is not None else ('i', i) if i is not None else ('o', op))
    return out


class Compiler:



    def __init__(self, db):
        self.db = db

    def compile(self, owner, expr):
        expr = (expr or '').strip()
        if len(expr) >= 2 and expr[0] == expr[-1] == '"': expr = expr[1:-1].strip()
        if not expr: return None
        self.t, self.k, self.owner = lex(expr), 0, owner
        out = self.ternary()
        if self.k != len(self.t): raise CalcError('trailing tokens in %r' % expr)
        return ' '.join(str(x) for x in out)

    def peek(self):
        return self.t[self.k] if self.k < len(self.t) else (None, None)

    def take(self, kind=None, val=None):
        tok = self.peek()
        if tok[0] is None or (kind and tok[0] != kind) or (val is not None and tok[1] != val): raise CalcError('expected %s %s, got %r' % (kind, val, tok))
        self.k += 1
        return tok

    def ternary(self):
        c = self.binary(1)
        if self.peek() == ('o', '?'):
            self.take(); a = self.ternary(); self.take('o', ':'); b = self.ternary()
            return c + a + b + ['?:']
        return c

    def binary(self, prec):
        left = self.unary()
        while True:
            tok = self.peek()
            p = _BIN.get(tok[1]) if tok[0] == 'o' else None
            if p is None or p < prec: return left
            self.take()
            left = left + self.binary(p + 1) + [tok[1]]

    def unary(self):
        tok = self.peek()
        if tok == ('o', '-'): self.take(); return self.unary() + ['neg']
        if tok == ('o', '+'): self.take(); return self.unary()
        if tok == ('o', '!'): self.take(); return self.unary() + ['!']
        return self.primary()

    def primary(self):
        kind, val = self.take()
        if kind == 'n': return [val]
        if kind == 'o' and val == '(':
            e = self.ternary()
            if self.peek()[0] is None: return e
            self.take('o', ')'); return e
        if kind != 'i': raise CalcError('unexpected %r' % val)
        if self.peek() == ('o', '('):
            self.take()
            if val in ('skill', 'stat', 'miss'):
                ref = self.take('s')[1]; self.take('o', '.'); member = self.take('i')[1]
                if self.peek()[0] is not None: self.take('o', ')')
                return self.db.ref(self.owner, val, ref, member)
            args = [self.ternary()]
            while self.peek() == ('o', ','):
                self.take(); args.append(self.ternary())
            self.take('o', ')')
            if val in ('min', 'max') and len(args) == 2: return args[0] + args[1] + [val]
            if val == 'rand' and len(args) == 2: return args[0] + args[1] + ['avg']
            raise CalcError('unknown function %s/%d' % (val, len(args)))
        return self.db.ident(self.owner, val)


class Db:


    def __init__(self, name, t, anim, stat_ids):
        self.name, self.t, self.anim, self.stat_ids = name, t, anim, stat_ids
        h, self.skills = t['skills']
        need(h, ('skill', '*Id', 'charclass', 'skilldesc', 'srvstfunc', 'srvdofunc', 'anim', 'seqnum', 'summon', 'pettype', 'aura', 'passive', 'aurastate',
                 'localdelay', 'globaldelay', 'mana', 'lvlmana', 'minmana', 'manashift', 'ToHit', 'LevToHit', 'HitShift', 'SrcDam', 'MinDam',
                 'MaxDam', 'EType', 'EMin', 'EMax', 'ELen', 'Kick', 'periodic', 'perdelay', 'alwayshit', 'UseAttackRate', 'srvmissile',
                 'srvmissilea', 'range', 'requiresweapon', 'reqlevel', 'maxlvl') + tuple(CALC_FIELDS)
             + tuple('Param%d' % i for i in range(1, 21)) + tuple('MinLevDam%d' % i for i in range(1, 6)) + tuple('MaxLevDam%d' % i for i in range(1, 6))
             + tuple('EMinLev%d' % i for i in range(1, 6)) + tuple('EMaxLev%d' % i for i in range(1, 6)) + tuple('ELevLen%d' % i for i in range(1, 4))
             + tuple('aurastat%d' % i for i in range(1, 7)) + tuple('passivestat%d' % i for i in range(1, 15))
             + tuple(c for _, cols in HAND_COLS for c in cols), 'skills')
        self.skill_by_name = {r['skill']: r for r in self.skills if r.get('*Id', '').isdigit()}
        h, sd = t['skilldesc']; need(h, ('skilldesc', 'descdam', 'ddam calc1', 'ddam calc2', 'descmissile1', 'descmissile2', 'descmissile3', 'str name'), 'skilldesc')
        self.desc = {r['skilldesc']: r for r in sd}
        h, self.missiles = t['missiles']
        need(h, ('Missile', '*ID', 'Vel', 'Range', 'NextHit', 'NextDelay', 'SrcDamage', 'HitShift', 'MinDamage', 'MaxDamage', 'EType', 'EMin',
                 'EMax', 'ELen', 'Skill', 'SubMissile1', 'SubMissile2', 'SubMissile3', 'HitSubMissile1', 'HitSubMissile2', 'HitSubMissile3',
                 'HitSubMissile4', 'ExplosionMissile', 'DmgCalc1', 'DmgSymPerCalc', 'EDmgSymPerCalc', 'Param1', 'dParam1', 'dParam2',
                 'ApplyMastery', 'SrcMissDmg', 'MissileSkill', 'pSrvDoFunc', 'InitSteps', 'Accel', 'MaxVel'), 'missiles')
        self.miss_by_name = {r['Missile']: r for r in self.missiles if r.get('*ID', '').strip().lstrip('-').isdigit()}
        h, sc = t['skillcalc']; need(h, ('code', '*Description'), 'skillcalc')
        self.level_calc = {}
        for r in sc:
            m = re.match(r'(ln|dm)\d\d$', r['code'])
            if m:
                ps = re.findall(r'\bpa(?:r)?(\d+)', r['*Description'])
                if len(ps) >= 2:
                    a, b = (ps[1], ps[0]) if m.group(1) == 'dm' else (ps[0], ps[1])
                    self.level_calc[r['code']] = (m.group(1), int(a), int(b))
        self.comp = Compiler(self)
        self.skill_closure = set()

    def params(self, row, kind):
        return [num(row.get('Param%d' % i)) for i in range(1, 21 if kind == 'skill' else 6)]

    def level_formula(self, row, kind, name, lvl_token):
        if name in self.level_calc: f, a, b = self.level_calc[name]
        else:
            m = re.match(r'(ln|dm)(\d)(\d)$', name)
            if not m: return None
            f, a, b = m.group(1), int(m.group(2)), int(m.group(3))
        p = self.params(row, kind)
        if not (1 <= a <= len(p) and 1 <= b <= len(p)): raise CalcError('param out of range in %s' % name)
        return [p[a - 1], p[b - 1], lvl_token, 'LN' if f == 'ln' else 'DM']

    def ident(self, owner, name):
        kind, row = owner
        m = re.match(r'pa(?:r)?(\d+)$', name)
        if m:
            p = self.params(row, kind); i = int(m.group(1))
            if not 1 <= i <= len(p): raise CalcError('no param %s' % name)
            return [p[i - 1]]
        lf = self.level_formula(row, kind, name, 'lvl')
        if lf: return lf
        if kind == 'miss' and name == 'dl12': return [num(row.get('dParam1')), num(row.get('dParam2')), 'lvl', 'LN']
        if name == 'clc0': return [0]
        if name in VARS: return [name]
        raise CalcError('unknown identifier %s' % name)

    def ref(self, owner, func, ref, member):
        if func == 'stat':
            if ref not in self.stat_ids: raise CalcError('unknown stat %s' % ref)
            if member not in ('accr', 'base', 'mod'): raise CalcError('stat member %s' % member)
            return ['#%d' % self.stat_ids[ref]]
        if func == 'miss':
            r = self.miss_by_name.get(ref); m = re.match(r'par(\d)$', member)
            if r is None or not m: raise CalcError('missile reference %s.%s not evaluated' % (ref, member))
            return [num(r.get('Param%s' % m.group(1)))]
        row = self.skill_by_name.get(ref)
        if not row: raise CalcError('unknown skill %s' % ref)
        sid = int(row['*Id']); self.skill_closure.add(sid)
        m = re.match(r'pa(?:r)?(\d+)$', member)
        if m:
            p = self.params(row, 'skill'); i = int(m.group(1))
            return [p[i - 1] if 1 <= i <= len(p) else 0]
        lf = self.level_formula(row, 'skill', member, '@%d:lvl' % sid)
        if lf: return lf
        if member in VARS: return ['@%d:%s' % (sid, member)]
        raise CalcError('unknown skill member %s' % member)


def collect_skills(db):

    rows = {int(r['*Id']): r for r in db.skills if r.get('*Id', '').isdigit()}
    want = {sid for sid, r in rows.items() if r.get('charclass')} | {0}
    monsters = {r['Id'].lower(): r for r in db.t['monstats'][1] if r.get('Id')}
    todo = list(want)
    while todo:
        sid = todo.pop(); r = rows.get(sid)
        if r is None: continue
        refs = set()
        desc = db.desc.get(r.get('skilldesc', ''), {})
        expressions = [r.get(f, '') for f in CALC_FIELDS] + [desc.get('ddam calc1', ''), desc.get('ddam calc2', '')]
        pet = monsters.get(r.get('summon', '').lower(), {})
        for k in range(1, 9):
            name = pet.get('Skill%d' % k, '')
            if name in db.skill_by_name: refs.add(int(db.skill_by_name[name]['*Id']))
        for expression in expressions:
            refs |= {int(db.skill_by_name[n]['*Id']) for n in re.findall(r"skill\('([^']*)'", expression) if n in db.skill_by_name}
        for k in range(1, 6):
            n = r.get('sumskill%d' % k, '')
            if n in db.skill_by_name: refs.add(int(db.skill_by_name[n]['*Id']))
        for m in reach_missiles(db, r):
            mr = db.miss_by_name[m]
            for f in MISS_CALC_FIELDS:
                refs |= {int(db.skill_by_name[n]['*Id']) for n in re.findall(r"skill\('([^']*)'", mr.get(f, '')) if n in db.skill_by_name}
            n = mr.get('Skill', '')
            if n in db.skill_by_name: refs.add(int(db.skill_by_name[n]['*Id']))
        for s in refs - want:
            want.add(s); todo.append(s)
    return {sid: rows[sid] for sid in sorted(want) if sid in rows}


def reach_missiles(db, srow):

    seen, todo = [], []
    d = db.desc.get(srow.get('skilldesc', ''), {})
    for c in ('srvmissile', 'srvmissilea', 'srvmissileb', 'srvmissilec'):
        if srow.get(c): todo.append(srow[c])
    for c in ('descmissile1', 'descmissile2', 'descmissile3'):
        if d.get(c): todo.append(d[c])
    if num(srow.get('srvdofunc')) == 149:
        todo.extend('necromage%d' % i for i in range(1, 5))
    while todo:
        m = todo.pop()
        if m in seen or m not in db.miss_by_name: continue
        seen.append(m); r = db.miss_by_name[m]
        for c in ('SubMissile1', 'SubMissile2', 'SubMissile3', 'HitSubMissile1', 'HitSubMissile2', 'HitSubMissile3', 'HitSubMissile4', 'ExplosionMissile'):
            if r.get(c): todo.append(r[c])
    return seen


def element(code):
    code = (code or '').strip().lower()
    return ELEMENTS.index(code) if code in ELEMENTS else (-1 if not code else 6)



POTIONS = (('hp1', 'hpot'), ('hp2', 'hpot'), ('hp3', 'hpot'), ('hp4', 'hpot'), ('hp5', 'hpot'), ('mp1', 'mpot'), ('mp2', 'mpot'), ('mp3', 'mpot'),
           ('mp4', 'mpot'), ('mp5', 'mpot'), ('rvs', 'rpot'), ('rvl', 'rpot'))




HAND_COLS = (('itypea', ('itypea1', 'itypea2', 'itypea3')), ('etypea', ('etypea1', 'etypea2')),
             ('itypeb', ('itypeb1', 'itypeb2', 'itypeb3')), ('etypeb', ('etypeb1', 'etypeb2')))
HAND_ROOTS = ('weap', 'shld')


def hand_types(itemtypes):

    head, rows = itemtypes
    need(head, ('Code', 'Equiv1', 'Equiv2'), 'itemtypes')
    parents = {r['Code']: [r[k] for k in ('Equiv1', 'Equiv2') if r.get(k)] for r in rows if r.get('Code')}

    def supers(code, path=()):
        if code in path: raise Refuse('item type cycle at %s' % code)
        found = set()
        for parent in parents[code]:
            if parent not in parents: raise Refuse('item type %s names unknown type %s' % (code, parent))
            found |= {parent} | supers(parent, path + (code,))
        return found
    hand = {code for code in parents if code in HAND_ROOTS or supers(code) & set(HAND_ROOTS)}
    if len(hand) > 64: raise Refuse('%d hand item types' % len(hand))
    return sorted(((code, sorted(supers(code) & hand)) for code in hand), key=lambda t: (len(t[1]), t[0]))


def hand_requirement(row, cols, known):
    codes = [row[c] for c in cols if row.get(c)]
    for code in codes:
        if code not in known: raise Refuse('skill %s names item type %s outside the hand types' % (row['skill'], code))
    return ','.join(codes) or '-'


def write_potions(out, misc):

    h, rows = misc; need(h, ('code', 'type', 'stat1', 'calc1', 'stat2', 'calc2', 'len'), 'misc')
    by_code = {r['code']: r for r in rows if r.get('code')}
    out.append('cols\tpotion\tcode\tkind\tlife\tmana\tlen')
    for code, kind in POTIONS:
        r = by_code.get(code)
        if not r or r['type'] != kind: raise Refuse('misc lacks potion %s of type %s' % (code, kind))
        calc1, calc2, length = num(r['calc1']), num(r['calc2']), num(r['len'])
        if kind == 'hpot' and r['stat1'] == 'hpregen' and not r['stat2'] and 0 < calc1 <= 100000 and 0 < length <= 1000:
            out.append('potion\t%s\t0\t%d\t0\t%d' % (code, calc1, length))
        elif kind == 'mpot' and r['stat1'] == 'manarecovery' and not r['stat2'] and 0 < calc1 <= 100000 and 0 < length <= 1000:
            out.append('potion\t%s\t1\t0\t%d\t%d' % (code, calc1, length))
        elif kind == 'rpot' and r['stat1'] == 'hitpoints' and r['stat2'] == 'mana' and 0 < calc1 <= 100 and 0 < calc2 <= 100 and not length:
            out.append('potion\t%s\t2\t%d\t%d\t0' % (code, calc1, calc2))
        else: raise Refuse('potion %s has an unexpected effect (%s %s, %s %s, len %s)' % (code, r['stat1'], r['calc1'], r['stat2'], r['calc2'], r['len']))


def write_set(out, db, codes, base_levels):

    t, anim = db.t, db.anim
    out.append('set\t%s' % db.name)

    h, dl = t['difficultylevels']
    dcols = ('ResistPenalty', 'MonsterSkillBonus', 'MonsterFreezeDivisor', 'MonsterColdDivisor', 'LifeStealDivisor', 'ManaStealDivisor',
             'UniqueDamageBonus', 'ChampionDamageBonus', 'PlayerDamagePercentVSPrimeEvil', 'MercenaryDamagePercentVSBoss',
             'PrimeEvilDamagePercentVSPet', 'PrimeEvilDamagePercentVSMercenary', 'StaticFieldMin', 'MonsterCEDamagePercent')
    need(h, dcols, 'difficultylevels')
    if len(dl) != 3: raise Refuse('difficultylevels has %d rows' % len(dl))
    out.append('cols\tdiff\tidx\t' + '\t'.join(dcols))
    for i, r in enumerate(dl): out.append('diff\t%d\t%s' % (i, '\t'.join(str(num(r[c])) for c in dcols)))

    h, cs = t['charstats']; need(h, ('class', 'ToHitFactor', 'BlockFactor', 'MinimumCastingDelay', 'ManaRegen', 'LifePerVitality', 'ManaPerMagic',
                                     'HealthPotionPercent', 'ManaPotionPercent'), 'charstats')
    h, pt = t['plrtype']; need(h, ('Name', 'Token'), 'plrtype')
    tokens = [r['Token'] for r in pt if r.get('Token')]
    cls = [r for r in cs if r.get('class') and r['class'] != 'Expansion']
    if not (len(cls) == len(codes) == len(tokens)): raise Refuse('class tables disagree: %d/%d/%d' % (len(cls), len(codes), len(tokens)))
    out.append('cols\tclass\tidx\tcode\ttoken\tToHitFactor\tBlockFactor\tMinimumCastingDelay\tManaRegen\tLifePerVitality\tManaPerMagic'
               '\tHealthPotionPercent\tManaPotionPercent')
    for i, r in enumerate(cls):
        if not (0 < num(r['HealthPotionPercent']) <= 1000 and 0 < num(r['ManaPotionPercent']) <= 1000): raise Refuse('potion factors of %s' % r['class'])
        out.append('class\t%d\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d' % (i, codes[i], tokens[i], num(r['ToHitFactor']), num(r['BlockFactor']),
                   num(r['MinimumCastingDelay']), num(r['ManaRegen']), num(r['LifePerVitality']), num(r['ManaPerMagic']),
                   num(r['HealthPotionPercent']), num(r['ManaPotionPercent'])))
    write_potions(out, t['misc'])

    out.append('cols\tanim\ttoken\tmode\twclass\tframes\tspeed\taction')
    n_anim = 0
    for name in sorted(anim):
        if len(name) >= 5 and name[:2] in tokens + ['40', 'TG'] and name[2:4] in PLAYER_MODES:
            f, s, a = anim[name]; out.append('anim\t%s\t%s\t%s\t%d\t%d\t%d' % (name[:2], name[2:4], name[4:], f, s, a)); n_anim += 1
    if n_anim < 100: raise Refuse('AnimData has only %d player animations' % n_anim)

    out.append('cols\tstat\tname\tid')
    for n in sorted(db.stat_ids):
        if n in db.stat_ids: out.append('stat\t%s\t%d' % (n, db.stat_ids[n]))
    h, st = t['states']; need(h, ('state', '*ID', 'sunderfull', 'sunder-res-reduce'), 'states')
    out.append('cols\tstate\tname\tid\tsunderfull\tsunderresreduce')
    for r in st:
        if r.get('state'): out.append('state\t%s\t%d\t%d\t%d' % (r['state'], num(r['*ID']), num(r['sunderfull']), num(r['sunder-res-reduce'])))
    types = hand_types(t['itemtypes'])
    out.append('cols\titype\tcode\tsupers')
    for code, sup in types: out.append('itype\t%s\t%s' % (code, ','.join(sup) or '-'))

    skills = collect_skills(db)
    miss_used = []
    for r in skills.values():
        for m in reach_missiles(db, r):
            if m not in miss_used: miss_used.append(m)

    for m in miss_used:
        n = db.miss_by_name[m].get('Skill', '')
        if n in db.skill_by_name and int(db.skill_by_name[n]['*Id']) not in skills: skills[int(db.skill_by_name[n]['*Id'])] = db.skill_by_name[n]
    miss_idx = {m: int(db.miss_by_name[m]['*ID']) for m in miss_used}
    mon_ids = monster_index(t)
    scol = ['id', 'name', 'cls', 'fam', 'descdam', 'srvstfunc', 'srvdofunc', 'anim', 'seqnum', 'aura', 'passive', 'summon', 'pettype', 'aurastate', 'localdelay',
            'globaldelay', 'mana', 'lvlmana', 'minmana', 'manashift', 'ToHit', 'LevToHit', 'HitShift', 'SrcDam', 'MinDam', 'MaxDam',
            'MinLevDam', 'MaxLevDam', 'EType', 'EMin', 'EMax', 'EMinLev', 'EMaxLev', 'ELen', 'ELevLen', 'Kick', 'periodic', 'perdelay', 'alwayshit',
            'UseAttackRate', 'range', 'requiresweapon', 'reqlevel', 'maxlvl', 'missile', 'descmissile', 'params', 'namekey', 'astats', 'pstats', 'roles', 'sumskills', 'passivestate', 'auratargetstate', 'hitfactor', 'frameburn', 'timed', 'burnmiss', 'burnfactor', 'unavailable', 'variants', 'area', 'weapsel'] + [name for name, _ in HAND_COLS]
    out.append('cols\tskill\t' + '\t'.join(scol))
    calc_lines, bad, n_class = [], [], 0
    known = {code for code, _ in types}
    for sid, r in skills.items():
        c = r.get('charclass', '')
        ci = codes.index(c) if c in codes else -1
        if ci >= 0 or sid == 0:
            fam = FAMILY.get(sid)
            if fam is None: raise Refuse('class skill %s has no family' % r['skill'])
            if not fam_parts(fam): raise Refuse('bad family %s' % fam)
            n_class += ci >= 0
        else:
            fam = '-'
        d = db.desc.get(r.get('skilldesc', ''), {})
        sm = r.get('srvmissile') or r.get('srvmissilea') or ''
        dm = d.get('descmissile1', '')
        st_ids = lambda cols: ','.join(str(db.stat_ids.get(r.get(k, ''), -1)) if r.get(k) else '-' for k in cols)
        vals = [sid, r['skill'].replace('\t', ' '), ci, fam, num(d.get('descdam')), num(r['srvstfunc']), num(r['srvdofunc']), r['anim'] or '-', num(r['seqnum']),
                num(r['aura']), num(r['passive']), mon_ids.get(r['summon'].lower(), -1), r['pettype'] or '-', r['aurastate'] or '-', num(r['localdelay']),
                num(r['globaldelay']), num(r['mana']), num(r['lvlmana']), num(r['minmana']), num(r['manashift'], 8), num(r['ToHit']),
                num(r['LevToHit']), num(r['HitShift'], 8), num(r['SrcDam']), num(r['MinDam']), num(r['MaxDam']),
                ','.join(str(num(r['MinLevDam%d' % i])) for i in range(1, 6)), ','.join(str(num(r['MaxLevDam%d' % i])) for i in range(1, 6)),
                element(r['EType']), num(r['EMin']), num(r['EMax']), ','.join(str(num(r['EMinLev%d' % i])) for i in range(1, 6)),
                ','.join(str(num(r['EMaxLev%d' % i])) for i in range(1, 6)), num(r['ELen']), ','.join(str(num(r['ELevLen%d' % i])) for i in range(1, 4)),
                num(r['Kick']), num(r['periodic']), num(r['perdelay']), num(r['alwayshit']), num(r['UseAttackRate']), (r['range'] or '-').strip() or '-',
                num(r['requiresweapon']), num(r['reqlevel']), num(r['maxlvl']), miss_idx.get(sm, -1), miss_idx.get(dm, -1),
                ','.join(str(p) for p in db.params(r, 'skill')), d.get('str name') or '-',
                st_ids(['aurastat%d' % i for i in range(1, 7)]), st_ids(['passivestat%d' % i for i in range(1, 15)]), roles(r), ','.join(str(int(db.skill_by_name[r[k]]['*Id'])) if r.get(k) in db.skill_by_name else '-1' for k in ('sumskill1', 'sumskill2', 'sumskill3', 'sumskill4', 'sumskill5')), r.get('passivestate') or '-', r.get('auratargetstate') or r.get('aurastate') or '-',
                HIT_FACTORS.get(sid, 100), FRAME_BURNS.get(sid, 0), int(sid in TIMED_DAMAGE),
                miss_idx.get('meteorfire', -1) if sid == BURNING_IMPACT else -1, 100 if sid == BURNING_IMPACT else 0,
                0, ','.join(str(miss_idx['necromage%d' % i]) for i in range(1, 5)) if num(r['srvdofunc']) == 149 else '-1,-1,-1,-1',
                int(bool(area_reasons(db, r))), num(r.get('weapsel', ''))] + [hand_requirement(r, cols, known) for _, cols in HAND_COLS]
        out.append('skill\t' + '\t'.join(str(v) for v in vals))
        fields = [(f, r.get(f, '')) for f in CALC_FIELDS] + [('ddam1', d.get('ddam calc1', '')), ('ddam2', d.get('ddam calc2', ''))]
        if sid in TIMED_DAMAGE: fields.append(('strikeperiod', TIMED_DAMAGE[sid]))
        if sid == BURNING_IMPACT: fields.append(('burnlen', 'ln34'))
        for f, e in fields:
            if not e.strip(): continue
            try: pf = db.comp.compile(('skill', r), e)
            except CalcError as err:
                pf = '?'
                if (ci >= 0 or sid == 0) and f in DAMAGE_FIELDS: bad.append('%s.%s: %s' % (r['skill'], f, err))
            calc_lines.append('calc\t%d\t%s\t%s' % (sid, f, pf))
    per_class = {}
    for r in skills.values():
        if r.get('charclass') in codes: per_class[r['charclass']] = per_class.get(r['charclass'], 0) + 1
    if any(v != 30 for v in per_class.values()) or (db.name == 'rotw' and len(per_class) != len(codes)):
        raise Refuse('class skills per class %s' % per_class)
    if bad: raise Refuse('untranslatable damage formulas: %s' % '; '.join(bad[:5]))
    out.append('cols\tcalc\tskill\tfield\tpostfix')
    out.extend(calc_lines)

    mcol = ['id', 'name', 'Vel', 'NextHit', 'NextDelay', 'SrcDamage', 'HitShift', 'MinDamage', 'MaxDamage', 'MinLevDam', 'MaxLevDam', 'EType',
            'EMin', 'EMax', 'EMinLev', 'EMaxLev', 'ELen', 'ELevLen', 'skill', 'sub', 'hitsub', 'explosion', 'params', 'sHitPar2', 'dParam1', 'typefilter',
            'ApplyMastery', 'SrcMissDmg', 'MissileSkill', 'pSrvDoFunc', 'InitSteps', 'Accel', 'MaxVel']
    out.append('cols\tmiss\t' + '\t'.join(mcol))
    mcalc = []
    for m in miss_used:
        r = db.miss_by_name[m]
        g = lambda k: num(r.get(k))
        lst = lambda pre, n: ','.join(str(g('%s%d' % (pre, i))) for i in range(1, n + 1))
        sk = r.get('Skill', '')
        refs = lambda cols: ','.join(str(miss_idx.get(r.get(c, ''), -1)) for c in cols)
        vals = [miss_idx[m], m.replace('\t', ' '), g('Vel'), g('NextHit'), g('NextDelay'), g('SrcDamage'), g('HitShift'), g('MinDamage'),
                g('MaxDamage'), lst('MinLevDam', 5), lst('MaxLevDam', 5), element(r.get('EType')), g('EMin'), g('EMax'), lst('MinELev', 5),
                lst('MaxELev', 5), g('ELen'), lst('ELevLen', 3), int(db.skill_by_name[sk]['*Id']) if sk in db.skill_by_name else -1,
                refs(('SubMissile1', 'SubMissile2', 'SubMissile3')), refs(('HitSubMissile1', 'HitSubMissile2', 'HitSubMissile3', 'HitSubMissile4')),
                miss_idx.get(r.get('ExplosionMissile', ''), -1), ','.join(str(g('Param%d' % i)) for i in range(1, 6)), g('sHitPar2'), g('dParam1'), int(m == 'holybolt'),
                g('ApplyMastery'), g('SrcMissDmg'), g('MissileSkill'), g('pSrvDoFunc'), g('InitSteps'), g('Accel'), g('MaxVel')]
        out.append('miss\t' + '\t'.join(str(v) for v in vals))
        for f in MISS_CALC_FIELDS:
            e = r.get(f, '')
            if not e.strip(): continue
            try: pf = db.comp.compile(('miss', r), e)
            except CalcError: pf = '?'
            mcalc.append('mcalc\t%d\t%s\t%s' % (miss_idx[m], f, pf))
    out.append('cols\tmcalc\tmissile\tfield\tpostfix')
    out.extend(mcalc)
    write_monsters(out, db, skills, mon_ids, base_levels)
    return n_class, len(miss_used)


def runtime_indices(table, key, reference):
    header, rows = table
    need(header, (key, reference), key)
    indices = {}
    for row in rows:
        name = row.get(key, '')
        if name == 'Expansion':
            continue
        index = row.get(reference, '')
        if not name or name.lower() in indices or not index.isdigit() or int(index) != len(indices):
            raise Refuse('Invalid runtime index for %s: %s' % (key, name))
        indices[name.lower()] = len(indices)
    return indices


def monster_index(t):
    return runtime_indices(t['monstats'], 'Id', '*hcIdx')


def damaging_skill(db, name):
    row = db.skill_by_name.get(name, {})
    if name == 'Healing Vortex': return False
    if any(num(row.get(k)) for k in ('SrcDam', 'MinDam', 'MaxDam', 'EMin', 'EMax')): return True

    if num(row.get('srvdofunc')) in (52, 55, 83, 131, 141, 150): return True
    for missile in reach_missiles(db, row):
        m = db.miss_by_name[missile]
        if any(num(m.get(k)) for k in ('SrcDamage', 'MinDamage', 'MaxDamage', 'EMin', 'EMax')): return True
    return False


def write_monsters(out, db, skills, mon_ids, base_levels):
    t, anim = db.t, db.anim
    h, ms = t['monstats']
    need(h, ('Id', 'NameStr', 'Code', 'enabled', 'killable', 'boss', 'primeevil', 'isMelee', 'rangedtype', 'lUndead', 'hUndead', 'demon', 'noRatio',
             'ShieldBlockOverride', 'Crit', 'DamageRegen', 'nopreventmonsterheal', 'MinGrp', 'MaxGrp', 'SkillDamage', 'El1Type', 'El1Mode',
             'El2Type', 'El2Mode', 'El3Type', 'El3Mode', 'isSpawn', 'Level', 'Level(N)', 'Level(H)', 'minHP', 'maxHP', 'MinHP(N)', 'MaxHP(N)', 'MinHP(H)',
             'MaxHP(H)', 'AC', 'A1MinD', 'A1MaxD', 'A1TH', 'A2MinD', 'A2MaxD', 'A2TH', 'ToBlock', 'Drain', 'coldeffect', 'aidel', 'ResDm', 'ResMa',
             'ResFi', 'ResLi', 'ResCo', 'ResPo') + tuple('El%dPct' % k for k in (1, 2, 3)), 'monstats')
    h, m2 = t['monstats2']; need(h, ('Id', 'BaseW', 'SH', 'critter'), 'monstats2')
    m2 = {r['Id']: r for r in m2}
    h, ml = t['monlvl']; need(h, ('Level', 'L-HP', 'L-HP(N)', 'L-HP(H)', 'L-AC', 'L-AC(N)', 'L-AC(H)', 'L-TH', 'L-TH(N)', 'L-TH(H)', 'L-DM', 'L-DM(N)', 'L-DM(H)'), 'monlvl')
    lvls = {num(r['Level'], -1): r for r in ml if r.get('Level', '').strip().isdigit()}
    if max(lvls) < 110: raise Refuse('monlvl stops at %d' % max(lvls))
    out.append('cols\tmonlvl\tlevel\thp\thpN\thpH\tac\tacN\tacH\tth\tthN\tthH\tdm\tdmN\tdmH')
    for L in sorted(lvls):
        r = lvls[L]
        out.append('monlvl\t%d\t%s' % (L, '\t'.join(str(num(r[c + s])) for c in ('L-HP', 'L-AC', 'L-TH', 'L-DM') for s in DIFF_SUFFIX)))
    h, um = t['monumod']; need(h, ('constants',), 'monumod')
    out.append('cols\tuconst\tidx\tvalue')
    for i, r in enumerate(um): out.append('uconst\t%d\t%d' % (i, num(r.get('constants'), -1)))
    out.append('cols\tumod\tname\tid')
    for r in um:
        if r.get('uniquemod') and r.get('id', '').isdigit(): out.append('umod\t%s\t%s' % (r['uniquemod'], r['id']))
    h, lv = t['levels']; need(h, ('Id', 'Act', 'Name', 'LevelName', 'QuestFlagEx', 'MonLvlEx', 'MonLvlEx(N)', 'MonLvlEx(H)', 'mon1', 'nmon1'), 'levels')
    areas = {}
    for r in lv:
        if not r.get('Id', '').isdigit(): continue
        mons = [r.get('mon%d' % i, '') for i in range(1, 26)]
        nmons = [r.get('nmon%d' % i, '') for i in range(1, 26)]
        areas[int(r['Id'])] = (r, [x for x in mons if x], [x for x in nmons if x])

    h, su = t['superuniques']; need(h, ('Superunique', 'Name', 'Class', 'Mod1', 'Mod2', 'Mod3', 'MinGrp', 'MaxGrp', 'TC'), 'superuniques')
    su_indices = runtime_indices(t['superuniques'], 'Superunique', 'hcIdx')
    boss_keys = {i: k for k, i in mon_ids.items() if i in BOSSES}
    want = set(boss_keys.values())
    for a, (r, mons, nmons) in areas.items(): want |= set(mons) | set(nmons)
    want |= {s['Class'] for s in su if s.get('Class')}
    for r in skills.values():
        if r.get('summon'): want.add(r['summon'].lower())
    h, mp = t['monpet']; need(h, ('monster',), 'monpet')
    pets = {r['monster'] for r in mp if r.get('monster')}
    want = {w for w in want if w in mon_ids}
    mcol = ['idx', 'id', 'namekey', 'flags', 'lvl', 'lvlN', 'lvlH', 'el', 'crit', 'regen', 'grp', 'a1', 'a2', 'skilldamage', 'skills']
    out.append('cols\tmon\t' + '\t'.join(mcol))
    dcol = ['idx', 'diff', 'hpmin', 'hpmax', 'ac', 'a1min', 'a1max', 'a1th', 'a2min', 'a2max', 'a2th', 'res', 'block', 'drain', 'coldeffect',
            'aidel', 'elpct', 'elmin', 'elmax', 'eldur', 's1']
    rows = []
    base_ms = None
    for r in ms:
        if r.get('Id', '').lower() not in want: continue
        i = mon_ids[r['Id'].lower()]; x2 = m2.get(r['Id'], {})
        undead = r['lUndead'] == '1' or r['hUndead'] == '1'
        block_ok = r['ShieldBlockOverride'] == '1' or (r['ShieldBlockOverride'] != '2' and x2.get('SH') == '1')
        flags = ''.join(f for f, on in (('b', r['boss'] == '1'), ('p', r['primeevil'] == '1'), ('m', r['isMelee'] == '1'), ('r', r['rangedtype'] == '1'),
                                        ('u', undead), ('d', r['demon'] == '1'), ('n', r['noRatio'] == '1'), ('s', block_ok), ('k', r['killable'] == '1'),
                                        ('e', r['enabled'] == '1'), ('h', r['nopreventmonsterheal'] == '1'), ('c', x2.get('critter') == '1'),
                                        ('t', r['Id'] in pets), ('x', any(damaging_skill(db, r.get('Skill%d' % k, '')) for k in range(1, 9)))) if on) or '-'
        bw = (x2.get('BaseW') or 'HTH').upper()
        def an(mode):
            a = anim.get((r['Code'] + mode + bw).upper())
            return '%d,%d,%d' % a if a else '-'
        el = ';'.join('%d,%s' % (element(r['El%dType' % k]) if r['El%dType' % k] else -1, r['El%dMode' % k] or '-') for k in (1, 2, 3))
        sd = r.get('SkillDamage', '')
        rows.append('mon\t' + '\t'.join(str(v) for v in [i, r['Id'], r['NameStr'] or '-', flags, num(r['Level']), num(r['Level(N)']), num(r['Level(H)']),
                    el, num(r['Crit']), num(r['DamageRegen']), '%d,%d' % (num(r['MinGrp']), num(r['MaxGrp'])), an('A1'), an('A2'),
                    int(db.skill_by_name[sd]['*Id']) if sd in db.skill_by_name else -1,
                    ','.join(str(int(db.skill_by_name[r['Skill%d' % k]]['*Id'])) if r.get('Skill%d' % k) in db.skill_by_name else '-1' for k in range(1, 9))]))
        for d, s in enumerate(DIFF_SUFFIX):
            hpn = ('minHP', 'maxHP') if d == 0 else ('MinHP' + s, 'MaxHP' + s)
            g = lambda c: num(r.get(c + s))
            res = ','.join(str(g('Res' + e)) for e in ('Dm', 'Fi', 'Li', 'Co', 'Po', 'Ma'))
            rows.append('mond\t' + '\t'.join(str(v) for v in [i, d, num(r[hpn[0]]), num(r[hpn[1]]), g('AC'), g('A1MinD'), g('A1MaxD'), g('A1TH'),
                        g('A2MinD'), g('A2MaxD'), g('A2TH'), res, g('ToBlock'), g('Drain'), g('coldeffect'), g('aidel'),
                        ','.join(str(g('El%dPct' % k)) for k in (1, 2, 3)), ','.join(str(g('El%dMinD' % k)) for k in (1, 2, 3)),
                        ','.join(str(g('El%dMaxD' % k)) for k in (1, 2, 3)), ','.join(str(g('El%dDur' % k)) for k in (1, 2, 3)), ','.join(str(g(k)) for k in ('S1MinD', 'S1MaxD', 'S1TH'))]))
    out.extend(x for x in rows if x.startswith('mon\t'))
    out.append('cols\tmond\t' + '\t'.join(dcol))
    out.extend(x for x in rows if x.startswith('mond\t'))

    out.append('cols\tarea\tid\tact\tnamekey\tmlvl\tmlvlN\tmlvlH\trotw')
    spawn = []
    for a, (r, mons, nmons) in sorted(areas.items()):
        if a == 0: continue
        out.append('area\t%d\t%d\t%s\t%d\t%d\t%d\t%d' % (a, num(r['Act']) + 1, r['LevelName'] or '-', num(r['MonLvlEx']), num(r['MonLvlEx(N)']),
                   num(r['MonLvlEx(H)']), 0 if base_levels is None or r['Name'] in base_levels else 1))
        if mons: spawn.append('spawn\t%d\t0\t%s' % (a, ','.join(str(mon_ids[m]) for m in mons if m in mon_ids)))
        if nmons: spawn.append('spawn\t%d\t1\t%s' % (a, ','.join(str(mon_ids[m]) for m in nmons if m in mon_ids)))
    out.append('cols\tspawn\tarea\tlist\tmonsters')
    out.extend(spawn)

    out.append('cols\tsu\tidx\tnamekey\tmon\tmods\tgrp\tarea\tarea_guess\tact')
    for s in su:
        c = s.get('Class', '')
        if not c or c not in mon_ids or mon_ids[c] in BOSSES: continue
        i = su_indices[s['Superunique'].lower()]
        act = num(re.match(r'Act (\d)', s.get('TC', '')).group(1)) if re.match(r'Act (\d)', s.get('TC', '')) else 0
        area, guess = SU_AREA.get(i, 0), 0
        if not area:
            guess = 1
            cand = [(areas[a][0], a) for a in areas if c in areas[a][1] + areas[a][2] and (act == 0 or num(areas[a][0]['Act']) + 1 == act)]
            area = max(cand, key=lambda x: num(x[0]['MonLvlEx(H)']))[1] if cand else 0
        if area in areas and not act: act = num(areas[area][0]['Act']) + 1
        out.append('su\t%d\t%s\t%d\t%s\t%d,%d\t%d\t%d\t%d' % (i, s['Name'] or '-', mon_ids[c], ','.join(str(num(s['Mod%d' % k])) for k in (1, 2, 3)),
                   num(s['MinGrp']), num(s['MaxGrp']), area, guess, act))
    out.append('cols\tboss\tmon\tact\tkind\tarea')
    for i, b in sorted(boss_keys.items(), key=lambda kv: kv[1]):
        act, kind, area = BOSSES[i]
        if b in want and (kind != 'ancient' or db.name == 'rotw'): out.append('boss\t%d\t%d\t%s\t%d' % (i, act, kind, area))
    write_standard(out, db, ms, m2, lvls, areas)


def scaled(r, d, mlvl, lvls, col, mcol):

    v = num(r.get(col))
    if r.get('noRatio') == '1': return v
    L = lvls[max(0, min(mlvl, max(lvls)))]
    return v * num(L[mcol + DIFF_SUFFIX[d]]) // 100


def base(r, d, mlvl, lvls, col, mcol):

    v = num(r.get(col))
    if r.get('noRatio') != '1': return v
    return v * 100 / max(1, num(lvls[max(0, min(mlvl, max(lvls)))][mcol + DIFF_SUFFIX[d]]))




UBER_AREAS = (133, 134, 135, 136)


def typical_areas(areas):






    missing = [a for a in UBER_AREAS if a not in areas]
    if missing: raise Refuse('levels has no uber area %s' % missing)
    gates = {a: num(r['QuestFlagEx']) for a, (r, _, _) in areas.items()}
    final = max(gates.values())
    if sum(1 for g in gates.values() if g == final) > 1: raise Refuse('levels: the highest entry quest gates more than one area')
    return {a for a in areas if a not in UBER_AREAS and gates[a] != final}


def write_standard(out, db, ms, m2, lvls, areas):





    byid = {r['Id']: r for r in ms if r.get('Id')}
    typical = typical_areas(areas)
    cols = ['diff', 'act', 'n', 'mlvl', 'hp', 'ac', 'ar', 'dmin', 'dmax', 'elavg', 'elshare', 'res', 'immune', 'ranged', 'undead', 'demon', 'block',
            'cycle', 'regen', 'drain', 'elpct', 'lmin', 'lmax', 'bhp', 'bac', 'bar', 'bdmin', 'bdmax', 'bel']
    out.append('cols\tstd\t' + '\t'.join(cols))
    for d in range(3):
        per_act = {}
        for a, (r, mons, nmons) in areas.items():
            if a not in typical: continue
            lst = mons if d == 0 else nmons
            for mid in lst:
                m = byid.get(mid)
                if not m or m['boss'] == '1' or m['enabled'] != '1' or m['isSpawn'] != '1' or m2.get(mid, {}).get('critter') == '1': continue
                mlvl = num(m['Level']) if d == 0 else num(r['MonLvlEx' + DIFF_SUFFIX[d]])
                s = DIFF_SUFFIX[d]
                hpn = ('minHP', 'maxHP') if d == 0 else ('MinHP' + s, 'MaxHP' + s)
                hp = (scaled(m, d, mlvl, lvls, hpn[0], 'L-HP') + scaled(m, d, mlvl, lvls, hpn[1], 'L-HP')) / 2
                attack_el = [k for k in (1, 2, 3) if m['El%dType' % k] and m['El%dMode' % k] in ('', 'A1') and num(m['El%dPct%s' % (k, s)]) > 0]
                els = [(scaled(m, d, mlvl, lvls, 'El%dMinD%s' % (k, s), 'L-DM') + scaled(m, d, mlvl, lvls, 'El%dMaxD%s' % (k, s), 'L-DM')) / 2
                       for k in attack_el]
                bels = [(base(m, d, mlvl, lvls, 'El%dMinD%s' % (k, s), 'L-DM') + base(m, d, mlvl, lvls, 'El%dMaxD%s' % (k, s), 'L-DM')) / 2
                        for k in attack_el]
                pcts = [num(m['El%dPct%s' % (k, s)]) for k in (1, 2, 3)
                        if m['El%dType' % k] and m['El%dMode' % k] in ('', 'A1') and num(m['El%dPct%s' % (k, s)]) > 0]
                x2 = m2.get(mid, {})
                block_ok = m['ShieldBlockOverride'] == '1' or (m['ShieldBlockOverride'] != '2' and x2.get('SH') == '1')
                a1 = db.anim.get((m['Code'] + 'A1' + (x2.get('BaseW') or 'HTH')).upper())
                cyc = (-(-a1[0] * 256 // a1[1]) + num(m['aidel' + s])) if a1 and a1[1] else None
                e = dict(mlvl=mlvl, hp=hp, ac=scaled(m, d, mlvl, lvls, 'AC' + s, 'L-AC'), ar=scaled(m, d, mlvl, lvls, 'A1TH' + s, 'L-TH'),
                         dmin=scaled(m, d, mlvl, lvls, 'A1MinD' + s, 'L-DM'), dmax=scaled(m, d, mlvl, lvls, 'A1MaxD' + s, 'L-DM'),
                         el=sum(els) if els else None, elpct=statistics.median(pcts) if pcts else None, res=[num(m['Res%s%s' % (x, s)]) for x in ('Dm', 'Fi', 'Li', 'Co', 'Po', 'Ma')],
                         ranged=m['isMelee'] != '1', undead=m['lUndead'] == '1' or m['hUndead'] == '1', demon=m['demon'] == '1',
                         block=num(m['ToBlock' + s]) if block_ok else 0, cycle=cyc, regen=num(m['DamageRegen']), drain=num(m['Drain' + s]),
                         bhp=(base(m, d, mlvl, lvls, hpn[0], 'L-HP') + base(m, d, mlvl, lvls, hpn[1], 'L-HP')) / 2,
                         bac=base(m, d, mlvl, lvls, 'AC' + s, 'L-AC'), bar=base(m, d, mlvl, lvls, 'A1TH' + s, 'L-TH'),
                         bdmin=base(m, d, mlvl, lvls, 'A1MinD' + s, 'L-DM'), bdmax=base(m, d, mlvl, lvls, 'A1MaxD' + s, 'L-DM'), bel=sum(bels) if bels else None)
                per_act.setdefault(num(r['Act']) + 1, []).append(e)
        groups = [(0, [e for v in per_act.values() for e in v])] + sorted(per_act.items())
        for act, xs in groups:
            if not xs: continue
            med = lambda k: statistics.median([e[k] for e in xs if e[k] is not None]) if any(e[k] is not None for e in xs) else 0
            share = lambda f: round(100 * sum(1 for e in xs if f(e)) / len(xs))
            res, imm = [], []
            for k in range(6):
                v = [e['res'][k] for e in xs if e['res'][k] < 100]
                res.append(int(statistics.median(v)) if v else 0); imm.append(share(lambda e, k=k: e['res'][k] >= 100))
            out.append('std\t' + '\t'.join(str(v) for v in [d, act, len(xs), int(med('mlvl')), int(med('hp') + 0.5), int(med('ac')), int(med('ar')),
                       int(med('dmin')), int(med('dmax')), int(med('el')), share(lambda e: e['el'] is not None), ','.join(map(str, res)),
                       ','.join(map(str, imm)), share(lambda e: e['ranged']), share(lambda e: e['undead']), share(lambda e: e['demon']),
                       int(med('block')), int(med('cycle')), int(med('regen')), int(med('drain')), int(med('elpct')),
                       min(e['mlvl'] for e in xs), max(e['mlvl'] for e in xs)] + [round(100 * med(k)) for k in ('bhp', 'bac', 'bar', 'bdmin', 'bdmax', 'bel')]))


def build(read):
    raw = {p: read(p) for p in TABLES}
    sha = hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in TABLES)).hexdigest()
    if sha != TABLES_SHA: raise Refuse('tables fingerprint %s is not the validated %s' % (sha, TABLES_SHA))
    anim = animdata(raw[ANIMDATA])
    out = ['DM_SIM\t%d\t%s' % (FORMAT, sha)]
    stats = {}
    t_by = {}
    for set_name, prefix in SETS:
        t_by[set_name] = {n: table(raw[prefix + n + '.txt'].decode('utf-8', 'replace')) for n in NAMES}
    base_levels = {r['Name'] for r in t_by['lod']['levels'][1] if r.get('Name')}
    for set_name, prefix in SETS:
        t = t_by[set_name]
        h, isc = t['itemstatcost']; need(h, ('Stat', '*ID'), 'itemstatcost')
        sid = {r['Stat']: int(r['*ID']) for r in isc if r.get('Stat') and r.get('*ID', '').strip().lstrip('-').isdigit()}
        h, pc = t['playerclass']; need(h, ('Player Class', 'Code'), 'playerclass')
        codes = [r['Code'] for r in pc if r.get('Code')]
        db = Db(set_name, t, anim, sid)
        n_class, n_miss = write_set(out, db, codes, base_levels if set_name == 'rotw' else None)
        stats[set_name] = (n_class, n_miss)
    payload = '\n'.join(out[1:]) + '\n'
    out[0] += '\t%08x' % (zlib.crc32(payload.encode('utf-8')) & 0xffffffff)
    out.append('end\t%d' % (len(out) + 1))
    return '\n'.join(out) + '\n', stats


def write_data(read, dst):

    path = os.path.join(dst, 'sim.tsv')
    temporary = None
    try:
        text, stats = build(read)
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', newline='\n', dir=dst, prefix='.sim-', delete=False) as stream:
            temporary = stream.name
            stream.write(text)
        os.replace(temporary, path)
    except (Exception, SystemExit) as error:
        for old in (temporary, path):
            if old and os.path.exists(old): os.remove(old)
        print('Combat simulation: no data (%r)' % (error,))
        return False
    print('Combat simulation: sim.tsv, %s' % ', '.join('%s %d skills / %d missiles' % (k, v[0], v[1]) for k, v in stats.items()))
    return True


UI_TABLES = ('global/excel/weapons.txt', 'global/excel/armor.txt', 'hd/global/excel/desecratedzones.json', 'local/lng/strings/monsters.json', 'local/lng/strings/levels.json', 'local/lng/strings/skills.json',
             'local/lng/strings/item-names.json')
UI_TABLES_SHA = 'a0ed45cb5a0aac561e2bb03da4220157b1821fa819b76b6d07392954fff11919'

POTION_GROUP_NAMES = ('hp3', 'mp3', 'rvs')

GAME_LANGUAGES = ('enUS', 'deDE', 'esES', 'frFR', 'itIT', 'koKR', 'plPL', 'ruRU', 'zhCN', 'zhTW', 'esMX', 'jaJP', 'ptBR')

def game_names(row, what, strict=False):


    names = [untag(row.get(language)) if isinstance(row.get(language), str) else '' for language in GAME_LANGUAGES]
    if strict and not all(names): raise ValueError('Game strings lack a language for ' + what)
    return [name or names[0] for name in names]

def write_ui(read, dst):
    if hashlib.sha256(b"".join(hashlib.sha256(read(path)).digest() for path in UI_TABLES)).hexdigest() != UI_TABLES_SHA:
        raise ValueError("Combat UI data does not match the product")
    names = {}
    for file in ('monsters', 'levels', 'skills'):
        rows = json.loads(read('local/lng/strings/' + file + '.json').decode('utf-8-sig'))
        for row in rows:
            key = row['Key']
            names[key] = game_names(row, key)
    desc = {row['skilldesc']: row for row in csv.DictReader(io.StringIO(read('global/excel/skilldesc.txt').decode('utf-8-sig')), delimiter='\t')}
    for row in csv.DictReader(io.StringIO(read('global/excel/skills.txt').decode('utf-8-sig')), delimiter='\t'):
        key = desc.get(row.get('skilldesc'), {}).get('str name')
        if row.get('*Id', '').isdigit() and key in names:
            names['@skill:' + row['*Id']] = names[key]
    for row in csv.DictReader(io.StringIO(read('global/excel/levels.txt').decode('utf-8-sig')), delimiter='\t'):
        if row.get('Id', '').isdigit() and row.get('LevelName') in names:
            names['@area:' + row['Id']] = names[row['LevelName']]
    items = {row['Key']: row for row in json.loads(read('local/lng/strings/item-names.json').decode('utf-8-sig'))}
    for group, key in enumerate(POTION_GROUP_NAMES):
        row = items.get(key)
        if not row: raise ValueError('Item names lack potion ' + key)
        names['@potion:%d' % group] = game_names(row, key, True)
    zones = json.loads(read('hd/global/excel/desecratedzones.json').decode('utf-8-sig'))['desecrated_zones'][0]['game_difficulties']
    lines = ['DM_SIM_UI\t1']
    for version, key in (('lod', 'expansion'), ('rotw', 'rotw')):
        for diff, name in enumerate(('normal', 'nightmare', 'hell')):
            defaults = zones[key][name]['defaults']
            lines.append('tz\t%s\t%d\t%d\t%d\t%d' % (version, diff, defaults['bound_incl_min'], defaults['bound_incl_max'], defaults['boost_level']))
            for tier, herald in enumerate(defaults.get('herald_tiers', []), 1):
                lines.append('herald\t%s\t%d\t%d\t%d\t%d' % (version, diff, tier, herald['herald_health_boost_percent'], herald['herald_damage_boost_percent']))
    for version, prefix in SETS:
        rows = csv.DictReader(io.StringIO(read(prefix + 'monstats.txt').decode('utf-8-sig')), delimiter='\t')
        for row in rows:
            if row.get('CannotHerald') == '1':
                lines.append('exclude\t%s\t%s' % (version, row['Id']))
    weapon_lines = ['DM_SIM_WEAPONS\t2']
    rows = csv.DictReader(io.StringIO(read('global/excel/weapons.txt').decode('utf-8-sig')), delimiter='\t')
    for row in rows:
        if not row.get('code') or not row.get('wclass'): continue
        weapon_lines.append('%s\t%s\t%s\t%d\t0\t0' % (row['code'], row['wclass'].upper(), (row.get('2handedwclass') or row['wclass']).upper(), int(row.get('speed') or 0)))
    rows = csv.DictReader(io.StringIO(read('global/excel/armor.txt').decode('utf-8-sig')), delimiter='\t')
    for row in rows:
        if row.get('code'):
            weapon_lines.append('%s\t-\t-\t0\t%d\t%d' % (row['code'], int(row.get('mindam') or 0), int(row.get('maxdam') or 0)))
    weapon_lines.append('end\t%d'  % (len(weapon_lines) + 1))
    lines.append('end\t%d' % (len(lines) + 1))
    for file, text in (('sim-names.json', json.dumps(names, ensure_ascii=False, separators=(',', ':'))), ('sim-ui.tsv', '\n'.join(lines) + '\n'), ('sim-weapons.tsv', '\n'.join(weapon_lines) + '\n')):
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=dst, prefix='.sim-ui-', delete=False) as stream:
            stream.write(text)
            temporary = stream.name
        os.replace(temporary, os.path.join(dst, file))


def write(romfs, dst):
    sys.path.insert(0, HERE)
    import romfs_art
    read = lambda path: romfs_art.read(romfs, path)
    if not write_data(read, dst): return False
    write_ui(read, dst)
    return True


def read_dir(root):

    return lambda path: (pathlib.Path(root) / path).read_bytes()


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--sha':
        sys.path.insert(0, HERE); import romfs_art
        raw = {p: romfs_art.read(sys.argv[2], p) for p in TABLES}
        print(hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in TABLES)).hexdigest()); sys.exit(0)
    if len(sys.argv) == 4 and sys.argv[1] == '--dir':
        sys.exit(0 if write_data(read_dir(sys.argv[2]), sys.argv[3]) else 1)
    sys.exit(0 if write(sys.argv[1], sys.argv[2]) else 1)
