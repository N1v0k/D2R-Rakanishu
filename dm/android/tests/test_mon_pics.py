#!/usr/bin/env python3








import hashlib
import json
import math
import os
import pathlib
import struct
import sys
import tempfile

import numpy as np

dm = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(dm / 'android' / 'tools'))
sys.path.insert(0, str(dm / 'android' / 'tests'))
sys.path.insert(0, str(dm / 'ios' / 'tools'))
import gen_mon_pics as g
import pic_tolerance

checks = 0


def check(value, what):
    global checks
    checks += 1
    if not value:
        raise AssertionError(what)


def raises(kind, fn, what):
    try:
        fn()
    except kind:
        check(True, what)
        return
    raise AssertionError(what + ': no ' + kind.__name__)


KIND = {'str': 8, 'i4': 19, 'u4': 20, 'f4': 10, 'i2': 15, 'u2': 16, 'u1': 12, 'i1': 11, 'f2': 21, 'transform': 9, 'inline': 1, 'ref': 2,
        'records': 3, 'refs': 4, 'variant': 5, 'varray': 7}
SIZE = {'str': 8, 'i4': 4, 'u4': 4, 'f4': 4, 'i2': 2, 'u2': 2, 'u1': 1, 'i1': 1, 'f2': 2, 'transform': 68, 'ref': 8, 'records': 12, 'refs': 12,
        'variant': 16, 'varray': 20}
NUMPY = {'f4': '<f4', 'i4': '<i4', 'u4': '<u4', 'i2': '<i2', 'u2': '<u2', 'u1': 'u1', 'i1': 'i1', 'f2': '<f2'}


class Granny:





    def __init__(self):
        self.data, self.fixups, self.types = bytearray(16), [], {}

    def put(self, raw, align=8):
        self.data += bytes(-len(self.data) % align)
        at = len(self.data)
        self.data += raw
        return at

    def pointer(self, at, target):
        self.fixups.append((at, target))

    def string(self, s):
        return self.put(s.encode() + b'\0', 1)

    def _type(self, members):
        key = tuple(members)
        if key not in self.types:
            at = self.put(bytes(44 * (len(members) + 1)))
            for i, (kind, name, sub, count) in enumerate(members):
                struct.pack_into('<I', self.data, at + 44 * i, kind)
                self.pointer(at + 44 * i + 4, self.string(name))
                if sub:
                    self.pointer(at + 44 * i + 12, sub)
                struct.pack_into('<i', self.data, at + 44 * i + 20, count)
            self.types[key] = at
        return self.types[key]

    def _dtype_type(self, dtype):
        members = []
        for name in dtype.names:
            base, shape = dtype[name].base, dtype[name].shape
            kind = next(k for k, v in NUMPY.items() if np.dtype(v) == base)
            members.append((KIND[kind], name, 0, shape[0] if shape else 0))
        return self._type(members)

    def type_of(self, fields):
        members = []
        for name, kind, value in fields:
            sub, count = 0, 0
            if kind in ('inline', 'ref', 'variant'):
                if kind == 'ref' and value is None:
                    sub = self._type([(KIND['i4'], 'Unused', 0, 0)])
                elif kind != 'variant':
                    sub = self.type_of(value)
            elif kind in ('records', 'refs'):
                if isinstance(value, np.ndarray):
                    sub = self._dtype_type(value.dtype)
                else:
                    sub = self.type_of(value[0]) if value else self._type([(KIND['i4'], 'Unused', 0, 0)])
            elif kind in NUMPY and isinstance(value, (list, tuple)):
                count = len(value)
            members.append((KIND[kind], name, sub, count))
        return self._type(members)

    def size_of(self, fields):
        total = 0
        for name, kind, value in fields:
            if kind == 'inline':
                total += self.size_of(value)
            elif kind in NUMPY and isinstance(value, (list, tuple)):
                total += SIZE[kind] * len(value)
            else:
                total += SIZE[kind]
        return total

    def obj(self, fields):

        at = self.put(bytes(self.size_of(fields)))
        self.fill(at, fields)
        return at

    def fill(self, at, fields):
        for name, kind, value in fields:
            if kind == 'inline':
                self.fill(at, value)
                at += self.size_of(value)
                continue
            if kind in NUMPY:
                values = list(value) if isinstance(value, (list, tuple)) else [value]
                for v in values:
                    struct.pack_into('<' + {'f4': 'f', 'i4': 'i', 'u4': 'I', 'i2': 'h', 'u2': 'H', 'u1': 'B', 'i1': 'b', 'f2': 'e'}[kind], self.data, at, v)
                    at += SIZE[kind]
                continue
            if kind == 'str':
                if value is not None:
                    self.pointer(at, self.string(value))
            elif kind == 'transform':
                position, orientation, scale = value
                struct.pack_into('<I16f', self.data, at, 0, *position, *orientation, *scale)
            elif kind == 'ref':
                if value is not None:
                    self.pointer(at, self.obj(value))
            elif kind == 'variant':
                if value is not None:
                    self.pointer(at, self.type_of(value))
                    self.pointer(at + 8, self.obj(value))
            elif kind in ('records', 'refs', 'varray'):
                if isinstance(value, np.ndarray):
                    target, n = (self.put(value.tobytes(), 4) if len(value) else 0), len(value)
                    sub = self._dtype_type(value.dtype)
                elif kind == 'refs':
                    target, n = (self.put(bytes(8 * len(value))) if value else 0), len(value)
                    for i, v in enumerate(value):
                        if v is not None:
                            self.pointer(target + 8 * i, self.obj(v))
                else:
                    size = self.size_of(value[0]) if value else 0
                    target, n = (self.put(bytes(size * len(value))) if value else 0), len(value)
                    for i, v in enumerate(value):
                        self.fill(target + size * i, v)
                if kind == 'varray':
                    if n:
                        self.pointer(at, sub)
                    struct.pack_into('<I', self.data, at + 8, n)
                    if n:
                        self.pointer(at + 12, target)
                else:
                    struct.pack_into('<I', self.data, at, n)
                    if n:
                        self.pointer(at + 4, target)
            at += SIZE[kind]

    def file(self, root_fields):
        root = self.obj(root_fields)
        root_type = self.type_of(root_fields)
        sec = bytes(self.data)
        fix = b''.join(struct.pack('<3I', at, 0, to) for at, to in self.fixups)
        head = 0x20 + 0x24
        data_off = head + 44
        fix_off = data_off + len(sec)
        out = bytearray(bytes.fromhex('e59b495e6f631f141e13eba990beedc4') + struct.pack('<IIQ', 0x1b8, 0, 0))
        out += struct.pack('<5I', 7, fix_off + len(fix), 0, 0x24, 1) + struct.pack('<4I', 0, root_type, 0, root)
        out += bytes(head - len(out))
        out += struct.pack('<11I', 0, data_off, len(sec), len(sec), 4, len(sec), len(sec), fix_off, len(self.fixups), 0, 0)
        return bytes(out + sec + fix)


