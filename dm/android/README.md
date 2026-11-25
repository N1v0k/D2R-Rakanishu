# Android port

中文：[README.zh-CN.md](README.zh-CN.md)

This is the ARM64 Android version. The APK links the shared host, the Vulkan
replay engine, AAudio, a transparent touch overlay drawn the same way as iOS, and
system text input. It has been played on real phones (camp, fights, panels, NPC
dialogs); measurements are in `docs/android-notes/`. Players
install the release APK and import their own game content: see "Installing as a
player" below.

## Source layout

- `app/`: Java Activity, fullscreen SurfaceView, touch overlay, and Gradle app.
- `jni_android.c`, `front_android.c`: startup, resource locations, lifecycle, and
  platform front-end interfaces.
- `touch_android*`: bridge to the shared `dm/ios/Sources/dm_touch_controls.c` state
  machine; `text_input_jni.c` bridges synchronous guest prompts to Android dialogs.
- `vulkan_present.*`: Android window ownership, swapchain, and presentation.
- `../gfx/vulkan/`: NVN replay adapter and Vulkan engine.
- `compat/`: Android implementations of the host's Apple platform interfaces.
- `tools/`: source-list parsing, ELF assembly conversion, and localized resources.

## Installing as a player

The release page's `D2RMobile-android.apk` contains no game content (built by
`dm/android/tools/shell_apk.sh`: the three tables generated from game files are
replaced by stand-ins that read the imported `derived.tsv` at run time, as in the
iOS shell). You make the game content on a computer from your own game files:

```sh
python3 dm/tools/d2rm_android.py <Update directory of the Switch version 3.3.94012> <output directory>
```

It rewrites the game program for Android, translates every shader into a Vulkan
bundle, generates the touch overlay and a few data tables, and writes
`<output directory>/android/`. The tools it needs are listed at the top of the
script (Python packages, the .NET SDK, and `spirv-opt` / `spirv-val` / `glslc`
from the Vulkan SDK); it reports anything missing.

Copy `android/` to `Download/S2X/D2R Mobile/r4k4n1shu/` on the phone.
Copy the full `RomFS/data/` directory to the shared `Download/S2X/D2R Mobile/data/`. Open the App: when it finds a new import pack it copies it into the
internal `files/dm/` (with progress; the first time takes a minute or two), then
starts. The import pack must come from the same release as the App (the product
code in `pack_info.txt` must equal the one built into the App); otherwise the App
says so. After the import you can delete the `android` folder on the phone. The
import pack contains game content; keep it to yourself.

## Build locally

The current configuration uses JDK 17 or newer, Gradle 9.8.1, Android SDK 36,
NDK 29.0.14206865, CMake 3.31.6, and Python 3. ARM64 devices must support Android
12/API 31 or newer and Vulkan 1.1 with the required device features. These are build/manifest requirements, not a
claim that all matching devices have been tested. The first target is vivo X200
Ultra (Android 16/API 36, Adreno 830).

Vulkan compatibility is selected from actual format features, not the Android version. Startup logs `format_feature_flags2` and `depth16_as_d24`; D16 is widened only when D16 lacks linear filtering and X8_D24 has all required uses. A Vulkan 1.1 version number alone does not establish compatibility. vivo should log `1 / 0`; the Redmi K50 Ultra (Adreno 730, Vulkan 1.1) logs `0 / 1` and runs this path.

Prepare your own game files according to the root setup documentation. Generate
`dm_vault_tab.h`, `dm_hud_exp.h`, and `dm_bright_levels.inc` outside the repository
using the rules in `dm/host/Makefile`. Set `ANDROID_HOME` and `ANDROID_SDK_ROOT` to
the same SDK directory, and `JAVA_HOME` to the JDK. From the repository root:

```sh
gradle -p dm/android :app:assembleDebug :app:lintDebug \
  -PdmGenDir=/absolute/path/to/private/generated/headers \
  -PdmPython=/absolute/path/to/python
```

The APK is `dm/android/app/build/outputs/apk/debug/app-debug.apk`. Native linking
uses `--no-undefined`. `-PdmPrebuiltJni=...` is a packaging-only fallback; it is not
evidence that current native sources compile.

On Linux or WSL, `dm/android/docker/` runs the same build in a container with these
versions pinned, so no Windows-side Python, NDK or CMake is needed:

```sh
docker build -t dm-android-build:1 dm/android/docker
DM_GEN_DIR=/absolute/path/to/private/generated/headers dm/android/docker/build.sh
```

The script runs as your user and keeps the debug signing key in `~/.android-dm`; see
its header for the mounts.

