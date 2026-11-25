#include "dm_sim_ui.h"
#include "dm_sim_text.h"
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

static pthread_mutex_t snapshot_lock=PTHREAD_MUTEX_INITIALIZER;
static dm_sim_snapshot snapshot;
void dm_sim_publish(const dm_sim_snapshot *s) {
    pthread_mutex_lock(&snapshot_lock);
    if(s)snapshot=*s;else memset(&snapshot,0,sizeof snapshot);
    pthread_mutex_unlock(&snapshot_lock);
}
int dm_sim_snapshot_copy(dm_sim_snapshot *out) {
    if(!out)return 0;
    pthread_mutex_lock(&snapshot_lock);*out=snapshot;pthread_mutex_unlock(&snapshot_lock);
    return out->valid;
}
int dm_sim_quest_words(const uint8_t *buffer,uint32_t bits,uint16_t out[48]) {
    if(!buffer || bits!=8*DM_SIM_QUEST_BYTES)return -1;
    for(int i=0;i<48;i++)out[i]=(uint16_t)(buffer[2*i]|buffer[2*i+1]<<8);
    return 0;
}
static pthread_mutex_t quest_lock=PTHREAD_MUTEX_INITIALIZER;
static struct {char name[16];uint16_t words[3][48];int state[3];} quests;
void dm_sim_quest_publish(const char *name,const uint16_t words[3][48],const int state[3]) {
    pthread_mutex_lock(&quest_lock);
    if(name){memcpy(quests.name,name,sizeof quests.name);memcpy(quests.words,words,sizeof quests.words);memcpy(quests.state,state,sizeof quests.state);}
    else memset(&quests,0,sizeof quests);
    pthread_mutex_unlock(&quest_lock);
}
void dm_sim_quest_next(const char *name,int out[3]) {
    pthread_mutex_lock(&quest_lock);
    int same=name && quests.name[0] && !strncmp(name,quests.name,sizeof quests.name);
    for(int d=0;d<3;d++)out[d]=same && quests.state[d]==0?dm_sim_next_boss(quests.words[d]):-1;
    pthread_mutex_unlock(&quest_lock);
}
int dm_sim_quest_copy(const char *name,uint16_t words[3][48],int state[3]) {
    pthread_mutex_lock(&quest_lock);
    int same=name && quests.name[0] && !strncmp(name,quests.name,sizeof quests.name);
    if(same){memcpy(words,quests.words,sizeof quests.words);memcpy(state,quests.state,sizeof quests.state);}
    pthread_mutex_unlock(&quest_lock);
    return same;
}
int dm_sim_next_boss(const uint16_t words[48]) {
    if(!words)return -1;
    const int word[5]={6,14,22,26,40};
    for(int i=0;i<5;i++)if(!(words[word[i]]&1))return i+1;
    return 0;
}
void dm_sim_bar_add(dm_sim_char *c,int id) {
    if(id<0 || id>=DM_SIM_MAXSKILL || !c->lvl[id])return;
    for(int i=0;i<c->nbar;i++)if(c->bar[i]==id)return;
    if(c->nbar<DM_SIM_MAXBAR)c->bar[c->nbar++]=id;
}
void dm_sim_bar_bind(dm_sim_char *c,int id) {
    dm_sim_bar_add(c,id);
    for(int i=0;i<c->nbar;i++)if(c->bar[i]==id)c->bound[id>>6]|=1ull<<(id&63);
}
typedef struct {char *s;size_t n,cap;int bad;} output;
__attribute__((format(printf,2,3)))
static void put(output *w,const char *fmt,...) {
    if(w->bad)return;
    va_list ap;va_start(ap,fmt);int n=vsnprintf(w->s+w->n,w->cap-w->n,fmt,ap);va_end(ap);
    if(n<0 || (size_t)n>=w->cap-w->n)w->bad=1;else w->n+=(size_t)n;
}
static void skill(output *w,const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,int id) {
    dm_sim_skill_out s={0};
    if(dm_sim_skill_eval(d,c,t,id,&s))s.flags|=DM_SF_UNAVAIL;
    put(w,"skill\t%d\t%s\t%.2f\t%d\t%.2f\t%.2f\t%d\n",id,d->skill[id]?d->skill[id]->name:"",s.dps,s.flags,s.mana_seconds,s.mana_x256/256.0,s.pets);
}

