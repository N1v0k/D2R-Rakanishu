#include "dm_sim_ui.h"
#include "dm_sim_internal.h"
#include "dm_unit.h"
#include "dm_hud.h"
#include "dm_rift.h"
#include "dm_bossrush.h"
#include "../ios/Sources/dm_touch_game_state.h"
#include <time.h>
#include "dm_sim_text.h"
#include "dm_gear_live.h"
#include "dm_fightlog.h"
#include "dm_splash.h"
#include "dm_sim_grants.inc"

static int sim_bound[DM_SIM_MAXBAR], sim_nbound;

static char sim_skill_lines[DM_SIM_MAXSKILL][2*DM_TIPX_LINEB];

static int sim_get(const dm_sim_char *c,const dm_sim_data *d,const char *key) {
    for(int i=0;i<512;i++)if(!strcmp(d->stat_name[i],key))return c->stat_value[i];
    return 0;
}
static void sim_values(const dm_sim_data *d,dm_sim_char *c) {
#define V(field,key) c->field=sim_get(c,d,key)
    V(str,"strength");V(dex,"dexterity");V(vit,"vitality");V(ene,"energy");
    V(life,"maxhp");c->life/=256;V(mana,"maxmana");c->mana/=256;
    V(ias,"item_fasterattackrate");V(fcr,"item_fastercastrate");V(fhr,"item_fastergethitrate");V(fbr,"item_fasterblockrate");
    V(cs,"passive_critical_strike");V(dodge,"passive_dodge");
    V(ds,"item_deadlystrike");V(cb,"item_crushingblow");V(ow,"item_openwounds");V(ll,"lifedrainmindam");V(ml,"manadrainmindam");
    V(dr_flat,"normal_damage_reduction");V(mdr_flat,"magic_damage_reduction");V(liferegen,"hpregen");V(manaregen_pct,"manarecoverybonus");
    V(prevent_heal,"item_preventheal");V(dmg_demon,"item_demondamage_percent");V(dmg_undead,"item_undeaddamage_percent");
    const char *absorb[]={"","fire","light","cold","","magic"};
    const char *sunder[]={"damage","fire","light","cold","poison","magic"};
    const char *res[]={"damageresist","fireresist","lightresist","coldresist","poisonresist","magicresist"};
    const char *max[]={"","maxfireresist","maxlightresist","maxcoldresist","maxpoisonresist",""};
    const char *mast[]={"","passive_fire_mastery","passive_ltng_mastery","passive_cold_mastery","passive_pois_mastery","passive_mag_mastery"};
    const char *pierce[]={"","passive_fire_pierce","passive_ltng_pierce","passive_cold_pierce","passive_pois_pierce","passive_mag_pierce"};
    for(int e=0;e<6;e++) {
        char key[64];snprintf(key,sizeof key,"item_pierce_%s_immunity",sunder[e]);c->sunder[e]=sim_get(c,d,key)>0;
        if(absorb[e][0]){snprintf(key,sizeof key,"item_absorb%s",absorb[e]);c->absorb_flat[e]=sim_get(c,d,key);snprintf(key,sizeof key,"item_absorb%s_percent",absorb[e]);c->absorb_pct[e]=sim_get(c,d,key);}
        c->res[e]=sim_get(c,d,res[e]);
        c->maxres[e]=75+sim_get(c,d,max[e]);c->mastery[e]=sim_get(c,d,mast[e]);c->pierce[e]=sim_get(c,d,pierce[e]);
    }
#undef V
}

typedef struct {char code[4],one[4],two[4];int speed,min,max;} sim_gear;
static sim_gear *sim_gear_row(uint8_t *b,uint64_t item) {
    if(!item)return NULL;
    static sim_gear rows[1024];static int count,loaded;
    if(!loaded) {
        loaded=1;char path[1200],line[128];
        FILE *f=dm_overlay_file("sim-weapons.tsv",path,sizeof path)?fopen(path,"r"):NULL;
        if(f) {
            int n=0,end=0,bad=0;
            while(fgets(line,sizeof line,f)) {
                n++;if(n==1){if(strcmp(line,"DM_SIM_WEAPONS\t2\n"))bad=1;continue;}
                if(sscanf(line,"end\t%d",&end)==1)continue;
                if(count>=1024 || sscanf(line,"%3s\t%3s\t%3s\t%d\t%d\t%d",rows[count].code,rows[count].one,rows[count].two,&rows[count].speed,&rows[count].min,&rows[count].max)!=6)bad=1;else count++;
            }
            if(ferror(f) || n!=end)bad=1;fclose(f);if(bad)count=0;
        }
        if(!count)LOG("ASSUME combat.weapon_data: weapon timing table unavailable");
    }
    uint32_t code=item_code(b,item);
    for(int i=0;i<count;i++)if(!memcmp(rows[i].code,&code,3))return &rows[i];
    return NULL;
}
static int sim_weapon(uint8_t *b,uint64_t item,int mode,int cls,dm_sim_weapon *w) {
    w->start_known=0;memset(w->stype,0,sizeof w->stype);w->srange=0;
    if(cls<0 || cls>=8)return 0;
    unsigned index=0;
    if(item) {
        unsigned version=*(uint8_t *)(item+0x1bd);
        if(version<1 || version>3)return 0;
        index=((unsigned (*)(unsigned,int))(b+GA_ITEM_WEAPON_CLASS))(version,*(int32_t *)(item+4))&0xffff;
    }
    if(index>7 && index!=12)return 0;
    w->start_frame=*(int32_t *)(b+GA_WEAPON_START_FRAMES+index*32+cls*4);
    if(w->start_frame<0 || w->start_frame>144)return 0;
    w->start_known=1;
    if(!item){w->kind=DM_WK_NONE;w->wsm=0;snprintf(w->wclass,4,"HTH");return 1;}
    const sim_gear *row=sim_gear_row(b,item);
    if(row && strcmp(row->one,"-")) {
        snprintf(w->wclass,4,"%s",mode==2?row->two:row->one);w->wsm=row->speed;
        w->kind=!strcmp(w->wclass,"BOW")?DM_WK_BOW:!strcmp(w->wclass,"XBW")?DM_WK_XBOW:!strcmp(w->wclass,"HT1") || !strcmp(w->wclass,"HT2")?DM_WK_CLAW:!strcmp(w->wclass,"1HT") || !strcmp(w->wclass,"2HT")?DM_WK_THROW:DM_WK_MELEE;

        if(dm_splash_weapon_type(item,w->stype,&w->srange)) {
            memset(w->stype,0,sizeof w->stype);w->srange=0;
            static int said;if(!said++)LOG("ASSUME combat.weapon_type: a weapon's item type is not readable, rated without melee splash");
        }
        return 1;
    }
    return 0;
}
static void sim_armor(uint8_t *b,dm_sim_char *c,const dm_sim_data *d,uint64_t left,uint64_t right,uint64_t boots) {
    c->shield_min=c->shield_max=c->kick_min=c->kick_max=0;c->capture_bad&=~4u;
    const sim_gear *boot=sim_gear_row(b,boots);
    if(boots && !boot)c->capture_bad|=4;
    if(boot){c->kick_min=boot->min;c->kick_max=boot->max;}
    if(c->shield) {
        const sim_gear *shield=sim_gear_row(b,left);
        if(!shield || strcmp(shield->one,"-"))shield=sim_gear_row(b,right);
        if(!shield || strcmp(shield->one,"-"))c->capture_bad|=4;
        else {c->shield_min=shield->min;c->shield_max=shield->max;}
        for(int id=1;id<512;id++)if(c->active_lvl[id]>0 && d->skill[id] && id==DM_SK_SHIELD_BUFF) {
            int bad=0;c->shield_min+=(int)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_PMIN,&bad);c->shield_max+=(int)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_PMAX,&bad);if(bad)c->capture_bad|=4;
        }
    }
}
static void sim_combat(dm_sim_char *c,const dm_ic_cb *v) {
    int values[IC_C_N+1];
    if(!dm_ic_cb_eval(v,values)){c->capture_bad|=3;c->endpoint_bad=1;return;}
    c->def=values[IC_C_DEF];c->block=values[IC_C_BLK];c->ar=values[IC_C_AR];
    c->ar_base=v->ar_zero?0:v->tohit+5*v->dex+v->thf-35;c->ar_base_known=1;
    dm_sim_weapon *w=&c->hand[0];
    int hand=v->mode2?1:0;
    w->min=v->mn[hand]+v->bmn+(v->armed?0:1);w->max=v->mx[hand]+v->bmx+(v->armed?0:2);
    w->strbonus=v->armed?(v->strb==1?100:v->strb):100;w->dexbonus=v->dexb==1?100:v->dexb;
    c->ed=v->dpct+v->dmast;w->twohand=v->mode2;
    w->edmin=v->edmin;w->edmax=v->edmax;w->postflat=v->flat;
    c->shield=v->can_block;
    const int ix[]={-1,0,2,6,9,4};
    for(int e=1;e<6;e++){w->elmin[e]=v->el[ix[e]];w->elmax[e]=v->el[ix[e]+1];}
    c->poison_frames=v->plen_ovr>0?v->plen_ovr:v->plen/(v->pcnt>0?v->pcnt:1);
    w->elmin[4]=w->elmin[4]*c->poison_frames/256;w->elmax[4]=w->elmax[4]*c->poison_frames/256;
}


