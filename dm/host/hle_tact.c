





















#include "hle.h"
#include "../third_party/libdeflate/libdeflate.h"
#include <CommonCrypto/CommonDigest.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread/qos.h>
#include <stdatomic.h>





static FILE *g_chk; static pthread_mutex_t g_chk_lock = PTHREAD_MUTEX_INITIALIZER;
static void chk_on_uimsg(const char *cat, const char *act, const char *arg) {
    (void)cat; if (strcmp(arg, "MainMenuButtonRibbon") || !strstr(act, "OpenPanel")) return;
    pthread_mutex_lock(&g_chk_lock); if (g_chk) { fclose(g_chk); g_chk = NULL; LOG("DM_TACTCHK：到选角，停记"); } pthread_mutex_unlock(&g_chk_lock);
}
void tact_chk_init(void) {
    const char *e = getenv("DM_TACTCHK"); if (!e) return;
    if (!(g_chk = fopen(e, "w"))) die("DM_TACTCHK 打不开 %s", e);
    void uimsg_subscribe(void (*)(const char *, const char *, const char *)); uimsg_subscribe(chk_on_uimsg);
}
static void chk_log(const uint8_t *in, uint32_t n, const uint8_t *out, size_t got) {
    if (!g_chk) return;
    uint8_t md[16]; CC_MD5(out, (CC_LONG)got, md);
    pthread_mutex_lock(&g_chk_lock);
    if (!g_chk) { pthread_mutex_unlock(&g_chk_lock); return; }
    for (uint32_t i = 0; i < 16 && i < n; i++) fprintf(g_chk, "%02x", in[i]);
    uint32_t thr_cur_handle(void);
    fprintf(g_chk, "\t%u\t%zu\t", n, got); for (int i = 0; i < 16; i++) fprintf(g_chk, "%02x", md[i]); fprintf(g_chk, "\t%#x\n", thr_cur_handle());
    pthread_mutex_unlock(&g_chk_lock);
}

static __thread struct libdeflate_decompressor *t_dec;


static pthread_key_t g_dec_key; static pthread_once_t g_dec_once = PTHREAD_ONCE_INIT;
static void dec_free(void *p) { libdeflate_free_decompressor(p); }
static void dec_key_init(void) { if (pthread_key_create(&g_dec_key, dec_free)) die("libdeflate 解压器键创建失败"); }
static struct libdeflate_decompressor *thread_dec(void) {
    if (t_dec) return t_dec;
    if (!(t_dec = libdeflate_alloc_decompressor())) die("libdeflate 分配解压器失败");
    pthread_once(&g_dec_once, dec_key_init); pthread_setspecific(g_dec_key, t_dec); return t_dec;
}



















