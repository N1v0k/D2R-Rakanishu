#!/usr/bin/env python3


from pathlib import Path
import sys
import tempfile
import unittest

import numpy as np

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
import game_addrs
import gameport


def filler(n, seed):

    r = np.random.default_rng(seed)
    return (0x8b000000 | r.integers(0, 1 << 21, n, dtype=np.uint32)).astype(np.uint32)


def adrp(rd, pc, target):
    pages = ((target & ~0xfff) - (pc & ~0xfff)) >> 12
    return 0x90000000 | (pages & 3) << 29 | ((pages >> 2) & 0x7ffff) << 5 | rd


def add_imm(rd, rn, imm12):
    return 0x91000000 | imm12 << 10 | rn << 5 | rd


class MaskTest(unittest.TestCase):
    def test_page_pair_low_bits_masked_until_the_register_is_written(self):
        w = np.array([adrp(8, 0, 0x5000), add_imm(0, 8, 0x123), add_imm(8, 9, 0), add_imm(1, 8, 0x456)], dtype=np.uint32)
        m, pair, is_adrp = gameport.masks(w)
        self.assertEqual(is_adrp.tolist(), [True, False, False, False])
        self.assertEqual(pair.tolist(), [False, True, False, False])
        self.assertEqual(int(m[0]), 0x9f00001f)
        self.assertEqual(int(m[1]), 0xffc003ff)
        self.assertEqual(int(m[3]), 0xffffffff)

    def test_branch_target_masked(self):
        w = np.array([0x94000010, 0x14000003, 0xb4000048], dtype=np.uint32)
        m, _, _ = gameport.masks(w)
        self.assertEqual([int(x) for x in m], [0xfc000000, 0xfc000000, 0xff00001f])


class AlignTest(unittest.TestCase):

    A, B, C = filler(3000, 1), filler(2000, 2), filler(3000, 3)
    X = filler(50, 4)

    def program(self):
        b2 = self.B.copy()
        b2[700] ^= 0x400
        b2[701] ^= 0x400
        old = np.concatenate([self.A, self.B, self.C])
        new = np.concatenate([self.A, self.X, b2, self.C])
        return old, new

    def test_map(self):
        old, new = self.program()
        runs = gameport.align_code(old, new)
        m = gameport.Map.from_alignment(runs, {}, len(old) * 4, 'o', 'n')
        self.assertEqual(sum(n for _, _, n in runs), len(old) - 2)
        self.assertEqual(m.code(4 * 10), (4 * 10, 'aligned'))
        self.assertEqual(m.code(4 * (3000 + 5)), (4 * (3050 + 5), 'aligned'))
        self.assertEqual(m.code(4 * (3000 + 700)), (4 * (3050 + 700), 'position'))
        self.assertEqual(m.code(4 * (5000 + 2999)), (4 * (5050 + 2999), 'aligned'))

    def test_inserted_function_only_in_new(self):
        old, new = self.program()
        m = gameport.Map.from_alignment(gameport.align_code(old, new), {}, len(old) * 4, 'o', 'n')
        news = {m.code(4 * i)[0] for i in range(len(old))}
        self.assertFalse(any(4 * (3000 + j) in news for j in range(50)), 'no old address maps into the inserted block')


class FieldTest(unittest.TestCase):


    def test_fields(self):
        import types
        A, B, C, X = AlignTest.A, AlignTest.B.copy(), AlignTest.C, AlignTest.X
        B[700], B[702] = 0xb94ca2e8, 0x9100a000
        b2 = B.copy()
        b2[700], b2[702] = 0xb94d32e8, 0x9100a001
        old = np.concatenate([A, B, C])
        new = np.concatenate([A, X, b2, C])
        m = gameport.Map.from_alignment(gameport.align_code(old, new), {}, len(old) * 4, 'o', 'n')
        w = 4 * (3000 + 700)
        table = types.SimpleNamespace(fields=[('main', 'MOVED', 0xca0, w, 'x'), ('main', 'KEPT', 0x28, 4 * 3001, 'x'),
                                              ('main', 'RESHAPED', 0x28, w + 8, 'x'), ('main', 'WRONG', 0x10, w, 'x')])
        B2 = B.copy()
        B2[1] = 0x9100a000
        old2 = np.concatenate([A, B2, C])
        new2 = np.concatenate([A, X, np.where(np.arange(len(b2)) == 1, np.uint32(0x9100a000), b2).astype(np.uint32), C])
        m2 = gameport.Map.from_alignment(gameport.align_code(old2, new2), {}, len(old2) * 4, 'o', 'n')
        prog = lambda words: types.SimpleNamespace(words=lambda: words)
        got = {f[0][1]: f[1:] for f in gameport.place_fields(table, prog(old2), prog(new2), m2)}
        self.assertEqual(got['MOVED'], (0xd30, 4 * (3050 + 700), 'position'))
        self.assertEqual(got['KEPT'], (0x28, 4 * (3050 + 1), 'aligned'))
        self.assertEqual(got['RESHAPED'], (None, 4 * (3050 + 702), 'witness changed shape'))
        self.assertEqual(got['WRONG'], (None, None, 'old witness does not encode the offset'))
        self.assertTrue(m.code(w)[1] == 'position')