static void sim_belt(uint8_t *b,uint64_t pl,dm_sim_char *c) {
    static const char codes[DM_POT_N][4]={"hp1","hp2","hp3","hp4","hp5","mp1","mp2","mp3","mp4","mp5","rvs","rvl"};
    int items=0,unreadable=0;memset(c->belt,0,sizeof c->belt);c->capture_bad&=~8u;
    int k=0;for(uint64_t it=first_item(pl);it && k<4096;it=next_item(it),k++) {
        if(*(uint32_t *)(it+0xc)!=2)continue;
        items++;uint32_t code=item_code(b,it);const char *s=(const char *)&code;
        if(!code){unreadable++;continue;}
        for(int i=0;i<DM_POT_N;i++)if(!memcmp(s,codes[i],3) && (s[3]==' ' || !s[3]))c->belt[i]++;
    }
    if(items>16 || unreadable) {
        static int reported;if(!reported){reported=1;LOG("ASSUME combat.belt: %d belt items, %d without item code (at most 16 cells)",items,unreadable);}
        memset(c->belt,0,sizeof c->belt);c->capture_bad|=8;
    }
}
static int sim_charge_state(uint8_t *b,uint64_t pl) {
    unsigned ver=*(uint8_t *)(pl+0x1bd);
    uint64_t root=*(uint64_t *)(pl+0x88);
    if(!root || *(int32_t *)(root+0x1c)>=0)return 0;
    if(ver<1 || ver>3)return -1;
    uint64_t table=*(uint64_t *)(b+GA_DATA_TABLES+ver*16);
    if(!table)return -1;
    uint64_t states=*(uint64_t *)(table+0x458),count=*(uint64_t *)(table+0x460);
    if(count>512 || (count && !states))return -1;
    for(uint64_t i=0;i<count;i++) {
        int state=*(int16_t *)(states+2*i);
        if(state<=0 || state>510 || state>=*(int32_t *)(table+0x2d0))continue;
        if(!(*(uint32_t *)(root+0xb30+4*(state>>5))>>(state&31)&1))continue;
        uint64_t list=0;
        for(int chain=0;chain<2 && !list;chain++) {
            uint64_t node=*(uint64_t *)(root+0xd0+8*chain);int walked=0;
            for(;node && walked<1024;node=*(uint64_t *)(node+0x68),walked++)if(*(int32_t *)(node+0x20)==state){list=node;break;}
            if(node && !list && walked==1024)return -1;
        }
        if(!list)continue;
        int (*stat)(int,uint64_t,int,int)=(void *)(b+GA_STATLIST_GET_OWN);
        int skill=stat(ver,list,350,0),level=stat(ver,list,351,0);
        if(skill<1 || level<1)continue;
        uint64_t skills=*(uint64_t *)(table+0x11e8),nskills=*(uint64_t *)(table+0x11f0);
        if(!skills || nskills>DM_SIM_MAXSKILL || skill>=(int)nskills)return -1;
        int charge=*(int16_t *)(skills+(uint64_t)skill*0x2ec+0x74);
        int nstats=*(int32_t *)(table+0x1298);
        if(nstats<1 || nstats>4096)return -1;
        if(charge<0 || charge>=nstats)continue;
        if(stat(ver,list,charge,0)>0)return 1;
    }
    return 0;
}
static void sim_ar_inputs(uint8_t *b,uint64_t pl,uint64_t weapon,const dm_ic_cb *v,dm_sim_char *c) {
    int charge=sim_charge_state(b,pl);
    if(charge<0)c->endpoint_bad=1;
    for(int id=0;id<DM_SIM_MAXSKILL;id++) {
        uint64_t node=skill_node(pl,id),record=node?*(uint64_t *)node:0;
        if(!record)continue;
        int mastery=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,weapon,node,0);
        c->ar_bonus[id]=v->tohitpct+mastery+((*(uint8_t *)(record+0x24)&8)?c->stat_value[325]:0);
        c->damage_mastery_delta[id]=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,weapon,node,1)-v->dmast;
        c->endpoint_state_bad[id]=charge>0 && (*(uint8_t *)(record+0x24)&8)!=0;
    }
}
#include "dm_sim_dual.inc"
typedef struct {
    uint8_t *b;uint64_t pl,cand,olds[4],hands[2];int nold,sl,mask,charm;
    sim_dual_base dual;
    const int *gdelta;
    int mastery_th[DM_SIM_MAXSKILL],mastery_dmg[DM_SIM_MAXSKILL];
    dm_sim_snapshot now,after;int swap;
    const char *attack_why;
#ifndef DM_PLAYER_BUILD
    cb_res reference;
#endif
} sim_capture;

static uint32_t sim_worn_stats(uint64_t u) {
    uint64_t sl=u?*(uint64_t *)(u+0x88):0;uint32_t h=0;if(!sl)return 0;
    for(int part=0;part<2;part++) {
        if(part && *(int32_t *)(sl+0x1c)>=0)break;
        uint64_t a=sl+(part?0xe8:0x30),arr=*(uint64_t *)a,cnt=*(uint64_t *)(a+8);
        for(uint64_t i=0;arr && i<cnt && i<512;i++)h+=dm_sim_gear_stat(*(uint32_t *)(arr+8*i),*(int32_t *)(arr+8*i+4));
    }
    return h;
}