enum { PF_EMPTY, PF_BUSY, PF_READY, PF_GONE };
typedef struct { uint16_t fi; uint8_t sid; uint64_t off; uint32_t n, dl, next, pos; uint8_t key[16]; _Atomic int st; uint8_t *cmp, *dec; } pf_ent;
#define PF_MAXS 8
static struct { uint32_t h, n, cap, *ix, next, cur, ev; } g_ps[PF_MAXS]; static int g_nps;
static pf_ent *g_pf; static uint32_t g_npf; static int *g_pffd; static int g_npffd;
#define PF_HBITS 14
static uint32_t g_pfh[1u << PF_HBITS];
static _Atomic int64_t g_pfout, g_pfpeak; static int64_t g_pfcap; static uint32_t g_pflag; static _Atomic int g_pfstop;
static _Atomic uint32_t g_pfhit, g_pfwait, g_pfmiss, g_pfdrop, g_pfdone; static _Atomic uint64_t g_pfwait_ns;
static pthread_mutex_t g_pfl = PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t g_pfc = PTHREAD_COND_INITIALIZER;
static unsigned pf_hash(const uint8_t *k, uint32_t n) { uint64_t h = n * 0x9E3779B97F4A7C15ull; for (int i = 0; i < 16; i++) h = (h ^ k[i]) * 0x100000001b3ull; return (unsigned)(h >> (64 - PF_HBITS)); }
static void pf_free(pf_ent *e) {
    if (e->dec) { atomic_fetch_sub(&g_pfout, (int64_t)e->dl + e->n); free(e->dec); free(e->cmp); e->dec = e->cmp = NULL; }
    atomic_store(&e->st, PF_GONE); pthread_cond_broadcast(&g_pfc);
}
static void pf_consumed(pf_ent *e) {
    typeof(g_ps[0]) *ps = &g_ps[e->sid];
    if (e->pos + 1 > ps->cur) ps->cur = e->pos + 1;
    for (; ps->ev + g_pflag < ps->cur; ps->ev++) { pf_ent *x = &g_pf[ps->ix[ps->ev]]; if (atomic_load(&x->st) == PF_READY) { pf_free(x); atomic_fetch_add(&g_pfdrop, 1); } }
}
static void *pf_worker(void *arg) {
    (void)arg; pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
    struct libdeflate_decompressor *d = libdeflate_alloc_decompressor(); if (!d) die("libdeflate 分配解压器失败（预取）");
    for (;;) {
        pthread_mutex_lock(&g_pfl);
        int best = -1; int64_t lead = INT64_MAX;
        for (int k = 0; k < g_nps; k++) if (g_ps[k].next < g_ps[k].n && (int64_t)g_ps[k].next - g_ps[k].cur < lead) { lead = (int64_t)g_ps[k].next - g_ps[k].cur; best = k; }
        if (best < 0 || atomic_load(&g_pfstop)) { pthread_mutex_unlock(&g_pfl); break; }
        pf_ent *e = &g_pf[g_ps[best].ix[g_ps[best].next++]];
        while (!atomic_load(&g_pfstop) && atomic_load(&e->st) == PF_EMPTY && atomic_load(&g_pfout) > 0 && atomic_load(&g_pfout) + e->dl + e->n > g_pfcap) pthread_cond_wait(&g_pfc, &g_pfl);
        if (atomic_load(&g_pfstop)) { pthread_mutex_unlock(&g_pfl); break; }
        int ex = PF_EMPTY; int mine = atomic_compare_exchange_strong(&e->st, &ex, PF_BUSY);
        pthread_mutex_unlock(&g_pfl);
        if (!mine) continue;
        uint8_t *c = malloc(e->n), *o = malloc(e->dl); size_t got = 0;
        int ok = c && o && dm_data_pread(g_pffd[e->fi], c, e->n, (off_t)e->off) == (ssize_t)e->n
                 && libdeflate_zlib_decompress(d, c, e->n, o, e->dl, &got) == LIBDEFLATE_SUCCESS && got == e->dl;
        if (ok) { void granny_offer(const uint8_t *, size_t); granny_offer(o, e->dl); }
        pthread_mutex_lock(&g_pfl);
        if (ok && atomic_load(&g_pfstop)) ok = 0;




        if (ok) { e->cmp = c; e->dec = o; int64_t v = atomic_fetch_add(&g_pfout, (int64_t)e->dl + e->n) + e->dl + e->n;
                  for (int64_t pk = atomic_load(&g_pfpeak); v > pk && !atomic_compare_exchange_weak(&g_pfpeak, &pk, v);) {}
                  atomic_store(&e->st, PF_READY); atomic_fetch_add(&g_pfdone, 1); }
        else { free(c); free(o); atomic_store(&e->st, PF_GONE); }
        pthread_cond_broadcast(&g_pfc); pthread_mutex_unlock(&g_pfl);
    }
    libdeflate_free_decompressor(d); return NULL;
}


