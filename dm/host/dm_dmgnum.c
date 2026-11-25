

































#include "dm.h"
#include "hle.h"
#include "dm_custom.h"
#include "dm_splash.h"
#include "dm_unit.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

enum { E_PHYSICAL, E_FIRE, E_COLD, E_LIGHTNING, E_POISON, E_MAGIC, NE };
enum { RING = 128, LIVE = 32, DOT = 48, DOT_STEP = 25, FIELDS = 8, STATE_POISON = 2, STATE_BURNING = 0x73, RESULT_CRIT = 0x2000 };

enum { F_DOT = 1, F_MINION = 2, F_WHITE = 4, F_CRIT = 8, F_SPLASH = 16 };
enum { MODE_OFF, MODE_SIMPLE, MODE_COLOUR };
static const uint64_t LIFE_NS = 1100000000ull;
static const uint16_t k_share_at[NE] = { 0x18, 0x20, 0x34, 0x2c, 0x38, 0x30 };

typedef struct { uint32_t unit; int32_t share[NE]; uint8_t flags; } hit_t;
typedef struct { uint32_t unit, seq; int32_t value; uint8_t kind, flags; uint64_t born; float x, y; } live_t;
typedef struct { uint32_t unit, seen; int32_t life, hits, rest; } dot_t;

static uint8_t *_Atomic B;
static atomic_int g_on;
static atomic_uint g_epoch;
static atomic_uint g_resume;
static hit_t g_ring[RING];
static atomic_uint g_head, g_tail;
static live_t g_live[LIVE];
static float g_pub[LIVE * FIELDS];
static int g_npub;
static pthread_mutex_t g_pub_lock = PTHREAD_MUTEX_INITIALIZER;
static dot_t g_dot[DOT];
static uint64_t g_game;

#ifndef DM_PLAYER_BUILD
static atomic_uint g_qa_kills;
unsigned dm_dmgnum_qa_kills(void) { return atomic_load(&g_qa_kills); }
static atomic_uint g_gate_calls;
unsigned dm_dmgnum_gate_calls(void) { return atomic_load(&g_gate_calls); }
#endif

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }
static int owned(uint64_t u) { return rd32(u + 0x124) >> 31; }
static int theirs(uint64_t u) { uint32_t t = rd32(u); return t == 0 || (t == 1 && owned(u)); }
static int log_on(void) { static int log = -1; if (log < 0) log = getenv("DM_DMG_LOG") != NULL; return log; }

static void push(const hit_t *h) {
    unsigned hd = atomic_load_explicit(&g_head, memory_order_relaxed);
    if (hd - atomic_load_explicit(&g_tail, memory_order_acquire) >= RING) return;
    g_ring[hd % RING] = *h;
    atomic_store_explicit(&g_head, hd + 1, memory_order_release);
}






static int32_t dm_dmgnum_split(const int32_t share[NE], int mode, int *kind) {
    int64_t sum = 0; int top = 0;
    for (int e = 0; e < NE; e++) { if (share[e] > 0) sum += share[e]; if (share[e] > share[top]) top = e; }
    *kind = E_PHYSICAL;
    if (sum <= 0 || mode == MODE_OFF) return 0;
    if (mode == MODE_COLOUR) *kind = top;
    int64_t v = (sum + 128) >> 8;
    return v < 1 ? 1 : v > 999999 ? 999999 : (int32_t)v;
}


static int mode_now(void) { int m = dm_custom_get("dmg_num"); return m >= MODE_OFF && m <= MODE_COLOUR ? m : MODE_COLOUR; }

static uint8_t *base(void) {
    uint8_t *b = atomic_load(&B); if (b) return b;
    extern module_t g_mod[]; extern int g_nmod;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) { b = (uint8_t *)g_mod[i].base; atomic_store(&B, b); }
    return b;
}

#include "dm_combat_log.inc"