static int scaled(int value,double factor){return (int)fmin(1000000000.0,fmax(0,value*factor));}
static void target_scale(const dm_sim_data *d,const dm_sim_char *c,dm_sim_target *t,const dm_sim_options *opt) {
    if(opt->terror && t->mon>=0 && t->mlvl>0) {
        int level=c->clvl+opt->tz_boost;
        if(level<opt->tz_min)level=opt->tz_min;
        if(level>opt->tz_max)level=opt->tz_max;
        if(t->kind>=DM_TK_CHAMPION && t->kind<=DM_TK_SUPERUNIQUE)level+=t->kind==DM_TK_CHAMPION?2:3;
        if(level>t->mlvl && level<=110 && !(t->flags&DM_MF_NORATIO)) {
            double ratio[4];for(int i=0;i<4;i++){int col=c->diff+3*i;ratio[i]=d->monlvl[t->mlvl][col]>0?(double)d->monlvl[level][col]/d->monlvl[t->mlvl][col]:1;}
            t->hp=scaled(t->hp,ratio[0]);t->ac=scaled(t->ac,ratio[1]);t->ar=scaled(t->ar,ratio[2]);
            t->dmin=scaled(t->dmin,ratio[3]);t->dmax=scaled(t->dmax,ratio[3]);
            for(int i=0;i<3;i++){t->elmin[i]=scaled(t->elmin[i],ratio[3]);t->elmax[i]=scaled(t->elmax[i],ratio[3]);}t->mlvl=level;
        }
    }
    if(opt->herald && t->kind==DM_TK_NORMAL && t->mon>=0) {
        int excluded=0;for(int i=0;i<opt->nexcluded;i++)if(!strcmp(opt->excluded[i],d->mon[t->mon].id))excluded=1;
        if(!excluded){double damage=1+opt->herald_damage/100.0;t->hp=scaled(t->hp,1+opt->herald_hp/100.0);t->dmin=scaled(t->dmin,damage);t->dmax=scaled(t->dmax,damage);for(int i=0;i<3;i++){t->elmin[i]=scaled(t->elmin[i],damage);t->elmax[i]=scaled(t->elmax[i],damage);}t->flags|=DM_MF_UNDERESTIMATED;}
    }
}


static void trace_lines(output *w,const dm_sim_trace *tr) {
    put(w,"fight\t%d\t%d\t%d\t%d\t%d\n",tr->end_kind,tr->end,tr->width,tr->n,tr->best);
    for(int i=0;i<tr->nskill && i<64;i++)put(w,"act\t%d\t%d\n",i,tr->skill[i]);
    for(int i=0;i<tr->n;i++) {
        const dm_sim_trace_bucket *b=&tr->b[i];
        put(w,"trace\t%d",i);
        for(int k=0;k<DM_TS_N;k++)put(w,"\t%.1f,%.1f,%.1f",b->lo[k],b->hi[k],b->last[k]);
        put(w,"\t%d\t%llx\n",b->shortage,(unsigned long long)b->acts);
    }
    for(int i=0;i<tr->npotion;i++)put(w,"drink\t%d\t%d\t%.1f\t%.1f\n",tr->potion[i].frame,tr->potion[i].kind,tr->potion[i].life,tr->potion[i].mana);
}


static void look_key(char *out,size_t cap,const dm_sim_data *d,const dm_sim_target *t,int su) {
    out[0]=0;
    if(su>=0){snprintf(out,cap,"su:%d",d->su[su].idx);return;}
    if(t->kind==DM_TK_STD || t->mon<0 || t->mon>=d->nmon)return;
    size_t i=0;for(const char *p=d->mon[t->mon].id;*p && i+1<cap;p++)out[i++]=(char)tolower((unsigned char)*p);
    out[i]=0;
}





