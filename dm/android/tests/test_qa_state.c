






#include "../qa_state.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

static char out[65536];
static const char *esc(const char *s) { qa_buf b = {out, sizeof out, 0, 0}; out[0] = 0; qa_json_str(&b, s, strlen(s)); assert(!b.full); return out; }

typedef struct { uint64_t seq; char fill[4096]; uint64_t check; } blob;
static blob g_blob[3]; static qa_tri g_tri; static _Atomic int g_done;
static void *producer(void *arg) {
    (void)arg;
    for (uint64_t i = 1; i <= 200000; i++) { blob *b = &g_blob[g_tri.back]; b->seq = i; memset(b->fill, (int)(i & 0xff), sizeof b->fill); b->check = i; qa_tri_publish(&g_tri); }
    atomic_store(&g_done, 1); return NULL;
}

int main(int argc, char **argv) {

    assert(!strcmp(esc("a\"b\\c\nd\te"), "\"a\\\"b\\\\c\\nd\\u0009e\""));
    assert(!strcmp(esc("Dolch \xee\x81\xbe" "4Schaden"), "\"Dolch \xee\x81\xbe" "4Schaden\""));
    assert(!strcmp(esc("\xe5\x8c\x95\xe9\xa6\x96"), "\"\xe5\x8c\x95\xe9\xa6\x96\""));
    assert(!strcmp(esc("x\xff" "y"), "\"x\\ufffdy\""));
    assert(!strcmp(esc("\xc0\xaf"), "\"\\ufffd\\ufffd\""));
    assert(!strcmp(esc("\xed\xa0\x80"), "\"\\ufffd\\ufffd\\ufffd\""));
    assert(!strcmp(esc("\xe5\x8c"), "\"\\ufffd\\ufffd\""));
    assert(!strcmp(esc("\x7f"), "\"\\u007f\""));

    assert(qa_utf8_cut("ab\xe5\x8c\x95", 5, 4) == 2 && qa_utf8_cut("ab\xe5\x8c\x95", 5, 5) == 5 && qa_utf8_cut("abc", 3, 2) == 2);

    assert(qa_token_ok("app_menu", QA_NAME) && qa_token_ok("recipe_book", QA_NAME) && qa_token_ok("a1", QA_NAME));
    assert(!qa_token_ok("", QA_NAME) && !qa_token_ok("App", QA_NAME) && !qa_token_ok("a b", QA_NAME) && !qa_token_ok("a\n", QA_NAME) && !qa_token_ok("x;rm", QA_NAME) && !qa_token_ok("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", QA_NAME) && !qa_token_ok(NULL, QA_NAME));
    {
        char st[QA_OV_STATE + 1]; memset(st, '1', sizeof st); st[QA_OV_STATE - 1] = 0; assert(qa_token_ok(st, QA_OV_STATE) && !qa_token_ok(st, QA_NAME));
        st[QA_OV_STATE - 1] = '1'; st[QA_OV_STATE] = 0; assert(!qa_token_ok(st, QA_OV_STATE));
    }

    const uint64_t ms = 1000000ull;
    assert(qa_write_due(0, 1, 0, 500 * ms, 5000 * ms));
    assert(!qa_write_due(1, 1000 * ms + 499 * ms, 1000 * ms, 500 * ms, 5000 * ms) && qa_write_due(1, 1500 * ms, 1000 * ms, 500 * ms, 5000 * ms));
    assert(!qa_write_due(0, 5999 * ms, 1000 * ms, 500 * ms, 5000 * ms) && qa_write_due(0, 6000 * ms, 1000 * ms, 500 * ms, 5000 * ms));

    static qa_tips t; memset(&t, 0, sizeof t);
    qa_tips_add(&t, "Dolch", 10, 0, 1); qa_tips_add(&t, "Dolch", 10, 0, 1); assert(t.n == 1 && t.frame == 10);
    qa_tips_add(&t, "Kurzschwert", 10, 0, 1); qa_tips_add(&t, "Dritter", 10, 0, 1); assert(t.n == 2 && t.overflow);
    qa_tips_add(&t, "Neu", 11, 0, 1); assert(t.n == 1 && !t.overflow && t.frame == 11 && !strcmp(t.text[0], "Neu"));
    { static char big[2000]; memset(big, 'a', sizeof big - 1); big[sizeof big - 1] = 0; memcpy(big + QA_TIP - 1, "\xe5\x8c\x95", 3);
      qa_tips_add(&t, big, 12, 0, 1); assert(t.n == 1 && t.truncated && strlen(t.text[0]) == QA_TIP - 1); }
    static qa_tips u; memcpy(&u, &t, sizeof u); u.frame = 99; assert(qa_tips_same(&t, &u)); u.text[0][0] = 'b'; assert(!qa_tips_same(&t, &u));
    memcpy(&u, &t, sizeof u); u.id = 7; assert(!qa_tips_same(&t, &u));


    static qa_snapshot s; memset(&s, 0, sizeof s);
    s.kills = 3;
    s.screen = QA_SCREEN_INGAME; s.hud.valid = 1; s.hud.area = 40; s.hud.x = 5100; s.hud.y = 4900; s.hud.level = 7; s.hud.hp = 90; s.hud.hp_max = 120; s.hud.mp = 30; s.hud.mp_max = 40;
    s.panels.n = 2; strcpy(s.panels.name[0], "PlayerInventoryExpansionLayout"); strcpy(s.panels.name[1], "SkillsTreePanel");
    s.tips.n = 1; strcpy(s.tips.text[0], "Dolch\n\xee\x81\xbe" "4Schaden: 1-4");
    s.overlays = 2; strcpy(s.ov_name[0], "app_menu"); strcpy(s.ov_state[0], "closed"); strcpy(s.ov_name[1], "pad_layout"); strcpy(s.ov_state[1], "touch");
    static char body[64000], file[72000]; qa_buf bb = {body, sizeof body, 0, 0};
    assert(!qa_render_body(&bb, &s));
    assert(strstr(body, "\"screen\":\"ingame\""));
    assert(strstr(body, "\"kills\":3"));
    assert(strstr(body, "\"area\":40,\"act\":2,\"x\":5100,\"y\":4900,\"level\":7"));
    assert(strstr(body, "\"hp\":[90,120],\"mp\":[30,40]"));
    assert(strstr(body, "\"panels\":[\"PlayerInventoryExpansionLayout\",\"SkillsTreePanel\"],\"panels_overflow\":0"));
    assert(strstr(body, "\"tooltip\":{\"texts\":[\"Dolch\\n\xee\x81\xbe" "4Schaden: 1-4\"],\"truncated\":0,\"overflow\":0,\"hovered\":0}"));
    assert(strstr(body, "\"overlays\":{\"app_menu\":\"closed\",\"pad_layout\":\"touch\"}"));

    assert(!strstr(body, "mono") && !strstr(body, "frame") && !strstr(body, "age"));
    qa_stamps st = {.pid = 1234, .start_ms = 10, .mono_ms = 20000, .wall_ms = 1730000000000LL, .hud_age_ms = 16, .tip_age_ms = 40, .tip_frames = 1, .overlay_age_ms = -1, .seq = 7, .frame = 5123};
    qa_buf fb = {file, sizeof file, 0, 0}; assert(!qa_render_file(&fb, &st, body));
    assert(!strncmp(file, "{\"v\":1,\"session\":{\"pid\":1234,\"start_ms\":10},\"seq\":7,\"mono_ms\":20000,", 66));
    assert(strstr(file, "\"age_ms\":{\"hud\":16,\"tooltip\":40,\"tooltip_frames\":1,\"overlays\":-1}}\n"));

    s.tips.n = 0; s.panels.n = 0; bb = (qa_buf){body, sizeof body, 0, 0}; assert(!qa_render_body(&bb, &s));
    assert(strstr(body, "\"panels\":[],") && strstr(body, "\"tooltip\":null"));

    char tiny[64]; qa_buf tb = {tiny, sizeof tiny, 0, 0}; assert(qa_render_body(&tb, &s) == -1 && tb.full);

    s.panels.n = QA_PANELS; for (int i = 0; i < QA_PANELS; i++) { memset(s.panels.name[i], 'P', QA_PANEL_NAME - 1); s.panels.name[i][QA_PANEL_NAME - 1] = 0; }
    s.tips.n = 2; for (int i = 0; i < 2; i++) { memset(s.tips.text[i], 1, QA_TIP); s.tips.text[i][QA_TIP] = 0; }
    s.overlays = QA_OVERLAYS; for (int i = 0; i < QA_OVERLAYS; i++) { memset(s.ov_name[i], 'n', QA_NAME - 1); s.ov_name[i][QA_NAME - 1] = 0; memset(s.ov_state[i], 's', QA_OV_STATE - 1); s.ov_state[i][QA_OV_STATE - 1] = 0; }
    bb = (qa_buf){body, sizeof body, 0, 0}; assert(!qa_render_body(&bb, &s));
    fb = (qa_buf){file, sizeof file, 0, 0}; assert(!qa_render_file(&fb, &st, body));

    if (argc > 1) {
        memset(&s, 0, sizeof s); s.screen = QA_SCREEN_INGAME; s.hud.valid = 1; s.hud.area = 40; s.panels.n = 1; strcpy(s.panels.name[0], "PlayerInventoryExpansionLayout");
        s.tips.n = 2; strcpy(s.tips.text[0], "Dolch\n\xee\x81\xbe" "4\"Schaden\"\\ 1-4"); memset(s.tips.text[1], 2, 10); s.tips.text[1][10] = (char)0xff; s.tips.text[1][11] = 0;
        s.overlays = 1; strcpy(s.ov_name[0], "app_menu"); strcpy(s.ov_state[0], "open");
        bb = (qa_buf){body, sizeof body, 0, 0}; assert(!qa_render_body(&bb, &s)); fb = (qa_buf){file, sizeof file, 0, 0}; assert(!qa_render_file(&fb, &st, body));
        FILE *f = fopen(argv[1], "w"); assert(f); fputs(file, f); fclose(f);
    }


    qa_tri_init(&g_tri); pthread_t th; assert(!pthread_create(&th, NULL, producer, NULL));
    uint64_t last = 0, takes = 0;
    while (!atomic_load(&g_done) || qa_tri_take(&g_tri)) {
        if (!qa_tri_take(&g_tri)) continue;
        const blob *b = &g_blob[g_tri.front]; takes++;
        assert(b->seq == b->check && b->seq >= last);
        for (size_t i = 0; i < sizeof b->fill; i += 512) assert((unsigned char)b->fill[i] == (b->seq & 0xff));
        last = b->seq;
    }
    pthread_join(th, NULL);
    qa_tri_take(&g_tri); assert(g_blob[g_tri.front].seq == 200000);
    printf("PASS qa_state: JSON escaping/UTF-8, body/file format, pacing, names, tooltip gathering, triple buffer (%llu takes, no tearing)\n", (unsigned long long)takes);
    return 0;
}