static int pf_take(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t cap, size_t *got) {
    if (!g_pf || n < 16) return 0;
    for (uint32_t i1 = g_pfh[pf_hash(in, n)]; i1; i1 = g_pf[i1 - 1].next) {
        pf_ent *e = &g_pf[i1 - 1];
        if (e->n != n || memcmp(e->key, in, 16)) continue;
        int st = atomic_load(&e->st);
        if (st == PF_GONE) continue;
        if (st == PF_EMPTY) {
            pf_ent *nx = e->next ? &g_pf[e->next - 1] : NULL;
            if (!nx || nx->n != n || memcmp(nx->key, in, 16)) {
                int ex = PF_EMPTY;
                if (atomic_compare_exchange_strong(&e->st, &ex, PF_GONE)) { pthread_mutex_lock(&g_pfl); pf_consumed(e); pthread_mutex_unlock(&g_pfl); }
            }
            atomic_fetch_add(&g_pfmiss, 1); return 0;
        }
        pthread_mutex_lock(&g_pfl);
        if (atomic_load(&e->st) == PF_BUSY) {
            uint64_t t0 = now_ns(); atomic_fetch_add(&g_pfwait, 1);
            while (atomic_load(&e->st) == PF_BUSY) pthread_cond_wait(&g_pfc, &g_pfl);
            atomic_fetch_add(&g_pfwait_ns, now_ns() - t0);
        }
        if (atomic_load(&e->st) == PF_READY && !memcmp(e->cmp, in, n) && e->dl <= cap) {
            memcpy(out, e->dec, e->dl); *got = e->dl;
            pf_free(e); pf_consumed(e);
            pthread_mutex_unlock(&g_pfl); atomic_fetch_add(&g_pfhit, 1); return 1;
        }
        pthread_mutex_unlock(&g_pfl);
    }
    atomic_fetch_add(&g_pfmiss, 1); return 0;
}
void tact_prefetch_stop(void) {
    if (!g_pf) return;
    pthread_mutex_lock(&g_pfl); atomic_store(&g_pfstop, 1); pthread_cond_broadcast(&g_pfc); pthread_mutex_unlock(&g_pfl);
}
static void tact_prefetch_finish(void);
static void pf_on_uimsg(const char *cat, const char *act, const char *arg) {
    (void)cat; if (!strcmp(arg, "MainMenuButtonRibbon") && strstr(act, "OpenPanel")) tact_prefetch_finish();
}




