





import importlib.util
import io
import os
from pathlib import Path
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
spec = importlib.util.spec_from_file_location('check_style', TOOLS / 'check_style.py')
cs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cs)

VIEW = '''package com.xiaoboluo.d2r;
import android.graphics.Typeface;   // 导入不算 | an import does not count
final class View1 {
    private static final int GOLD = 0xFFC7B377, RED = 0xFFE05050;
    private final Paint text = new Paint(1), box = new Paint(1), red = new Paint(1), gold = new Paint(1), bare = new Paint(1), edge = new Paint(1);
    View1(AppArt art) {
        text.setColor(0xFFE8DCC0);
        text.setTypeface(art.font("title.otf"));
        box.setColor(0xFF101010);
        red.setColor(RED); red.setTypeface(null);
        gold.setColor(GOLD); gold.setTypeface(null);
        edge.set(text); edge.setStyle(Paint.Style.STROKE);
        // text.setColor(0xFF000000); 注释里的不算 | not in a comment
        String url = "http://example//x";
    }
    private int colour(String k) { switch (k) { case "magic": return 0xFF6C6CFF; default: return 0xFFFFFFFF; } }
    private int alpha(int a) { return a * 2; }
    void draw(Canvas c, boolean low, String k) {
        c.drawRect(0, 0, 1, 1, box);
        text.setColor(low ? 0xFF111111 : 0xFF222222);
        c.drawText("a", 0, 0, text);
        text.setColor(0xFF333333); c.drawRect(0, 0, 1, 1, text);
        text.setColor(rgba(.1, .2, .3, 1)); c.drawText("b", 0, 0, text);
        text.setColor(Color.WHITE); c.drawText("c", 0, 0, text);
        text.setColor(colour(k)); c.drawText("d", 0, 0, text);
        text.setColor(alpha(3)); c.drawText("e", 0, 0, text);
        Paint p = low ? red : gold;
        c.drawText("f", 0, 0, p);
        c.drawText("g", 0, 0, edge);
        c.drawText("h", 0, 0, bare);
        text.setShadowLayer(2, 1, 1, 0xC8000000);
        box.setStyle(Paint.Style.STROKE); c.drawRect(0, 0, 1, 1, box);
        text.setTypeface(Typeface.SERIF);
    }
}
'''


VIEW2 = '''final class View2 {
    private final Paint hi = new Paint(1), lo = new Paint(1), plain = new Paint(1), nul = new Paint(1), base = new Paint(1);
    View2(Typeface face) {
        hi.setColor(0xFF00FF00); lo.setColor(0xFF0000FF);
        nul.setTypeface(null);
        base.setTypeface(face);
    }
    void draw(Canvas c, boolean up) {
        Paint p = up ? hi : lo;
        Paint q = p;
        c.drawText("a", 0, 0, q);
        plain.setColor(0xFFFF0000); plain.setTypeface(face());
        c.drawRect(0, 0, 1, 1, plain);
        c.drawText("b", 0, 0, plain);
        c.drawText("c", 0, 0, nul);
        Paint copy = new Paint(base);
        c.drawText("d", 0, 0, copy);
    }
}
'''


def kinds(found):
    out = {}
    for kind, stmt in found: out.setdefault(kind, []).append(stmt)
    return out