class DataTest(unittest.TestCase):
    def test_votes_and_anchors(self):

        body_old, body_new = [], []
        for k, (do, dn) in enumerate(((0x41010, 0x43010), (0x41800, 0x43800))):
            pc = 4 * (len(body_old) + 400)
            body_old += [adrp(8, pc, do), add_imm(0, 8, do & 0xfff)]
            body_new += [adrp(8, pc, dn), add_imm(0, 8, dn & 0xfff)]
        pad = filler(400, 5)
        tail = filler(400, 6)
        old = np.concatenate([pad, np.array(body_old, dtype=np.uint32), tail])
        new = np.concatenate([pad, np.array(body_new, dtype=np.uint32), tail])
        runs = gameport.align_code(old, new)
        votes = gameport.data_votes(old, new, runs)
        self.assertEqual(dict(votes), {0x41010: {0x43010: 1}, 0x41800: {0x43800: 1}})
        m = gameport.Map.from_alignment(runs, votes, len(old) * 4, 'o', 'n')
        self.assertEqual(m.data(0x41010), (0x43010, 'xref'))
        self.assertEqual(m.data(0x41400), (0x43400, 'between'))
        self.assertEqual(m.data(0x50000), (None, 'none'))

    def test_majority(self):
        m = gameport.Map([], {0x9000: {0xa000: 8, 0xa100: 1}, 0x9100: {0xa200: 2, 0xa300: 2}}, 0x1000)
        self.assertEqual(m.data(0x9000), (0xa000, 'xref-majority'))
        self.assertEqual(m.data(0x9100), (None, 'ambiguous'))


class StoredMapTest(unittest.TestCase):
    def test_write_read_gaddr(self):
        m = gameport.Map([(0x100, 0x180, 10), (0x200, 0x300, 4)], {0x9000: {0xa000: 3}}, 0x1000, 'old', 'new', sdk_same=True)
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'map.tsv'
            m.write(p)
            r = gameport.Map.read(p)
        self.assertEqual(r.runs, m.runs)
        self.assertEqual((r.old_build, r.new_build, r.old_text_end), ('old', 'new', 0x1000))
        self.assertEqual(r.translate(0x104), (0x184, 'aligned'))
        self.assertEqual(r.translate(0x150), (None, 'none'))
        self.assertEqual(r.translate(0x9000), (0xa000, 'xref'))
        self.assertEqual(r.translate(0x104, 'sdk'), (0x104, 'module unchanged'))
        self.assertEqual(gameport.Map([], {}, 0x1000).translate(0x104, 'sdk'), (None, 'module changed'))


class ShaderKeyTest(unittest.TestCase):
    def test_keys_follow_the_block(self):
        import types
        r = np.random.default_rng(7)
        block = r.integers(0, 256, 64, dtype=np.uint8).tobytes()
        old_img = bytearray(0x3000)
        new_img = bytearray(0x3000)
        old_img[0x1100:0x1140] = block
        new_img[0x1180:0x11c0] = block
        old = types.SimpleNamespace(img=bytes(old_img), ro_start=0x1000, data_start=0x2000)
        new = types.SimpleNamespace(img=bytes(new_img), ro_start=0x1000, data_start=0x2000)
        with tempfile.TemporaryDirectory() as d:
            src, out = Path(d) / 'map.tsv', Path(d) / 'new.tsv'
            src.write_text('# head\nmain.rodata@1100\t0\tvs\nother.bin\t1\tps\nmain.rodata@1100\t2\tps\n')
            saved, gameport.SHADER_MAP = gameport.SHADER_MAP, str(src)
            try:
                rows = gameport.shader_keys(old, new, gameport.Map([], {}, 0x1000), str(out))
            finally:
                gameport.SHADER_MAP = saved
            text = out.read_text()
        self.assertEqual(rows, [(0x1100, 0x1180, 'bytes')])
        self.assertEqual(text, '# head\nmain.rodata@1180\t0\tvs\nother.bin\t1\tps\nmain.rodata@1180\t2\tps\n')