static void stats_line(output *w,const dm_sim_data *d,const dm_sim_target *t,int su,int players) {
    dm_sim_stats s;if(dm_sim_target_stats(d,t,su,&s))return;
    double strength=1+(players-1)/16.0;
    const dm_sim_mon *m=&d->mon[t->mon];
    put(w,"stats\t%d\t%d\t%d\t%d\t%d\t%d\t",t->mlvl,(int)fmin(1e9,s.hpmin*(players+1)/2.0),(int)fmin(1e9,s.hpmax*(players+1)/2.0),s.ac,s.block,s.drain);
    int n=0;
    for(int a=0;a<3;a++)if(s.amax[a]>0)put(w,"%s%d,%d,%d,%d",n++?";":"",a,(int)(s.amin[a]*strength),(int)(s.amax[a]*strength),(int)(s.ar[a]*strength));
    put(w,"%s\t",n?"":"-");n=0;
    for(int k=0;k<3;k++) {
        if(s.eltype[k]<0 || s.eltype[k]>=DM_EL_N || s.elmax[k]<=0)continue;
        int lo=(int)(s.elmin[k]*strength),hi=(int)(s.elmax[k]*strength),len=s.eldur[k];


        if(s.eltype[k]==DM_EL_POIS){len=2*s.eldur[k];lo=(int)((int64_t)lo*10*len/256);hi=(int)((int64_t)hi*10*len/256);}
        put(w,"%s%d,%d,%d,%d,%d,%d",n++?";":"",s.elslot[k],s.eltype[k],lo,hi,s.elpct[k],len);
    }
    put(w,"%s\t",n?"":"-");n=0;
    for(int k=0;k<8;k++)if(m->skills[k]>=0)put(w,"%s%d",n++?",":"",m->skills[k]);
    put(w,"%s\t",n?"":"-");n=0;
    for(int k=0;su>=0 && k<3;k++)if(d->su[su].mods[k]>0)put(w,"%s%d",n++?",":"",d->su[su].mods[k]);
    put(w,"%s\t",n?"":"-");n=0;
    const char *trait="udmr";const int flag[4]={DM_MF_UNDEAD,DM_MF_DEMON,DM_MF_MELEE,DM_MF_RANGED};
    for(int k=0;k<4;k++)if(m->flags&flag[k]){put(w,"%c",trait[k]);n++;}
    put(w,"%s\n",n?"":"-");
}
int dm_sim_kind_key(const dm_sim_data *d,int cls,int su,char *out,size_t cap) {
    if(!out || !cap)return -1;
    out[0]=0;
    if(!d)return -1;
    if(su>=0)for(int i=0;i<d->nsu;i++)if(d->su[i].idx==su)return snprintf(out,cap,"S%d",su)<(int)cap?0:-1;
    if(cls<0)return -1;
    return snprintf(out,cap,"M%d",cls)<(int)cap?0:-1;
}
static void row(output *w,const dm_sim_data *d,const dm_sim_char *c,dm_sim_target *t,int id,int su,const dm_sim_options *opt) {
    if(opt->target>=0 && id!=opt->target)return;
    target_scale(d,c,t,opt);

    t->hp=(int)fmin(1000000000.0,t->hp*(opt->players+1)/2.0);
    double strength=1+(opt->players-1)/16.0;
    t->ar=(int)(t->ar*strength);t->dmin=(int)(t->dmin*strength);t->dmax=(int)(t->dmax*strength);
    for(int e=0;e<3;e++){t->elmin[e]=(int)(t->elmin[e]*strength);t->elmax[e]=(int)(t->elmax[e]*strength);}
    dm_sim_rot_out r={0};dm_sim_surv_out s={0};dm_sim_light_out l={0};dm_sim_trace trace;
    if(dm_sim_fight(d,c,t,&r,&s,opt->target==id?&trace:NULL))return;
    dm_sim_light(t,r.ttk,&s,r.dps<=0 && r.ttk==-1,&l);
    int uncertain=r.ttk==-2 || s.ttd==-2 || (r.flags&DM_SF_UNAVAIL);
    char look[32],kind[16]="";look_key(look,sizeof look,d,t,su);
    if(t->kind!=DM_TK_STD)dm_sim_kind_key(d,t->mon,su>=0?d->su[su].idx:-1,kind,sizeof kind);
    put(w,"enemy\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.3f\t%.2f\t%d\t%d\t%d\t%d\t%d,%d,%d,%d,%d,%d\t%s\t%s\t%d\n",
        id,t->name,t->kind,t->act,t->area,t->mlvl,t->hp,r.dps,r.ttk,s.ttd,s.max_hit,s.hits_to_death,uncertain?-1:l.light,l.reason,s.underestimated,
        r.flags|((opt->players>1 || opt->potions || opt->terror || opt->herald)?DM_SF_ESTIMATE:0),t->res[0],t->res[1],t->res[2],t->res[3],t->res[4],t->res[5],look,kind,t->area_estimated?1:0);
    if(opt->target==id) {
        if(t->kind!=DM_TK_STD)stats_line(w,d,t,su,opt->players);
        skill(w,d,c,t,0);
        for(int i=0;i<c->nbar;i++)if(c->bar[i])skill(w,d,c,t,c->bar[i]);
        put(w,"rotation\t%.2f\t%d\t%.2f",r.dps,r.flags,r.mana_seconds);
        for(int i=0;i<r.nseq;i++)put(w,"\t%d",r.seq[i]);
        put(w,"\n");
        put(w,"survival\t%.2f\t%.2f\t%.2f\t%d\n",s.in_dps,s.sustain,s.dmg_hit,s.underestimated);
        if(trace.n>0)trace_lines(w,&trace);
        double taken[6]={0};dm_sim_damage_taken(d,c,t,taken);
        for(int e=0;e<6;e++){put(w,"damage\t%d\t%.2f\n",e,r.dps_element[e]);put(w,"taken\t%d\t%.2f\n",e,taken[e]);}
    }
}
int dm_sim_is_town(int area){return area==1||area==40||area==75||area==103||area==109;}
int dm_sim_ref_capture(dm_sim_ref *r,unsigned gen,const char *name,int act,int area) {
    if(gen!=r->gen || strcmp(name,r->name)){r->gen=gen;snprintf(r->name,sizeof r->name,"%s",name);memset(r->last,0,sizeof r->last);}
    int town=dm_sim_is_town(area);
    if(!town && area>0 && act>=1 && act<=5)r->last[act]=area;
    if(!town)return area;
    return act>=1 && act<=5?r->last[act]:0;
}
int dm_sim_ui_report(const dm_sim_data *d,const dm_sim_snapshot *snap,const dm_sim_options *opt,char *out,size_t cap) {
    if(!d || !d->ok || !snap || !snap->valid || !opt || !out || !cap || opt->diff<0 || opt->diff>2 || opt->players<1 || opt->players>8 || opt->norder<0 || opt->norder>DM_SIM_MAXBAR || opt->target < -1 || opt->potions<0 || opt->potions>1 || opt->ordered<0 || opt->ordered>1 || opt->terror<0 || opt->terror>1 || opt->herald<0 || opt->herald>5)return -1;
    output w={out,0,cap,0};out[0]=0;dm_sim_char c=snap->c;c.diff=opt->diff;c.potions=opt->potions;c.ordered=opt->ordered;
    if(opt->ordered && opt->norder) {
        c.nbar=0;for(int i=0;i<opt->norder;i++) {
            int present=0;for(int j=0;j<snap->c.nbar;j++)if(opt->order[i]==snap->c.bar[j])present=1;
            if(present)dm_sim_bar_add(&c,opt->order[i]);
        }
        for(int i=0;i<snap->c.nbar;i++)dm_sim_bar_add(&c,snap->c.bar[i]);
    }
    put(&w,"hero\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",snap->name,c.cls,c.clvl,c.diff,snap->act,(int)c.life,(int)c.mana,snap->rift,snap->rush,snap->next_boss_by_diff[c.diff],snap->session,snap->set,opt->players,snap->area,snap->ref_area,snap->elysium?1:0);
    put(&w,"loot\t%d\n",snap->magic_find);
    if(snap->item_charges)put(&w,"note\tcharges\n");
    if(c.capture_bad&8)put(&w,"belt\t-1\n");
    else {put(&w,"belt");for(int k=0;k<DM_POT_N;k++)put(&w,"\t%d",c.belt[k]);put(&w,"\n");}

    {int h[DM_SIM_MAXBAR][2],n=dm_sim_bar_hands(d,&c,h,DM_SIM_MAXBAR);for(int i=0;i<n;i++)put(&w,"hands\t%d\t%d\n",h[i][0],h[i][1]);}
    dm_sim_target t;
    c.act=snap->act;
    if(dm_sim_target_act(d,&c,&t)>=0) {
        if(opt->target<0) {
            dm_sim_rot_out r={0};dm_sim_rotation(d,&c,&t,&r);
            put(&w,"rotation\t%.2f\t%d\t%.2f",r.dps,r.flags,r.mana_seconds);
            for(int i=0;i<r.nseq;i++)put(&w,"\t%d",r.seq[i]);
            put(&w,"\n");
            for(int i=0;i<c.nbar;i++)skill(&w,d,&c,&t,c.bar[i]);
        }
        row(&w,d,&c,&t,0,-1,opt);
    }
    int id=1;
    for(int i=0;i<d->nspawn;i++)for(int j=0;j<d->spawn[i].n;j++) {
        if(!dm_sim_target_spawn(d,i,j,c.diff,DM_TK_NORMAL,&t))row(&w,d,&c,&t,id,-1,opt);
        id++;
    }
    for(int i=0;i<d->nsu;i++,id++)if(!dm_sim_target_su(d,i,c.diff,&t))row(&w,d,&c,&t,id,i,opt);
    for(int i=0;i<d->nboss;i++,id++) {
        const dm_sim_boss *b=&d->boss[i];int kinds[]={DM_TK_ACTBOSS,DM_TK_QUEST,DM_TK_UBER,DM_TK_ANCIENT,DM_TK_CLONE};
        if(c.diff!=2 && (b->kind==2 || b->kind==3 || b->kind==4))continue;
        if(!dm_sim_target_mon(d,b->mon,c.diff,0,kinds[b->kind],1,&t)){t.act=b->act;t.area=b->area;row(&w,d,&c,&t,id,-1,opt);}
    }
    if(w.bad){out[0]=0;return -2;}return (int)w.n;
}
static int options_load(dm_sim_options *o,int rotw) {
    if(!o->terror && !o->herald)return 0;
    if(o->herald<0 || o->herald>5)return -1;
    if(!rotw || o->diff!=2)o->herald=0;
    extern int dm_overlay_file(const char *,char *,size_t);
    char path[1200],line[256],set[8];if(!dm_overlay_file("sim-ui.tsv",path,sizeof path))return -1;
    FILE *f=fopen(path,"r");if(!f)return -1;
    int rows=0,end=0,tz=0,herald=!o->herald,bad=0;
    while(fgets(line,sizeof line,f)) {
        rows++;if(rows==1){if(strcmp(line,"DM_SIM_UI\t1\n"))bad=1;continue;}
        int diff,a,b,c;
        if(sscanf(line,"tz\t%7s\t%d\t%d\t%d\t%d",set,&diff,&a,&b,&c)==5) {
            if(!strcmp(set,rotw?"rotw":"lod") && diff==o->diff){if(a<1 || b<a || b>110 || c<0 || c>10)bad=1;o->tz_min=a;o->tz_max=b;o->tz_boost=c;tz=1;}
        } else if(sscanf(line,"herald\t%7s\t%d\t%d\t%d\t%d",set,&diff,&a,&b,&c)==5) {
            if(!strcmp(set,rotw?"rotw":"lod") && diff==o->diff && a==o->herald){if(b<0 || b>10000 || c<0 || c>10000)bad=1;o->herald_hp=b;o->herald_damage=c;herald=1;}
        } else if(!strncmp(line,"exclude\t",8)) {
            char mon[24];if(sscanf(line,"exclude\t%7s\t%23s",set,mon)!=2)bad=1;
            else if(!strcmp(set,rotw?"rotw":"lod")){if(o->nexcluded>=1024)bad=1;else snprintf(o->excluded[o->nexcluded++],24,"%s",mon);}
        } else if(sscanf(line,"end\t%d",&end)!=1)bad=1;
    }
    if(ferror(f))bad=1;
    fclose(f);return !bad && end==rows && tz && herald?0:-1;
}