static void build_id_of(const char *path, char *out, size_t n) {
    out[0] = 0; FILE *f = fopen(path, "r"); if (!f) return;
    char l[256]; if (fgets(l, sizeof l, f) && !strncmp(l, "# BUILD_ID", 10)) { char *p = l + 10; while (*p == '\t' || *p == ' ') p++; snprintf(out, n, "%.32s", p); }
    fclose(f);
}
static void tact_data_path(const char *rom, const char *name, char *out, size_t n) {
    char rel[300]; snprintf(rel, sizeof rel, "data/%s", name);
    if (!dm_overlay_file(rel, out, n)) snprintf(out, n, "%s/%s", rom, name);
}
void tact_prefetch_start(const char *outdir) {
    const char *e = getenv("DM_TACT_PREFETCH"); char def[1100];
    if (e && !strcmp(e, "0")) { LOG("DM_TACT_PREFETCH=0：启动块预取关闭"); return; }
    if (!e) { snprintf(def, sizeof def, "%s/bootlist.tsv", outdir); if (access(def, R_OK)) return; e = def; }
    char want[40], have[40], cuts[1100]; snprintf(cuts, sizeof cuts, "%s/main.cuts.tsv", outdir);
    build_id_of(cuts, want, sizeof want); build_id_of(e, have, sizeof have);
    if (want[0] && strcmp(want, have)) { LOG("DM_TACT_PREFETCH：清单 %s 的 main 构建号「%s」与产物「%s」不符，不启用预取", e, have[0] ? have : "无", want); return; }
    FILE *f = fopen(e, "r"); if (!f) die("DM_TACT_PREFETCH 打不开 %s", e);
    char rom[1024]; const char *rs = getenv("DM_ROMFS");
    if (rs) snprintf(rom, sizeof rom, "%s/data", rs); else snprintf(rom, sizeof rom, "%s%s/data/switch/3.3.94012/Update/RomFS/data", getenv("DM_ROOT") ?: getenv("HOME"), getenv("DM_ROOT") ? "" : "/dm-work");
    char names[64][32]; uint32_t cap = 0; char line[256];
    while (fgets(line, sizeof line, f)) {
        char fn[32], kh[40]; unsigned long long off; unsigned n, dl, th = 0;
        int nf = sscanf(line, "%31s %llu %u %u %39s %x", fn, &off, &n, &dl, kh, &th);
        if (nf < 5 || strlen(kh) != 32) continue;
        int fi = 0; while (fi < g_npffd && strcmp(names[fi], fn)) fi++;
        if (fi == g_npffd) { if (g_npffd == 64) die("DM_TACT_PREFETCH：数据文件超过 64 个"); strcpy(names[g_npffd++], fn); }
        int si = 0; while (si < g_nps && g_ps[si].h != th) si++;
        if (si == g_nps) { if (g_nps == PF_MAXS) die("DM_TACT_PREFETCH：发起线程超过 %d 个", PF_MAXS); g_ps[g_nps++].h = th; }
        if (g_npf == cap) { cap = cap ? cap * 2 : 8192; g_pf = realloc(g_pf, cap * sizeof *g_pf); }
        pf_ent *x = &g_pf[g_npf]; memset(x, 0, sizeof *x); x->fi = (uint16_t)fi; x->sid = (uint8_t)si; x->off = off; x->n = n; x->dl = dl;
        for (int i = 0; i < 16; i++) { unsigned v; sscanf(kh + 2 * i, "%2x", &v); x->key[i] = (uint8_t)v; }
        typeof(g_ps[0]) *ps = &g_ps[si];
        if (ps->n == ps->cap) { ps->cap = ps->cap ? ps->cap * 2 : 1024; ps->ix = realloc(ps->ix, ps->cap * sizeof *ps->ix); }
        x->pos = ps->n; ps->ix[ps->n++] = g_npf;
        g_npf++;
    }
    fclose(f);
    if (!g_npf) die("DM_TACT_PREFETCH 清单为空：%s", e);
    g_pffd = malloc(g_npffd * sizeof *g_pffd);
    for (int i = 0; i < g_npffd; i++) { char p[1100]; tact_data_path(rom, names[i], p, sizeof p); if ((g_pffd[i] = open(p, O_RDONLY)) < 0) die("DM_TACT_PREFETCH 打不开数据文件 %s", p); dm_data_note_fd(g_pffd[i], p); }
    for (uint32_t i = g_npf; i-- > 0;) { unsigned h = pf_hash(g_pf[i].key, g_pf[i].n); g_pf[i].next = g_pfh[h]; g_pfh[h] = i + 1; }
    const char *mb = getenv("DM_TACT_PREFETCH_MB"), *nt = getenv("DM_TACT_PREFETCH_THREADS");
    g_pfcap = (mb ? atoll(mb) : 256) << 20; int th = nt ? atoi(nt) : 2;
    const char *lg = getenv("DM_TACT_PREFETCH_LAG"); g_pflag = lg ? (uint32_t)atoi(lg) : UINT32_MAX / 2;
    for (int i = 0; i < th; i++) { pthread_t t; if (pthread_create(&t, NULL, pf_worker, NULL)) die("DM_TACT_PREFETCH 起线程失败"); pthread_detach(t); }
    char sb[256] = ""; for (int k = 0; k < g_nps; k++) snprintf(sb + strlen(sb), sizeof sb - strlen(sb), "%s%#x:%u", k ? " " : "", g_ps[k].h, g_ps[k].n);
    LOG("DM_TACT_PREFETCH：清单 %u 块（%d 个数据文件；按线程分流 %s），%d 线程，封顶 %lld MiB", g_npf, g_npffd, sb, th, (long long)(g_pfcap >> 20));
    void uimsg_subscribe(void (*)(const char *, const char *, const char *)); uimsg_subscribe(pf_on_uimsg);
}
static void tact_prefetch_finish(void) {
    static _Atomic int once; if (!g_pf || atomic_exchange(&once, 1)) return;
    pthread_mutex_lock(&g_pfl);
    atomic_store(&g_pfstop, 1); uint32_t left = 0;
    for (uint32_t i = 0; i < g_npf; i++) if (atomic_load(&g_pf[i].st) == PF_READY) { pf_free(&g_pf[i]); left++; }
    pthread_cond_broadcast(&g_pfc); pthread_mutex_unlock(&g_pfl);
    { void granny_finish(const char *); granny_finish("到选角"); }
    if (left) LOG("DM_TACT_PREFETCH：到选角时缓存里还有 %u 块没人要，已释放", left);
    LOG("DM_TACT_PREFETCH：命中 %u（其中等后台解完 %u，共等 %.3f s）未命中 %u 丢弃 %u；缓存峰值 %.1f MiB；后台已解 %u / %u 块",
        atomic_load(&g_pfhit), atomic_load(&g_pfwait), atomic_load(&g_pfwait_ns) / 1e9, atomic_load(&g_pfmiss), atomic_load(&g_pfdrop),
        atomic_load(&g_pfpeak) / 1048576.0, atomic_load(&g_pfdone), g_npf);
}

