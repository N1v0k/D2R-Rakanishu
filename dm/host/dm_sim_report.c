#include "dm_sim_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>

typedef struct {const char *key;size_t offset;int lo,hi;} field;
#define FIELD(k,m,l,h) {k,offsetof(dm_sim_char,m),l,h}
static const field fields[]={
    FIELD("class",cls,0,7),FIELD("level",clvl,1,99),FIELD("diff",diff,0,2),FIELD("players",players,1,1),
    FIELD("str",str,0,10000),FIELD("dex",dex,0,10000),
    FIELD("vit",vit,0,10000),FIELD("ene",ene,0,10000),FIELD("ar",ar,0,1000000),FIELD("def",def,0,1000000),FIELD("block",block,0,75),
    FIELD("ias",ias,0,10000),FIELD("fcr",fcr,0,10000),FIELD("fhr",fhr,0,10000),FIELD("fbr",fbr,0,10000),FIELD("sias",sias,-100,1000),
    FIELD("ed",ed,-100,10000),FIELD("ds",ds,0,100),FIELD("cs",cs,0,100),FIELD("cb",cb,0,100),FIELD("ow",ow,0,100),
    FIELD("ll",ll,0,100),FIELD("ml",ml,0,100),FIELD("liferegen",liferegen,0,10000),FIELD("manaregen",manaregen_pct,0,10000),
    FIELD("dr",dr_flat,0,10000),FIELD("mdr",mdr_flat,0,10000),FIELD("shield",shield,0,1),FIELD("shield_min",shield_min,0,10000),
    FIELD("shield_max",shield_max,0,10000),FIELD("kick_min",kick_min,0,10000),FIELD("kick_max",kick_max,0,10000),
    FIELD("form",form,0,2),FIELD("charges",charges,0,3),FIELD("es",es_pct,0,95),FIELD("es_ratio",es_ratio,1,32),
    FIELD("bone_pool",bone_pool,0,100000),FIELD("cyclone_pool",cyclone_pool,0,100000),FIELD("ward_pool",ward_pool,0,100000),
    FIELD("poison_frames",poison_frames,0,15000),FIELD("missile_frame",missile_frame,0,15000),FIELD("prevent_heal",prevent_heal,0,1),FIELD("dodge",dodge,0,100),FIELD("mastery_cs",mastery_cs,0,100),FIELD("mercenary",mercenary,0,2),
    FIELD("potions",potions,0,1),FIELD("act",act,0,5)
};
static int integer(const char *s,int lo,int hi,int *v) {
    char *end;errno=0;long n=strtol(s,&end,10);
    if(!*s || *end || errno || n<lo || n>hi)return -1;
    *v=(int)n;return 0;
}
static int skill_id(const dm_sim_data *d,const char *s) {
    int id=dm_sim_skill_by_name(d,s);if(id>=0)return id;
    if(integer(s,0,DM_SIM_MAXSKILL-1,&id) || !d->skill[id])return -1;
    return id;
}
static int list(char *s,int *v,int n,int lo,int hi) {
    for(int i=0;i<n;i++) {
        char *p=strchr(s,',');if(p)*p=0;
        if(integer(s,lo,hi,&v[i]) || (i<n-1 && !p) || (i==n-1 && p))return -1;
        s=p?p+1:s;
    }
    return 0;
}
int dm_sim_profile_parse(const dm_sim_data *d,const char *text,dm_sim_char *c,char *err,size_t cap) {
    if(!d || !text || !c || strlen(text)>65536)return -1;
    dm_sim_char_init(c,0);size_t n=strlen(text);char *buf=malloc(n+1);if(!buf)return -1;memcpy(buf,text,n+1);
    int line=0,rc=-1;char *s=buf;
    while(s && *s) {
        char *next=strchr(s,'\n');if(next)*next++=0;line++;
        size_t len=strlen(s);if(len && s[len-1]=='\r')s[len-1]=0;
        if(!*s || *s=='#'){s=next;continue;}
        char *eq=strchr(s,'=');if(!eq)goto done;*eq++=0;
        int found=0;
        for(size_t i=0;i<sizeof fields/sizeof *fields;i++)if(!strcmp(s,fields[i].key)) {
            if(integer(eq,fields[i].lo,fields[i].hi,(int *)((char *)c+fields[i].offset)))goto done;
            found=1;break;
        }
        if(found){s=next;continue;}
        if(!strcmp(s,"life") || !strcmp(s,"mana")) {
            char *end;errno=0;double value=strtod(eq,&end);
            if(!*eq || *end || errno || !isfinite(value) || value<(!strcmp(s,"life")?1:0) || value>1000000 || floor(value*256)!=value*256)goto done;
            if(!strcmp(s,"life"))c->life=value;else c->mana=value;
        }
        else if(!strcmp(s,"set")){if((strcmp(eq,"rotw") && strcmp(eq,"lod")) || (d->set_name[0] && strcmp(eq,d->set_name)))goto done;}
        else if(!strcmp(s,"skill")) {
            char *comma=strchr(eq,',');if(!comma)goto done;*comma++=0;int id=skill_id(d,eq),v[2];
            if(id<0 || list(comma,v,2,0,256) || v[1]>v[0] || v[1]>20)goto done;
            c->lvl[id]=(int16_t)v[0];c->blvl[id]=(int16_t)v[1];
        } else if(!strcmp(s,"bar")) {
            int id=skill_id(d,eq);if(id<0 || c->nbar>=DM_SIM_MAXBAR)goto done;c->bar[c->nbar++]=id;
        } else if(!strcmp(s,"stat") || !strcmp(s,"active_level")) {
            char *comma=strchr(eq,',');if(!comma)goto done;*comma++=0;int v;
            if(integer(comma,0,1000000,&v))goto done;
            if(!strcmp(s,"active_level")){int id=skill_id(d,eq);if(id<0 || v>256)goto done;c->active_lvl[id]=(int16_t)v;}
            else {int id=-1;for(int j=0;j<512;j++)if(!strcmp(eq,d->stat_name[j]))id=j;if(id<0)goto done;c->stat_value[id]=v;}
        } else if(!strcmp(s,"belt")) {

            static const char *const codes[DM_POT_N]={"hp1","hp2","hp3","hp4","hp5","mp1","mp2","mp3","mp4","mp5","rvs","rvl"};
            int total=0;
            for(char *item=eq,*rest;item;item=rest) {
                rest=strchr(item,',');if(rest)*rest++=0;
                char *colon=strchr(item,':');int k=-1,count;if(!colon)goto done;*colon++=0;
                for(int j=0;j<DM_POT_N;j++)if(!strcmp(item,codes[j]))k=j;
                if(k<0 || integer(colon,0,16,&count))goto done;
                c->belt[k]+=count;total+=count;
            }
            if(total>16)goto done;
        } else if(!strcmp(s,"aura")) {int id=skill_id(d,eq);if(id<0 || !d->skill[id]->aura)goto done;c->active_aura=id;}
        else if(!strcmp(s,"active")) {int id=skill_id(d,eq);if(id<0 || c->nactive_buff>=8)goto done;c->active_buff[c->nactive_buff++]=id;}
        else if(!strcmp(s,"grant")) {


            static const char *const kinds[DM_GR_N]={"aura","buff","charge","attack","hit","struck"};
            char *p=strchr(eq,',');if(!p || c->ngrant>=DM_SIM_MAXGRANT)goto done;*p++=0;
            int kind=-1,set2=!strcmp(eq,"obuff");for(int k=0;k<DM_GR_N;k++)if(!strcmp(eq,kinds[k]))kind=k;
            if(set2)kind=DM_GR_BUFF;
            char *q=strchr(p,',');if(kind<0 || !q)goto done;*q++=0;
            int id=skill_id(d,p),v[3];if(id<0 || list(q,v,3,0,256) || v[0]<1)goto done;
            if((kind==DM_GR_AURA && !d->skill[id]->aura) || (kind>=DM_GR_ATTACK && (v[1]<1 || v[1]>100)) || (kind==DM_GR_CHARGE && v[1]<1))goto done;
            dm_sim_grant *g=&c->grant[c->ngrant++];g->kind=(int16_t)kind;g->skill=(int16_t)id;g->lvl=(int16_t)v[0];g->held=(int16_t)v[2];g->set2=(int16_t)set2;
            if(kind==DM_GR_CHARGE)g->charges=(int16_t)v[1];else g->chance=(int16_t)v[1];
        }
        else if(!strcmp(s,"weapon") || !strcmp(s,"offhand")) {
            int h=!strcmp(s,"offhand"),v[8];char *comma=strrchr(eq,',');if(!comma)goto done;*comma++=0;
            if(strlen(comma)>3 || list(eq,v,8,-100,10000))goto done;
            if(v[0]<0 || v[1]<v[0] || v[5]<0 || v[5]>DM_WK_CLAW || v[6]<0 || v[6]>1)goto done;
            dm_sim_weapon *w=&c->hand[h];w->min=v[0];w->max=v[1];w->strbonus=v[2];w->dexbonus=v[3];w->wsm=v[4];w->kind=v[5];w->twohand=v[6];
            w->ias=v[7];if(w->ias<0)goto done;
            int known=0;for(int j=0;j<d->nanim;j++)if(!strcmp(comma,d->anim[j].wclass))known=1;if(!known)goto done;
            strcpy(w->wclass,comma);if(h)c->nhands=2;
        } else if(!strcmp(s,"weapon_delta") || !strcmp(s,"offhand_delta")) {

            int v[4];dm_sim_weapon *w=&c->hand[!strcmp(s,"offhand_delta")];
            if(list(eq,v,4,-100000,100000))goto done;
            w->ar_base_d=v[0];w->ar_pct_d=v[1];w->ed_d=v[2];w->ds_d=v[3];
        } else if(!strcmp(s,"weapon_mastery_cs") || !strcmp(s,"offhand_mastery_cs")) {

            int v[1];if(list(eq,v,1,-100,100))goto done;
            c->hand[!strcmp(s,"offhand_mastery_cs")].cs_d=v[0];
        } else if(!strcmp(s,"weapon_extra") || !strcmp(s,"offhand_extra")) {


            int v[7];dm_sim_weapon *w=&c->hand[!strcmp(s,"offhand_extra")];
            if(list(eq,v,7,-100000,100000))goto done;
            w->cb_d=v[0];w->ow_d=v[1];w->ll_d=v[2];w->ml_d=v[3];w->pmh_d=v[4];w->demon_d=v[5];w->undead_d=v[6];
        } else {
            int *arr=NULL;
            if(!strcmp(s,"res"))arr=c->res;else if(!strcmp(s,"maxres"))arr=c->maxres;
            else if(!strcmp(s,"mastery"))arr=c->mastery;else if(!strcmp(s,"pierce"))arr=c->pierce;else if(!strcmp(s,"sunder"))arr=c->sunder;
            else if(!strcmp(s,"absorb_pct"))arr=c->absorb_pct;else if(!strcmp(s,"absorb_flat"))arr=c->absorb_flat;
            else if(!strcmp(s,"weapon_elmin"))arr=c->hand[0].elmin;else if(!strcmp(s,"weapon_elmax"))arr=c->hand[0].elmax;
            else if(!strcmp(s,"offhand_elmin"))arr=c->hand[1].elmin;else if(!strcmp(s,"offhand_elmax"))arr=c->hand[1].elmax;
            if(!arr || list(eq,arr,6,-1000,100000))goto done;
        }
        s=next;
    }
    if(c->cls>=d->ncls || (!strcmp(d->set_name,"lod") && c->cls==7))goto done;
    for(int i=0;i<c->nbar;i++)if(!c->lvl[c->bar[i]])goto done;
    rc=0;
done:
    if(rc && err && cap)snprintf(err,cap,"invalid profile at line %d",line);
    free(buf);return rc;
}
typedef struct {char *s;size_t cap,n;int bad;} writer;
static void put(writer *w,const char *fmt,...) {
    if(w->bad)return;
    va_list ap;va_start(ap,fmt);int n=vsnprintf(w->s+w->n,w->cap-w->n,fmt,ap);va_end(ap);
    if(n<0 || (size_t)n>=w->cap-w->n){w->bad=1;return;}w->n+=(size_t)n;
}
static void fight_row(writer *w,const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t) {
    dm_sim_rot_out r;dm_sim_surv_out s;dm_sim_light_out l;
    if(dm_sim_rotation(d,c,t,&r) || dm_sim_survive(d,c,t,&r,&s)){w->bad=1;return;}
    dm_sim_light(t,r.ttk,&s,r.dps<=0 && r.ttk==-1,&l);
    put(w,"fight\t%s\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.3f\t%.2f\t%s\t%d\t%d\t%d,%d,%d,%d,%d,%d\n",
        t->name,t->mlvl,t->hp,r.dps,r.ttk,s.ttd,s.max_hit,s.hits_to_death,
        l.light==DM_LIGHT_UNKNOWN?"unknown":l.light==DM_LIGHT_GREEN?"green":l.light==DM_LIGHT_ORANGE?"orange":"red",l.reason,s.underestimated,
        t->res[0]>=100,t->res[1]>=100,t->res[2]>=100,t->res[3]>=100,t->res[4]>=100,t->res[5]>=100);
}
int dm_sim_report(const dm_sim_data *d,const dm_sim_char *in,int bosses,char *out,size_t cap) {
    if(!out || !cap || !sim_valid_char(d,in))return -1;
    writer w={out,cap,0,0};out[0]=0;
    put(&w,"profile\tclass=%d\tlevel=%d\tlife=%.17g\tmana=%.17g\tplayers=1\n",in->cls,in->clvl,in->life,in->mana);
    put(&w,"flags\t1=unavailable 2=estimate 4=sequence 8=immune 16=no-target 32=modifier 64=utility 128=no-weapon 256=pet 512=corpse 1024=percent-life 2048=mana-short 4096=timed-damage 8192=minion\n");
    put(&w,"columns\tskill\tname\tdps\tframes\tmana\tflags\tttk\tmana_seconds\tcb_fraction\tstatic_fraction\tsurvival_delta\tdps_phys,fire,light,cold,poison,magic\n");
    for(int df=0;df<3;df++) {
        dm_sim_char c=*in;c.diff=df;dm_sim_target t;if(dm_sim_target_act(d,&c,&t)<0)return -1;
        put(&w,"difficulty\t%d\timmune_share=%d,%d,%d,%d,%d,%d\n",df,t.immune_share[0],t.immune_share[1],t.immune_share[2],t.immune_share[3],t.immune_share[4],t.immune_share[5]);
        for(int i=-1;i<c.nbar;i++) {
            int id=i<0?0:c.bar[i];dm_sim_skill_out s;dm_sim_skill_eval(d,&c,&t,id,&s);
            if(s.flags&DM_SF_UNAVAIL){put(&w,"skill\t%s\tunavailable\n",d->skill[id]->name);continue;}
            put(&w,"skill\t%s\t%.2f\t%d\t%.2f\t%d\t%.2f\t%.2f\t%.6f\t%.4f\t%.2f\t%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",d->skill[id]->name,s.dps,s.frames,s.mana_x256/256.0,s.flags,s.ttk,s.mana_seconds,s.cb_frac,s.sf_frac,s.survival_delta,s.dps_element[0],s.dps_element[1],s.dps_element[2],s.dps_element[3],s.dps_element[4],s.dps_element[5]);
        }
        dm_sim_rot_out r;dm_sim_rotation(d,&c,&t,&r);
        put(&w,"rotation\t%.2f\t%.2f\tmana_short=%.4f\tmana_seconds=%.2f\tflags=%d",r.dps,r.ttk,r.mana_short,r.mana_seconds,r.flags);
        for(int k=0;k<r.nseq;k++)put(&w,"\t%s",d->skill[r.seq[k]]->name);
        put(&w,"\n");fight_row(&w,d,&c,&t);
    }
    if(bosses) {
        put(&w,"columns\tfight\tname\tlevel\thp\tdps\tttk\tttd\tmax_hit_ratio\thits_to_death\tlight\treason\tunderestimated\timmunities\n");
        for(int i=0;i<d->nboss;i++) {
            const dm_sim_boss *b=&d->boss[i];dm_sim_target t;
            int kind=b->kind==DM_BK_ACTBOSS?DM_TK_ACTBOSS:b->kind==DM_BK_QUEST?DM_TK_QUEST:b->kind==DM_BK_UBER?DM_TK_UBER:b->kind==DM_BK_ANCIENT?DM_TK_ANCIENT:DM_TK_CLONE;
            if(dm_sim_target_mon(d,b->mon,in->diff,0,kind,1,&t))return -1;
            fight_row(&w,d,in,&t);
        }
        for(int i=0;i<d->nsu;i++){dm_sim_target t;if(dm_sim_target_su(d,i,in->diff,&t))return -1;
            fight_row(&w,d,in,&t);}
    }
    if(w.bad){out[0]=0;return -2;}return (int)w.n;
}