enum { REPORTS = 4 };
static struct {unsigned generation;dm_sim_snapshot snap;dm_sim_options opt;char *text;int n;unsigned used;} reports[REPORTS];
static pthread_mutex_t report_lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned report_clock;
static int report_find(unsigned generation,const dm_sim_snapshot *snap,const dm_sim_options *opt) {
    for(int i=0;i<REPORTS;i++)if(reports[i].text && reports[i].generation==generation && !memcmp(&reports[i].snap,snap,sizeof *snap) && !memcmp(&reports[i].opt,opt,sizeof *opt))return i;
    return -1;
}
typedef struct {dm_sim_snapshot snap;const dm_sim_options *opt;char *out;size_t cap;} request;

static int read_report(const dm_sim_data *d,void *ctx) {
    request *q=ctx;
    pthread_mutex_lock(&report_lock);
    int hit=report_find(d->generation,&q->snap,q->opt),rc=hit>=0 && (size_t)reports[hit].n<q->cap?reports[hit].n:-1;
    if(rc>=0){memcpy(q->out,reports[hit].text,(size_t)rc+1);reports[hit].used=++report_clock;}
    pthread_mutex_unlock(&report_lock);
    if(rc>=0)return rc;
    rc=dm_sim_ui_report(d,&q->snap,q->opt,q->out,q->cap);
    if(rc<0)return rc;
    char *copy=malloc((size_t)rc+1);if(!copy)return rc;
    memcpy(copy,q->out,(size_t)rc);copy[rc]=0;
    pthread_mutex_lock(&report_lock);
    int slot=report_find(d->generation,&q->snap,q->opt);
    if(slot<0){slot=0;for(int i=1;i<REPORTS;i++)if(reports[i].used<reports[slot].used)slot=i;}
    free(reports[slot].text);reports[slot].generation=d->generation;reports[slot].snap=q->snap;reports[slot].opt=*q->opt;reports[slot].text=copy;reports[slot].n=rc;reports[slot].used=++report_clock;
    pthread_mutex_unlock(&report_lock);
    return rc;
}
int dm_sim_ui_query(const dm_sim_options *opt,char *out,size_t cap) {
    if(!opt || !out || !cap)return -1;
    request q={.opt=opt,.out=out,.cap=cap};
    if(!dm_sim_snapshot_copy(&q.snap))return -1;
    dm_sim_options resolved=*opt;if(resolved.diff<0)resolved.diff=q.snap.c.diff;if(resolved.players<0)resolved.players=q.snap.players; if(options_load(&resolved,q.snap.set))return -1;q.opt=&resolved;
    return dm_sim_read(q.snap.set?"rotw":"lod",read_report,&q);
}

