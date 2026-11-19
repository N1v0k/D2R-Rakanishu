#include "sim_test_io.h"
#include <math.h>
#include <string.h>

static uint64_t state=0x9e3779b97f4a7c15ull;
static uint32_t random_word(void) {
    state^=state>>12;state^=state<<25;state^=state>>27;
    return (uint32_t)((state*2685821657736338717ull)>>32);
}
static int strike(int critical,int resistance,int chance,int fixed) {
    if(random_word()%100>=(unsigned)chance)return 0;
    int value=100*256+(int)(random_word()%(fixed?256:50*256));
    if(critical && (random_word()%100<50 || random_word()%100<50))value*=2;
    return value*(100-resistance)/100;
}
typedef struct {double mean,m2;int n;} sample;
static void observe(sample *s,double value) {
    s->n++;double delta=value-s->mean;s->mean+=delta/s->n;s->m2+=delta*(value-s->mean);
}
static double error(const sample *s) {return sqrt(s->m2/(s->n-1)/s->n);}
int main(int argc,char **argv) {
    if(argc!=2){fputs("usage: sim_monte_carlo sim.tsv\n",stderr);return 2;}
    size_t size;char *text=sim_read(argv[1],&size);dm_sim_data *d=calloc(1,sizeof *d);
    if(!text || !d)return 2;
    if(dm_sim_load(d,text,size,"rotw")){fprintf(stderr,"%s\n",d->err);return 2;}free(text);
    puts("class,family,critical,resistance,hp,chance,damage_samples,kills,predicted_dps,sampled_dps,dps_se,predicted_ttk,mean_ttk,ttk_se,ttk_half_width,dps_pass,ttk_pass");
    int failures=0,rows=0;

    const double confidence=5,precision=.02;
    for(int cls=0;cls<8;cls++)for(int variant=0;variant<13;variant++) {
        int fixed=variant==8,critical=variant<8?variant/4:variant==11;
        int res=variant<8?((variant/2)%2)*50:variant==12?50:0;
        int hp=variant<8?99+(variant%2)*900:variant==8?99:variant==10?151:variant==11?150:125;
        int chance=fixed?95:50;
        dm_sim_char c;dm_sim_char_init(&c,cls);c.clvl=50;c.life=1000;c.mana=1000;c.ar=fixed?100000:100;
        c.hand[0].kind=DM_WK_MELEE;c.hand[0].min=100;c.hand[0].max=fixed?100:150;
        c.hand[0].strbonus=c.hand[0].dexbonus=0;strcpy(c.hand[0].wclass,"1HS");
        c.hand[0].start_known=1;c.hand[0].start_frame=0;
        c.cs=c.ds=critical?50:0;
        dm_sim_target t={0};t.hp=hp;t.mlvl=50;t.ac=fixed?0:100;t.res[DM_EL_PHYS]=res;t.cycle=25;
        dm_sim_skill_out out;if(dm_sim_skill_eval(d,&c,&t,0,&out) || !out.roll.known || out.ttk<0)return 2;
        sample damage={0},kills={0};
        for(int i=0;i<200000;i++)observe(&damage,strike(critical,res,chance,fixed)/256.0*25/out.cooldown);
        for(int i=0;i<100000;i++) {
            int life=hp*256,attacks=0;
            while(life>=256 && attacks<100000){life-=strike(critical,res,chance,fixed);attacks++;}
            if(life>=256)return 2;
            observe(&kills,(out.action_frame+out.travel+(attacks-1)*out.cooldown)/25.0);
        }
        double dse=error(&damage),tse=error(&kills);
        int dps=fabs(damage.mean-out.dps)<=confidence*dse && confidence*dse<=precision*damage.mean;
        int ttk=fabs(kills.mean-out.ttk)<=confidence*tse && confidence*tse<=precision*kills.mean;
        failures+=!dps || !ttk;rows++;
        printf("%d,W1,%d,%d,%d,%d,%d,%d,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%d,%d\n",cls,critical,res,hp,chance,damage.n,kills.n,out.dps,damage.mean,dse,out.ttk,kills.mean,tse,confidence*tse,dps,ttk);
    }
    fprintf(stderr,"V7 direct attack: %d/%d pass; independent host sampling only\n",rows-failures,rows);
    dm_sim_free(d);free(d);return failures?1:0;
}