## Private runtime files

The APK and Git repository contain no game program, shaders, game art, or saves.
The import pack provides them (see "Installing as a player"), and the App imports
it into the internal directories below.

| Location | Contents |
| --- | --- |
| App internal `files/dm/out/` | `main` and `sdk` rewriter products built with `DM_TARGET=android`, including patched modules, fixups, and gates |
| App internal `files/dm/shaders/` | External `.xgs` bundles matching the game and Vulkan backend |
| App internal `files/dm/overlay/` | Required CASC touch overlay, including `overlay.manifest`, generated from the matching source and the user's resources |
| App internal `files/dm/overlay/touchart/`, `files/dm/overlay/hud_v4_ui/` | Skill atlases and their silver silhouettes, map icon, circle frame, bottom bar art, and the skill-button socket (the repository's `dm/ios/Sources/ControlArt/dm_mobile_socket.png`), staged by `stage_touch_assets.py`. The touch controls draw them the same way as iOS (`dm/ios/Sources/DmTouchControlArt.h`, `DmTouchOverlay.m`); the joystick is drawn in code |
| App internal `files/dm/overlay_variants/` | Optional: a complete overlay per combination of the Custom switches that go through the overlay (now only "Item notes and old names"), each with its own `overlay.manifest`; at start the App layers the one matching the switches in front of `overlay/` |
| Shared `Download/S2X/D2R Mobile/data/` | User's resource archives and `.build.config`; requires Android's all-files-access setting. When this directory does not exist but the earlier `Download/data/` does, the App keeps using `Download/data/` |
| App internal `files/dm/derived.tsv`, `files/dm/pack_info.txt` | Data tables generated from game files (read at startup by the release APK without game content); the import pack's product code and creation time |
| App internal `files/save/` | Saves; preserve during upgrades |
| App internal `files/temp/`, `files/dm/boot.log` | Temporary files and appended startup logs |

The launcher uses external app-private `dm/` as a fallback if no internal main
module exists. It checks required paths before startup; this check does not prove
shader identity or resource compatibility. Use the debug application's `run-as`
access to provision internal files, and overlay-install with the same signing
key. Never uninstall or clear data as an upgrade step. Back up existing saves
before device validation. Do not upload APKs or any of these private files to a PR.

The overlay generator `dm/ios/tools/gen_romfs_overlay.py` supports
`DM_DESWIZZLE_LIB` for a locally compiled native texture helper. Its thin CASC
output contains only changes and references the original archives; it must not
replace the original resource directory.

### Shader bundles

`files/dm/shaders/` holds one `<key>.xgs` for each shader the game uses; the
key is the first 16 hexadecimal digits of the SHA-256 of the shader's control
section. A bundle is an XGS1 file: a header, the bindings, the vertex input
locations, the c1 bytes, then the SPIR-V (`xbg_shader_header` in
`dm/gfx/vulkan/engine/xbg_game.h`). The set tested so far has 6,931 bundles:

- **6,927 are translations of the game's own shaders.** They are derived from
  game files, so they stay out of Git, PRs and the APK. `dm/gfx/tools/dmsh_vk.py`
  makes them from start to finish: `dm/gfx/tools/dmsh_xlate` translates each
  shader to SPIR-V for Vulkan, the bindings are renumbered for the single
  descriptor set (original number × 6 + stage number), the capabilities for
  reading and writing storage images of unknown format are added, unused
  capabilities are trimmed, `<key>.vk.json` is exported, and the packing function
  of `pack_game_shaders.py` writes the bundle. The 6,927 bundles it makes are
  byte-for-byte identical to the tested ones made with biglawine's HarmonyOS
  shader pipeline (`dmsh_vk.py --compare <reference dir> <output dir>`).
- **4 are the query kernels embedded in the main program** (`q_val0`,
  `q_val8`, `q_dlt0`, `q_dlt8`; Mac and iOS use the MSL versions in
  `dm/gfx/nvn/nvn_builtin.h`). They are hand-written, so they can be built from
  source: `python3 dm/gfx/tools/pack_builtin_queries.py --output <new
  directory>` compiles `dm/gfx/vulkan/kernels/builtin_queries.comp` with
  `glslc` (the NDK has it under `shader-tools/`) and writes the 4 files.

A bundle set can also carry `vk-pipeline-list.seed`. On a first launch, before
`files/dm/vk-pipeline-list.bin` exists, the engine builds the pipelines the seed
lists during start-up, so a first visit to the places it covers does not
stutter while pipelines build. `dm/android/tools/merge_warm_lists.py` merges
several devices' `files/dm/vk-pipeline-list.bin` into one seed. A seed only
works with the bundles it was recorded with, so it ships with them:
`dm/tools/d2rm_android.py --warm-seed <file>` puts one into the import pack,
checks it before the slow steps, and records it in `pack_info.txt` as
`WARM_SEED=<sha256 prefix>`. Keep seeds outside the repository. The seed only
helps a first launch: a phone that already has `files/dm/vk-pipeline-list.bin`
keeps using its own list after a re-import. The tool's check runs with
`make -C dm/host/tests run` (or `python3 dm/tools/tests/test_warm_seed.py`).
The measurements are in `docs/android-notes/2026-10-22-first-play-stutter.md`.

## Validation and remaining work

Run the root contribution checks, plus `:app:lintDebug`. The pure Java input
checks can run without a device:

```sh
javac -encoding UTF-8 -d /tmp/dm-input-tests \
  dm/android/app/src/main/java/com/xiaoboluo/d2r/TextInputRules.java \
  dm/android/tests/TextInputRulesTest.java
java -cp /tmp/dm-input-tests com.xiaoboluo.d2r.TextInputRulesTest
```

Device acceptance must separately record installation, genuine GPU output/FPS,
character creation, entering a world, moving and stopping, NPC interaction,
inventory/skills, sound, background/resume, and saving/reloading. Headless host
FPS is not a rendering result. Metal GPU timing and scanout timestamps are not
implemented by this backend; unavailable metrics must not be treated as zero cost.

The Vulkan bridge currently replays all draws and uses synchronous host
translation (`DM_NVN_SYNC=1`); the upstream asynchronous path needs an independent
object-lifetime audit. The presenter still waits for its queue each frame.
Advanced iOS skill artwork, empty-slot skill menus, specialized quantity/stat
popovers, and shop drag-follow visuals are incomplete. Generic controls and text
input do not establish complete iOS parity. macOS/iOS regressions must be run on
an Apple build host before upstream integration.

The overlay generator writes a source fingerprint and exact file hashes to `overlay.manifest`. The APK embeds the same generator-input fingerprint. Missing, incomplete, or mismatched overlays stop startup with a localized message before the host thread starts. Deploy the complete generated directory; do not copy a manifest onto old resources or generate one on the phone. Regenerate after generator/layout changes. This contract does not authenticate third-party resource downloads.

`gen_romfs_overlay.py` requests overlay-only assets: it skips the iOS skill atlas and its unused skill silhouettes. Full `gen_assets.ensure()` still requires them. Skill atlas validation compares dimensions and decoded RGBA pixels while retaining source sprite SHA256/MD5 checks. Generator subprocesses explicitly use UTF-8/LF and preserve an existing read-only resource adapter. Windows/Linux byte equality must still be verified with the same game input and tool versions.

Vulkan compatibility is selected from actual format features, not the Android version. Startup logs `format_feature_flags2` and `depth16_as_d24`; D16 is widened only when D16 lacks linear filtering and X8_D24 has all required uses. A Vulkan 1.1 version number alone does not establish compatibility. vivo should retain `1 / 0`; Redmi K50 Ultra needs separate device validation.
# Current iOS touch controls

Android shares the upstream touch state machine and its read-only skill-binding observer. Generate the binding index and atlas PNGs from your own matching RomFS outside the checkout:

```
python dm/ios/tools/native_art/generate.py <RomFS> --output-dir <external-native-art>
```

Copy only `dm_touch_native_art.c` into the existing `DM_GEN_DIR`. CMake requires that external index alongside the existing generated host headers. After generating a fresh overlay and HUD, run:

```
python dm/android/tools/stage_touch_assets.py --native-art <external-native-art>/TouchArt --hud <assets>/NativeHUD/v4/hud_v4_ui --overlay <candidate-overlay>
```

Import the complete finalized candidate overlay. Skill PNGs and HUD images stay in the external resource payload; do not add them to Git or APK assets. The renderer reads them from the validated imported `files/dm/overlay` directory. `DM_NATIVE_ART_DIR=<external-native-art>` lets the native-art capture test verify the external index and pixels. The five navigation buttons remain enabled until the complete replacement inventory flow passes device validation.

## Test hooks and log rotation

The debuggable qa APK (the default `shell_apk.sh` build, CMake Debug) carries a few hooks for external test tools. They only record state; nothing reads commands from outside. The player variant and a release build contain none of them: `test_hooks.c` is compiled only with `DM_ANDROID_TEST_HOOKS` (qa Debug builds), and the host keeps only hook slots that stay NULL.

- **Overlay lines** (on by default in debug builds): each state change of a Java overlay writes one line to `boot.log`, `[qa] overlay <name> <state>`, and, once the first frame in that state has been submitted to the compositor (frame commit callback), `[qa] drawn <name> <state>` (on screen one vsync later). Names and states: `app_menu` (the Quick Menu) closed / open / paused / hint, `tech_page` closed / open, `recipe_book` closed / loading / open / inserting, `pad_layout` touch / pad; the build guide page reports `build_book` the same way (`TestHooks.overlay(view, name, state)`). `pad_layout` has no drawn line of its own: the game rebuilds its HUD asynchronously, and the existing line `手柄 / 触屏布局：切到…版——完成` marks the moment it is done.
- **State file** (off by default; `DM_QA_STATE=1` in `files/dm/env`): `files/dm/state.json` in internal storage (mode 0600, replaced atomically). A low-priority thread writes it at most twice a second, only when the content changed, plus a heartbeat every 5 s. Fields: `session` (pid, start), `seq`, `mono_ms` (CLOCK_MONOTONIC, compare with `/proc/uptime`), `wall_ms`, `frame`, `screen` (boot / frontend / loading / ingame), `paused`, `hud` (area, act, position, level, life, mana, belt, …, the same numbers as the `dm_hud：` lines), `panels` (the non-resident panel stack, as the `[hud] 面板栈` lines), `tooltip` (`null` or the texts of the description boxes on screen, colour codes kept), `overlays` (the states above) and `age_ms` (how old the HUD, tooltip and overlay parts were when written; a HUD that keeps ageing means the ticks stand, e.g. paused).
- `DM_QA=0` turns overlay lines and the state file off.
- **Script errors** (`DM_KEYS`, `DM_TEST_CURSOR`): with `DM_KEYS_SOFT=1` (the default on Android) a script that does not parse is dropped as a whole (`【红】hid 脚本作废：…`), and a supervision timeout while it runs aborts the whole script and releases what it holds (`【红】hid 脚本中止：…`); the game goes on. Numbers and lengths are checked strictly in this mode. `DM_KEYS_SOFT=0` (the default on Mac / iOS) ends the process as before.

`boot.log` (every build): at start a file of 8 MiB or more becomes `boot.log.1` (one generation is kept); a file over 32 MiB is renamed and only its last 8 MiB are copied to `boot.log.1` by a background thread; within a session the file is replaced at 32 MiB. The log is created with mode 0600. Readers that follow the log across an app start must expect the file to be replaced (a follower that keeps the old file open must switch to the new one). The graphics engine's `NVN PERF` lines are sampled: the `NVN PERF v=1` summary (fps, submit CPU, GPU) stays in every 60-frame window, whole windows of detail lines only for the first three and then one every 30 s, each followed by `NVN PERF sampling thinned=… dropped=…`; lines that only appear when something happened (uploads, shader reuse, evguard, new pipelines) and unknown keys are always kept. `DM_LOG_PERF_EVERY=<seconds>` changes the interval, `0` keeps every line. The pure logic is tested by `dm/android/tests/test_log_policy.c` and `test_qa_state.c` (host `cc`); the script handling by `dm/host/tests/t_hid_script.c`.

## Quick Menu and Extras

The first belt cell (pentagram), holding the controller's menu button for 0.5 s and the shoulder action "Quick Menu" open the **Quick Menu**: one column in the look of the game's controller pause menu with Pause, Recipes (only while the recipe book can be used), Builds, Players ◀ n ▶ and Close. The player count stops at 1 and 8, every step is requested from the host at once and lands on the first tick after resuming; the menu always shows the requested value. Everything it draws comes from the player's game files through the import pack (`appart/quick/`, `appart/ui_colors.txt`, `dm/android/tools/gen_app_art.py`); the App decodes all of it before the game starts (`AppArt.verify`) and refuses a pack without the mark `CAPS=quickmenu1` in `pack_info.txt` like a pack of another version.

Rarely changed settings are not in the Quick Menu: frame rate and status line are on the options page **Video**; the shoulder triggers (`sw_custom.txt` keys `shoulder_l` / `shoulder_r`, actions 0–14, 13 = reserved for the recipe book) and the **Tech** row with its "Open" button are at the end of **Extras**. The shoulder section only shows on phones with triggers (first F7 / F8, or nubia / REDMAGIC as a guess). The old Java preferences are migrated once; they stay for a downgrade. The first opening after an update shows a one-time hint saying where things moved (`shared_prefs/dm_quick.xml`, `hint` 1 = due, 2 = done).

## Save export and import

`SaveTransfer.java` runs once per process before the host first starts (never on a re-attach to a running host, never while the game runs), on a thread of its own; a notice panel in the game's style shows the outcome in the main menu (`TransferNotice.java`). Triggers: the file or folder `export` and the folder `save-import` in `Download/S2X/D2R Mobile/r4k4n1shu/`, and the Export / Import buttons of the tech page, which save, end the process and open the game again through `Restart` (its own process, `:restart`).

- Export: `files/save` to `save-export/<stamp>/` through a `.part` folder, plus `export-source.txt` (the install id in `files/save-transfer-id`) and `_app/` (`sw_custom.txt`, `stash_backup/`, the followed build guides as `builds.properties`, the ledger).
- Import: characters and Settings only where the phone has none; shared stash and Chronicle vault per mode are components of their own, copied whole when the phone lacks them (a vault only on a clean restore: no stash of that name, no vault, no character with `.swpage` / `.swpouch`), otherwise packed into one inbox entry `_cloudinbox/<millis>-import/mine.pack` that `dm_cloudmerge.c` merges in game like an iCloud fork. Open inbox entries of the backup come along, finished ones do not; a backup with this install's id is never merged.
- Idempotence: `files/save-transfer/ledger` records every copied or queued component (`stash`, `vault`, `inbox` and a SHA-256); a component in the ledger is never copied or merged again. The ledger is exported and merged on import.
- Crash safety: everything is staged in `files/save-transfer/stage`, then `files/save-transfer/journal` is written atomically and replayed step by step (moves, the elysium marks of imported characters, the ledger, the build guides, the folder rename) until done, also at the next start; a stage without a journal is dropped. An unfinished export is removed through `files/save-transfer/export`.
- Checks before anything changes: at most 4096 files, 16 MiB per file and 64 files per pack (the limits of the native pack reader), valid `.d2s`, `.d2i` and `.swvault`.

Tests: `dm/android/tests/SaveTransferTest.java` (in `run.sh`; a crash at every publish boundary through the system property `dm.transfer.kill`) and `dm/host/tests/t_cloudmerge.c` (an `-import` entry with a vault, gold once, recipe lock, kill points `cm:1` / `cm:2` / `cm:4`, more than 64 inbox entries).

## 3D scale and frame rate

The player sets both on the options page, tab **Video**, with the game's own sliders (stored in `sw_custom.txt`, keys `s3d`, `fps`, `res_pct`; `dm/host/dm_custom.c`, sizes and notches in `dm/host/dm_res_scale.h`). The 3D sharpness (75-100 %) applies immediately, the frame rate too (5-fps notches from 30 up to below the panel's highest refresh rate, then Max; the app asks the display for the smallest supported refresh rate the frame rate divides evenly, `Frame rate vote` in `vlog`, `display: refresh` in `boot.log`); the output resolution (a percentage of the physical panel in 5 % notches, at least 720 lines; the text beside the slider shows the final size) switches on Apply when it is no larger than the size this start uses (the app pushes the performance-mode notification, the game changes its output area inside the window it built at start, the presenter follows with a new swapchain; `res live:` lines in `boot.log`, checked against the presented swapchain within 120 frames and reverted otherwise; `DM_RES_LIVE=0` in `files/dm/env` turns this off), and after an app restart when it is larger. Defaults are a 3D scale of 0.85 on Android (0.75 on the other phone platforms), dynamic scaling enabled, floor 0.60, frame rate by screen (72 on screens above 130 Hz, else 60), 900 output lines. Markers for measurements: `gfx-frame-rate-policy=highest` (vote the largest evenly divided refresh rate instead), `gfx-frame-rate-strategy=always` (allow non-seamless refresh switches).

