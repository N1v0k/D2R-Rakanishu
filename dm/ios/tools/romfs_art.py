









import ctypes as C, io, os, struct, subprocess, tempfile
from PIL import Image
import touch_layout

HERE = os.path.dirname(os.path.abspath(__file__)); DM = os.path.normpath(os.path.join(HERE, '..', '..'))


from gen_paths import gen_dir as out_dir


class Desc(C.Structure): _fields_ = [('target', C.c_int), ('format', C.c_int), ('flags', C.c_int), ('samples', C.c_int), ('width', C.c_uint32), ('height', C.c_uint32),
                                     ('depth', C.c_uint32), ('levels', C.c_uint32), ('stride', C.c_uint32), ('pkg_block_h', C.c_int)]
class Layout(C.Structure): _fields_ = [('size', C.c_uint64), ('layer_size', C.c_uint64), ('align', C.c_uint32), ('layers', C.c_uint32), ('bpb', C.c_uint32), ('tileH', C.c_uint), ('tileD', C.c_uint)]
_lib = None


def lib():

    global _lib
    if _lib is None:


        p = os.environ.get('DM_DESWIZZLE_LIB')


        win = ['-Wl,/EXPORT:nvn_tex_layout', '-Wl,/EXPORT:nvn_deswizzle', '-Wl,/EXPORT:nvn_swizzle'] if os.name == 'nt' else ['-fPIC']
        if not p:
            p = os.path.join(tempfile.mkdtemp(), 'deswizzle.dll' if os.name == 'nt' else 'deswizzle.dylib')
            subprocess.run(['clang', '-O2', '-shared', *win, '-I' + os.path.join(DM, 'gfx', 'nvn'), '-I' + os.path.join(DM, 'gfx', 'tex'), '-o', p,
                            os.path.join(DM, 'gfx', 'tex', 'nvn_deswizzle.c'), os.path.join(DM, 'gfx', 'nvn', 'nvn_tex.c')], check=True)
        L = C.CDLL(p); L.nvn_tex_layout.argtypes = [C.POINTER(Desc), C.POINTER(Layout)]
        L.nvn_deswizzle.argtypes = [C.POINTER(Desc), C.c_uint, C.c_uint, C.c_void_p, C.c_void_p, C.c_size_t]
        L.nvn_swizzle.argtypes = [C.POINTER(Desc), C.c_uint, C.c_uint, C.c_void_p, C.c_size_t, C.c_void_p]
        _lib = L
    return _lib


def read(romfs, path):
    want = path.lower().lstrip('/')
    if not want.startswith('data/'): want = 'data/' + want
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) >= 11 and f[9].lower().split(':', 1)[-1] == want: return touch_layout._read_entry(romfs, f)
    raise SystemExit('romfs_art：游戏文件里没有 %s（RomFS 版本不对？要 3.4.95035）' % path)


def dxt5(b, w, h):
    d = Desc(1, 0x44, 0, 0, w, h, 1, 1, 0, b[23] + 1); pitch = (w + 3) // 4 * 16
    lin = C.create_string_buffer(pitch * ((h + 3) // 4)); src = C.create_string_buffer(b[40:], len(b) - 40)
    assert lib().nvn_deswizzle(C.byref(d), 0, 0, src, lin, pitch) == 0
    hdr = [124, 0x81007, h, w, len(lin), 0, 1] + [0] * 11 + [32, 4, struct.unpack('<I', b'DXT5')[0], 0, 0, 0, 0, 0] + [0x1000, 0, 0, 0, 0]
    return Image.open(io.BytesIO(b'DDS ' + struct.pack('<31I', *hdr) + lin.raw)).convert('RGBA')


def sprite(romfs, path):
    b = read(romfs, path); assert b[:4] == b'SpA1', path
    ver, fw, w, h = struct.unpack_from('<HHII', b, 4)
    if ver == 61: return dxt5(b, w, h)
    if ver == 31: return Image.frombytes('RGBA', (w, h), b[40:40 + w * h * 4])
    raise SystemExit('romfs_art：%s 精灵图版本 %d 不认识' % (path, ver))


def frame(romfs, path, i=0):

    b = read(romfs, path); fw = struct.unpack_from('<H', b, 6)[0]; im = sprite(romfs, path)
    return im.crop((i * fw, 0, (i + 1) * fw, im.height))


_fonts = {}
def font(romfs, name):
    if name not in _fonts:
        p = os.path.join(tempfile.mkdtemp(), os.path.basename(name)); open(p, 'wb').write(read(romfs, 'hd/ui/fonts/' + name)); _fonts[name] = p
    return _fonts[name]
