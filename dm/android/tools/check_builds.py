#!/usr/bin/env python3
"""构建指南校验：dm/android/builds/<id>.json 是我们自己写的指南（只有自己的话、游戏表里的键 / 编号与数字）。
  不带游戏文件（CI）：按 builds/schema.json 查结构，再查语义——两种语言都在、阶段不重叠、步骤按等级排、属性规则最后一条是「其余」、
  至少两个不同站点的来源、日期与链接格式。
  带游戏文件（本机，--romfs <RomFS> 或 --tables <已解出的表目录>，只读，什么都不写）：表必须正是 3.3.94012 的（指纹）；
  技能号 ↔ 键 ↔ 职业、前置与等级要求；逐级模拟加点（每级 1 点，任务点按下面写明的假设，另按「一个任务点都没有」的最坏情况再跑一遍），
  两种情况都不许走进死路；有任务点时每个阶段到它的末级必须走完；符文之语的键 / 底材类型 / 孔数 / 等级、暗金与套装的键与等级、
  佣兵（hireling.txt）、构建画像里的属性名（itemstatcost.txt），以及按具体底材算出的力量 / 敏捷要求。
  加点的规则（下一点给谁）与 App 的 BuildPlan.java 完全相同，两边共用 dm/android/tests/build_plan_cases.tsv 的用例。
用法：check_builds.py [--romfs <RomFS> | --tables <目录>] [--report] [指南文件 …]（缺省 = builds/ 下全部）；有错误时退出码 1。
Build guide validator: dm/android/builds/<id>.json are our own guides (only our own words, keys / ids from the game tables, and numbers).
  Without game files (CI): structure per builds/schema.json, then semantics -- both languages present, phases do not overlap, steps ordered by level,
  the last stat rule takes "the rest", at least two sources from different sites, date and link formats.
  With game files (locally, --romfs <RomFS> or --tables <directory of extracted tables>, read only, nothing is written): the tables must be exactly those
  of 3.3.94012 (fingerprint); skill id <-> key <-> class, prerequisites and level requirements; a per-level simulation of the skill points (1 per level,
  quest points by the assumption written below, and again for the worst case without any quest point) that must never run into a dead end;
  with quest points every phase must be complete by its last level; rune word keys / base types / sockets / levels, unique and set keys and levels,
  the mercenary (hireling.txt), the stat names of the build profile (itemstatcost.txt), and the strength / dexterity requirements of specific bases.
  The allocation rule (who gets the next point) is exactly that of the app's BuildPlan.java; both share the cases in dm/android/tests/build_plan_cases.tsv.
Usage: check_builds.py [--romfs <RomFS> | --tables <directory>] [--report] [guide files ...] (default = all of builds/); exit code 1 on errors."""
import argparse
import csv
import hashlib
import io
import json
import os
import re
import sys
from datetime import date
from urllib.parse import urlparse

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BUILDS = os.path.join(DM, 'android', 'builds')
SCHEMA = os.path.join(BUILDS, 'schema.json')
VERSION = '3.3.94012'
CLASSES = ('ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass', 'war')
STATS = ('str', 'dex', 'vit', 'ene')
STAT_COLUMN = {'str': 'str', 'dex': 'dex', 'vit': 'vit', 'ene': 'int'}


QUESTS = ((8, 1, 'Den of Evil (Normal)'), (18, 1, 'Radament (Normal)'), (27, 2, 'Izual (Normal)'),
          (40, 1, 'Den of Evil (Nightmare)'), (45, 1, 'Radament (Nightmare)'), (52, 2, 'Izual (Nightmare)'),
          (70, 1, 'Den of Evil (Hell)'), (72, 1, 'Radament (Hell)'), (76, 2, 'Izual (Hell)'))
SKILL_MAX = 20

NEXT, UPCOMING, SAVE, WAIT, BLOCKED, DONE = 'NEXT', 'UPCOMING', 'SAVE', 'WAIT', 'BLOCKED', 'DONE'


OK, R_LEVEL, R_PREREQ, R_CAP, R_MAX, R_DATA = 0, 2, 3, 5, 6, 7
PERMANENT = (R_CAP, R_MAX, R_DATA)
TABLES = ('skills.txt', 'skilldesc.txt', 'charstats.txt', 'uniqueitems.txt', 'setitems.txt', 'Runes.txt', 'armor.txt', 'weapons.txt', 'misc.txt',
          'itemtypes.txt', 'itemstatcost.txt', 'hireling.txt')
TABLES_SHA = '6b195791e092fb41c860f21011eb6b223d6df304ecfb5d2b53acf8d2cee7e7d0'
SLOT_TYPES = {'weapon': ('weap',), 'offhand': ('shld', 'weap'), 'switch': ('weap', 'shld'), 'helm': ('helm',), 'armor': ('tors',), 'gloves': ('glov',),
              'belt': ('belt',), 'boots': ('boot',), 'amulet': ('amul',), 'ring': ('ring',), 'charm': ('char',)}