static void note_hit(const uint64_t *regs) {
    uint64_t d = regs[19], defender = regs[20], attacker = regs[21];
    if (!d || !defender || !attacker || rd32(defender) != 1 || owned(defender) || !theirs(attacker)) return;
    int32_t total = (int32_t)rd32(d + 0x134); if (total < 1) return;
    hit_t h = { rd32(defender + 8), { 0 }, (uint8_t)((rd32(attacker) == 1 ? F_MINION : 0) | (rd32(d + 4) & RESULT_CRIT ? F_CRIT : 0) | (dm_splash_running() ? F_SPLASH : 0)) };
    int64_t sum = 0;
    for (int e = 0; e < NE; e++) { int32_t v = (int32_t)rd32(d + k_share_at[e]); h.share[e] = v > 0 ? v : 0; sum += h.share[e]; }
    if (sum != total) {
        if (log_on()) LOG("damage: the shares add up to %lld, not to the total %d; shown as one white total", (long long)sum, total);
        memset(h.share, 0, sizeof h.share); h.share[E_PHYSICAL] = total; h.flags |= F_WHITE;
    }
    if (log_on()) LOG("damage: attacker type %u class %u, monster %u, total %d, physical %d fire %d lightning %d magic %d cold %d poison %d, result %#x", rd32(attacker), rd32(attacker + 4), rd32(defender + 8), total, (int32_t)rd32(d + 0x18), (int32_t)rd32(d + 0x20), (int32_t)rd32(d + 0x2c), (int32_t)rd32(d + 0x30), (int32_t)rd32(d + 0x34), (int32_t)rd32(d + 0x38), rd32(d + 4) & 0xffff);
    push(&h);
    uint32_t id = h.unit; int at = -1, room = -1;
    for (int i = 0; i < DOT; i++) { if (g_dot[i].unit == id) { at = i; break; } if (!g_dot[i].unit && room < 0) room = i; }
    if (at >= 0) g_dot[at].hits += total;
    else if (room >= 0 && ((int32_t)rd32(d + 0x38) > 0 || (int32_t)rd32(d + 0x24) > 0)) {
        int life = ((int (*)(uint64_t, int, int))(uintptr_t)(base() + 0x5b9290))(defender, 6, 0);
        g_dot[room] = (dot_t){id, rd32(regs[22] + 0x170), life > total ? life - total : 0, 0, 0};
    }
}


static void read_hit(const uint64_t *regs) {
    if (!atomic_load_explicit(&g_on, memory_order_relaxed)) return;
    uint64_t t0 = log_on() ? now_ns() : 0;
    note_hit(regs);
    if (log_on()) { static uint64_t sum; static unsigned n; sum += now_ns() - t0; if (++n == 32) { LOG("damage: read a hit in %llu ns on average (server thread)", (unsigned long long)(sum / n)); sum = 0; n = 0; } }
}




void svc_damage(uint64_t *regs) {
    { int dm_resume_shields(uint64_t); if (dm_resume_shields(regs[0])) return; }
#ifndef DM_PLAYER_BUILD
    atomic_fetch_add(&g_gate_calls, 1);
    probe_direct(regs);
    combat_capture(regs);
#endif
    read_hit(regs);
    void dm_fightlog_hit(const uint64_t *); dm_fightlog_hit(regs);
#ifndef DM_PLAYER_BUILD

    static int observe = -1;
    if (observe < 0) { const char *e = getenv("DM_QA_STATE"); observe = e && !strcmp(e, "1"); }
    uint64_t target = regs[0], attacker = regs[21];
    if (observe && target && attacker && (int)regs[1] == 6 && (int32_t)regs[2] <= 0 && rd32(target) == 1 && !owned(target) && theirs(attacker)) {
        int before = ((int (*)(uint64_t, int, int))(uintptr_t)(base() + 0x5b9290))(target, 6, 0);
        if (before > 0) atomic_fetch_add(&g_qa_kills, 1);
    }
#endif
    ((void (*)(uint64_t, int, int, int))(uintptr_t)(base() + 0x5bc1f0))(regs[0], (int)regs[1], (int)regs[2], (int)regs[3]);
}


