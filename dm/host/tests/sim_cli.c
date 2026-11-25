#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include <string.h>
#include <time.h>
#include <math.h>
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static int compare(const void *a,const void *b){double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
static void bench(const dm_sim_data *d,dm_sim_char *c) {
    dm_sim_char expanded=*c;c=&expanded;
    int ids[12]={0},count=1;
    for(int i=0;i<c->nbar && count<12;i++){int duplicate=0;for(int j=0;j<count;j++)if(ids[j]==c->bar[i])duplicate=1;if(!duplicate)ids[count++]=c->bar[i];}
    for(int i=0;i<d->nskill && count<12;i++)if(d->skills[i].cls==c->cls) {
        int id=d->skills[i].id,duplicate=0;for(int j=0;j<count;j++)if(ids[j]==id)duplicate=1;
        if(!duplicate){ids[count++]=id;if(!c->lvl[id]){c->lvl[id]=20;c->blvl[id]=1;}}
    }
    c->nbar=count-1;for(int i=1;i<count;i++)c->bar[i-1]=ids[i];
    double samples[100],checksum=0;dm_sim_target t;dm_sim_target_std(d,c->diff,0,&t);
    for(int k=0;k<100;k++) {
        double start=seconds();
        for(int state=0;state<2;state++) {
            dm_sim_char x=*c;x.ed+=state*20;
            for(int j=0;j<12;j++){int id=ids[j%count];dm_sim_skill_out o;dm_sim_skill_eval(d,&x,&t,id,&o);checksum+=o.dps;}
            dm_sim_rot_out r;dm_sim_surv_out s;dm_sim_rotation(d,&x,&t,&r);dm_sim_survive(d,&x,&t,&r,&s);checksum+=r.dps+s.in_dps;
        }
        samples[k]=(seconds()-start)*1000;
    }
    qsort(samples,100,sizeof *samples,compare);
    printf("tooltip_ms median=%.4f p99=%.4f max=%.4f cases=100 rows=12 states=2\n",samples[50],samples[98],samples[99]);
    double start=seconds();int rows=0;
    for(int i=0;i<d->nspawn;i++) {
        const dm_sim_spawn *s=&d->spawn[i];if(s->list!=(c->diff>0))continue;
        int lv=0;for(int a=0;a<d->narea;a++)if(d->area[a].id==s->area)lv=d->area[a].mlvl[c->diff];
        for(int j=0;j<s->n;j++) {
            dm_sim_target enemy;if(dm_sim_target_mon(d,s->mon[j],c->diff,lv,DM_TK_NORMAL,1,&enemy))continue;
            for(int k=0;k<10;k++){dm_sim_skill_out o;dm_sim_skill_eval(d,c,&enemy,ids[k%count],&o);checksum+=o.dps;}rows++;
        }
    }
    printf("menu_ms %.3f rows=%d skills=10 checksum=%.3f\n",(seconds()-start)*1000,rows,checksum);
    printf("arm64_estimate_ms p99_x1_to_x4=%.4f..%.4f unmeasured\n",samples[98],4*samples[98]);
}
int main(int argc,char **argv) {
    const char *path=NULL,*prof=NULL,*set="rotw";int diff=-1,benchmark=0;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--bench"))benchmark=1;
        else if(!strcmp(argv[i],"--set") && i+1<argc)set=argv[++i];
        else if(!strcmp(argv[i],"--diff") && i+1<argc){const char *v=argv[++i];diff=!strcmp(v,"hell")?2:!strcmp(v,"nightmare")?1:!strcmp(v,"normal")?0:-1;if(diff<0)return 2;}
        else if(!path)path=argv[i];else if(!prof)prof=argv[i];else return 2;
    }
    if(!path)path=getenv("DM_SIM_DATA");
    if(!path){fprintf(stderr,"usage: sim_cli sim.tsv [profile.prof] [--set rotw|lod] [--diff normal|nightmare|hell] [--bench]\n");return 2;}
    size_t n;char *text=sim_read(path,&n);dm_sim_data *d=calloc(1,sizeof *d);
    if(!text || !d){free(text);free(d);return 1;}
    if(dm_sim_load(d,text,n,set)){fprintf(stderr,"%s\n",d->err);free(text);free(d);return 1;}free(text);
    dm_sim_char c;dm_sim_char_init(&c,1);c.clvl=80;c.diff=2;c.life=1200;c.mana=700;c.fcr=105;
    int id=47;if(d->skill[id]){c.lvl[id]=20;c.blvl[id]=20;c.bar[c.nbar++]=id;}
    if(prof){text=sim_read(prof,&n);char err[128];if(!text || dm_sim_profile_parse(d,text,&c,err,sizeof err)){fprintf(stderr,"profile unavailable or invalid\n");free(text);dm_sim_free(d);free(d);return 1;}free(text);}
    if(diff>=0)c.diff=diff;
    if(benchmark)bench(d,&c);
    else {char *out=malloc(262144);int rc=out?dm_sim_report(d,&c,1,out,262144):-1;if(rc<0){free(out);dm_sim_free(d);free(d);return 1;}fputs(out,stdout);free(out);}
    dm_sim_free(d);free(d);return 0;
}
