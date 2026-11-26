# Playing on an Android phone

The Android version works like the iOS shell: the release page has an **APK that contains no game content**, you make an **import pack** on a computer from your own game files, copy it to the phone, and the App imports it when you open it. No adb, no Android Studio, and no need to build the APK yourself.

中文：[ANDROID.zh-CN.md](ANDROID.zh-CN.md)

**Where it stands:**

| Step | Status |
|---|---|
| Making the import pack on a computer | Works on a Mac: about 5 minutes from scratch (on an Apple silicon Mac; translating the shaders is more than half of it), an import pack of about 670 MB. Not yet tried on Windows / Linux; please open an Issue and tell us how it goes |
| Shaders in the import pack | The 6,927 game shader bundles are byte-for-byte identical to the set the port's authors tested on their phones |
| Installing the APK, importing, playing | Works on a vivo X200 Ultra (Android 16): permission, import, character creation, playing, panels, NPC dialogs |
| Frame rate | Measured by the port's authors: median 60 fps in the camp and in fights on a Redmi K50 Ultra (Snapdragon 8+ Gen 1); playable on a OnePlus 13 |

## Phone requirements

- ARM64, Android 12 (API 31) or later, a GPU with Vulkan 1.1
- About 7 GB free: about 5.6 GB of game resources plus about 0.7 GB of imported content (after the import you can delete the import pack on the phone and get 0.7 GB back)

## What you need