def _is_type(v, t):
    if t == 'integer': return isinstance(v, int) and not isinstance(v, bool)
    if t == 'number': return isinstance(v, (int, float)) and not isinstance(v, bool)
    return {'string': str, 'boolean': bool, 'object': dict, 'array': list, 'null': type(None)}[t] is type(v) or (t == 'object' and isinstance(v, dict))


def schema_errors(v, s, root, path='$'):
    """v 按 s 查，返回错误列表 | check v against s, returns a list of errors"""
    if '$ref' in s:
        node = root
        for part in s['$ref'].lstrip('#/').split('/'): node = node[part]
        return schema_errors(v, node, root, path)
    out = []
    if 'const' in s and (v != s['const'] or type(v) is not type(s['const'])): return ['%s: muss %r sein | must be %r' % (path, s['const'], s['const'])]
    if 'enum' in s and not any(v == e and type(v) is type(e) for e in s['enum']): return ['%s: %r nicht erlaubt | not allowed (%s)' % (path, v, s['enum'])]
    if 'oneOf' in s:
        ok = [i for i, sub in enumerate(s['oneOf']) if not schema_errors(v, sub, root, path)]
        return [] if len(ok) == 1 else ['%s: passt nicht genau zu einer Form | does not match exactly one form' % path]
    if 'type' in s:
        types = s['type'] if isinstance(s['type'], list) else [s['type']]
        if not any(_is_type(v, t) for t in types): return ['%s: Typ %s erwartet | type %s expected' % (path, '/'.join(types), '/'.join(types))]
    if isinstance(v, str):
        if len(v) < s.get('minLength', 0): out.append('%s: zu kurz | too short' % path)
        if len(v) > s.get('maxLength', 1 << 30): out.append('%s: zu lang (%d > %d) | too long' % (path, len(v), s['maxLength']))
        if 'pattern' in s and not re.search(s['pattern'], v): out.append('%s: Form %r falsch | wrong form %r' % (path, v, v))
    if _is_type(v, 'number'):
        if v < s.get('minimum', float('-inf')) or v > s.get('maximum', float('inf')): out.append('%s: %r außerhalb | out of range' % (path, v))
    if isinstance(v, list):
        if len(v) < s.get('minItems', 0) or len(v) > s.get('maxItems', 1 << 30): out.append('%s: Anzahl %d falsch | wrong count' % (path, len(v)))
        if s.get('uniqueItems') and len({json.dumps(x, sort_keys=True) for x in v}) != len(v): out.append('%s: doppelte Einträge | duplicate entries' % path)
        if 'items' in s:
            for i, x in enumerate(v): out += schema_errors(x, s['items'], root, '%s[%d]' % (path, i))
    if isinstance(v, dict):
        props = s.get('properties', {})
        for k in s.get('required', []):
            if k not in v: out.append('%s: %s fehlt | %s missing' % (path, k, k))
        if len(v) < s.get('minProperties', 0) or len(v) > s.get('maxProperties', 1 << 30): out.append('%s: Anzahl Felder falsch | wrong field count' % path)
        extra = s.get('additionalProperties', True)
        for k, x in v.items():
            if k in props: out += schema_errors(x, props[k], root, '%s.%s' % (path, k))
            elif extra is False: out.append('%s: unbekanntes Feld %s | unknown field %s' % (path, k, k))
            elif isinstance(extra, dict): out += schema_errors(x, extra, root, '%s.%s' % (path, k))
    return out



MAX_LEVEL = 99