static int over_time(uint64_t G, uint64_t u) {
    uint64_t list = rd(u + 0x88); if (!list || !(rd32(list + 0x1c) >> 31)) return 0;
    int found = 0;
    for (int pass = 0; pass < 2; pass++) for (uint64_t n = rd(list + (pass ? 0xd8 : 0xd0)), k = 0; n && k < 64; n = rd(n + 0x68), k++) {
        uint32_t state = rd32(n + 0x20); if (state != STATE_POISON && state != STATE_BURNING) continue;
        uint32_t type = rd32(n + 8), id = rd32(n + 0xc), mine = type == 0;
        if (type == 1) for (uint64_t o = rd(G + 0x2230 + 0x400 + 8 * (uint64_t)(id & 0x7f)), j = 0; o && j < 256; o = rd(o + 0x158), j++) if (rd32(o + 8) == id) { mine = owned(o); break; }
        if (mine && (state == STATE_POISON || !found)) found = (int)state;
    }
    return found;
}
int dm_dmgnum_over_time(uint64_t G, uint64_t u) { return over_time(G, u); }


static uint64_t g_pass_game; static uint32_t g_pass_tick, g_pass_ids[DOT]; static int g_pass_n = -1;
int dm_dmgnum_dot_pass(uint64_t G, uint32_t tick, const uint32_t **ids) {
    if (g_pass_n < 0 || G != g_pass_game || tick != g_pass_tick) return -1;
    *ids = g_pass_ids; return g_pass_n;
}


void dm_dmgnum_tick(uint64_t G) {
    static unsigned epoch;
    if (!atomic_load_explicit(&g_on, memory_order_relaxed)) return;
    unsigned ep = atomic_load(&g_epoch);
    if (G != g_game || ep != epoch) { g_game = G; epoch = ep; memset(g_dot, 0, sizeof g_dot); }
    uint32_t now = rd32(G + 0x170); if (now % DOT_STEP) return;
    int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(base() + 0x5b9290);
    uint64_t t0 = log_on() ? now_ns() : 0;
    int found = 0;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 0x400 + 8 * (uint64_t)b); u; u = rd(u + 0x158)) {
        uint32_t mode = rd32(u + 0xc); if (mode == 0 || mode == 12 || owned(u)) continue;
        int state = over_time(G, u); if (!state) continue;
        uint32_t id = rd32(u + 8); int life = get(u, 6, 0), at = -1, room = -1;
        if (found >= 0) { if (found < DOT) g_pass_ids[found++] = id; else found = -1; }
        for (int i = 0; i < DOT; i++) { if (g_dot[i].unit == id) { at = i; break; } if (!g_dot[i].unit && room < 0) room = i; }
        if (at < 0) { if (room >= 0) g_dot[room] = (dot_t){id, now, life, 0, 0}; continue; }
        dot_t *e = &g_dot[at];
        int32_t lost = e->life - life - e->hits + e->rest;
        e->life = life; e->hits = 0; e->seen = now; e->rest = lost > 0 ? lost % 256 : 0;
        if (lost >= 256) { hit_t h = { id, { 0 }, F_DOT }; h.share[state == STATE_POISON ? E_POISON : E_FIRE] = lost - e->rest; push(&h); }
        if (log_on()) LOG("damage over time: monster %u state %d lost %d", id, state, lost);
    }
    g_pass_game = G; g_pass_tick = now; g_pass_n = found;
    for (int i = 0; i < DOT; i++) if (g_dot[i].unit && now - g_dot[i].seen >= DOT_STEP) g_dot[i].unit = 0;
    if (log_on() && !(now % 250)) LOG("damage over time: one pass over the monsters took %llu ns (server thread, once a second)", (unsigned long long)(now_ns() - t0));
}