static uint32_t sim_worn_identity(uint8_t *b,uint64_t pl) {
    uint32_t h=0;
    for(uint64_t it=first_item(pl);it;it=next_item(it)) {
        uint64_t di=*(uint64_t *)(it+0x10);if(!di)continue;
        dm_sim_gear_item g={.cls=*(uint32_t *)(it+4),.mode=(int)*(uint32_t *)(it+0xc),.page=*(uint8_t *)(di+0x55),.slot=*(uint8_t *)(di+0x54)};
        if(g.mode==0 && g.page==0 && is_type(b,it,13)){g.charm=1;g.counted=item_effective(b,pl,it);}
        if(!dm_sim_gear_counts(&g))continue;
        g.stats=sim_worn_stats(it);
        uint64_t c=first_item(it);for(int n=0;c && n<6;n++,c=next_item(c))g.stats+=sim_worn_stats(c)*31u+*(uint32_t *)(c+4);
        h=dm_sim_gear_add(h,&g);
    }
    return h?h:1;
}
static int sim_capture_read(const dm_sim_data *d,void *ctx) {
    sim_capture *q=ctx;uint8_t *b=q->b;uint64_t pl=q->pl;dm_sim_snapshot *s=&q->now;
    memset(s,0,sizeof *s);dm_sim_char *c=&s->c;dm_sim_char_init(c,(int)*(uint32_t *)(pl+4));
    c->clvl=gstat(b,pl,12,0);c->diff=*(uint8_t *)(b+GA_DIFFICULTY);if(c->diff>2)return -1;
    s->players=dm_hud_players_wanted();
    s->set=*(uint8_t *)(pl+0x1bd)==3;s->session=dm_build_session();s->elysium=dm_elysium_active();
    uint64_t pd=*(uint64_t *)(pl+0x10);if(pd)snprintf(s->name,sizeof s->name,"%.16s",(char *)pd);
    s->area=dm_unit_level(pl);s->act=dm_sim_area_act(d,s->area);
    c->act=s->act;


    { static dm_sim_ref ref; s->ref_area=dm_sim_ref_capture(&ref,dm_fl_games(),s->name,s->act,s->area); }
    { static int said;if(!said && c->act>=1 && c->act<=5 && !d->std[c->diff][c->act].n){said=1;LOG("ASSUME sim.std: act %d has no regular monsters in difficulty %d; the standard enemy over all acts is used",c->act,c->diff);} }
    dm_rift_hud_t rift={0};s->rift=dm_rift_hud(&rift) && !rift.done?rift.tier:0;s->rush=dm_bossrush_stage();
    for(int i=0;i<512;i++)if(d->stat_name[i][0])c->stat_value[i]=gstat(b,pl,i,0);
    c->stats_known=1;
    s->magic_find=sim_get(c,d,"item_magicbonus");
    c->roll_inputs_bad=lstat_(b,pl,111)!=0;
    uint64_t roll_list=*(uint64_t *)(pl+0x88);
    if(!roll_list)c->roll_inputs_bad=1;
    else for(int part=0;part<2;part++) {
        if(part && *(int32_t *)(roll_list+0x1c)>=0)break;
        uint64_t header=roll_list+(part?0xe8:0x30),array=*(uint64_t *)header,count=*(uint64_t *)(header+8);
        if(count>512 || (count && !array)){c->roll_inputs_bad=1;break;}
        for(uint64_t i=0;i<count;i++)if((*(uint32_t *)(array+8*i)>>16)==180 && *(int32_t *)(array+8*i+4))c->roll_inputs_bad=1;
    }
    sim_values(d,c);c->capture_bad|=4;
    uint64_t l=*(uint64_t *)(pl+0x100),nd=l?*(uint64_t *)l:0;
    for(int k=0;nd && k<1024;k++,nd=*(uint64_t *)(nd+8)) {
        uint64_t r=*(uint64_t *)nd;if(!r)continue;int id=*(int16_t *)r;if(id<0 || id>=512 || !d->skill[id])continue;
        int base=*(int32_t *)(nd+0x38),item=*(int32_t *)(nd+0x44),lv=base;
        if(item==-1)lv+=((int (*)(uint64_t,uint64_t,int))(b+GA_SKILL_BONUS_LEVEL))(pl,nd,0);
        if(lv<0)lv=0;if(lv>256)lv=256;if(lv>c->lvl[id])c->lvl[id]=(int16_t)lv;
        if(item==-1)c->blvl[id]=(int16_t)base;
        if(item!=-1){s->item_charges=1;}
        if(item!=-1 || (base==0 && lv>0))dm_sim_bar_add(c,id);
    }
    uint64_t stats=*(uint64_t *)(pl+0x88);
    for(int id=0;id<512;id++)if(d->skill[id] && c->lvl[id]) {
        const dm_sim_skill *skill=d->skill[id];int active=skill->passive;
        for(int j=0;j<d->nstate;j++)if(!strcmp(d->state[j].name,skill->aurastate) || !strcmp(d->state[j].name,skill->passivestate)) {
            int state=d->state[j].id;
            if(state>=0 && state<256 && *(int32_t *)(stats+0x1c)<0 && (*(uint32_t *)(stats+0xb30+4*(state>>5))>>(state&31)&1))active=1;
        }
        if(active){
            c->active_lvl[id]=c->lvl[id];if(skill->aura)c->active_aura=id;
            if(!strcmp(skill->aurastate,"wolf"))c->form=1;
            if(!strcmp(skill->aurastate,"bear"))c->form=2;
            if(!strcmp(skill->aurastate,"energyshield")){int bad=0;c->es_pct=(int)sim_calc(d,c,id,0,&bad);c->es_ratio=(int)sim_calc(d,c,id,1,&bad);if(bad)c->capture_bad|=2;}
            if(!strcmp(skill->aurastate,"bonearmor"))c->bone_pool=sim_get(c,d,"bonearmor")/256;
            if(!strcmp(skill->aurastate,"cyclonearmor"))c->cyclone_pool=sim_get(c,d,"bonearmor")/256;
            if(!strcmp(skill->aurastate,"psychicward"))c->ward_pool=sim_get(c,d,"psychicward")/256;
        }
    }
    c->lvl[0]=c->lvl[0]>0?c->lvl[0]:1;
    for(int i=0;i<sim_nbound;i++)dm_sim_bar_bind(c,sim_bound[i]);
    for(int i=0;l && i<2;i++){uint64_t node=*(uint64_t *)(l+8+i*8),r=node?*(uint64_t *)node:0;if(r)dm_sim_bar_bind(c,*(int16_t *)r);}
    dm_sim_bar_add(c,0);
    uint64_t hands[2]={0},boots=0;
    for(uint64_t it=first_item(pl);it;it=next_item(it)) {uint64_t di=*(uint64_t *)(it+0x10);if(di && *(uint32_t *)(it+0xc)==1){int sl=*(uint8_t *)(di+0x54);if(sl==4 || sl==5)hands[sl-4]=it;if(sl==9)boots=it;}}
    q->hands[0]=hands[0];q->hands[1]=hands[1];s->gear=sim_worn_identity(b,pl);
    cb_res R;cb_snapshot(b,pl,hands[0],hands[1],&R);sim_combat(c,&R.now);
#ifndef DM_PLAYER_BUILD
    q->reference=R;
#endif
    int dual=0;uint64_t weapon=hands_weapon(b,hands[0],hands[1],&dual);

    int two=dual && sim_dual_class(b,c->cls,hands[0],hands[1]);
    if(!two && R.v.sim&((1u<<IC_C_AR)|(1u<<IC_C_DMG)))c->capture_bad|=1;
    if(R.v.sim&((1u<<IC_C_DEF)|(1u<<IC_C_BLK)))c->capture_bad|=2;
    if((R.live&3) || !skill_node(pl,0))c->endpoint_bad=1;
    if(two)sim_dual_now(b,d,pl,c,hands,&R,&q->dual);
    else {
        if(!sim_weapon(b,weapon,R.now.mode2?2:1,c->cls,&c->hand[0]) || dual){c->capture_bad|=1;c->endpoint_bad=1;}
        c->sias=sim_get(c,d,"attackrate")-100+c->hand[0].wsm;
        sim_ar_inputs(b,pl,weapon,&R.now,c);
        c->mastery_cs=weapon?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,weapon,0,2):0;
    }
    sim_armor(b,c,d,hands[0],hands[1],boots);
    sim_belt(b,pl,c);
    s->valid=c->life>0 && c->clvl>0 && s->players>=1 && s->players<=8;
    dm_sim_quest_next(pd?(const char *)pd:NULL,s->next_boss_by_diff);
    s->next_boss=s->next_boss_by_diff[c->diff];
    sim_grants_now(b,d,pl,c);
    q->after=*s;
    if(q->swap) {
        dm_sim_char *a=&q->after.c;
        a->roll_inputs_bad=1;
        if(q->gdelta) {
            for(int i=0;i<512;i++)if(d->stat_name[i][0])a->stat_value[i]+=q->gdelta[i];
        } else {
            for(int i=0;i<512;i++)if(d->stat_name[i][0])a->stat_value[i]+=gstat(b,q->cand,i,0)-gsum(b,q->olds,q->nold,i,0);
        }
        int aura_off[512]={0};sim_grants_after(b,d,pl,c,a,q->cand,q->olds,q->nold,q->sl,q->charm,aura_off);
        if(!q->gdelta) {
            dm_ic_in in={0};read_stats(b,pl,c->cls,in.tot);read_stats(b,q->cand,c->cls,in.new_);
            for(int k=0;k<q->nold;k++){int v[IC_S_N];read_stats(b,q->olds[k],c->cls,v);for(int j=0;j<IC_S_N;j++)in.old_[j]+=v[j];}
            in.lpv=D.lpv[c->cls];in.mpm=D.mpm[c->cls];int totals[IC_S_N];dm_ic_after(&in,totals);
            a->stat_value[7]=totals[IC_S_HP];a->stat_value[9]=totals[IC_S_MANA];
        }
        sim_values(d,a);


        int dsk=gstat(b,q->cand,127,0)-gsum(b,q->olds,q->nold,127,0)+gstat(b,q->cand,107,0)-gsum(b,q->olds,q->nold,107,0);
        cb_gather_scoped(b,pl,q->cand,q->olds,q->nold,q->sl,q->mask,hands[0],hands[1],dsk,1,&R,0,q->gdelta,q->charm);dm_ic_cb_minus(&R.aft,aura_off);sim_combat(a,&R.aft);

        uint64_t after_hands[2];dm_ic_hands_after(q->sl,q->mask,q->cand,hands[0],hands[1],after_hands);
        int two_after=c->nhands==2 || sim_dual_class(b,c->cls,after_hands[0],after_hands[1]);
        int mastery_only=0;
        if(!two_after){const char *why;mastery_only=(int)dm_ic_sim_swap_attack(&R.v,R.live,&why);if(!q->attack_why)q->attack_why=why;}
        if(mastery_only&2)a->capture_bad|=1;
        if(R.v.sim&((1u<<IC_C_DEF)|(1u<<IC_C_BLK)))a->capture_bad|=2;

        unsigned char known[DM_SIM_MAXSKILL]={0};
        for(int id=0;id<512;id++)if(d->skill[id]) {
            int now,si,own,lv=skill_pred(b,pl,q->cand,q->olds,q->nold,c->cls,id,&now,&si,&own);
            int oskill=sim_oskill_after(b,pl,c->cls,id,q->cand,q->olds,q->nold);known[id]=(own && lv>=0) || oskill>=0;
            int os=gstat(b,q->cand,97,id)-gsum(b,q->olds,q->nold,97,id);
            if(lv>=0)a->lvl[id]=(int16_t)lv;
            else if(own)a->lvl[id]=(int16_t)(now+gstat(b,q->cand,107,id)-gsum(b,q->olds,q->nold,107,id));
            if((lv==-1 || lv==-2) && own){int el=sim_elem_after(b,d,pl,c->cls,id,q->cand,q->olds,q->nold,a);if(el>=0)a->lvl[id]=(int16_t)el;}


            uint64_t own_node=skill_node(pl,id);int level_now=own_node?*(int32_t *)(own_node+0x38)+((int (*)(uint64_t,uint64_t,int))(b+GA_SKILL_BONUS_LEVEL))(pl,own_node,0):0;
            if(lv==-4 || !own)a->lvl[id]=(int16_t)(own_node?level_now+sim_bonus_change(b,c->cls,id,q->cand,q->olds,q->nold)-os:0);
            a->lvl[id]+=os;
            if(oskill>=0)a->lvl[id]=(int16_t)(oskill>256?256:oskill);
            if(a->lvl[id]<0)a->lvl[id]=0;if(a->lvl[id]>256)a->lvl[id]=256;
            if(gstat(b,q->cand,107,id)>0 || gstat(b,q->cand,97,id)>0)dm_sim_bar_add(a,id);
        }
        int16_t charged[512]={0};
        uint64_t node=l?*(uint64_t *)l:0;
        for(int k=0;node && k<1024;k++,node=*(uint64_t *)(node+8)) {
            uint64_t r=*(uint64_t *)node;if(!r)continue;int id=*(int16_t *)r,source=*(int32_t *)(node+0x44);
            if(id<0 || id>=512 || source==-1)continue;
            int removed=0;for(int j=0;j<q->nold;j++)if((int)*(uint32_t *)(q->olds[j]+8)==source)removed=1;
            if(!removed && *(int32_t *)(node+0x38)>charged[id])charged[id]=(int16_t)*(int32_t *)(node+0x38);

        }
        for(int id=1;id<512;id++)if(charged[id]>a->lvl[id])a->lvl[id]=charged[id];
        uint64_t list=q->cand?*(uint64_t *)(q->cand+0x88):0;
        if(list) {
            uint64_t base=*(int32_t *)(list+0x1c)<0?list+0xe8:list+0x30,arr=*(uint64_t *)base,count=*(uint64_t *)(base+8);
            for(uint64_t i=0;arr && i<count && i<512;i++) {
                uint32_t key=*(uint32_t *)(arr+8*i);int value=*(int32_t *)(arr+8*i+4);
                if(key>>16!=204 || !(value&255))continue;
                int id=(key&65535)>>6,lv=key&63;
                if(id<512 && d->skill[id]){if(lv>a->lvl[id])a->lvl[id]=(int16_t)lv;dm_sim_bar_add(a,id);}
            }
        }
        uint64_t left=after_hands[0],right=after_hands[1];
        sim_armor(b,a,d,left,right,q->sl==9?q->cand:boots);
        uint64_t after_weapon=hands_weapon(b,left,right,&dual);
        sim_ar_inputs(b,pl,after_weapon,&R.aft,a);
        if(dual || (R.live&3))a->endpoint_bad=1;


        a->mastery_cs=after_weapon?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,after_weapon,0,2):0;
        for(int id=0;id<DM_SIM_MAXSKILL;id++) {
            uint64_t node=skill_node(pl,id);q->mastery_th[id]=q->mastery_dmg[id]=DM_SIM_NO_NODE;
            if(node && *(uint64_t *)node){q->mastery_th[id]=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,after_weapon,node,0);q->mastery_dmg[id]=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+GA_SKILL_MASTERY))(pl,after_weapon,node,1);}
        }
        dm_sim_passive_shift shift;int passive_unknown=dm_sim_passive_levels(d,c,a,q->mastery_th,q->mastery_dmg,a->mastery_cs,&shift)!=0;
        if(passive_unknown && mastery_only==1){a->capture_bad|=1|16;a->endpoint_bad=1;}
        if(!passive_unknown) {
            sim_values(d,a);
            if(shift.tohit_pct || shift.armor_pct || shift.ar_mastery || shift.damage_mastery) {
                R.aft.tohitpct+=shift.tohit_pct;R.aft.acpct+=shift.armor_pct;R.aft.ar_mast+=shift.ar_mastery;R.aft.dmast+=shift.damage_mastery;sim_combat(a,&R.aft);
            }
        }
        unsigned char has_node[DM_SIM_MAXSKILL];for(int id=0;id<DM_SIM_MAXSKILL;id++)has_node[id]=skill_node(pl,id)!=0;
        if(!dm_sim_swap_skills_ok(d,c,a,has_node,known,passive_unknown)){a->capture_bad|=1|16;a->endpoint_bad=1;}


        if(two_after) {
            int passive=0;for(int id=1;id<DM_SIM_MAXSKILL;id++)passive|=d->skill[id] && d->skill[id]->passive && a->lvl[id]!=c->lvl[id];
            if(q->gdelta){a->capture_bad|=1;a->endpoint_bad=1;}
            else sim_dual_after(b,d,pl,c,a,q->cand,q->olds,q->nold,hands,after_hands,&q->dual,aura_off,dsk,passive);
            dm_sim_hands_drop(d,c,a);
            return 0;
        }

        if(dm_ic_hands_change(q->sl,q->mask,q->cand,hands[0],hands[1])) {
            uint64_t w=hands_weapon(b,left,right,&dual);
            if(w!=weapon && (R.now.ar_mast || R.now.dmast || R.aft.ar_mast || R.aft.dmast))a->endpoint_bad=1;
            if(!sim_weapon(b,w,R.aft.mode2?2:1,a->cls,&a->hand[0]) || dual){a->capture_bad|=1;a->endpoint_bad=1;}
        }
        a->sias=sim_get(a,d,"attackrate")-100+a->hand[0].wsm;
        dm_sim_hands_drop(d,c,a);
    }
    return 0;
}