static uint32_t tact_decode_zlib(uint64_t *r) {
    uint8_t *out = (uint8_t *)r[0]; uint32_t cap = (uint32_t)r[1];
    const uint8_t *in = (const uint8_t *)r[2]; uint32_t n = (uint32_t)r[3], flags = (uint32_t)r[4];
    void **ov = (void **)r[6];
    if (ov) {
        uint32_t (*cb)(void *, void *, uint32_t, const void *, uint32_t, uint32_t) = (*(void ***)ov)[2];
        if (!(cb(ov, out, cap, in, n, 1) & 0xff)) return 0;
    }
    if (flags & 1) {
        if (n < 2 || in[0] != 'Z') { LOG("tact：zlib 块编码字节不是 'Z'（%#x）", n ? in[0] : 0); return 8; }
        in++; n--;
    }
    thread_dec();
    size_t got = 0; uint64_t t0 = now_ns();
    if (pf_take(in, n, out, cap, &got)) { dm_prof_add(PROF_TACT, now_ns() - t0, got); chk_log(in, n, out, got); return 0; }
    enum libdeflate_result rc = libdeflate_zlib_decompress(t_dec, in, n, out, cap, &got);
    dm_prof_add(PROF_TACT, now_ns() - t0, got);
    if (rc == LIBDEFLATE_SUCCESS) { chk_log(in, n, out, got); return 0; }
    LOG("tact：zlib 解码失败 %d（输入 %u，容量 %u）", rc, n, cap);
    return rc == LIBDEFLATE_INSUFFICIENT_SPACE ? 4 : 1;
}



__attribute__((weak)) int dm_touch_game_hide_own_glyph(uint64_t widget) { (void)widget; return 0; }