#ifndef DM_PLAYER_BUILD
static pthread_once_t speed_once=PTHREAD_ONCE_INIT;
static int speed_fixture[4],speed_enabled;
static void speed_fixture_read(void) {
    const char *input=getenv("DM_TEST_COMBAT_SPEED");int used=0;
    if(!input)return;
    if(sscanf(input,"%d:%d:%d:%d%n",&speed_fixture[0],&speed_fixture[1],&speed_fixture[2],&speed_fixture[3],&used)!=4 || input[used]) {
        LOG("ASSUME combat.fixture: invalid speed inputs");return;
    }
    for(int k=0;k<4;k++)if(speed_fixture[k]<0 || speed_fixture[k]>1000) {
        LOG("ASSUME combat.fixture: speed input outside supported range");return;
    }
    speed_enabled=1;
}
void dm_combat_fixture_speed(uint8_t *b,uint64_t unit,int side) {
    static uint64_t applied[2];
    pthread_once(&speed_once,speed_fixture_read);
    if(!speed_enabled || !b || !unit || side<0 || side>1 || applied[side]==unit || rd32(unit)!=0 || !rd(unit+0x88))return;
    const int stats[4]={93,105,99,102};int observed[4];
    for(int k=0;k<4;k++) {
        ((void (*)(uint64_t,int,int,int))(b+0x5bc1f0))(unit,stats[k],speed_fixture[k],0);
        observed[k]=((int (*)(uint64_t,int,int))(b+0x5b9290))(unit,stats[k],0);
    }
    applied[side]=unit;
    LOG("[combat-speed] {\"side\":%d,\"base\":[%d,%d,%d,%d],\"observed\":[%d,%d,%d,%d]}",side,
        speed_fixture[0],speed_fixture[1],speed_fixture[2],speed_fixture[3],observed[0],observed[1],observed[2],observed[3]);
}
void dm_combat_fixture_cast(uint64_t game,uint64_t player) {
    const char *server=getenv("DM_TEST_CAST_SERVER");
    if((server && !strcmp(server,"1"))!=(game!=0))return;
    static int at = -1, every, n, k, ntarget, limit, left_hand, pre_at, pre_skill; static int sk[8], target_class[4]; static uint64_t next;
    if (at < 0) { const char *e = getenv("DM_TEST_CAST"), *c = e ? strchr(e, ':') : NULL; at = 0;
        const char *early=getenv("DM_TEST_CAST_PRE"); int read=0;
        if(early && (sscanf(early,"%d:%d%n",&pre_at,&pre_skill,&read)!=2 || early[read] || pre_at<1 || pre_skill<0)) {
            LOG("ASSUME combat.fixture: invalid early cast");pre_at=0;
        }
        if (c && sscanf(e, "%d:%d", &at, &every) == 2 && (c = strchr(c + 1, ':'))) while (c && n < 8 && sscanf(c + 1, "%d", &sk[n]) == 1) { n++; c = strchr(c + 1, ','); }
        const char *target=getenv("DM_TEST_CAST_CLASS"), *count=getenv("DM_TEST_CAST_LIMIT"), *side=getenv("DM_TEST_CAST_LEFT");
        left_hand=side && !strcmp(side,"1");
        int used=0;

        for(const char *p=target;p && ntarget<4;p=*p==','?p+1:NULL) {
            if(sscanf(p,"%d%n",&target_class[ntarget],&used)!=1 || target_class[ntarget]<0 || (p[used] && p[used]!=',')) {LOG("ASSUME combat.fixture: invalid cast target class");n=0;ntarget=0;break;}
            ntarget++;p+=used;
            if(*p==',' && ntarget==4) {LOG("ASSUME combat.fixture: invalid cast target class");n=0;ntarget=0;break;}
        }
        used=0;
        if(count && (sscanf(count,"%d%n",&limit,&used)!=1 || count[used] || limit<1)) {
            LOG("ASSUME combat.fixture: invalid cast request limit");n=0;
        }
        if (!n || every < 1) at = 0; else LOG("DM_TEST_CAST: from frame %d every %d frames, %d skills", at, every, n); }
    uint8_t *b = base(); if (!b) return;
    uint64_t f = game?rd32(game+0x170):dm_guest_frame(); int pre = pre_at > 0 && f >= (uint64_t)pre_at;
    if (!pre && (at <= 0 || (limit && k>=limit) || f < (uint64_t)at || f < next)) return;
    uint64_t pl = game?player:((uint64_t (*)(uint32_t))(uintptr_t)(b + 0x45f9d0))(rd32((uint64_t)(uintptr_t)(b + 0x2d20e4c))), pp = pl ? rd(pl + 0x38) : 0; if (!pp) return;
    int lv = dm_unit_level(pl); if (lv == 0 || lv == 1 || lv == 40 || lv == 75 || lv == 103 || lv == 109) return;
    int px = *(volatile uint16_t *)(uintptr_t)(pp + 2), py = *(volatile uint16_t *)(uintptr_t)(pp + 6); uint32_t best = 0, cls = 0; long bd = 30 * 30 + 1;
    uint64_t monsters=game?game+0x2230+0x400:(uint64_t)(uintptr_t)(b+0x2d603a0+0x400);
    for (int bk = 0; bk < 128; bk++) for (uint64_t u = rd(monsters + 8 * bk), j = 0; u && j < 256; u = rd(u + 0x158), j++) {
        uint32_t m = rd32(u + 0xc), c = rd32(u + 4); uint64_t q = rd(u + 0x38); if (m == 0 || m == 12 || c == 271 || c == 338 || c == 359 || c == 560 || c == 561 || !q) continue;
        if(!pre && ntarget) { int want=0; for(int i=0;i<ntarget;i++) want|=c==(unsigned)target_class[i]; if(!want)continue; }
        long dx = (long)*(volatile uint16_t *)(uintptr_t)(q + 2) - px, dy = (long)*(volatile uint16_t *)(uintptr_t)(q + 6) - py, d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = rd32(u + 8); cls = rd32(u + 4); } }
    if (!best) return;
    int s, left = pre ? 0 : left_hand;
    if (pre) { s = pre_skill; pre_at = 0; } else { next = f + (uint64_t)every; s = sk[k++ % n]; }

    uint8_t sel[9] = { 0x3c, (uint8_t)s, (uint8_t)(s >> 8), 0, left ? 0x80 : 0, 0xff, 0xff, 0xff, 0xff }, use[9] = { left ? 0x06 : 0x0d, 1, 0, 0, 0, (uint8_t)best, (uint8_t)(best >> 8), (uint8_t)(best >> 16), (uint8_t)(best >> 24) };
    if(game) {
        static int selected=-1;
        int (*dispatch)(uint64_t,uint64_t,uint64_t,uint32_t)=(void *)(b+0x6e5b40);
        volatile uint32_t *busy=(volatile uint32_t *)(uintptr_t)(game+0x19050);uint32_t saved=*busy;
        *busy=1;
        if(selected!=s || pre){dispatch(game,pl,(uint64_t)(uintptr_t)sel,9);selected=pre?-1:s;}
        int before=(int)rd32(pl+0xc),result=dispatch(game,pl,(uint64_t)(uintptr_t)use,9);*busy=saved;
        uint64_t skills=rd(pl+0x100),right=skills?rd(skills+(left?8:0x10)):0,record=right?rd(right):0;
        int actual=record?*(int16_t *)(uintptr_t)record:-1;
        LOG("[combat-request] {\"tick\":%llu,\"skill\":%d,\"selected\":%d,\"target\":%u,\"class\":%u,\"mode_before\":%d,\"mode_after\":%u,\"result\":%d}",(unsigned long long)f,s,actual,best,cls,before,rd32(pl+0xc),result);
    } else {
        ((void (*)(const uint8_t *, int))(uintptr_t)(b + 0x2ceab0))(sel, 9); ((void (*)(const uint8_t *, int))(uintptr_t)(b + 0x2ceab0))(use, 9);
    }
    LOG("DM_TEST_CAST: frame %llu skill %d on monster %u (class %u, %ld cells^2 away)%s", (unsigned long long)f, s, best, cls, bd, left ? " left" : "");
}
#endif