def next_step(steps, level, base, reason, points, future=None):
    """steps: [(lvl, skill, to, save)] 按顺序；base: 技能 → 基础等级；reason(skill) → 现在能不能学（与点数无关）；points: 剩余技能点；
    future(level, base) → 那一级、那些基础等级下的 reason（技能表；None = 不往后看）。返回 (状态, 步骤下标, 等级)。
    按顺序扫，跳过已完成的。计划在这一级或之前的步骤：能学 → NEXT（没点数 → UPCOMING）；永久学不了 → BLOCKED；暂时学不了、标了 save → 留点（hold）；
    暂时学不了、没标 → 看下一步。遇到计划在更高等级的步骤、或者只剩暂时学不了的 → 留点（hold）；都完成 → DONE。
    留点（hold，第 i 步是下一个要等的步骤）：没点数 → WAIT，等级 = 真正花下一点的那一级（下一级就花 → 0）。有点数：先按计划往后逐级模拟（每级 1 点、
    不算任务点、每级能花多少花多少），need(L) = 到 L 级计划花的点 − 到那时升级给的点；第 i 步现在就能学、而在计划给它花点那一级之前 need 一直
    小于手上的点 → NEXT i；手上的点多于 keep = max need，多出的给第 i 步之后第一个现在能学、计划等级不晚于 need 头一次到 keep 那一级的步骤
    （NEXT）；否则 SAVE 第 i 步，等级 = 计划头一次给它花点的那一级。
    | steps: [(lvl, skill, to, save)] in order; base: skill -> base level; reason(skill) -> learnable now (regardless of points); points: free skill points;
    future(level, base) -> the reason at that level with those base levels (the skill table; None = no look ahead). Returns (state, step index, level).
    The steps are scanned in order, skipping complete ones. A step planned at or below the level: learnable -> NEXT (UPCOMING without points); unlearnable
    for good -> BLOCKED; unlearnable for now and marked save -> hold; unlearnable for now without the mark -> on to the next step. A step planned above the
    level, or only steps unlearnable for now left -> hold; all complete -> DONE.
    hold (step i is the next one to wait for): without points -> WAIT with the level at which the next point is really spent (0 when that is the next
    level). With points the plan is first simulated level by level (one point per level, no quest points, as many points spent per level as it can use);
    need(L) = the points the plan spends up to level L minus the level-ups until then. Step i learnable now while need stays below the points held until
    the level the plan spends on it -> NEXT i; points beyond keep = max need go to the first step after i that is learnable now and planned no later than
    the level where need first reaches keep (NEXT); otherwise SAVE step i, with the level at which the plan first spends a point on it."""
    first_open = -1
    for i, (lvl, skill, to, save) in enumerate(steps):
        if base.get(skill, 0) >= to: continue
        if first_open < 0: first_open = i
        if lvl > level: return hold(steps, i, first_open, level, base, reason, points, future)
        r = reason(skill)
        if r == OK: return (NEXT if points > 0 else UPCOMING, i, 0)
        if r in PERMANENT: return (BLOCKED, i, 0)
        if save: return hold(steps, i, first_open, level, base, reason, points, future)
    return (DONE, -1, 0) if first_open < 0 else hold(steps, first_open, first_open, level, base, reason, points, future)


def plain(steps, level, base, reason):
    """计划本身在这一级给下一点的步骤（不留点、不提前），没有 → -1 | the step the plan itself gives the next point at this level (nothing kept, nothing pulled forward), -1 if none"""
    for i, (lvl, skill, to, save) in enumerate(steps):
        if base.get(skill, 0) >= to: continue
        if lvl > level: return -1
        r = reason(skill)
        if r == OK: return i
        if r in PERMANENT or save: return -1
    return -1


def demand(steps, level, base, future):
    """从下一级起逐级照计划花点（每级能花多少花多少）：need {L: 到 L 级花的点 − 升级给的点} 与每一步头一次拿点的等级
    | the plan spending level by level from the next level on (as many points as it can use per level): need {L: points spent up to L minus the level-ups}
    and the level each step first gets a point at"""
    b, spent, need, first = dict(base), 0, {}, {}
    for L in range(level + 1, MAX_LEVEL + 1):
        r = future(L, b)
        while True:
            k = plain(steps, L, b, r)
            if k < 0: break
            b[steps[k][1]] = b.get(steps[k][1], 0) + 1; spent += 1
            first.setdefault(k, L)
        need[L] = spent - (L - level)
        if all(b.get(s, 0) >= to for _, s, to, _ in steps): break
    return need, first


def hold(steps, i, first_open, level, base, reason, points, future):
    lvl, skill = steps[i][0], steps[i][1]
    learnable = lvl > level and reason(skill) == OK
    if future is None:
        if points <= 0: return (WAIT, first_open, 0)
        return (NEXT, i, 0) if learnable else (SAVE, i, lvl if lvl > level else 0)
    if points <= 0:
        for L in range(level + 1, MAX_LEVEL + 1):
            st, k, _ = next_step(steps, L, base, future(L, base), L - level, future)
            if st == NEXT: return (WAIT, k, L if L > level + 1 else 0)
            if st in (BLOCKED, DONE): break
        return (WAIT, first_open, 0)
    need, first = demand(steps, level, base, future)
    if learnable and all(n < points for L, n in need.items() if L < first.get(i, MAX_LEVEL + 1)): return (NEXT, i, 0)
    keep = max([0] + list(need.values()))
    if keep > 0 and points > keep:
        top = min(L for L, n in need.items() if n == keep)
        for j in range(i + 1, len(steps)):
            if steps[j][0] <= top and base.get(steps[j][1], 0) < steps[j][2] and reason(steps[j][1]) == OK: return (NEXT, j, 0)
    return (SAVE, i, first.get(i, lvl if lvl > level else 0))


def table_reason(skills, level, base):
    """能不能学，按技能表（与 BuildPlan.reason 同）：上限、等级（要求 + 已有基础等级 ≤ 角色等级）、前置 | learnable per the skill table (same as BuildPlan.reason): cap, level (requirement + current base <= character level), prerequisites"""
    def reason(skill):
        s = skills.get(skill)
        if not s: return R_DATA
        b = base.get(skill, 0)
        if b >= min(s['max'], SKILL_MAX): return R_MAX
        if level < s['req'] + b: return R_LEVEL
        if any(base.get(p, 0) < 1 for p in s['pre']): return R_PREREQ
        return OK
    return reason


