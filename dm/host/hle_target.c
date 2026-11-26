











#include "dm.h"
#include "game_addrs.h"
#include <TargetConditionals.h>
#include <mach/mach.h>
#include <math.h>

enum { UNIT_MONSTER = 1, TF_SUPERUNIQUE = 0x02, TF_CHAMPION = 0x04, TF_UNIQUE = 0x08 };


static const float RANK_BONUS[3] = {0, 1000, 2000};


static const uint16_t BOSS_CLASS[] = {156, 211, 229, 242, 243, 250, 256, 267, 333, 365, 526, 544, 546, 547, 548, 549, 550,
                                      559, 570, 704, 705, 706, 707, 708, 709, 745, 746, 747};

static int read_live(uint64_t a, void *out, size_t n) {
    vm_size_t got = 0;
    return a >= 0x1000 && a <= UINT64_MAX - n &&
           vm_read_overwrite(mach_task_self(), a, n, (vm_address_t)out, &got) == KERN_SUCCESS && got == n;
}



static int unit_rank(uint64_t unit, uint32_t *cls, uint8_t *flags) {
    uint32_t head[2]; uint64_t data = 0;


    if (!read_live(unit, head, sizeof head) || head[0] != UNIT_MONSTER) return -1;
    *cls = head[1];
    for (size_t i = 0; i < sizeof BOSS_CLASS / sizeof *BOSS_CLASS; i++) if (BOSS_CLASS[i] == head[1]) return 2;
    if (!read_live(unit + 0x10, &data, 8) || !read_live(data + 0x1a, flags, 1)) return -1;
    return (*flags & (TF_SUPERUNIQUE | TF_CHAMPION | TF_UNIQUE)) ? 1 : 0;
}


int dm_target_rank(uint64_t unit) { uint32_t cls = 0; uint8_t flags = 0; return unit_rank(unit, &cls, &flags); }





int dm_threat_tier(uint64_t unit) {
    uint32_t cls = 0; uint8_t flags = 0; int r = unit_rank(unit, &cls, &flags);
    return r < 0 ? -1 : r == 2 ? 3 : (flags & (TF_SUPERUNIQUE | TF_UNIQUE)) ? 2 : (flags & TF_CHAMPION) ? 1 : 0; }

static FILE *g_log; static unsigned g_nlog;
static void target_rank_obs(const uint64_t *regs) {


    float *s0 = (float *)(void *)&regs[32];
    uint32_t cls = 0; uint8_t flags = 0;
    int rank = regs[1] ? unit_rank(regs[1], &cls, &flags) : -1;
    float before = *s0;
    if (rank > 0 && isfinite(before)) *s0 = before + RANK_BONUS[rank];
    if (g_log && g_nlog < 4096) {
        g_nlog++;
        fprintf(g_log, "{\"unit\":\"%#llx\",\"kind\":%u,\"class\":%u,\"flags\":%u,\"rank\":%d,\"score\":%.6g,\"adjusted\":%.6g}\n",
                (unsigned long long)regs[1], (unsigned)regs[2], cls, flags, rank, before, *s0);
        fflush(g_log);
    }
}

void target_rank_install(void) {
    if (!gate_obs_present(OBS_TARGET_RANK)) {
#if DM_PHONE
        LOG("【红】产物没有观测门 OBS_TARGET_RANK：自动选怪（Boss > 精英 > 普通）不工作，退回原版选目标");
#endif
        return;
    }
    const char *lg = getenv("DM_TARGET_LOG");
    if (lg && !(g_log = fopen(lg, "w"))) LOG("【红】DM_TARGET_LOG 打不开 %s", lg);
    gate_obs_set(OBS_TARGET_RANK, target_rank_obs);
    LOG("自动选怪：Boss > 精英 > 普通（观测门 %d）%s%s", OBS_TARGET_RANK, lg ? "，候选记录 → " : "", lg ? lg : "");
}
