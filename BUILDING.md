# Building this release from source

Everything here is what a player needs to reproduce the released APK byte for byte
from this source tree and their own game files. The versions below are the exact ones
the release was built and verified with; checksums are SHA-256 so you can confirm you
installed the same tools.

## Toolchain

| Tool | Version | Checksum (SHA-256) |
|---|---|---|
| JDK (Eclipse Temurin) | 17.0.21+9 | `java` binary: `a2195ae9f8a91b82f53df7e1d1b9a650937ebbabbf67d0240c6378b4fbd9990b` |
| Gradle | 9.8.1 | distribution zip: `dce76f55f8e251a3a1f130eb120f30b3d271de2b76c9b0729d316b5a1b6dc01f` |
| Android Gradle Plugin | 9.4.1 | main jar: `4021fa6ea76b6fac076dd0bdacbb3e1b76c31f33b6c392e1a49a2e42072196bf` |
| Android platform | android-36 rev 2 | `source.properties`: `72291ff6611dbfc46fdfb6de2821c4a62c87befa5200713847eda426e3a501c2` |
| Android build tools | 36.0.0 | `source.properties`: `57dee6632e9ad6cb111da2bb99d747211e27927061b1276d040bb1d71fded5ebb`, `aapt2`: `1a6a396b9cd071f7040071fdd108718cb98c3c9f4960044f373b288993d19eb7`, `zipalign`: `c5f559e946de5a9e7d58792181db20383b228877812136bc469d97ae00a43b0a`, `lib/apksigner.jar`: `3716d9311e55d2b0918a2fd9d54ba9e406c5f6abeea700b287f11259bc163dec` |
| NDK | 29.0.14206865 (clang 21.0.0, r563880c) | `source.properties`: `716f3518a923198cfab037abb32dc3f1b1f7e9a9dcdcda6b66be5906215d2658`, `clang-21`: `d326ed4fc4f71e5315b9ce388fc23ec792f81652ca7b29e66294de6ed2520844` |
| CMake | 3.31.6 | `cmake`: `eb597128ba5a1e3108fe207743a37e8828c0bea5a7658da4a044ad3fe4299af7` |
| Ninja | 1.12.1 | `ninja`: `f5d6ae74f53c3714734d0bde7f6ede82c926c84bec06120677bad0c9c331952a` |
| Python | 3.12 | the APK build needs the standard library only |
| .NET SDK | 10.0.403 (no roll forward) | the pack builder's shader translator; every NuGet package is pinned by SHA-512 in `dm/android/shaders/xlate-packages.txt` |
| Host tools of the Python runtime | python3 3.11 or later, CMake (3.28.3 verified), pkg-config, curl, xz | every download is pinned by SHA-256 in `dm/android/python/build_runtime.sh`; the wheel builds' tools by hash in `build-requirements.txt` and `build-wheel-requirements.txt` |

Gradle resolves the Android Gradle Plugin from the Google Maven repository by the
version pinned in `dm/android/settings.gradle`; the checksum above is of the resolved
`gradle-9.4.1.jar` (find it under `caches/modules-2/files-2.1/com.android.tools.build/gradle/9.4.1/`
of your `GRADLE_USER_HOME`).

## Prerequisites

- JDK 17 (`JAVA_HOME`), Android SDK with the platform, build tools, NDK and CMake
  versions above (`ANDROID_HOME`), and a Gradle 9.8.1 binary on `PATH`.
