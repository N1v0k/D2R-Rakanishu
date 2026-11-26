







#include "../dm_pouch.c"
#include "../game_addrs.h"
#include <setjmp.h>
module_t g_mod[MAX_MOD]; int g_nmod;
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)


void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);
uint64_t dm_fs_commit_count(void) { return 0; }
void dm_fs_save_poison(int e) { (void)e; }
void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn) { (void)h; (void)hn; (void)n; (void)nn; return NULL; }
static jmp_buf back; static char written[32]; static int writes;
static void set_text_stub(uint64_t str, const char *s, uint64_t n) { (void)str; snprintf(written, sizeof written, "%.*s", (int)n, s); writes++; longjmp(back, 1); }
typedef struct { _Alignas(8) unsigned char b[0x100]; } widget;
static widget box, count;
static uint64_t kids[1];
static uint64_t P(const void *p) { return (uint64_t)(uintptr_t)p; }
static void text_of(widget *w, const char *t) { *(const char **)(w->b + 0x88) = t; *(uint64_t *)(w->b + 0x90) = strlen(t); }
static void sync(void) { written[0] = 0; if (!setjmp(back)) pouch_count_sync(P(&box)); }

int main(void) {
    B = (uint8_t *)((uintptr_t)set_text_stub - GA_TEXT_SET);
    memset(&box, 0, sizeof box); *(const char **)(box.b + 8) = "TouchPouchBox"; kids[0] = P(&count); *(uint64_t *)(box.b + 0x58) = P(kids); *(uint64_t *)(box.b + 0x60) = 1;
    memset(&count, 0, sizeof count); *(const char **)(count.b + 8) = "TouchPouchCount"; *(uint64_t *)count.b = (uint64_t)(uintptr_t)B + GA_VT_TEXTBOX; count.b[0x50] = 1;

    atomic_store(&g_ucount, 5); text_of(&count, "0"); sync(); CHECK(writes == 1 && !strcmp(written, "5"), "应补写 5（写了 %d 次：%s）", writes, written);

    text_of(&count, "5"); sync(); CHECK(writes == 1, "字对时不该写（写了 %d 次）", writes);

    for (int i = 0; i < 5; i++) sync(); CHECK(writes == 1, "对的时候连着几帧不写（写了 %d 次）", writes);
    text_of(&count, "0"); sync(); CHECK(writes == 2 && !strcmp(written, "5"), "又重建后应再补写（写了 %d 次：%s）", writes, written);

    atomic_store(&g_ucount, 0); text_of(&count, "0"); sync(); CHECK(writes == 2, "空囊时不该写（写了 %d 次）", writes);

    atomic_store(&g_ucount, 3); *(const char **)(count.b + 8) = "Count"; *(const char **)(box.b + 8) = "TouchPouchBox"; sync(); CHECK(writes == 2, "名字不对不该写（写了 %d 次）", writes);
    *(const char **)(count.b + 8) = "TouchPouchCount"; *(uint64_t *)count.b = (uint64_t)(uintptr_t)B + GA_VT_TOGGLE_BUTTON; sync(); CHECK(writes == 2, "类不对不该写（写了 %d 次）", writes);
    printf("t_pouch_count: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
