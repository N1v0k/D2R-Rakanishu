




#include "../cpu_translation.h"
#include "../../gfx/vulkan/engine/xbg_wtrack_switch.h"
#include <assert.h>
#include <stdio.h>

static const char *const keys[] = {"DM_GFX_VCACHE", "DM_GFX_TEX_DIRTY", "DM_GFX_WTRACK"};



static void boot(int translation, const char *const *env_file) {
    for (unsigned i = 0; i < 3; ++i) unsetenv(keys[i]);
    for (; env_file && *env_file; env_file += 2) setenv(env_file[0], env_file[1], 1);
    if (translation == DM_CPU_TRANSLATED) dm_android_translator_defaults();
    setenv("DM_GFX_VCACHE", "1", 0); setenv("DM_GFX_TEX_DIRTY", "1", 0);
}
static int env_is(const char *key, const char *want) { const char *v = getenv(key); return want ? v && !strcmp(v, want) : !v; }

static int wtrack_on(int marker_byte, int no_wtrack) { return xbg_wtrack_enabled(xbg_wtrack_marker(marker_byte, no_wtrack, getenv("DM_GFX_WTRACK")), true); }

int main(void) {
    char abi[8];

    assert(dm_android_cpu_translation("x86_64,arm64-v8a", "x86_64") == DM_CPU_TRANSLATED);

    assert(dm_android_cpu_translation("x86_64,x86,arm64-v8a,armeabi-v7a,armeabi", "x86_64") == DM_CPU_TRANSLATED);
    assert(dm_android_cpu_translation("x86,armeabi-v7a", "") == DM_CPU_TRANSLATED);

    assert(dm_android_cpu_translation("arm64-v8a,armeabi-v7a,armeabi", "arm64-v8a") == DM_CPU_NATIVE);
    assert(dm_android_cpu_translation("arm64-v8a", "arm64-v8a") == DM_CPU_NATIVE);

    assert(dm_android_cpu_translation("", "arm64-v8a") == DM_CPU_NATIVE);
    assert(dm_android_cpu_translation(NULL, "x86_64") == DM_CPU_TRANSLATED);

    assert(dm_android_cpu_translation("", "") == DM_CPU_UNKNOWN);
    assert(dm_android_cpu_translation(NULL, NULL) == DM_CPU_UNKNOWN);
    assert(dm_android_cpu_translation(",arm64-v8a", "x86_64") == DM_CPU_UNKNOWN);
    assert(dm_android_cpu_translation("arm64-v8a-hwasan,arm64-v8a", "arm64-v8a") == DM_CPU_UNKNOWN);
    assert(dm_android_cpu_translation("x86_64-foo", "x86_64") == DM_CPU_UNKNOWN);
    assert(dm_android_cpu_translation("riscv64", "riscv64") == DM_CPU_UNKNOWN);

    dm_android_primary_abi("x86_64,arm64-v8a", "", abi, sizeof abi); assert(!strcmp(abi, "x86_64"));
    dm_android_primary_abi("arm64-v8a", "", abi, sizeof abi); assert(!strcmp(abi, "arm64-v"));
    dm_android_primary_abi(NULL, NULL, abi, sizeof abi); assert(!abi[0]);


    boot(DM_CPU_NATIVE, NULL);
    assert(env_is("DM_GFX_VCACHE", "1") && env_is("DM_GFX_TEX_DIRTY", "1") && env_is("DM_GFX_WTRACK", NULL));
    assert(wtrack_on(XBG_WTRACK_NO_MARKER, 0));
    boot(DM_CPU_UNKNOWN, NULL);
    assert(env_is("DM_GFX_VCACHE", "1") && env_is("DM_GFX_TEX_DIRTY", "1") && env_is("DM_GFX_WTRACK", NULL));

    boot(DM_CPU_TRANSLATED, NULL);
    assert(env_is("DM_GFX_VCACHE", "0") && env_is("DM_GFX_TEX_DIRTY", "0") && env_is("DM_GFX_WTRACK", "0"));
    assert(!wtrack_on(XBG_WTRACK_NO_MARKER, 0));

    const char *const on[] = {"DM_GFX_VCACHE", "1", "DM_GFX_TEX_DIRTY", "1", "DM_GFX_WTRACK", "1", NULL};
    boot(DM_CPU_TRANSLATED, on);
    assert(env_is("DM_GFX_VCACHE", "1") && env_is("DM_GFX_TEX_DIRTY", "1") && env_is("DM_GFX_WTRACK", "1"));
    assert(wtrack_on(XBG_WTRACK_NO_MARKER, 0));

    boot(DM_CPU_TRANSLATED, NULL);
    assert(wtrack_on('1', 0) && !wtrack_on('1', 1));

    const char *const off[] = {"DM_GFX_VCACHE", "0", "DM_GFX_WTRACK", "0", NULL};
    boot(DM_CPU_NATIVE, off);
    assert(env_is("DM_GFX_VCACHE", "0") && env_is("DM_GFX_TEX_DIRTY", "1") && !wtrack_on(XBG_WTRACK_NO_MARKER, 0));

    boot(DM_CPU_NATIVE, NULL);
    assert(!wtrack_on('0', 0) && !wtrack_on(-1, 0));
    puts("PASS cpu_translation: emulator, houdini, phones, abi fallback, unknown, whole-entry match, truncation, env/marker precedence");
    return 0;
}
