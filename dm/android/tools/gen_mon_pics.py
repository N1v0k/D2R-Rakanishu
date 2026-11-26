#!/usr/bin/env python3
























import csv
import ctypes
import glob
import hashlib
import io
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
DM = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
sys.path.insert(0, os.path.join(DM, 'verify'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(DM, 'tools'))
from game_addrs import GA

STANDARD_TURN = math.radians(45)
MAX_BYTES = 14 * 1024 * 1024
NAME = re.compile(r'^[a-z0-9_]{1,40}$')

REJECTED = {}



TYPICAL = {'zombie1': {'S3': 0}, 'fallen1': {'SH': 1}}



EFFECTS = frozenset(('willowisp1', 'colbarbwhirl', 'colbarbthrow', 'colbarbfrenzy'))

UNVERIFIED_COLOURS = {}
PARTS = (('HD', 'head'), ('TR', 'torso'), ('LG', 'legs'), ('RA', 'rarm'), ('LA', 'larm'), ('RH', 'rhand'), ('LH', 'lhand'), ('SH', 'shield'),
         ('S1', 'special1'), ('S2', 'special2'), ('S3', 'special3'), ('S4', 'special4'), ('S5', 'special5'), ('S6', 'special6'),
         ('S7', 'special7'), ('S8', 'special8'))
ATTACH_BONES = {'rhand': 'r_weapon_attach', 'lhand': 'l_weapon_attach', 'shield': 'shield_attach'}
NEUTRAL_MODE = 1




COLOUR_BASE = 2
OWN_COLOURS = 8
SHARED_COLOURS = 'hd/global/palette/randtransforms.json'
IDENTITY = {'colorAdjustment': {'x': 0, 'y': 0, 'z': 0, 'w': 1}, 'colorTint': {'x': 0, 'y': 0, 'z': 0, 'w': 0},
            **{f: {'x': 0, 'y': 0, 'z': 0, 'w': 0} for f in ('hueAdjustment0', 'hueAdjustment1', 'satAdjustment0', 'satAdjustment1')}}


MAIN_SHA256 = 'bfe6f4d733ca56b9ca52fdda108f33284337b63d8490d166acc06d63aa2b4b5a'
QUATERNION_RANGES = GA.GRANNY_QUAT_RANGES


class PictureError(Exception):
    """A look that should have a picture cannot be rendered from the player's files."""


class Unsupported(PictureError):
    """A definition that uses something the pictures do not draw: its looks get no picture, with this as the reason, which stops the build
    for an enemy of the book (documented). Every other error of a rendering means damaged or unexpected game files and stops the build."""


class Archive:


    def __init__(self, romfs):
        import touch_layout
        self.romfs, self._read = romfs, touch_layout._read_entry
        self.entries = {}
        with open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8') as f:
            for line in f:
                fields = line.rstrip('\n').split('|')
                if len(fields) >= 11 and not line.startswith('#'):
                    self.entries.setdefault(fields[9].lower().split(':', 1)[-1], fields)

    def read(self, path):
        fields = self.entries.get(self._key(path))
        if fields is None:
            raise PictureError('the game files have no %s' % path)
        return self._read(self.romfs, fields)

    @staticmethod
    def _key(path):
        p = path.lower().lstrip('/')
        return p if p.startswith('data/') else 'data/' + p


def table(arc, name):
    rows = list(csv.DictReader(io.StringIO(arc.read('global/excel/%s.txt' % name).decode('utf-8-sig')), delimiter='\t'))
    return [{k.lower(): (v or '') for k, v in r.items() if k} for r in rows]


def typical(variants):


    counts = {}
    for v in variants:
        counts[v] = counts.get(v, 0) + 1
    if not counts:
        return None
    top = max(counts.values())
    tied = [v for v in counts if counts[v] == top]
    chosen = tied[len(tied) // 2]
    return None if chosen == 'nil' else chosen


def chosen_parts(row2, definition=None):


    out = {}
    for column, part in PARTS:
        if row2.get(column.lower()) != '1':
            continue
        variants = [v.strip().lower() for v in row2.get(column.lower() + 'v', '').split(',') if v.strip()]
        pick = TYPICAL.get(definition, {}).get(column)
        if pick is not None:
            if not 0 <= pick < len(variants):
                raise PictureError('%s: typical variant %d of %s outside its %d variants' % (definition, pick, column, len(variants)))
            chosen = None if variants[pick] == 'nil' else variants[pick]
        else:
            chosen = typical(variants)
        if chosen:
            out[part] = chosen
    return out


def model_levels(arc, path):
    stem = Archive._key(path)[:-len('.model')]
    return sorted((int(m.group(1)), p) for p in arc.entries if p.startswith(stem + '_lod') and (m := re.match(re.escape(stem) + r'_lod(\d+)\.model$', p)))


def has_model(arc, path):
    return bool(model_levels(arc, path))


def model_file(arc, path):

    levels = model_levels(arc, path)
    if not levels:
        raise PictureError('the game files have no level of %s' % path)
    return levels[0][1]


def component(entity, kind):
    return [c for c in entity.get('components', []) if c.get('type') == kind]



TRANSFORM_FIELDS = {'colorAdjustment': 'xyzw', 'colorTint': 'xyzw', 'hueAdjustment0': 'xyz', 'hueAdjustment1': 'xyz', 'satAdjustment0': 'xyz',
                    'satAdjustment1': 'xyz'}


def checked_transform(transform, where):


    if not isinstance(transform, dict):
        raise PictureError('%s: a colour transform that is not an object' % where)
    for field, components in TRANSFORM_FIELDS.items():
        value = transform.get(field)
        for k in components:
            v = value.get(k) if isinstance(value, dict) else None
            if isinstance(v, bool) or not isinstance(v, (int, float)) or not math.isfinite(v):
                raise PictureError('%s: colour transform %s.%s is %r, not a finite number' % (where, field, k, v))
    return transform


def variant_table(arc, root):


    names = component(root, 'VariantDefinitionComponent')
    if not names or not names[0].get('filename'):
        return None
    entries = json.loads(arc.read(names[0]['filename'])).get('entries') or []
    if len(entries) != OWN_COLOURS or any(len(e.get('transforms') or []) != 3 for e in entries):
        raise Unsupported('colour table %s is not %d entries of three transforms' % (names[0]['filename'], OWN_COLOURS))
    return [[checked_transform(t, names[0]['filename']) for t in e['transforms']] for e in entries]


def shared_colours(arc):

    transforms = json.loads(arc.read(SHARED_COLOURS)).get('transforms') or []
    if not transforms:
        raise PictureError('%s holds no colour transforms' % SHARED_COLOURS)
    return [checked_transform(t, SHARED_COLOURS) for t in transforms]


def colour_transforms(table_, shared, index):



    if index < OWN_COLOURS:
        return table_[index] if table_ is not None else [IDENTITY] * 3
    index -= OWN_COLOURS
    return [shared[index]] * 3 if index < len(shared) else None


def transform_numbers(transform):


    return [float(transform[field][k]) for field, components in TRANSFORM_FIELDS.items() for k in components]


def job(arc, definition, row2, colour=COLOUR_BASE):



    out = []
    try:
        _job(arc, definition, row2, colour, out)
    except Unsupported as e:
        out.append(('fail', 'unsupported', str(e)))
    except Exception as e:
        out.append(('fail', 'error', '%s: %s' % (type(e).__name__, e)))
    return out


def _job(arc, definition, row2, colour, out):
    d = json.loads(arc.read('hd/character/enemy/%s.json' % definition))
    entities = d.get('entities') or []
    root = next((e for e in entities if component(e, 'SkeletonDefinitionComponent')), None)
    if root is None:
        raise Unsupported('no skeleton')
    out.append(('skeleton', component(root, 'SkeletonDefinitionComponent')[0]['filename']))
    unit = component(root, 'UnitRootComponent')
    if not unit or not unit[0].get('state_machine_filename'):
        raise Unsupported('no state machine')
    machine = json.loads(arc.read(unit[0]['state_machine_filename']))
    weapon = (row2.get('basew') or 'hth').lower()
    neutral = next((s for s in machine.get('states', []) if s.get('modeId') == NEUTRAL_MODE), None)
    bound = (neutral or {}).get('animationBindings', {})
    names = bound.get(weapon) or bound.get('hth')
    if not names:
        raise Unsupported('no neutral animation for weapon class %s' % weapon)
    item = next((a for a in machine.get('animations', []) if a.get('name') == names[0]), None)
    if item is None or not item.get('filename', '').endswith('.animation'):
        raise Unsupported('neutral animation %s without a file' % names[0])

    clip_name = os.path.basename(item['filename'])[:-len('.animation')]
    out.append(('animation', os.path.dirname(item['filename']) + '/combined.animations', clip_name))
    variant = colour_transforms(variant_table(arc, root), shared_colours(arc) if colour >= OWN_COLOURS else [], colour)
    if variant is None:
        raise PictureError('%s: no colour %d' % (definition, colour))
    out.append(('colour',) + tuple(v for t in variant for v in transform_numbers(t)))
    out.append(('pose',))
    turn = unit[0].get('rotationOverride') or {'x': 0, 'y': 0, 'z': 0, 'w': 1}
    if abs(turn.get('x', 0)) > 1e-3 or abs(turn.get('z', 0)) > 1e-3:
        raise Unsupported('model turned around a tilted axis')
    out.append(('turn', STANDARD_TURN - 2 * math.atan2(turn['y'], turn['w'])))
    parts = chosen_parts(row2, definition)

    shipped = {(component(e, 'UnitPartComponent')[0].get('part') or '').lower() for e in entities if component(e, 'UnitPartComponent')
               and any(md.get('filename') and has_model(arc, md['filename']) for md in component(e, 'ModelDefinitionComponent'))}
    for e in entities:
        part = component(e, 'UnitPartComponent')
        name = (part[0].get('part') or '').lower() if part else ''
        if part and parts.get(name) != (part[0].get('variant') or '').lower():
            continue
        if not part and e.get('name', '').lower().endswith('_dead'):
            continue
        attachments = component(e, 'UnitAttachmentComponent')
        for md in component(e, 'ModelDefinitionComponent'):
            path = md.get('filename')
            if path and has_model(arc, path):
                out.append(('model', int(md.get('visibleLayers', 1)), path, model_file(arc, path)))
            elif attachments or (part and name not in shipped):
                continue
            elif part:
                raise Unsupported('part %s variant %s has no model in the game files (%s)' % (name, part[0].get('variant'), path))
            else:
                raise PictureError('%s: the game files lack the model %s' % (definition, path))
        for a in attachments:
            if a.get('filename'):
                out.append(('attach', (a.get('overrideBoneName') or ATTACH_BONES.get((a.get('part') or name).lower(), '')).lower())
                           + attachment_transform(a))
                _item(arc, a['filename'], out)


def attachment_transform(attachment):



    t = attachment.get('attachmentTransform') or {}
    r, s, p = t.get('rotation') or {}, t.get('scale') or {}, t.get('translation') or {}
    return tuple([float(r.get(a, 0)) for a in 'xyz'] + [float(s.get(a, 1)) for a in 'xyz'] + [float(p.get(a, 0)) for a in 'xyz'])


def _item(arc, item_path, out):


    item = json.loads(arc.read(item_path))
    models = [md for e in item.get('entities') or [] for md in component(e, 'ModelDefinitionComponent')]
    if not models:
        raise Unsupported('attached item %s without a model' % item_path)
    for md in models:
        out.append(('item', int(md.get('visibleLayers', 1)), md['filename'], model_file(arc, md['filename'])))


def act_bosses(monstats):

    import gen_sim_data
    acts = {i for i, (_, kind, _) in gen_sim_data.BOSSES.items() if kind == 'actboss'}
    found = {int(r['*hcidx']): r['id'].lower() for r in monstats if r.get('*hcidx', '').isdigit() and int(r['*hcidx']) in acts}
    if set(found) != acts:
        raise PictureError('monstats lacks the act bosses %s' % sorted(acts - set(found)))
    return sorted(found.values())


def listed(arc):


    import gen_book_data
    return gen_book_data.book_monsters(arc.read)


def look(row2, definition):

    return tuple(sorted(chosen_parts(row2, definition).items())), (row2.get('basew') or 'hth').lower()


def identity(transform):

    a, tint = transform['colorAdjustment'], transform['colorTint']
    return (a['x'], a['y'], a['z'], a['w']) == (0, 0, 0, 1) and tint['w'] == 0 and not any(
        transform[f][k] for f in ('hueAdjustment0', 'hueAdjustment1', 'satAdjustment0', 'satAdjustment1') for k in 'xyz')


def colour_key(transforms):

    return json.dumps([None if identity(t) else t for t in transforms], sort_keys=True)


def plan(arc):




    monstats, monstats2 = table(arc, 'monstats'), {r['id'].lower(): r for r in table(arc, 'monstats2')}
    hd = {k.lower(): v.lower() for k, v in json.loads(arc.read('hd/character/monsters.json')).items()}
    bosses, shown = act_bosses(monstats), listed(arc)
    shared, tables_ = shared_colours(arc), {}

    def colours(definition):
        if definition not in tables_:
            d = json.loads(arc.read('hd/character/enemy/%s.json' % definition))
            root = next((e for e in d.get('entities') or [] if component(e, 'SkeletonDefinitionComponent')), None)
            try:
                tables_[definition] = variant_table(arc, root) if root is not None else None
            except Unsupported as e:
                tables_[definition] = e
        return tables_[definition]

    draw, none, wanted, base, in_class = {}, {}, {}, {}, {}

    def want(key, definition, row2, index, cls=None):
        t = colours(definition)
        if isinstance(t, Unsupported):
            none[key] = 'unsupported: %s' % t
            return
        transforms = colour_transforms(t, shared, index)
        if transforms is None and cls is not None:
            in_class[key] = cls
        elif transforms is None:
            none[key] = 'colour: the game files hold no colour %d' % index
        elif key in UNVERIFIED_COLOURS:
            none[key] = 'colour: ' + UNVERIFIED_COLOURS[key]
        else:
            looks = wanted.setdefault(definition, {}).setdefault(look(row2, definition), {})
            looks.setdefault(colour_key(transforms), []).append((index, key, row2))

    for r in monstats:
        key = r['id'].lower()
        if not key:
            continue
        definition = hd.get(key)
        row2 = monstats2.get((r.get('monstatsex') or key).lower())
        level = r.get('translvl') or '0'
        if key not in shown:
            none[key] = 'unlisted'
        elif not definition:
            none[key] = 'nomodel'
        elif definition in EFFECTS:
            none[key] = 'effect'
        elif definition in REJECTED:
            none[key] = 'rejected: ' + REJECTED[definition]
        elif not NAME.match(definition) or row2 is None:
            none[key] = 'unsupported: definition %r, monstats2 row %s' % (definition, row2 is not None)
        elif not level.isdigit():
            none[key] = 'unsupported: colour level %r' % level
        else:
            base[key] = (definition, row2)
            want(key, definition, row2, int(level) + COLOUR_BASE)
    for r in table(arc, 'superuniques'):
        if not r.get('hcidx', '').isdigit():
            continue
        cls, key = r.get('class', '').lower(), 'su:%d' % int(r['hcidx'])
        utrans = {r.get(c, '') for c in ('utrans', 'utrans(n)', 'utrans(h)')}
        if cls not in base:
            none[key] = none.get(cls, 'nomodel')
        elif len(utrans) != 1 or not next(iter(utrans)).isdigit():
            none[key] = 'unsupported: colours %s by difficulty' % sorted(utrans)
        else:
            want(key, base[cls][0], base[cls][1], int(next(iter(utrans))), cls)
    jobs = {}
    for definition, looks in sorted(wanted.items()):
        own = colour_transforms(colours(definition), shared, COLOUR_BASE)
        for n, (_, groups) in enumerate(sorted(looks.items())):
            look_name = definition if n == 0 else '%s_%d' % (definition, n + 1)
            for ck, keys in groups.items():
                index = min(i for i, _, _ in keys)
                name = look_name if ck == colour_key(own) else '%s_c%d' % (look_name, index)
                if not NAME.match(name) or name in jobs:
                    raise PictureError('picture name %r' % name)
                jobs[name] = (definition, keys[0][2], index)
                draw.update((k, name) for _, k, _ in keys)
    for key, cls in in_class.items():
        if cls in draw:
            draw[key] = draw[cls]
        else:
            none[key] = none[cls]
    return draw, none, jobs, bosses


def documented(reason):



    return reason in ('unlisted', 'nomodel', 'effect') or reason.startswith('rejected: ') \
        or reason in {'colour: ' + why for why in UNVERIFIED_COLOURS.values()}


def missing(out):


    try:
        with open(os.path.join(out, 'mon.json'), encoding='utf-8') as f:
            data = json.load(f)
        if not isinstance(data, dict) or set(data) != {'pictures', 'none'} or not all(isinstance(data[k], dict) for k in data) \
                or not data['pictures'] or any(not isinstance(v, str) or not NAME.match(v) for v in data['pictures'].values()) \
                or any(not isinstance(v, str) or not v for v in data['none'].values()) or set(data['pictures']) & set(data['none']):
            return ['mon.json']
    except (OSError, ValueError):
        return ['mon.json']
    lack = []
    for name in sorted(set(data['pictures'].values())):
        for rel in ('mon/%s.webp' % name, 'mon/s/%s.webp' % name):
            p = os.path.join(out, rel)
            if not os.path.isfile(p) or os.path.getsize(p) == 0:
                lack.append(rel)
    return lack


def quaternion_ranges(main):


    import nsoimg
    with open(main, 'rb') as f:
        digest = hashlib.sha256(f.read()).hexdigest()
    if digest != MAIN_SHA256:
        raise PictureError('the game executable %s has SHA-256 %s, not the verified 3.4.95035 build' % (main, digest))
    values = struct.unpack_from('<32f', nsoimg.load(main), QUATERNION_RANGES)
    if not all(math.isfinite(v) for v in values):
        raise PictureError('the quaternion ranges of %s are not finite numbers' % main)
    return values


def field(value):

    if isinstance(value, float):
        return repr(value)
    text = str(value)
    if any(c in text for c in '\t\n\r'):
        raise PictureError('a job file field holds a tab or line break: %r' % text)
    return text


def write_jobs(path, ranges, jobs):

    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('monpic 1\n')
        f.write('\t'.join(['ranges'] + [field(float(v)) for v in ranges]) + '\n')
        for name, records in sorted(jobs.items()):
            f.write('picture\t%s\n' % name)
            for r in records:
                f.write('\t'.join(field(v) for v in r) + '\n')
            f.write('end\n')


_lib = None


def lib():



    global _lib
    if _lib is not None:
        return _lib
    path = os.environ.get('DM_MONPIC_LIB') or build_lib()
    L = ctypes.CDLL(path)
    L.monpic_bitknit_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t]
    L.monpic_render.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
    _lib = L
    return L


def _compilers():
    cc = os.environ.get('CC') or ('clang' if shutil.which('clang') else 'cc')
    cxx = os.environ.get('CXX') or ('clang++' if shutil.which('clang++') else 'c++')
    for c in (cc, cxx):
        if not shutil.which(c):
            raise PictureError('no C/C++ compiler %s for the picture library (install clang, or name one with CC / CXX)' % c)
    return cc, cxx


def build_lib():
    src = os.path.join(HERE, 'monpic')
    with open(os.path.join(src, 'sources.txt'), encoding='utf-8') as f:
        patterns = [line.strip() for line in f if line.strip() and not line.startswith('#')]
    sources = [p for pattern in patterns for p in sorted(glob.glob(os.path.join(src, pattern)))]
    headers = sorted({h for s in sources for h in glob.glob(os.path.join(os.path.dirname(s), '*.h'))}
                     | set(glob.glob(os.path.join(DM, 'third_party', 'libwebp', 'src', 'webp', '*.h'))))
    cc, cxx = _compilers()
    nt = os.name == 'nt'

    common = ['-O2', '-ffp-contract=off', '-I' + src] + ['-I' + os.path.join(DM, d) for d in (
        'gfx/tex', 'gfx/nvn', 'third_party/libdeflate', 'third_party/libwebp', 'third_party/libwebp/src')]
    if not nt:
        common += ['-fPIC', '-fvisibility=hidden', '-pthread']
    flags = {'.c': [cc, '-std=c11'] + common, '.cpp': [cxx, '-std=c++17'] + common}
    digest = hashlib.sha256(repr(flags).encode())
    for p in sources + headers:
        with open(p, 'rb') as f:
            digest.update(p.replace(DM, '').encode() + b'\0' + f.read())
    root = os.path.abspath(os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work'))
    out_dir = os.path.join(root, 'build', 'cache', 'monpic', digest.hexdigest()[:16])
    lib_path = os.path.join(out_dir, 'monpic.dll' if nt else 'libmonpic.so')
    if os.path.isfile(lib_path):
        return lib_path
    work = tempfile.mkdtemp(prefix='monpic-')
    try:
        def compile_one(i):
            s = sources[i]
            obj = os.path.join(work, '%04d.o' % i)
            r = subprocess.run(flags[os.path.splitext(s)[1]] + ['-c', s, '-o', obj], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if r.returncode:
                raise PictureError('compiling %s failed:\n%s' % (s, r.stdout.decode('utf-8', 'replace')))
            return obj
        with ThreadPoolExecutor(os.cpu_count() or 1) as pool:
            objs = list(pool.map(compile_one, range(len(sources))))
        os.makedirs(out_dir, exist_ok=True)
        tmp = lib_path + '.part'
        r = subprocess.run([cxx, '-shared'] + ([] if nt else ['-pthread']) + ['-o', tmp] + objs, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if r.returncode:
            raise PictureError('linking the picture library failed:\n%s' % r.stdout.decode('utf-8', 'replace'))
        os.replace(tmp, lib_path)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return lib_path


def render(romfs, jobs, ranges, out, threads):


    work = tempfile.mkdtemp(prefix='monpic-jobs-')
    try:
        job_path, results = os.path.join(work, 'jobs.txt'), os.path.join(work, 'results.txt')
        write_jobs(job_path, ranges, jobs)
        err = ctypes.create_string_buffer(1024)
        if lib().monpic_render(os.fsencode(romfs), os.fsencode(job_path), os.fsencode(out), threads, os.fsencode(results), err, len(err)):
            raise PictureError('the picture library stopped: %s' % err.value.decode('utf-8', 'replace'))
        outcome = {}
        with open(results, encoding='utf-8') as f:
            for line in f:
                name, kind, *message = line.rstrip('\n').split('\t')
                if name not in jobs or kind not in ('ok', 'unsupported', 'error') or name in outcome:
                    raise PictureError('the picture library reported %r' % line)
                outcome[name] = (kind, message[0] if message else '')
        return outcome
    finally:
        shutil.rmtree(work, ignore_errors=True)


def stage(romfs, out, main, workers=None):



    ranges = quaternion_ranges(main)
    arc = Archive(romfs)
    draw, none, jobs, bosses = plan(arc)
    os.makedirs(os.path.join(out, 'mon', 's'), exist_ok=True)
    threads = workers if workers is not None else min(os.cpu_count() or 1, 8)
    outcome = render(romfs, {name: job(arc, *j) for name, j in jobs.items()}, ranges, out, threads)
    errors = sorted((name, message) for name, (kind, message) in outcome.items() if kind == 'error')
    if errors:
        name, message = errors[0]
        raise SystemExit('appart: the enemy picture of %s could not be rendered: %s' % (jobs[name][0], message))
    pictures = []
    for name in sorted(jobs):
        definition = jobs[name][0]
        kind, message = outcome[name]
        if kind == 'unsupported':
            lost = sorted(k for k, v in draw.items() if v == name)
            if any(k in bosses for k in lost):
                raise SystemExit('appart: the act boss picture of %s could not be rendered: %s' % (definition, message))
            for k in lost:
                del draw[k]
                none[k] = 'unsupported: %s' % message
            print('appart: no enemy picture of %s (%d looks): %s' % (definition, len(lost), message))
        else:
            pictures.append(name)
    lack = [b for b in bosses if b not in draw]
    if lack:
        raise SystemExit('appart: act bosses without a picture: %s' % ', '.join(lack))
    gaps = sorted('%s (%s)' % (k, v) for k, v in none.items() if not documented(v))
    if gaps:
        raise SystemExit('appart: enemies of the book without a picture: %s' % ', '.join(gaps))
    total = sum(os.path.getsize(os.path.join(out, rel)) for name in pictures for rel in ('mon/%s.webp' % name, 'mon/s/%s.webp' % name))
    with open(os.path.join(out, 'mon.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump({'pictures': dict(sorted(draw.items())), 'none': dict(sorted(none.items()))}, f, sort_keys=True, separators=(',', ':'))
    if total > MAX_BYTES:
        raise SystemExit('appart: the enemy pictures take %d bytes, more than %d' % (total, MAX_BYTES))
    lack = missing(out)
    if lack:
        raise SystemExit('appart: enemy pictures incomplete, missing %s' % ', '.join(lack[:8]))
    reasons = {}
    for v in none.values():
        reasons[v.split(':')[0]] = reasons.get(v.split(':')[0], 0) + 1
    print('appart: %d enemy pictures (%d KiB) for %d look keys, %d without (%s)' % (len(pictures), total // 1024, len(draw), len(none),
          ', '.join('%s %d' % kv for kv in sorted(reasons.items()))))


if __name__ == '__main__':
    stage(sys.argv[1], sys.argv[2], sys.argv[3])