def quest_points(level, quests):
    return sum(n for at, n, _ in QUESTS if at <= level) if quests else 0


def simulate(phases, skills, quests):
    """逐级跑所有阶段，返回每阶段的结果：每步在哪一级完成、阶段末剩几点、死路 | run all phases level by level; per phase: the level each step completes at, points left at the end, dead ends"""
    results, base, spent, chain = [], {}, 0, []
    for ph in phases:
        lo, hi = ph['levels']
        if ph['plan'] == 'respec': base, spent, chain = {}, 0, []
        own = [(s['lvl'], s['skill'], s['to'], bool(s.get('save'))) for s in ph['skills']]
        steps = chain + own
        chain = steps
        done_at = [None] * len(steps)
        dead = None
        for level in range(lo, hi + 1):
            points = (level - 1) + quest_points(level, quests) - spent
            while True:
                st, i, _ = next_step(steps, level, base, table_reason(skills, level, base), points, lambda lv, b: table_reason(skills, lv, b))
                if st == BLOCKED: dead = (level, i); break
                if st != NEXT: break
                sk = steps[i][1]; base[sk] = base.get(sk, 0) + 1; spent += 1; points -= 1
            for k, (_, sk, to, _) in enumerate(steps):
                if done_at[k] is None and base.get(sk, 0) >= to: done_at[k] = level
            if dead: break
        left = (hi - 1) + quest_points(hi, quests) - spent
        if dead: dead = (dead[0], dead[1] - (len(steps) - len(own)))
        results.append({'phase': ph['id'], 'done_at': done_at[len(steps) - len(own):], 'left': left, 'dead': dead, 'base': dict(base)})
    return results



def parse_table(raw):
    rows = list(csv.reader(io.StringIO(raw.decode('utf-8', 'replace')), delimiter='\t', quoting=csv.QUOTE_NONE))
    return [dict(zip(rows[0], r)) for r in rows[1:]]


class Tables:
    def __init__(self, read):
        raw = {f: read(f) for f in TABLES}
        self.sha = hashlib.sha256(b''.join(hashlib.sha256(raw[f]).digest() for f in TABLES)).hexdigest()
        t = {f: parse_table(raw[f]) for f in TABLES}
        desc = {r['skilldesc']: r for r in t['skilldesc.txt'] if r.get('skilldesc')}
        by_key = {r['skill']: r for r in t['skills.txt'] if r.get('*Id')}
        self.skills = {}
        for r in t['skills.txt']:
            if not r.get('*Id', '').isdigit(): continue
            d = desc.get(r['skilldesc'], {})
            pre = [int(by_key[r['reqskill%d' % k]]['*Id']) for k in (1, 2, 3) if r.get('reqskill%d' % k) in by_key]
            self.skills[int(r['*Id'])] = {'key': r['skill'], 'class': r['charclass'], 'req': int(r['reqlevel'] or 0), 'pre': pre,
                                          'max': int(r['maxlvl'] or 0), 'page': int(d.get('SkillPage') or 0)}
        self.start = {CLASSES[i]: r for i, r in enumerate(x for x in t['charstats.txt'] if x['class'] != 'Expansion')}
        self.items = {}
        for f in ('armor.txt', 'weapons.txt', 'misc.txt'):
            for r in t[f]:
                if r.get('code') and r['code'] not in self.items: self.items[r['code']] = r
        self.types = {r['Code']: r for r in t['itemtypes.txt'] if r.get('Code')}
        self.uniques = {r['index']: r for r in t['uniqueitems.txt'] if r.get('index')}
        self.sets = {r['index']: r for r in t['setitems.txt'] if r.get('index')}
        self.words = {r['Name']: r for r in t['Runes.txt'] if r.get('Name')}
        self.stats = {r['Stat'] for r in t['itemstatcost.txt'] if r.get('Stat')}
        self.hirelings = [r for r in t['hireling.txt'] if r.get('Version') == '100']

    def lineage(self, code, seen=None):
        seen = [] if seen is None else seen
        if code and code not in seen and code in self.types:
            seen.append(code)
            for k in ('Equiv1', 'Equiv2'): self.lineage(self.types[code].get(k), seen)
        return seen

    def item_types(self, code):
        r = self.items[code]
        return set(self.lineage(r.get('type'))) | set(self.lineage(r.get('type2')))


def romfs_reader(romfs):
    sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
    import romfs_art
    return lambda f: romfs_art.read(romfs, 'global/excel/' + f)


def dir_reader(d):
    def read(f):
        with open(os.path.join(d, f), 'rb') as fh: return fh.read()
    return read



def text_ok(t):
    return isinstance(t, dict) and t.get('de') and t.get('en')


