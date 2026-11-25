#!/usr/bin/env python3






import glob
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pack_zip


IOS_TOOLS = 'dm/ios/tools/'


def test_only(repo):

    with open(os.path.join(repo, IOS_TOOLS, 'test_only.txt'), encoding='utf-8') as f:
        return {IOS_TOOLS + l.strip() for l in f if l.strip() and not l.startswith('#')}


def tool_files(repo, listing=os.path.join(HERE, 'tools.txt'), variant='player'):


    if variant not in ('qa', 'player'):
        raise SystemExit('variant must be qa or player, got %r' % variant)
    out = {}
    with open(listing, encoding='utf-8') as f:
        for line in f:
            pattern = line.strip()
            if not pattern or pattern.startswith('#'):
                continue
            if pattern.startswith('qa '):
                if variant != 'qa':
                    continue
                pattern = pattern[3:].strip()
            hits = [p for p in glob.glob(os.path.join(repo, pattern), recursive=True) if os.path.isfile(p)]
            hits = [p for p in hits if '__pycache__' not in p.split(os.sep) and not p.endswith('.pyc')]
            if not hits:
                raise SystemExit('tools.txt: %s matches no file' % pattern)
            for p in hits:
                if os.path.islink(p):
                    raise SystemExit('tools.txt: %s is a link' % os.path.relpath(p, repo))
                out[os.path.relpath(p, repo).replace(os.sep, '/')] = p
    if variant == 'player':
        skip = test_only(repo)
        out = {k: v for k, v in out.items() if k not in skip and not any(d.endswith('/') and k.startswith(d) for d in skip)}
    return out


def queries(repo, archive):

    with tempfile.TemporaryDirectory() as t:
        q = os.path.join(t, 'q')
        subprocess.run([sys.executable, os.path.join(repo, 'dm', 'gfx', 'tools', 'pack_builtin_queries.py'), '--pinned', '--output', q], check=True,
                       stdout=subprocess.DEVNULL)
        names = sorted(f for f in os.listdir(q) if f.endswith('.xgs'))
        if len(names) != 4:
            raise SystemExit('pack_builtin_queries.py made %d bundles, expected 4' % len(names))
        pack_zip.write({n: os.path.join(q, n) for n in names}, archive)


def sha256(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def main(repo, runtime, out, variant='player'):
    with open(os.path.join(runtime, 'RECIPE'), encoding='utf-8') as f:
        recipe = f.read().strip()
    os.makedirs(out, exist_ok=True)
    pack_zip.write(tool_files(repo, variant=variant), os.path.join(out, 'tools.zip'))
    shutil.copyfile(os.path.join(runtime, 'runtime.zip'), os.path.join(out, 'runtime.zip'))
    queries(repo, os.path.join(out, 'queries.zip'))
    with open(os.path.join(out, 'VERSION'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('runtime=%s\ntools=%s\nqueries=%s\n' % (recipe, sha256(os.path.join(out, 'tools.zip')), sha256(os.path.join(out, 'queries.zip'))))


if __name__ == '__main__':
    if len(sys.argv) not in (4, 5):
        sys.exit(__doc__)
    main(*sys.argv[1:])
