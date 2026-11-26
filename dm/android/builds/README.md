# Build guides

Each file `<id>.json` here is one build guide written by us: our own words in German and English, plus keys, ids and
numbers from the game tables of 3.4.95035. No game text, no names, no pictures: those come from the player's own game
files when the import pack is made (`dm/android/tools/gen_build_names.py` → `appart/builds/`). The guides go into the APK
as `assets/builds/` (`copyBuildGuides` in `dm/android/app/build.gradle`); `shell_apk.sh` lets exactly these files into the
APK and checks that they are byte-identical to this folder and pass the validator.

## Format

`schema.json` describes the structure (version 1). In short:

| Field | Meaning |
|---|---|
| `id`, `version` | file name without `.json`; raise `version` on every content change |
| `class`, `classIndex` | `ama sor nec pal bar dru ass war` = 0..7 |
| `title`, `summary`, `overview` | our own text, `de` and `en` (other app languages show English) |
| `tags`, `difficulty`, `mode`, `patch` | card facts; reference is Softcore non-ladder |
| `keySkills`, `trees` | one to three skill ids for the card icons, skill pages for the card subtitle |
| `phases[]` | `id`, `name`, `levels [from, to]` (no overlap), `plan` `cumulative` (continues the previous plan) or `respec` (starts from zero, only becomes active after the player confirms it on the page) |
| `phases[].skills[]` | `{lvl, skill, key, to, save?, note?}`: from character level `lvl` on, raise `skill` (id + `skills.txt` key) to the base level `to`; `save` = when this step is blocked, keep the point instead of skipping to a later step |
| `phases[].stats[]` | rules top to bottom on **base** points: `{stat, to}` up to a target, `{stat, none}` nothing, last rule `{stat, rest}` |
| `gear[]` | `{phase, slot, tier budget/bis/alt, ref, base?, note?}`; `ref` is one of `runeword: RunewordNN` (`Runes.txt` `Name`), `unique`/`set` (`index` column), `item` (code), `type` (item type + optional quality); rune words need `base.sockets` and `base.item` or `base.type` |
| `merc` | `act`, `difficulty` 1..3, `aura` (skill key in `hireling.txt`), gear like above |
| `profile` | the build profile of the sim (see the sim docs): damage types, key skills, `itemstatcost` stat weights, caps, breakpoints, requirements, `bestgear` (the item rating goal: `survivalHits` enemy hits to survive, `bossShare` 0..1 of the single target damage in the damage value); a phase may override top-level keys |
| `sources[]` | site, title, `https` link, `seen` date, patch, kind; at least two, from different sites for "two-sources" |
| `review` | `checks` (`schema`, `tables-3.4.95035`, `points-budget`, `two-sources`, `review`), `played` |

### The allocation rule

Shared by the validator (`check_builds.py next_step`) and the app (`BuildPlan.next`), with common cases in
`dm/android/tests/build_plan_cases.tsv`: scan the steps in order and skip complete ones. A step planned at or below the
character level that can be learned now gets the point (without free points it is only shown as the point for the next level-up);
a permanently blocked one (cap, rift lock, data error) is reported as blocked; one that cannot be learned yet is skipped, unless it
has `save`. The scan stops at a step with `save` that cannot be learned yet, at the first step whose `lvl` is above the character
level, or when only steps that cannot be learned yet are left. Then, without free points, the plan waits and names the level at
which the next point is really spent (when that is not the next level-up). With free points the plan is simulated level by level
(one point per level, no quest points, as many points per level as it can use): if the step it stopped at can be learned now and
no level before the plan spends on it needs all the points held, it gets the point now; points beyond the largest shortfall of
any later level go to the first step after it that can be learned now and is planned no later than the level of that shortfall;
otherwise all points are kept for that step (the card shows "N points to keep", the step, the level the plan starts it at and
the step's reason when it has `save` and its note gives one).
"Can be learned" never depends on free points: level requirement + current base level ≤ character level, prerequisites
learned, below the cap. A cumulative phase walks the steps of the earlier phases first.

### Quest points (assumption)

The validator simulates every phase twice: with the documented quest assumption below and without any quest point.
Without quests it must never hit a dead end; with quests every phase must be complete by its last level.

| Difficulty | Den of Evil (+1) | Radament (+1) | Izual (+2) |
|---|---|---|---|
| Normal | level 8 | 18 | 27 |
| Nightmare | 40 | 45 | 52 |
| Hell | 70 | 72 | 76 |

## Checking

```bash
python3 dm/android/tools/check_builds.py                            # without game files (CI)
python3 dm/android/tools/check_builds.py --romfs <RomFS> --report   # with the 3.4.95035 tables (local, read only)
python3 dm/android/tests/test_check_builds.py                       # validator unit tests
dm/android/tests/run.sh                                             # BuildPlanTest, GearPreviewTest (Java) and RecipeMatchTest
```

With `--romfs` the validator also checks the **gear preview data**: it builds the guide names exactly as the import pack
does (`gen_build_names.verified`, same table and result fingerprints) and requires for every gear ref of every guide (mercenary
included) a preview entry: uniques / set items / rune words with their table row (the host makes the game's own property
text by it), the base item, the runes' short names, the rune word's base item or type, item types and their quality, and the
tooltip labels (required level / strength / dexterity, defense, damage, socketed, ethereal).

### Gear preview

Tapping a gear row (gear and mercenary tabs) or a key item on the overview, or A on it with the controller, shows a card in
the look of the game's item tooltip (`ItemTooltip`, style module, shared with the recipe book's preview): the name in the
quality colour, the base, runes and sockets for rune words, requirements, and the property lines the game itself writes in the
current game language (`nativeItemText`: the host makes them on the game thread through the original Chronicle path, on demand,
cached per language). Names and labels come from the import pack in German or English. A tap outside, B, the menu
button or Back closes it; a long card turns its page with a tap on it or left / right. Item types (magic boots and the like)
show the type and quality only, their affixes are random.

