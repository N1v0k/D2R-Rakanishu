#include "sim_test_io.h"
#include "../dm_sim_internal.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#define NEAR(a,b) assert(fabs((double)(a)-(double)(b))<1e-6)
static int id(const dm_sim_data *d,int i){assert(i>=0 && i<DM_SIM_MAXSKILL && d->skill[i]);return i;}
void sim_review(const dm_sim_data *);
static void references(dm_sim_data *d,int rotw) {
    dm_sim_char c;dm_sim_char_init(&c,1);int bad=0,fb=id(d,47);
    NEAR(dm_sim_skill_value(d,&c,fb,1,DM_SV_EMIN,&bad),6);NEAR(dm_sim_skill_value(d,&c,fb,1,DM_SV_EMAX,&bad),14);
    NEAR(dm_sim_skill_value(d,&c,fb,20,DM_SV_EMAX,&bad),226);assert(dm_sim_mana_cost(d->skill[fb],1)==5*256);
    int fan=id(d,122);c.lvl[fan]=3;NEAR(sim_calc(d,&c,fan,DM_CF_AURASTAT1,&bad),20);
    c.lvl[fan]=20;NEAR(sim_calc(d,&c,fan,DM_CF_AURASTAT1,&bad),35);
    assert(d->skill[id(d,12)]->srcdam==96);
    if(rotw)assert(d->skill[id(d,388)]->srcdam==116);
    int conv=id(d,123);c.lvl[conv]=40;NEAR(sim_calc(d,&c,conv,DM_CF_AURASTAT1+1,&bad),-150);assert(!bad);
    assert(d->diff[1].rp==-40 && d->diff[2].rp==-100);
    const int boss[]={156,211,242,243,544,526};
    const int hp[6][3]={{1024,24800,60031},{3995,55799,84524},{6036,74547,94320},{13818,90749,113812},{26484,117596,493701},{911,3218,13345}};
    for(int b=0;b<6;b++) {
        int mon=boss[b];assert(mon<d->nmon && d->mon[mon].id[0]);
        for(int df=0;df<3;df++){dm_sim_target t;assert(!dm_sim_target_mon(d,mon,df,1,DM_TK_ACTBOSS,1,&t));assert(t.hp==hp[b][df]);}
    }
    assert(d->std[2][0].mlvl==81);assert(d->std[2][0].hp==5357);
    int cast[8]={19,13,15,15,13,18,16,15};
    for(int cl=0;cl<8;cl++) {
        int found=0;
        for(int i=0;i<d->nanim;i++)if(!strcmp(d->anim[i].token,d->cls[cl].token) && !strcmp(d->anim[i].mode,"SC") && !strcmp(d->anim[i].wclass,"HTH")) {
            assert(dm_sim_frames_cast(d->anim[i].frames,d->anim[i].speed,0,0)==cast[cl]);found=1;
        }
        assert(found);
    }
    int bp[]={0,9,20,37,63,105,200};for(int i=0;i<7;i++)assert(dm_sim_frames_cast(14,256,bp[i],0)==13-i);
    int pal[]={0,9,18,30,48,75,125};for(int i=0;i<7;i++)assert(dm_sim_frames_cast(16,256,pal[i],0)==15-i);
    int light[]={0,7,15,23,35,52,78,117,194};for(int i=0;i<9;i++)assert(dm_sim_frames_cast(19,256,light[i],1)==19-i);
}
static void coverage(dm_sim_data *d,const char *manifest,int rotw) {
    FILE *f=fopen(manifest,"r");assert(f);char line[256];int seen=0,missing=0,levels[]={1,8,9,16,17,22,23,28,29};
    while(fgets(line,sizeof line,f)) {
        char *family=strchr(line,'\t');assert(family);*family++=0;char *status=strchr(family,'\t');assert(status);*status++=0;
        char *end;long sk=strtol(line,&end,10);assert(end!=line && !*end && sk>=0 && sk<DM_SIM_MAXSKILL);
        if(!d->skill[sk]){missing++;assert(!rotw);continue;}
        uint32_t fam;assert(!dm_sim_fam_parse(family,&fam));assert(d->skill[sk]->fam==fam);seen++;
        for(int l=0;l<9;l++) {
            dm_sim_char c;dm_sim_char_init(&c,d->skill[sk]->cls);c.clvl=80;c.diff=2;c.mana=10000;c.hand[0].kind=DM_WK_MELEE;c.hand[0].min=100;c.hand[0].max=150;
            c.shield=1;c.shield_min=20;c.shield_max=30;c.kick_min=20;c.kick_max=30;c.lvl[sk]=levels[l];c.blvl[sk]=levels[l]>20?20:levels[l];
            if(d->skill[sk]->hand[DM_HAND_B]){c.nhands=2;c.hand[1]=c.hand[0];c.shield=0;}
            c.bar[c.nbar++]=sk;dm_sim_target t;assert(!dm_sim_target_std(d,2,0,&t));dm_sim_skill_out o;
            int rc=dm_sim_skill_eval(d,&c,&t,sk,&o);
            if(!strncmp(status,"unavailable",11)){assert(!rc && o.flags&DM_SF_UNAVAIL);continue;}
            if(rc || o.flags&DM_SF_UNAVAIL)fprintf(stderr,"unavailable: %s level=%d flags=%d\n",line,levels[l],o.flags);
            assert(!rc);assert(!(o.flags&DM_SF_UNAVAIL));assert(isfinite(o.dps) && isfinite(o.ttk));
            if(!strncmp(status,"utility",7))assert(o.flags&DM_SF_UTILITY);
            if(!strncmp(status,"modifier",8))assert(o.flags&DM_SF_MODIFIER);
            if(!strncmp(status,"estimate",8))assert(o.flags&DM_SF_ESTIMATE);
        }
    }
    fclose(f);assert(seen==(rotw?240:210));assert(missing==(rotw?0:30));
    printf("%s: %d skills at 9 boundary levels; reference formulas, boss HP and cast frames passed\n",rotw?"rotw":"lod",seen);
}
static void reject_field(dm_sim_data *d,const char *text,size_t n,const char *type,const char *field,const char *value) {
    char prefix[64];snprintf(prefix,sizeof prefix,"cols\t%s\t",type);const char *header=strstr(text,prefix);assert(header);
    const char *p=header+strlen(prefix);int column=0;
    while(*p && *p!='\n') {
        const char *end=strpbrk(p,"\t\n");assert(end);
        if((size_t)(end-p)==strlen(field) && !memcmp(p,field,strlen(field)))break;
        column++;p=end+1;
    }
    assert(*p && *p!='\n');snprintf(prefix,sizeof prefix,"\n%s\t",type);
    const char *row=strstr(header,prefix);assert(row);p=row+strlen(prefix);
    for(int i=0;i<column;i++){p=strchr(p,'\t');assert(p);p++;}
    const char *end=strpbrk(p,"\t\n");assert(end);
    size_t length=n-(size_t)(end-p)+strlen(value);char *copy=malloc(length+1);assert(copy);
    memcpy(copy,text,(size_t)(p-text));memcpy(copy+(p-text),value,strlen(value));strcpy(copy+(p-text)+strlen(value),end);
    char *body=strchr(copy,'\n'),*tail=strstr(copy,"\nend\t");assert(body && tail);tail++;
    uint32_t crc=~0u;for(char *q=body+1;q<tail;q++){crc^=(unsigned char)*q;for(int k=0;k<8;k++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}
    char sum[9];snprintf(sum,sizeof sum,"%08x",crc^~0u);memcpy(body-8,sum,8);
    assert(dm_sim_load(d,copy,length,"rotw")<0);assert(!d->ok);free(copy);
}

int main(int argc,char **argv) {
    const char *path=getenv("DM_SIM_DATA");if(!path){fprintf(stderr,"DM_SIM_DATA is required\n");return 2;}
    size_t n;char *text=sim_read(path,&n);assert(text);dm_sim_data *d=calloc(1,sizeof *d);assert(d);
    for(int rotw=1;rotw>=0;rotw--){if(dm_sim_load(d,text,n,rotw?"rotw":"lod")){fprintf(stderr,"%s\n",d->err);return 1;}references(d,rotw);sim_review(d);coverage(d,argc>1?argv[1]:"sim_skills.manifest",rotw);dm_sim_free(d);}
    char version=text[7];text[7]='2';assert(dm_sim_load(d,text,n,"rotw")<0);text[7]=version;
    reject_field(d,text,n,"skill","variants","65536,-1,-1,-1");
    reject_field(d,text,n,"std","elpct","65536");
    reject_field(d,text,n,"skill","cls","65536");
    reject_field(d,text,n,"skill","HitShift","65544");
    reject_field(d,text,n,"skill","EType","256");
    reject_field(d,text,n,"skill","astats","65536,-1,-1,-1,-1,-1");
    reject_field(d,text,n,"anim","frames","65552");
    reject_field(d,text,n,"class","MinimumCastingDelay","65536");
    text[n/2]^=1;assert(dm_sim_load(d,text,n,"rotw")<0);assert(!d->ok);text[n/2]^=1;
    assert(dm_sim_load(d,text,n-20,"rotw")<0);assert(!d->ok);
    free(text);free(d);puts("sim reference: damaged payload and truncation rejected");return 0;
}