- A Python 3 interpreter for the generators (`DM_PYTHON`). Building the APK itself
  needs no Python packages (the pack builder's runtime brings its own, see below); building the import pack needs the Vulkan tools
  `spirv-opt`, `spirv-val`, `glslc`, the .NET SDK, a C/C++ compiler (clang or gcc,
  for the texture deswizzler and the enemy picture library) and the packages
  `pillow==12.1.1 numpy==2.4.4 lz4==4.4.5 capstone==5.0.9` from
  [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md).

Set the environment for one build:

```sh
export JAVA_HOME=/path/to/jdk17
export ANDROID_HOME=/path/to/android-sdk
export DM_PYTHON=/path/to/python3
export DM_ROOT=/path/to/build-root      # every build artifact goes under here
export DM_ROMFS_SRC=/path/to/your/3.4.95035/Update/RomFS
```

`shell_apk.sh` sizes its runtime tables from your own game files, then replaces them
with content-free stubs: the APK contains no game data, only table shapes.

## The pack builder in the APK

The published APK builds the game pack on the phone. It carries an embedded Python
runtime and a shader translator, made from pinned recipes at build time (they are not
stored in the repository):

- `dm/android/python/build_runtime.sh <folder>`: CPython 3.14.10 (the official
  android-arm64 build from python.org) with NumPy, Pillow and its libraries, lz4 and
  Capstone, cross-compiled with NDK r29. It needs network access for the pinned
  downloads (checked by SHA-256) and builds in `/tmp/dm-pyrt-<id>` (about 1.5 GB,
  removed afterwards; that folder's path length is the same on every machine, which
  keeps the output byte-identical).
- `dm/android/shaders/build_xlate.sh android <folder>`: the shader translator
  `libdmsh_xlate.so`, compiled ahead of time with .NET NativeAOT. It needs the .NET SDK
  above and network access for the first NuGet restore (packages checked by SHA-512).

`shell_apk.sh --builder <runtime folder> <translator folder>` runs both (an output whose
recipe is unchanged is reused) and puts them into the APK. Without `--builder` the
player APK imports packs made on a computer only and needs neither network nor .NET.
The FriBiDi library inside is LGPL: its exact source archive is attached to every release.

## Building the APK

Version metadata needs no arguments: the defaults in `dm/android/app/build.gradle`
are this release's published numbers (versionName `1.1.0`, versionCode `30004`;
the player variant appends `-player`), so the commands below build exactly the
published version metadata. `-PdmVersionName=<name> -PdmVersionCode=<n>` override
them for test builds only.

Unsigned (no keystore is read; this is what the reproducibility check compares):

```sh
dm/android/tools/shell_apk.sh release -PdmVariant=player -PdmUnsigned=true \
  --builder "$DM_ROOT/build/android_python" "$DM_ROOT/build/android_xlate" --public
# result: $DM_ROOT/build/android_shell/D2RMobile-android-player.apk
```

`--public` checks what the published APK must be: the complete pack builder, the default
shared folder and the release application ID.

Signed (for installing; needs a release key configured outside the repository):

```sh
dm/android/tools/shell_apk.sh release -PdmVariant=player \
  --builder "$DM_ROOT/build/android_python" "$DM_ROOT/build/android_xlate" --public
```

The signing key comes from `~/.config/d2r-release/signing.properties` or the
`DM_SIGNING_PROPERTIES` / `DM_ANDROID_KEYSTORE` environment variables; see
`dm/android/README.md`. The published APK is signed with RSA 4096, APK signature
scheme v2 only; its certificate SHA-256 is printed in the release notes.

## Building the import pack

The pack must come from the same source tree as the APK: its tools and the APK
fingerprint each other, a pack built from other sources is refused visibly.

```sh
python3 dm/tools/d2rm_android.py /path/to/your/Update /path/to/pack
# result: /path/to/pack/android/ — copy it as described in README.md
```

## Reproducing the released APK

1. Download the release's source archive and unpack it; do not add or edit files.
2. Build unsigned twice with the toolchain above, in two different directories:
   `DM_ROOT=/tmp/root1 ... shell_apk.sh release -PdmVariant=player -PdmUnsigned=true --builder /tmp/root1/build/android_python /tmp/root1/build/android_xlate --public`,
   then the same with `/tmp/root2`. Each build makes the Python runtime and the shader
   translator from their recipes; both outputs and both unsigned APKs must be byte-identical:
   ```sh
   sha256sum /tmp/root1/build/android_shell/D2RMobile-android-player.apk \
             /tmp/root2/build/android_shell/D2RMobile-android-player.apk
   ```
3. Compare against the published signed APK by transferring the signature
   (any apksigcopier-style tool works; the release notes carry the exact certificate
   fingerprint to verify with `apksigner verify --print-certs`).
4. Native libraries are reproducible per file as well; to compare entry by entry:
   ```sh
   unzip -Z1 app.apk | while read e; do echo "$e $(unzip -p app.apk "$e" | sha256sum | cut -d' ' -f1)"; done
   ```

Known inputs of the build that must match: game version 3.4.95035 (only used to size
the stub tables), the tool versions above, the pinned downloads of the two recipes, and a
clean source tree. Build directory paths and the machine user do not affect the result.
