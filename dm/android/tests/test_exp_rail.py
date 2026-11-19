


import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

from PIL import Image

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
spec = importlib.util.spec_from_file_location('gen_exp_rail', TOOLS / 'gen_exp_rail.py')
rail = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rail)


def synthetic_base(spikes=range(9)):

    im = Image.new('RGBA', rail.BASE_SIZE, (0, 0, 0, 0)); px = im.load()
    for x in range(rail.COL0, rail.COL1):
        for y in range(89, 130): px[x, y] = (90, 90, 90, 255)
        for y in range(101, 109): px[x, y] = (24, 24, 24, 255)
    for k in range(9):
        x0 = rail.STUD0 + rail.PITCH * k
        for x in range(x0 + 1, x0 + 7):
            for y in range(101, 109): px[x, y] = (90, 90, 90, 255)
        if k in spikes:
            for x in range(x0 + 2, x0 + 6):
                for y in range(85, 89): px[x, y] = (90, 90, 90, 255)
    return im


def synthetic_fill(pieces=10):

    im = Image.new('RGBA', rail.FILL_SIZE, (0, 0, 0, 0)); px = im.load()
    gaps = [rail.STUD0 + rail.PITCH * k + rail.STUD_W / 2 - rail.FILL_X for k in range(pieces - 1)]
    for x in range(im.width):
        if any(abs(x + 0.5 - g) < 1.5 for g in gaps): continue
        for y in range(2, 9): px[x, y] = (255, 240, 150, 255)
    return im


class ExpRail(unittest.TestCase):
    def run_stage(self, base, fill, out):
        reader = lambda path: {rail.BASE: base, rail.FILL: fill}[path] if not callable(base) else base(path)
        with contextlib.redirect_stdout(io.StringIO()) as log:
            ok = rail.stage(None, str(out), read=reader)
        return ok, log.getvalue()

    def test_writes_three_checked_files(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'appart' / 'exp'
            ok, log = self.run_stage(synthetic_base(), synthetic_fill(), out)
            self.assertTrue(ok, log)
            self.assertEqual(sorted(p.name for p in out.iterdir()), ['fill.png', 'rail.json', 'rail.png'])
            meta = json.loads((out / 'rail.json').read_text())
            self.assertEqual(meta['v'], 1)
            self.assertEqual(len(meta['edges']), 22)
            self.assertEqual(meta['edges'][0], 0)
            self.assertEqual(meta['edges'][-1], rail.COL1 - rail.COL0)
            self.assertTrue(all(b > a for a, b in zip(meta['edges'], meta['edges'][1:])))
            self.assertEqual((meta['fill_x'], meta['fill_y']), (rail.FILL_X - rail.COL0, rail.FILL_Y - rail.ROW0))
            with Image.open(out / 'rail.png') as r, Image.open(out / 'fill.png') as f:
                self.assertEqual((r.size, f.size), ((rail.COL1 - rail.COL0, rail.ROW1 - rail.ROW0), rail.FILL_SIZE))
            self.assertFalse((Path(temp) / 'appart' / 'exp.part').exists())

    def test_any_mismatch_writes_nothing(self):
        def missing(path): raise SystemExit('romfs_art: no such file ' + path)
        cases = {
            'other size': (synthetic_base().resize((1580, 206)), synthetic_fill(), 'sizes'),
            'spike missing': (synthetic_base(spikes=[k for k in range(9) if k != 4]), synthetic_fill(), 'no spike at stud 4'),
            'nine fill pieces': (synthetic_base(), synthetic_fill(pieces=9), 'fill has 9 pieces'),
            'fill shifted': (synthetic_base(), synthetic_fill().transform(rail.FILL_SIZE, Image.AFFINE, (1, 0, 4, 0, 1, 0)), 'is not on its stud'),
            'missing sprite': (missing, None, 'no such file'),
        }
        for name, (base, fill, why) in cases.items():
            with self.subTest(name), tempfile.TemporaryDirectory() as temp:
                out = Path(temp) / 'appart' / 'exp'
                out.mkdir(parents=True); (out / 'stale.png').write_bytes(b'old')
                ok, log = self.run_stage(base, fill, out)
                self.assertFalse(ok)
                self.assertIn('flat experience bar', log)
                self.assertIn(why, log)
                self.assertFalse(out.exists())
                self.assertFalse((Path(temp) / 'appart' / 'exp.part').exists())


if __name__ == '__main__':
    unittest.main()