void dm_sim_quest_server_step(uint64_t player) {
    if(!player || *(uint32_t *)player!=0)return;
    uint64_t pd=*(uint64_t *)(player+0x10);if(!pd)return;
    uint16_t words[3][48]={{0}};int state[3];
    for(int diff=0;diff<3;diff++) {
        uint64_t record=*(uint64_t *)(pd+0x40+8*diff);
        uint32_t bits=record?*(uint32_t *)(record+8):0;
        state[diff]=dm_sim_quest_words(record?*(const uint8_t **)record:NULL,bits,words[diff]);
        static uint32_t reported[3];
        if(state[diff] && reported[diff]!=bits+1){reported[diff]=bits+1;LOG("ASSUME combat.quests: difficulty %d quest record %s, %u bits (expected %d)",diff,record?"unexpected":"missing",bits,8*DM_SIM_QUEST_BYTES);}
    }
    char name[16];memcpy(name,(const char *)pd,sizeof name);
    dm_sim_quest_publish(name,(const uint16_t (*)[48])words,state);
}
static void sim_skill_previews(const dm_sim_data *d,sim_capture *q);
#ifndef DM_PLAYER_BUILD
static void sim_dual_probe(uint8_t *b,uint64_t pl);
typedef struct {int cls,unit,skill,level,type,lo,hi;} sim_native_range;
static void sim_native_range_emit(uint64_t player,uint64_t node,int level,int type,int lo,int hi) {
    if(!player || !node || !*(uint64_t *)node || level<1 || level>256){LOG("ASSUME combat.description_context: invalid native inputs");return;}
    sim_native_range v={(int)*(uint32_t *)(player+4),(int)*(uint32_t *)(player+8),*(int16_t *)*(uint64_t *)node,level,type,lo,hi};
    if(v.cls<0 || v.cls>7 || v.skill<0 || v.skill>=512){LOG("ASSUME combat.description_context: invalid native identity");return;}
    static sim_native_range seen[64];static unsigned count;static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    for(unsigned k=0;k<count;k++)if(!memcmp(&seen[k],&v,sizeof v)){pthread_mutex_unlock(&lock);return;}
    if(count==64){pthread_mutex_unlock(&lock);LOG("ASSUME combat.description_overflow: native range capacity exceeded");return;}
    seen[count++]=v;
    fprintf(stderr,"[dm] [sim-native-range] {\"class\":%d,\"unit\":%d,\"skill\":%d,\"level\":%d,\"descdam\":%d,\"game\":[%d,%d]}\n",v.cls,v.unit,v.skill,v.level,v.type,v.lo,v.hi);
    pthread_mutex_unlock(&lock);
}
static void sim_native_stream(const uint64_t *r){sim_native_range_emit(r[22],r[23],(int)r[21],8,(int)r[8],(int)r[3]);}
static void sim_native_aura(const uint64_t *r){sim_native_range_emit(r[22],r[23],(int)r[21],25,(int)r[8],(int)r[3]);}
static _Thread_local struct {uint64_t frame,player,node;int level,valid;} sim_native_ground_context;
static void sim_native_ground_level(const uint64_t *r) {
    if(sim_native_ground_context.valid){LOG("ASSUME combat.description_pair: ground range context already open");return;}
    sim_native_ground_context.frame=r[29];sim_native_ground_context.player=r[23];sim_native_ground_context.node=r[22];
    sim_native_ground_context.level=(int)r[24];sim_native_ground_context.valid=1;
}
static void sim_native_ground(const uint64_t *r) {
    if(!sim_native_ground_context.valid || sim_native_ground_context.frame!=r[29] || sim_native_ground_context.player!=r[23] || sim_native_ground_context.node!=r[22]) {
        sim_native_ground_context.valid=0;LOG("ASSUME combat.description_pair: ground range context mismatch");return;
    }
    sim_native_ground_context.valid=0;
    sim_native_range_emit(r[23],r[22],sim_native_ground_context.level,9,(int)r[20],(int)r[3]);
}
static void sim_native_dispatch(const uint64_t *r) {
    static int previous_skill=-1,previous_status=-1;
    uint64_t record=r[22],player=r[21];
    int skill=record?*(int16_t *)record:-1,status=(int)r[0];
    if(skill==previous_skill && status==previous_status)return;
    previous_skill=skill;previous_status=status;
    fprintf(stderr,"[dm] [sim-description-dispatch] {\"frame\":%llu,\"unit\":%u,\"skill\":%d,\"level\":%d,\"status\":%d}\n",
        (unsigned long long)dm_guest_frame(),player?*(uint32_t *)(player+8):UINT32_MAX,skill,(int)r[24],status);
}
static void sim_native_range_install(void) {
    static int initialized;
    if(initialized)return;
    initialized=1;
    if(env_int("DM_TEST_SIM_TOOLTIPS",0)!=1)return;
    const struct {int id;void (*fn)(const uint64_t *);} gates[]={
        {OBS_SIM_STREAM_RANGE,sim_native_stream},{OBS_SIM_GROUND_LEVEL,sim_native_ground_level},
        {OBS_SIM_GROUND_RANGE,sim_native_ground},{OBS_SIM_AURA_RANGE,sim_native_aura},
        {OBS_SIM_DESCRIPTION_DISPATCH,sim_native_dispatch}
    };
    for(unsigned k=0;k<sizeof gates/sizeof *gates;k++)if(!gate_obs_present(gates[k].id)){LOG("ASSUME combat.description_pack: missing native range gate %d",gates[k].id);return;}
    for(unsigned k=0;k<sizeof gates/sizeof *gates;k++)gate_obs_set(gates[k].id,gates[k].fn);
}
static void sim_exact_combat(const dm_sim_data *d,const dm_sim_char *c,unsigned sequence,uint64_t player) {
    static int initialized,enabled,mon,kind,level,skill,eligible=1;
    if(!initialized) {
        initialized=1;const char *input=getenv("DM_TEST_SIM_COMBAT");int used=0;
        if(input) {
            if(sscanf(input,"%d:%d:%d:%d%n",&mon,&kind,&level,&skill,&used)!=4 ||
               mon<0 || mon>=d->nmon || kind<DM_TK_NORMAL || kind>DM_TK_CLONE ||
               level<0 || level>110 || skill<0 || skill>=DM_SIM_MAXSKILL) {
                LOG("ASSUME combat.model_fixture: invalid model target");return;
            }
            if(input[used]) {
                int tail=0;
                if(sscanf(input+used,":%d%n",&eligible,&tail)!=1 || input[used+tail] || eligible<1 || eligible>1024) {
                    LOG("ASSUME combat.model_fixture: invalid eligible target count");return;
                }
            }
            enabled=1;
        }
    }
    if(!enabled)return;
    dm_sim_target t;dm_sim_skill_out out;
    if(!d->skill[skill] || !c->lvl[skill] || dm_sim_target_mon(d,mon,c->diff,level,kind,1,&t)) {
        LOG("ASSUME combat.model_fixture: model inputs unavailable");return;
    }
    t.extra_targets=eligible-1;
    if(dm_sim_skill_eval(d,c,&t,skill,&out)) {
        LOG("ASSUME combat.model_fixture: model inputs unavailable");return;
    }
    fprintf(stderr,"[dm] [sim-model-combat] {\"schema\":1,\"unit\":%u,\"frame\":%llu,\"sequence\":%u,\"skill\":%d,\"level\":%d,\"capture_bad\":%u,\"inputs\":{\"fcr\":%d,\"ias\":%d,\"form\":%d,\"class\":%d,\"wclass\":\"%s\",\"hands\":%d,\"start_frame\":%d,\"start_known\":%d},\"roll\":{\"known\":%d,\"low\":%d,\"high\":%d,\"resistance\":%d,\"element\":%d,\"chance\":%.17g,\"critical\":%.17g},\"target\":{\"class\":%d,\"kind\":%d,\"level\":%d,\"difficulty\":%d,\"hp\":%d,\"defense\":%d,\"ar\":%d,\"block\":%d,\"regen\":%d,\"resists\":[%d,%d,%d,%d,%d,%d]},\"flags\":%d,\"frames\":%d,\"action_frame\":%d,\"cooldown\":%d,\"travel\":%d,\"cth\":%.17g,\"hits\":%.17g,\"burst_attacks\":%d,\"eligible_targets\":%d,\"raw\":[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],\"hit\":[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],\"dot\":%.17g,\"dot_frames\":%d,\"burn\":%.17g,\"burn_frames\":%d,\"dps\":%.17g,\"ttk\":%.17g}\n",
        *(uint32_t *)(player+8),(unsigned long long)dm_guest_frame(),sequence,skill,c->lvl[skill],c->capture_bad,c->fcr,c->ias,c->form,c->cls,c->hand[0].wclass,c->nhands,c->hand[0].start_frame,c->hand[0].start_known,out.roll.known,out.roll.low,out.roll.high,out.roll.resistance,out.roll.element,out.roll.chance,out.roll.critical,mon,kind,t.mlvl,c->diff,t.hp,t.ac,t.ar,t.block,t.regen,
        t.res[0],t.res[1],t.res[2],t.res[3],t.res[4],t.res[5],out.flags,out.frames,out.action_frame,out.cooldown,out.travel,out.cth,out.hits,out.burst_attacks,eligible,
        out.raw[0],out.raw[1],out.raw[2],out.raw[3],out.raw[4],out.raw[5],out.hit[0],out.hit[1],out.hit[2],out.hit[3],out.hit[4],out.hit[5],
        out.dot,out.dot_frames,out.burn,out.burn_frames,out.dps,out.ttk);
    dm_sim_surv_out incoming;
    if(dm_sim_survive(d,c,&t,NULL,&incoming)) {
        LOG("ASSUME combat.model_fixture: incoming inputs unavailable");return;
    }
    fprintf(stderr,"[dm] [sim-model-incoming] {\"sequence\":%u,\"capture_bad\":%u,\"cth\":%.17g,\"p_land\":%.17g,\"dmg_hit\":%.17g,\"player\":{\"level\":%d,\"defense\":%d,\"block\":%d,\"dr\":%d,\"mdr\":%d,\"dodge\":%d,\"resists\":[%d,%d,%d,%d,%d,%d],\"absorb_pct\":[%d,%d,%d,%d,%d,%d],\"absorb_flat\":[%d,%d,%d,%d,%d,%d]},\"target\":{\"level\":%d,\"ar\":%d,\"min\":%d,\"max\":%d,\"crit\":%d,\"cycle\":%d}}\n",
        sequence,c->capture_bad,incoming.cth,incoming.p_land,incoming.dmg_hit,c->clvl,c->def,c->block,c->dr_flat,c->mdr_flat,c->dodge,
        c->res[0],c->res[1],c->res[2],c->res[3],c->res[4],c->res[5],
        c->absorb_pct[0],c->absorb_pct[1],c->absorb_pct[2],c->absorb_pct[3],c->absorb_pct[4],c->absorb_pct[5],
        c->absorb_flat[0],c->absorb_flat[1],c->absorb_flat[2],c->absorb_flat[3],c->absorb_flat[4],c->absorb_flat[5],
        t.mlvl,t.ar,t.dmin,t.dmax,t.crit,t.cycle);
}
static void sim_exact_values(const dm_sim_data *d,sim_capture *q) {
    static int enabled=-1;
    if(enabled<0)enabled=env_int("DM_TEST_SIM_VALUES",0)==1;
    if(!enabled || !q->now.valid)return;
    sim_native_range_install();
    const dm_sim_char *c=&q->now.c;
    static dm_sim_char previous;
    static unsigned sequence;
    previous.stat_value[6]=c->stat_value[6];previous.stat_value[8]=c->stat_value[8];previous.stat_value[10]=c->stat_value[10];
    if(sequence && !memcmp(&previous,c,sizeof previous))return;
    previous=*c;sequence++;
    uint8_t *b=q->b;uint64_t pl=q->pl;
    int dual=0,game[IC_C_N+3]={0};
    uint64_t weapon=hands_weapon(b,q->hands[0],q->hands[1],&dual);
    cb_game(b,pl,skill_node(pl,0),c->lvl[0],weapon,game);
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"class\":%d,\"level\":%d,\"difficulty\":%d,\"capture_bad\":%u,\"sheet_model\":[%d,%d,%d,%d,%d],\"sheet_game\":[%d,%d,%d,%d,%d]}\n",
        sequence,c->cls,c->clvl,c->diff,c->capture_bad,c->def,c->block,c->ar,(int)(c->life*256),(int)(c->mana*256),
        game[IC_C_DEF],game[IC_C_BLK],game[IC_C_AR],gstat(b,pl,7,0),gstat(b,pl,9,0));
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"attributes\":[%d,%d,%d,%d],\"mastery\":[%d,%d,%d,%d,%d,%d],\"resists\":[%d,%d,%d,%d,%d,%d]}\n",
        sequence,c->str,c->dex,c->vit,c->ene,c->mastery[0],c->mastery[1],c->mastery[2],c->mastery[3],c->mastery[4],c->mastery[5],c->res[0],c->res[1],c->res[2],c->res[3],c->res[4],c->res[5]);
    for(uint64_t it=first_item(pl);it;it=next_item(it)) {
        uint64_t item=*(uint64_t *)(it+0x10);
        if(item && *(uint32_t *)(it+0xc)==1) {
            const int ids[]={0,1,2,3,17,18,19,21,22,23,24,31,39,41,43,45,48,49,50,51,52,53,54,55,56,57,58,59,60,62,93,99,102,135,136,141};
            char values[1024];size_t used=0;
            for(unsigned k=0;k<sizeof ids/sizeof *ids;k++) {
                int n=snprintf(values+used,sizeof values-used,"%s\"%d\":%d",k?",":"",ids[k],gstat(b,it,ids[k],0));
                if(n<0 || (size_t)n>=sizeof values-used){LOG("ASSUME combat.equipment_capture: stat record overflow");return;}
                used+=(size_t)n;
            }
            fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"slot\":%u,\"code\":%u,\"quality\":%u,\"file_index\":%u,\"item_flags\":%u,\"item_stats\":{%s}}\n",
                sequence,*(uint8_t *)(item+0x54),item_code(b,it),*(uint32_t *)item,*(uint32_t *)(item+0x34),*(uint32_t *)(item+0x18),values);
        }
    }
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"version\":%u}\n",sequence,*(uint8_t *)(pl+0x1bd));
    int physical[2]={0};int physical_bad=sim_normal_physical(c,physical);
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"physical_bad\":%d,\"physical_model\":[%d,%d],\"physical_game\":[%d,%d]}\n",
        sequence,physical_bad,physical[0],physical[1],game[IC_C_N+1],game[IC_C_N+2]);
    const cb_res *reference=&q->reference;
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"reference_bad\":%u,\"reference_live\":%u,\"reference_model\":[%d,%d,%d,%d,%d],\"reference_game\":[%d,%d,%d,%d,%d]}\n",
        sequence,reference->v.bad,reference->live,reference->nowv[0],reference->nowv[1],reference->nowv[2],reference->nowv[3],reference->nowv[4],
        reference->gamev[0],reference->gamev[1],reference->gamev[2],reference->gamev[3],reference->gamev[4]);
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"ar_base\":%d}\n",sequence,c->ar_base);
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"speed_native\":[%d,%d,%d,%d],\"sias\":%d,\"form\":%d}\n",
        sequence,sim_get(c,d,"attackrate"),weapon?gstat(b,weapon,68,0):0,sim_get(c,d,"item_fasterattackrate"),weapon?gstat(b,weapon,93,0):0,c->sias,c->form);
    int count=0;
    const uintptr_t functions[]={GA_SKILL_ELEM_MIN,GA_SKILL_ELEM_MAX,GA_SKILL_PHYS_MIN,GA_SKILL_PHYS_MAX};
    for(int id=0;id<DM_SIM_MAXSKILL;id++)if(c->lvl[id] && d->skill[id] && skill_node(pl,id)) {
        int raw[4],model[4],display[4],bad=0;
        long long model_raw[4];
        const dm_sim_skill *s=d->skill[id];
        int units=s->etype==DM_EL_POIS || s->frameburn?256:1;
        char ar_model[32]="null",ar_game[32]="null";
        if((s->fam&DM_FAM_WEAPON) && !s->alwayshit && s->srvdofunc!=150) {
            int actual=0,colour=0;
            ((void (*)(uint64_t,int *,int *,int,uint64_t,uint64_t))(b+GA_CHAR_ATTACK_RATING))(pl,&actual,&colour,c->lvl[id],skill_node(pl,id),weapon);
            snprintf(ar_model,sizeof ar_model,"%d",sim_attack_rating(d,c,id,&bad));
            snprintf(ar_game,sizeof ar_game,"%d",actual);
        }
        for(int k=0;k<4;k++) {
            raw[k]=((int (*)(int,uint64_t,int,int,int))(b+functions[k]))(*(uint8_t *)(pl+0x1bd),pl,id,c->lvl[id],k<2);
            display[k]=units==256?raw[k]:raw[k]>>8;
            model[k]=(int)(dm_sim_skill_value(d,c,id,c->lvl[id],k,&bad)*units);
            model_raw[k]=(long long)dm_sim_skill_raw(d,c,id,c->lvl[id],k,&bad);
        }
        fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"skill\":%d,\"level\":%d,\"hard\":%d,\"units\":%d,\"bad\":%d,\"model\":[%d,%d,%d,%d],\"game\":[%d,%d,%d,%d],\"raw\":[%d,%d,%d,%d],\"model_raw\":[%lld,%lld,%lld,%lld],\"ar_model\":%s,\"ar_game\":%s}\n",
            sequence,id,c->lvl[id],c->blvl[id],units,bad,model[0],model[1],model[2],model[3],display[0],display[1],display[2],display[3],raw[0],raw[1],raw[2],raw[3],model_raw[0],model_raw[1],model_raw[2],model_raw[3],ar_model,ar_game);
        int mastery_model[3],mastery_game[3];
        const int variables[]={DM_V_MATH,DM_V_MADM,DM_V_MACR};
        for(int k=0;k<3;k++) {
            mastery_model[k]=(int)sim_skill_var(d,c,id,variables[k],&bad);
            mastery_game[k]=((int (*)(int,uint64_t,int,int,int))(b+GA_SKILL_MASTERY_VALUE))(*(uint8_t *)(pl+0x1bd),pl,id,c->lvl[id],k);
        }
        fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"mastery_skill\":%d,\"bad\":%d,\"mastery_model\":[%d,%d,%d],\"mastery_game\":[%d,%d,%d]}\n",
            sequence,id,bad,mastery_model[0],mastery_model[1],mastery_model[2],mastery_game[0],mastery_game[1],mastery_game[2]);
        if(s->descdam==5 || s->descdam==7 || s->descdam==8 || s->descdam==9 || s->descdam==18 || s->descdam==24 || s->descdam==25) {
            int expected[2]={0},actual[2]={0},colour=0,*colour_pointer=&colour;
            uint64_t node=skill_node(pl,id);
            int status=dm_sim_skill_endpoints(d,c,id,expected);
            int length=((int (*)(int,uint64_t,int,int,int))(b+GA_SKILL_ELEM_LEN))(*(uint8_t *)(pl+0x1bd),pl,id,c->lvl[id],1);
            int length_model=(int)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_ELEN,&bad);
            if(s->descdam==5 || s->descdam==25) {
                ((void (*)(int,uint64_t,int **,uint64_t,int *,int *,uint64_t,int))(b+GA_SKILL_DESC_DAMAGE))(
                    *(uint8_t *)(pl+0x1bd),*(uint64_t *)node,&colour_pointer,pl,&actual[0],&actual[1],node,c->lvl[id]);
                if(s->descdam==25)for(int k=0;k<2;k++)actual[k]=(int)((float)actual[k]*(((float)skill_calc(b,pl,node,c->lvl[id],k)+100.0f)/100.0f));
            } else if(s->descdam==8) {
                int a=skill_calc(b,pl,node,c->lvl[id],0),z=skill_calc(b,pl,node,c->lvl[id],1);
                if(!a)a=1;
                if(!z)z=1;
                for(int k=0;k<2;k++)actual[k]=(int)((int64_t)raw[k]*12*a/z)>>8;
            } else {
                int with_percent=s->descdam==7 || s->descdam==18;
                ((void (*)(uint64_t,int *,int *,int *,int,int,uint64_t,int,uint64_t))(b+GA_CHAR_DAMAGE_BASE))(
                    pl,&actual[0],&actual[1],&colour,with_percent?skill_calc(b,pl,node,c->lvl[id],0):0,with_percent?skill_calc(b,pl,node,c->lvl[id],1):0,node,128,0);
                ((void (*)(uint64_t,int *,int *,int *,uint64_t))(b+GA_CHAR_DAMAGE_ADD))(pl,&actual[0],&actual[1],&colour,node);
                uint64_t record=*(uint64_t *)node;
                int source=*(uint8_t *)(record+0x24d);
                for(int k=0;k<2;k++) {
                    actual[k]=(int)((int64_t)actual[k]*source/128);
                    if(s->descdam==9)actual[k]+=(int)((((int64_t)*(int32_t *)(record+0x250+k*4)<<*(uint8_t *)(record+0x24c))+raw[k])*75)>>8;
                    else actual[k]+=(raw[2+k]>>8)+(int)(((int64_t)raw[k]*(s->descdam==24 && s->etype==DM_EL_POIS?length:1))>>8);
                }
            }
            if(actual[1]<=actual[0])actual[1]=actual[0]+1;
            fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"description_skill\":%d,\"descdam\":%d,\"description_status\":%d,\"description_model\":[%d,%d],\"description_game\":[%d,%d],\"length_model\":%d,\"length_game\":%d,\"bad\":%d}\n",
                sequence,id,s->descdam,status,expected[0],expected[1],actual[0],actual[1],length_model,length,bad);
        }
        count++;
    }
    fprintf(stderr,"[dm] [sim-exact] {\"sequence\":%u,\"complete\":%d}\n",sequence,count);
    sim_exact_combat(d,c,sequence,pl);
}
#endif
#ifndef DM_PLAYER_BUILD
static void sim_test_select(uint8_t *b,uint64_t player) {
    static int initialized,count,next;
    static struct {int frame,skill,side;} actions[16];
    if(!initialized) {
        initialized=1;const char *input=getenv("DM_TEST_SIM_SELECT");
        while(input && *input) {
            int frame,skill,side,used=0;
            if(count==16 || sscanf(input,"%d:%d:%d%n",&frame,&skill,&side,&used)!=3 ||
               (input[used] && input[used]!=';') || frame<1 || skill<0 || skill>=DM_SIM_MAXSKILL ||
               side<0 || side>1 || (count && frame<=actions[count-1].frame)) {
                count=0;LOG("ASSUME combat.skill_selection: invalid action list");return;
            }
            actions[count].frame=frame;actions[count].skill=skill;actions[count++].side=side;
            input+=used;if(*input)input++;
        }
    }
    if(count) {
        uint64_t list=*(uint64_t *)(player+0x100);static int previous[2]={-2,-2};int selected[2];
        for(int side=0;side<2;side++) {
            uint64_t node=list?*(uint64_t *)(list+(side?8:16)):0,record=node?*(uint64_t *)node:0;
            selected[side]=record?*(int16_t *)record:-1;
        }
        if(memcmp(previous,selected,sizeof selected)) {
            memcpy(previous,selected,sizeof selected);
            fprintf(stderr,"[dm] [sim-selection-state] {\"frame\":%llu,\"right\":%d,\"left\":%d}\n",
                (unsigned long long)dm_guest_frame(),selected[0],selected[1]);
        }
    }
    if(next>=count || dm_guest_frame()<(uint64_t)actions[next].frame)return;
    int skill=actions[next].skill,side=actions[next++].side;
    int result=((int (*)(int,int,int))(b+GA_SKILL_SELECT))(skill,side,-1);
    uint64_t list=*(uint64_t *)(player+0x100),node=list?*(uint64_t *)(list+(side?8:16)):0;
    uint64_t record=node?*(uint64_t *)node:0;
    int actual=record?*(int16_t *)record:-1,item=node?*(int32_t *)(node+0x44):0;
    fprintf(stderr,"[dm] [sim-selected] {\"frame\":%llu,\"unit\":%u,\"requested\":%d,\"side\":%d,\"result\":%d,\"actual\":%d,\"item\":%d}\n",
        (unsigned long long)dm_guest_frame(),*(uint32_t *)(player+8),skill,side,result,actual,item);
    if(result!=1 || actual!=skill || item!=-1)LOG("ASSUME combat.skill_selection: original selection failed");
}
#endif
static int sim_live_read(const dm_sim_data *d,void *ctx) {
    sim_capture *q=ctx;int result=sim_capture_read(d,q);
#ifndef DM_PLAYER_BUILD
    if(!result)sim_exact_values(d,q);
#endif
    memset(sim_skill_lines,0,sizeof sim_skill_lines);
    if(!result && dm_custom_get("combat_sim") && dm_ui_panel_shown("SkillsTreePanel"))sim_skill_previews(d,q);
    return result;
}
void dm_sim_live_pump(uint8_t *b,uint64_t pl) {
    if(!b || !pl || !*(uint64_t *)(pl+0x88)){dm_sim_publish(NULL);sim_nbound=0;memset(sim_skill_lines,0,sizeof sim_skill_lines);return;}
    static int tick;if(++tick%10)return;
#ifndef DM_PLAYER_BUILD
    dm_combat_fixture_speed(b,pl,0);
    void dm_test_peel_client(uint8_t *b); dm_test_peel_client(b);
    dm_test_give_stats_client(b);
    sim_test_select(b,pl);
    sim_dual_probe(b,pl);
#endif
    sim_nbound=dm_touch_game_skills(sim_bound,DM_SIM_MAXBAR);
    data_load();sim_capture *q=calloc(1,sizeof *q);
    if(!q){LOG("ASSUME combat.capture_memory: allocation failed");dm_sim_publish(NULL);return;}
    q->b=b;q->pl=pl;
    int rc=dm_sim_try_read(*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod",sim_live_read,q);

    if(!rc){dm_sim_publish(&q->now);static unsigned bad;unsigned now=q->now.c.capture_bad&~(8u|SIM_GRANT_BAD);if(now!=bad){bad=now;if(bad)LOG("ASSUME combat.snapshot: incomplete combat fields mask %u",bad);}}else if(rc!=-3)dm_sim_publish(NULL);
    free(q);
}
typedef struct {int diff,area,clvl;uint64_t key;} sim_key_request;
static int sim_key_read(const dm_sim_data *d,void *ctx){sim_key_request *q=ctx;q->key=dm_sim_tip_key(d,q->diff,q->area,q->clvl,q->key);return 0;}


static uint64_t sim_tip_key(uint8_t *b,uint64_t pl,uint64_t key) {
    if(!dm_custom_get("combat_sim"))return key;
    uint64_t sp=(uint64_t)dm_custom_splash_on();
    sim_key_request q={*(uint8_t *)(b+GA_DIFFICULTY),dm_unit_level(pl),gstat(b,pl,12,0),key};
    if(dm_sim_try_read(*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod",sim_key_read,&q))return ((key ^ 0x5157)*1099511628211ull ^ sp)*1099511628211ull;
    return (q.key ^ sp)*1099511628211ull;
}

enum { GEAR_OFF, GEAR_SHOWN, GEAR_PENDING, GEAR_UNRATED };
static void add_unrated(uint8_t *b,const char *reason) {
    char line[DM_TIPX_LINEB];snprintf(line,sizeof line,dm_str(b,"dmGearUnrated"),dm_str(b,reason));
    add(K_WHY,0,CC "5%s",line);
}
static void gear_lines(uint8_t *b,const dm_gear_tip *t) {
    static const char *const measure[DM_GEAR_M_N]={"dmGearGroup","dmGearSingle","dmGearSurvival","dmGearRecovery"};
    static const char *const group[DM_GEAR_R_N]={"dmGearRDefense","dmGearRLife","dmGearRMana","dmGearRRegen","dmGearRLeech","dmGearRResist","dmGearRCast",
                                                 "dmGearRAttack","dmGearRHitRec","dmGearRSkills","dmGearRDamage"};

    dm_gear_words w;
    for(int k=0;k<DM_GEAR_M_N;k++)w.measure[k]=dm_str(b,measure[k]);
    for(int g=0;g<DM_GEAR_R_N;g++)w.group[g]=dm_str(b,group[g]);
    w.more=dm_str(b,"dmGearMore");w.less=dm_str(b,"dmGearLess");w.missed=dm_str(b,"dmGearMissed");w.reached=dm_str(b,"dmGearReached");
    w.below=dm_str(b,"dmGearBelow");w.below_same=dm_str(b,"dmGearBelowSame");
    int lang=dm_lang_group();
    static char line[DM_GEAR_LINES][DM_GEAR_LINEB];
    int n=dm_gear_tip_text(t,&w,lang==DM_LG_DE || lang==DM_LG_PT,line,DM_GEAR_LINES);
    if(!n)return;
    char heading[DM_TIPX_LINEB];dm_sim_heading_text(heading,sizeof heading,t->act,lang==DM_LG_DE);
    add(K_NOTE,0,"%s",heading);
    for(int i=0;i<n;i++)add(K_NOTE,0,"%s",line[i]);

    static const char *const immune_key[DM_EL_N]={NULL,"dmSimImmFire","dmSimImmLtng","dmSimImmCold","dmSimImmPois","dmSimImmMag"};
    for(int e=0;e<DM_EL_N;e++)if(t->immune[e]>0 && immune_key[e]) {
        char text[DM_TIPX_LINEB];snprintf(text,sizeof text,dm_str(b,immune_key[e]),t->immune[e]);
        add(K_NOTE,0,CC "5%s",text);
    }
}

static dm_gear_tip shown_tip;
static const dm_gear_tip *gear_shown(void){return &shown_tip;}



static int gear_tip(uint8_t *b,uint64_t pl,uint64_t key,uint64_t cand,const uint64_t *olds,int nold,int sl,int mask,const int *gdelta,int charm,int *dots,const char **why) {
    *dots=0;*why=NULL;
    dm_gear_tip t;uint64_t asked=0;
    int state=dm_gear_tip_get(key,&t,&asked);
    if(state==DM_GEAR_TIP_DONE) {
        static uint64_t said;
        if(said!=key){said=key;LOG("Combat Sim item: rated in %.3f ms, dots %d / %d",dm_gear_tip_ms(),t.score.damage_base?t.score.dots_damage:0,t.score.dots_survival);}
        gear_lines(b,&t);*dots=dm_gear_dots_code(&t.score);shown_tip=t;return GEAR_SHOWN;
    }
    if(state==DM_GEAR_TIP_FAILED){*why="dmGearNoSim";return GEAR_UNRATED;}
    if(state==DM_GEAR_TIP_GRANTS) {
        static uint64_t said;
        if(said!=key && t.grant.stat>=0 && t.grant.stat<512) {
            said=key;
            LOG("ASSUME bestgear.aura_stats: item aura %d level %d differs from game state values (stat %d: %d vs %d)",t.grant.skill,t.grant.level,t.grant.stat,t.grant.model,t.grant.game);
        }
        *why="dmGearNoAura";return GEAR_UNRATED;
    }
    if(state==DM_GEAR_TIP_PENDING) {
        static uint64_t said;
        if(dm_guest_frame()>asked+3 && said!=key){said=key;LOG("ASSUME bestgear.tip_late: block not ready after 3 frames");}
        return GEAR_PENDING;
    }
    sim_capture *q=calloc(1,sizeof *q);
    if(!q){LOG("ASSUME combat.capture_memory: allocation failed");return GEAR_PENDING;}
    q->b=b;q->pl=pl;q->cand=cand;q->nold=nold;q->sl=sl;q->mask=mask;q->charm=charm;q->swap=1;q->gdelta=gdelta;
    for(int i=0;i<nold;i++)q->olds[i]=olds[i];
    const char *set=*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod";
    int rc=dm_sim_try_read(set,sim_capture_read,q),result=GEAR_PENDING;
    unsigned bad=rc?0:q->now.c.capture_bad|q->after.c.capture_bad;
    if(rc==-3)result=GEAR_PENDING;
    else if(rc || (bad&1 && !(bad&16)) || !q->now.valid) {

        *why=dm_ic_unrated_key(rc,q->now.valid,rc?0:q->now.c.capture_bad,rc?0:q->after.c.capture_bad,q->attack_why);result=GEAR_UNRATED;
        static uint64_t said;
        if(said!=key){said=key;LOG("Combat Sim item: not rated (%s): read %d, capture bits now %#x after %#x, valid %d",*why,rc,rc?0:q->now.c.capture_bad,rc?0:q->after.c.capture_bad,q->now.valid);}
    }
    else if(bad&16){*why="dmGearNoPassive";result=GEAR_UNRATED;}
    else if(bad&SIM_GRANT_BAD){*why="dmGearNoAura";result=GEAR_UNRATED;}
    else {
        dm_gear_goal goal;dm_gear_goal_get(dm_build_session(),&goal);
        static int said;
        if(dm_gear_tip_put(key,set,&q->now.c,&q->after.c,&goal,dm_custom_splash_on(),dm_guest_frame()) && !said++)LOG("ASSUME bestgear.tip_queue: no free rating slot");
    }
    free(q);
    return result;
}




typedef struct { sim_capture q; int claws_after, block[2], ok; } sim_block_req;
static int sim_block_read(const dm_sim_data *d,void *ctx) {
    sim_block_req *r=ctx;int rc=sim_capture_read(d,&r->q);if(rc)return rc;
    const dm_sim_char *c=&r->q.now.c;
    if(!r->q.now.valid || (c->nhands==2 && !dm_sim_two_claws(c)))return 0;
    r->block[0]=dm_sim_fight_block(d,c);r->block[1]=dm_sim_swap_block(d,&r->q.after.c,r->claws_after);
    r->ok=r->block[0]>=0 && r->block[1]>=0;
    return 0;
}

static int sim_block_pair(uint8_t *b,uint64_t pl,uint64_t cand,const uint64_t *olds,int nold,int sl,int mask,const int *gdelta,int charm,int claws_after,int out[2]) {
    sim_block_req *r=calloc(1,sizeof *r);if(!r)return 0;
    sim_capture *q=&r->q;q->b=b;q->pl=pl;q->cand=cand;q->nold=nold;q->sl=sl;q->mask=mask;q->charm=charm;q->swap=1;q->gdelta=gdelta;
    for(int i=0;i<nold;i++)q->olds[i]=olds[i];
    r->claws_after=claws_after;
    int rc=dm_sim_try_read(*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod",sim_block_read,r),known=rc==-3?-1:!rc && r->ok;
    if(known==1){out[0]=r->block[0];out[1]=r->block[1];}
    free(r);return known;
}
static void sim_skill_previews(const dm_sim_data *d,sim_capture *q) {
    dm_sim_char *c=&q->now.c;dm_sim_target target;
    int act=dm_sim_target_act(d,c,&target);if(act<0)return;
    int de=dm_lang_group()==DM_LG_DE;
    char heading[DM_TIPX_LINEB],line[DM_TIPX_LINEB];dm_sim_heading_text(heading,sizeof heading,act,de);
    uint64_t table=*(uint64_t *)(q->b+GA_DATA_TABLES+(uint64_t)*(uint8_t *)(q->pl+0x1bd)*16);
    int (*bonus)(uint64_t,uint64_t,int)=(int (*)(uint64_t,uint64_t,int))(q->b+GA_SKILL_BONUS_LEVEL);
    for(int i=0;i<D.nsk;i++) {
        int id=D.sk[i].id;
        if(D.sk[i].cls!=c->cls || id<=0 || id>=DM_SIM_MAXSKILL || !d->skill[id])continue;
        const dm_sim_skill *sk=d->skill[id];
        if(sk->fam&(DM_FAM_MODIFIER|DM_FAM_U) || dm_skill_reason(q->b,q->pl,c->clvl,id,c->blvl[id]))continue;

        uint64_t node[9]={0},existing=skill_node(q->pl,id);
        int own=existing?*(int32_t *)(existing+0x38)+bonus(q->pl,existing,0):0;
        if(existing)memcpy(node,(void *)existing,sizeof node);
        else {node[0]=*(uint64_t *)(table+0x11e8)+(uint64_t)id*0x2ec;*(int32_t *)((uint8_t *)node+0x44)=-1;}
        *(int32_t *)((uint8_t *)node+0x38)=c->blvl[id]+1;
        int next=c->blvl[id]+1+bonus(q->pl,(uint64_t)node,0);
        if(own<0)own=0;
        if(dm_sim_skill_preview(d,c,&target,id,existing?own:0,next,de,line,sizeof line)>0)
            snprintf(sim_skill_lines[id],sizeof sim_skill_lines[id],"%s\n%s\n",line,heading);
    }
}
const char *dm_sim_skill_tooltip(int id) {
    return dm_custom_get("combat_sim") && id>0 && id<DM_SIM_MAXSKILL?sim_skill_lines[id]:"";
}
#include "dm_sim_bench.inc"