For measurements, `DM_3D_SCALE`, `DM_3D_SCALE_DYN` and `DM_3D_SCALE_FLOOR` in the game's `files/dm/env` (qa variant only) still work and pin the value (the Video row then only shows it); `DM_GAME_RES=<w>x<h>` pins the output resolution the same way. A fixed-scale A/B comparison:

```
DM_3D_SCALE=0.75
DM_3D_SCALE_DYN=0
DM_3D_SCALE_FLOOR=0.60
```

## Screens narrower than 16:9 (true fullscreen)

On a screen narrower than 16:9 by more than 0.01 (4:3 tablets, foldable inner panels) the picture in a run follows the screen aspect and the 3D fills it; the game lays its UI out at 16:9 anchored to the bottom (`dm/host/dm_uiscreen.c`, the graphics layer shifts the UI pass), and the front-end pages and the loading screen are a 16:9 picture centered with bars. Screens 16:9 and wider are unchanged. It is decided at start from the screen shape (`front_android.c`; the `boot:` output resolution line in `boot.log` then ends with 真全屏) and follows the window when it changes shape at run time (folding / unfolding).

The output size follows the Video page as everywhere else, with the whole screen as the percentage base (100 % = the panel) and the width following the screen aspect. The output never gets narrower than 1280 pixels, so the 16:9 UI keeps at least 720 lines and the game never has to raise it (4:3: at least 960 lines; auto = the larger of 900 lines and that minimum; the slider's lowest notch respects it). The device default and the 3D sharpness work as on wide screens. When the window grows past the size the game built its window texture for at start (unfolding), the picture is scaled into that size at once and gets sharper after a restart.

Fullscreen starts count in the crash guard whatever their height: two fullscreen starts in a row that never presented 300 frames turn fullscreen off for this product code (`res_full_off` next to `sw_custom.txt`, a build with a new product code tries again), and the next start shows 16:9 with bars. `DM_ANDROID_FULL=0` switches it off for measurements (`files/dm/env`, qa); players can do the same with an empty file named `fullscreen-off` in the game folder (`Download/S2X/D2R Mobile`).

Our own layers follow `front_android_view_rect` through `GameRect.java`: the touch controls and the bottom bar, the potion buttons and the camera buttons sit in the 16:9 block; the status block and its effects sit at the top of the picture, under the portraits the game lifts there; our pages (recipe book, build guides, glossary, tech page, Quick Menu and pause screen, transfer notice) take the 16:9 block centered in the picture, where the game centres its own menus; damage numbers, sort buttons, the camera arrows and the aim ring use the whole picture. Touches above the block still reach the world.

## Build variants

`shell_apk.sh` (and Gradle directly) builds one of two variants, chosen with `-PdmVariant=qa|player` (default `qa`):

| | qa | player |
|---|---|---|
| For | test phones and emulators | the player's own phone |
| Application id, signing key, versionCode | as given | the same, so it installs over a qa build and back |
| Debuggable (`run-as` save backups) | debug builds only | no |
| versionName | as given | as given plus `-player` |
| `files/dm/env`, every `DM_TEST_*`, `DM_KEYS`, `DM_QA*` | read | ignored |
| Test hooks (`[qa]` lines, `state.json`), recipe / effect test switches | yes | compiled out |
| Engine marker files `files/dm/gfx-*`, `files/dm/spv-override/` | read | ignored, except the pipeline crash guard's `gfx-pipe-inflight.<n>`, `gfx-pipe-suspect`, `gfx-pipe-skip` |
| `am start … --ez dmArtDump true` | yes | compiled out |
| boot.log rotation, options pages (Video, Extras with Tech and shoulder triggers) | yes | yes |

Both variants pair with the same import pack: the pack is matched by `dm/PRODUCT` and the overlay fingerprint, neither of which depends on the variant. Build both from one commit.

How player is made:

- **Gradle / CMake**: `-PdmVariant` becomes `BuildConfig.DM_VARIANT` / `DM_QA_BUILD`, the CMake cache variable `DM_VARIANT` and the Java source directory `app/src/<variant>/java` (the real `TestHooks` in `qa`, an empty class with the same methods in `player`). Test code is compiled only for qa Debug (`DM_ANDROID_TEST_HOOKS`, `DM_RECIPE_TESTS`, `DM_EFFECT_TESTS`); player defines `DM_PLAYER_BUILD` (public on `dmcore`, private on the engine). iOS, Mac, HarmonyOS and the host unit tests never define it.
- **No env file**: player never reads `files/dm/env`; when the file is there, `boot.log` says `boot: variant player ignores files/dm/env (…)` and the file is left alone.
- **getenv list**: `compat/include/dm_player_env.h`, force-included into every C file of the player build, lets through names without the `DM_` prefix and only the `DM_*` variables the App sets itself before `dm_main` (`jni_android.c`, `cpu_translation.h`, `DmActivity.java`; the three 3D scale keys of the former `android-debug.env` are gone with the Video page); every other `DM_*` reads as not set, which is how a player's phone has always run. For a literal name the comparison folds at compile time and the name disappears from the library. `tools/check_player_env.py` checks the list against the source on every `shell_apk.sh` run.
- **Remaining test paths**: the switches whose names would survive in log text (`diag.c`, `dm_actions.c`, `hle_hid.c` script input and others) are inside `#ifndef DM_PLAYER_BUILD`.
- **Marker files**: player links `libdm.so` with `-Wl,--wrap=fopen`; `player_guard.c` makes `<runtime>/gfx-*` and `<runtime>/spv-override/` behave as missing and logs `boot: variant player ignores marker <name>` once per name. Every `gfx-*` name in the library must be classified in `tools/player_markers.tsv`.
- Every boot logs `boot: variant qa` or `boot: variant player` right after `boot: 内部目录`.

`tools/check_variant.sh <apk> <qa|player>` checks a finished APK (shell_apk.sh runs it on every build): expected debug state, signing certificate, versionName suffix, the positive marks of each variant, and for player that `libdm.so` contains none of `DM_*TEST*`, `DM_KEYS`, `DM_UI_TAPS`, `DM_QA`, `[qa]`, `dm-qa`, `state.json`, `test hooks`, `boot: 环境`, that no `classes*.dex` contains `dmArtDump`, `nativeOverlay` or `nativeEnabled`, and that every `gfx-*` name is classified. Unit tests: `tests/test_player_env.c`, `tests/test_player_guard.c` (host `cc`), `tests/test_check_player_env.py`.

## arm64 translators and the x86_64 emulator

The APK runs unchanged on an x86_64 Android emulator, whose arm64 translator (libndk_translation / Berberis in the API 35 `google_apis` image) runs `libdm.so`. Two things make that work:

- **No LSE atomics on Android.** The NVN counting thunks (`dm/gfx/nvn/nvn_thunks.S`, generated by `dm/gfx/tools/gen_protos.py`), the `DM_NVN_PROF` wrapper (`nvn_entry.S`) and the `DM_HLECOUNT` stubs (`dm/host/hle_count.c`) count with an ARMv8.0 `ldxr`/`stxr` loop on Android (`DM_NVN_COUNT_LLSC=1`, set by `CMakeLists.txt` for the assembly). The translator executes ARMv8.1 LSE atomics such as `stadd` wrongly, and ARMv8.0 phones do not have them at all. Apple and HarmonyOS keep `stadd`.
- **Write-protect tracking off under a translator.** At start the App reads `ro.product.cpu.abilist` (else `ro.product.cpu.abi`) once. When the primary ABI is `x86` or `x86_64`, this arm64-only library can only be running through a native bridge; the emulator's translator cannot handle the write-protect faults, so `DM_GFX_VCACHE`, `DM_GFX_TEX_DIRTY` and `DM_GFX_WTRACK` default to 0. Values in `files/dm/env` still apply, and the engine's marker files (`gfx-vcache`, `gfx-tex-dirty`, `gfx-wtrack`, `gfx-no-wtrack`) still win over the environment. `boot.log` records the decision: `boot: cpu translated (…)`; on phones `boot: cpu native (…)`, and for any other primary ABI `boot: cpu unknown (no change)`; neither changes anything. The diagnostic marker `gfx-event-guard` faults on purpose and still fails under a translator. The regressions are `dm/android/tests/test_cpu_translation.c` (decision and env/marker precedence) and `dm/gfx/vulkan/engine/tests/wtrack_switch_test.cpp`; build and run them with any C / C++ compiler. `dm/android/tests/test_hlecount_stub.c` checks the Android `DM_HLECOUNT` stub on a device or the emulator.

`DM_NVN_PROF` cannot be switched on through `files/dm/env` on Android: the timing wrapper is installed by a library constructor (`nvn_nops_register` in `dm/gfx/nvn/hle_nvn.c`), which runs when the library loads, before `nativeStart` reads the env file. The wrapper itself is LL/SC-safe.

## Signing and transfer storage

See the [root build instructions](../../README.md#build-the-apk) for external signing properties and environment variables. QA and player use the same configured key. Without release credentials, QA debug builds use an existing debug key with a visible warning from the build script. Release builds stop unless `-PdmAllowDebugSigning=true` or `DM_ALLOW_DEBUG_SIGNING=1` explicitly permits a local test build. The former `~/.config/dm-android/release.env` file is no longer read. Player is never debuggable, including a direct Gradle debug build.

Pack and transfer files live in `Download/S2X/D2R Mobile/r4k4n1shu/`. The shared `data/` resource directory stays one level above. Transfer markers in the shared parent belong to upstream installations and are not consumed here. A matching legacy `android/` pack can be moved into our folder or read in place if the move fails.

## Own graphics drivers

`libdm.so` no longer links `libvulkan.so`. `tools/gen_vk_shim.py` writes, at build time from the NDK's `vulkan_core.h` / `vulkan_android.h`, one forwarder per `vk*` function (`vk_shim.c` in the build directory); `vk_loader.c` resolves each through `vkGetInstanceProcAddr` of the chosen loader on first use (global commands with a `NULL` instance, the rest with the instance; again after a loader switch or a new instance). Without an own driver the first call opens the system `libvulkan.so`, as before. The engine (`dm/gfx`) is unchanged; `CMakeLists.txt` drops `vulkan` from the engine's link libraries the way the HarmonyOS build drops `log`. Host test: `tests/test_vk_loader.sh` (two fake loaders, 8 threads, loader switch, missing function).

Stage 2 picks drivers inside the App (stage 1's external folder is unreachable on recent Android - other apps cannot write there and even adb gets "Permission denied"). The tech page has a third button "Graphics drivers" (`DriverPage.java`, the tech page's panel, art, fonts and colours): a list whose first row is always the system driver, then one row per installed driver (name and version from the ZIP's `meta.json`, the file name when it cannot be read), marked `· active` (this process runs it), `· chosen` (the next start uses it) or in the game's red `· blocked` (the start guard). A tap or pad A chooses for the next start; the buttons below add (through the system file picker), delete, restart (the same save-and-restart path as the tech page's Export; enabled only while the choice differs from what runs) and close; the line under the list says what just happened or shows the guard's notice. At most eight drivers; rows past the visible ones scroll by drag and by pad. The chosen ZIP is copied into `files/drivers/incoming.tmp` (64 MiB at most, above that it is given up), checked the stage 1 way and only then moved into the library `files/drivers/lib/<sha256 of the ZIP>.zip`; the same ZIP is never added twice, and adding never changes the selection. The library, the selection, the deletion and the migration are pure `java.io` in `DriverLibrary.java`.

At the next start `DriverStore.java` (on a worker thread) imports the selected ZIP: it checks it without running anything (`DriverZip.java`: limits, paths, `meta.json`; `DriverElf.java`: AArch64 `ET_DYN`, bounds, page alignment, exported `HMI`), places the one `.so` read-only under `files/drivers/<sha256>/` and hashes it again before every load. The choice for the next start lives in `files/drivers/selected.txt` (the ZIP's sha, empty or missing = the system driver); it and the library are written atomically (temp, fsync, rename, directory fsync) so they survive a phone the driver hung. Devices without `/dev/kgsl-3d0` or with an x86 primary ABI are refused. libadrenotools (`dm/third_party/adrenotools`) lives in its own `libdm_driverload.so`, dlopened by `vk_driver.c` only for an own driver; its two hook libraries are copied out of the APK into `files/drivers/hooks/` when needed, so `useLegacyPackaging` stays off. Because adrenotools silently falls back to the system driver, `vk_driver.c` checks `/proc/self/maps` for the placed `.so` after `vkCreateInstance`.

The external folder `Android/data/<package>/files/drivers/` only serves old stage 1 setups (adb tests): while the library is still empty and the marker `files/drivers/lib-migrated` is absent, its ZIPs are copied into the library once - the folder is left as it is, and exactly one ZIP there is also selected; several ZIPs are all imported and none is selected. Optional `tu_debug.txt` there still holds one `TU_DEBUG` value from the list `sysmem`, `gmem`, `nolrz`, `noubwc`, `flushall`, `syncdraw` (comma separated).

Start guard (`DriverGuard.java`, state in `files/drivers/guard.txt`): a clean failure before `vkCreateDevice` switches back to the system loader in the same process (`files/drivers/attempt.fallback`); a start counts as finished after 300 presented frames (`attempt.ok`); two unfinished starts in a row block the package and the system driver is used again. Either way the tech page opens once with a red notice. The block keys on the ZIP's sha; deleting the entry on the driver page makes the guard forget it, and choosing another entry lifts the old block, so both start with a clean count. Safe start stays three ways - the guard's own fallback, the launcher shortcut "Start with the system driver" (shown while a driver is chosen, one start) and choosing the system driver on the driver page - and there is no confirmation dialog anywhere. The tech page shows the driver that really runs and the package. Identity of the running driver (device, API, driverID, name) is in `files/drivers/active.txt` and in `boot.log` (`vk driver: …`). qa debug builds only: `files/drivers/test-fail` = `open`, `verify` or `hooks` injects a failure. The strings are in `res/values*/driver.xml` (outside `ui_text.tsv`, so the import pack does not change). Unit tests: `DriverGuardTest`, `DriverZipTest`, `DriverElfTest`, `DriverLibraryTest`, `DriverTextTest` (in `tests/run.sh`); `tools/check_driver_libs.py` (run by `shell_apk.sh`) checks the packaging.
