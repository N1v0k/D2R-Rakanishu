# libadrenotools (subset)

Source: https://github.com/bylaws/libadrenotools, commit `8fae8ce254dfc1344527e05301e43f37dea2df80`, with its submodule
linkernsbypass at commit `aa3975893d83ef1bc84c321ec60c65fbf1287887`. License: BSD 2-Clause (`LICENSE`, `lib/linkernsbypass/LICENSE`).

Only the files of the custom driver path are kept, unmodified: `include/adrenotools/{driver,priv}.h`, `src/driver.cpp`,
`src/hook/{hook_impl.cpp,hook_impl.h,hook_impl_params.h,main_hook.c,kgsl.h}` and `lib/linkernsbypass/`. Left out: the BC enabler,
the file redirect and GSL allocation hooks, the tools and the upstream build files. `CMakeLists.txt` is our own.

Used by the Android build only (`dm/android/CMakeLists.txt`): the static library goes into `libdm_driverload.so`, which `libdm.so` loads
only when the player has picked a graphics driver of his own. Without one, none of this code is loaded. The two hook libraries
`libhook_impl.so` and `libmain_hook.so` are packaged next to it.

Our changes (marked "D2R change" in the source), all in `lib/linkernsbypass/android_linker_ns.cpp`, constructor
`resolve_linker_symbols`: the search for the `bl` instruction in `dlopen` stops after 64 instructions (without it the library reports
"not loaded" and `adrenotools_open_libvulkan` returns `nullptr`, which the caller treats as a clean failure); the protection of those
instructions is changed only when `/proc/self/maps` says they are not readable, then without write permission, with the result checked
and undone after the search (the original always made the page writable and ignored the result); and the `mprotect` on the address of a
local variable (a stack page, not the linker function it was meant for) is removed.