void dm_dmgnum_pump(void) {
#ifndef DM_PLAYER_BUILD
    dm_combat_fixture_cast(0,0);
#endif
    if (!atomic_load_explicit(&g_on, memory_order_relaxed)) return;
    int dm_world_unit_head(uint32_t id, float *xy);
    static unsigned epoch;
    uint64_t now = guest_ns();
    unsigned ep = atomic_load(&g_epoch);
    unsigned head = atomic_load_explicit(&g_head, memory_order_acquire), tail = atomic_load_explicit(&g_tail, memory_order_relaxed);
    if (ep != epoch) {
        unsigned r = atomic_load(&g_resume); epoch = ep;
        tail = r - tail <= head - tail ? r : head;
        memset(g_live, 0, sizeof g_live);
    }
    int mode = mode_now();
    for (; tail != head; tail++) {
        const hit_t *h = &g_ring[tail % RING];
        int kind; float xy[2]; int32_t v = dm_dmgnum_split(h->share, mode, &kind);
        if (!v || !dm_world_unit_head(h->unit, xy)) continue;
        static uint32_t seq;
        live_t *slot = &g_live[0];
        for (int i = 0; i < LIVE; i++) { if (!g_live[i].unit) { slot = &g_live[i]; break; } if ((int32_t)(g_live[i].seq - slot->seq) < 0) slot = &g_live[i]; }
        *slot = (live_t){ .unit = h->unit, .seq = seq++, .value = v, .kind = (uint8_t)kind, .flags = (uint8_t)(h->flags | (mode == MODE_SIMPLE ? F_WHITE : 0)), .born = now, .x = xy[0], .y = xy[1] };
    }
    atomic_store_explicit(&g_tail, tail, memory_order_release);
    int n = 0; static float out[LIVE * FIELDS];
    for (int i = 0; i < LIVE; i++) {
        live_t *l = &g_live[i]; if (!l->unit) continue;
        uint64_t age = now - l->born; if (age >= LIFE_NS) { l->unit = 0; continue; }
        float xy[2]; if (dm_world_unit_head(l->unit, xy)) { l->x = xy[0]; l->y = xy[1]; }
        int cluster = i; for (int j = 0; j < i; j++) if (g_live[j].unit == l->unit) { cluster = j; break; }
        float *o = &out[n++ * FIELDS]; o[0] = l->x; o[1] = l->y; o[2] = (float)age / (float)LIFE_NS; o[3] = (float)(cluster | l->flags << 5);
        o[4] = (float)(l->seq & 0xffffff); o[5] = (float)l->kind; o[6] = (float)l->value; o[7] = (float)(l->unit & 0xffffff);
    }
    pthread_mutex_lock(&g_pub_lock);
    if (epoch == atomic_load(&g_epoch)) { memcpy(g_pub, out, (size_t)n * FIELDS * sizeof(float)); g_npub = n; }
    pthread_mutex_unlock(&g_pub_lock);
}








int dm_dmgnum_copy(float *out, int max) {
    int off = mode_now() == MODE_OFF;
    if (off || !atomic_load_explicit(&g_on, memory_order_relaxed)) {
        if (off) { if (atomic_exchange(&g_on, 0)) LOG("damage numbers: off (Custom page)"); }
        else {
            if (!base()) return 0;
            static int checked; if (!checked) { checked = 1; if (!gate_svc_present(SVC_HOST_DAMAGE)) LOG("damage numbers: this pack has no svc %#x, direct hits show nothing", SVC_HOST_DAMAGE); }
            atomic_store(&g_resume, atomic_load(&g_head)); atomic_fetch_add(&g_epoch, 1); atomic_store(&g_on, 1);
        }
        pthread_mutex_lock(&g_pub_lock); g_npub = 0; pthread_mutex_unlock(&g_pub_lock);
        if (off) return 0;
    }
    pthread_mutex_lock(&g_pub_lock);
    int n = g_npub < max ? g_npub : max;
    memcpy(out, g_pub, (size_t)n * FIELDS * sizeof(float));
    pthread_mutex_unlock(&g_pub_lock);
    return n;
}