"Checked" (shown as "Tables ✓ · Review ✓") means: the validator passes against the 3.4.95035 tables, at least two
independent sources were read, and a second review of the guide against those sources found no major problem (check
`review`). "Play-test" is a separate third mark.

## Writing a guide

1. Read at least two established sources; note link, date and patch. Write the plan, rules, gear (as keys), mercenary and
   profile **in our own words**; never copy text, tables or pictures. Shops that sell items are secondary sources only.
2. Run the validator with the game tables and read its report (completion level of every step, requirements from the gear).
3. Have the guide reviewed against the sources by a second reviewer, then fill in `review`.
4. Every season or patch: read the sources again, raise `version`, update the source access dates.

## Guides

Selection: for every class, every build rated S or A by at least two of the three tier lists Maxroll
(non-ladder), Icy Veins (endgame) and DiabloBytes (overall); for the Warlock only Icy Veins and DiabloBytes count, and both
must rate it S or A. Reference: Softcore non-ladder, patch 3.3 (all three lists were last re-ranked for 3.2 / season 14).
That gives 25 builds; the three earlier guides outside this selection (`bar-ww`, `dru-wind`, `war-demon`) stay. Sources that
sell items (`kind: vendor`) are listed but never count for "two-sources". Where only older sources were readable, the guide
says so in its source entries and an overview note. Sunder charms point to the latent row (`PreCrafted …`, the charm that
drops since 3.3) or the renewed one (`Crafted …`); the validator rejects the old rows.

All 28 guides are checked (tables with gear preview data, two sources read in full, second review); none is play-tested yet.

| Class | Build | id | Tier lists (S/A) | Sources read in full |
|---|---|---|---|---|
| Amazon | Lightning spear (Lightning Fury / Charged Strike) | `ama-javelin` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes 06/2026, Gamer Guides 2023 (leveling order) |
| Amazon | Lightning Strike (melee) | `ama-lstrike` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2), Gamer Guides 2023 (leveling order) |
| Amazon | Strafe (bow) | `ama-strafe` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Amazon | Multi-Shot (physical bow) | `ama-multishot` | Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Sorceress | Blizzard | `sor-blizzard` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, D2Runewizard 04/2025 (before 3.0) |
| Sorceress | Lightning | `sor-light` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Sorceress | Nova | `sor-nova` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Sorceress | Fire Ball / Meteor | `sor-fireball` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Sorceress | Enchant | `sor-enchant` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Necromancer | Summoner | `nec-summon` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, D2Runewizard 07/2025 (before 3.2), DiabloBytes 06/2026 |
| Necromancer | Poison Nova | `nec-poison` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Paladin | Hammerdin | `pal-hammer` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, Gamer Guides 2023 / D2Runewizard 04/2025 (before 3.0) |
| Paladin | Fist of the Heavens | `pal-foh` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Paladin | Smiter | `pal-smite` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Paladin | Dream (two Dreams, Holy Shock) | `pal-dream` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (undated) |
| Barbarian | Berserk | `bar-berserk` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Barbarian | Gold Find (Berserk + Find Item) | `bar-goldfind` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Barbarian | Double Throw | `bar-dthrow` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Barbarian | Whirlwind | `bar-ww` | Icy Veins only (earlier guide) | Maxroll S14, DiabloBytes 06/2026 |
| Druid | Fissure (fire) | `dru-fissure` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.3 header), Icy Veins (2.8, archived copy) |
| Druid | Elemental (Tornado / Hurricane) | `dru-wind` | Icy Veins only (earlier guide) | Maxroll S14, DiabloBytes 06/2026 |
| Assassin | Traps (Lightning / Death Sentry) | `ass-traps` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes 06/2026 |
| Assassin | Mosaic (Phoenix Strike), non-ladder | `ass-mosaic` | Maxroll, Icy Veins, DiabloBytes | Maxroll S14, DiabloBytes (3.3 / S15) |
| Assassin | Wake of Fire (fire traps) | `ass-wof` | Maxroll, DiabloBytes | Maxroll S14, DiabloBytes (3.2) |
| Warlock | Fire (Ring of Fire, Flame Wave, Apocalypse) | `war-fire` | Icy Veins, DiabloBytes | Maxroll S14, Mein-MMO / Game Rant |
| Warlock | Echoing Strike (Eldritch, Hex: Purge) | `war-echo` | Icy Veins, DiabloBytes | Maxroll S14, D2Runewizard |
| Warlock | Abyss (magic) | `war-abyss` | Icy Veins, DiabloBytes | Maxroll S14, Icy Veins 3.2 (archived copy), DiabloBytes, D2Emu |
| Warlock | Demon summoner | `war-demon` | none (earlier guide) | Maxroll S14, Blizzard forum guide |

Every guide also cites the official 3.3 patch notes. Most sources predate 3.3; the guides name the patch of every source.
