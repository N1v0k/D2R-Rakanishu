#!/usr/bin/env python3













import ctypes
import hashlib
import json
import os
import sys
import time

PART_SIZE = 256
OUT = 'shaders'
STEPS = {1: 'translate', 2: 'binding rewrite', 3: 'trim', 4: 'format capabilities', 5: 'validate', 6: 'input'}


class ShaderError(Exception):
    pass


class _Result(ctypes.Structure):
    _fields_ = [('tag', ctypes.c_int64), ('status', ctypes.c_int32), ('spv', ctypes.c_void_p), ('spv_len', ctypes.c_size_t),
                ('res', ctypes.c_void_p), ('res_len', ctypes.c_size_t), ('err', ctypes.c_void_p)]


class Pool:


    def __init__(self, worker_lib, xlate_lib, workers):
        lib = ctypes.CDLL(worker_lib)
        lib.dmsh_pool_open.restype = ctypes.c_void_p
        lib.dmsh_pool_open.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
        lib.dmsh_pool_submit.restype = ctypes.c_int
        lib.dmsh_pool_submit.argtypes = [ctypes.c_void_p, ctypes.c_int64, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t]
        lib.dmsh_pool_next.restype = ctypes.c_int
        lib.dmsh_pool_next.argtypes = [ctypes.c_void_p, ctypes.POINTER(_Result)]
        lib.dmsh_pool_close.restype = None
        lib.dmsh_pool_close.argtypes = [ctypes.c_void_p]
        lib.dmsh_result_free.restype = None
        lib.dmsh_result_free.argtypes = [ctypes.POINTER(_Result)]
        err = ctypes.create_string_buffer(512)
        self.lib, self.p = lib, lib.dmsh_pool_open(os.fsencode(xlate_lib), workers, err, len(err))
        if not self.p:
            raise ShaderError('the shader workers did not start: ' + err.value.decode('utf-8', 'replace'))

    def submit(self, tag, stage, job, code):
        if self.lib.dmsh_pool_submit(self.p, tag, stage, job, len(job), code, len(code)) != 0:
            raise ShaderError('the shader workers refused a job')

    def next(self):

        r = _Result()
        rc = self.lib.dmsh_pool_next(self.p, ctypes.byref(r))
        if rc == 0:
            return None
        if rc != 1:
            raise ShaderError('the shader workers stopped')
        try:
            out = (r.tag, r.status, ctypes.string_at(r.spv, r.spv_len) if r.spv else b'',
                   ctypes.string_at(r.res, r.res_len).decode('utf-8') if r.res else '',
                   ctypes.string_at(r.err).decode('utf-8', 'replace') if r.err else '')
        finally:
            self.lib.dmsh_result_free(ctypes.byref(r))
        return out

    def close(self):
        if self.p:
            self.lib.dmsh_pool_close(self.p)
            self.p = None


