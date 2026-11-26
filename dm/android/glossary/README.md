# Glossary

`terms.json` holds our own explanations of game terms in German and English, plus stat ids and string keys from the
game tables of 3.4.95035 and numbers of the game mechanics. It contains no game text: every term name comes from the
player's own game strings at runtime (the import pack's `appart/effects.json`: `stats`, `groups`, `words`), and the
monster values for hit chances come from `appart/glossary.json` (`dm/android/tools/gen_glossary_data.py`, made from
`monlvl.txt` and `difficultylevels.txt` when the pack is built). The file goes into the APK as
`assets/glossary/terms.json` (`copyGlossary` in `dm/android/app/build.gradle`); `shell_apk.sh` checks that it is
byte-identical to this folder and passes the validator.

| Field | Meaning |
|---|---|
| `categories` | `def`, `off`, `speed` with `de` / `en` labels (the tabs) |
| `sources` | site, title, `https` link, `seen` date, note; every term needs two sources from different sites |
| `breakpoints.fcr` / `.fhr` | per class (`ama sor nec pal bar dru ass war` = 0..7) `[percent, frames]` steps, base form only |
| `phrases` | the "what does it mean for you" sentences, `{n}` placeholders, the value in `[ ]` is drawn in the game's blue |
| `terms[]` | `id`, `cat`, `name` (`{stat}`, `{group}` or `{word}` of effects.json), `fallback` name, `keywords` for the search, `stats` (status card links), `text` (`what`, `how`, `helps`, `not`, `rule`), `limits` (what the live values do not cover), `sources`, `checked` (tables and columns checked) |

Check: `python3 dm/android/tools/check_glossary.py [--tables <extracted .txt tables>]`; tests:
`python3 dm/android/tests/test_check_glossary.py` and `GlossaryTest` in `dm/android/tests/run.sh`.