def texture(rgb, alpha=255):

    r, gr, b = rgb
    c = (r >> 3) << 11 | (gr >> 2) << 5 | (b >> 3)
    block = bytes([alpha, alpha]) + bytes(6) + struct.pack('<HH', c, c) + bytes(4)
    level = block * 128
    raw = bytearray(b'<DE(' + bytes(40))
    raw[4], raw[7], raw[23] = 0x3d, 1, 7
    struct.pack_into('<II', raw, 8, 16, 16)
    struct.pack_into('<II', raw, 36, len(level), 4)
    return bytes(raw + level)


def romfs(root, files):

    os.makedirs(os.path.join(root, 'data'), exist_ok=True)
    rows, data = ['FileId!STRING:0|KeyLayout!DEC:4|Chunk!DEC:8|Archive!DEC:8|CKey!HEX:16|CSize!DEC:4|EKey!HEX:16|ESize!DEC:4|'
                  'Encoding!STRING:0|Path!STRING:0|Flags!STRING:0'], bytearray()
    for path, content in sorted(files.items()):
        ekey = bytes(9) + bytes([0, 1]) + len(data).to_bytes(5, 'big')
        rows.append('|'.join(['x', '0', '0', '1', hashlib.md5(content).hexdigest(), str(len(content)), ekey.hex(), str(len(content)), 'n',
                              'data:' + path, 'table']))
        data += content
    with open(os.path.join(root, 'data', '00-00000001.data'), 'wb') as f:
        f.write(data)
    with open(os.path.join(root, 'data', 'build_manifest.psv'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(rows) + '\n')


class Archive:
    def __init__(self, files):
        self.files = files
        self.entries = {k: None for k in files}

    def read(self, path):
        key = path.lower()
        key = key if key.startswith('data/') else 'data/' + key
        if key not in self.files:
            raise g.PictureError('no ' + path)
        return self.files[key]


def tsv(rows):
    head = list(dict.fromkeys(h for r in rows for h in r))
    return ('\t'.join(head) + '\n' + ''.join('\t'.join(r.get(h, '') for h in head) + '\n' for r in rows)).encode()


def tables(drop_boss=None, by_difficulty=True):
    import gen_sim_data
    acts = sorted(i for i, (_, kind, _) in gen_sim_data.BOSSES.items() if kind == 'actboss')
    mon = [{'Id': 'boss%d' % i, '*hcIdx': str(i), 'TransLvl': '0', 'MonStatsEx': 'boss%d' % i} for i in acts]



    mon += [{'Id': 'Walker', '*hcIdx': '1', 'TransLvl': '0', 'MonStatsEx': 'Walker'}, {'Id': 'walker2', '*hcIdx': '2', 'TransLvl': '3', 'MonStatsEx': 'walker2'},
            {'Id': 'ghost', '*hcIdx': '3', 'TransLvl': '0', 'MonStatsEx': 'ghost'}, {'Id': 'walker3', '*hcIdx': '4', 'TransLvl': '0', 'MonStatsEx': 'walker3'},
            {'Id': 'walker4', '*hcIdx': '8', 'TransLvl': '0', 'MonStatsEx': 'walker4'}, {'Id': 'hidden', '*hcIdx': '9', 'TransLvl': '0', 'MonStatsEx': 'hidden'},
            {'Id': 'wisp', '*hcIdx': '10', 'TransLvl': '0', 'MonStatsEx': 'wisp'}, {'Id': 'odd', '*hcIdx': '11', 'TransLvl': '0', 'MonStatsEx': 'odd'},
            {'Id': 'rogue', '*hcIdx': '12', 'TransLvl': '0', 'MonStatsEx': 'walker4'},
            {'Id': 'walker5', '*hcIdx': '13', 'TransLvl': '4', 'MonStatsEx': 'walker5'}, {'Id': 'walker6', '*hcIdx': '14', 'TransLvl': '1', 'MonStatsEx': 'walker6'}]


    enemy = {'enabled': '1', 'killable': '1', 'NameStr': 'Name'}
    next(r for r in mon if r['Id'] == 'Walker')['minion1'] = 'minion'
    next(r for r in mon if r['Id'] == 'walker2')['Skill1'] = 'Call'
    next(r for r in mon if r['Id'] == 'walker2')['spawn'] = 'ally'
    mon += [{'Id': 'minion', '*hcIdx': '17', 'TransLvl': '0', 'MonStatsEx': 'minion', **enemy}, {'Id': 'summoned', '*hcIdx': '18', 'TransLvl': '0', 'MonStatsEx': 'summoned', **enemy},
            {'Id': 'ally', '*hcIdx': '19', 'TransLvl': '0', 'MonStatsEx': 'ally', 'Align': '1', **enemy}]
    for r in mon:
        for c in ('minion1', 'minion2', 'spawn', 'Align', 'npc', 'inTown', 'enabled', 'killable', 'neverCount', 'NameStr') + tuple('Skill%d' % k for k in range(1, 9)):
            r.setdefault(c, '')
    hd = {'boss%d' % i: 'boss%d' % i for i in acts if i != drop_boss}
    hd.update({'walker': 'walker1', 'walker2': 'walker1', 'walker3': 'walker1', 'walker4': 'walker1', 'hidden': 'walker1', 'wisp': 'willowisp1',
               'odd': 'odd1', 'rogue': 'walker1', 'walker5': 'walker1', 'walker6': 'walker1', 'minion': 'walker1', 'summoned': 'walker1', 'ally': 'walker1'})
    mon2 = [{'Id': r['Id'], 'TR': '1', 'TRv': 'nil,lit', 'Rav': 'med', 'RA': '1', 'HD': '0', 'HDv': 'lit'} for r in mon if r['Id'] != 'rogue']
    next(r for r in mon2 if r['Id'] == 'walker4')['RHv'] = 'axe'
    next(r for r in mon2 if r['Id'] == 'walker4')['RH'] = '1'


    def su_row(name, idx, cls, utrans):
        return {'Superunique': name, 'hcIdx': idx, 'Class': cls, 'Utrans': utrans[0], 'Utrans(N)': utrans[1], 'Utrans(H)': utrans[2]}
    su = [su_row('a', '5', 'boss%d' % acts[0], '222'), su_row('b', '6', 'walker', '999'), su_row('c', '7', 'nobody', '222'),
          {**su_row('d', '15', 'walker', '000'), 'Utrans': '30', 'Utrans(N)': '30', 'Utrans(H)': '30'}] + ([su_row('e', '16', 'walker', '223')] if by_difficulty else [])
    levels = [{'Id': '1', 'mon1': 'walker', 'mon2': 'walker2', 'nmon1': 'walker3', 'nmon2': 'ghost', 'mon5': 'walker5', 'mon6': 'walker6'},
              {'Id': '2', 'mon1': 'walker4', 'mon2': 'wisp', 'mon3': 'odd', 'mon4': 'rogue'}]
    files = {'data/global/excel/monstats.txt': tsv(mon), 'data/global/excel/monstats2.txt': tsv(mon2), 'data/global/excel/superuniques.txt': tsv(su),
             'data/global/excel/levels.txt': tsv(levels), 'data/hd/character/monsters.json': json.dumps(hd).encode(),
             'data/global/excel/monpreset.txt': tsv([{'Act': '1', 'Place': ''}]), 'data/global/excel/skills.txt': tsv([{'skill': 'Call', 'summon': 'summoned'}]),
             'data/local/lng/strings/monsters.json': json.dumps([{'id': 1, 'Key': 'Name', 'enUS': 'Name'}]).encode()}
    for definition in set(hd.values()):
        root = {'name': 'root', 'components': [{'type': 'SkeletonDefinitionComponent', 'filename': 'x.skeleton'}]}
        if definition == 'walker1':
            root['components'].append({'type': 'VariantDefinitionComponent', 'filename': 'data/hd/character/enemy/walker1/walker1_variant.json'})
        files['data/hd/character/enemy/%s.json' % definition] = json.dumps({'entities': [root]}).encode()
    shift = transform(x=0.5)
    entries = [[transform()] * 3 for _ in range(8)]
    entries[5] = entries[6] = [shift, transform(), transform()]
    files['data/hd/character/enemy/walker1/walker1_variant.json'] = json.dumps({'entries': [{'transforms': e} for e in entries]}).encode()
    files['data/hd/global/palette/randtransforms.json'] = json.dumps({'transforms': [transform(z=-0.5), transform(y=-1)]}).encode()
    return files, acts


def transform(x=0.0, y=0.0, z=0.0, w=1.0, tint=(0.0, 0.0, 0.0, 0.0), hue=(0.0,) * 6, sat=(0.0,) * 6):

    def v(a, b, c, d=0.0):
        return {'x': a, 'y': b, 'z': c, 'w': d}
    return {'colorAdjustment': v(x, y, z, w), 'colorTint': v(*tint), 'hueAdjustment0': v(*hue[:3]), 'hueAdjustment1': v(*hue[3:]),
            'satAdjustment0': v(*sat[:3]), 'satAdjustment1': v(*sat[3:])}




def identity_transform(position):
    return (tuple(position), (0, 0, 0, 1), (1, 0, 0, 0, 1, 0, 0, 0, 1))


def inverse(position):
    m = np.eye(4)
    m[:3, 3] = -np.asarray(position, float)
    return m.T.reshape(16).tolist()


def bone(name, parent, position):
    return [('Name', 'str', name), ('ParentIndex', 'i4', parent), ('Transform', 'transform', identity_transform(position)),
            ('InverseWorldTransform', 'f4', inverse(position))]


SKELETON = [('Name', 'str', 'fig'), ('Bones', 'records', [bone('Root', -1, (0, 0, 0)), bone('Arm', 0, (0, 1, 0))])]
ITEM_SKELETON = [('Name', 'str', 'axe'), ('Bones', 'records', [bone('grip', -1, (0, 0, 0))])]
VERTEX = np.dtype([('Position', '<f4', (3,)), ('BoneWeights', 'u1', (4,)), ('BoneIndices', 'u1', (4,)), ('Normal', '<f4', (3,)),
                   ('Tangent', '<f4', (4,)), ('TextureCoordinates0', '<f2', (2,))])


def material(name, albedo, mask=None):
    maps = [[('Usage', 'str', 'AlbedoTexture'), ('Map', 'ref', [('Texture', 'ref', [('FromFileName', 'str', albedo)])])]]
    if mask:
        maps.append([('Usage', 'str', 'MaskTexture'), ('Map', 'ref', [('Texture', 'ref', [('FromFileName', 'str', mask)])])])
    return [('Name', 'str', name), ('Maps', 'records', maps), ('ExtendedData', 'variant', [('technique', 'str', 'Standard')])]


def quad(name, z, bones, albedo, mask=None):


    v = np.zeros(4, VERTEX)
    v['Position'] = [(-0.5, 0, z), (0.5, 0, z), (0.5, 1, z), (-0.5, 1, z)]
    v['BoneWeights'] = [(255, 0, 0, 0)] * 4
    top = len(bones) - 1
    v['BoneIndices'] = [(0, 0, 0, 0), (0, 0, 0, 0), (top, 0, 0, 0), (top, 0, 0, 0)]
    v['Normal'] = [(0, 0, 1)] * 4
    v['Tangent'] = [(1, 0, 0, 1)] * 4
    v['TextureCoordinates0'] = [(0, 1), (1, 1), (1, 0), (0, 0)]
    idx = np.array([(i,) for i in (0, 1, 2, 0, 2, 3)], dtype=[('Int16', '<i2')])
    groups = np.array([(0, 0, 2), (1, 0, 2)], dtype=[('MaterialIndex', '<i4'), ('TriFirst', '<i4'), ('TriCount', '<i4')])
    sets = np.array([(0, 1, 0), (1, 1, 0)], dtype=[('FirstIndex', '<i4'), ('Count', '<i4'), ('NameHash', '<u4')])
    return [('Name', 'str', name), ('PrimaryVertexData', 'ref', [('Vertices', 'varray', v)]),
            ('BoneBindings', 'records', [[('BoneName', 'str', b)] for b in bones]),
            ('PrimaryTopology', 'ref', [('Groups', 'records', groups), ('Indices16', 'records', idx),
                                        ('Indices', 'records', np.zeros(0, dtype=[('Int32', '<i4')]))]),
            ('MaterialBindings', 'records', [[('Material', 'ref', material('alive', albedo, mask))], [('Material', 'ref', material('dead', albedo))]]),
            ('ExtendedData', 'variant', [('MaterialSets', 'records', sets), ('meshRenderLayer', 'i4', 1)])]


def model_file(skeleton, meshes):
    return Granny().file([('Models', 'refs', [[('Name', 'str', 'm'), ('Skeleton', 'ref', skeleton),
                                                ('MeshBindings', 'records', [[('Mesh', 'ref', m)] for m in meshes])]])])


def curve(header, *fields):
    return [('CurveData', 'variant', [('CurveDataHeader_' + header, 'u1', 0)] + list(fields))]


def fig_files(arm_bone='ARM'):



    files = {}
    files['data/hd/character/enemy/fig/skeleton/fig.skeleton'] = Granny().file([('Skeletons', 'refs', [SKELETON])])
    a = quad('a', 0, ['root', 'ARM'], 'data/hd/red.texture', 'data/hd/mask.texture')
    b = quad('b', -0.2, ['root', 'ARM'], 'data/hd/red.texture', 'data/hd/mask.texture')
    files['data/hd/character/enemy/fig/body_lod2.model'] = Granny().file([('Models', 'refs', [
        [('Name', 'str', 'a'), ('Skeleton', 'ref', SKELETON), ('MeshBindings', 'records', [[('Mesh', 'ref', a)]])],
        [('Name', 'str', 'b'), ('Skeleton', 'ref', SKELETON), ('MeshBindings', 'records', [[('Mesh', 'ref', b)]])]])])
    files['data/hd/character/enemy/fig/body_lod3.model'] = model_file(SKELETON, [])
    files['data/hd/character/enemy/fig/dead_lod2.model'] = model_file(SKELETON, [quad('dead', 0, ['root', 'ARM'], 'data/hd/red.texture')])
    files['data/hd/character/enemy/fig/legs_med_lod2.model'] = model_file(SKELETON, [quad('legs', 0, ['root', 'ARM'], 'data/hd/red.texture')])
    files['data/hd/items/axe/axe_lod2.model'] = model_file(ITEM_SKELETON, [quad('blade', 0, ['grip'], 'data/hd/blue.texture')])
    lift = [('Name', 'str', 'arm'), ('OrientationCurve', 'inline', curve('DaIdentity', ('Dimension', 'i2', 4))),
            ('PositionCurve', 'inline', curve('D3Constant32f', ('Controls', 'f4', [0, 2, 0]))),
            ('ScaleShearCurve', 'inline', curve('DaIdentity', ('Dimension', 'i2', 9)))]
    files['data/hd/character/enemy/fig/animation/combined.animations'] = Granny().file([('Animations', 'refs', [
        [('Name', 'str', 'Neutral'), ('TrackGroups', 'refs', [[('TransformTracks', 'records', [lift])]])]])])
    files['data/hd/red.texture'] = texture((248, 0, 0))
    files['data/hd/blue.texture'] = texture((0, 0, 248))
    files['data/hd/mask.texture'] = texture((248, 0, 0))
    half = math.radians(45) / 2
    root = {'name': 'root', 'components': [{'type': 'SkeletonDefinitionComponent', 'filename': 'data/hd/character/enemy/fig/skeleton/fig.skeleton'},
                                           {'type': 'UnitRootComponent', 'state_machine_filename': 'data/hd/fig_sm.json',
                                            'rotationOverride': {'x': 0, 'y': math.sin(half), 'z': 0, 'w': math.cos(half)}},
                                           {'type': 'VariantDefinitionComponent', 'filename': 'data/hd/character/enemy/fig/fig_variant.json'}]}

    def model(path):
        return {'type': 'ModelDefinitionComponent', 'filename': 'data/hd/character/enemy/fig/%s.model' % path, 'visibleLayers': 1}
    attach = {'type': 'UnitAttachmentComponent', 'filename': 'data/hd/items/axe.json', 'overrideBoneName': arm_bone, 'part': 'rhand',
              'attachmentTransform': {'translation': {'x': 3, 'y': 0, 'z': 0}, 'rotation': {'x': 0, 'y': 0, 'z': 90}, 'scale': {'x': 1, 'y': 1, 'z': 1}}}
    entities = [root, {'name': 'entity_body', 'components': [model('body')]},
                {'name': 'entity_spine_dead', 'components': [model('dead')]},
                {'name': 'entity_legs_med', 'components': [{'type': 'UnitPartComponent', 'part': 'legs', 'variant': 'med'}, model('legs_med')]},
                {'name': 'entity_legs_lit', 'components': [{'type': 'UnitPartComponent', 'part': 'legs', 'variant': 'lit'}, model('legs_lit')]},
                {'name': 'entity_special3_lit', 'components': [{'type': 'UnitPartComponent', 'part': 'special3', 'variant': 'lit'}, model('special3_lit')]},
                {'name': 'entity_rhand_axe', 'components': [{'type': 'UnitPartComponent', 'part': 'RHAND', 'variant': 'axe'}, model('rhand_axe'), attach]},
                {'name': 'entity_lhand', 'components': [{'type': 'UnitPartComponent', 'part': 'lhand', 'variant': 'lit'},
                                                        {'type': 'UnitAttachmentComponent', 'filename': '', 'part': 'lhand'}]}]
    files['data/hd/character/enemy/fig.json'] = json.dumps({'entities': entities}).encode()
    files['data/hd/fig_sm.json'] = json.dumps({'states': [{'modeId': 1, 'animationBindings': {'hth': ['neutral001']}}],
                                               'animations': [{'name': 'neutral001', 'filename': 'data/hd/character/enemy/fig/animation/neutral.animation'}]}).encode()
    files['data/hd/items/axe.json'] = json.dumps({'entities': [{'name': 'root', 'components': [{'type': 'ModelDefinitionComponent',
                                                                                                 'filename': 'data/hd/items/axe/axe.model'}]}]}).encode()
    entries = [[transform()] * 3 for _ in range(8)]
    entries[3] = [transform(x=0.5), transform(), transform()]
    files['data/hd/character/enemy/fig/fig_variant.json'] = json.dumps({'entries': [{'transforms': e} for e in entries]}).encode()
    return files


ROW2 = {'rh': '1', 'rhv': 'axe', 'lh': '1', 'lhv': 'lit', 'lg': '1', 'lgv': 'nil', 's3': '1', 's3v': 'lit'}


def job_tests():


    files = fig_files()
    arc = Archive(files)
    records = g.job(arc, 'fig', ROW2)
    kinds = [r[0] for r in records]
    check(kinds == ['skeleton', 'animation', 'colour', 'pose', 'turn', 'model', 'attach', 'item'], 'job steps: %s' % kinds)
    check(records[1][1:] == ('data/hd/character/enemy/fig/animation/combined.animations', 'neutral'), 'the clip of the neutral animation')
    check(len(records[2]) == 61 and abs(records[4][1]) < 1e-9, 'three colour transforms, the standard turn leaves the front at +z')
    check(records[5][1:] == (1, 'data/hd/character/enemy/fig/body.model', 'data/hd/character/enemy/fig/body_lod2.model'),
          'the most detailed level of the body: %s' % (records[5],))
    check(records[6][1] == 'arm' and records[6][2:] == (0.0, 0.0, 90.0, 1.0, 1.0, 1.0, 3.0, 0.0, 0.0), 'the attachment: %s' % (records[6],))
    check(records[7][3] == 'data/hd/items/axe/axe_lod2.model', 'the item model')
    check(g.job(arc, 'fig', ROW2, 3)[2][1] == 0.5, 'colour index 3 of the definition\'s table')

    fail = g.job(arc, 'fig', dict(ROW2, lgv='lit'))[-1]
    check(fail[:2] == ('fail', 'unsupported') and 'legs' in fail[2] and 'legs_lit' in fail[2], 'missing part variant named: %s' % (fail,))

    broken = dict(files)
    del broken['data/hd/character/enemy/fig/body_lod2.model'], broken['data/hd/character/enemy/fig/body_lod3.model']
    fail = g.job(Archive(broken), 'fig', ROW2)[-1]
    check(fail[:2] == ('fail', 'error') and 'fig' in fail[2] and 'body.model' in fail[2], 'missing body names definition and path: %s' % (fail,))
    bad = json.loads(files['data/hd/character/enemy/fig.json'])
    bad['entities'] = [e for e in bad['entities'] if e['name'] != 'root']
    files['data/hd/character/enemy/nofig.json'] = json.dumps(bad).encode()
    check(g.job(Archive(files), 'nofig', ROW2) == [('fail', 'unsupported', 'no skeleton')], 'definition without a skeleton')
    files['data/hd/fig_sm.json'] = json.dumps({'states': [], 'animations': []}).encode()
    fail = g.job(Archive(files), 'fig', ROW2)
    check([r[0] for r in fail] == ['skeleton', 'fail'] and fail[1][1] == 'unsupported', 'definition without a neutral animation: %s' % fail)
    files['data/hd/character/enemy/fig.json'] = b'{"entities": 7}'
    check(g.job(Archive(files), 'fig', ROW2)[-1][:2] == ('fail', 'error'), 'an unexpected shape of the files is an error')


def library_tests():



    from PIL import Image
    ranges = (2.0, -1.0) * 16
    with tempfile.TemporaryDirectory() as t:
        files = fig_files()
        romfs(os.path.join(t, 'romfs'), files)
        arc = g.Archive(os.path.join(t, 'romfs'))
        jobs = {'fig': g.job(arc, 'fig', ROW2), 'fig_c3': g.job(arc, 'fig', ROW2, 3)}
        out = os.path.join(t, 'out')
        os.makedirs(os.path.join(out, 'mon', 's'))
        outcome = g.render(os.path.join(t, 'romfs'), jobs, ranges, out, 2)
        check(outcome == {'fig': ('ok', ''), 'fig_c3': ('ok', '')}, 'rendered: %s' % outcome)
        big = np.asarray(Image.open(os.path.join(out, 'mon', 'fig.webp')).convert('RGBA')).astype(int)
        small = np.asarray(Image.open(os.path.join(out, 'mon', 's', 'fig.webp')).convert('RGBA'))
        check(big.shape == (256, 256, 4) and small.shape == (96, 96, 4), 'both sizes')
        opaque = big[:, :, 3] > 250
        red = opaque & (big[:, :, 0] > 120) & (big[:, :, 2] < 80)
        blue = opaque & (big[:, :, 2] > 120) & (big[:, :, 0] < 80)
        rows, cols = np.nonzero(red)
        check(red.sum() > 1000 and (rows.max() - rows.min()) > 1.4 * (cols.max() - cols.min()), 'the lifted arm stretches the body')
        brows, bcols = np.nonzero(blue)
        check(blue.sum() > 300 and bcols.min() > cols.max(), 'the item hangs right of the body in its own colour')
        cyan = np.asarray(Image.open(os.path.join(out, 'mon', 'fig_c3.webp')).convert('RGBA')).astype(int)
        body = cyan[rows, cols]
        check(body[:, 1].mean() > body[:, 0].mean() + 40 and body[:, 2].mean() > body[:, 0].mean() + 40, 'colour index 3 turns the body cyan')
        again = os.path.join(t, 'again')
        os.makedirs(os.path.join(again, 'mon', 's'))
        g.render(os.path.join(t, 'romfs'), jobs, ranges, again, 1)
        check(all(open(os.path.join(out, 'mon', p), 'rb').read() == open(os.path.join(again, 'mon', p), 'rb').read()
                  for p in ('fig.webp', 's/fig.webp', 'fig_c3.webp')), 'one thread or two: the same files')

        import io
        b = io.BytesIO()
        Image.fromarray(np.asarray(Image.open(os.path.join(out, 'mon', 's', 'fig.webp')).convert('RGBA')), 'RGBA').save(b, 'WEBP', lossless=True, quality=100, method=6)
        check(b.getvalue() == open(os.path.join(out, 'mon', 's', 'fig.webp'), 'rb').read(), 'the lossless file pillow writes for its pixels')

        files = fig_files(arm_bone='tail')
        files['data/hd/character/enemy/fig/legs_med_lod2.model'] = b'damaged'
        romfs(os.path.join(t, 'odd'), files)
        arc = g.Archive(os.path.join(t, 'odd'))
        jobs = {'fig': g.job(arc, 'fig', ROW2), 'legs': g.job(arc, 'fig', dict(ROW2, lgv='med'))}
        outcome = g.render(os.path.join(t, 'odd'), jobs, ranges, out, 2)
        check(outcome['fig'] == ('unsupported', "attachment on bone 'tail', which the skeleton lacks"), 'missing attach bone: %s' % (outcome,))
        check(outcome['legs'][0] == 'error' and 'legs_med_lod2.model' in outcome['legs'][1], 'damaged model file: %s' % (outcome,))
        raises(g.PictureError, lambda: g.render(os.path.join(t, 'nothing'), jobs, ranges, out, 1), 'a RomFS without manifest stops the library')


def tolerance_tests():


    from PIL import Image
    rng = np.random.default_rng(5)
    base = np.zeros((96, 96, 4), np.uint8)
    base[10:80, 20:70] = (180, 60, 40, 255)

    def folder(path, pic, json_text='{}', extra=False):
        os.makedirs(os.path.join(path, 'mon', 's'))
        Image.fromarray(pic, 'RGBA').save(os.path.join(path, 'mon', 's', 'a.webp'), 'WEBP', lossless=True)
        if extra:
            Image.fromarray(pic, 'RGBA').save(os.path.join(path, 'mon', 's', 'b.webp'), 'WEBP', lossless=True)
        open(os.path.join(path, 'mon.json'), 'w').write(json_text)
        return path
    with tempfile.TemporaryDirectory() as t:
        ref = folder(os.path.join(t, 'ref'), base)
        noisy = base.copy()
        noisy[:, :, :3] = np.clip(noisy[:, :, :3].astype(int) + rng.integers(-3, 4, (96, 96, 3)), 0, 255)
        noisy[base[:, :, 3] == 0] = 0
        check(pic_tolerance.compare(ref, folder(os.path.join(t, 'same'), base))[0] == [], 'equal pictures pass')
        check(pic_tolerance.compare(ref, folder(os.path.join(t, 'noisy'), noisy))[0] == [], 'small colour noise passes')
        edge = base.copy()
        edge[10, 20] = 0
        check([p for _, p in pic_tolerance.compare(ref, folder(os.path.join(t, 'edge'), edge))[0]] == ['1 alpha values differ'], 'a changed silhouette fails')
        shifted = base.copy()
        shifted[10:80, 20:70, 1] = 120
        check(len(pic_tolerance.compare(ref, folder(os.path.join(t, 'shift'), shifted))[0]) == 1, 'a large colour change fails')
        check(pic_tolerance.compare(ref, folder(os.path.join(t, 'more'), base, extra=True))[0] == [('mon/s/b.webp', 'only in the candidate')],
              'an extra picture fails')
        check(pic_tolerance.compare(ref, folder(os.path.join(t, 'json'), base, '{"x": 1}'))[0] == [('mon.json', 'differs')], 'mon.json must be equal')


def tool_tests():
    check(g.chosen_parts({'tr': '1', 'trv': 'lit,med,hvy', 'ra': '1', 'rav': 'MED', 'hd': '0', 'hdv': 'lit', 'sh': '1', 'shv': 'nil'})
          == {'torso': 'med', 'rarm': 'med'}, 'parts: on, the middle of equally frequent variants, columns without case, nil leaves the part out')
    check(g.typical(['lit', 'lit', 'lit', 'med', 'hvy', 'hvy', 'hvy']) == 'hvy' and g.typical(['axe', 'hax', 'hax', 'mac']) == 'hax'
          and g.typical(['nil', 'nil', 'lit']) is None and g.typical(['nil', 'sml', 'buc', 'tch']) == 'buc' and g.typical([]) is None,
          'typical variant: the most frequent entry, of tied ones the middle in list order')
    real, g.TYPICAL = g.TYPICAL, {'fig': {'TR': 0, 'SH': 1}}
    try:
        check(g.chosen_parts({'tr': '1', 'trv': 'lit,med,hvy', 'sh': '1', 'shv': 'nil,buc'}, 'fig') == {'torso': 'lit', 'shield': 'buc'}
              and g.chosen_parts({'tr': '1', 'trv': 'lit,med,hvy'}, 'other') == {'torso': 'med'}, 'a definition\'s own pick by index')
        raises(g.PictureError, lambda: g.chosen_parts({'sh': '1', 'shv': 'nil'}, 'fig'), 'a pick outside the variant list')
    finally:
        g.TYPICAL = real
    files, acts = tables()
    draw, none, jobs, bosses = g.plan(Archive(files))
    check(sorted(bosses) == ['boss%d' % i for i in acts] and all(draw['boss%d' % i] == 'boss%d' % i for i in acts), 'act bosses drawn')

    check(draw['walker'] == draw['walker3'] == 'walker1_2' and draw['walker4'] == draw['rogue'] == 'walker1', 'looks of one definition: shared or own: %s' % draw)
    check(jobs['walker1'][0] == jobs['walker1_2'][0] == 'walker1' and jobs['walker1'][1]['rhv'] == 'axe', 'a look renders its definition with its parts')
    check(none['ghost'] == 'nomodel' and none['hidden'] == 'unlisted' and none['wisp'] == 'effect', 'reasons for no picture: %s' % none)
    check('minion' in draw and 'summoned' in draw and none['ally'] == 'unlisted', 'the book\'s minions and summons get pictures, the player\'s side none: %s' % none)
    check(draw['walker2'] == draw['walker5'] == 'walker1_2_c5' and jobs['walker1_2_c5'][2] == 5 and draw['walker6'] == 'walker1_2',
          'colour levels: index level + 2, equal colours share a picture, colours that change nothing share the base picture')
    check(draw['su:5'] == 'boss%d' % acts[0] and draw['su:6'] == 'walker1_2_c9' and none['su:7'] == 'nomodel', 'superunique keys take their class')
    check(draw['su:15'] == draw['walker'] and 'su:15' not in none, 'a superunique colour the files lack: its class\'s picture')
    check(none['su:16'].startswith('unsupported: colours'), 'superunique colours that differ by difficulty: no picture')
    check(sorted(k for k, v in none.items() if not g.documented(v)) == ['su:16'], 'only the gap of the tool is undocumented: %s' % none)
    check(all(g.documented(r) for r in ('unlisted', 'nomodel', 'effect', 'rejected: x')) and not any(g.documented(r) for r in (
          'unsupported: no visible mesh', 'colour: the game files hold no colour 40', 'effects', '')), 'documented reasons for no picture')
    real_effects, g.EFFECTS = g.EFFECTS, frozenset()
    try:
        draw2, none2, _, _ = g.plan(Archive(files))
        check(draw2['wisp'] == 'willowisp1' and 'wisp' not in none2, 'a definition off the effects list gets its picture')
    finally:
        g.EFFECTS = real_effects
    check(all(len(j) == 3 for j in jobs.values()), 'a job names definition, monstats2 row and colour index')
    real_unverified, g.UNVERIFIED_COLOURS = g.UNVERIFIED_COLOURS, {'walker2': 'not seen in the game'}
    try:
        draw2, none2, _, _ = g.plan(Archive(files))
        check(none2['walker2'] == 'colour: not seen in the game' and draw2['walker5'] == 'walker1_2_c6', 'an unverified colour: no picture, with its reason; the others keep theirs')
    finally:
        g.UNVERIFIED_COLOURS = real_unverified


    good = json.dumps(transform())
    for bad in (good.replace('"z": 0.0', '"z": NaN', 1), good.replace('"x": 0.0', '"x": Infinity', 1),
                good.replace('"y": 0.0', '"y": -Infinity', 1), good.replace('"z": 0.0', '"z": 1e309', 1), good.replace('"x": 0.0', '"x": "0"', 1),
                good.replace('"x": 0.0', '"x": true', 1), good.replace('"colorTint"', '"colourTint"', 1)):
        assert bad != good, bad
        broken = dict(files)
        broken['data/hd/character/enemy/walker1/walker1_variant.json'] = json.dumps(
            {'entries': [{'transforms': [transform()] * 3}] * 5 + [{'transforms': [transform(), json.loads(bad), transform()]}] + [{'transforms': [transform()] * 3}] * 2}).encode()
        raises(g.PictureError, lambda: g.plan(Archive(broken)), 'damaged colour table refused: ' + bad[:60])
        broken = dict(files)
        broken['data/hd/global/palette/randtransforms.json'] = ('{"transforms": [%s, %s]}' % (good, bad)).encode()
        raises(g.PictureError, lambda: g.plan(Archive(broken)), 'damaged shared colours refused: ' + bad[:60])
    check(g.checked_transform(transform(x=0.25, w=0.5), 'x') == transform(x=0.25, w=0.5), 'a sound transform passes unchanged')
    t = g.colour_transforms(None, [transform(x=0.1)], 8)
    check(g.colour_transforms(None, [], 3) == [g.IDENTITY] * 3 and t == [transform(x=0.1)] * 3 and g.colour_transforms(None, [], 8) is None
          and all(g.identity(x) for x in g.colour_transforms(None, [], 2)), 'colour index: own table, no table, shared transforms')
    check(not g.identity(transform(w=0.9)) and not g.identity(transform(hue=(0, 0, 0, 0, 0.1, 0))), 'a transform that changes colours')
    check(g.transform_numbers(transform(x=0.25, w=0.5, tint=(1, 2, 3, 4), hue=(5, 6, 7, 8, 9, 10), sat=(11, 12, 13, 14, 15, 16))) ==
          [0.25, 0, 0, 0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], 'a transform in the order of the job file')
    check(jobs['boss%d' % acts[0]][1]['trv'] == 'nil,lit', 'monstats2 row of the drawn definition')
    files2, _ = tables(drop_boss=acts[1], by_difficulty=False)
    files1, _ = tables(by_difficulty=False)
    with tempfile.TemporaryDirectory() as t:
        real = g.render, g.quaternion_ranges, g.Archive
        g.quaternion_ranges = lambda main: (0.0,) * 32
        try:
            from PIL import Image
            pic = np.zeros((256, 256, 4), np.uint8); pic[40:200, 60:180] = (200, 30, 20, 255)
            small = np.zeros((96, 96, 4), np.uint8); small[20:70, 30:60] = (200, 30, 20, 255)

            def library(outcome_of):

                def render(romfs, jobs, ranges, out, threads):
                    check(all(isinstance(r, list) and r for r in jobs.values()) and len(ranges) == 32, 'jobs and ranges handed over')
                    outcome = {}
                    for name in jobs:
                        outcome[name] = outcome_of(name)
                        if outcome[name][0] == 'ok':
                            Image.fromarray(pic, 'RGBA').save(os.path.join(out, 'mon', name + '.webp'), 'WEBP', quality=90)
                            Image.fromarray(small, 'RGBA').save(os.path.join(out, 'mon', 's', name + '.webp'), 'WEBP', lossless=True)
                    return outcome
                return render
            g.render = library(lambda name: ('ok', ''))
            g.Archive = lambda romfs: Archive(files1)
            out = os.path.join(t, 'ok')
            g.stage('romfs', out, 'main', 2)
            data = json.load(open(os.path.join(out, 'mon.json')))
            check(set(data) == {'pictures', 'none'} and data['pictures']['su:5'] == 'boss%d' % acts[0], 'mon.json written')
            check(data['none']['ghost'] == 'nomodel' and data['none']['wisp'] == 'effect' and data['pictures']['su:15'] == data['pictures']['walker'],
                  'documented reasons pass, a missing superunique colour shows its class')
            check(g.missing(out) == [], 'complete pack')
            check(sorted(os.listdir(os.path.join(out, 'mon', 's'))) == sorted(n + '.webp' for n in set(data['pictures'].values())), 'one file per picture')
            os.remove(os.path.join(out, 'mon', 's', 'boss%d.webp' % acts[2]))
            check(g.missing(out) == ['mon/s/boss%d.webp' % acts[2]], 'missing small picture found')
            for bad in ('{', '[]', json.dumps({'pictures': {}, 'none': {}}), json.dumps({'pictures': {'a': '../x'}, 'none': {}}),
                        json.dumps({'pictures': {'a': 'a'}, 'none': {'a': 'colour'}}), json.dumps({'pictures': {'a': 'a'}})):
                open(os.path.join(out, 'mon.json'), 'w').write(bad)
                check(g.missing(out) == ['mon.json'], 'malformed mon.json refused: ' + bad)



            g.render = library(lambda name: ('unsupported', 'no visible mesh') if name.startswith('odd1') else ('ok', ''))
            try:
                g.stage('romfs', os.path.join(t, 'oddgap'), 'main', 1)
                raise AssertionError('an enemy of the book without a picture did not stop the build')
            except SystemExit as e:
                check('odd (unsupported: no visible mesh)' in str(e), 'an enemy the pictures cannot draw stops the build: %s' % e)
            g.render = library(lambda name: ('ok', ''))
            g.Archive = lambda romfs: Archive(files)
            try:
                g.stage('romfs', os.path.join(t, 'sugap'), 'main', 1)
                raise AssertionError('a superunique without a picture did not stop the build')
            except SystemExit as e:
                check('su:16' in str(e) and 'odd' not in str(e), 'a superunique without a documented reason stops the build: %s' % e)
            g.Archive = lambda romfs: Archive(files1)
            g.render = library(lambda name: ('error', "KeyError: 'entities'"))
            try:
                g.stage('romfs', os.path.join(t, 'odd'), 'main', 1)
                raise AssertionError('an unexpected shape of the game files did not stop the build')
            except SystemExit as e:
                check('enemy picture of' in str(e) and 'KeyError' in str(e), 'an unexpected shape names the definition: %s' % e)
            g.render = library(lambda name: ('unsupported', 'no visible mesh') if name.startswith('boss') else ('ok', ''))
            raises(SystemExit, lambda: g.stage('romfs', os.path.join(t, 'bossodd'), 'main', 1), 'an act boss the pictures cannot draw stops the build')
            g.render = library(lambda name: ('ok', ''))
            g.Archive = lambda romfs: Archive(files2)
            raises(SystemExit, lambda: g.stage('romfs', os.path.join(t, 'noboss'), 'main', 1), 'act boss without a picture stops the build')
            g.Archive = lambda romfs: Archive(files1)
            limit, g.MAX_BYTES = g.MAX_BYTES, 100
            try:
                raises(SystemExit, lambda: g.stage('romfs', os.path.join(t, 'big'), 'main', 1), 'pictures over the size limit stop the build')
            finally:
                g.MAX_BYTES = limit
        finally:
            g.render, g.quaternion_ranges, g.Archive = real
        exe = os.path.join(t, 'main')
        open(exe, 'wb').write(b'NSO0' + bytes(60))
        raises(g.PictureError, lambda: g.quaternion_ranges(exe), 'an executable that is not the verified one is refused')
        raises(g.PictureError, lambda: g.stage('romfs', os.path.join(t, 'exe'), exe), 'the build checks the executable first')




def sections(raw):

    _, total, _, table, count = struct.unpack_from('<5I', raw, 0x20)
    out = []
    for i in range(count):
        fmt, off, stored, size, _, _, _, fix, nfix, _, _ = struct.unpack_from('<11I', raw, 0x20 + table + 44 * i)
        if fmt == 4 and size:
            out.append((raw[off:off + stored], size))
        if fmt == 4 and nfix:
            n = struct.unpack_from('<I', raw, fix)[0]
            out.append((raw[fix + 4:fix + 4 + n], 12 * nfix))
    return out



SPARSE = {'swarm1': 0.015}


def game_tests(game):




    import ctypes
    import time
    import mon_decoder
    from PIL import Image
    main = os.path.join(game, 'ExeFS', 'main')
    romfs_dir = os.path.join(game, 'RomFS')
    arc = g.Archive(romfs_dir)
    draw, none, jobs, bosses = g.plan(arc)
    records = {name: g.job(arc, *j) for name, j in jobs.items()}
    paths = sorted({r[3] if r[0] in ('model', 'item') else r[1] for rs in records.values() for r in rs if r[0] in ('skeleton', 'animation', 'model', 'item')})
    oracle = mon_decoder.GameDecoder(main)
    lib = g.lib()
    seen, streams, start = set(), 0, time.time()
    for p in paths:
        for src, size in sections(arc.read(p)):
            key = hashlib.sha256(src).digest() + struct.pack('<I', size)
            if key in seen:
                continue
            seen.add(key)
            out = ctypes.create_string_buffer(size)
            check(lib.monpic_bitknit_decode(src, len(src), out, size) == 0 and out.raw == oracle.decode(src, size),
                  '%s: a stream of %d bytes decodes as the game decodes it' % (p, size))
            streams += 1
    print('real data: %d unique compressed streams of %d files equal to the game\'s decoder (%.1f s)' % (streams, len(paths), time.time() - start))
    with tempfile.TemporaryDirectory() as t:


        sys.path.insert(0, str(dm / 'tools'))
        import pack_steps
        b = pack_steps.Build(dm=str(dm), exefs=os.path.join(game, 'ExeFS'), romfs=romfs_dir, stage=os.path.join(t, 'stage'),
                             work=os.path.join(t, 'work'), python=sys.executable, monpic=os.path.abspath(lib._name))
        out = os.path.join(b.stage, 'monpics')
        start = time.time()
        try:
            pack_steps.run(b, ['pictures'], expect=pack_steps.load_expect(str(dm / 'tools' / 'pack_steps.tsv')))
        except pack_steps.StepError as e:
            log = os.path.join(b.work, 'build', 'pack_steps', 'pictures.log')
            tail = open(log, encoding='utf-8', errors='replace').read()[-2000:] if os.path.isfile(log) else ''
            check(False, 'the pictures step as the phone runs it: %s\n%s' % (e.detail, tail))
        took = time.time() - start
        data = json.load(open(os.path.join(out, 'mon.json')))
        names = sorted(set(data['pictures'].values()))
        for name in names:
            for rel, size in (('mon/%s.webp', 256), ('mon/s/%s.webp', 96)):
                a = np.asarray(Image.open(os.path.join(out, rel % name)).convert('RGBA'))[:, :, 3] > 0
                rows_on, cols_on = np.nonzero(a.any(1))[0], np.nonzero(a.any(0))[0]
                span = max(rows_on.max() - rows_on.min(), cols_on.max() - cols_on.min()) + 1
                least = SPARSE.get(jobs[name][0], 0.04)
                check(a.shape == (size, size) and a.mean() > least and span >= size * 0.9, '%s: the figure fills its picture' % (rel % name))
        again = os.path.join(t, 'again')
        os.makedirs(os.path.join(again, 'mon', 's'))
        boss_pictures = sorted({draw[b] for b in bosses})
        outcome = g.render(romfs_dir, {n: records[n] for n in boss_pictures}, g.quaternion_ranges(main), again, 3)
        check(all(outcome[n][0] == 'ok' and open(os.path.join(out, 'mon', n + '.webp'), 'rb').read() == open(os.path.join(again, 'mon', n + '.webp'), 'rb').read()
                  for n in boss_pictures), 'act bosses: same bytes twice')
        fallen = []
        for k in ('fallen1', 'fallen2'):
            px = np.asarray(Image.open(os.path.join(out, 'mon', 's', data['pictures'][k] + '.webp')).convert('RGBA')).astype(float)
            fallen.append(px[:, :, :3][px[:, :, 3] > 200].mean(0))
        check(fallen[0][0] > fallen[0][2] and fallen[1][2] > fallen[1][0], 'the fallen are red, their second level blue: %s' % fallen)
        reasons = {}
        for v in data['none'].values():
            reasons[v.split(':')[0]] = reasons.get(v.split(':')[0], 0) + 1
        check(set(reasons) == {'effect', 'unlisted'}, 'reasons %s' % reasons)

        effects = sorted(k for k, v in data['none'].items() if v != 'unlisted')
        check(effects == ['colbarbfrenzy', 'colbarbthrow', 'colbarbwhirl', 'su:33', 'willowisp1', 'willowisp3', 'willowisp5', 'willowisp6',
                          'willowisp7'], 'the book\'s enemies without a picture: %s' % effects)
        print('real data: %d pictures for %d look keys in %.1f s, without picture %s' % (len(names), len(data['pictures']), took, reasons))


def main():
    tool_tests()
    job_tests()
    library_tests()
    tolerance_tests()
    game = os.environ.get('D2R_GAME')
    if game:
        game_tests(os.path.abspath(game))
    else:
        print('test_mon_pics: SKIPPED real data (set D2R_GAME to the Update directory with ExeFS and RomFS)')
    print('test_mon_pics: %d checks passed' % checks)


if __name__ == '__main__':
    main()
