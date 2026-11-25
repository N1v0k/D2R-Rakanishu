
import hashlib
import os
import re
import struct


def text_metrics(romfs, strings):
    import io
    import json
    from PIL import ImageFont
    import romfs_art
    raw = romfs_art.read(romfs, "hd/ui/fonts/ExocetBlizzardOT-Medium.otf")
    tables = {}
    for index in range(struct.unpack_from(">H", raw, 4)[0]):
        tag, _, offset, length = struct.unpack_from(">4sIII", raw, 12 + index * 16)
        tables[tag] = raw[offset:offset + length]
    em = struct.unpack_from(">H", tables[b"head"], 18)[0]
    ascent, descent = struct.unpack_from(">hh", tables[b"OS/2"], 68)
    if not 0 < ascent - descent <= em * 2:
        raise ValueError("Unexpected tab font metrics")
    font = ImageFont.truetype(io.BytesIO(raw), em)
    _, cap_top, _, cap_bottom = font.getbbox("H", anchor="ls")
    keys = {"Audio", "Gameplay", "dmTabGameSettings", "dmTabCustom", "OptionsMenuVideo", "Controls", "Unique", "Sets", "dmChronRunewords", "Bosses"}
    with open(strings, encoding="utf-8-sig") as source:
        rows = [row for row in json.load(source) if row["Key"] in keys]
    if {row["Key"] for row in rows} != keys:
        raise ValueError("Missing tab labels")
    labels = sorted({row[lang] for row in rows for lang in ("enUS", "deDE", "ptBR")})
    result = ["TAB_INK_V1"]
    for label in labels:
        if not label or len(label.encode("utf-8")) >= 128 or any(c in label for c in "\t\r\n"):
            raise ValueError("Invalid tab label")
        _, top, _, bottom = font.getbbox(label, anchor="ls")

        delta = round((cap_top + cap_bottom - top - bottom) * 1000 / (2 * (ascent - descent)))
        result.append("%d\t%s" % (delta, label))
    return "\n".join(result + ["END", ""])