uint64_t svc_host_call(int num, uint64_t *regs) {
    switch (num) {
    case SVC_HOST_PRIVATE_SKILL_AIM: { void svc_private_skill_aim(uint64_t *); svc_private_skill_aim(regs); return 1; }
    case SVC_HOST_PRIVATE_SKILL_RENDER: {




        { int dm_padmode_current(void); regs[8] = !dm_padmode_current() && dm_touch_game_hide_own_glyph(regs[19]) ? 1 : 0; }
        const char *path=getenv("DM_SKILL_RENDER_GATE_LOG");
        if(path) { static FILE *fp; static unsigned n[2]; if(!fp)fp=fopen(path,"w");
            if(fp && n[regs[8]]++ < 64) { fprintf(fp,"widget=%#llx match=%llu\n",regs[19],regs[8]);fflush(fp); } }
        return 1;
    }
    case SVC_HOST_TACT_ZLIB: regs[0] = tact_decode_zlib(regs); return 1;
    case SVC_HOST_C2S_DELIVER: { void c2s_deliver(uint64_t *); c2s_deliver(regs); return 1; }
    case SVC_HOST_C2S_SYS: { void c2s_deliver_sys(uint64_t *); c2s_deliver_sys(regs); return 1; }
    case SVC_HOST_GRANNY_A: { void svc_granny_chunk_a(uint64_t *); svc_granny_chunk_a(regs); return 1; }
    case SVC_HOST_DROP_SOUND: { void svc_drop_sound(uint64_t *); svc_drop_sound(regs); return 1; }
    case SVC_HOST_GROUND_ASSET: { void svc_ground_asset(uint64_t *); svc_ground_asset(regs); return 1; }
    case SVC_HOST_GRANNY_B: { void svc_granny_chunk_b(uint64_t *); svc_granny_chunk_b(regs); return 1; }
    case SVC_HOST_RIFT_EXIT: { void svc_rift_exit(uint64_t *); svc_rift_exit(regs); return 1; }
    case SVC_HOST_JOIN_SPAWN: case SVC_HOST_JOIN_CORPSE: { void svc_join_spawn(uint64_t *, int); svc_join_spawn(regs, num == SVC_HOST_JOIN_CORPSE); return 1; }
    case SVC_HOST_SAVE_ACT: { void svc_save_act(uint64_t *); svc_save_act(regs); return 1; }
    case SVC_HOST_MERC_SLOT: { void svc_merc_slot(uint64_t *); svc_merc_slot(regs); return 1; }
    case SVC_HOST_MERC_SLOT_CL: { void svc_merc_slot_cl(uint64_t *); svc_merc_slot_cl(regs); return 1; }
    case SVC_HOST_CAM_AHEAD: { void svc_cam_ahead(uint64_t *); svc_cam_ahead(regs); return 1; }
    case SVC_HOST_LEGEND_APPEND: { void svc_legend_append(uint64_t *); svc_legend_append(regs); return 1; }
    case SVC_HOST_DAMAGE: { void svc_damage(uint64_t *); svc_damage(regs); return 1; }
    case SVC_HOST_MELEE_RESIST: { void svc_melee_resist(uint64_t *); svc_melee_resist(regs); return 1; }
    case SVC_HOST_MELEE_RESULT: { void svc_melee_result(uint64_t *); svc_melee_result(regs); return 1; }
    case SVC_HOST_SHRINE_APPLY: { void svc_shrine_apply(uint64_t *); svc_shrine_apply(regs); return 1; }
    case SVC_HOST_LOOK_MODEL: { void svc_look_model(uint64_t *); svc_look_model(regs); return 1; }
    case SVC_HOST_LOOK_ICON: { void svc_look_icon(uint64_t *); svc_look_icon(regs); return 1; }
    case SVC_HOST_LOOK_COLOUR: { void svc_look_colour(uint64_t *); svc_look_colour(regs); return 1; }
    case SVC_HOST_TRANSMUTE: { void svc_transmute(uint64_t *); svc_transmute(regs); return 1; }
    case SVC_HOST_TRANSMUTE_SPILL: { void svc_unsocket_spill(uint64_t *); svc_unsocket_spill(regs); return 1; }
    case SVC_HOST_TRANSMUTE_KEEP: { void svc_unsocket_keep(uint64_t *); svc_unsocket_keep(regs); return 1; }
    }
    return 0;
}
