#ifndef DM_PLAYER_BUILD
#include <unistd.h>

enum { COMBAT_RING = 4096 };
typedef struct {
    unsigned seq, epoch, tick, attacker_type, attacker_class, attacker_id;
    unsigned target_type, target_class, target_id, result;
    int has_attacker, level, difficulty, total, life_before, life_after, share[NE];
} combat_event;
static combat_event combat_ring[COMBAT_RING];
static atomic_uint combat_head, combat_tail, combat_lost;
static atomic_int combat_reader;
#include "dm_combat_probe.inc"

void dm_combat_log_start(void) { atomic_store(&combat_reader,1); }

static void combat_capture(const uint64_t *regs) {
    static int enabled=-1;
    if(enabled<0) {
        const char *e=getenv("DM_TEST_COMBAT_LOG");
        enabled=e && !strcmp(e,"1");
        if(enabled && (getenv("DM_TEST_MON_HP") || getenv("DM_TEST_SLAY") || getenv("DM_TEST_MON_KILL"))) {
            LOG("ASSUME combat.measurement_setup: forced monster death invalidates capture");
            enabled=0;
        }
    }
    if(!enabled)return;
    if(!atomic_load(&combat_reader)) {
        LOG("ASSUME combat.event_reader: capture requires the qa state writer");
        enabled=0;return;
    }
    uint64_t damage=regs[19],target=regs[20],attacker=regs[21],game=regs[22];
    if(!damage || !target || !game || (int)regs[1]!=6)return;
    static uint64_t last_game;
    static unsigned epoch,seq,last_tick;
    unsigned tick=rd32(game+0x170);
    if(game!=last_game || tick<last_tick)epoch++;
    last_game=game;last_tick=tick;
    unsigned head=atomic_load_explicit(&combat_head,memory_order_relaxed);
    unsigned serial=++seq;
    if(head-atomic_load_explicit(&combat_tail,memory_order_acquire)>=COMBAT_RING) {
        atomic_fetch_add_explicit(&combat_lost,1,memory_order_relaxed);return;
    }
    int (*get)(uint64_t,int,int)=(int (*)(uint64_t,int,int))(uintptr_t)(base()+0x5b9290);
    combat_event *e=&combat_ring[head%COMBAT_RING];
    *e=(combat_event){.seq=serial,.epoch=epoch,.tick=tick,
        .has_attacker=attacker!=0,.attacker_type=attacker?rd32(attacker):0,
        .attacker_class=attacker?rd32(attacker+4):0,.attacker_id=attacker?rd32(attacker+8):0,
        .target_type=rd32(target),.target_class=rd32(target+4),.target_id=rd32(target+8),
        .result=rd32(damage+4)&0xffff,.level=get(target,12,0),
        .difficulty=*(const uint8_t *)(uintptr_t)(game+0x104),
        .total=(int32_t)rd32(damage+0x134),.life_before=get(target,6,0),.life_after=(int32_t)regs[2]};
    for(int i=0;i<NE;i++)e->share[i]=(int32_t)rd32(damage+k_share_at[i]);
    atomic_store_explicit(&combat_head,head+1,memory_order_release);
}


#define COMBAT_LOG(fmt, ...) fprintf(stderr, "[dm] " fmt "\n", __VA_ARGS__)

void dm_combat_log_drain(void) {
    probe_drain();
    static int header;
    static unsigned last_seq,last_lost;
    unsigned tail=atomic_load_explicit(&combat_tail,memory_order_relaxed);
    unsigned head=atomic_load_explicit(&combat_head,memory_order_acquire);
    unsigned count=head-tail;
    if(tail!=head && !header) {header=1;COMBAT_LOG("[combat] {\"schema\":1,\"pid\":%d,\"scope\":\"direct_life_write\",\"units\":256,\"fps\":25,\"elements\":[\"physical\",\"fire\",\"cold\",\"lightning\",\"poison\",\"magic\"]}",getpid());}
    for(;tail!=head;tail++) {
        const combat_event *e=&combat_ring[tail%COMBAT_RING];
        last_seq=e->seq;
        char attacker[80];
        if(e->has_attacker)snprintf(attacker,sizeof attacker,"[%u,%u,%u]",e->attacker_type,e->attacker_class,e->attacker_id);
        else strcpy(attacker,"null");
        COMBAT_LOG("[combat] {\"seq\":%u,\"epoch\":%u,\"tick\":%u,\"attacker\":%s,\"target\":[%u,%u,%u],\"level\":%d,\"difficulty\":%d,\"skill\":null,\"source\":null,\"outcome\":\"hit\",\"before\":null,\"after\":[%d,%d,%d,%d,%d,%d],\"total\":%d,\"life\":[%d,%d],\"result\":%u,\"crit_or_deadly\":%s,\"crushing_blow\":null,\"open_wounds\":null}",
            e->seq,e->epoch,e->tick,attacker,
            e->target_type,e->target_class,e->target_id,e->level,e->difficulty,
            e->share[0],e->share[1],e->share[2],e->share[3],e->share[4],e->share[5],
            e->total,e->life_before,e->life_after,e->result,e->result&RESULT_CRIT?"true":"false");
    }
    atomic_store_explicit(&combat_tail,tail,memory_order_release);
    unsigned lost=atomic_load_explicit(&combat_lost,memory_order_relaxed);
    if(lost!=last_lost)LOG("ASSUME combat.event_overflow: lost %u direct events",lost);
    if(count || lost!=last_lost)COMBAT_LOG("[combat] {\"drained\":%u,\"last_seq\":%u,\"lost\":%u}",head,last_seq,lost);
    last_lost=lost;
}
#undef COMBAT_LOG
#endif
