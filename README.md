<p align="center"><img src="dm/ios/Assets.xcassets/AppIcon.appiconset/AppIcon1024.png" width="160" alt="D2R-Rakanishu logo"></p>

# D2R-Rakanishu

Diablo II: Resurrected on Android, with the extras I kept wishing the game had.

This is a fork of [d2r-mobile](https://github.com/jhaizhou-ops/d2r-mobile) by jhaizhou. They did the hard part. Their port runs the game's own ARM64 program on the phone and draws it with Vulkan, and the touch controls, endgame modes and farming helpers come from them too. I started playing it on my phone and then kept adding things. Most of them answer questions I had while playing. Can my character take the next boss? Is this ring actually better? Why does this helm look so bad?

You need your own copy of the game. The APK contains none of its files, and everything runs offline. The app builds its game pack from your files on the phone itself. All screenshots were taken on a phone (2480 × 1116), the pack build pages on an emulator with test files.

## What I added

### Transmog at Charsi

Bring Charsi her hammer back and she will change how your gear looks. Pick any identified item of the same kind that you have owned, preview it on your character and pay in gold. You can also hide your helm or shield. The character select shows the new looks too. The real items in your save stay as they are; the looks are kept in a separate file next to it. Each difficulty unlocks this again with its own hammer quest.

| Before | After |
|:---:|:---:|
| ![Before transmog](https://github.com/user-attachments/assets/b872c98b-8272-4bbd-950f-fb7133164eb3) | ![After transmog](https://github.com/user-attachments/assets/a3ba2bc8-b059-4f3e-aead-768a7656319c) |

![Charsi's Appearance tab with live preview and price](https://github.com/user-attachments/assets/41c82f39-ef4f-4fe6-a0c5-a7833762e656)

![The chosen look on the character select](https://github.com/user-attachments/assets/7067a254-7cb3-4c47-9030-ad623a03f9ae)

### Dye with gems

Gems in your inventory turn into colours in the same tab. Dyeing an item uses up one gem of that colour.

| Before | After |
|:---:|:---:|
| ![Before dyeing](https://github.com/user-attachments/assets/73b7493e-7919-4ad0-95b2-f31f7a3df51e) | ![After dyeing](https://github.com/user-attachments/assets/bcd77d7b-a288-45b3-a7e6-ec7197fd3d6f) |

![Gem colours in the Appearance tab](https://github.com/user-attachments/assets/506290ae-993b-4a6e-8ebc-48d4b2059890)

### Combat simulation

Before I walk into Andariel I want to know if I will come back out. The combat page simulates the next main boss, the mini bosses of the act and any other monster against your character as it is right now. You get the time to kill, the time until you die and a rating, and the numbers update when you change gear or skills. Open a fight to see how it plays out. The enemies have their pictures, made from your own game files when you build the import pack, with the colour variants of a monster family and the superuniques in their own colours.

| Overview | One fight |
|:---:|:---:|
| ![Act bosses and mini bosses with their pictures](https://github.com/user-attachments/assets/b912a8f8-8d02-416b-b84a-7f00c071258d) | ![One simulated fight with the enemy's picture](https://github.com/user-attachments/assets/0a332407-d9e9-4bd2-9623-d71982875920) |

### Monster book

Every kind of monster in the game has a tile in the book, with its picture and a badge in the colour of the combat simulation's rating against your character. Monsters you haven't killed yet on that difficulty stay grey. You can narrow the book down to one act or to the area you are in, or search by name. Search for an item instead and you get the monsters that can drop it, best chance first.

A monster's page has its life, defense, attacks and resistances, its skills, every area where it shows up with the monster level there, and how many you have killed on each difficulty. It lists everything the monster can drop with the chance per kill for your magic find, and one tap shows the simulated fight. The book is in the quick menu and is also the second tab of the combat page.

| Grid | A monster's page |
|:---:|:---:|
| ![The monster book, act 2](https://github.com/user-attachments/assets/a264a879-967d-4bff-84c5-5f95565e5e46) | ![Duriel's page with loot and chance per kill](https://github.com/user-attachments/assets/ba9bfc39-5f27-4ab8-b93b-76d3b10a22ef) |

### Best gear

This searches everything you carry and store (worn gear, inventory, every personal stash page and the shared tabs) and finds the best combination for your goal. You say how many hits you want to survive and how much boss damage matters compared to clearing groups. The search is exact, and it knows two weapons: the barbarian's weapon pairs, including a two-handed sword held in one hand, and the assassin's two claws with Weapon Block. Skills like Double Swing, Frenzy and Dragon Claw only count with the weapons they need, and when nothing is better, your worn gear stays on. With the stash open, one tap puts on a single item or the whole set. Items with auras, item skills or charges are left out for now.

![Best set for your goal, equip in one tap](https://github.com/user-attachments/assets/a8caf667-7799-419b-afd0-c09ce20866e5)

| Two weapons | Two claws |
|:---:|:---:|
| ![Barbarian: a new weapon for each hand](https://github.com/user-attachments/assets/4bb0d4c6-6e1e-4d83-912e-d81fbb3e1031) | ![Assassin: a pair of claws](https://github.com/user-attachments/assets/3652309e-51a2-4a3c-8718-725b591fab88) |

### Unsocketing

Put a socketed item, a Ral rune and a Scroll of Town Portal into the Horadric Cube and transmute. Every gem, jewel and rune comes back, and the item keeps its empty sockets. In return the item breaks and needs a repair, and a runeword on it is gone. Ethereal items can't be repaired, so they keep their durability. Inserts that don't fit into the cube go to your inventory. The game's own Hel rune recipe, which destroys the inserts, still works as before.

| Before | After |
|:---:|:---:|
| ![A socketed cap, a Ral rune and a scroll in the cube](https://github.com/user-attachments/assets/035750a4-aae1-429e-add4-8b76fdf5b7ef) | ![The runes back, the cap needs a repair](https://github.com/user-attachments/assets/5ba2c808-f973-431a-929e-25a6806018ae) |

### Achievements

Your characters collect achievements, 60 of them in eight groups: kills, the acts and difficulties, kill streaks and act bosses, Greater Rifts, the cube and the smith, rare treasures and the mercenaries. A short line above the action bar tells you when you unlock one. The Achievements page in the quick menu shows the day you got each one, or how far along you are, for the character you play or for all your characters together. Characters you already have get most of what they have done before filled in quietly the first time you play them. The achievements are kept in a small file next to each save and move with save export and import.

| This character | All characters |
|:---:|:---:|
| ![The campaign achievements of one character](https://github.com/user-attachments/assets/eea38ba8-eb51-40e7-b7f9-dd6e4c7e549a) | ![Hell with every class, counted over all characters](https://github.com/user-attachments/assets/915d3902-f74e-4437-804a-67826e4a7097) |

![An achievement unlocked in the field](https://github.com/user-attachments/assets/682d46d3-a1aa-4ba2-8e61-4b8b0724f961)

### Your real DPS

The fight log measures your own damage per second over your last 50 fights with your current gear and skills, shows the range, and splits it between your skills, your mercenary and your summons. Solo kills are compared with what the simulation expected. A curve shows how your DPS changed over time, with a marker wherever you changed gear, levelled up or spent points.

![Measured DPS with range and history](https://github.com/user-attachments/assets/c09fd8a4-1b77-473d-8dd7-782ac08d3d3f)

### Melee splash (optional)

Melee hits also hit the enemies in a cone in front of you. How much they take depends on your weapon, your class and your strength, and each of those hits shows its own damage number.

![One swing, a damage number on every enemy in the cone](https://github.com/user-attachments/assets/f3250c09-daf0-466b-9bfd-72430960a07d)

### Shrine stacking (optional)

Shrines no longer replace each other. Each one keeps its own timer, picking up the same shrine again adds to its time, and the symbols sit next to each other above your head. The stamina shrine can also make you run and walk 50 % faster.

| One shrine | Three shrines |
|:---:|:---:|
| ![One shrine](https://github.com/user-attachments/assets/e5ba4f12-73da-451f-b947-5ad6649744e5) | ![Three shrines side by side](https://github.com/user-attachments/assets/eb1326d8-04e2-45d2-9082-915396abfea3) |

### Item comparison in the tooltip

When you look at an item, the tooltip also tells you what it would change for your character. Resistances show up as how much less damage you take, attack and cast speed as animation frames, and you see the difference in life, block chance and skill levels (skill tab bonuses included). At the bottom is the combat simulation's verdict.

| Game tooltip | With comparison |
|:---:|:---:|
| ![Game tooltip](https://github.com/user-attachments/assets/72ae23f0-ba25-4672-9398-71ae09ed96b3) | ![Tooltip with comparison](https://github.com/user-attachments/assets/138ab8ee-699a-4f58-b09a-10323e5baf69) |

### Fight log

Each character keeps its last 50 fights, also across sessions. Every fight has a graph of your life during the fight, and where the simulation can compare, its version of the fight is drawn on top.

![Last fights with life graph and simulated fight](https://github.com/user-attachments/assets/5909785e-2c46-4609-9284-bb4b163ca14e)

### Kill streak (optional)

A small counter above the action bar. Keep hitting things to keep it going, climb through the tier titles, and get bonus experience when the streak ends.

![Kill streak counter with tier title](https://github.com/user-attachments/assets/6a0d45e0-e108-4d34-a0b8-74f016ead16b)

### Build guides, recipe book and glossary

There are 28 build guides with skill and stat plans. The cube recipe book is searchable and knows which recipes your stash can make right now. The glossary explains game mechanics using your character's current values.

| | |
|:---:|:---:|
| ![Build guide](https://github.com/user-attachments/assets/b2bd4685-74ce-4b7b-a419-3d62c9388d09) | ![Cube recipe book](https://github.com/user-attachments/assets/b378063a-3eb2-47ae-b09c-e5a90831d90e) |
| ![Glossary with your live values](https://github.com/user-attachments/assets/10058d00-240e-4dad-b2fd-c588ba390dc9) | |

### Smaller things

Most of the gameplay changes can be turned off on the Extras page. The pages I added use the game's own art and fonts, loaded from your files when the app starts.

The Video tab lets you install and pick a different Vulkan driver, for example Turnip. My phone had flickering stripes with its system driver, and a newer driver fixed them. The same tab has sliders for resolution, frame rate and 3D quality, and a picture preset that sets resolution and 3D sharpness in one go, up to the full screen resolution.

The added pages work with a controller. You can map the phone's shoulder triggers, see status effects with their remaining time, and keep a clock, battery and frame rate line on screen.

The game saves every minute, when you leave the app and after stash actions. It pauses in the background instead of being closed. If Android closes it anyway while you are out in the field, the next start takes you straight back to that spot. The game waits there, paused, with a To camp button, and after you continue, direct hits cost no life for up to 3 seconds. After a boss kill you start at the nearest waypoint you have instead.

The shared stash tabs have a Sort button too.

| | |
|:---:|:---:|
| ![The Extras page](https://github.com/user-attachments/assets/f258e228-a7d9-4254-9738-400ac9572000) | ![Driver picker in the Video tab](https://github.com/user-attachments/assets/67655f94-73db-4f69-bb0d-f6bcd55ee486) |
| ![Back where Android closed the game](https://github.com/user-attachments/assets/ba6a6753-a20a-49e2-8351-f182b3b17f21) | |

| Shared tab | After Sort |
|:---:|:---:|
| ![A shared stash tab before sorting](https://github.com/user-attachments/assets/598b5f9c-6069-4db0-bffc-e84f32e4f74d) | ![The same tab after Sort](https://github.com/user-attachments/assets/6a92ced7-112b-4ae1-86f4-466da5c3054a) |

From upstream, with thanks to jhaizhou and the people around the community port: the ARM64 and Vulkan runtime, touch and controller controls, Greater Rift, Boss Rush, Elysium mode and the Kill Board, the Chronicle stash, Trove and Pouch, auto-recast, auto-potions, the loot filter, auto-pickup, the runeword helper, save export and import, and the home screen widgets. From d2r-mobile 2.3.4 also come the lighting fix for Adreno 8xx GPUs, start fixes for Samsung Xclipse GPUs, older Adreno 650 and Huawei Maleoon drivers and ARMv8.0 CPUs, sorting the shared stash tabs with a long press, the mercenary's skills when you hold a row in the hire list, and auto-pickup leaving a rune on the ground once its stack in the stash holds 99.

## Install and build

- [Install the APK](#install)
- [Build the game pack on your phone](#build-the-game-pack-on-your-phone)
- [Build the game pack on a computer (alternative)](#build-the-game-pack-on-a-computer-alternative)
- [Saves and switching from d2r-mobile](#saves-and-switching-from-d2r-mobile)
- [Build the APK](#build-the-apk)

## Install

You need an ARM64 phone with Android 12 or newer and Vulkan 1.1. I can't promise it runs on every phone that meets that, since GPUs differ a lot.

1. Download the `player` APK from the release and check it against the SHA-256 checksum next to it.
2. Install it, open it and allow access to all files when asked. The application ID is `io.r4k4n1shu.d2r`.
3. Build the game pack on the phone, as described next.

Back up your saves before you upgrade or uninstall.

## Build the game pack on your phone

Bring three files from your own copy of the game, saved in the phone's own storage (not a cloud folder):

- the base game as an `.nsp` file,
- the update 1.0.35 (game version 3.4.95035) as an `.nsp` file,
- your key file `prod.keys`.

The build needs about 8 GB of free space besides those two game files. It reads them on the phone and nothing leaves it; the key file is only held in memory while it is read and is never stored.

On the first page, tap **Build on this phone**. Pick each file with the button in its row, check the space on the next page and tap **Start**. The bar shows how far the build is, and when it is full you tap **Start game**. It took 4:39 minutes on a RedMagic 10 Pro and about 5.5 minutes on a RedMagic 8 Pro. Stay on the page while it runs; the screen stays on.

| Your files | The build |
|:---:|:---:|
| ![The game files page with three files picked](https://github.com/user-attachments/assets/e61f0863-7123-48a2-b646-8c6e5728d5b8) | ![The pack build running](https://github.com/user-attachments/assets/7ed5846c-c6c4-45a4-bf6f-2fccd98938bf) |

After that you can delete the two game files and the key file. The game archives the build unpacked stay in `Download/S2X/D2R Mobile/data/`, where the game reads them. When an app update needs a new pack, the first page says so, and one tap on **Rebuild game pack** makes it again from what the phone kept, without your files. Only a reinstall or a rare large update asks for them again.

## Build the game pack on a computer (alternative)

The supported input is the Switch version 3.4.95035 (console update 1.0.35) with `Update/ExeFS/` and `Update/RomFS/data/`, taken from your own copy of the game. The upstream docs in `docs/d2r-mobile/` explain how to get them. This project does not provide game files.

You need Python 3.9 or newer with `pillow numpy lz4 capstone`, a C/C++ compiler (clang or gcc), the .NET SDK, and the Vulkan tools `spirv-opt`, `spirv-val` and `glslc`. The pack tool tells you if something is missing. From the source tree of the release:

```sh
python3 dm/tools/d2rm_android.py /path/to/Update /path/to/private-pack
```

This creates `/path/to/private-pack/android/`. Copy it and the resource directory to the phone like this:

```text
Download/S2X/D2R Mobile/
  data/                    <- the complete RomFS/data directory
  r4k4n1shu/
    android/               <- the complete import pack
      pack_info.txt
      out/
      shaders/
      overlay/
      ...
```

Then open the app; it imports the pack (on the first page, **Import from computer** does the same). The pack has to match the APK's product code. The big resource archives stay in the shared `data/` folder, and the app only copies the much smaller pack into its own storage. The pack contains game content, so keep it private and out of any repository. When the native patch tools or overlay layouts change, you have to build the pack again, because a new APK alone can't update those parts.

If you already have a matching pack in the old shared `android/` folder, the app moves it to `r4k4n1shu/`, or reads it where it is if moving fails. A pack in the new place always wins, and the game archives are never copied twice.

## Saves and switching from d2r-mobile

Export and Import are on the app's technical page. You can also do it by hand in `Download/S2X/D2R Mobile/r4k4n1shu/`. An empty file named `export` makes the app export to `save-export/<stamp>/`, and a folder named `save-import/` is imported on the next start. A `save-import/` folder that only holds `Settings/` is accepted too; its settings are used when the phone has none yet. Each character's achievements travel with it.

Coming from an upstream version that can export: export there first, copy the contents of its `save-export/<stamp>/` into our `r4k4n1shu/save-import/`, and start this app. Keep the original export as a backup. Both apps share the game archives, but saves, packs and transfer markers are separate. Check your characters, shared stash and settings before you remove the old app.

## Build the APK

You need JDK 17 or newer, Gradle 9.8.1, Android SDK 36, NDK 29.0.14206865 and CMake 3.31.6. Point `JAVA_HOME`, `ANDROID_HOME`, `DM_ROMFS_SRC` and `DM_PYTHON` at your installation and game files. More in the [Android build details](dm/android/README.md).

```sh
dm/android/tools/shell_apk.sh -PdmVariant=qa
dm/android/tools/shell_apk.sh release -PdmVariant=player \
  --builder "$DM_ROOT/build/android_python" "$DM_ROOT/build/android_xlate"
```

The player build with `--builder` carries the pack builder that makes the game pack on the phone; its Python runtime and shader translator are built from pinned recipes, see [BUILDING.md](BUILDING.md). Without `--builder` it imports computer-built packs only.

The APKs end up in `${DM_ROOT:-$HOME/dm-work}/build/android_shell/` as `D2RMobile-android.apk` and `D2RMobile-android-player.apk`. The qa build has the test tools and is debuggable in debug mode. The player build has no test controls and is never debuggable. Both are signed with the same key.

Create your signing key yourself and keep it outside the repository. Put its settings in `~/.config/d2r-release/signing.properties`:

```properties
storeFile=/private/path/release.keystore
storeType=PKCS12
keyAlias=release
storePassword=your-store-password
keyPassword=your-key-password
```

A different path can be given with `-PdmSigningProperties=/private/path/signing.properties` or `DM_SIGNING_PROPERTIES`. The environment variables `DM_ANDROID_KEYSTORE`, `DM_ANDROID_STORE_TYPE`, `DM_ANDROID_KEY_ALIAS`, `DM_ANDROID_STORE_PASSWORD` and `DM_ANDROID_KEY_PASSWORD` override the file. A relative keystore path is read relative to the properties file.

Without a release key, qa debug builds fall back to the Android debug key and print a warning. Release and player builds stop instead, unless you allow the debug key for local testing with `-PdmAllowDebugSigning=true` or `DM_ALLOW_DEBUG_SIGNING=1`. Don't publish an APK signed with the debug key. The scripts never create a key; a missing debug key or an incomplete release setup stops the build. The old `~/.config/dm-android/release.env` is no longer read, so move its settings to the properties file or the variables above.

Use the same key for qa and player if one should replace the other. Player builds don't allow `adb run-as`, so move private data with a qa build signed with the same key before you install the player build.

## Credits and license

Based on [d2r-mobile](https://github.com/jhaizhou-ops/d2r-mobile) by jhaizhou. Thanks to biglawine for the HarmonyOS port and to everyone who contributed upstream.

The [MIT license](LICENSE) is unchanged, and third-party components keep their own licenses. Diablo II: Resurrected and its content belong to their owners and are not part of this project.

The app's pack builder contains CPython, NumPy, Pillow, the Ryujinx shader translator and the .NET runtime, among others. [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) names every third-party part with its license text; the same file ships inside the APK. FriBiDi is used under the LGPL 2.1 or later, and its source archive is attached to every release.

This project is not affiliated with or endorsed by Blizzard Entertainment or Nintendo.

The original upstream READMEs and contribution guides are kept unchanged in [docs/d2r-mobile/](docs/d2r-mobile/README.md). Their relative links refer to the upstream layout.
