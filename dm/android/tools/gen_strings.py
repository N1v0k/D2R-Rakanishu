#!/usr/bin/env python3
"""从统一中英表生成安卓启动界面的资源；不读取游戏文件。
Generate Android startup resources from the shared string table; no game files.
"""
import argparse
import importlib.util
from pathlib import Path
from xml.sax.saxutils import escape


def generate(output):
    dm = Path(__file__).resolve().parents[2]
    spec = importlib.util.spec_from_file_location("dm_i18n", dm / "ios/tools/dm_i18n.py")
    table = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(table)
    shared = {"dmInputTitle", "dmNameTitle", "dmNameRule", "dmQtyTitle", "dmNameBlankRandom",
              "dmNamePickIme", "dmNameTooShort", "dmNameEdge", "dmCancel", "dmConfirm", "dmTcCancel", "dmHudLife", "dmHudMana"}
    rows = [r for r in table.rows() if r[0].startswith("dmAndroid") or r[0] in shared]



    for folder, column, lang in (("values", 2, "en"), ("values-zh", 1, "zh"), ("values-es", 3, "es"), ("values-b+es+419", 4, "mx"),
                                 ("values-ru", 5, "ru"), ("values-ko", 6, "ko"), ("values-de", 7, "de"),
                                 ("values-pt", 8, "pt")):
        lines = ['<?xml version="1.0" encoding="utf-8"?>', '<resources>']


        lines.append(f'    <string name="dmAndroidStringsLang" translatable="false">{lang}</string>')
        for row in rows:

            value = escape(row[column] or row[2]).replace("\\", "\\\\").replace("'", "\\'").replace('"', '\\"').replace("\n", "\\n")


            attribute = ' formatted="false"' if row[0] in {"dmHudLife", "dmHudMana", "dmAndroidHudLevel"} else ''
            lines.append(f'    <string name="{row[0]}"{attribute}>{value}</string>')
        lines.append('</resources>')
        directory = output / folder
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "strings.xml").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    generate(parser.parse_args().output)