def widen(frame, width):
    from PIL import Image
    if frame.size not in ((134, 60), (136, 58)) or width < frame.width or width % 2:
        raise ValueError("Unexpected tab dimensions")
    image = Image.new("RGBA", (width, frame.height))
    source = (0, 6, frame.width // 2 - 8, frame.width // 2 + 8, frame.width - 6, frame.width)
    target = (0, 6, width // 2 - 8, width // 2 + 8, width - 6, width)
    for left, right, x0, x1 in zip(source, source[1:], target, target[1:]):
        part = frame.crop((left, 0, right, frame.height))
        image.paste(part.resize((x1 - x0, frame.height), Image.Resampling.BILINEAR), (x0, 0))
    return image


def build(romfs, outdir, widths, chronicle=False):
    from PIL import Image
    import romfs_art
    import gen_widestash_art as codec
    path = "hd/global/ui/Panel/Chronicle/Chronicle_Tabs.sprite" if chronicle else "hd/global/ui/Controller/Panel/Stash/V2/StashTabs.sprite"
    raw = romfs_art.read(romfs, path)
    version, frame_width, sheet_width, height = struct.unpack_from("<HHII", raw, 4)
    frames = struct.unpack_from("<H", raw, 20)[0]
    expected = (61, 136, 273, 58, 2) if chronicle else (61, 134, 539, 60, 4)
    if (version, frame_width, sheet_width, height, frames) != expected:
        raise ValueError("Unexpected source tab sprite")
    lib = codec.lib()
    source = codec.decode(lib, raw)
    replacements = {}
    for units in sorted(set(widths)):
        if units % 2 or units < frame_width * 2:
            raise ValueError("Tab slot must contain an even number of layout units")
        width = units // 2

        sheet = Image.new("RGBA", (width * frames, height))
        for index in range(frames):
            left = index * (sheet_width // frames)
            frame = source.crop((left, 0, left + frame_width, height))
            sheet.paste(widen(frame, width), (index * width, 0))
        name = ("chronicle_tabs_%d.sprite" if chronicle else "options_tabs_%d.sprite") % units
        path = os.path.join(outdir, name)
        with open(path, "wb") as output:
            output.write(codec.pack(lib, raw, codec.bc3(sheet), sheet.width, height, width, frames))
        replacements["data/hd/global/ui/ztouch_options_ui/" + name] = path
    return replacements



SKILL_TAB_KEYS = tuple(("skills.json", "SkillCategory%s%d" % (cls, index))
                       for cls in ("Am", "So", "Ne", "Pa", "Ba", "Dr", "As", "Wa") for index in (1, 2, 3)) + (("ui.json", "GeneralSkills"),)


SKILL_TAB_LANGS = ("enUS", "deDE", "esES", "esMX", "frFR", "itIT", "plPL", "ptBR", "ruRU")
SKILL_TAB_KERNING, SKILL_TAB_LEADING = 0.95, 0.9

SKILL_TAB_SIZE, SKILL_TAB_WIDTH, SKILL_TAB_HEIGHT = 30, 285, 102

SKILL_TAB_INNER = 303

SKILL_TAB_CLEARANCE = 7

CHRONICLE_TAB_SIZE, CHRONICLE_TAB_INNER = 31, 386
CHRONICLE_TAB_KEYS = (("ui.json", "Unique"), ("ui.json", "Sets"), ("ui.json", "dmChronRunewords"), ("ui.json", "Bosses"))





SKILL_TAB_WRAP_BAND = (1.001, 1.009)







SKILL_TAB_REWRITES = {
    ("skills.json", "SkillCategoryAm2", "ruRU"): ("6f871d5b82cff11dab8f5b996a83269ab2679dbd38255083f57ca6cd207d968a", ("words", 3)),
    ("skills.json", "SkillCategoryWa2", "ruRU"): ("2b69930c2595f7d5fb3a0d66e3a9380a4ff05705d751f77692d8782ad523b2f5", ("break", 10)),
    ("skills.json", "SkillCategoryNe1", "plPL"): ("2860cd7e6c44e16a7c44b7a6178103680824bf90eb86236dfe08c32dcaacf583", ("break", 8)),
    ("skills.json", "SkillCategoryWa2", "plPL"): ("e97f86ae32aa1ec61aad4ae970f61fade97e3352ee3a98cf4b7f14f79118aed2", ("break", 6)),
    ("skills.json", "SkillCategoryWa2", "itIT"): ("f9fc5ddf263d7d0353ad9f8e926d8ee7d29b3ac62acdfce0fff33dc47e10fec1", ("break", 5)),
}


def rewrite_skill_tab_label(label, rewrite):

    digest, (kind, count) = rewrite
    if hashlib.sha256(label.encode("utf-8")).hexdigest() != digest:
        raise ValueError("Skill tab label does not match its rewrite fingerprint")
    if kind == "break" and 0 < count < len(label) and label[count - 1].isalpha() and label[count].isalpha():
        return label[:count] + "-\n" + label[count:]
    words = label.split(" ")
    if kind == "words" and 0 < count < len(words):
        return " ".join(words[:count])
    raise ValueError("Skill tab rewrite rule %r does not fit its label" % ((kind, count),))


class TabFont:


    def __init__(self, raw):
        import io
        from PIL import ImageFont
        tables = {}
        for index in range(struct.unpack_from(">H", raw, 4)[0]):
            tag, _, offset, length = struct.unpack_from(">4sIII", raw, 12 + index * 16)
            tables[tag] = raw[offset:offset + length]
        self.em = struct.unpack_from(">H", tables[b"head"], 18)[0]
        ascent, descent = struct.unpack_from(">hh", tables[b"OS/2"], 68)
        line_ascent, line_descent = struct.unpack_from(">hh", tables[b"hhea"], 4)
        if not 0 < ascent - descent <= self.em * 2 or not 0 < line_ascent - line_descent <= self.em * 3:
            raise ValueError("Unexpected tab font metrics")

        self.span = ascent - descent
        self.line_span = line_ascent - line_descent
        self.font = ImageFont.truetype(io.BytesIO(raw), self.em)
        self.cache = {}

    @classmethod
    def load(cls, romfs):
        import romfs_art
        return cls(romfs_art.read(romfs, "hd/ui/fonts/ExocetBlizzardOT-Medium.otf"))

    def _units(self, text, kerning):
        key = (text, kerning)
        if key not in self.cache:
            x, left, right = 0.0, None, None
            for char in text:
                x0, _, x1, _ = self.font.getbbox(char, anchor="ls")
                if x1 > x0:
                    left = x + x0 if left is None else min(left, x + x0)
                    right = x + x1 if right is None else max(right, x + x1)
                x += self.font.getlength(char) * kerning
            self.cache[key] = 0.0 if left is None else right - left
        return self.cache[key]

    def width(self, text, size, kerning=SKILL_TAB_KERNING):
        return self._units(text, kerning) * size / self.span

    def line_pitch(self, size, leading=SKILL_TAB_LEADING):
        return self.line_span * size * leading / self.span

    def cap_height(self, size):
        _, top, _, bottom = self.font.getbbox("H", anchor="ls")
        return (bottom - top) * size / self.span


def wrap(font, text, size, limit):

    lines = []
    for word in text.replace("\n", " ").split():
        candidate = lines[-1] + " " + word if lines else word
        if lines and font.width(candidate, size) <= limit:
            lines[-1] = candidate
        else:
            lines.append(word)
    return lines


def layouts(font, label, size, width):

    low, high = (width * factor for factor in SKILL_TAB_WRAP_BAND)
    words = label.replace("\n", " ").split()
    limits = {low, high}
    for first in range(len(words)):
        for last in range(first + 2, len(words) + 1):
            line = font.width(" ".join(words[first:last]), size)
            if low < line <= high:
                limits.update((line, line - 1e-6))
    result = []
    for limit in sorted(limits):
        lines = wrap(font, label, size, limit)
        if lines not in result:
            result.append(lines)
    return result


def skill_tab_problems(font, label, size, width, height):


    problems = []
    for word in label.replace("\n", " ").split():
        if font.width(word, size) > width * SKILL_TAB_WRAP_BAND[0]:
            problems.append("word %r is wider than the box" % word)
    for lines in layouts(font, label, size, width):
        if len(lines) > 2:
            problems.append("%d lines: %r" % (len(lines), lines))
        widest = max(font.width(line, size) for line in lines)
        if widest > SKILL_TAB_INNER - 2 * SKILL_TAB_CLEARANCE:
            problems.append("line ink %.1f leaves less than %d units to the rims: %r" % (widest, SKILL_TAB_CLEARANCE, lines))
        if font.cap_height(size) + font.line_pitch(size) * (len(lines) - 1) > height - 2 * SKILL_TAB_CLEARANCE:
            problems.append("%d lines do not fit the box height: %r" % (len(lines), lines))
        if "-\n" in label and lines != label.split("\n"):
            problems.append("the hyphenated break is not where the translators put it: %r" % lines)
    return problems


def skill_tab_labels(romfs, repl=None):

    import json
    import touch_layout
    tables, result = {}, {}
    for name, key in SKILL_TAB_KEYS:
        if name not in tables:
            path, text = touch_layout.read_layout(romfs, None, suffix="/lng/strings/" + name)
            previous = (repl or {}).get(path.lower())
            if previous:
                with open(previous, encoding="utf-8-sig") as source:
                    text = source.read()
            tables[name] = {row["Key"]: row for row in json.loads(text)}
        row = tables[name].get(key)
        if row is None or any(not isinstance(row.get(lang), str) or not row[lang] for lang in SKILL_TAB_LANGS):
            raise ValueError("Missing skill tab label %s" % key)
        result[(name, key)] = {lang: row[lang] for lang in SKILL_TAB_LANGS}
    return result


def shown_labels(font, labels, size, width):



    result = {}
    for (name, key), texts in labels.items():
        result[(name, key)] = shown = {}
        for lang, label in texts.items():
            rewrite = SKILL_TAB_REWRITES.get((name, key, lang))
            if rewrite:
                try:
                    label = rewrite_skill_tab_label(label, rewrite)
                except ValueError as error:
                    raise ValueError("Unexpected %s skill tab label %s: %s" % (lang, key, error)) from None
            whole = label.replace("-\n", "")
            if "-\n" in label and all(font.width(word, size) <= width * SKILL_TAB_WRAP_BAND[0] for word in whole.split()):
                label = whole
            shown[lang] = label
    return result


def build_skill_strings(romfs, outdir, repl, size, width):

    import json
    import touch_layout
    font = TabFont.load(romfs)
    labels = skill_tab_labels(romfs, repl)
    changes = {(name, key, lang): label for (name, key), texts in shown_labels(font, labels, size, width).items()
               for lang, label in texts.items() if label != labels[(name, key)][lang]}
    replacements = {}
    for name in sorted({name for name, _, _ in changes}):
        path, text = touch_layout.read_layout(romfs, None, suffix="/lng/strings/" + name)
        previous = repl.get(path.lower())
        if previous:
            with open(previous, encoding="utf-8-sig") as source:
                text = source.read()
        for (table, key, lang), label in sorted(changes.items()):
            if table != name:
                continue
            matches = list(re.finditer(r'"Key":\s*"%s",.*?"%s":\s*("(?:[^"\\]|\\.)*")' % (re.escape(key), lang), text, re.S))
            if len(matches) != 1 or json.loads(matches[0].group(1)) != labels[(table, key)][lang]:
                raise ValueError("Unexpected %s skill tab label %s" % (lang, key))
            text = text[:matches[0].start(1)] + json.dumps(label, ensure_ascii=False) + text[matches[0].end(1):]
        output = os.path.join(outdir, "skill_tabs_" + name)
        with open(output, "w", encoding="utf-8-sig", newline="") as target:
            target.write(text)
        replacements[path.lower()] = output
    return replacements


def chronicle_tab_labels(romfs):

    import json
    import dm_i18n
    import touch_layout
    _, text = touch_layout.read_layout(romfs, None, suffix="/lng/strings/ui.json")
    rows = {row["Key"]: row for row in json.loads(text)}

    own = {key: {lang: {"esES": es, "esMX": mx, "ruRU": ru, "deDE": de, "ptBR": pt or en}.get(lang, en) for lang in SKILL_TAB_LANGS}
           for key, _, en, es, mx, ru, _, de, pt in dm_i18n.rows()}
    result = {}
    for _, key in CHRONICLE_TAB_KEYS:
        texts = own.get(key) or {lang: rows.get(key, {}).get(lang) for lang in SKILL_TAB_LANGS}
        if any(not isinstance(texts[lang], str) or not texts[lang] or "\n" in texts[lang] for lang in SKILL_TAB_LANGS):
            raise ValueError("Missing Chronicle tab label %s" % key)
        result[key] = texts
    return result


def chronicle_tab_problems(font, label, size):
    widest = font.width(label, size)
    if widest > CHRONICLE_TAB_INNER - 2 * SKILL_TAB_CLEARANCE:
        return ["line ink %.1f leaves less than %d units to the rims" % (widest, SKILL_TAB_CLEARANCE)]
    return []
