#!/usr/bin/env python3















import argparse, glob, hashlib, json, os, pathlib, re, shutil, subprocess, tempfile

from pack_game_shaders import pack

DM = pathlib.Path(__file__).resolve().parents[2]
SOURCE = DM / 'gfx/vulkan/kernels/builtin_queries.comp'


MODES = ('q_val0', 'q_val8', 'q_dlt0', 'q_dlt8')
DESCRIPTORS = [
    {'name': 'count', 'binding': 0, 'type': 'uniform_buffer', 'count': 1, 'block': {'bytes': 16},
     'source': {'category': 'nvn', 'nvn': {'kind': 'uniform', 'slot': 0}}},
    {'name': 'src', 'binding': 1, 'type': 'storage_buffer', 'count': 1,
     'source': {'category': 'nvn', 'nvn': {'kind': 'storage_pointer', 'slot': 0}}},
    {'name': 'dst', 'binding': 2, 'type': 'storage_buffer', 'count': 1,
     'source': {'category': 'nvn', 'nvn': {'kind': 'storage_pointer', 'slot': 1}}},
]


def find_glslc(given):
    found = given or os.environ.get('GLSLC') or shutil.which('glslc')
    if found:
        return found
    roots = [os.environ.get(v) for v in ('ANDROID_NDK_HOME', 'ANDROID_NDK_ROOT')]
    for sdk in (os.environ.get('ANDROID_HOME'), os.environ.get('ANDROID_SDK_ROOT')):
        if sdk:
            roots += sorted(glob.glob(os.path.join(sdk, 'ndk', '*')), reverse=True)
    for root in filter(None, roots):
        hits = glob.glob(os.path.join(root, 'shader-tools', '*', 'glslc*'))
        if hits:
            return hits[0]
    raise SystemExit('glslc not found: pass --glslc, set GLSLC, or install the Android NDK')




PINNED_GLSLC = ('shaderc v2026.3 v2026.3', 'spirv-tools v2026.3 v2022.4-1283-gb707790a', 'glslang 11.1.0-1493-g168d452a')


def pinned_glslc():

    glslc = os.environ.get('GLSLC')
    if not glslc or not os.path.isfile(glslc):
        raise SystemExit('GLSLC must name the glslc of the Vulkan SDK 1.4.357.0 (%s)' % PINNED_GLSLC[0])
    got = tuple(line.strip() for line in subprocess.run([glslc, '--version'], capture_output=True, text=True).stdout.splitlines()[:3])
    if got != PINNED_GLSLC:
        raise SystemExit('GLSLC %s is %s, the query kernels need %s' % (glslc, ' / '.join(got) or 'no version', ' / '.join(PINNED_GLSLC)))
    return glslc


def kernel_keys():
    text = (DM / 'gfx/nvn/nvn_builtin.h').read_text(encoding='utf-8')
    keys = {name: key for key, name in re.findall(r'\{"([0-9a-f]{16})", "(q_\w+)"\}', text)}
    missing = [name for name in MODES if name not in keys]
    if missing:
        raise SystemExit(f'nvn_builtin.h has no key for {missing}')
    return keys


def main():
    ap = argparse.ArgumentParser(description='build the 4 query-kernel .xgs bundles from builtin_queries.comp')
    ap.add_argument('--output', type=pathlib.Path, required=True)
    ap.add_argument('--glslc')
    ap.add_argument('--pinned', action='store_true', help='only the pinned compiler ($GLSLC, PINNED_GLSLC), as for the APK')
    args = ap.parse_args()
    if args.output.resolve().is_relative_to(DM):
        raise SystemExit('output must be outside dm/')
    if args.pinned and args.glslc:
        raise SystemExit('--pinned takes the compiler from GLSLC only')
    glslc, keys = pinned_glslc() if args.pinned else find_glslc(args.glslc), kernel_keys()
    args.output.mkdir(parents=True, exist_ok=False)
    with tempfile.TemporaryDirectory() as work:
        for mode, name in enumerate(MODES):
            key = keys[name]
            spv = pathlib.Path(work, key + '.spv')
            subprocess.run([glslc, '-fshader-stage=comp', f'-DMODE={mode}', str(SOURCE), '-o', str(spv)], check=True)
            reflection = {'identity': {'key': key, 'name': name}, 'stage': 'cs', 'descriptors': DESCRIPTORS,
                          'frozen_job': {}, 'sha256': hashlib.sha256(spv.read_bytes()).hexdigest()}
            meta = pathlib.Path(work, key + '.vk.json')
            meta.write_text(json.dumps(reflection, indent=2) + '\n', encoding='utf-8')
            key, data = pack(meta)
            (args.output / (key + '.xgs')).write_bytes(data)
            print(f'{key}.xgs {name} {len(data)} bytes sha256 {hashlib.sha256(data).hexdigest()}')


if __name__ == '__main__':
    main()
