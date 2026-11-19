#ifndef DM_PLAYER_BUILD
#include <math.h>

enum { PROBE_DEPTH=16, PROBE_RING=4096, PROBE_LISTS=16 };
enum { PC_REDUCTION=1, PC_CHANCE, PC_MELEE, PC_MISSILE, PC_CB, PC_OW, PC_REGEN, PC_DIRECT, PC_POISON, PC_BURN, PC_SKILL, PC_SUMMON, PC_UNKNOWN, PC_REACTION, PC_ANIMATION, PC_DEATH, PC_PHYSICAL_ROLL, PC_DECISION, PC_SCHEDULE };
enum { PP_ENTER, PP_EXIT, PP_ROLL, PP_APPLY, PP_WRITE, PP_OUTCOME };
typedef struct {
    uint64_t game,attacker,target,damage,missile,pending_list;
    unsigned call,parent;
    int kind,skill,skill_level,refresh,chance_inputs[7],animation[5],decision[10];
} probe_context;
typedef struct { unsigned state,type,id,flags;int rate,contributes,skill,level,source;float expires; } probe_list;
typedef struct {
    unsigned seq,epoch,tick,call,parent,attacker[3],target[3],missile[3];
    int kind,phase,source,skill,skill_level,origin_retained,origin[4],damage[8],resists[6],life[2],value[4],regen,nlist,chance_inputs[7],animation[5],positions[4],decision[10];
    probe_list lists[PROBE_LISTS];
} probe_event;
static probe_event probe_ring[PROBE_RING];
static unsigned probe_head,probe_tail,probe_lost,probe_errors,probe_calls,probe_seq,probe_epoch,probe_tick;
static uint64_t probe_game;
static pthread_mutex_t probe_lock=PTHREAD_MUTEX_INITIALIZER;
static _Thread_local probe_context probe_stack[PROBE_DEPTH];
static _Thread_local unsigned probe_depth,probe_excess;
static atomic_int probe_enabled;
typedef struct {uint64_t game,unit;unsigned id,cls;int skill,level,owner_type,owner_id,source;} probe_origin;
static probe_origin probe_origins[4096];
static unsigned probe_norigin;
static int probe_decision_class=-1;
typedef struct {
    uint64_t game,list;unsigned target,state,type,id,expiry;int rate,skill,level,source;
} probe_effect;
static probe_effect probe_effects[4096];
static unsigned probe_neffect;
static void probe_owner(uint64_t unit,int out[4]) {
    for(int k=0;k<4;k++)out[k]=-1;
    if(!unit)return;
    if(rd32(unit)==0){out[0]=0;out[1]=(int)rd32(unit+8);return;}
    if(rd32(unit)!=1 || !owned(unit))return;
    uint64_t data=rd(unit+0x10),control=data?rd(data+0x38):0;
    if(control){out[0]=(int)rd32(control+0x6c);out[1]=(int)rd32(control+0x68);}
}

