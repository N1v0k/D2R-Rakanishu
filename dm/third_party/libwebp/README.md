# libwebp (third party, BSD 3-Clause, see COPYING and PATENTS)

- Upstream: https://chromium.googlesource.com/webm/libwebp, release 1.6.0, tarball `libwebp-1.6.0.tar.gz`
  (SHA-256 `e4ab7009bf0629fd11982d4c2aa83964cf244cffba7347ecd39019a9e38c4564`), the version the pillow builds the pack tools use carry
- Used for: the WebP files of the enemy pictures, written by the picture library `dm/android/tools/monpic` (files listed in its `sources.txt`)
- Included: the `.c` and `.h` files of `src/dec`, `src/enc`, `src/dsp`, `src/utils`, `src/webp` and `sharpyuv`, plus `COPYING`,
  `PATENTS` and `AUTHORS`; nothing else of the release (build files, examples, tests, documentation)
- Upstream code unmodified
