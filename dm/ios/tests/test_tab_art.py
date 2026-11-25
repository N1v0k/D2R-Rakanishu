import hashlib
import os
import sys
import struct
import json
import tempfile
from unittest.mock import patch
import unittest
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import tab_art
from tab_art import widen, build, text_metrics


class TabArtTest(unittest.TestCase):
    def test_rims_and_gem_keep_their_dimensions(self):
        for source_width, height in ((134, 60), (136, 58)):
            source = Image.new("RGBA", (source_width, height))
            source.putdata([(x, y, (x + y) % 256, 255) for y in range(height) for x in range(source_width)])
            for width in (source_width, source_width + 16, source_width + 64):
                with self.subTest(source=source.size, width=width):
                    result = widen(source, width)
                    self.assertEqual(result.size, (width, height))
                    self.assertEqual(result.crop((0, 0, 6, height)).tobytes(), source.crop((0, 0, 6, height)).tobytes())
                    self.assertEqual(result.crop((width - 6, 0, width, height)).tobytes(), source.crop((source_width - 6, 0, source_width, height)).tobytes())
                    self.assertEqual(result.crop((width // 2 - 8, 0, width // 2 + 8, height)).tobytes(), source.crop((source_width // 2 - 8, 0, source_width // 2 + 8, height)).tobytes())

    def test_frames_are_contiguous_after_widening(self):
        source = Image.new("RGBA", (539, 60))
        colours = [(220, 20, 20, 255), (20, 220, 20, 255), (20, 20, 220, 255), (220, 220, 20, 255)]
        for index, colour in enumerate(colours):
            source.paste(colour, (index * 134, 0, (index + 1) * 134, 60))
        header = bytearray(24)
        struct.pack_into("<HHII", header, 4, 61, 134, 539, 60)
        struct.pack_into("<H", header, 20, 4)
        with tempfile.TemporaryDirectory() as directory, \
                patch("romfs_art.read", return_value=bytes(header)), \
                patch("gen_widestash_art.lib"), patch("gen_widestash_art.decode", return_value=source), \
                patch("gen_widestash_art.bc3", side_effect=lambda image: image.copy()) as compress, \
                patch("gen_widestash_art.pack", return_value=b"sprite"):
            build("unused", directory, (300, 340))
        for call, width in zip(compress.call_args_list, (150, 170)):
            sheet = call.args[0]
            self.assertEqual(sheet.size, (width * 4, 60))
            for index, colour in enumerate(colours):
                self.assertEqual(sheet.getpixel((index * width, 30)), colour)
                self.assertEqual(sheet.getpixel(((index + 1) * width - 1, 30)), colour)

    def test_text_offsets_follow_ink_and_typographic_span(self):
        raw = bytearray(200)
        struct.pack_into(">H", raw, 4, 2)
        struct.pack_into(">4sIII", raw, 12, b"head", 0, 44, 54)
        struct.pack_into(">4sIII", raw, 28, b"OS/2", 0, 98, 100)
        struct.pack_into(">H", raw, 62, 1000)
        struct.pack_into(">hh", raw, 166, 600, -127)
        class Font:
            def getbbox(self, label, anchor):
                return {"H": (0, -586, 713, 2), "Jbcd": (0, -586, 2323, 230),
                        "Ábcde": (0, -773, 3078, 40), "Úbcde": (0, -773, 3078, 2)}[label]
        keys = ("Audio", "Gameplay", "dmTabGameSettings", "dmTabCustom", "OptionsMenuVideo", "Controls", "Unique", "Sets", "dmChronRunewords", "Bosses")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "strings.json"
            path.write_text(json.dumps([{"Key": key, "enUS": "H", "deDE": "Jbcd", "ptBR": "Úbcde" if key == "Unique" else "Ábcde"} for key in keys]))
            with patch("romfs_art.read", return_value=bytes(raw)), patch("PIL.ImageFont.truetype", return_value=Font()):
                self.assertEqual(text_metrics("unused", path), "TAB_INK_V1\n0\tH\n-157\tJbcd\n102\tÁbcde\n129\tÚbcde\nEND\n")
            path.write_text("[]")
            with patch("romfs_art.read", return_value=bytes(raw)), patch("PIL.ImageFont.truetype", return_value=Font()), self.assertRaises(ValueError):
                text_metrics("unused", path)

    def test_chronicle_uses_its_own_frame_geometry(self):
        source = Image.new("RGBA", (273, 58), (80, 90, 100, 255))
        header = bytearray(24)
        struct.pack_into("<HHII", header, 4, 61, 136, 273, 58)
        struct.pack_into("<H", header, 20, 2)
        with tempfile.TemporaryDirectory() as directory, \
                patch("romfs_art.read", return_value=bytes(header)), \
                patch("gen_widestash_art.lib"), patch("gen_widestash_art.decode", return_value=source), \
                patch("gen_widestash_art.bc3", side_effect=lambda image: image.copy()) as compress, \
                patch("gen_widestash_art.pack", return_value=b"sprite"):
            paths = build("unused", directory, (400,), chronicle=True)
        self.assertEqual(compress.call_args.args[0].size, (400, 58))
        self.assertIn("data/hd/global/ui/ztouch_options_ui/chronicle_tabs_400.sprite", paths)

    def test_skill_tabs_keep_whole_wrapped_labels_in_both_layouts(self):
        import touch_layout
        source = json.dumps({"fields": {"skillBackgroundFile": ["Controller/Panel/Spells/V2/" + name + "Skilltree"
                            for name in ("Am", "So", "Ne", "Pa", "Ba", "Dr", "As", "Wa")]},
                            "children": [{"type": "ImageWidget", "name": "BackgroundExtra", "fields": {"filename": "extra"}}] +
                            [{"name": "Background", "fields": {"rect": {"x": 249, "y": -2}}} for _ in range(3)] +
                            [{"type": "TextBoxWidget", "name": "TextTab%d" % i,
                            "fields": {"rect": {"x": 0}, "style": "$StyleSkillTreeTabs", "truncateWithEllipsis": True}}
                           for i in range(4)]})
        result = json.loads(touch_layout._skill_tab_labels(source))
        original = json.loads(source)
        for widget, before in zip(result["children"], original["children"]):
            if not widget["name"].startswith("TextTab"):

                self.assertEqual(widget, before)
                continue
            self.assertFalse(widget["fields"]["truncateWithEllipsis"])
            self.assertTrue(widget["fields"]["style"]["options"]["lineWrap"])
            self.assertEqual(widget["fields"]["style"]["pointSize"], tab_art.SKILL_TAB_SIZE)
            self.assertEqual(widget["fields"]["style"]["spacing"], {"leading": 0.9, "kerning": 0.95})
            rect = widget["fields"]["rect"]
            self.assertEqual((rect["width"], rect["height"]), (tab_art.SKILL_TAB_WIDTH, tab_art.SKILL_TAB_HEIGHT))

            self.assertLessEqual(abs(rect["x"] + rect["width"] / 2 - ((277, 587, 903, 1219)[int(widget["name"][-1])] + 150)), 3)
        with self.assertRaises(ValueError):
            touch_layout._skill_tab_labels(source.replace("$StyleSkillTreeTabs", "$Other"))

    def test_rejects_unexpected_geometry(self):
        for size, width in (((133, 60), 170), ((134, 59), 170), ((134, 60), 133), ((134, 60), 171)):
            with self.subTest(size=size, width=width), self.assertRaises(ValueError):
                widen(Image.new("RGBA", size), width)


class FakeFont:

    span, line_span = 1000, 1000

    def width(self, text, size, kerning=0.95):
        return sum(3 if char == " " else 10 for char in text) * size / 10

    def line_pitch(self, size, leading=0.9):
        return size * leading

    def cap_height(self, size):
        return size * 0.8


class SkillTabRulesTest(unittest.TestCase):
    def test_newline_wraps_like_a_space(self):
        self.assertEqual(tab_art.wrap(FakeFont(), "Ab-\ncd", 10, 100), ["Ab- cd"])
        self.assertEqual(tab_art.wrap(FakeFont(), "Ab-\ncd", 10, 40), ["Ab-", "cd"])

    def test_every_layout_inside_the_wrap_band_is_checked(self):

        self.assertEqual(tab_art.layouts(FakeFont(), "Abcd Efgh", 10, 82.5), [["Abcd", "Efgh"], ["Abcd Efgh"]])
        self.assertEqual(tab_art.skill_tab_problems(FakeFont(), "Abcd Efgh", 10, 82.5, 103), [])

        problems = tab_art.skill_tab_problems(FakeFont(), "Abcd Ef Ghijk", 10, 62.7, 200)
        self.assertEqual(problems, ["3 lines: ['Abcd', 'Ef', 'Ghijk']"])
        self.assertEqual(tab_art.skill_tab_problems(FakeFont(), "Abcd Ef Ghijk", 10, 70, 200), [])

    def test_hyphen_break_must_stay_where_translators_put_it(self):
        problems = tab_art.skill_tab_problems(FakeFont(), "Ab-\ncd", 10, 100, 103)
        self.assertTrue(any("hyphenated break" in problem for problem in problems))
        self.assertEqual(tab_art.skill_tab_problems(FakeFont(), "Abcdefg-\nhijklmnop", 10, 120, 103), [])

    def test_rims_lines_and_words_are_checked(self):
        font = FakeFont()
        self.assertTrue(any("rims" in p for p in tab_art.skill_tab_problems(font, "A" * 29, 10, 300, 103)))
        self.assertTrue(any("wider than the box" in p for p in tab_art.skill_tab_problems(font, "A" * 12, 10, 100, 103)))
        self.assertTrue(any("3 lines" in p for p in tab_art.skill_tab_problems(font, "Aaaa Bbbb Cccc", 10, 50, 103)))

    def test_fitting_compounds_are_joined_in_every_language(self):
        labels = {("skills.json", "A"): {"deDE": "Ab-\ncd", "plPL": "Ab-\ncd", "enUS": "x"},
                  ("skills.json", "B"): {"deDE": "Abcdefghi-\njklmnop", "plPL": "y", "enUS": "x"},
                  ("skills.json", "C"): {"deDE": "Ab- und Cd", "plPL": "y", "enUS": "x"}}
        shown = tab_art.shown_labels(FakeFont(), labels, 10, 100)
        self.assertEqual(shown[("skills.json", "A")], {"deDE": "Abcd", "plPL": "Abcd", "enUS": "x"})
        self.assertEqual(shown[("skills.json", "B")]["deDE"], "Abcdefghi-\njklmnop")
        self.assertEqual(shown[("skills.json", "C")]["deDE"], "Ab- und Cd")

    def test_rewrites_apply_only_to_the_expected_game_text(self):
        labels = {("skills.json", "A"): {"ruRU": "Abcdefghijkl", "enUS": "Ab cd ef"}}
        rewrites = {("skills.json", "A", "ruRU"): (sha256("Abcdefghijkl"), ("break", 6)),
                    ("skills.json", "A", "enUS"): (sha256("Ab cd ef"), ("words", 2))}
        with patch.dict(tab_art.SKILL_TAB_REWRITES, rewrites):
            self.assertEqual(tab_art.shown_labels(FakeFont(), labels, 10, 100)[("skills.json", "A")],
                             {"ruRU": "Abcdef-\nghijkl", "enUS": "Ab cd"})
            labels[("skills.json", "A")]["ruRU"] = "Changed"
            with self.assertRaisesRegex(ValueError, "fingerprint"):
                tab_art.shown_labels(FakeFont(), labels, 10, 100)

    def test_rewrite_rules_must_fit_their_label(self):
        for label, rule in (("Ab cd", ("break", 2)), ("Abcd", ("break", 4)), ("Ab cd", ("words", 2)), ("Abcd", ("cut", 1))):
            with self.subTest(label=label, rule=rule), self.assertRaisesRegex(ValueError, "does not fit"):
                tab_art.rewrite_skill_tab_label(label, (sha256(label), rule))

    def test_replacement_changes_only_the_shown_values(self):
        rows = [{"id": 1, "Key": "SkillCategoryBa1", "enUS": "Abcd", "deDE": "Efg-\nhij", "plPL": "Klmno pqrs tu",
                 "ptBR": "Vwxy"}]
        rewrite = {("skills.json", "SkillCategoryBa1", "plPL"): (sha256("Klmno pqrs tu"), ("words", 2))}
        with tempfile.TemporaryDirectory() as directory, patch.dict(tab_art.SKILL_TAB_REWRITES, rewrite):
            source = Path(directory) / "skills.json"
            source.write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8-sig")
            labels = {("skills.json", "SkillCategoryBa1"): {key: rows[0][key] for key in ("enUS", "deDE", "plPL", "ptBR")}}
            with patch.object(tab_art.TabFont, "load", return_value=FakeFont()), \
                    patch.object(tab_art, "skill_tab_labels", return_value=labels), \
                    patch("touch_layout.read_layout", return_value=("data/local/lng/strings/skills.json", "")):
                result = tab_art.build_skill_strings("unused", directory, {"data/local/lng/strings/skills.json": str(source)}, 10, 200)
                written = json.loads(Path(result["data/local/lng/strings/skills.json"]).read_text(encoding="utf-8-sig"))
                self.assertEqual(written, [dict(rows[0], deDE="Efghij", plPL="Klmno pqrs")])
                labels[("skills.json", "SkillCategoryBa1")]["deDE"] = "Efg-\nxyz"
                with self.assertRaises(ValueError):
                    tab_art.build_skill_strings("unused", directory, {"data/local/lng/strings/skills.json": str(source)}, 10, 200)


def sha256(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def covered(raw, text):

    tables = {}
    for index in range(struct.unpack_from(">H", raw, 4)[0]):
        tag, _, offset, length = struct.unpack_from(">4sIII", raw, 12 + index * 16)
        tables[tag] = raw[offset:offset + length]
    cmap = tables[b"cmap"]
    subtables = {struct.unpack_from(">HH", cmap, 4 + i * 8): struct.unpack_from(">I", cmap, 8 + i * 8)[0]
                 for i in range(struct.unpack_from(">H", cmap, 2)[0])}
    offset = subtables.get((3, 1), subtables.get((0, 3)))
    if struct.unpack_from(">H", cmap, offset)[0] != 4:
        raise ValueError("Unexpected cmap format")
    segments = struct.unpack_from(">H", cmap, offset + 6)[0] // 2
    ends = struct.unpack_from(">%dH" % segments, cmap, offset + 14)
    starts = struct.unpack_from(">%dH" % segments, cmap, offset + 16 + 2 * segments)
    deltas = struct.unpack_from(">%dh" % segments, cmap, offset + 16 + 4 * segments)
    ranges_at = offset + 16 + 6 * segments
    ranges = struct.unpack_from(">%dH" % segments, cmap, ranges_at)

    def glyph(code):
        for i, (start, end) in enumerate(zip(starts, ends)):
            if start <= code <= end:
                if not ranges[i]:
                    return (code + deltas[i]) & 0xFFFF
                value = struct.unpack_from(">H", cmap, ranges_at + 2 * i + ranges[i] + 2 * (code - start))[0]
                return value and (value + deltas[i]) & 0xFFFF
        return 0
    return all(char.isspace() or ord(char) < 0x10000 and glyph(ord(char)) for char in text)


ROMFS = os.environ.get("DM_ROMFS_SRC")


@unittest.skipUnless(ROMFS, "set DM_ROMFS_SRC to the game's RomFS to measure the real labels")
class MeasuredTabLabelsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.font = tab_art.TabFont.load(ROMFS)
        cls.labels = tab_art.skill_tab_labels(ROMFS)

    def problems(self, size, width):
        return [(key, lang, label, problem) for key, texts in tab_art.shown_labels(self.font, self.labels, size, width).items()
                for lang, label in texts.items()
                for problem in tab_art.skill_tab_problems(self.font, label, size, width, tab_art.SKILL_TAB_HEIGHT)]

    def test_every_game_language_drawn_with_the_tab_font_is_measured(self):
        import romfs_art
        import touch_layout
        raw = romfs_art.read(ROMFS, "hd/ui/fonts/ExocetBlizzardOT-Medium.otf")
        rows = {}
        for name in ("skills.json", "ui.json"):
            _, text = touch_layout.read_layout(ROMFS, None, suffix="/lng/strings/" + name)
            rows.update({(name, row["Key"]): row for row in json.loads(text)})
        tree = [rows[key] for key in tab_art.SKILL_TAB_KEYS]
        languages = set(tree[0]) - {"id", "Key"}
        self.assertEqual(len(languages), 13)
        drawn = {lang for lang in languages if all(covered(raw, row[lang]) for row in tree)}
        self.assertEqual(drawn, set(tab_art.SKILL_TAB_LANGS))

        self.assertTrue(all(not covered(raw, "".join(row[lang] for row in tree)) for lang in languages - drawn))

    def test_wrap_band_matches_the_screen(self):

        size, width = tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_WIDTH
        for key, lang in (("SkillCategoryAs1", "enUS"), ("SkillCategoryAs1", "itIT"), ("SkillCategoryBa1", "ruRU")):
            label = self.labels[("skills.json", key)][lang]
            self.assertEqual(tab_art.layouts(self.font, label, size, width), [[label]], (key, lang))
        for key, lang in (("SkillCategoryBa3", "enUS"), ("SkillCategoryDr2", "ruRU")):
            label = self.labels[("skills.json", key)][lang]
            self.assertEqual(tab_art.layouts(self.font, label, size, width), [label.split(" ")], (key, lang))

    def test_skill_tab_size_is_the_largest_that_keeps_every_label_whole(self):
        self.assertEqual(len(self.labels), 25)
        self.assertEqual(self.problems(tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_WIDTH), [])
        larger = tab_art.SKILL_TAB_SIZE + 1
        self.assertEqual([width for width in range(200, tab_art.SKILL_TAB_INNER + 1) if not self.problems(larger, width)], [])

    def test_rewrites_are_exactly_the_labels_that_cannot_stay_whole(self):
        with patch.dict(tab_art.SKILL_TAB_REWRITES, clear=True):
            failing = {(key[0], key[1], lang) for key, lang, _, _ in self.problems(tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_WIDTH)}
        self.assertEqual(failing, set(tab_art.SKILL_TAB_REWRITES))

    def test_compounds_are_whole_where_they_fit(self):
        size, width = tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_WIDTH
        shown = tab_art.shown_labels(self.font, self.labels, size, width)
        joined = {(key[1], lang) for key, texts in self.labels.items() for lang, label in texts.items()
                  if "-\n" in label and "-\n" not in shown[key][lang]}
        self.assertEqual(joined, {("SkillCategoryNe1", "deDE"), ("SkillCategoryDr3", "deDE"), ("SkillCategoryBa1", "deDE")})
        for key, lang in joined:
            self.assertEqual(shown[("skills.json", key)][lang], self.labels[("skills.json", key)][lang].replace("-\n", ""))
        for key, texts in shown.items():
            for lang, label in texts.items():
                for lines in tab_art.layouts(self.font, label, size, width):
                    if "-\n" in label:

                        self.assertEqual(lines, label.split("\n"))
                    for line in lines:
                        self.assertNotRegex(line, r"-\S*\s" if "- " not in label else r"-$^", (key, lang, label))

    def test_chronicle_size_is_the_largest_with_rim_clearance(self):
        import dm_i18n
        import touch_layout
        chronicle = tab_art.chronicle_tab_labels(ROMFS)
        labels = [label for texts in chronicle.values() for label in texts.values()]

        own = {row[0]: row for row in dm_i18n.rows()}
        self.assertEqual(chronicle["dmChronRunewords"]["ptBR"], own["dmChronRunewords"][8] or own["dmChronRunewords"][2])
        _, text = touch_layout.read_layout(ROMFS, None, suffix="/lng/strings/ui.json")
        game = {row["Key"]: row for row in json.loads(text)}
        self.assertEqual(chronicle["Unique"]["ruRU"], game["Unique"]["ruRU"])
        problems = lambda size: [p for label in labels for p in tab_art.chronicle_tab_problems(self.font, label, size)]
        self.assertEqual(problems(tab_art.CHRONICLE_TAB_SIZE), [])
        self.assertNotEqual(problems(tab_art.CHRONICLE_TAB_SIZE + 1), [])


if __name__ == "__main__":
    unittest.main()