uint64_t dm_sim_tip_key(const dm_sim_data *d,int diff,int area,int clvl,uint64_t key) {
    dm_sim_char c;c.diff=diff;c.act=dm_sim_area_act(d,area);c.clvl=clvl;dm_sim_target t;
    int act=diff>=0 && diff<=2?dm_sim_target_act(d,&c,&t):-1,level=act>=0?t.mlvl:0;
    return (key ^ (uint64_t)(uint32_t)(diff*8+act+1)<<40 ^ (uint64_t)(uint32_t)level<<48)*1099511628211ull;
}
int dm_sim_skill_preview(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,int id,int own_level,int next_level,int german,char *out,size_t cap) {
    out[0]=0;
    if(id<=0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || own_level<0 || next_level<1 || next_level>256)return 0;
    dm_sim_skill_out before={0},after={0};
    dm_sim_char probe=*c;
    if(own_level>0){probe.lvl[id]=(int16_t)own_level;if(dm_sim_skill_eval(d,&probe,t,id,&before))return 0;}
    probe.blvl[id]=(int16_t)(c->blvl[id]+1);probe.lvl[id]=(int16_t)next_level;
    if(dm_sim_skill_eval(d,&probe,t,id,&after))return 0;
    if((before.flags|after.flags)&(DM_SF_UNAVAIL|DM_SF_MODIFIER|DM_SF_UTILITY))return 0;
    return own_level>0?dm_sim_skill_delta_text(out,cap,before.dps,after.dps,german):dm_sim_skill_first_text(out,cap,after.dps,german);
}