static void probe_error(void) { pthread_mutex_lock(&probe_lock);probe_errors++;pthread_mutex_unlock(&probe_lock); }
static int probe_stat(uint64_t unit,int stat) {
    return unit?((int (*)(uint64_t,int,int))(uintptr_t)(base()+0x5b9290))(unit,stat,0):0;
}
static void probe_unit(uint64_t unit,unsigned out[3]) {
    if(unit){out[0]=rd32(unit);out[1]=rd32(unit+4);out[2]=rd32(unit+8);}
    else out[0]=out[1]=out[2]=UINT32_MAX;
}
static void probe_position(uint64_t unit,int out[2]) {
    out[0]=out[1]=INT32_MIN;
    if(!unit || rd32(unit)>1)return;
    uint64_t path=rd(unit+0x38);
    if(path){out[0]=*(uint16_t *)(uintptr_t)(path+2);out[1]=*(uint16_t *)(uintptr_t)(path+6);}
}
static int probe_source(uint64_t unit) {
    if(!unit)return -1;
    unsigned type=rd32(unit);
    if(type==0)return 0;
    if(type!=1)return -1;
    if(!owned(unit))return 3;
    unsigned ver=*(uint8_t *)(uintptr_t)(unit+0x1bd);
    if(ver<1 || ver>3)return -1;
    uint64_t table=rd((uint64_t)(uintptr_t)(base()+0x2d64a18+ver*16)),rows=table?rd(table+0x558):0,count=table?rd(table+0x560):0;
    if(!rows || count>4096)return -1;
    for(uint64_t k=0;k<count;k++)if(rd32(rows+k*0x150+8)==rd32(unit+4))return 1;
    return 2;
}
static probe_context probe_context_make(int kind,uint64_t game,uint64_t attacker,uint64_t target,uint64_t damage) {
    probe_context c={.game=game,.attacker=attacker,.target=target,.damage=damage,.kind=kind,.skill=-1,.skill_level=-1};
    for(int k=0;k<7;k++)c.chance_inputs[k]=INT32_MIN;
    for(int k=0;k<10;k++)c.decision[k]=INT32_MIN;
    if(probe_depth) {
        const probe_context *p=&probe_stack[probe_depth-1];c.parent=p->call;
        if(!c.game)c.game=p->game;
        if(damage && damage==p->damage && attacker==p->attacker) {
            c.missile=p->missile;c.skill=p->skill;c.skill_level=p->skill_level;
        }
    }
    for(unsigned k=probe_depth;k>0;k--) {
        const probe_context *p=&probe_stack[k-1];
        if(p->kind==PC_SKILL || p->kind==PC_REACTION) {
            if(attacker && p->attacker==attacker){c.skill=p->skill;c.skill_level=p->skill_level;c.missile=p->missile;}
            break;
        }
        if(attacker && p->attacker==attacker && p->missile && (!damage || p->damage==damage)) {
            c.skill=p->skill;c.skill_level=p->skill_level;c.missile=p->missile;break;
        }
    }
    pthread_mutex_lock(&probe_lock);
    if(c.game) {
        unsigned tick=rd32(c.game+0x170);
        if(c.game!=probe_game || tick<probe_tick){probe_epoch++;probe_norigin=probe_neffect=0;}
        probe_game=c.game;probe_tick=tick;
    }
    c.call=++probe_calls;pthread_mutex_unlock(&probe_lock);
    return c;
}
static void probe_emit(const probe_context *c,int phase,const int value[4],int life_after,int lists) {
    if(!probe_enabled)return;
    if(!atomic_load(&combat_reader)){probe_error();return;}
    probe_event e={.call=c->call,.parent=c->parent,.kind=c->kind,.phase=phase,.source=probe_source(c->attacker),
        .skill=c->skill,.skill_level=c->skill_level,.tick=c->game?rd32(c->game+0x170):UINT32_MAX};
    probe_owner(c->attacker,e.origin);
    pthread_mutex_lock(&probe_lock);
    for(unsigned k=0;c->attacker && rd32(c->attacker)==1 && k<probe_norigin;k++) {
        const probe_origin *origin=&probe_origins[k];
        if(origin->game!=c->game || origin->unit!=c->attacker || origin->id!=rd32(c->attacker+8) || origin->cls!=rd32(c->attacker+4))continue;
        e.origin[2]=origin->skill;e.origin[3]=origin->level;
        if(e.origin[0]<0 && e.origin[1]<0 && origin->owner_type>=0 && origin->owner_id>=0 && (origin->source==1 || origin->source==2)) {
            e.origin[0]=origin->owner_type;e.origin[1]=origin->owner_id;
            e.source=origin->source;e.origin_retained=1;
        }
        break;
    }
    pthread_mutex_unlock(&probe_lock);
    probe_unit(c->attacker,e.attacker);probe_unit(c->target,e.target);probe_unit(c->missile,e.missile);
    probe_position(c->attacker,e.positions);probe_position(c->target,e.positions+2);
    e.life[0]=probe_stat(c->target,6);e.life[1]=life_after;
    if(value)memcpy(e.value,value,sizeof e.value);
    memcpy(e.chance_inputs,c->chance_inputs,sizeof e.chance_inputs);
    memcpy(e.animation,c->animation,sizeof e.animation);
    memcpy(e.decision,c->decision,sizeof e.decision);
    if(c->damage){for(int k=0;k<NE;k++)e.damage[k]=(int32_t)rd32(c->damage+k_share_at[k]);e.damage[6]=(int32_t)rd32(c->damage+0x24);e.damage[7]=(int32_t)rd32(c->damage+0x134);}
    if(c->kind==PC_REDUCTION && c->damage) {
        e.value[0]=(int)(rd32(c->damage+4)&0xffff);
        e.value[1]=(int)rd32(c->damage+0x144);
    }
    const int res[]={36,39,43,41,45,37};
    for(int k=0;k<NE;k++)e.resists[k]=probe_stat(c->target,res[k]);
    e.regen=probe_stat(c->target,74);
    if(lists && c->target) {
        uint64_t root=rd(c->target+0x88);
        if(root && (rd32(root+0x1c)>>31)) {
            uint64_t node=rd(root+0xd0);unsigned walked=0;
            unsigned ver=*(uint8_t *)(uintptr_t)(c->target+0x1bd);
            for(;node && walked<256;node=rd(node+0x68),walked++) {
                if(ver<1 || ver>3){probe_error();break;}
                int rate=((int (*)(int,uint64_t,int,int))(uintptr_t)(base()+0x5bc690))(ver,node,74,0);
                if(!rate)continue;
                if(e.nlist==PROBE_LISTS){probe_error();break;}
                probe_list *l=&e.lists[e.nlist++];
                *l=(probe_list){.state=rd32(node+0x20),.type=rd32(node+8),.id=rd32(node+0xc),.flags=rd32(node+0x1c),.rate=rate,.skill=-1,.level=-1,.source=-1};
                l->contributes=-1;
                uint64_t table=rd((uint64_t)(uintptr_t)(base()+0x2d64a18+ver*16));
                uint64_t stats=table?rd(table+0x1290):0;int count=table?(int32_t)rd32(table+0x1298):0;
                if(rd(node)==c->target && rd(node+0x78)==root && stats && count>74 && count<=4096) {
                    unsigned statflags=*(uint8_t *)(uintptr_t)(stats+74*0x144+4);
                    l->contributes=!(l->flags&0x2000) && !((l->flags&0x40000000) && (statflags&4));
                }
                memcpy(&l->expires,(void *)(uintptr_t)(node+0x24),sizeof l->expires);
                if(!isfinite(l->expires)){probe_error();l->expires=0;}
                pthread_mutex_lock(&probe_lock);
                for(unsigned k=0;k<probe_neffect;k++) {
                    const probe_effect *effect=&probe_effects[k];
                    if(effect->game==c->game && effect->list==node && effect->target==rd32(c->target+8) &&
                        effect->state==l->state && effect->type==l->type && effect->id==l->id && effect->rate==rate && effect->expiry==rd32(node+0x24)) {
                        l->skill=effect->skill;l->level=effect->level;l->source=effect->source;break;
                    }
                }
                pthread_mutex_unlock(&probe_lock);
            }
            if(node && walked==256)probe_error();
        }
    }
    pthread_mutex_lock(&probe_lock);
    e.epoch=probe_epoch;e.seq=++probe_seq;
    if(probe_head-probe_tail==PROBE_RING)probe_lost++;
    else probe_ring[probe_head++%PROBE_RING]=e;
    pthread_mutex_unlock(&probe_lock);
}
static void probe_enter(probe_context c,const int value[4]) {
    if(probe_depth==PROBE_DEPTH){probe_excess++;probe_error();return;}
    probe_stack[probe_depth++]=c;probe_emit(&c,PP_ENTER,value,INT32_MIN,0);
}
static void probe_leave(int kind,const uint64_t *r) {
    if(probe_excess){probe_excess--;return;}
    if(!probe_depth || probe_stack[probe_depth-1].kind!=kind){probe_error();probe_depth=0;return;}
    probe_context c=probe_stack[--probe_depth];
    probe_emit(&c,PP_EXIT,(int[4]){(int)r[0],0,0,0},INT32_MIN,0);
}
static probe_context *probe_top(int kind) {
    if(probe_excess || !probe_depth || probe_stack[probe_depth-1].kind!=kind){probe_error();return NULL;}
    return &probe_stack[probe_depth-1];
}
static void probe_track_effect(const probe_context *c,uint64_t list) {
    if(!list || !c->target || rd(list)!=c->target || rd(list+0x78)!=rd(c->target+0x88))return;
    unsigned ver=*(uint8_t *)(uintptr_t)(c->target+0x1bd);if(ver<1 || ver>3){probe_error();return;}
    probe_effect effect={.game=c->game,.list=list,.target=rd32(c->target+8),.state=rd32(list+0x20),
        .type=rd32(list+8),.id=rd32(list+0xc),.expiry=rd32(list+0x24),
        .rate=((int (*)(int,uint64_t,int,int))(uintptr_t)(base()+0x5bc690))(ver,list,74,0),
        .skill=c->skill,.level=c->skill_level,.source=probe_source(c->attacker)};
    pthread_mutex_lock(&probe_lock);unsigned k=0;
    for(;k<probe_neffect;k++)if(probe_effects[k].game==c->game && probe_effects[k].list==list)break;
    if(k<sizeof probe_effects/sizeof *probe_effects){probe_effects[k]=effect;if(k==probe_neffect)probe_neffect++;}
    else probe_errors++;
    pthread_mutex_unlock(&probe_lock);
}
static void probe_reduction_in(const uint64_t *r){probe_enter(probe_context_make(PC_REDUCTION,r[0],r[1],r[2],r[3]),NULL);}
static void probe_reduction_out(const uint64_t *r){probe_leave(PC_REDUCTION,r);}
static void probe_physical_in(const uint64_t *r) {
    uint64_t game=r[20]?rd(r[20]+0xd8):0;
    if(!game || !r[22] || game!=rd(r[22]+0xd8)){probe_error();game=0;}
    probe_context c=probe_context_make(PC_PHYSICAL_ROLL,game,r[20],r[22],r[21]);
    probe_enter(c,(int[4]){(int)rd32(r[21]+0x18),(int)rd32(r[21]+0x1c),(int)(r[26]&255),0});
}
static probe_context *probe_physical_current(const uint64_t *r) {
    if(probe_excess || !probe_depth || probe_stack[probe_depth-1].kind!=PC_PHYSICAL_ROLL)return NULL;
    probe_context *c=&probe_stack[probe_depth-1];
    if(c->attacker!=r[21]){probe_error();return NULL;}
    return c;
}
static void probe_physical_bounds(const uint64_t *r) {
    probe_context *c=probe_physical_current(r);if(!c)return;
    c->refresh=1;
    probe_emit(c,PP_APPLY,(int[4]){(int)r[8],(int)r[9],(int)r[20],(int)(r[19]&255)},INT32_MIN,0);
}
static void probe_physical_out(const uint64_t *r) {
    if(probe_excess){probe_excess--;return;}
    probe_context *c=probe_physical_current(r);if(!c)return;
    probe_emit(c,PP_EXIT,(int[4]){(int)r[0],(int)r[20],c->refresh,0},INT32_MIN,0);
    probe_depth--;
}
static void probe_chance_in(const uint64_t *r) {
    probe_context c=probe_context_make(PC_CHANCE,0,r[0],r[1],0);
    if(r[0] && r[1] && rd32(r[0])==1 && rd32(r[1])==0) {
        unsigned ver=*(uint8_t *)(uintptr_t)(r[0]+0x1bd);
        uint64_t root=rd(r[0]+0x88);
        if(ver<1 || ver>3 || !root)probe_error();
        else {
            c.chance_inputs[0]=probe_stat(r[0],12);c.chance_inputs[1]=probe_stat(r[1],12);
            c.chance_inputs[2]=probe_stat(r[0],19);c.chance_inputs[3]=probe_stat(r[0],2);
            c.chance_inputs[4]=((int (*)(int,uint64_t,int,int))(uintptr_t)(base()+0x5bc810))(ver,root,119,0);
            c.chance_inputs[5]=((int (*)(uint64_t))(uintptr_t)(base()+0x5cd320))(r[1]);
            c.chance_inputs[6]=probe_stat(r[1],(uint32_t)r[3]?32:33);
        }
    }
    probe_enter(c,(int[4]){(int)r[2],(int)r[3],0,0});
}
static void probe_chance_out(const uint64_t *r){probe_leave(PC_CHANCE,r);}
static void probe_chance_roll(const uint64_t *r){probe_context *c=probe_top(PC_CHANCE);if(c)probe_emit(c,PP_ROLL,(int[4]){(int)r[9],(int)r[8],0,0},INT32_MIN,0);}
static void probe_melee_in(const uint64_t *r){probe_enter(probe_context_make(PC_MELEE,r[0],r[1],r[2],0),(int[4]){(int)r[3],(int)r[4],(int)r[5],0});}
static void probe_melee_out(const uint64_t *r){probe_leave(PC_MELEE,r);}
static void probe_block_roll(const uint64_t *r){probe_context *c=probe_top(PC_MELEE);if(c)probe_emit(c,PP_ROLL,(int[4]){(int)r[8],(int)r[22],1,0},INT32_MIN,0);}
static void probe_missile_in(const uint64_t *r) {
    uint64_t missile=r[1],owner=0;
    if(missile && rd32(missile)==3)owner=((uint64_t (*)(uint64_t,uint64_t))(uintptr_t)(base()+0x7cd7b0))(r[0],missile);
    probe_context c=probe_context_make(PC_MISSILE,r[0],owner,r[2],r[3]);c.missile=missile;
    uint64_t data=missile && rd32(missile)==3?rd(missile+0x10):0;
    if(data){c.skill=*(int16_t *)(uintptr_t)(data+0xc);c.skill_level=*(int16_t *)(uintptr_t)(data+0xe);}
    probe_enter(c,NULL);
}
static void probe_missile_out(const uint64_t *r){probe_leave(PC_MISSILE,r);}
static void probe_missile_result(const uint64_t *r){probe_context *c=probe_top(PC_MISSILE);if(c)probe_emit(c,PP_OUTCOME,(int[4]){(int)r[0],0,0,0},INT32_MIN,0);}
static void probe_missile_block_roll(const uint64_t *r) {
    probe_context *c=probe_top(PC_MISSILE);
    if(!c)return;
    if(c->target!=r[20]){probe_error();return;}
    probe_emit(c,PP_ROLL,(int[4]){(int)r[8],(int)r[21],1,0},INT32_MIN,0);
}
static void probe_cb_in(const uint64_t *r){probe_enter(probe_context_make(PC_CB,r[0],r[2],r[3],r[4]),(int[4]){(int)r[1],0,0,0});}
static void probe_cb_out(const uint64_t *r){probe_leave(PC_CB,r);}
static void probe_cb_roll(const uint64_t *r){probe_context *c=probe_top(PC_CB);if(c)probe_emit(c,PP_ROLL,(int[4]){(int)r[8],(int)r[0],0,0},INT32_MIN,0);}
static void probe_cb_life(const uint64_t *r){probe_context *c=probe_top(PC_CB);if(c)probe_emit(c,PP_WRITE,(int[4]){(int)r[22],0,0,0},(int)r[2],0);}
static void probe_ow_in(const uint64_t *r){probe_enter(probe_context_make(PC_OW,r[0],r[2],r[3],r[4]),(int[4]){(int)r[1],0,0,0});}
static void probe_ow_out(const uint64_t *r){probe_leave(PC_OW,r);}
static void probe_ow_roll(const uint64_t *r){probe_context *c=probe_top(PC_OW);if(c)probe_emit(c,PP_ROLL,(int[4]){(int)r[8],(int)r[0],0,0},INT32_MIN,0);}
static void probe_ow_apply(const uint64_t *r){probe_context *c=probe_top(PC_OW);if(c){probe_track_effect(c,r[0]);probe_emit(c,PP_APPLY,(int[4]){r[0]!=0,0,0,0},INT32_MIN,1);}}
static void probe_mon_regen(const uint64_t *r){probe_context c=probe_context_make(PC_REGEN,r[19],0,r[20],0);probe_emit(&c,PP_WRITE,NULL,(int)r[2],1);}
static void probe_unit_regen(const uint64_t *r){probe_context c=probe_context_make(PC_REGEN,r[21],0,r[20],0);probe_emit(&c,PP_WRITE,NULL,(int)r[2],1);}
static void probe_skill_in(const uint64_t *r) {
    probe_context c=probe_context_make(PC_SKILL,r[0],r[1],0,0);
    c.skill=(int)r[2];c.skill_level=(int)r[3];c.missile=0;
    probe_enter(c,(int[4]){(int)r[4],(int)r[5],(int)r[6],0});
}
static void probe_skill_out(const uint64_t *r){probe_leave(PC_SKILL,r);}
static void probe_reaction_in(const uint64_t *r) {
    probe_context c=probe_context_make(PC_REACTION,r[0],r[2],r[3],0);
    c.skill=c.skill_level=-1;c.missile=0;
    probe_enter(c,(int[4]){(int)r[1],(int)r[5],6,0});
}
static void probe_reaction_out(const uint64_t *r){probe_leave(PC_REACTION,r);}
static void probe_animation_enter(const uint64_t *r,int route) {
    uint64_t unit=r[1];probe_context c=probe_context_make(PC_ANIMATION,r[0],unit,0,0);
    c.skill=c.skill_level=-1;c.missile=0;
    if(unit) {
        int sequence=rd(unit+0x40)!=0;
        c.animation[0]=(int)rd32(unit+0xc);c.animation[1]=sequence;
        c.animation[2]=(int)rd32(unit+(sequence?0x48:0x64));
        c.animation[3]=sequence?(int)rd32(unit+0x54):*(int16_t *)(uintptr_t)(unit+0x68);
        c.animation[4]=(int)rd32(unit+0x12c);
        uint64_t list=rd(unit+0x100),node=list?rd(list+0x18):0,record=node?rd(node):0;
        if(record)c.skill=*(int16_t *)(uintptr_t)record;
    }
    probe_enter(c,(int[4]){route,route?(int)r[2]:0,route==2?(int)r[3]:0,0});
}
static void probe_animation_in(const uint64_t *r){probe_animation_enter(r,0);}
static void probe_restart_in(const uint64_t *r){probe_animation_enter(r,1);}
static void probe_sequence_in(const uint64_t *r){probe_animation_enter(r,2);}
static void probe_animation_event(const uint64_t *r) {
    probe_context *c=probe_top(PC_ANIMATION);if(!c)return;
    if(c->game!=r[0] || c->attacker!=r[1]){probe_error();return;}
    probe_emit(c,PP_APPLY,(int[4]){(int)r[2],(int)r[3],(int)r[5],(int)r[6]},INT32_MIN,0);
}
static void probe_animation_out(const uint64_t *r){(void)r;probe_leave(PC_ANIMATION,(uint64_t[1]){0});}
static void probe_animation_tail(const uint64_t *r){probe_animation_event(r);probe_animation_out(r);}
static int probe_decision_unit(uint64_t unit) {
    return probe_decision_class>=0 && unit && rd32(unit)==1 && rd32(unit+4)==(unsigned)probe_decision_class;
}
static int probe_code_offset(uint64_t address) {
    uint64_t offset=address-(uint64_t)(uintptr_t)base();
    for(int k=0;k<g_nmod;k++)if(g_mod[k].base==base() && offset<g_mod[k].text_end)return (int)offset;
    return INT32_MIN;
}
static void probe_decision_in(const uint64_t *r) {
    if(!probe_decision_unit(r[1]))return;
    if(!r[0] || rd(r[1]+0xd8)!=r[0])probe_error();
    probe_context c=probe_context_make(PC_DECISION,r[0],r[1],0,0);
    c.skill=c.skill_level=-1;c.missile=0;
    probe_enter(c,(int[4]){(int)rd32(r[1]+0xc),0,0,INT32_MIN});
}
static void probe_decision_call(const uint64_t *r,int site) {
    if(!probe_decision_unit(r[19]))return;
    probe_context *c=probe_top(PC_DECISION);if(!c)return;
    uint64_t input=r[2],data=rd(r[19]+0x10),control=data?rd(data+0x38):0;
    if(c->game!=r[20] || c->attacker!=r[19] || !input || !control || rd(input)!=control){probe_error();return;}
    c->target=rd(input+0x10);
    int values[10]={site,probe_code_offset(r[8]),site==OBS_COMBAT_DECISION_CALL1?probe_code_offset(r[3]):INT32_MIN,
        (int)rd32(input+0x20),(int)rd32(input+0x24),*(uint8_t *)(uintptr_t)(input+0x3c),
        (int)rd32(control+0x44),rd(control+0x50)!=0,(int)rd32(r[19]+0x28),(int)rd32(r[19]+0x2c)};
    memcpy(c->decision,values,sizeof values);
    if(values[1]==INT32_MIN || (site==OBS_COMBAT_DECISION_CALL1 && values[2]==INT32_MIN))probe_error();
    probe_emit(c,PP_APPLY,(int[4]){(int)rd32(r[19]+0xc),0,0,INT32_MIN},INT32_MIN,0);
}
static void probe_decision_call0(const uint64_t *r){probe_decision_call(r,OBS_COMBAT_DECISION_CALL0);}
static void probe_decision_call1(const uint64_t *r){probe_decision_call(r,OBS_COMBAT_DECISION_CALL1);}
static void probe_decision_call2(const uint64_t *r){probe_decision_call(r,OBS_COMBAT_DECISION_CALL2);}
static void probe_decision_out(const uint64_t *r) {
    if(!probe_decision_unit(r[19]))return;
    if(probe_excess){probe_excess--;return;}
    probe_context *top=probe_top(PC_DECISION);if(!top)return;
    probe_context c=*top;probe_depth--;
    int status=1,count=0,deadline=INT32_MIN;
    if(c.game!=r[20] || c.attacker!=r[19] || rd(r[19]+0xd8)!=r[20])status=0;
    uint64_t node=rd(r[19]+0x148),seen[64];unsigned walked=0;
    while(status && node) {
        if(walked==64){status=0;break;}
        for(unsigned k=0;k<walked;k++)if(seen[k]==node)status=0;
        if(!status || rd(node+8)!=r[19]){status=0;break;}
        seen[walked++]=node;
        unsigned type=*(uint8_t *)(uintptr_t)node,flags=*(uint16_t *)(uintptr_t)(node+2);
        if(type==2 && !(flags&9)){count++;deadline=(int)rd32(node+4);}
        node=rd(node+0x38);
    }
    if(!status || count>1){probe_error();status=0;}
    if(count!=1)deadline=INT32_MIN;
    probe_emit(&c,PP_EXIT,(int[4]){(int)rd32(r[19]+0xc),status,count,deadline},INT32_MIN,0);
}
static void probe_schedule(const uint64_t *r,int persistent) {
    if((unsigned)r[24]!=2 || !probe_decision_unit(r[23]))return;
    uint64_t node=r[0];
    int deadline=persistent?-1:(int)r[26];
    if(!r[19] || rd(r[23]+0xd8)!=r[19] || !node || *(uint8_t *)(uintptr_t)node!=2 ||
       rd(node+8)!=r[23] || (int)rd32(node+4)!=deadline){probe_error();return;}
    probe_context c=probe_context_make(PC_SCHEDULE,r[19],r[23],0,0);
    c.skill=c.skill_level=-1;c.missile=0;

    int flags=persistent?(uint16_t)r[8]:*(uint16_t *)(uintptr_t)(node+2);
    probe_emit(&c,PP_APPLY,(int[4]){2,deadline,persistent?OBS_COMBAT_SCHEDULE_PERSIST:OBS_COMBAT_SCHEDULE_FINITE,flags},INT32_MIN,0);
}
static void probe_schedule_finite(const uint64_t *r){probe_schedule(r,0);}
static void probe_schedule_persist(const uint64_t *r){probe_schedule(r,1);}
static void probe_death_in(const uint64_t *r) {
    probe_context c=probe_context_make(PC_DEATH,r[0],r[2],r[1],0);
    probe_enter(c,(int[4]){r[1]?(int)rd32(r[1]+0xc):-1,(int)r[3],0,0});
}
static void probe_death_out(const uint64_t *r) {
    (void)r;
    if(probe_excess){probe_excess--;return;}
    probe_context *c=probe_top(PC_DEATH);if(!c)return;
    probe_emit(c,PP_EXIT,(int[4]){c->target?(int)rd32(c->target+0xc):-1,0,0,0},probe_stat(c->target,6),0);
    probe_depth--;
}
static void probe_proc_stack_in(const uint64_t *r) {

    uint64_t args[7]={r[0],r[1],r[113],r[3],r[4],r[5],r[6]};probe_skill_in(args);
}
static void probe_missile_skill(const uint64_t *r,uint64_t missile,uint64_t target,const int *value) {
    uint64_t owner=missile && rd32(missile)==3?((uint64_t (*)(uint64_t,uint64_t))(uintptr_t)(base()+0x7cd7b0))(r[0],missile):0;
    probe_context c=probe_context_make(PC_SKILL,r[0],owner,target,0);
    c.missile=missile;c.skill=(int)r[2];c.skill_level=(int)r[3];
    probe_enter(c,value);
}
static void probe_missile_skill_in(const uint64_t *r){probe_missile_skill(r,r[1],0,NULL);}
static void probe_missile_stack_in(const uint64_t *r){probe_missile_skill(r,r[19],0,NULL);}
static void probe_missile_hit(const uint64_t *r,int area) {
    uint64_t missile=r[1],data=missile && rd32(missile)==3?rd(missile+0x10):0;
    uint64_t args[4]={r[0],missile,UINT64_MAX,UINT64_MAX};
    if(data){args[2]=(uint64_t)*(int16_t *)(uintptr_t)(data+0xc);args[3]=(uint64_t)*(int16_t *)(uintptr_t)(data+0xe);}

    uint64_t caller=r[101]-(uint64_t)(uintptr_t)base();
    int value[4]={area?67:65,(int)r[3],caller<0x4000000?(int)caller:INT32_MIN,missile?(int)rd32(missile+0xc):-1};
    probe_missile_skill(args,missile,area?0:r[2],value);
}
static void probe_missile_hit_in(const uint64_t *r){probe_missile_hit(r,0);}
static void probe_missile_area_in(const uint64_t *r){probe_missile_hit(r,1);}
static void probe_abyss_area_in(const uint64_t *r) {
    uint64_t missile=r[19],data=missile && rd32(missile)==3?rd(missile+0x10):0;
    uint64_t args[4]={r[20],missile,UINT64_MAX,UINT64_MAX};
    if(data){args[2]=(uint64_t)*(int16_t *)(uintptr_t)(data+0xc);args[3]=(uint64_t)*(int16_t *)(uintptr_t)(data+0xe);}
    probe_missile_skill(args,missile,0,(int[4]){OBS_COMBAT_ABYSS_AREA_IN,0,0,0});
}
static void probe_abyss_area_out(const uint64_t *r) {

    if(r[20] && r[19])probe_skill_out(r);
}
static void probe_unknown(const uint64_t *r) {
    for(unsigned k=0;k<probe_depth;k++){probe_stack[k].skill=probe_stack[k].skill_level=-1;probe_stack[k].missile=0;}
    probe_context c=probe_context_make(PC_UNKNOWN,r[0],0,0,0);c.skill=c.skill_level=-1;
    probe_emit(&c,PP_OUTCOME,(int[4]){(int)r[2],(int)r[3],0,0},INT32_MIN,0);
}
static void probe_summon(const uint64_t *r) {
    if(!r[0] || rd32(r[0])!=1)return;
    const probe_context *parent=NULL;
    for(unsigned k=probe_depth;k>0;k--)if(probe_stack[k-1].kind==PC_SKILL){parent=&probe_stack[k-1];break;}
    uint64_t creator_unit=r[21]?rd(r[21]+8):0;
    probe_context c=probe_context_make(PC_SUMMON,r[19],creator_unit,r[0],0);
    int creator[4];probe_owner(r[0],creator);
    if(parent && parent->attacker && creator[0]==(int)rd32(parent->attacker) && creator[1]==(int)rd32(parent->attacker+8)) {
        c.skill=parent->skill;c.skill_level=parent->skill_level;
    }else {c.skill=c.skill_level=-1;}
    int source=probe_source(r[0]);
    int owner_known=creator_unit && creator[0]>=0 && creator[0]<=1 && creator[0]==(int)rd32(creator_unit) && creator[1]==(int)rd32(creator_unit+8);
    pthread_mutex_lock(&probe_lock);
    unsigned k=0,id=rd32(r[0]+8);
    for(;k<probe_norigin;k++)if(probe_origins[k].game==c.game && probe_origins[k].id==id)break;
    if(k<sizeof probe_origins/sizeof *probe_origins) {
        probe_origins[k]=(probe_origin){.game=c.game,.unit=r[0],.id=id,.cls=rd32(r[0]+4),.skill=c.skill,.level=c.skill_level,
            .owner_type=owner_known?creator[0]:-1,.owner_id=owner_known?creator[1]:-1,.source=source};
        if(k==probe_norigin)probe_norigin++;
    }else probe_errors++;
    pthread_mutex_unlock(&probe_lock);
    probe_emit(&c,PP_APPLY,NULL,INT32_MIN,0);
}
static void probe_poison_in(const uint64_t *r) {

    probe_enter(probe_context_make(PC_POISON,r[22],r[0],r[1],r[19]),(int[4]){(int)r[2],(int)r[3],(int)r[4],(int)r[5]});
}
static void probe_candidate(int kind,const uint64_t *r,int refresh) {
    probe_context *c=probe_top(kind);if(c){c->pending_list=r[1];c->refresh=refresh;}
}
static void probe_poison_refresh(const uint64_t *r){probe_candidate(PC_POISON,r,1);}
static void probe_poison_new(const uint64_t *r){probe_candidate(PC_POISON,r,0);}
static void probe_application_done(int kind) {
    if(probe_excess){probe_excess--;return;}
    probe_context *current=probe_top(kind);if(!current)return;
    probe_context c=*current;probe_depth--;
    int accepted=0,rate=0;
    if(c.pending_list && c.target && rd(c.pending_list)==c.target && rd(c.pending_list+0x78)==rd(c.target+0x88)) {
        unsigned ver=*(uint8_t *)(uintptr_t)(c.target+0x1bd);
        if(ver>=1 && ver<=3){accepted=1;rate=((int (*)(int,uint64_t,int,int))(uintptr_t)(base()+0x5bc690))(ver,c.pending_list,74,0);}
        else probe_error();
    }
    if(accepted)probe_track_effect(&c,c.pending_list);
    probe_emit(&c,PP_APPLY,(int[4]){accepted,c.refresh,rate,0},INT32_MIN,1);
    probe_emit(&c,PP_EXIT,NULL,INT32_MIN,0);
}
static void probe_poison_done(const uint64_t *r){(void)r;probe_application_done(PC_POISON);}
static void probe_burn_in(const uint64_t *r) {
    if(probe_depth && probe_stack[probe_depth-1].kind==PC_POISON)probe_application_done(PC_POISON);
    probe_enter(probe_context_make(PC_BURN,r[22],r[21],r[20],r[19]),
        (int[4]){r[19]?(int)rd32(r[19]+0x24):0,r[19]?(int)rd32(r[19]+0x28):0,0,0});
}
static void probe_burn_refresh(const uint64_t *r){probe_candidate(PC_BURN,r,1);}
static void probe_burn_new(const uint64_t *r){probe_candidate(PC_BURN,r,0);}
static void probe_burn_out(const uint64_t *r){(void)r;probe_application_done(PC_BURN);}
static void probe_direct(const uint64_t *r) {
    if(!probe_enabled)return;
    probe_context c=probe_context_make(PC_DIRECT,r[22],r[21],r[20],r[19]);
    probe_emit(&c,PP_WRITE,(int[4]){r[19]?(int)(rd32(r[19]+4)&0xffff):0,0,0,0},(int)r[2],0);
}
void dm_combat_probe_install(void) {
    const char *enabled=getenv("DM_TEST_COMBAT_CHAIN");
    if(!enabled || strcmp(enabled,"1"))return;
    if(getenv("DM_TEST_MON_HP") || getenv("DM_TEST_SLAY") || getenv("DM_TEST_MON_KILL")) {
        LOG("ASSUME combat.measurement_setup: forced monster death invalidates capture");return;
    }
    const char *decision=getenv("DM_TEST_COMBAT_DECISION");
    if(decision) {
        int value=0;size_t length=strlen(decision);
        if(!length || length>4){LOG("ASSUME combat.decision_setup: invalid monster class");return;}
        for(size_t k=0;k<length;k++) {
            if(decision[k]<'0' || decision[k]>'9'){LOG("ASSUME combat.decision_setup: invalid monster class");return;}
            value=value*10+decision[k]-'0';
        }
        if(value>4095){LOG("ASSUME combat.decision_setup: invalid monster class");return;}
        probe_decision_class=value;
    }
    const struct {int id;void (*fn)(const uint64_t *);} gates[]={
        {OBS_COMBAT_RES_IN,probe_reduction_in},{OBS_COMBAT_RES_OUT,probe_reduction_out},
        {OBS_COMBAT_MISSILE_IN,probe_missile_in},{OBS_COMBAT_MISSILE_OUT,probe_missile_out},
        {OBS_COMBAT_CB_IN,probe_cb_in},{OBS_COMBAT_CB_OUT,probe_cb_out},{OBS_COMBAT_CB_ROLL,probe_cb_roll},{OBS_COMBAT_CB_LIFE,probe_cb_life},
        {OBS_COMBAT_OW_IN,probe_ow_in},{OBS_COMBAT_OW_OUT,probe_ow_out},{OBS_COMBAT_OW_ROLL,probe_ow_roll},{OBS_COMBAT_OW_APPLY,probe_ow_apply},
        {OBS_COMBAT_UNIT_REGEN,probe_unit_regen},
        {OBS_COMBAT_POISON_IN,probe_poison_in},{OBS_COMBAT_POISON_REFRESH,probe_poison_refresh},{OBS_COMBAT_POISON_NEW,probe_poison_new},
        {OBS_COMBAT_POISON_DONE,probe_poison_done},{OBS_COMBAT_BURN_IN,probe_burn_in},
        {OBS_COMBAT_BURN_REFRESH,probe_burn_refresh},{OBS_COMBAT_BURN_NEW,probe_burn_new},{OBS_COMBAT_BURN_OUT,probe_burn_out},
        {OBS_COMBAT_SUMMON,probe_summon},
        {OBS_COMBAT_CHANCE_ROLL,probe_chance_roll},{OBS_COMBAT_BLOCK_ROLL,probe_block_roll},
        {OBS_COMBAT_MISSILE_BLOCK_ROLL,probe_missile_block_roll},
        {OBS_COMBAT_REACTION_IN,probe_reaction_in},{OBS_COMBAT_REACTION_OUT,probe_reaction_out},
        {OBS_COMBAT_ANIMATION_IN,probe_animation_in},{OBS_COMBAT_ANIMATION_ACTION1,probe_animation_event},
        {OBS_COMBAT_ANIMATION_ACTION2,probe_animation_event},{OBS_COMBAT_ANIMATION_END,probe_animation_event},
        {OBS_COMBAT_ANIMATION_OUT,probe_animation_out},{OBS_COMBAT_ANIMATION_TAIL,probe_animation_tail},
        {OBS_COMBAT_RESTART_IN,probe_restart_in},{OBS_COMBAT_RESTART_ACTION,probe_animation_event},
        {OBS_COMBAT_RESTART_TAIL,probe_animation_tail},{OBS_COMBAT_SEQUENCE_IN,probe_sequence_in},
        {OBS_COMBAT_SEQUENCE_ACTION1,probe_animation_event},{OBS_COMBAT_SEQUENCE_ACTION2,probe_animation_event},
        {OBS_COMBAT_SEQUENCE_TAIL,probe_animation_tail},
        {OBS_COMBAT_DECISION_IN,probe_decision_in},{OBS_COMBAT_DECISION_CALL0,probe_decision_call0},
        {OBS_COMBAT_DECISION_CALL1,probe_decision_call1},{OBS_COMBAT_DECISION_CALL2,probe_decision_call2},
        {OBS_COMBAT_DECISION_OUT,probe_decision_out},
        {OBS_COMBAT_SCHEDULE_FINITE,probe_schedule_finite},{OBS_COMBAT_SCHEDULE_PERSIST,probe_schedule_persist},
        {OBS_COMBAT_DEATH_IN,probe_death_in},{OBS_COMBAT_DEATH_OUT,probe_death_out},
        {OBS_COMBAT_PHYSICAL_IN,probe_physical_in},{OBS_COMBAT_PHYSICAL_BOUNDS,probe_physical_bounds},{OBS_COMBAT_PHYSICAL_OUT,probe_physical_out}
    };
    for(size_t k=0;k<sizeof gates/sizeof *gates;k++)if(!gate_obs_present(gates[k].id)) {
        LOG("ASSUME combat.measurement_pack: missing observation gate %d",gates[k].id);return;
    }

    void dm_fightlog_obs_chain(int id,void (*fn)(const uint64_t *regs));
    const struct {int id;void (*fn)(const uint64_t *);} shared[]={
        {OBS_ATTACK_ROLL_IN,probe_chance_in},{OBS_ATTACK_ROLL_OUT,probe_chance_out},
        {OBS_MELEE_RESULT_IN,probe_melee_in},{OBS_MELEE_RESULT_OUT,probe_melee_out},{OBS_MISSILE_AVOID,probe_missile_result},
        {OBS_COMBAT_SKILL_IN,probe_skill_in},{OBS_COMBAT_SKILL_OUT,probe_skill_out},{OBS_COMBAT_PROC_STACK_IN,probe_proc_stack_in},{OBS_COMBAT_PROC_STACK_OUT,probe_skill_out},{OBS_COMBAT_PROC_IN,probe_skill_in},{OBS_COMBAT_PROC_OUT,probe_skill_out},{OBS_COMBAT_STATE_IN,probe_skill_in},{OBS_COMBAT_STATE_OUT,probe_skill_out},{OBS_COMBAT_UNKNOWN,probe_unknown},{OBS_COMBAT_MISSILE_STACK_IN,probe_missile_stack_in},{OBS_COMBAT_MISSILE_STACK_OUT,probe_skill_out},{OBS_COMBAT_MISSILE_SKILL_IN,probe_missile_skill_in},{OBS_COMBAT_MISSILE_SKILL_OUT,probe_skill_out},{OBS_COMBAT_MISSILE_HIT_IN,probe_missile_hit_in},{OBS_COMBAT_MISSILE_HIT_OUT,probe_skill_out},{OBS_COMBAT_MISSILE_AREA_IN,probe_missile_area_in},{OBS_COMBAT_MISSILE_AREA_OUT,probe_skill_out},{OBS_COMBAT_ABYSS_AREA_IN,probe_abyss_area_in},{OBS_COMBAT_ABYSS_AREA_OUT,probe_abyss_area_out},
        {OBS_COMBAT_MON_REGEN,probe_mon_regen}
    };
    for(size_t k=0;k<sizeof shared/sizeof *shared;k++)if(!gate_obs_present(shared[k].id)) {
        LOG("ASSUME combat.measurement_pack: missing observation gate %d",shared[k].id);return;
    }
    for(size_t k=0;k<sizeof gates/sizeof *gates;k++)gate_obs_set(gates[k].id,gates[k].fn);
    for(size_t k=0;k<sizeof shared/sizeof *shared;k++)dm_fightlog_obs_chain(shared[k].id,shared[k].fn);
    probe_enabled=1;
}
static void probe_drain(void) {
    if(!probe_enabled)return;
    static int header;
    static unsigned last_drained;
    if(!header){header=1;fprintf(stderr,"[dm] [combat-probe] {\"schema\":1,\"pid\":%d,\"units\":256,\"fps\":25,\"unknown\":-2147483648,\"features\":[\"chance_roll\",\"block_roll\",\"reduction_result\",\"chance_inputs\",\"item_reaction\",\"animation_schedule\",\"death_transition\",\"physical_roll\",\"positions\",\"monster_critical\",\"animation_reschedule\",\"abyss_area\",\"monster_decisions\",\"accepted_decisions\",\"retained_summon_owner\"],\"damage\":[\"physical\",\"fire\",\"cold\",\"lightning\",\"poison_rate\",\"magic\",\"burn_rate\",\"total\"]}\n",getpid());}
    unsigned drained=0;
    for(;;) {
        pthread_mutex_lock(&probe_lock);
        if(probe_tail==probe_head){pthread_mutex_unlock(&probe_lock);break;}
        probe_event e=probe_ring[probe_tail++%PROBE_RING];pthread_mutex_unlock(&probe_lock);drained++;last_drained=e.seq;
        char lists[2048]="";size_t used=0;
        for(int k=0;k<e.nlist;k++) {
            const probe_list *l=&e.lists[k];
            int n=snprintf(lists+used,sizeof lists-used,"%s[%u,%u,%u,%d,%u,%.9g,%d,%d,%d,%d]",k?",":"",l->state,l->type,l->id,l->rate,l->flags,l->expires,l->contributes,l->skill,l->level,l->source);
            if(n<0 || (size_t)n>=sizeof lists-used){probe_error();break;}used+=(size_t)n;
        }
        fprintf(stderr,"[dm] [combat-probe] {\"seq\":%u,\"epoch\":%u,\"tick\":%u,\"call\":%u,\"parent\":%u,\"kind\":%d,\"phase\":%d,\"attacker\":[%u,%u,%u],\"target\":[%u,%u,%u],\"missile\":[%u,%u,%u],\"source\":%d,\"origin_retained\":%d,\"origin\":[%d,%d,%d,%d],\"skill\":%d,\"skill_level\":%d,\"damage\":[%d,%d,%d,%d,%d,%d,%d,%d],\"resists\":[%d,%d,%d,%d,%d,%d],\"life\":[%d,%d],\"value\":[%d,%d,%d,%d],\"regen\":%d,\"chance_inputs\":[%d,%d,%d,%d,%d,%d,%d],\"animation\":[%d,%d,%d,%d,%d],\"positions\":[%d,%d,%d,%d],\"decision\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\"lists\":[%s]}\n",
            e.seq,e.epoch,e.tick,e.call,e.parent,e.kind,e.phase,e.attacker[0],e.attacker[1],e.attacker[2],e.target[0],e.target[1],e.target[2],e.missile[0],e.missile[1],e.missile[2],e.source,e.origin_retained,e.origin[0],e.origin[1],e.origin[2],e.origin[3],e.skill,e.skill_level,
            e.damage[0],e.damage[1],e.damage[2],e.damage[3],e.damage[4],e.damage[5],e.damage[6],e.damage[7],e.resists[0],e.resists[1],e.resists[2],e.resists[3],e.resists[4],e.resists[5],e.life[0],e.life[1],e.value[0],e.value[1],e.value[2],e.value[3],e.regen,e.chance_inputs[0],e.chance_inputs[1],e.chance_inputs[2],e.chance_inputs[3],e.chance_inputs[4],e.chance_inputs[5],e.chance_inputs[6],e.animation[0],e.animation[1],e.animation[2],e.animation[3],e.animation[4],e.positions[0],e.positions[1],e.positions[2],e.positions[3],e.decision[0],e.decision[1],e.decision[2],e.decision[3],e.decision[4],e.decision[5],e.decision[6],e.decision[7],e.decision[8],e.decision[9],lists);
    }
    pthread_mutex_lock(&probe_lock);unsigned lost=probe_lost,errors=probe_errors,last=last_drained;pthread_mutex_unlock(&probe_lock);
    if(drained || lost || errors)fprintf(stderr,"[dm] [combat-probe] {\"last_seq\":%u,\"lost\":%u,\"errors\":%u}\n",last,lost,errors);
}
#endif