class FindingsTest(unittest.TestCase):
    def setUp(self): self.k = kinds(cs.findings('View1.java', VIEW))

    def test_colours(self):
        lit = self.k['text-colour-literal']
        self.assertIn('text.setColor(0xFFE8DCC0)', lit)
        self.assertIn('text.setColor(low ? 0xFF111111 : 0xFF222222)', lit)
        self.assertIn('text.setColor(colour(k))', lit)
        self.assertNotIn('text.setColor(0xFF333333)', lit)
        self.assertNotIn('box.setColor(0xFF101010)', lit)
        self.assertNotIn('text.setColor(0xFF000000)', lit)
        self.assertEqual(sorted(self.k['text-colour-rgba']), ['text.setColor(Color.WHITE)', 'text.setColor(rgba(.1, .2, .3, 1))'])
        self.assertEqual(sorted(self.k['text-colour-constant']), ['gold.setColor(GOLD)', 'red.setColor(RED)'])
        self.assertNotIn('text.setColor(alpha(3))', sum(self.k.values(), []))

    def test_fonts_shadows_strokes(self):
        self.assertEqual(self.k['font-file'], ['text.setTypeface(art.font("title.otf"))'])
        self.assertEqual(self.k['typeface'], ['text.setTypeface(Typeface.SERIF)'])
        self.assertEqual(self.k['shadow-layer'], ['text.setShadowLayer(2, 1, 1, 0xC8000000)'])
        self.assertEqual(self.k['text-stroke'], ['edge.setStyle(Paint.Style.STROKE)'])
        self.assertEqual(self.k['system-font'], ['bare', 'gold', 'red'])

    def test_aliases_lifetimes_copies(self):
        k = kinds(cs.findings('View2.java', VIEW2))
        lit = k.get('text-colour-literal', [])
        self.assertIn('hi.setColor(0xFF00FF00)', lit)
        self.assertIn('lo.setColor(0xFF0000FF)', lit)
        self.assertIn('plain.setColor(0xFFFF0000)', lit)
        self.assertEqual(sorted(k.get('system-font', [])), ['hi', 'lo', 'nul'])

    def test_panel_geometry(self):



        src = ('class RecipeBook {\n  void d(Canvas c, float h) {\n    c.drawColor(0xE0000000);\n    image(c, "panel", panel);\n    float rowH = h * 0.085f, x = 1;\n'
               '    float tabW = rowH * 3.5f;\n    style.backdrop(c);\n  }\n  float gamePx() { return getHeight() / 1080f; }\n}\n')
        k = kinds(cs.findings('RecipeBook.java', src))
        self.assertEqual(k.get('panel-backdrop'), ['c.drawColor(0xE0000000)'])
        self.assertEqual(k.get('panel-frame'), ['image(c, "panel", panel)'])
        self.assertEqual(sorted(k.get('panel-metric')), ['float rowH = h * 0.085f, x = 1', 'return getHeight() / 1080f'])
        self.assertFalse(any(x.startswith('panel-') for x, _ in cs.findings('Other.java', src)))

    def test_strip_keeps_positions(self):
        src = 'a = "x // y"; // c\n/* d\n e */ b;'
        out = cs.strip(src)
        self.assertEqual(len(out), len(src))
        self.assertIn('"x // y"', out)
        self.assertNotIn('// c', out); self.assertNotIn('e */', out)
        self.assertEqual(out.count('\n'), 2)


class NamedColourTest(unittest.TestCase):
    def test_style_colour_does_not_call_a_local_helper(self):
        self.assertIsNone(cs.colour_kind('style.colour("FontColorPartyGreen")', ['colour']))
        self.assertEqual(cs.colour_kind('this.colour("set")', ['colour']), 'text-colour-literal')


class BaselineTest(unittest.TestCase):
    def tree(self, files):
        d = tempfile.mkdtemp()
        for name, text in files.items():
            with open(os.path.join(d, name), 'w', encoding='utf-8') as f: f.write(text)
        return d, os.path.join(d, 'baseline.txt')

    def run_check(self, src, base, update=False):
        out = io.StringIO()
        return cs.check(src, base, update=update, out=out), out.getvalue()

    def test_flow(self):
        src, base = self.tree({'View1.java': VIEW, 'GameText.java': VIEW})
        rc, log = self.run_check(src, base)
        self.assertEqual(rc, 1); self.assertIn('NEW    View1.java', log); self.assertNotIn('GameText.java', log)
        rc, log = self.run_check(src, base, update=True)
        self.assertEqual(rc, 0)
        self.assertEqual(self.run_check(src, base)[0], 0)

        with open(os.path.join(src, 'View1.java'), 'w', encoding='utf-8') as f: f.write(VIEW.replace('text.setShadowLayer(2, 1, 1, 0xC8000000);', ''))
        rc, log = self.run_check(src, base)
        self.assertEqual(rc, 1); self.assertIn('GONE   View1.java\tshadow-layer', log)

        self.run_check(src, base, update=True)
        with open(os.path.join(src, 'View1.java'), 'w', encoding='utf-8') as f: f.write(VIEW.replace('c.drawText("h", 0, 0, bare);', 'c.drawText("h", 0, 0, bare); bare.setShadowLayer(1, 1, 1, 0);'))
        rc, log = self.run_check(src, base)
        self.assertEqual(rc, 1); self.assertIn('NEW    View1.java\tshadow-layer\tbare.setShadowLayer(1, 1, 1, 0)', log)

    def test_strict(self):
        src, base = self.tree({'EffectLayout.java': VIEW.replace('View1', 'EffectLayout')})
        rc, log = self.run_check(src, base, update=True)
        self.assertEqual(rc, 1); self.assertIn('STRICT EffectLayout.java', log)
        self.assertFalse(os.path.exists(base))
        with open(base, 'w', encoding='utf-8') as f: f.write('EffectLayout.java\tshadow-layer\ttext.setShadowLayer(2, 1, 1, 0xC8000000)\n')
        rc, log = self.run_check(src, base)
        self.assertEqual(rc, 1); self.assertIn('STRICT EffectLayout.java\tshadow-layer', log)

    def test_repository_matches_its_baseline(self):
        rc, log = self.run_check(cs.SRC, cs.BASELINE)
        self.assertEqual(rc, 0, log)
        found = cs.scan()
        self.assertFalse([k for k in found if k[0] in cs.STRICT], 'status effect classes must stay clean')
        self.assertFalse([k for k in found if k[0] in cs.EXEMPT])


if __name__ == '__main__':
    unittest.main()
