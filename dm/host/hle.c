

#include "dm.h"
#include "dm_sect.h"
#include <dlfcn.h>
#include <time.h>

typedef struct { const char *name; void *fn; } hle_ent;



#define MAX_STUB 4096
static unsigned g_nstub;
static const char *g_stub_name[MAX_STUB];
static uint8_t g_stub_seen[MAX_STUB];
extern uint64_t unimpl_stubs[MAX_STUB];

uint64_t unimpl_c(unsigned idx, uint64_t *regs) {
    if (!g_stub_seen[idx]++)
        LOG("UNIMPL %s (x0=%#llx x1=%#llx x2=%#llx)", g_stub_name[idx], regs[0], regs[1], regs[2]);
    return 0;
}



static uint64_t g_cache[MAX_MOD][8192];

uint64_t resolve_import(module_t *m, const char *name, int weak) {
    static const hle_ent *sec; static unsigned long secsz;
    if (!sec) DM_SECT_GET(swhle, const hle_ent, sec, secsz);
    for (unsigned i = 0; i < secsz / sizeof(hle_ent); i++)
        if (!strcmp(sec[i].name, name)) {
            if (g_flag.imports) { Dl_info di; dladdr(sec[i].fn, &di); LOG("import %s -> %s", name, di.dli_sname); }
            void *hlecount_wrap(const char *, const char *, void *); return (uint64_t)hlecount_wrap("HLE", name, sec[i].fn);
        }


    if (weak) return 0;


    void *hlecount_wrap(const char *, const char *, void *);
    for (unsigned k = 0; k < g_nstub; k++)
        if (!strcmp(g_stub_name[k], name)) return (uint64_t)hlecount_wrap("STUB", name, (void *)&unimpl_stubs[k]);
    if (g_nstub >= MAX_STUB) die("stub overflow");
    unsigned k = g_nstub++;
    g_stub_name[k] = name;
    (void)m;
    return (uint64_t)hlecount_wrap("STUB", name, (void *)&unimpl_stubs[k]);
}

void stubs_report(void) { LOG("stubs=%u", g_nstub); }







enum { G_SVC, G_CNTPCT, G_CNTVCT, G_OBS };


static void (*g_obs_fn[OBS_MAX])(const uint64_t *regs); static int g_obs_have[OBS_MAX];
void gate_obs_set(int id, void (*fn)(const uint64_t *regs)) { if (id <= 0 || id >= OBS_MAX) die("观测门编号越界 %d", id); g_obs_fn[id] = fn; }
int gate_obs_present(int id) { return id > 0 && id < OBS_MAX && g_obs_have[id]; }
static uint8_t g_svc_have[256];
int gate_svc_present(int num) { return num >= 0 && num < 256 && g_svc_have[num]; }
typedef struct { uint64_t ret; int kind, arg; uint64_t orig; } gate_t;
#define GATE_BITS 12
static gate_t g_gate[1u << GATE_BITS];
static int g_ngate;
static inline unsigned gate_h(uint64_t a) { return (unsigned)(((a >> 2) * 0x9E3779B97F4A7C15ull) >> (64 - GATE_BITS)); }

void gate_register(uint64_t ret_abs, const char *kind, int arg, uint64_t orig) {
    int k = !strcmp(kind, "svc") ? G_SVC : !strcmp(kind, "cntpct") ? G_CNTPCT : !strcmp(kind, "cntvct") ? G_CNTVCT : !strcmp(kind, "obs") ? G_OBS : -1;
    if (k < 0) die("未知门类 %s", kind);
    if (k == G_OBS) { if (arg <= 0 || arg >= OBS_MAX) die("观测门编号越界 %d", arg); g_obs_have[arg] = 1; }
    if (k == G_SVC && arg >= 0 && arg < 256) g_svc_have[arg] = 1;
    if (++g_ngate > (1 << GATE_BITS) / 4) die("gate overflow");
    unsigned i = gate_h(ret_abs);
    while (g_gate[i].ret) { if (g_gate[i].ret == ret_abs) die("门重复登记 %#llx", ret_abs); i = (i + 1) & ((1u << GATE_BITS) - 1); }
    g_gate[i] = (gate_t){ret_abs, k, arg, orig};
}

uint64_t svc_dispatch(int num, uint64_t *regs, uint64_t orig_addr);

uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs) {
    for (unsigned i = gate_h(ret_addr);; i = (i + 1) & ((1u << GATE_BITS) - 1)) {
        const gate_t *g = &g_gate[i];
        if (g->ret == ret_addr) switch (g->kind) {
            case G_CNTPCT: case G_CNTVCT: {
                uint64_t t = guest_ns() * 12 / 625;
                regs[g->arg] = t;
                return t;
            }
            case G_OBS: if (g_obs_fn[g->arg]) g_obs_fn[g->arg](regs); return 0;
            default: return svc_dispatch(g->arg, regs, g->orig);
        }
        if (!g->ret) die("unknown gate at %#llx", ret_addr);
    }
}