def check_ci(g, name):
    """不带游戏文件的检查 | checks without game files"""
    err, warn = [], []
    with open(SCHEMA, encoding='utf-8') as f: schema = json.load(f)
    err += schema_errors(g, schema, schema)
    if err: return err, warn
    if name != g['id'] + '.json': err.append('Dateiname %s passt nicht zur id %s | file name does not match the id' % (name, g['id']))
    if CLASSES.index(g['class']) != g['classIndex']: err.append('classIndex %d passt nicht zu %s | does not match the class' % (g['classIndex'], g['class']))
    for k in ('strengths', 'weaknesses', 'notes'):
        lst = g['overview'].get(k)
        if lst and len(lst['de']) != len(lst['en']): err.append('overview.%s: de und en verschieden lang | de and en differ in length' % k)
    ids, prev_hi = set(), 0
    for p in g['phases']:
        lo, hi = p['levels']
        if p['id'] in ids: err.append('Phase %s doppelt | phase twice' % p['id'])
        ids.add(p['id'])
        if lo > hi or lo <= prev_hi: err.append('Phase %s: Stufen %d-%d überlappen oder rückwärts | levels overlap or run backwards' % (p['id'], lo, hi))
        elif prev_hi and lo != prev_hi + 1: warn.append('Phase %s: Lücke vor Stufe %d | gap before level %d' % (p['id'], lo, lo))
        prev_hi = hi
        last = 0
        for i, s in enumerate(p['skills']):
            if s['lvl'] < last: err.append('Phase %s Schritt %d: Stufe %d vor %d | steps not ordered by level' % (p['id'], i + 1, s['lvl'], last))
            if not lo <= s['lvl'] <= hi: err.append('Phase %s Schritt %d: Stufe %d außerhalb der Phase | level outside the phase' % (p['id'], i + 1, s['lvl']))


            n = s.get('note')
            if s.get('save') and n and ('aufheben' in n['de'].lower()) != n['en'].lower().startswith('keep'):
                err.append('Phase %s Schritt %d: Notiz sagt nur in einer Sprache „aufheben“ / „Keep“ | note says keep in one language only' % (p['id'], i + 1))
            last = s['lvl']
        rules = p['stats']
        for i, r in enumerate(rules):
            kinds = [k for k in ('to', 'rest', 'none') if k in r]
            if len(kinds) != 1: err.append('Phase %s Wert-Regel %d: genau eins von to / rest / none | exactly one of to / rest / none' % (p['id'], i + 1))
        if [r['stat'] for r in rules if 'rest' in r] == [] or 'rest' not in rules[-1]: err.append('Phase %s: letzte Wert-Regel muss rest sein | the last stat rule must be rest' % p['id'])
        if len([r for r in rules if 'rest' in r]) > 1: err.append('Phase %s: nur eine rest-Regel | only one rest rule' % p['id'])
    for i, x in enumerate(g['gear'] + g['merc']['gear']):
        if 'phase' in x and x['phase'] not in ids: err.append('gear[%d]: Phase %s gibt es nicht | no such phase' % (i, x['phase']))
        kinds = [k for k in ('runeword', 'unique', 'set', 'item', 'type') if k in x['ref']]
        if len(kinds) != 1 or ('quality' in x['ref'] and kinds != ['type']): err.append('gear[%d]: ref braucht genau eine Art | ref needs exactly one kind' % i)
        if kinds == ['runeword'] and not (x.get('base') and x['base'].get('sockets')): err.append('gear[%d]: Runenwort ohne base.sockets | rune word without base.sockets' % i)
    planned = {s['skill'] for p in g['phases'] for s in p['skills']}
    for sk in g['keySkills']:
        if sk not in planned: err.append('keySkills: %d kommt im Plan nicht vor | not in the plan' % sk)
    today = date.today().isoformat()
    for d in [s['seen'] for s in g['sources']]:
        if d > today: err.append('Datum %s liegt in der Zukunft | date in the future' % d)
    hosts = {}
    for s in g['sources']:
        h = urlparse(s['url']).hostname or ''
        h = '.'.join(h.split('.')[-2:])
        if s['kind'] != 'vendor': hosts.setdefault(h, []).append(s)
    if 'two-sources' in g['review']['checks'] and len(hosts) < 2: err.append('review two-sources: weniger als zwei unabhängige Seiten | fewer than two independent sites')
    return err, warn


