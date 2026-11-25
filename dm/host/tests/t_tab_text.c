#define _GNU_SOURCE
#include <assert.h>
#include "../dm_tab_text.c"

int dm_overlay_file(const char *rel, char *out, size_t n) { (void)rel; (void)out; (void)n; return 0; }
static void put64(unsigned char *p, int at, uint64_t value) { memcpy(p + at, &value, 8); }
static void put32(unsigned char *p, int at, int32_t value) { memcpy(p + at, &value, 4); }
static int y(unsigned char *p) { int32_t v; memcpy(&v, p + 0x74, 4); return v; }
static int parse(const char *s) { FILE *f = fmemopen((void *)s, strlen(s), "r"); int ok = read_metrics(f); fclose(f); return ok; }
int main(void) {
    const char *valid = "TAB_INK_V1\n-156\tJbcd\n94\tÁbcde\n0\tWxyz\nEND\n";
    const char *invalid[] = {"", "TAB_INK_V2\n", "TAB_INK_V1\n0\tX\n", "TAB_INK_V1\nEND\n",
        "TAB_INK_V1\n1001\tX\nEND\n", "TAB_INK_V1\n0\tX\n1\tX\nEND\n", "TAB_INK_V1\n0\tX\nEND\nextra"};
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; i++) { assert(!parse(invalid[i])); assert(!metric_count); }
    assert(parse(valid)); loaded = 1;
    unsigned char tabs[0xe50] = {0}, image[0x100] = {0}, text[0x100] = {0};
    uint8_t *base = (uint8_t *)0x10000000;
    put64(tabs, 0, (uintptr_t)base + DM_VT_TABBAR); put32(tabs, 0x558, 1); put32(tabs, 0xb2c, -4);
    put64(image, 0, (uintptr_t)base + DM_VT_IMAGE); put32(image, 0x74, 100);
    put64(text, 0, (uintptr_t)base + DM_VT_TEXT); put32(text, 0xb8, 40);
    put64(tabs, 0xdc8, (uintptr_t)image); put64(tabs, 0xe08, (uintptr_t)text);
    char label[128] = "Jbcd"; put64(text, 0x88, (uintptr_t)label); put64(text, 0x90, strlen(label));
    for (int i = 0; i < 20; i++) dm_tab_text_center(base, (uintptr_t)tabs, 12);
    assert(y(text) == 90);
    put32(image, 0x74, 200); dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 190);
    strcpy(label, "Wxyz"); dm_tab_text_center(base, (uintptr_t)tabs, 0); assert(y(text) == 196);
    strcpy(label, "Ábcde"); put64(text, 0x90, strlen(label)); dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 200);
    put32(text, 0xb8, 20); dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 198);
    put32(text, 0x74, 123); dm_tab_text_center(base, (uintptr_t)tabs, 10); assert(y(text) == 123);
    put64(text, 0, 0); dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 123);
    put64(text, 0, (uintptr_t)base + DM_VT_TEXT); put32(tabs, 0x558, 9);
    dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 123);
    assert(!parse("TAB_INK_V1\n0\tX\n")); put32(tabs, 0x558, 1);
    dm_tab_text_center(base, (uintptr_t)tabs, 12); assert(y(text) == 123);
    puts("tab text: parser, font scaling, relayout, repeated updates, locale and widget guards passed");
}
