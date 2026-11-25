#include "sim_test_io.h"
#include <math.h>
#include <string.h>

static const char *weapons[]={"HTH","1HS","1HT","2HS","2HT","STF","BOW","XBW","HT1","HT2"};
static void variant(dm_sim_char *c,int index) {
    dm_sim_weapon *w=&c->hand[0];
    if(index<10) {
        snprintf(w->wclass,sizeof w->wclass,"%s",weapons[index]);
        w->kind=index==0?DM_WK_NONE:index==6?DM_WK_BOW:index==7?DM_WK_XBOW:index>=8?DM_WK_CLAW:DM_WK_MELEE;
        w->twohand=index>=3 && index<=7;
    } else if(index==10){c->nhands=2;c->hand[1]=c->hand[0];}
    else if(index==11)w->kind=DM_WK_THROW;
    else if(index<15)c->ias=59+index-12;
    else if(index<18)c->fcr=104+index-15;
    else if(index==18)c->ed=150;
    else if(index==19){w->min+=20;w->max+=20;}
    else if(index==20)for(int e=1;e<DM_EL_N;e++){w->elmin[e]=5;w->elmax[e]=10;}
    else if(index==21){c->cs=50;c->ds=50;}
    else if(index==22)c->cb=50;
    else if(index==23)c->ow=50;
    else if(index==24)for(int e=0;e<DM_EL_N;e++)c->pierce[e]=50;
    else if(index==25)for(int e=0;e<DM_EL_N;e++)c->sunder[e]=1;
    else if(index==26)c->ll=10;
    else if(index==27){c->def=2000;c->block=75;c->dr_flat=20;c->mdr_flat=20;}
    else if(index==28)for(int e=0;e<DM_EL_N;e++){c->res[e]=75;c->absorb_flat[e]=10;c->absorb_pct[e]=20;}
    else if(index==29)c->form=1;
    else if(index==30)c->form=2;
}
int main(int argc,char **argv) {
    if(argc!=2){fputs("usage: sim_matrix sim.tsv\n",stderr);return 2;}
    size_t size;char *text=sim_read(argv[1],&size);dm_sim_data *d=calloc(1,sizeof *d);
    if(!text || !d)return 1;
    if(dm_sim_load(d,text,size,"rotw")){fprintf(stderr,"%s\n",d->err);return 1;}free(text);
    if(d->nboss<1){fputs("No boss targets available\n",stderr);dm_sim_free(d);free(d);return 1;}
    puts("class,skill,family,variant,difficulty,monster,flags,predicted_dps,predicted_ttk,observed_n,verdict");
    unsigned count=0;
    for(int i=0;i<d->nskill;i++) {
        const dm_sim_skill *s=&d->skills[i];if(s->cls<0 || !s->fam)continue;
        for(int diff=0;diff<3;diff++)for(int v=0;v<32;v++) {
            dm_sim_char c;dm_sim_char_init(&c,s->cls);c.clvl=80;c.diff=diff;c.life=1500;c.mana=1000;
            c.str=c.dex=100;c.ar=1000;c.hand[0].min=100;c.hand[0].max=150;
            c.hand[0].kind=DM_WK_MELEE;strcpy(c.hand[0].wclass,"1HS");
            c.shield=1;c.shield_min=20;c.shield_max=30;c.kick_min=20;c.kick_max=30;
            c.lvl[s->id]=v==31?23:20;c.blvl[s->id]=20;c.bar[c.nbar++]=s->id;
            variant(&c,v);
            dm_sim_target target;int rc;
            if(v<10)rc=dm_sim_target_std(d,diff,0,&target);
            else {const dm_sim_boss *b=&d->boss[(i+v)%d->nboss];const int kinds[]={DM_TK_ACTBOSS,DM_TK_QUEST,DM_TK_UBER,DM_TK_ANCIENT,DM_TK_CLONE};if(b->kind<0 || b->kind>4)return 1;rc=dm_sim_target_mon(d,b->mon,diff,80,kinds[b->kind],1,&target);}
            if(rc){fprintf(stderr,"target error skill=%d variant=%d\n",s->id,v);return 1;}
            dm_sim_skill_out out;
            if(dm_sim_skill_eval(d,&c,&target,s->id,&out) || !isfinite(out.dps) || !isfinite(out.ttk))return 1;
            printf("%d,%d,%u,%d,%d,%d,%d,%.9f,%.9f,0,unmeasured_synthetic\n",s->cls,s->id,s->fam,v,diff,target.mon,out.flags,out.dps,out.ttk);
            count++;
        }
    }
    fprintf(stderr,"model rows: %u; emulator measurements: 0\n",count);
    dm_sim_free(d);free(d);return 0;
}