def check_tables(g, T):
    """mit 3.3.94012-Tabellen | with the 3.3.94012 tables"""
    err, warn, report = [], [], []
    cls = g['class']
    for p in g['phases']:
        for i, s in enumerate(p['skills']):
            sk = T.skills.get(s['skill'])
            where = 'Phase %s Schritt %d' % (p['id'], i + 1)
            if not sk: err.append('%s: Fertigkeit %d gibt es nicht | no such skill' % (where, s['skill'])); continue
            if sk['key'] != s['key']: err.append('%s: Fertigkeit %d heißt %r, nicht %r | key mismatch' % (where, s['skill'], sk['key'], s['key']))
            if sk['class'] != cls: err.append('%s: %s gehört nicht zur Klasse %s | not of this class' % (where, sk['key'], cls))
            if s['to'] > sk['max']: err.append('%s: Ziel %d über maxlvl %d | target above maxlvl' % (where, s['to'], sk['max']))
            if s['lvl'] < sk['req']: warn.append('%s: %s erst ab Stufe %d (Schritt sagt %d) | only from level %d' % (where, sk['key'], sk['req'], s['lvl'], sk['req']))
    for sk in g['keySkills'] + [k['skill'] for k in g['profile'].get('keySkills', [])]:
        if sk not in T.skills or T.skills[sk]['class'] != cls: err.append('Schlüssel-Fertigkeit %d nicht in der Klasse | key skill not of the class' % sk)
    if any(s not in T.skills for p in g['phases'] for s in [x['skill'] for x in p['skills']]): return err, warn, report
    for quests in (True, False):
        for p, r in zip(g['phases'], simulate(g['phases'], T.skills, quests)):
            label = 'mit Questpunkten | with quest points' if quests else 'ohne Questpunkte | without quest points'
            if r['dead']:
                lvl, i = r['dead']
                err.append('Phase %s (%s): Sackgasse bei Stufe %d, Schritt %s | dead end' % (p['id'], label, lvl, '%d (%s)' % (i + 1, p['skills'][i]['key']) if i >= 0 else 'einer früheren Phase | of an earlier phase'))
            open_steps = [i for i, d in enumerate(r['done_at']) if d is None]
            if open_steps:
                msg = 'Phase %s (%s): bis Stufe %d nicht fertig, offen ab Schritt %d (%s) | not complete by its last level' % (p['id'], label, p['levels'][1], open_steps[0] + 1, p['skills'][open_steps[0]]['key'])
                (err if quests else warn).append(msg)
            last = 'offen | open' if None in r['done_at'] else str(max(r['done_at'] or [0]))
            report.append('Phase %s, %s: %d Schritte, letzter fertig mit Stufe %s, übrig am Ende %d Punkte | steps, last done at level, points left at the end'
                          % (p['id'], label, len(r['done_at']), last, r['left']))
            if quests:
                for i, (s, d) in enumerate(zip(p['skills'], r['done_at'])):
                    report.append('  %2d. ab %2d  %-18s → %2d  fertig mit Stufe %s%s' % (i + 1, s['lvl'], s['key'], s['to'], d, ' (save)' if s.get('save') else ''))

    start = T.start[cls]
    for p in g['phases']:
        hi = p['levels'][1]
        need = {}
        for x in g['gear']:
            if x['phase'] != p['id']: continue
            req, code, lvlreq, what = gear_facts(x, T, err, 'gear %s/%s' % (x['phase'], x['slot']))
            if lvlreq is not None and lvlreq > hi: err.append('gear %s/%s: Stufe %d über Phasenende %d | level above the phase end' % (x['phase'], x['slot'], lvlreq, hi))
            for st, v in req.items():
                if v > need.get(st, (0, ''))[0]: need[st] = (v, what)
        points, used = 5 * (hi - 1), 0
        for r in p['stats']:
            if 'to' in r:
                have = int(start[STAT_COLUMN[r['stat']]])
                if r['to'] < have: warn.append('Phase %s: %s-Ziel %d unter dem Startwert %d | target below the start value' % (p['id'], r['stat'], r['to'], have))
                used += max(0, r['to'] - have)
        if used > points: warn.append('Phase %s: Wert-Ziele brauchen %d Punkte, bis Stufe %d gibt es %d | stat targets need more points' % (p['id'], used, hi, points))
        for st, (v, what) in need.items():
            target = next((r.get('to') for r in p['stats'] if r['stat'] == st and 'to' in r), int(start[STAT_COLUMN[st]]))
            if target < v: warn.append('Phase %s: %s %d für %s, Regel-Ziel %d (Rest über Ausrüstung) | requirement above the rule target' % (p['id'], st, v, what, target))
            report.append('Phase %s: Anforderung %s %d (%s) | requirement' % (p['id'], st, v, what))
        prof = dict(g['profile']); prof.update(p.get('profile', {}))
        want = prof.get('requirements', {})
        for st in ('str', 'dex'):
            if p is g['phases'][-1] and st in need and want.get(st) != need[st][0]:
                warn.append('profile.requirements.%s = %s, aus der Ausrüstung %d | profile requirement differs from the gear' % (st, want.get(st), need[st][0]))
    for prof in [g['profile']] + [p['profile'] for p in g['phases'] if 'profile' in p]:
        for k, v in prof.get('stats', {}).items():
            if k not in T.stats: err.append('profile.stats: %s ist kein itemstatcost-Name | not an itemstatcost name' % k)
            if isinstance(v, dict) and k == 'item_singleskill' and int(v['param']) not in T.skills: err.append('profile.stats.item_singleskill: Fertigkeit %s fehlt | missing skill' % v['param'])
        for k in prof.get('caps', {}):
            if k not in T.stats: err.append('profile.caps: %s ist kein itemstatcost-Name | not an itemstatcost name' % k)
    m = g['merc']
    rows = [h for h in T.hirelings if h['Act'] == str(m['act']) and h['Difficulty'] == str(m['difficulty']) and m['aura'] in [h.get('Skill%d' % k) for k in range(1, 7)]]
    if not rows: err.append('merc: kein Söldner Akt %d, Schwierigkeit %d mit %s | no such mercenary' % (m['act'], m['difficulty'], m['aura']))
    for x in m['gear']: gear_facts(x, T, err, 'merc/%s' % x['slot'])
    return err, warn, report