def parts(loc):

    by_pack = {}
    for key, (name, _) in loc.items():
        by_pack.setdefault(name if name.endswith('.bin') else None, []).append(key)
    out = []
    for name in sorted(n for n in by_pack if n):
        keys = sorted(by_pack[name])
        for i in range(0, len(keys), PART_SIZE):
            out.append(('%s.%d' % (name[:-len('.bin')], i // PART_SIZE), name, keys[i:i + PART_SIZE]))
    if None in by_pack:
        out.append(('main', None, sorted(by_pack[None])))
    return out


def manifest_text(entries):

    return ''.join('%s\t%d\t%s\n' % (p, n, h) for p, (n, h) in sorted(entries.items()))


def kept_parts(units, keep):

    out, fingerprint = {}, None
    try:
        with open(keep, encoding='utf-8') as f:
            lines = f.read().splitlines()
    except FileNotFoundError:
        return out, fingerprint
    for line in lines:
        if not line:
            continue
        part, digest = line.split('\t')
        if part == 'fingerprint':
            fingerprint = digest
            continue
        try:
            with open(os.path.join(units, part + '.tsv'), 'rb') as f:
                text = f.read()
        except FileNotFoundError:
            continue
        if hashlib.sha256(text).hexdigest() == digest:
            out[part] = [x.split('\t', 1)[0] for x in text.decode('utf-8').splitlines() if x]
    return out, fingerprint


def clean(stage, units, kept):

    keep = {p for paths in kept.values() for p in paths}
    out = os.path.join(stage, OUT)
    os.makedirs(out, exist_ok=True)
    os.makedirs(units, exist_ok=True)
    for f in os.listdir(out):
        if OUT + '/' + f not in keep:
            os.remove(os.path.join(out, f))
    for f in os.listdir(units):
        if not (f.endswith('.tsv') and f[:-4] in kept):
            os.remove(os.path.join(units, f))


def build(gfx_tools, exefs, data, stage, queries, worker_lib, xlate_lib, flush, sync_dir, workers=4, say=print, shader_map=None):



    for p in (gfx_tools, os.path.join(gfx_tools, '..', '..', 'tools'), os.path.join(gfx_tools, '..', '..', 'verify')):
        if p not in sys.path:
            sys.path.insert(0, p)
    import dmsh_vk
    import dmsh_facts
    import dmx
    import nsoimg
    import pack_builtin_queries
    import pack_game_shaders
    t0 = time.time()
    facts = dmsh_facts.load()
    if not facts:
        raise ShaderError('no answer table nvn/shader_facts.tsv')
    loc, stage_of = dmsh_vk.shader_index(shader_map or dmsh_vk.MAP)
    qkeys = sorted(pack_builtin_queries.kernel_keys()[m] for m in pack_builtin_queries.MODES)
    want = set(loc) | set(qkeys)
    out, units = os.path.join(stage, OUT), os.path.join(stage, '.units', OUT)
    kept, fingerprint = kept_parts(units, os.path.join(stage, '.units', OUT + '.keep'))
    clean(stage, units, kept)
    if fingerprint:
        with open(os.path.join(units, '.fingerprint.part'), 'w', encoding='utf-8', newline='\n') as f:
            f.write(fingerprint + '\n')
            f.flush()
            os.fsync(f.fileno())
        os.replace(os.path.join(units, '.fingerprint.part'), os.path.join(units, '.fingerprint'))
        sync_dir(units)

    def finish_part(part, entries):
        paths = [os.path.join(stage, p) for p in entries]
        flush(paths)
        sync_dir(out)
        target = os.path.join(units, part + '.tsv')
        with open(target + '.part', 'w', encoding='utf-8', newline='\n') as f:
            f.write(manifest_text(entries))
            f.flush()
            os.fsync(f.fileno())
        os.replace(target + '.part', target)
        sync_dir(units)

    def write(key, data_):
        with open(os.path.join(out, key + '.xgs'), 'wb') as f:
            f.write(data_)
        return OUT + '/' + key + '.xgs', (len(data_), hashlib.sha256(data_).hexdigest())

    if 'queries' not in kept:
        entries = {}
        for k in qkeys:
            with open(os.path.join(queries, k + '.xgs'), 'rb') as f:
                p, v = write(k, f.read())
            entries[p] = v
        finish_part('queries', entries)

    todo = [(part, name, keys) for part, name, keys in parts(loc) if part not in kept]
    if dmx.ROOT != data:
        for f in dmx._FH.values():
            f.close()
        dmx.ROOT, dmx._IDX, dmx._FH = data, None, {}
    main_img = None

    def jobs():

        nonlocal main_img
        for i, (part, name, keys) in enumerate(todo):
            if name is None:
                if main_img is None:
                    main_img = nsoimg.load(os.path.join(exefs, 'main'))
                recs = {k: dmsh_vk.main_rec(main_img, loc[k][1]) for k in keys}
            else:
                pack = dmsh_vk.records(dmx.get('hd/shaders/' + name))
                recs = {k: pack[loc[k][1]] for k in keys}
            for k in keys:
                job, c1, code = dmsh_vk.job_for(k, name or 'main', recs[k], stage_of, facts)
                yield i, k, dmsh_vk.STAGE_NO[job['stage']], json.dumps(job).encode('utf-8'), job['stage'], c1, code

    left = [len(keys) for _, _, keys in todo]
    entries = [dict() for _ in todo]
    meta, tags = {}, 0
    made, export_s, wait_s = 0, 0.0, 0.0
    pool = Pool(worker_lib, xlate_lib, workers)
    try:
        it, more, inflight = jobs(), True, 0
        while more or inflight:
            while more and inflight < workers * 8:
                try:
                    i, k, sno, job, sname, c1, code = next(it)
                except StopIteration:
                    more = False
                    break
                meta[tags] = (i, k, sname, c1)
                pool.submit(tags, sno, job, code)
                tags += 1
                inflight += 1
            if not inflight:
                break
            t = time.time()
            r = pool.next()
            wait_s += time.time() - t
            if r is None:
                raise ShaderError('the shader workers lost %d shaders' % inflight)
            inflight -= 1
            tag, status, spv, res, err = r
            i, k, sname, c1 = meta.pop(tag)
            if status != 0:
                raise ShaderError('%s: %s failed: %s' % (k, STEPS.get(status, status), err.splitlines()[0][:300] if err else '?'))
            t = time.time()
            vk = dmsh_vk.export(k, sname, c1, spv, json.loads(res))
            key, bundle = pack_game_shaders.pack_data(vk, spv)
            p, v = write(key, bundle)
            export_s += time.time() - t
            entries[i][p] = v
            made += 1
            left[i] -= 1
            if not left[i]:
                finish_part(todo[i][0], entries[i])
                entries[i] = None
    finally:
        pool.close()
    have = {f[:-4] for f in os.listdir(out) if f.endswith('.xgs')}
    if have != want or len(os.listdir(out)) != len(want):
        raise ShaderError('ASSUME packbuild.shaderkeys: %d bundles written, %d expected (%d missing, %d unexpected)'
                          % (len(os.listdir(out)), len(want), len(want - have), len(have - want)))
    hwm = ''
    try:
        with open('/proc/self/status') as f:
            hwm = next((x.split(':', 1)[1].strip() for x in f if x.startswith('VmHWM:')), '')
    except OSError:
        pass
    say('shaders: %d made, %d parts kept, %d bundles in %.1f s with %d workers (export and bundles %.1f s, waiting for workers %.1f s, peak %s)'
        % (made, len(kept), len(have), time.time() - t0, workers, export_s, wait_s, hwm or '?'))