- The files of the game you bought: `ExeFS` and `RomFS` of the Switch version 1.0.35 (PC 3.4.95035); the upstream project describes how to prepare them from your own copy
- The source of this repository **from the same release as the APK** (the App checks and tells you if they differ)
- Python 3.9 or later with these packages: `python3 -m pip install pillow numpy lz4 capstone`
- A C/C++ compiler: clang (gcc works for the enemy picture library too; `CC` / `CXX` name another). The tool compiles two small native libraries with it: the texture deswizzler and the enemy picture library (once, cached in `DM_ROOT`)
- The .NET SDK (to build the shader translation tool): <https://dotnet.microsoft.com/download>
- `spirv-opt`, `spirv-val` and `glslc` from the Vulkan SDK: the [LunarG Vulkan SDK](https://vulkan.lunarg.com/) on Windows / Linux, `brew install spirv-tools shaderc` on macOS. `spirv-opt` must be new enough to support `--trim-capabilities` (the tool checks); the copy that ships with the Android NDK is too old, so do not let it come first on `PATH`. `glslc` from the Android NDK works too
- About 2 GB of disk space: about 0.9 GB of caches and intermediate products (in `DM_ROOT`, default `~/dm-work`) and about 0.7 GB for the import pack

The tool lists anything missing at the start and says where to get it.

## Step 1: make the import pack on a computer

From the repository root:

```
python3 dm/tools/d2rm_android.py <game files directory> <output directory>
```

`<game files directory>` is the `Update` directory of the Switch version 3.4.95035 (with `ExeFS` and `RomFS` in it). The tool rewrites the game program for Android, extracts and translates every shader, generates the touch overlay and a few data tables, and writes `<output directory>/android/`:

```
android/out/          the rewritten game program and its tables
android/shaders/      Vulkan shader bundles (6,931)
android/overlay/      touch overlay and UI art
android/overlay_variants/  the overlay used while "item notes and old names" is on (Custom page, after a restart)
android/derived.tsv   a few data tables generated from the game files
android/pack_info.txt which release this import pack is for
```

Steps already done are skipped on the next run. This folder contains game content; keep it to yourself and do not distribute it.

**Optional: a warm-up seed.** The first time the game reaches a place, the phone builds the GPU pipelines that place needs, and the first fight stutters while it does. A seed (`vk-pipeline-list.seed`, a list of the pipelines other phones have already seen) lets the App build them during start-up instead. If you have one, add it when making the pack:

```
python3 dm/tools/d2rm_android.py --warm-seed <seed file> <game files directory> <output directory>
```

Keep the seed file outside the repository. It is only read on the App's first launch, so it helps a fresh install; a phone that has already played keeps the list it records itself, even after a new import.

## Step 2: copy the files to the phone

Connect the phone by USB (choose "File transfer" on the phone) and, in the phone's `Download`, create `S2X/D2R Mobile/` holding two things:

```
Download/S2X/D2R Mobile/android/   the android folder from step 1, copied whole
Download/S2X/D2R Mobile/data/      game resources: the 18 .data archives and .build.config from your game files' RomFS/data
```

The 18 archives are listed in the upstream project's game-file guide in the section The "game resource folder" that goes on the phone; do not forget `.build.config` as well (a hidden file starting with a dot that computers often do not show; `build.config` without the dot is accepted too). Copy any `.data.holes` files along with them if you have them; they are optional.

## Step 3: install the APK and open it

1. Download `D2RMobile-android.apk` from the [release page](https://github.com/jhaizhou-ops/d2r-mobile/releases), copy it to the phone and tap it in the file manager to install. The phone may first ask you to allow "install unknown apps" or show a security check; confirm as it asks.
2. Open the App. It needs "All files access" to read the game resources in `Download`: tap the screen to open the system setting, turn it on and come back.
3. The App finds the `android` import pack and imports it into its internal storage with a progress screen (a minute or two the first time; stay on that screen), then starts the game.

After the import you can delete the `android` folder on the phone; keep the `data` folder, the game reads its resources from there.

## Moving to a new release

Each new release needs both: **download the new APK**, and **redo step 1 with the source of the same release**, copying the new `android` folder over the old one. The new APK installs over the old one and keeps your saves; when opened, the App finds the new import pack and imports it again. The `data` folder stays as it is.

If the APK and the import pack are from different releases, the App says "This import pack does not match this version of the app"; redo the import pack as it says.

## Saves and uninstalling

Saves live in the App's internal storage. **Uninstalling the App deletes the saves with it.** In these cases you have to uninstall before you can install our APK, so find a way to back up your saves first:

- the phone has an earlier build signed by the port's authors (same package name, different signature)
- you built the APK yourself from source (signed with the key on your computer)

The other way round holds too: as long as you upgrade with our released APK, your saves stay.

### Backing up and moving saves

Both go through the game folder `Download/S2X/D2R Mobile/` and run while the App starts, before the game loads:

- **Back up:** in the game, open Options › Extras › Tech › Open and press **Export** twice. The App saves, restarts and copies your saves to `save-export/<date-time>/` in the game folder. Copy that folder somewhere safe. Without the button: put an empty file named `export` in the game folder, close the App fully (swipe it away from recent apps) and open it again.
- **Restore or move from another phone:** copy a `save-export/<date-time>/` folder (or just the saves in it) to a folder named `save-import` in the game folder. Then press **Import** twice on the Tech page, or close the App fully and open it again.

What an import does:

- Characters are **added**; nothing on the phone is replaced. A character with the same name as one already on the phone is skipped, and the game settings are only taken when the phone has none yet. A folder with only the game settings (`Settings/`) in it counts as a backup too.
- An imported character stays an Elysium character exactly when it was one in the backup; an Elysium mark left on the phone by an earlier, deleted character of the same name goes.
- The shared stash and the Chronicle vault, softcore and hardcore each on their own, are copied in when the phone does not have them. Otherwise they are **merged** in game: the next time a character of that kind opens the stash, the backup's items the phone does not have go into your inventory (drag them into the stash or the Chronicle yourself; if the inventory fills up, open the stash again later for the rest), and the backup's gold is added to the shared stash.
- Importing the same backup again adds nothing: every stash and vault that was copied or merged is remembered, so no gold or item comes twice. A backup made on this same phone is never merged back into its stash.
- A merge that was still unfinished when the backup was made comes along and finishes on the new phone.
- The App's own settings, the followed build guides and the stash backups come along only where the phone has none.
- Before anything changes, the import is checked: file sizes (at most 16 MB per file), the number of files and damaged saves. If a check fails, nothing changes and the folder is renamed to `save-import.rejected-<date-time>`. Otherwise it is renamed to `save-import.done-<date-time>` so it is not imported again.
- If the App is closed in the middle of an import, the next start finishes it.

A notice in the main menu says what was exported, imported, skipped or merged. To replace a character you already have with another copy, delete the one on the phone in the game first, then import.

## When something goes wrong

| Message | What to do |
|---|---|
| Needs "All files access" | Tap the screen, turn it on in Settings, come back to the App |
| Game resources were not found | The `data` folder is not at `Download/S2X/D2R Mobile/data`, or has no `.data` archives |
| The .build.config file is missing | Copy `.build.config` from your game files' `RomFS/data` into `data` too |
| Game content has not been imported yet | The `android` folder is not at `Download/S2X/D2R Mobile/android`, or was not copied completely |
| This import pack does not match this version of the app | Redo step 1 with the source of the same release as the APK |
| The import did not finish | Usually not enough free space; free about 1 GB and open the App again |

To build the APK yourself from source, or for the layout of the Android source, see [dm/android/README.md](../dm/android/README.md).