def gear_facts(x, T, err, where):
    """一件装备：(力量 / 敏捷要求, 底材代码, 等级要求, 说明)，顺带查键、类型、孔数与部位 | one piece of gear: (str / dex requirements, base code, level requirement, label), checking keys, types, sockets and slot on the way"""
    ref, base = x['ref'], x.get('base', {})
    code, lvlreq, what, kinds = None, None, '', None
    if 'runeword' in ref:
        w = T.words.get(ref['runeword'])
        if not w or w.get('complete') != '1': err.append('%s: Runenwort %s fehlt oder ist nicht vollständig | rune word missing or incomplete' % (where, ref['runeword'])); return {}, None, None, ''
        runes = [w['Rune%d' % k] for k in range(1, 7) if w.get('Rune%d' % k)]
        lvlreq = max(int(T.items[r]['levelreq'] or 0) for r in runes)
        what = '%s (%s)' % (ref['runeword'], w.get('*Rune Name', ''))
        if base.get('sockets') != len(runes): err.append('%s: %s braucht %d Fassungen, base.sockets = %s | socket count' % (where, ref['runeword'], len(runes), base.get('sockets')))
        allowed = {w['itype%d' % k] for k in range(1, 7) if w.get('itype%d' % k)}
        banned = {w['etype%d' % k] for k in range(1, 4) if w.get('etype%d' % k)}
        if 'item' in base:
            if base['item'] not in T.items: err.append('%s: Basis %s gibt es nicht | no such base' % (where, base['item'])); return {}, None, lvlreq, what
            code = base['item']; kinds = T.item_types(code)
            if int(T.items[code].get('gemsockets') or 0) < len(runes): err.append('%s: %s hat höchstens %s Fassungen | base cannot hold the sockets' % (where, code, T.items[code].get('gemsockets')))
        elif 'type' in base:
            if base['type'] not in T.types: err.append('%s: Typ %s gibt es nicht | no such type' % (where, base['type'])); return {}, None, lvlreq, what
            kinds = set(T.lineage(base['type']))
        else: err.append('%s: Runenwort braucht base.item oder base.type | rune word needs a base' % where); return {}, None, lvlreq, what
        if not kinds & allowed or kinds & banned: err.append('%s: Basis %s passt nicht zu %s (%s) | base does not fit the rune word' % (where, code or base.get('type'), ref['runeword'], sorted(allowed)))
    elif 'unique' in ref or 'set' in ref:
        src, key = (T.uniques, ref['unique']) if 'unique' in ref else (T.sets, ref['set'])
        r = src.get(key)
        if not r or r.get('disabled') == '1': err.append('%s: %s fehlt oder ist abgeschaltet | missing or disabled' % (where, key)); return {}, None, None, ''


        if 'unique' in ref and r.get('spawnable') != '1' and 'PreCrafted ' + key in src:
            err.append('%s: %s fällt nicht mehr, gemeint ist PreCrafted %s | no longer drops, use PreCrafted %s' % (where, key, key, key)); return {}, None, None, ''
        code = r.get('code') or r.get('item'); lvlreq = int(r.get('lvl req') or 0); what = key
        if code not in T.items: err.append('%s: Basis %s von %s fehlt | base missing' % (where, code, key)); return {}, None, lvlreq, what
        kinds = T.item_types(code)
    elif 'item' in ref:
        if ref['item'] not in T.items: err.append('%s: Gegenstand %s fehlt | item missing' % (where, ref['item'])); return {}, None, None, ''
        code = ref['item']; kinds = T.item_types(code); lvlreq = int(T.items[code].get('levelreq') or 0); what = code
    else:
        if ref['type'] not in T.types: err.append('%s: Typ %s fehlt | type missing' % (where, ref['type'])); return {}, None, None, ''
        kinds = set(T.lineage(ref['type'])); what = ref['type']
    if kinds is not None and not kinds & set(SLOT_TYPES.get(x['slot'], ('weap',))):
        err.append('%s: passt nicht in den Platz %s (%s) | does not fit the slot' % (where, x['slot'], sorted(kinds)))
    req = {}
    if code and code in T.items:
        for st, col in (('str', 'reqstr'), ('dex', 'reqdex')):
            v = int(T.items[code].get(col) or 0)
            if v: req[st] = v
        lvlreq = max(lvlreq or 0, int(T.items[code].get('levelreq') or 0))
    return req, code, lvlreq, what + (' / ' + code if code else '')