class TableTest(unittest.TestCase):
    TABLE = '''// test
GB(main, "1.0", "00112233445566778899aabbccddeeff")
GL(main, ADDR_LOW, 0x1000)
GL(main, TEXT_END, 0x8000)
GL(main, RO_START, 0x8000)
GL(main, DATA_START, 0xa000)
GL(main, IMAGE_END, 0x10000)
GA(main, PLACED, 0x1100, func, "placed")
GA(main, MOVED_DATA, 0x9000, data, "placed by an instruction pair")
GA(main, GONE, 0x1200, site, "not in the new program")
GA(main, GUESSED, 0x1300, func, "first instruction changed")
GA(main, BYTES_DIFFER, 0x8800, rodata, "read-only data")
'''

    class New:
        text_end, ro_start, data_start, image_end, build_id = 0x8100, 0x9000, 0xb000, 0x11000, 'ff' * 16

    def test_unresolved_does_not_load(self):
        with tempfile.TemporaryDirectory() as d:
            src = Path(d) / 'old.def'
            src.write_text(self.TABLE)
            t = game_addrs.Table(str(src))
            placed = [(t.entries[0], 0x1180, 'aligned', None), (t.entries[1], 0x9100, 'xref', None), (t.entries[2], None, 'none', None),
                      (t.entries[3], 0x1380, 'position', None), (t.entries[4], 0x8900, 'xref', False)]
            out, chk = Path(d) / 'new.def', Path(d) / 'check.def'
            fields = [(('main', 'ZOOM', 0x1348, 0x1104, 'zoom'), 0x13d8, 0x1184, 'position'),
                      (('main', 'LOST', 0x10, 0x1204, 'lost'), None, None, 'witness gone')]
            gameport.write_table(out, t, placed, self.New, None, '2.0', fields=fields)
            gameport.write_table(chk, t, placed, self.New, None, '2.0', check_copy=True, fields=fields)
            text = out.read_text()
            self.assertIn('GA_UNRESOLVED(main, GONE, 0x1200, site, "not in the new program")', text)
            self.assertIn('GA_UNRESOLVED(main, GUESSED, 0x1300, func, "first instruction changed")', text)
            self.assertIn('GA_UNRESOLVED(main, BYTES_DIFFER, 0x8800, rodata, "read-only data")', text)
            self.assertEqual(sum(x.startswith('GA(') for x in text.split('\n')), 2)
            self.assertIn('GF(main, ZOOM, 0x13d8, 0x1184, "zoom")', text)
            self.assertIn('GF_UNRESOLVED(main, LOST, 0x10, 0x1204, "lost")', text)
            with self.assertRaises(game_addrs.TableError):
                game_addrs.Table(str(out))
            c = game_addrs.Table(str(chk))
        self.assertEqual((c.ga.PLACED, c.ga.MOVED_DATA, c.ga.GONE, c.ga.GUESSED, c.ga.BYTES_DIFFER), (0x1180, 0x9100, 0x1200, 0x1380, 0x8900))
        self.assertEqual((c.game.version, c.game.build_id), ('2.0', 'ff' * 16))
        self.assertEqual((c.gf.ZOOM, c.gf.LOST), (0x13d8, 0x10))
        self.assertEqual((c.gl.TEXT_END, c.gl.RO_START, c.gl.DATA_START, c.gl.IMAGE_END, c.gl.ADDR_LOW), (0x8100, 0x9000, 0xb000, 0x11000, 0x1000))

    def test_status(self):
        self.assertEqual(gameport.status('aligned', None), 'placed')
        self.assertEqual(gameport.status('xref', False), 'review')
        self.assertEqual(gameport.status('position', None), 'review')
        self.assertEqual(gameport.status('none', None), 'unresolved')


if __name__ == '__main__':
    unittest.main()