PREVIEW_LABELS = ('lvl', 'str', 'dex', 'def', 'dmg1', 'dmg2', 'sock', 'eth')


def preview_errors(g, names):
    """每件装备都要有预览数据（导入包的 gamenames.json，gen_build_names.py）：暗金 / 套装件 / 符文之语有行号（宿主按它出游戏的说明）、底材、符文的短名，
    类型与品质有名字。names = gamenames.json 的内容 | every piece of gear needs preview data (the import pack's gamenames.json, gen_build_names.py): uniques /
    set items / rune words have a row (the host makes the game's text by it), a base, the runes' short names; types and qualities have names. names = the content
    of gamenames.json"""
    items, err = names.get('items', {}), []
    row = lambda e: isinstance(e.get('row'), int) and not isinstance(e.get('row'), bool) and e['row'] >= 0
    for i, x in enumerate(g['gear'] + g['merc']['gear']):
        ref, base, where = x['ref'], x.get('base', {}), 'preview gear[%d] %s' % (i, json.dumps(x['ref'], sort_keys=True))
        if 'runeword' in ref:
            w = items.get('r:' + ref['runeword'])
            if not w or not row(w): err.append('%s: kein Runenwort mit Zeile | no rune word with a row' % where); continue
            for r in w.get('runes', []):
                if not (items.get('i:' + r) or {}).get('short'): err.append('%s: Rune %s ohne Kurzname | rune without a short name' % (where, r))
            b = items.get('i:' + base['item']) if 'item' in base else items.get('t:' + base.get('type', ''))
            if not b: err.append('%s: Basis ohne Vorschau | base without preview' % where)
        elif 'unique' in ref or 'set' in ref:
            e = items.get(('u:' + ref['unique']) if 'unique' in ref else ('s:' + ref['set']))
            if not e or not row(e): err.append('%s: kein Eintrag mit Zeile | no entry with a row' % where); continue
            if not items.get('i:' + e.get('code', '')): err.append('%s: Basis %s ohne Vorschau | base without preview' % (where, e.get('code')))
        elif 'item' in ref:
            if not items.get('i:' + ref['item']): err.append('%s: Gegenstand ohne Vorschau | item without preview' % where)
        else:
            if not items.get('t:' + ref['type']): err.append('%s: Typ ohne Vorschau | type without preview' % where)
            if ref.get('quality') and ref['quality'] not in names.get('qualities', {}): err.append('%s: Qualität ohne Namen | quality without a name' % where)
    missing = [k for k in PREVIEW_LABELS if k not in names.get('labels', {})]
    if missing: err.append('preview: Spieltexte fehlen | game strings missing: %s' % ', '.join(missing))
    return err


def load(paths):
    out = []
    for p in paths:
        with open(p, encoding='utf-8') as f: out.append((p, json.load(f)))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--romfs', help='RomFS 3.3.94012 (liest nur | read only)')
    ap.add_argument('--tables', help='Verzeichnis mit den .txt-Tabellen | directory with the .txt tables')
    ap.add_argument('--report', action='store_true', help='Simulation ausgeben | print the simulation')
    ap.add_argument('--print-sha', action='store_true', help='Fingerabdruck der Tabellen ausgeben | print the table fingerprint')
    ap.add_argument('files', nargs='*')
    a = ap.parse_args(argv)
    files = a.files or sorted(os.path.join(BUILDS, f) for f in os.listdir(BUILDS) if f.endswith('.json') and f != 'schema.json')
    T = None
    if a.romfs or a.tables:
        T = Tables(romfs_reader(a.romfs) if a.romfs else dir_reader(a.tables))
        if a.print_sha: print(T.sha); return 0
        if TABLES_SHA and T.sha != TABLES_SHA:
            print('ERROR: tables are not those of %s (%s)' % (VERSION, T.sha)); return 1
    names = None
    if a.romfs:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import gen_build_names
        try: names = gen_build_names.verified(a.romfs)
        except gen_build_names.Refuse as e: print('FEHLER | ERROR: Vorschau-Daten | preview data: %s' % e); return 1
    elif T: print('note: preview check needs --romfs')
    bad = 0
    for path, g in load(files):
        err, warn = check_ci(g, os.path.basename(path))
        report = []
        if T and not err:
            e2, w2, report = check_tables(g, T); err += e2; warn += w2
        if names is not None and not err: err += preview_errors(g, names)
        print('%s: %s, %d Fehler | errors, %d Warnungen | warnings%s' % (os.path.basename(path), 'OK' if not err else 'FEHLER | ERROR', len(err), len(warn),
                                                                          (' (mit Tabellen %s%s | with tables%s)' % (VERSION, ' und Vorschau' if names is not None else '', ' and preview' if names is not None else '')) if T else ' (ohne Spieldateien | without game files)'))
        for e in err: print('  FEHLER  ' + e)
        for w in warn: print('  Warnung ' + w)
        if a.report:
            for r in report: print('  ' + r)
        bad += bool(err)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
